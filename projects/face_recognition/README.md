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
| SD card | SDIO1, 4-line; the models are loaded from the card (`1:/tflite`) |

### Model Sources

Both models used here come from open-source projects:

| Model | Purpose | Upstream repository |
| --- | --- | --- |
| SCRFD | Face detection (320×320 input) | <https://github.com/deepinsight/insightface/tree/master/detection/scrfd> |
| GhostFaceNets | Face feature extraction (112×112 input, 512-float embedding) | <https://github.com/HamadYA/GhostFaceNets> |

Both are int8 quantised and compiled by Vela into Ethos-U-executable `.tflite` files, which the firmware loads from the `tflite` directory on the SD card (see "SD Card Preparation" below).

> **License note**: The model code of SCRFD and GhostFaceNets is released under the MIT License and may be used commercially; both models used in this project were trained on data collected in-house.

### Jumper Configuration

Before powering on, verify that the following jumpers are connected:

| Purpose | Jumper connections |
| --- | --- |
| BK7259 flashing and serial logs | `P11 ↔ RXD`, `P10 ↔ TXD` |

### Interface Usage

- Use the **USB-UART** interface for BK7259 flashing and serial logs.
- No key is bound in this project; the demo starts automatically after boot.

## SD Card Preparation

Place the face models in the `tflite` directory on the SD card:

| File | Purpose |
| --- | --- |
| `/tflite/face_detection_int8_vela.tflite` | SCRFD face detection model |
| `/tflite/face_verify_int8_vela.tflite` | GhostFaceNets face feature model |

The SD card is mounted at `/sd0` and also registered as FatFs volume `1:`, which is the path the firmware uses to load the models (`1:/tflite/...`). If the mount fails or a model is missing, the log shows `model init failed` and the demo stops.

## Boot Flow

1. `bk_init()` + media service init.
2. Frame buffer init.
3. Mount the SD card at `/sd0`.
4. Bring up VG-Lite (GPU init only; the flexa display path stays closed).
5. Create the `face_task` thread and start the demo.

## Demo Flow

1. **Meet the crew**: the three images go through detect → align → verify once, and their 512-float embeddings are stored in RAM as Alice, Bob and Jason.
2. **Recognition loop**: in every round the three images step up as visitors and are each compared against all three memorised faces to produce a "who is it" verdict.
3. Visitors are 300 ms apart; rounds are 5 seconds apart and re-introduce the robot.

## Log Output

The serial console prints the whole pipeline and its timings. Below is the actual serial output captured from the board at **round 73** (one complete round with all three visitors):

