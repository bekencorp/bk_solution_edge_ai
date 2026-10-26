/*
 * scooter_dashboard_ai - face detect + verify demo driven by static frames.
 *
 * There is no camera. Three 320x320 BGRA frames (the SCRFD detection input
 * layout) are compiled into flash and pushed through the same
 * detect -> align -> verify pipeline the camera feed used to drive.
 *
 * Start-up reads all three faces once and keeps their 512-float fingerprints in
 * RAM (the enrolment pass). The loop that follows feeds the first frame back in
 * as an unknown visitor and works out which of the three known faces it is.
 */
/* os/os.h does NOT wrap its declarations in `extern "C"`, so it must be pulled
 * in here - BEFORE any other header - to keep C linkage. If another header
 * includes it first, the include guard is set with C++ linkage and APIs such as
 * rtos_create_thread get mangled, which fails at link time with undefined
 * references. */
extern "C" {
#include <os/os.h>
#include <os/mem.h>
}

#include "bk_private/bk_init.h"

#include <components/system.h>
#include <components/bk_frame_buffer.h>
#include <components/log.h>
#include <common/bk_err.h>
#include <driver/aon_rtc.h>
#include <stdint.h>
#include <string.h>

#include "media_service.h"
#include "app_gpu.h"
#include "FaceRecognitionModel.h"
#include "face_verify_image_input.h"

/* BK_LOG* takes a non-const char *tag, and C++ forbids binding a string
 * literal to char *, so keep the tag in a writable array. */
static char TAG[] = "face_app";
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Model files on the SD card. These match the names the tflm_face_recog
 * component defaults to, so keep them in sync with the card contents. */
#define FACE_MODEL_DETECT_PATH   "1:/tflite/face_detection_int8_vela.tflite"
#define FACE_MODEL_VERIFY_PATH   "1:/tflite/face_verify_int8_vela.tflite"

/* The embedded frames are SCRFD detection inputs: 320x320, 4 bytes per pixel
 * BGRA - exactly the layout FaceRecognitionModel::run() consumes. */
#define FACE_IMAGE_WIDTH         320
#define FACE_IMAGE_HEIGHT        320
#define FACE_IMAGE_BYTES         (FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT * 4)
#define FACE_IMAGE_COUNT         3

#define FACE_TASK_STACK_SIZE     (1024 * 8)

static const uint8_t *const s_face_images[FACE_IMAGE_COUNT] = {
    face3_image_input,
    face4_image_input,
    face5_image_input,
};

static const char *const s_face_names[FACE_IMAGE_COUNT] = {
    "Alice",
    "Bob",
    "Jason",
};

/* Frame currently in flight (0-based). */
static int s_frame_index = 0;

/* Set during the start-up pass, so the enroll callback knows whether to save
 * the fingerprint as a known face or to keep it as the visitor's. */
static bool s_enrolling = false;

/* The three faces read at start-up, and the fingerprint of whoever just walked
 * up in the loop. Both are filled in by the enroll callback. */
static float s_known_embedding[FACE_IMAGE_COUNT][kFaceEmbeddingDim];
static bool s_known_valid[FACE_IMAGE_COUNT];
static float s_probe_embedding[kFaceEmbeddingDim];
static bool s_probe_valid = false;

/* The enroll callback fires from inside run(), so the task needs a way to reach
 * the timings of the run that is in flight. */
static FaceRecognitionModel *s_model = NULL;

/* SD/FatFs mount contract, mirrored from ap/components/bk_vfs/bk_partition.h.
 * That header cannot be included from C++ (it nests named structs inside
 * anonymous unions), and bk_posix.h is unusable too because its
 * `mount`/`ftell`/`feof`/`rename` macros rewrite <cstdio>'s own declarations.
 * Keep these values in sync with the SDK headers. */
extern "C" {
int bk_vfs_mount(const char *source, const char *target, const char *fs_type,
                 unsigned long mount_flags, const void *data);
}

#define FACE_SD_MOUNT_PATH   "/sd0"      /* VFS_SD_0_PATITION_0 */
#define FACE_SD_FS_TYPE      "fatfs"     /* FS_TYPE_FATFS */
#define FACE_SD_DEV_NAME     "sdcard"    /* FATFS_DEV_SDCARD */
#define FACE_FATFS_DEVICE    0           /* FATFS_DEVICE */

/* Mirrors struct bk_fatfs_partition: its anonymous union's first member is
 * part_dev.device_name, so a plain pointer preserves the layout. */
struct face_fatfs_partition {
    int part_type;
    const char *mount_path;
    const char *device_name;
};

/* Mount the SD card as a VFS FatFs volume. The mount also registers it as the
 * bare FatFs volume "1:", which is the path the model loader uses. */
