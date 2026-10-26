#include "bk_private/bk_init.h"
#include <os/os.h>
#include "cli.h"
#include "media_service.h"
#include <components/bk_frame_buffer.h>

#if CONFIG_BUTTON
#include "key_app_service.h"
#endif

#include "media_devices.h"
#include "display_ui.h"
#include "boot_avi_play.h"
#include "boot_bg_preload.h"
#include "dashcam_config.h"
#include "dashcam_storage.h"
#include "dashcam_app.h"
#include "sdkconfig.h"
#include "dashcam_assitview.h"
#include "avdk_monitor.h"

#define TAG "scooter_dashboard_v1_1024_600_v2"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/* ==================== Stub functions for unimplemented features ==================== */
__attribute__((weak)) int bk_rand(void) { return 0; }

/*
 * display_ui_cast_hooks.c takes this symbol as a strong extern (the UI build
 * provided it via beken_ui.c). This camera-only project links without the
 * page tree, so supply the empty definition here.
 */
void beken_ui_kick_after_display_resume(void)
{
}

/* ==================== CLI ==================== */
void cli_widgets_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);

    if ((argc >= 2) && (os_strcmp(argv[1], "dashcam_count") == 0))
    {
        uint32_t count = 0;
        uint32_t free_mb = 0;
        bk_err_t count_ret = dashcam_storage_count(&count);
        bk_err_t free_ret = dashcam_storage_free_mb(&free_mb);

        LOGI("dashcam_count: ret=%d dir=%s count=%u limit=%u release=%u allowed=%u free_ret=%d free_mb=%u\n",
             count_ret,
             DASHCAM_STORAGE_DIR,
             (unsigned)count,
             (unsigned)DASHCAM_DEV_MAX_FILES,
             (unsigned)DASHCAM_RELEASE_BUILD,
             (unsigned)((count_ret == BK_OK) &&
                        (DASHCAM_RELEASE_BUILD || count < DASHCAM_DEV_MAX_FILES)),
             free_ret,
             (unsigned)free_mb);
    }
    else if ((argc >= 2) && (os_strcmp(argv[1], "dashcam_trim") == 0))
    {
        uint32_t deleted = 0;
        uint32_t failed = 0;
        bk_err_t ret;

        /* Finalize and close the active MP4 before removing its directory. */
        dashcam_app_record_stop();
        ret = dashcam_storage_delete_directory(&deleted, &failed);
        LOGI("dashcam_trim: stop+delete directory ret=%d deleted=%u failed=%u\n",
             ret, (unsigned)deleted, (unsigned)failed);
    }
    else
    {
        LOGI("usage: dashboard dashcam_count | dashcam_trim\n");
    }
}

static const struct cli_command s_widgets_commands[] =
{
    {"dashboard", "dashboard <command>", cli_widgets_cmd},
};

#define CMDS_COUNT  (sizeof(s_widgets_commands) / sizeof(struct cli_command))

bk_err_t cli_widgets_init(void)
{
    return cli_register_commands(s_widgets_commands, CMDS_COUNT);
}

/* ==================== Modular Init Functions ==================== */

static void app_board_init(void)
{
    LOGI("initializing frame buffer\n");
    bk_frame_buffer_init();
}

static void app_ui_init(void)
{
    /*
     * Camera-only build: the LVGL page tree (home/dashcam/ota/phone book/music
     * player/notification popup) is removed from this project. display_ui still
     * requires a registered init callback to rebuild LVGL after cast / video
     * playback / assist-view teardown, so keep an empty one.
     */
}

static void app_display_init(void)
{
    if (display_ui_register_init_callback(app_ui_init) != BK_OK)
    {
        LOGE("register UI init callback failed\n");
        return;
    }

#if defined(CONFIG_SCOOTER_LVGL_HOR_RES) && defined(CONFIG_SCOOTER_UI_CANVAS_WIDTH)
    LOGI("profile 1024_600: LVGL %dx%d rot=%d canvas %dx%d\n",
         CONFIG_SCOOTER_LVGL_HOR_RES, CONFIG_SCOOTER_LVGL_VER_RES,
#if defined(CONFIG_SCOOTER_UI_ROTATION)
         CONFIG_SCOOTER_UI_ROTATION,
#else
         -1,
#endif
         CONFIG_SCOOTER_UI_CANVAS_WIDTH, CONFIG_SCOOTER_UI_CANVAS_HEIGHT);
#endif

    LOGI("cast recovery: dpu_layer_config + display open after cast\n");

    if (display_ui_init_display_hw() != BK_OK) {
        LOGE("display hw init failed\n");
    } else {
        /* Decode the SD-card background while the boot animation is running.
         * Full-screen RGB565A8 designer assets are intentionally not linked
         * because five raw 1024x600 backgrounds exceed the AP flash region. */
        boot_bg_preload_start();

        boot_avi_play();

        boot_bg_preload_finish();
    }

    display_ui_register_cast_hooks_once();
    av_server_devices_init();
}

