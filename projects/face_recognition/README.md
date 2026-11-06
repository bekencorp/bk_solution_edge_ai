# Face Recognition Demo (face_recognition)

* [中文](./README_CN.md)

A face recognition project for demo use: SCRFD detects faces, GhostFaceNets extracts face features, and TensorFlow Lite Micro runs the inference on the U65 NPU. The input is three static images compiled into the firmware, and the device tells which of Alice, Bob, or Jason each one is.

## Development Board Guide

This guide applies to Dashboard V1.0 (SCH-7259_V1.0) hardware and corresponds to the `face_recognition` project.

> **Note**: The board requires a DC 12V power supply to boot normally.

### Project-Specific Configuration

| Setting | Project configuration |
| --- | --- |
| SoC | BK7259 |
| Inference framework | TensorFlow Lite Micro, running on the U65 NPU |
| Face detection | SCRFD, 320×320 BGRA input, int8 quantised |
| Face features | GhostFaceNets, 112×112 RGB input, 512-float embedding |
| Face alignment | VG-Lite GPU 5-point affine warp to 112×112 |
| Face gallery | 3 people (Alice / Bob / Jason), embeddings kept in RAM |
| Match decision | Cosine similarity, threshold 0.70 |
| Image input | Three 320×320 images compiled into the firmware; no camera |
| Display | No LCD and no LVGL; every result goes to the serial log |
| External storage | Not required; both models are compiled into the firmware |

### Model Sources

Both models used here come from open-source projects:

| Model | Purpose | Upstream repository |
| --- | --- | --- |
| SCRFD | Face detection (320×320 input) | <https://github.com/deepinsight/insightface/tree/master/detection/scrfd> |
| GhostFaceNets | Face feature extraction (112×112 input, 512-float embedding) | <https://github.com/HamadYA/GhostFaceNets> |

Both are int8 quantised and compiled by Vela into Ethos-U-executable `.tflite` files, then turned into C arrays and linked into the firmware (see "Models Built into Firmware" below); at boot they are copied to PSRAM before use.

> **License note**: The model code of SCRFD and GhostFaceNets is released under the MIT License and may be used commercially; both models used in this project were trained on data collected in-house.

### Jumper Configuration

Before powering on, verify that the following jumpers are connected:

| Purpose | Jumper connections |
| --- | --- |
| BK7259 flashing and serial logs | `P11 ↔ RXD`, `P10 ↔ TXD` |

### Interface Usage

- Use the **USB-UART** interface for BK7259 flashing and serial logs.
- No key is bound in this project; the demo starts automatically after boot.

## Models Built into Firmware

Both models are compiled into the firmware as C arrays, so the runtime has no SD-card dependency:

| File | Purpose |
| --- | --- |
| `components/avdk_nn_module/src/tflm_face_recog/face_detect_model_data.cc` | SCRFD face detection model (666,240 B) |
| `components/avdk_nn_module/src/tflm_face_recog/face_verify_model_data.cc` | GhostFaceNets face feature model (2,370,704 B) |

With `CONFIG_SDCARD` disabled, the `tflm_face_recog` component builds these two arrays automatically and copies the models to a PSRAM slab (uncoded) at boot for the NPU. Together they take about 2.9 MB, so `primary_ap_app` has been enlarged from 4148K to 5120K; the partition table must be reflashed as well.

## Boot Flow

1. `bk_init()` + media service init.
2. Frame buffer init.
3. Bring up VG-Lite (GPU init only; the flexa display path stays closed).
4. Create the `face_task` thread and start the demo.

## Demo Flow

1. **Meet the crew**: the three images go through detect → align → verify once, and their 512-float embeddings are stored in RAM as Alice, Bob and Jason.
2. **Recognition loop**: in every round the three images step up as visitors and are each compared against all three memorised faces to produce a "who is it" verdict.
3. Visitors are 300 ms apart; rounds are 5 seconds apart and re-introduce the robot.

## Log Output

The serial console prints the whole pipeline and its timings. Below is the actual serial output captured from the board at **round 4** (one complete round with all three visitors):

