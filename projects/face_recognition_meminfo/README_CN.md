# MobileNetV1 NPU 基准工程 face_recognition_meminfo

* [English](./README.md)

本工程原先用于人脸识别演示，现已按 `bk_avdk_smp_release_4.0.1/projects/tflite_micro/datasheet_model_test` 的做法改为 **MobileNetV1 推理基准**：模型 `mobilenet_v1_1_0_224_quant_vela_tflite` 直接编译进固件，启动时拷贝到 PSRAM，然后用 224×224×3 的全零输入反复调用 `Invoke()`，只关注 NPU 推理耗时。每轮结束后打印的 `ROUND N TEST REPORT` 与原人脸版本格式一致：模型大小、model / arena / fast scratch 的分布与占用、Invoke 耗时和 PASS/FAIL 判定。

## 开发板使用说明

本说明适用于 Dashboard V1.0（SCH-7259_V1.0）硬件，对应工程 `face_recognition_meminfo`。

> **备注**：板子正常启动需要用 DC 12V 供电。

### 工程适配信息

| 配置项 | 本工程配置 |
| --- | --- |
| 主控 | BK7259 |
| 推理框架 | TensorFlow Lite Micro，运行在 U65 NPU 上 |
| 模型 | MobileNetV1，uint8 量化，vela 编译，输入 224×224×3，输出 1001 类 |
| 模型来源 | 编译进固件（flash），启动时拷贝到 PSRAM slab (uncoded) |
| Tensor arena | 1024 KB，PSRAM slab |
| Ethos-U fast scratch | 448 KB，HSRAM |
| 输入数据 | 每次 Invoke 前填入 224×224×3 全零数据 |
| 显示 | 不使用 LCD 与 LVGL，结果全部通过串口日志输出 |
| SD 卡 | 不使用，模型不依赖 SD 卡 |
| 测试报告 | 每轮结束打印一次，含模型大小、内存分布、Invoke 耗时与判定 |

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

串口输出如下（以下为示例，数值以实际运行为准）：

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

日志要点：

- `ap0:multi-model:` 前缀来自 `AvdkMultiModel`，启动时会打印 `AvdkMultiModel::init mobilenet_v1`。
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
