# MobileNetV1 NPU Benchmark (face_recognition_meminfo)

* [中文](./README_CN.md)

This project started as a face-recognition demo. It now follows `bk_avdk_smp_release_4.0.1/projects/tflite_micro/datasheet_model_test` and runs a **MobileNetV1 benchmark**: the `mobilenet_v1_1_0_224_quant_vela_tflite` model is compiled into the firmware, copied to PSRAM at start-up, and then invoked repeatedly with a 224×224×3 all-zero input so that only the NPU inference time matters. The `ROUND N TEST REPORT` printed at the end of every round keeps the same format as the face version: model size, where the model / arena / fast scratch live and how much they use, invoke timings and a PASS/FAIL verdict.

## Development Board Guide

This guide applies to Dashboard V1.0 (SCH-7259_V1.0) hardware and corresponds to the `face_recognition_meminfo` project.

> **Note**: The board requires a DC 12V power supply to boot normally.

### Project-Specific Configuration

| Setting | Project configuration |
| --- | --- |
| SoC | BK7259 |
| Inference framework | TensorFlow Lite Micro, running on the U65 NPU |
| Model | MobileNetV1, uint8 quantised, Vela compiled, 224×224×3 input, 1001-class output |
| Model location | Compiled into flash, copied to a PSRAM slab (uncoded) at start-up |
| Tensor arena | 1024 KB, PSRAM slab |
| Ethos-U fast scratch | 448 KB, HSRAM |
| Input data | 224×224×3 all-zero data refilled before every invoke |
| Display | No LCD and no LVGL; every result goes to the serial log |
| SD card | Not used; the model does not depend on the card |
| Test report | Printed once per round: model size, memory map, invoke timings and the verdict |

### Jumper Configuration

Before powering on, verify that the following jumpers are connected:

| Purpose | Jumper connections |
| --- | --- |
| BK7259 flashing and serial logs | `P11 ↔ RXD`, `P10 ↔ TXD` |

### Interface Usage

- Use the **USB-UART** interface for BK7259 flashing and serial logs.
- No key is bound in this project; the benchmark starts automatically after boot.

## Boot Flow

1. `bk_init()` + media service init.
2. Frame buffer init (both the arena and the model buffer come from these heaps).
3. Create the `model_task` thread.
4. Allocate 448 KB of HSRAM as the Ethos-U fast scratch and bring up the NPU.
5. Copy the model from flash into a PSRAM slab (uncoded).
6. Allocate a 1024 KB PSRAM slab as the tensor arena, create the interpreter and call `AllocateTensors()`.
7. Verify that the input tensor is `[1,224,224,3]`, 150528 bytes, then enter the loop.

## Demo Flow

1. Every round calls `Invoke()` three times; the input tensor is zeroed before each call.
2. Each call prints `mobilenet_v1 Invoke #N: xx.xx ms`.
3. After the three runs the device prints `ROUND N TEST REPORT` with the round's average / minimum / maximum invoke time and the actual arena usage.
4. It waits 5 seconds and starts the next round.

## Observe the Log

Serial output (example only; values depend on the actual run):

```
================================================
mobilenet_v1 benchmark: input 224x224x3, 150528 zero bytes per invoke
================================================

------------------------------------------------
round 1: 3 invokes on an all-zero input
mobilenet_v1 Invoke #1: 18.42 ms
mobilenet_v1 Invoke #2: 18.35 ms
mobilenet_v1 Invoke #3: 18.51 ms

---------------- round 1 done, next one in 5 seconds ----------------

================ ROUND 1 TEST REPORT ================
[System]
  chip        : BK7259 (Cortex-M55 + Ethos-U65)
  firmware    : face_recognition_meminfo  built Nov  4 2026 18:30:00
  round       : 1
[Models]
  MobileNetV1       3215.8 KB  input 224x224x3
[Memory map]
  model         :  3215.8 KB  linked in flash -> PSRAM slab (uncoded)
  shared arena  :  1024.0 KB  PSRAM slab, tensor arena used by Invoke
  fast scratch  :   448.0 KB  HSRAM, Ethos-U fast scratch
[Result]
  invokes 3, failures 0, invoke avg 18.43 ms (min 18.35, max 18.51)
  arena used  :   468.0 KB /  1024.0 KB (45.7%)
  VERDICT     : PASS
CSV,round=1,invoke_us=18427,runs=3/3,pass=1
=======================================================
```

Reading the log:

- The `ap0:multi-model:` prefix comes from `AvdkMultiModel`; at start-up it prints `AvdkMultiModel::init mobilenet_v1`.
- The input tensor is refilled with zeros before every `Invoke()`, so the measurement covers `Invoke()` only, not data preparation.
- The test report has four sections: `[System]` runtime context, `[Models]` model size and input shape, `[Memory map]` which memory the model / arena / fast scratch live in, their sizes and what they are used for, and `[Result]` the round's invoke statistics and verdict.
- `arena used` is what `AllocateTensors()` really consumed out of the 1024 KB arena, so the remaining headroom is visible.
- The final CSV line can be imported directly into a spreadsheet to trend multiple rounds.

## Key Source Files

| Path | Purpose |
| --- | --- |
| `ap/ap_main.cc` | NPU bring-up, model load, per-round invokes and the test report |
| `ap/resource/mobilenet_v1_model_data.cc` | The MobileNetV1 Vela model compiled into flash (3,292,944 bytes) |
| `components/avdk_nn_module/src/core/AvdkMultiModel.cc` | Model loading, fast scratch / shared arena management and interpreter creation |
| `bk_avdk_smp_release_4.0.1/projects/tflite_micro/datasheet_model_test` | The original benchmark this project follows |

## Build

```bash
make bk7259
# or through the container build script
./dbuild.sh make bk7259 APP_VERSION=verify
```
