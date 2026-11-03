#pragma once

#include <stdint.h>

#include "AvdkDetectionModel.h"

constexpr int kFaceEmbeddingDim = 512;
constexpr float kFaceVerifySameThreshold = 0.7f;

struct FaceVerifyResult {
    bool valid;
    float norm;
    float normalized[kFaceEmbeddingDim];
};

typedef void (*faceVerifyResultCallbackT)(const FaceVerifyResult *result);
typedef void (*faceEnrollResultCallbackT)(const FaceVerifyResult *result,
                                          const uint8_t *aligned_rgb,
                                          uint32_t aligned_rgb_size);

/* Stage timings of one run(), in microseconds. The model switch cost is split
 * into prepare/release because both are dominated by MicroInterpreter
 * construction/deletion plus AllocateTensors, not by NPU execution. */
struct FaceRunStats {
    uint32_t input_norm_us;
    uint32_t detect_prepare_us;
    uint32_t detect_invoke_us;
    uint32_t detect_post_us;
    uint32_t align_us;
    uint32_t detect_release_us;
    uint32_t verify_prepare_us;
    uint32_t verify_input_us;
    uint32_t verify_invoke_us;
    uint32_t verify_post_us;
    uint32_t verify_release_us;
    uint32_t run_total_us;

    uint32_t detect_arena_used;
    uint32_t verify_arena_used;
    uint32_t shared_arena_size;
    uint32_t fast_scratch_size;
    uint32_t detect_model_size;
    uint32_t verify_model_size;

    int detect_count;
    bool verify_valid;
};

/* Minimal flatbuffer metadata used by the test report. */
struct FaceModelInfo {
    bool valid;
    uint32_t size;
    int input_dims[4];
    int input_dims_count;
};

float face_recognition_cosine_similarity(const float *a, const float *b, int dim);
bool face_recognition_is_same_person(const float *a, const float *b, int dim, float threshold);

class FaceDetectionRuntime;
class FaceVerifyRuntime;

class FaceRecognitionModel : public AvdkDetectionModel {
public:
    FaceRecognitionModel();
    ~FaceRecognitionModel() override;

    void setModelFilePath(const char *path);
    void setVerifyModelFilePath(const char *path);
    void setVerifyResultCallback(faceVerifyResultCallbackT cb);
    void setEnrollResultCallback(faceEnrollResultCallbackT cb);
    void setVerifyEnabled(bool enable);

    int init(void) override;
    int deinit(void) override;
    void resolverLoad(void) override;
    void resourceLoad(void) override;
    void resourceUnload(void) override;
    int run(uint8_t *data, uint32_t size, bk_pixel_format_t format) override;

    /* Stage timings of the most recent run(), in microseconds. Both are stored
     * before the enroll callback fires, so the callback can report them. */
    unsigned long long getLastDetectUs(void) const { return last_detect_us_; }
    unsigned long long getLastVerifyUs(void) const { return last_verify_us_; }

    /* Detailed per-run stage timings and memory water marks for the test report. */
    const FaceRunStats &getLastRunStats(void) const { return last_run_stats_; }
    const FaceModelInfo &getDetectModelInfo(void) const { return detect_model_info_; }
    const FaceModelInfo &getVerifyModelInfo(void) const { return verify_model_info_; }
    uint32_t getSharedArenaSize(void) const;
    uint32_t getFastScratchSize(void) const;

private:
    bool ensureDetectorInterpreter(void);
    void releaseDetectorInterpreter(void);

    FaceDetectionRuntime *detector_;
    FaceVerifyRuntime *verifier_;
    const char *verify_model_file_path_;
    faceVerifyResultCallbackT verify_result_callback_;
    faceEnrollResultCallbackT enroll_result_callback_;
    volatile bool verify_enabled_;
    bool runtimes_initialized_;
    unsigned long long last_detect_us_;
    unsigned long long last_verify_us_;

    FaceRunStats last_run_stats_;
    FaceModelInfo detect_model_info_;
    FaceModelInfo verify_model_info_;
};
