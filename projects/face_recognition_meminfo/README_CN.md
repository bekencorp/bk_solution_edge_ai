# 人脸识别演示工程 face_recognition_meminfo

* [English](./README.md)

在 `face_recognition` 基础上增强的演示工程：识别流程与原工程完全一致（SCRFD 做人脸检测、Arcface 做人脸特征提取，U65 NPU 上跑 TensorFlow Lite Micro 推理），额外在**每轮识别结束后打印一份测试报告**，集中给出模型大小、共享 tensor arena / fast scratch 的占用与复用方式、各阶段耗时、识别结果和 PASS/FAIL 判定，适合直接用于演示和性能说明。

## 开发板使用说明

本说明适用于 Dashboard V1.0（SCH-7259_V1.0）硬件，对应工程 `face_recognition_meminfo`。

> **备注**：板子正常启动需要用 DC 12V 供电。

### 工程适配信息

| 配置项 | 本工程配置 |
| --- | --- |
| 主控 | BK7259 |
| 推理框架 | TensorFlow Lite Micro，运行在 U65 NPU 上 |
| 人脸检测 | SCRFD，输入 320×320 BGRA，int8 量化 |
| 人脸特征 | Arcface，输入 112×112 RGB，输出 512 维特征 |
| 人脸对齐 | VG-Lite GPU 五点仿射变换到 112×112 |
| 人脸底库 | 3 人（Alice / Bob / Jason），特征常驻 RAM |
| 相似度判定 | 余弦相似度，阈值 0.70 |
| 图像输入 | 3 张 320×320 图片编译进固件，不使用摄像头 |
| 显示 | 不使用 LCD 与 LVGL，结论全部通过串口日志输出 |
| SD 卡 | SDIO1 4 线，模型文件从 SD 卡加载（`1:/tflite`） |
| 测试报告 | 每轮结束打印一次，含模型大小、内存分布、各阶段耗时与识别判定 |

### 跳线帽配置

上电前请确认以下跳线帽已短接：

| 用途 | 短接位置 |
| --- | --- |
| BK7259 下载与串口日志 | `P11 ↔ RXD`、`P10 ↔ TXD` |

### 接口使用

- BK7259 下载和串口日志均使用 **USB-UART** 接口。
- 本工程未绑定任何按键功能，设备上电后自动开始演示。

## SD 卡准备

人脸模型放在 SD 卡根目录的 `tflite` 目录下：

| 文件 | 用途 |
| --- | --- |
| `/tflite/face_detection_int8_vela.tflite` | SCRFD 人脸检测模型 |
| `/tflite/face_verify_int8_vela.tflite` | Arcface 人脸特征模型 |

SD 卡挂载到 `/sd0`，同时注册为 FatFs 卷 `1:`，固件按 `1:/tflite/...` 加载模型。启动时若挂载失败或模型缺失，会打印 `model init failed` 并停止演示。

## 启动流程

1. `bk_init()` 与 media service 初始化。
2. 初始化 frame buffer。
3. 挂载 SD 卡到 `/sd0`。
4. VG-Lite 上电（只调用 GPU 初始化，不打开 flexa 显示通路）。
5. 创建 `face_task` 线程，进入演示流程。

## 演示流程

1. **开会认人**：三张图片依次走完 detect → align → verify，512 维特征存入 RAM，分别记作 Alice、Bob、Jason。
2. **循环识别**：每轮依次让三张图片当访客，各自与三份已知特征逐一比较，得出「是谁」的结论。
3. 每个访客之间间隔 300 ms，每轮之间间隔 5 秒并重新自我介绍。
4. **测试报告**：每轮所有访客走完后打印 `ROUND N TEST REPORT`，汇总本轮的模型、内存、耗时和识别结果，然后等待 5 秒进入下一轮。

## 观察日志

串口会打印完整的推理过程与耗时（以下为节选）：

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

日志要点：

- `ap0:TFLM:` 前缀是 TFLM 的调试输出（由 `FaceRecognitionModel::LogEnable(true)` 打开），其中 `U65 NPU | tensorflow-lite-micro ... invoke` 明确标出该次推理跑在 U65 NPU 上。
- 每个阶段都单独打印一次毫秒耗时，按实际执行顺序排列：归一化 → NPU 检测 → 检测后处理 → 对齐 → verify 输入准备 → NPU 特征提取 → verify 后处理。
- `ap0:face_app:` 前缀是演示工程自己的日志：检测框位置与置信度、特征归一化模长、与三个人脸的相似度，以及最终判定。
- 相似度最大且不低于 0.70 判定为本人，否则打印 `stranger`。
- 测试报告分四段：`[System]` 运行环境、`[Models]` 两个模型的大小与输入尺寸、`[Memory map]` 模型加载位置以及两个模型分时复用的共享 arena / fast scratch、`[Result]` 本轮识别结果与判定。
- `detect avg` 是检测 NPU 推理加后处理的平均耗时，`verify avg` 是验证模型准备、输入归一化、NPU 推理和特征后处理的平均耗时。
- 报告最后一行 CSV 便于把多轮数据直接导入表格做趋势统计。

## 关键代码位置

| 路径 | 说明 |
| --- | --- |
| `ap/ap_main.cc` | 演示流程、SD 卡挂载、GPU 上电、每轮测试报告输出 |
| `ap/resource/face3_image_input.cc` 等 | 编译进固件的 3 张 320×320 BGRA 图像 |
| `components/avdk_nn_module` | `tflm_face_recog` 组件：检测/特征模型封装与对齐 |
| `components/avdk_nn_module/src/tflm_face_recog/FaceRecognitionModel.cc` | 模型元数据与各阶段耗时采集，供测试报告读取 |
| `components/multimedia_device_service/src/app_gpu.c` | VG-Lite 上电与 GPU 锁 |

## 编译

```bash
make bk7259
# 或使用容器编译脚本
./dbuild.sh make bk7259 APP_VERSION=verify
```
