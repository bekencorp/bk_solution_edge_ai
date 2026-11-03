# multimedia_device_service is a shared source bucket in this release - it has no
# CMakeLists.txt of its own, so it is NOT a component. Compile the pieces we need
# straight into the app, exactly like scooter_dashboard_v1_1024_600_v2 does.
set(MULTIMEDIA_DEVICE_SERVICE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../../../components/multimedia_device_service
)

set(incs
    .
    resource
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/include
)

set(srcs
    ap_main.cc
    resource/face3_image_input.cc
    resource/face4_image_input.cc
    resource/face5_image_input.cc
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_camera.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_codec.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_gpu.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_display.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/app_h264_encode.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/doorbell_img_manager.c
    ${MULTIMEDIA_DEVICE_SERVICE_DIR}/src/encode_frame_que.c
)

set(priv_req
    include
    bk_common
    bk_vfs
    bk_peripheral
    bk_frame_buffer
    bk_tflite_micro
    avdk_utils
    avdk_driver
    avdk_nn_module
    media_service
    bk_gpu
    bk_camera
    bk_display
    bk_encoder
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# The multimedia_device_service sources are compiled in here (see above) instead
# of by their own component, so they inherit this component's flags. Several of
# their paths are CONFIG-gated, which leaves locals unused in this project's
# configuration - same reason scooter_dashboard_v1_1024_600_v2 sets this.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable)
