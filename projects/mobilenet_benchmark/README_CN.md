# MobileNetV1 NPU 基准工程 mobilenet_benchmark

* [English](./README.md)

这是一个 MobileNetV1 推理基准工程：模型 `mobilenet_v1_1_0_224_quant_vela_tflite` 直接编译进固件，启动时拷贝到 PSRAM，然后用 224×224×3 的全零输入反复调用 `Invoke()`，用来测量 U65 NPU 的推理耗时。每轮结束后打印 `ROUND N TEST REPORT`：模型大小、model / arena / fast scratch 的分布与占用、Invoke 耗时和 PASS/FAIL 判定。资源配置与 `bk_avdk_smp_release_4.0.1/projects/tflite_micro/datasheet_model_test` 保持一致：1 MB PSRAM tensor arena、448 KB HSRAM fast scratch。

## 开发板使用说明

本说明适用于 Dashboard V1.0（SCH-7259_V1.0）硬件，对应工程 `mobilenet_benchmark`。

> **备注**：板子正常启动需要用 DC 12V 供电。

### 工程适配信息

| 配置项 | 本工程配置 |
| --- | --- |
| 主控 | BK7259 |
| 推理框架 | TensorFlow Lite Micro，运行在 U65 NPU 上 |
| 模型 | MobileNetV1，uint8 量化，vela 编译，输入 224×224×3，输出 1001 类 |
| 模型位置 | 编译进固件（flash），启动时拷贝到 PSRAM slab (uncoded) |
| Tensor arena | 1024 KB，PSRAM slab |
| Ethos-U fast scratch | 448 KB，HSRAM |
| 输入数据 | 每次 Invoke 前填入 224×224×3 全零数据 |
| 显示 | 不使用 LCD 与 LVGL，结果全部通过串口日志输出 |
| SD 卡 | 不使用，模型不依赖 SD 卡 |
| 测试报告 | 每轮结束打印一次，含模型大小、内存分布、Invoke 耗时与判定 |

### 模型来源

MobileNetV1 是 Google 开源的轻量级 CNN：

| 模型 | 用途 | 开源仓库 |
| --- | --- | --- |
| MobileNetV1 | 图像分类（224×224×3 输入，1001 类输出） | <https://github.com/tensorflow/models/tree/master/research/slim> |

本工程使用的模型经过 uint8 量化，并由 Vela 编译成 Ethos-U 可执行的 `.tflite`，直接编译进固件。

### 跳线帽配置

上电前请确认以下跳线帽已短接：

| 用途 | 短接位置 |
| --- | --- |
| BK7259 下载与串口日志 | `P11 ↔ RXD`、`P10 ↔ TXD` |

### 接口使用

- BK7259 下载和串口日志均使用 **USB-UART** 接口。
- 本工程未绑定任何按键功能，设备上电后自动开始基准测试。

## 启动流程

1. `bk_init()` 与 media service 初始化。
2. 初始化 frame buffer（arena 与模型缓冲区都从这里分配）。
3. 创建 `model_task` 线程。
4. 分配 448 KB HSRAM 作为 Ethos-U fast scratch 并初始化 NPU。
5. 把 flash 中的模型拷贝到 PSRAM slab (uncoded)。
6. 分配 1024 KB PSRAM slab 作为 tensor arena，创建 interpreter 并 `AllocateTensors()`。
7. 校验输入张量为 `[1,224,224,3]`、共 150528 字节，然后进入循环。

## 运行流程

1. 每轮调用 3 次 `Invoke()`，每次调用前把输入张量整体清零（全零图像）。
2. 每次调用打印一行 `mobilenet_v1 Invoke #N: xx.xx ms`。
3. 3 次跑完后打印 `ROUND N TEST REPORT`，汇总本轮的平均/最小/最大耗时以及 arena 实际占用。
4. 等待 5 秒进入下一轮，循环往复。

## 观察日志

以下是板子上运行到**第 2 轮**时的实际串口输出：

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

日志要点：

- `ap0:model_be:` 是本工程自己的日志前缀（源码中 `TAG = "model_bench"`，串口打印时截断为 `model_be`）；启动阶段还能看到 `ap0:multi-model:` 前缀的 `AvdkMultiModel::init mobilenet_v1`。
- 每次 `Invoke()` 前都会重新填入全零输入，计时只覆盖 `Invoke()` 本身，不含数据准备。
- 测试报告分四段：`[System]` 运行环境、`[Models]` 模型大小与输入尺寸、`[Memory map]` 模型 / arena / fast scratch 各在什么内存、多大、用途是什么、`[Result]` 本轮 Invoke 耗时统计与判定。
- `arena used` 是 `AllocateTensors()` 后 tensor arena 的实际占用量，可与 1024 KB 容量对比看出余量。
- 报告最后一行 CSV 便于把多轮数据直接导入表格做趋势统计。

## 关键代码位置

| 路径 | 说明 |
| --- | --- |
| `ap/ap_main.cc` | NPU 初始化、模型加载、每轮 Invoke 与测试报告输出 |
| `ap/resource/mobilenet_v1_model_data.cc` | 编译进固件的 MobileNetV1 vela 模型（3,292,944 字节） |
| `components/avdk_nn_module/src/core/AvdkMultiModel.cc` | 模型加载、fast scratch / 共享 arena 管理与 interpreter 创建 |
| `bk_avdk_smp_release_4.0.1/projects/tflite_micro/datasheet_model_test` | 本工程参考的原始基准工程 |

## 编译

```bash
make bk7259
# 或使用容器编译脚本
./dbuild.sh make bk7259 APP_VERSION=verify
```
