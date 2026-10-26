set(incs
    include
    ../../tflite_micro/gesture_detection/ap/ui/beken_generated
    # multimedia_device_service has no CMakeLists.txt in this release, so it is
    # not a component and cannot be listed in PRIV_REQUIRES. Its headers are
    # pulled in directly instead (app_camera.h / app_display.h / app_gpu.h); the
    # matching .c files are compiled by the project that uses this component.
    ${CMAKE_CURRENT_LIST_DIR}/../multimedia_device_service/include
)
set(srcs )

# The core model classes are TFLM-only: AvdkDetectionModel.h pulls in
# tensorflow/lite/... headers and the per-model sources below all derive from it.
# They must not be compiled when TFLITE_MICRO is off, because bk_tflite_micro
# only publishes its include dirs when that option is on - otherwise any project
# that scans components/ (v1_1024_600_v2, v1_camera_display) fails with
# "tensorflow/lite/c/common.h: No such file or directory". The TFLM_* options in
# Kconfig already depend on TFLITE_MICRO, so this gate matches their intent.
if (CONFIG_TFLITE_MICRO)
	list(APPEND srcs
		src/core/box.c
		src/core/AvdkDetectionModel.cc
		src/core/AvdkMultiModel.cc
		src/core/AvdkVideoReatorOSD.cc
	)
endif()

if (CONFIG_TFLM_YOLOFACE_V1)
	list(APPEND srcs
		src/tflm_yoloface_detection/YolofaceDetectionModel.cc
	)
	if (NOT CONFIG_SDCARD)
		list(APPEND srcs
			src/tflm_yoloface_detection/yoloface_detect_model_data.cc
		)
	endif()
endif()

if (CONFIG_TFLM_PALM_DETECTION_V1)
	list(APPEND srcs
		src/tflm_palm_detection/PalmDetectionModel.cc
	)
	if (NOT CONFIG_SDCARD)
		list(APPEND srcs
			src/tflm_palm_detection/palm_detect_model_data.cc
		)
	endif()
endif()

if (CONFIG_TFLM_HAND_GESTURE_DETECTION_V1)
	list(APPEND srcs
		src/tflm_hand_gesture_detection/HandGestureDetectionModel.cc
	)
	if (NOT CONFIG_SDCARD)
		list(APPEND srcs
			src/tflm_hand_gesture_detection/hand_gesture_detect_model_data.cc
		)
	endif()
endif()

if (CONFIG_TFLM_FACE_RECOG_V1)
	list(APPEND srcs
		src/tflm_face_recog/FaceRecognitionModel.cc
		src/tflm_face_recog/FaceDetectionRuntime.cc
		src/tflm_face_recog/FaceVerifyRuntime.cc
	)
	if (NOT CONFIG_SDCARD)
		list(APPEND srcs
			src/tflm_face_recog/face_detect_model_data.cc
			src/tflm_face_recog/face_verify_model_data.cc
		)
	endif()
endif()

set(priv_req bk_common unity include avdk_utils bk_tflite_micro bk_peripheral)

if (CONFIG_TFLM_FACE_RECOG_V1)
	list(APPEND priv_req bk_gpu media_service avdk_driver)
endif()

if (CONFIG_SDCARD)
	list(APPEND priv_req fatfs)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

add_definitions(
	-DTF_LITE_STATIC_MEMORY
	-DTF_LITE_DISABLE_X86_NEON
	-DTF_LITE_USE_CTIME
	-DTF_LITE_USE_GLOBAL_CMATH_FUNCTIONS
	-DCMSIS_NN
	-DKERNELS_OPTIMIZED_FOR_SPEED
)