```
ap0:face_app:I(19346):------------------------------------------------
ap0:face_app:I(19346):hi! I am BK7259's face recognition robot
ap0:face_app:I(19346):my SCRFD eyes spot faces, my GhostFaceNets brain tells who you are
ap0:face_app:I(19346):round 4: knock knock, who is at the door?
ap0:face_app:I(19346):visitor 1 of 3 is coming up
ap0:face_app:I(19346):let me look at you with SCRFD first...
ap0:TFLM:I(19365):FaceRecognition: input normalize (BGRA -> RGB int8) took 7 ms
ap0:TFLM:I(19365):
ap0:TFLM:I(19375):U65 NPU | tensorflow-lite-micro detect invoke: 9 ms
ap0:TFLM:I(19375):
ap0:TFLM:I(19377):FaceDetect: post-process (decode + NMS) took 2 ms
ap0:TFLM:I(19378):
ap0:face_app:I(19378):SCRFD: face found at (127,117 36x48), confidence 85%, took 11 ms
ap0:TFLM:I(19379):FaceRecognition: align (VG-Lite 5-point warp) took 0 ms
ap0:TFLM:I(19379):
ap0:TFLM:I(19381):FaceVerify: input prepare (copy + normalize) took 1 ms
ap0:TFLM:I(19381):
ap0:TFLM:I(19391):U65 NPU | tensorflow-lite-micro verify invoke: 10 ms
ap0:TFLM:I(19391):
ap0:TFLM:I(19391):FaceVerify: post-process (embedding copy) took 0 ms
ap0:TFLM:I(19392):
ap0:face_app:I(19392):ok, I can see you. now GhostFaceNets, let me tell who you are: verify took 12 ms
ap0:face_app:I(19392):let me compare you against my 3 friends...
ap0:face_app:I(19392):   vs Alice: similarity 1.0000
ap0:face_app:I(19392):   vs Bob: similarity 0.0821
ap0:face_app:I(19392):   vs Jason: similarity 0.1522
ap0:face_app:I(19392):...done, comparing 3 faces took 0.41 ms
ap0:face_app:I(19392):aha! you are Alice, nice to see you again (similarity=1.0000)
ap0:face_app:I(19693):visitor 2 of 3 is coming up
ap0:face_app:I(19693):let me look at you with SCRFD first...
ap0:TFLM:I(19712):FaceRecognition: input normalize (BGRA -> RGB int8) took 7 ms
ap0:TFLM:I(19712):
ap0:TFLM:I(19722):U65 NPU | tensorflow-lite-micro detect invoke: 9 ms
ap0:TFLM:I(19722):
ap0:TFLM:I(19724):FaceDetect: post-process (decode + NMS) took 2 ms
ap0:TFLM:I(19724):
ap0:face_app:I(19724):SCRFD: face found at (74,50 55x78), confidence 85%, took 11 ms
ap0:TFLM:I(19725):FaceRecognition: align (VG-Lite 5-point warp) took 0 ms
ap0:TFLM:I(19725):
ap0:TFLM:I(19727):FaceVerify: input prepare (copy + normalize) took 1 ms
ap0:TFLM:I(19727):
ap0:TFLM:I(19738):U65 NPU | tensorflow-lite-micro verify invoke: 10 ms
ap0:TFLM:I(19738):
ap0:TFLM:I(19738):FaceVerify: post-process (embedding copy) took 0 ms
ap0:TFLM:I(19738):
ap0:face_app:I(19739):ok, I can see you. now GhostFaceNets, let me tell who you are: verify took 12 ms
ap0:face_app:I(19739):let me compare you against my 3 friends...
ap0:face_app:I(19739):   vs Alice: similarity 0.0821
ap0:face_app:I(19739):   vs Bob: similarity 1.0000
ap0:face_app:I(19739):   vs Jason: similarity 0.0649
ap0:face_app:I(19739):...done, comparing 3 faces took 0.44 ms
ap0:face_app:I(19739):aha! you are Bob, nice to see you again (similarity=1.0000)
ap0:face_app:I(20039):visitor 3 of 3 is coming up
ap0:face_app:I(20039):let me look at you with SCRFD first...
cp0:cal:I(20191):temp_code:34.00->35.00 - adc_code:6408 - adc_trend:[14->15]
ap0:TFLM:I(20058):FaceRecognition: input normalize (BGRA -> RGB int8) took 7 ms
ap0:TFLM:I(20058):
ap0:TFLM:I(20068):U65 NPU | tensorflow-lite-micro detect invoke: 9 ms
ap0:TFLM:I(20068):
ap0:TFLM:I(20070):FaceDetect: post-process (decode + NMS) took 2 ms
ap0:TFLM:I(20071):
ap0:face_app:I(20071):SCRFD: face found at (115,69 33x48), confidence 83%, took 12 ms
ap0:TFLM:I(20072):FaceRecognition: align (VG-Lite 5-point warp) took 0 ms
ap0:TFLM:I(20072):
ap0:TFLM:I(20074):FaceVerify: input prepare (copy + normalize) took 1 ms
ap0:TFLM:I(20074):
ap0:TFLM:I(20084):U65 NPU | tensorflow-lite-micro verify invoke: 10 ms
ap0:TFLM:I(20084):
ap0:TFLM:I(20085):FaceVerify: post-process (embedding copy) took 0 ms
ap0:TFLM:I(20085):
ap0:face_app:I(20085):ok, I can see you. now GhostFaceNets, let me tell who you are: verify took 12 ms
ap0:face_app:I(20085):let me compare you against my 3 friends...
ap0:face_app:I(20085):   vs Alice: similarity 0.1522
ap0:face_app:I(20085):   vs Bob: similarity 0.0649
ap0:face_app:I(20085):   vs Jason: similarity 1.0000
ap0:face_app:I(20085):...done, comparing 3 faces took 0.41 ms
ap0:face_app:I(20086):aha! you are Jason, nice to see you again (similarity=1.0000)
ap0:face_app:I(20386):that is everybody for this round, let's play again
ap0:face_app:I(20386):
ap0:face_app:I(20386):---------------- round 4 done, next one in 5 seconds ----------------
```

Reading the log:

- The `ap0:TFLM:` prefix is TFLM's debug output (enabled by `FaceRecognitionModel::LogEnable(true)`). The `U65 NPU | tensorflow-lite-micro ... invoke` lines make it clear that the inference runs on the U65 NPU.
- Every stage prints its own millisecond timing, in true execution order: normalize → NPU detect → detect post-process → align → verify input prepare → NPU feature extract → verify post-process.
- The `ap0:face_app:` prefix marks the demo's own log: box position and confidence, the similarity against each of the three faces, and the final verdict.
- The highest similarity decides the match when it is at least 0.70; otherwise the log prints `stranger`.

## Key Source Files

| Path | Purpose |
| --- | --- |
| `ap/ap_main.cc` | Demo flow, GPU bring-up, log output |
| `ap/resource/face3_image_input.cc` etc. | The three 320×320 BGRA images compiled into the firmware |
| `components/avdk_nn_module` | The `tflm_face_recog` component: model wrappers and face alignment |
| `components/multimedia_device_service/src/app_gpu.c` | VG-Lite bring-up and the GPU lock |

## Build

```bash
make bk7259
# or through the container build script
./dbuild.sh make bk7259 APP_VERSION=verify
```
