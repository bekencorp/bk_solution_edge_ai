# MobileNetV1 NPU Benchmark (mobilenet_benchmark)

* [中文](./README_CN.md)

This is a MobileNetV1 inference benchmark: the `mobilenet_v1_1_0_224_quant_vela_tflite` model is compiled into the firmware, copied to PSRAM at start-up, and then invoked repeatedly with a 224×224×3 all-zero input to measure U65 NPU inference time. At the end of every round it prints a `ROUND N TEST REPORT` with the model size, where the model / arena / fast scratch live and how much memory each takes, the invoke timings and a PASS/FAIL verdict. The resource layout matches `bk_avdk_smp_release_4.0.1/projects/tflite_micro/datasheet_model_test`: a 1 MB PSRAM tensor arena and 448 KB of HSRAM fast scratch.

## Development Board Guide

This guide applies to Dashboard V1.0 (SCH-7259_V1.0) hardware and corresponds to the `mobilenet_benchmark` project.

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

### Model Sources

MobileNetV1 is a lightweight CNN open-sourced by Google:

| Model | Purpose | Upstream repository |
| --- | --- | --- |
| MobileNetV1 | Image classification (224×224×3 input, 1001 classes) | <https://github.com/tensorflow/models/tree/master/research/slim> |

The model used here is uint8 quantised and compiled by Vela into an Ethos-U-executable `.tflite` file that is linked directly into the firmware.

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

## Run Flow

1. Every round calls `Invoke()` three times; the input tensor is zeroed before each call.
2. Each call prints `mobilenet_v1 Invoke #N: xx.xx ms`.
3. After the three runs the device prints `ROUND N TEST REPORT` with the round's average / minimum / maximum invoke time and the actual arena usage.
4. It waits 5 seconds and starts the next round.

## Log Output

Below is the actual serial output captured from the board at **round 2**:

```
ap0:model_be:I(5146):------------------------------------------------
ap0:model_be:I(5146):round 2: 3 invokes on an all-zero input
ap0:model_be:I(5162):mobilenet_v1 Invoke #1: 15.63 ms
ap0:model_be:I(5178):mobilenet_v1 Invoke #2: 15.63 ms
ap0:model_be:I(5195):mobilenet_v1 Invoke #3: 15.63 ms
ap0:model_be:I(5195):
ap0:model_be:I(5195):---------------- round 2 done, next one in 5 seconds ----------------
ap0:model_be:I(5195):
ap0:model_be:I(5195):
ap0:model_be:I(5195):================ ROUND 2 TEST REPORT ================
ap0:model_be:I(5195):[System]
ap0:model_be:I(5195):  chip        : BK7259 (Cortex-M55 + Ethos-U65)
ap0:model_be:I(10249): firmware    : mobilenet_benchmark  built Nov  5 2026 15:13:41
ap0:model_be:I(5195):  round       : 2
ap0:model_be:I(5195):[Models]
ap0:model_be:I(5195):  MobileNetV1       3215.8 KB  input 224x224x3
ap0:model_be:I(5196):[Memory map]
ap0:model_be:I(5196):  model         :  3215.8 KB  linked in flash -> PSRAM slab (uncoded)
ap0:model_be:I(5196):  shared arena  :  1024.0 KB  PSRAM slab, tensor arena used by Invoke
ap0:model_be:I(5196):  fast scratch  :   448.0 KB  HSRAM, Ethos-U fast scratch
ap0:model_be:I(5196):[Result]
ap0:model_be:I(5196):  invokes 3, failures 0, invoke avg 15.63 ms (min 15.63, max 15.63)
ap0:model_be:I(5196):  arena used  :   445.9 KB /  1024.0 KB (43.5%)
ap0:model_be:I(5196):  VERDICT     : PASS
ap0:model_be:I(5196):CSV,round=2,invoke_us=15625,runs=3/3,pass=1
ap0:model_be:I(5196):=======================================================
```

Reading the log:

- The `ap0:model_be:` prefix is this project's own log tag (`TAG = "model_bench"`, truncated to `model_be` by the serial logger); during start-up you will also see `ap0:multi-model:` lines such as `AvdkMultiModel::init mobilenet_v1`.
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