static int face_sd_mount(void)
{
    struct face_fatfs_partition partition = {0};

    partition.part_type = FACE_FATFS_DEVICE;
    partition.mount_path = FACE_SD_MOUNT_PATH;
    partition.device_name = FACE_SD_DEV_NAME;

    int ret = bk_vfs_mount("SOURCE_NONE", partition.mount_path,
                           FACE_SD_FS_TYPE, 0, &partition);
    if (ret != BK_OK)
    {
        LOGE("mount SD card failed, ret=%d\r\n", ret);
    }

    return ret;
}

/* The robot says hello: once at boot and again at the top of every round, so
 * the demo still introduces itself when the log is picked up mid-run. */
static void face_say_hello(void)
{
    LOGI("hi! I am BK7259's face recognition robot\r\n");
    LOGI("my SCRFD eyes spot faces, my Arcface brain tells who you are\r\n");
}

/* Fires from inside run(), right after the detection stage, so the detect
 * timing is already stamped into the model. */
static void face_box_callback(Box *boxes, int count)
{
    if (count <= 0 || boxes == nullptr)
    {
        LOGI("SCRFD: hmm, no face in this one\r\n");
        return;
    }

    LOGI("SCRFD: face found at (%d,%d %dx%d), confidence %.0f%%, took %llu ms\r\n",
         (int)boxes[0].x, (int)boxes[0].y,
         (int)boxes[0].w, (int)boxes[0].h,
         boxes[0].score * 100.0f,
         s_model->getLastDetectUs() / 1000ULL);
}

static void face_enroll_callback(const FaceVerifyResult *result,
                                 const uint8_t *aligned_rgb,
                                 uint32_t aligned_rgb_size)
{
    (void)aligned_rgb;
    (void)aligned_rgb_size;

    /* run() only aligns/verifies detections[0], so this fires exactly once per
     * frame - the buffer is released as soon as the callback returns. */
    if (result == nullptr || !result->valid)
    {
        s_probe_valid = false;
        return;
    }

    if (s_enrolling)
    {
        memcpy(s_known_embedding[s_frame_index], result->normalized,
               sizeof(float) * kFaceEmbeddingDim);
        s_known_valid[s_frame_index] = true;
        LOGI("   %s: fingerprint saved, %d floats living in RAM (norm=%.3f)\r\n",
             s_face_names[s_frame_index], kFaceEmbeddingDim, result->norm);
        return;
    }

    memcpy(s_probe_embedding, result->normalized,
           sizeof(float) * kFaceEmbeddingDim);
    s_probe_valid = true;
}

/* Score the visitor against everybody memorised so far and report the closest
 * one. Prints the per-face similarity so the verdict can be eyeballed. */
static int face_find_best_match(const float *probe, float *best_similarity,
                                unsigned long long *took_us)
{
    int best_index = -1;
    float best_score = -2.0f;
    unsigned long long start_us = bk_aon_rtc_get_us();

    for (int i = 0; i < FACE_IMAGE_COUNT; i++)
    {
        float score;

        if (!s_known_valid[i])
        {
            continue;
        }

        score = face_recognition_cosine_similarity(s_known_embedding[i], probe,
                                                  kFaceEmbeddingDim);
        LOGI("   vs %s: similarity %.4f\r\n", s_face_names[i], score);

        if (score > best_score)
        {
            best_score = score;
            best_index = i;
        }
    }

    *took_us = bk_aon_rtc_get_us() - start_us;
    if (best_similarity != NULL)
    {
        *best_similarity = best_score;
    }

    return best_index;
}

