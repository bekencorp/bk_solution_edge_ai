# MobileNetV1 benchmark project: the model is linked straight into the app, so
# only the benchmark entry point and the model blob are compiled here. The
# multimedia_device_service sources the face demo needed are gone.
set(incs
    .
    resource
)

set(srcs
    ap_main.cc
    resource/mobilenet_v1_model_data.cc
)

set(priv_req
    include
    bk_common
    bk_peripheral
    bk_frame_buffer
    bk_tflite_micro
    avdk_utils
    avdk_driver
    avdk_nn_module
    media_service
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
