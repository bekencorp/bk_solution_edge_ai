# 人脸识别演示工程 face_recognition

* [English](./README.md)

仅用于演示的人脸识别工程：SCRFD 做人脸检测、GhostFaceNets 做人脸特征提取，U65 NPU 上跑 TensorFlow Lite Micro 推理。输入为编译进固件的 3 张静态图片，设备逐张识别出 Alice、Bob、Jason 中的哪一位。

## 开发板使用说明

本说明适用于 Dashboard V1.0（SCH-7259_V1.0）硬件，对应工程 `face_recognition`。

> **备注**：板子正常启动需要用 DC 12V 供电。

### 工程适配信息

| 配置项 | 本工程配置 |
| --- | --- |
| 主控 | BK7259 |
| 推理框架 | TensorFlow Lite Micro，运行在 U65 NPU 上 |
| 人脸检测 | SCRFD，输入 320×320 BGRA，int8 量化 |
| 人脸特征 | GhostFaceNets，输入 112×112 RGB，输出 512 维特征 |
| 人脸对齐 | VG-Lite GPU 五点仿射变换到 112×112 |
| 人脸底库 | 3 人（Alice / Bob / Jason），特征常驻 RAM |
| 相似度判定 | 余弦相似度，阈值 0.70 |
| 图像输入 | 3 张 320×320 图片编译进固件，不使用摄像头 |
| 显示 | 不使用 LCD 与 LVGL，结论全部通过串口日志输出 |
| 外部存储 | 不需要，两个模型已编译进固件 |

### 模型来源

本工程用到的两个模型均来自开源社区：

| 模型 | 用途 | 开源仓库 |
| --- | --- | --- |
| SCRFD | 人脸检测（320×320 输入） | <https://github.com/deepinsight/insightface/tree/master/detection/scrfd> |
| GhostFaceNets | 人脸特征提取（112×112 输入，512 维特征） | <https://github.com/HamadYA/GhostFaceNets> |

两个模型都经过 int8 量化，并由 Vela 编译成 Ethos-U 可执行的 `.tflite`，再转成 C 数组编译进固件（见下文「模型编译进固件」），启动时拷贝到 PSRAM 后运行。

> **License 说明**：SCRFD 与 GhostFaceNets 的模型代码均采用 MIT License，可商用；本工程使用的两个模型均由自行收集的数据训练得到。

### 跳线帽配置

上电前请确认以下跳线帽已短接：

| 用途 | 短接位置 |
| --- | --- |
| BK7259 下载与串口日志 | `P11 ↔ RXD`、`P10 ↔ TXD` |

### 接口使用

- BK7259 下载和串口日志均使用 **USB-UART** 接口。
- 本工程未绑定任何按键功能，设备上电后自动开始演示。

## 模型编译进固件

两个模型以 C 数组的形式编译进固件，运行时不依赖 SD 卡：

| 文件 | 用途 |
| --- | --- |
| `components/avdk_nn_module/src/tflm_face_recog/face_detect_model_data.cc` | SCRFD 人脸检测模型（666,240 B） |
| `components/avdk_nn_module/src/tflm_face_recog/face_verify_model_data.cc` | GhostFaceNets 人脸特征模型（2,370,704 B） |

`CONFIG_SDCARD` 关闭后，`tflm_face_recog` 组件会自动编译这两个数组，并在启动时把模型拷贝到 PSRAM slab（uncoded）供 NPU 使用。两个模型合计约 2.9 MB，因此 `primary_ap_app` 分区已从 4148K 扩大到 5120K，烧录时需要同时更新分区表。

## 启动流程

1. `bk_init()` 与 media service 初始化。
2. 初始化 frame buffer。
3. VG-Lite 上电（只调用 GPU 初始化，不打开 flexa 显示通路）。
4. 创建 `face_task` 线程，进入演示流程。

## 演示流程

1. **开会认人**：三张图片依次走完 detect → align → verify，512 维特征存入 RAM，分别记作 Alice、Bob、Jason。
2. **循环识别**：每轮依次让三张图片当访客，各自与三份已知特征逐一比较，得出「是谁」的结论。
3. 每个访客之间间隔 300 ms，每轮之间间隔 5 秒并重新自我介绍。

## 观察日志

串口会打印完整的推理过程与耗时。以下是板子上运行到**第 4 轮**时的实际串口输出（完整一轮，含 3 位访客）：

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

日志要点：

- `ap0:TFLM:` 前缀是 TFLM 的调试输出（由 `FaceRecognitionModel::LogEnable(true)` 打开），其中 `U65 NPU | tensorflow-lite-micro ... invoke` 明确标出该次推理跑在 U65 NPU 上。
- 每个阶段都单独打印一次毫秒耗时，按实际执行顺序排列：归一化 → NPU 检测 → 检测后处理 → 对齐 → verify 输入准备 → NPU 特征提取 → verify 后处理。
- `ap0:face_app:` 前缀是演示工程自己的日志：检测框位置与置信度、与三个人脸的相似度，以及最终判定。
- 相似度最大且不低于 0.70 判定为本人，否则打印 `stranger`。

## 关键代码位置

| 路径 | 说明 |
| --- | --- |
| `ap/ap_main.cc` | 演示流程、GPU 上电、日志输出 |
| `ap/resource/face3_image_input.cc` 等 | 编译进固件的 3 张 320×320 BGRA 图像 |
| `components/avdk_nn_module` | `tflm_face_recog` 组件：检测/特征模型封装与对齐 |
| `components/multimedia_device_service/src/app_gpu.c` | VG-Lite 上电与 GPU 锁 |

## 编译

```bash
make bk7259
# 或使用容器编译脚本
./dbuild.sh make bk7259 APP_VERSION=verify
```