static void face_task(void *arg)
{
    (void)arg;

    /* TFLM's MicroPrintf() output only reaches the console once a debug-log
     * callback is registered - DebugLog() drops everything while the callback
     * is NULL. Without this the component's own error traces never show up. */
    FaceRecognitionModel::LogEnable(true);

    FaceRecognitionModel *model = new FaceRecognitionModel();
    if (model == nullptr)
    {
        LOGE("model alloc failed\r\n");
        return;
    }
    s_model = model;

    model->setModelFilePath(FACE_MODEL_DETECT_PATH);
    model->setVerifyModelFilePath(FACE_MODEL_VERIFY_PATH);
    model->setBoxDetectionCallback(face_box_callback);
    model->setEnrollResultCallback(face_enroll_callback);
    /* verify_enabled_ gates the enroll callback, so it must stay on. */
    model->setVerifyEnabled(true);

    if (model->init() != 0)
    {
        LOGE("model init failed, check %s and %s on the SD card\r\n",
             FACE_MODEL_DETECT_PATH, FACE_MODEL_VERIFY_PATH);
        return;
    }

    LOGI("\r\n");
    LOGI("================================================\r\n");
    face_say_hello();
    LOGI("detect input %ux%u, %d faces on the guest list\r\n",
         (unsigned)model->getWidth(), (unsigned)model->getHeight(),
         FACE_IMAGE_COUNT);
    LOGI("================================================\r\n");
    LOGI("\r\n");

    /* The align step blits with VG-Lite, so the source has to be a contiguous,
     * DMA-able buffer - copy each flash frame in before running it. */
    uint8_t *frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                                       FACE_IMAGE_BYTES);
    if (frame == nullptr)
    {
        LOGE("frame buffer alloc failed (%u bytes)\r\n", (unsigned)FACE_IMAGE_BYTES);
        return;
    }

    /* Meet the crew: read every face once and keep its fingerprint in RAM. */
    LOGI("first, let me meet the crew and remember who is who\r\n");
    s_enrolling = true;
    for (int i = 0; i < FACE_IMAGE_COUNT; i++)
    {
        s_frame_index = i;
        LOGI("reading %s...\r\n", s_face_names[i]);
        memcpy(frame, s_face_images[i], FACE_IMAGE_BYTES);
        model->run(frame, FACE_IMAGE_BYTES, BK_PIXEL_FORMAT_BGRA8888);
        rtos_delay_milliseconds(300);
    }
    s_enrolling = false;

    int known_count = 0;
    for (int i = 0; i < FACE_IMAGE_COUNT; i++)
    {
        if (s_known_valid[i])
        {
            known_count++;
        }
    }
    if (known_count == 0)
    {
        LOGE("nobody said hello, check the models on the SD card\r\n");
        return;
    }

    LOGI("%d faces in my pocket, now the game begins!\r\n", known_count);

    int round = 1;
    while (1)
    {
        LOGI("------------------------------------------------\r\n");
        face_say_hello();
        LOGI("round %d: knock knock, who is at the door?\r\n", round);

        /* Every visitor steps up in turn and is scored against all three
         * memorised faces, so Alice, Bob and Jason each get their own verdict. */
        for (int visitor = 0; visitor < FACE_IMAGE_COUNT; visitor++)
        {
            float best_score = 0.0f;
            unsigned long long compare_us = 0;
            int best;

            s_frame_index = visitor;
            s_probe_valid = false;

            LOGI("visitor %d of %d is coming up\r\n", visitor + 1, FACE_IMAGE_COUNT);
            LOGI("let me look at you with SCRFD first...\r\n");
            memcpy(frame, s_face_images[visitor], FACE_IMAGE_BYTES);
            model->run(frame, FACE_IMAGE_BYTES, BK_PIXEL_FORMAT_BGRA8888);

            if (!s_probe_valid)
            {
                LOGI("hmm, no clear face this time, next please\r\n");
                rtos_delay_milliseconds(300);
                continue;
            }

            LOGI("ok, I can see you. now Arcface, let me tell who you are: verify took %llu ms\r\n",
                 s_model->getLastVerifyUs() / 1000ULL);
            LOGI("let me compare you against my %d friends...\r\n", known_count);

            best = face_find_best_match(s_probe_embedding, &best_score, &compare_us);

            LOGI("...done, comparing %d faces took %.2f ms\r\n",
                 known_count, (float)compare_us / 1000.0f);

            if (best >= 0 && best_score >= kFaceVerifySameThreshold)
            {
                LOGI("aha! you are %s, nice to see you again (similarity=%.4f)\r\n",
                     s_face_names[best], best_score);
            }
            else
            {
                LOGI("hmm, I have never seen you before, stranger (best similarity=%.4f)\r\n",
                     best_score);
            }

            rtos_delay_milliseconds(300);
        }

        LOGI("that is everybody for this round, let's play again\r\n");
        LOGI("\r\n");
        LOGI("---------------- round %d done, next one in 5 seconds ----------------\r\n",
             round);
        LOGI("\r\n");
        round++;
        rtos_delay_milliseconds(5000);
    }
}

int main(void)
{
    bk_init();

    media_service_init();

#ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
#endif

    if (face_sd_mount() != BK_OK)
    {
        LOGE("SD mount failed, the face models can not be loaded\r\n");
    }

    /* The align step inside the verify path blits with VG-Lite directly and
     * only guards it with app_gpu_lock()/unlock(), which fail until the GPU has
     * been brought up. Only init here - the flexa strip pipeline must stay
     * closed so it never touches the frames this task feeds in. */
    if (app_gpu_vg_lite_turn_on(FACE_IMAGE_WIDTH, FACE_IMAGE_HEIGHT) != 0)
    {
        LOGE("GPU bring-up failed, face verify will not work\r\n");
    }

    beken_thread_t thread = NULL;
    bk_err_t ret = rtos_create_thread(&thread,
                                      BEKEN_DEFAULT_WORKER_PRIORITY,
                                      "face_task",
                                      (beken_thread_function_t)face_task,
                                      FACE_TASK_STACK_SIZE,
                                      NULL);
    if (ret != BK_OK)
    {
        LOGE("create face task failed ret=%d\r\n", ret);
    }

    return 0;
}