```
ap0:face_app:I(439365):------------------------------------------------
ap0:face_app:I(439365):hi! I am BK7259's face recognition robot
ap0:face_app:I(439365):my SCRFD eyes spot faces, my Arcface brain tells who you are
ap0:face_app:I(439365):round 73: knock knock, who is at the door?
ap0:face_app:I(439365):visitor 1 of 3 is coming up
ap0:face_app:I(439365):let me look at you with SCRFD first...
ap0:TFLM:I(439384):FaceRecognition: input normalize (BGRA -> RGB int8) took 8 ms
ap0:TFLM:I(439384):
ap0:TFLM:I(439395):U65 NPU | tensorflow-lite-micro detect invoke: 10 ms
ap0:TFLM:I(439395):
ap0:TFLM:I(439398):FaceDetect: post-process (decode + NMS) took 2 ms
ap0:TFLM:I(439398):
ap0:face_app:I(439398):SCRFD: face found at (127,117 36x48), confidence 85%, took 13 ms
ap0:TFLM:I(439399):FaceRecognition: align (VG-Lite 5-point warp) took 0 ms
ap0:TFLM:I(439399):
ap0:TFLM:I(439401):FaceVerify: input prepare (copy + normalize) took 1 ms
ap0:TFLM:I(439401):
ap0:TFLM:I(439422):U65 NPU | tensorflow-lite-micro verify invoke: 20 ms
ap0:TFLM:I(439422):
ap0:TFLM:I(439422):FaceVerify: post-process (embedding copy) took 0 ms
ap0:TFLM:I(439422):
ap0:face_app:I(439422):ok, I can see you. now Arcface, let me tell who you are: verify took 23 ms
ap0:face_app:I(439423):let me compare you against my 3 friends...
ap0:face_app:I(439423):   vs Alice: similarity 1.0000
ap0:face_app:I(439423):   vs Bob: similarity 0.0821
ap0:face_app:I(439423):   vs Jason: similarity 0.1522
ap0:face_app:I(439423):...done, comparing 3 faces took 0.41 ms
ap0:face_app:I(439423):aha! you are Alice, nice to see you again (similarity=1.0000)

ap0:face_app:I(439723):visitor 2 of 3 is coming up
ap0:face_app:I(439723):let me look at you with SCRFD first...
ap0:TFLM:I(439742):FaceRecognition: input normalize (BGRA -> RGB int8) took 8 ms
ap0:TFLM:I(439742):
ap1:TFLM:I(439753):U65 NPU | tensorflow-lite-micro detect invoke: 10 ms
ap1:TFLM:I(439753):
ap1:TFLM:I(439755):FaceDetect: post-process (decode + NMS) took 2 ms
ap1:TFLM:I(439755):
ap1:face_app:I(439755):SCRFD: face found at (74,50 55x78), confidence 85%, took 13 ms
ap0:TFLM:I(439756):FaceRecognition: align (VG-Lite 5-point warp) took 0 ms
ap0:TFLM:I(439756):
ap0:TFLM:I(439758):FaceVerify: input prepare (copy + normalize) took 1 ms
ap0:TFLM:I(439758):
ap0:TFLM:I(439779):U65 NPU | tensorflow-lite-micro verify invoke: 20 ms
ap0:TFLM:I(439779):
ap0:TFLM:I(439780):FaceVerify: post-process (embedding copy) took 0 ms
ap0:TFLM:I(439780):
ap0:face_app:I(439780):ok, I can see you. now Arcface, let me tell who you are: verify took 23 ms
ap0:face_app:I(439780):let me compare you against my 3 friends...
ap0:face_app:I(439780):   vs Alice: similarity 0.0821
ap0:face_app:I(439780):   vs Bob: similarity 1.0000
ap0:face_app:I(439780):   vs Jason: similarity 0.0649
ap0:face_app:I(439781):...done, comparing 3 faces took 0.41 ms
ap0:face_app:I(439781):aha! you are Bob, nice to see you again (similarity=1.0000)

ap0:face_app:I(440081):visitor 3 of 3 is coming up
ap0:face_app:I(440081):let me look at you with SCRFD first...
ap0:TFLM:I(440100):FaceRecognition: input normalize (BGRA -> RGB int8) took 8 ms
ap0:TFLM:I(440100):
ap0:TFLM:I(440112):U65 NPU | tensorflow-lite-micro detect invoke: 10 ms
ap0:TFLM:I(440112):
ap0:TFLM:I(440114):FaceDetect: post-process (decode + NMS) took 2 ms
ap0:TFLM:I(440114):
ap0:face_app:I(440114):SCRFD: face found at (115,69 33x48), confidence 83%, took 13 ms
ap0:TFLM:I(440115):FaceRecognition: align (VG-Lite 5-point warp) took 0 ms
ap0:TFLM:I(440115):
ap0:TFLM:I(440117):FaceVerify: input prepare (copy + normalize) took 1 ms
ap0:TFLM:I(440117):
ap0:TFLM:I(440138):U65 NPU | tensorflow-lite-micro verify invoke: 20 ms
ap0:TFLM:I(440138):
ap0:TFLM:I(440139):FaceVerify: post-process (embedding copy) took 0 ms
ap0:TFLM:I(440139):
ap0:face_app:I(440139):ok, I can see you. now Arcface, let me tell who you are: verify took 23 ms
ap0:face_app:I(440139):let me compare you against my 3 friends...
ap0:face_app:I(440139):   vs Alice: similarity 0.1522
ap0:face_app:I(440139):   vs Bob: similarity 0.0649
ap0:face_app:I(440140):   vs Jason: similarity 1.0000
ap0:face_app:I(440140):...done, comparing 3 faces took 0.44 ms
ap0:face_app:I(440140):aha! you are Jason, nice to see you again (similarity=1.0000)

ap0:face_app:I(440440):that is everybody for this round, let's play again
ap0:face_app:I(440440):
ap0:face_app:I(440440):---------------- round 73 done, next one in 5 seconds ----------------
ap0:face_app:I(440440):
```

Reading the log:

- The `ap0:TFLM:` prefix is TFLM's debug output (enabled by `FaceRecognitionModel::LogEnable(true)`). The `U65 NPU | tensorflow-lite-micro ... invoke` lines make it clear that the inference runs on the U65 NPU.
- Every stage prints its own millisecond timing, in true execution order: normalize → NPU detect → detect post-process → align → verify input prepare → NPU feature extract → verify post-process.
- The `ap0:face_app:` prefix marks the demo's own log: box position and confidence, the similarity against each of the three faces, and the final verdict.
- The highest similarity decides the match when it is at least 0.70; otherwise the log prints `stranger`.

## Key Source Files

| Path | Purpose |
| --- | --- |
| `ap/ap_main.cc` | Demo flow, SD-card mount, GPU bring-up, log output |
| `ap/resource/face3_image_input.cc` etc. | The three 320×320 BGRA images compiled into the firmware |
| `components/avdk_nn_module` | The `tflm_face_recog` component: model wrappers and face alignment |
| `components/multimedia_device_service/src/app_gpu.c` | VG-Lite bring-up and the GPU lock |

## Build

```bash
make bk7259
# or through the container build script
./dbuild.sh make bk7259 APP_VERSION=verify
```
