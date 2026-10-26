include(${CMAKE_CURRENT_LIST_DIR}/beken_generated/beken_generated.cmake)

set(MULTIMEDIA_DEVICE_SERVICE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../../../components/multimedia_device_service
)

set(incs .
    dashcam
    ${BEKEN_GENERATOR_INCLUDE_DIRS}
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/include
)

set(srcs
    ap_main.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_camera.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_codec.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_gpu.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_display.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_h264_encode.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/doorbell_img_manager.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/encode_frame_que.c
    ${BEKEN_GENERATOR_SOURCES}
    )

set(priv_req  bk_init bk_common media_service bk_cli bk_display bk_peripheral avdk_utils lvgl
              bk_network_provisioning easy_flash bk_vfs
              bluetooth_dual bluetooth_secondary_controller
              bk_network_transfer media network_provisioning key_app bk_gpu display_ui
              decode_stream_pipeline fatfs bk_bluetooth avdk_driver bk_camera
              bk_video_recorder bk_video_player bk_encoder app boot_avi_play
              dashcam)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -O3)

# Workaround: SDK Kconfig missing LV_USE_GIF/LV_USE_FS_FATFS entries.
# Use LV_CONF_PATH to override lv_conf.h defaults. Remove when SDK is fixed.
set(LV_CONF_OVERRIDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf_override.h")
target_compile_definitions(__armino_lvgl PUBLIC "LV_CONF_PATH=\"${LV_CONF_OVERRIDE_PATH}\"")

# lv_conf_override.h points LV_MEM_POOL_ALLOC at display_ui_lv_pool_alloc(), whose
# header lives in the solution. lv_mem_core_builtin.c is SDK code and has no way to
# reach it - the lvgl component's include path is its own - so add it here rather
# than in the SDK, which keeps the pool policy on the product side.
target_include_directories(__armino_lvgl PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../components/display_ui/include")