#define APP_EVENT_QUEUE_COUNT  16
#define APP_EVENT_STACK_SIZE   (8 * 1024)
#define APP_EVENT_PRIORITY     4

typedef enum
{
    APP_EVENT_KEY_UP = 0,
    APP_EVENT_KEY_DOWN,
    APP_EVENT_KEY_LEFT,
    APP_EVENT_KEY_RIGHT,
    APP_EVENT_KEY_ENTER,
} app_event_t;

static beken_queue_t s_app_event_queue = NULL;
static beken_thread_t s_app_event_thread = NULL;

static void app_event_thread(void *param)
{
    app_event_t event;

    (void)param;

    while (1)
    {
        if (rtos_pop_from_queue(&s_app_event_queue,
                                &event,
                                BEKEN_WAIT_FOREVER) != BK_OK)
        {
            continue;
        }
        LOGI("app event: %d\n", (int)event);
    }
}

static void app_event_post(app_event_t event)
{
    if (s_app_event_queue == NULL)
    {
        LOGE("app event queue is not initialized\n");
        return;
    }

    if (rtos_push_to_queue(&s_app_event_queue, &event, BEKEN_NO_WAIT) != BK_OK)
    {
        LOGW("app event queue full, drop event: %d\n", (int)event);
    }
}

#if CONFIG_BUTTON
#define APP_EVENT_CALLBACK(name, event_value) \
    static void name(void) \
    { \
        app_event_post(event_value); \
    }

APP_EVENT_CALLBACK(app_event_key_up, APP_EVENT_KEY_UP)
APP_EVENT_CALLBACK(app_event_key_down, APP_EVENT_KEY_DOWN)
APP_EVENT_CALLBACK(app_event_key_left, APP_EVENT_KEY_LEFT)
APP_EVENT_CALLBACK(app_event_key_right, APP_EVENT_KEY_RIGHT)
APP_EVENT_CALLBACK(app_event_key_enter, APP_EVENT_KEY_ENTER)

#undef APP_EVENT_CALLBACK
#endif

static bk_err_t app_event_init(void)
{
    bk_err_t ret;

    ret = rtos_init_queue(&s_app_event_queue,
                          "app_event",
                          sizeof(app_event_t),
                          APP_EVENT_QUEUE_COUNT);
    if (ret != BK_OK)
    {
        LOGE("init app event queue failed: %d\n", ret);
        return ret;
    }

    ret = rtos_create_thread(&s_app_event_thread,
                             APP_EVENT_PRIORITY,
                             "app_event",
                             app_event_thread,
                             APP_EVENT_STACK_SIZE,
                             NULL);
    if (ret != BK_OK)
    {
        LOGE("create app event thread failed: %d\n", ret);
        rtos_deinit_queue(&s_app_event_queue);
        return ret;
    }

    return BK_OK;
}

static void app_key_init(void)
{
#if CONFIG_BUTTON
    /*
     * Short presses post KEY_* events to the app-event worker, which currently
     * only logs them. Double/long actions are unbound (handled by the CP side).
     */
    static const key_action_cfg_t s_key_actions[] =
    {
        { .pin_id = KEY_PIN_UP,     .short_callback = app_event_key_up,    .double_callback = NULL,              .long_callback = NULL },
        { .pin_id = KEY_PIN_DOWN,   .short_callback = app_event_key_down,  .double_callback = NULL,              .long_callback = NULL },
        { .pin_id = KEY_PIN_LEFT,   .short_callback = app_event_key_left,  .double_callback = NULL,              .long_callback = NULL },
        { .pin_id = KEY_PIN_RIGHT,  .short_callback = app_event_key_right, .double_callback = NULL,              .long_callback = NULL },
        { .pin_id = KEY_PIN_MIDDLE, .short_callback = app_event_key_enter, .double_callback = NULL,              .long_callback = NULL },
    };
    bk_key_service_init(s_key_actions,
                        sizeof(s_key_actions) / sizeof(s_key_actions[0]));
#endif
}

/* ==================== Main ==================== */

int main(void)
{
    bk_init();

    LOGI("ap is initializing\n");

    media_service_init();

    app_board_init();

    avdk_monitor_init();
    avdk_monitor_start();

    app_display_init();
    if (app_event_init() == BK_OK)
    {
        app_key_init();
    }
    cli_widgets_init();

    dashcam_assitview_init();
    dashcam_assitview_direct_start();

    return 0;
}
