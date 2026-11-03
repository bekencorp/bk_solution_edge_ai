# Face Recognition Demo (face_recognition_meminfo)

* [中文](./README_CN.md)

An enhanced version of the `face_recognition` demo. The recognition flow is identical (SCRFD detects faces, Arcface extracts face features, TensorFlow Lite Micro runs the inference on the U65 NPU); the addition is a **test report printed at the end of every round**, summarising model sizes, how the shared tensor arena / fast scratch are time-multiplexed, per-stage timings, the recognition result and a PASS/FAIL verdict. It is meant to be shown directly in a demo or used as a performance summary.

## Development Board Guide

This guide applies to Dashboard V1.0 (SCH-7259_V1.0) hardware and corresponds to the `face_recognition_meminfo` project.

> **Note**: The board requires a DC 12V power supply to boot normally.

### Project-Specific Configuration

| Setting | Project configuration |
| --- | --- |
| SoC | BK7259 |
| Inference framework | TensorFlow Lite Micro, running on the U65 NPU |
| Face detection | SCRFD, 320×320 BGRA input, int8 quantised |
| Face features | Arcface, 112×112 RGB input, 512-float embedding |
| Face alignment | VG-Lite GPU 5-point similarity transform to 112×112 |
| Face gallery | 3 people (Alice / Bob / Jason), embeddings kept in RAM |
| Match decision | Cosine similarity, threshold 0.70 |
| Image input | Three 320×320 images compiled into the firmware; no camera |
| Display | No LCD and no LVGL; every result goes to the serial log |
| SD card | SDIO1, 4-line; the models are loaded from the card (`1:/tflite`) |
| Test report | Printed once per round: model sizes, memory map, per-stage timings and the verdict |

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
| `/tflite/face_verify_int8_vela.tflite` | Arcface face feature model |

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
4. **Test report**: after all visitors in a round, the device prints `ROUND N TEST REPORT` with that round's models, memory map, timings and result, then waits 5 seconds before the next round.

## Observe the Log

The serial console prints the whole pipeline and its timings (excerpt):

```
================================================
hi! I am BK7259's face recognition robot
my SCRFD eyes spot faces, my Arcface brain tells who you are
detect input 320x320, 3 faces on the guest list
================================================
first, let me meet the crew and remember who is who
reading Alice...
ap0:TFLM:FaceRecognition: input normalize (BGRA -> RGB int8) took 7 ms
ap0:TFLM:U65 NPU | tensorflow-lite-micro detect invoke: 21 ms
ap0:TFLM:FaceDetect: post-process (decode + NMS) took 2 ms
ap0:face_app:SCRFD: face found at (127,117 36x48), confidence 85%, took 24 ms
ap0:TFLM:FaceRecognition: align (VG-Lite 5-point warp) took 1 ms
ap0:TFLM:FaceVerify: input prepare (copy + normalize) took 0 ms
ap0:TFLM:U65 NPU | tensorflow-lite-micro verify invoke: 35 ms
ap0:TFLM:FaceVerify: post-process (embedding copy) took 0 ms
ap0:face_app:   Alice: fingerprint saved, 512 floats living in RAM (norm=35.703)
...
3 faces in my pocket, now the game begins!
------------------------------------------------
hi! I am BK7259's face recognition robot
my SCRFD eyes spot faces, my Arcface brain tells who you are
round 1: knock knock, who is at the door?
visitor 1 of 3 is coming up
let me look at you with SCRFD first...
ap0:face_app:SCRFD: face found at (127,117 36x48), confidence 85%, took 24 ms
ap0:face_app:ok, I can see you. now Arcface, let me tell who you are: verify took 37 ms
ap0:face_app:let me compare you against my 3 friends...
ap0:face_app:   vs Alice: similarity 1.0000
ap0:face_app:   vs Bob: similarity 0.0821
ap0:face_app:   vs Jason: similarity 0.1522
ap0:face_app:...done, comparing 3 faces took 0.41 ms
ap0:face_app:aha! you are Alice, nice to see you again (similarity=1.0000)
that is everybody for this round, let's play again

---------------- round 1 done, next one in 5 seconds ----------------

================ ROUND 1 TEST REPORT ================
[System]
  chip        : BK7259 (Cortex-M55 + Ethos-U65)
  firmware    : face_recognition_meminfo  built Nov  3 2026 16:22:30
  round       : 1
[Models]
  SCRFD detect       727.5 KB  input 320x320x3
  ArcFace verify    2315.1 KB  input 112x112x3
[Memory map]
  detect model  :   727.5 KB  SD file -> PSRAM slab (uncoded)
  verify model  :  2315.1 KB  SD file -> PSRAM slab (uncoded)
  shared arena  :   750.0 KB  PSRAM slab (uncoded), one buffer reused by detect and verify
  fast scratch  :   256.0 KB  HSRAM, shared by both models
[Result]
  visitors 3, recognized 3, failures 0, detect avg 13.31 ms, verify avg 23.55 ms
  Alice    vs Alice 1.0000 | Bob   0.0821 | Jason 0.1522  -> Alice
  Bob      vs Alice 0.0821 | Bob   1.0000 | Jason 0.0649  -> Bob
  Jason    vs Alice 0.1522 | Bob   0.0649 | Jason 1.0000  -> Jason
  VERDICT     : PASS
CSV,round=1,detect_us=13312,verify_us=23552,acc=3/3,pass=1
=======================================================
```

Reading the log:

- The `ap0:TFLM:` prefix is TFLM's debug output (enabled by `FaceRecognitionModel::LogEnable(true)`). The `U65 NPU | tensorflow-lite-micro ... invoke` lines make it clear that the inference runs on the U65 NPU.
- Every stage prints its own millisecond timing, in true execution order: normalize → NPU detect → detect post-process → align → verify input prepare → NPU feature extract → verify post-process.
- The `ap0:face_app:` prefix marks the demo's own log: box position and confidence, embedding norm, the similarity against each of the three faces, and the final verdict.
- The highest similarity decides the match when it is at least 0.70; otherwise the log prints `stranger`.
- The test report has four sections: `[System]` runtime context, `[Models]` model size and input shape, `[Memory map]` where the models live and how the shared arena / fast scratch are multiplexed between them, and `[Result]` the round's recognition outcome and verdict.
- `detect avg` is the average cost of NPU detection plus post-processing; `verify avg` covers verify model preparation, input normalisation, NPU inference and embedding post-processing.
- The final CSV line can be imported directly into a spreadsheet to trend multiple rounds.

## Key Source Files

| Path | Purpose |
| --- | --- |
| `ap/ap_main.cc` | Demo flow, SD-card mount, GPU bring-up, per-round test report |
| `ap/resource/face3_image_input.cc` etc. | The three 320×320 BGRA images compiled into the firmware |
| `components/avdk_nn_module` | The `tflm_face_recog` component: model wrappers and face alignment |
| `components/avdk_nn_module/src/tflm_face_recog/FaceRecognitionModel.cc` | Collects model metadata and per-stage timings for the report |
| `components/multimedia_device_service/src/app_gpu.c` | VG-Lite bring-up and the GPU lock |

## Build

```bash
make bk7259
# or through the container build script
./dbuild.sh make bk7259 APP_VERSION=verify
```
