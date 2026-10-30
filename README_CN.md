# 博通集成 BK7259 边缘 AI 解决方案

- [English](./README.md)

## 概述

**BK7259 边缘 AI 解决方案**是 BEKEN 发布并开源的端侧 AI 整机方案。该方案基于 **BK7259** 主控芯片及其内置 NPU，依赖 Armino 基础 SDK **BK_AVDK_SMP**，采用 BK7259 与 BK3515N 双芯片架构。方案核心为离线人脸检测与识别（SCRFD 检测 + Arcface 比对，基于 TFLM 在 NPU 上运行），并提供智能仪表参考实现，涵盖 MIPI DSI + DPU + LVGL 显示、开机 AVI 播放、全屏摄像头预览、行车记录仪循环录像与回放、本地音乐播放、蓝牙电话、BLE / Wi-Fi 配网、手机投屏、SD 卡存储与 MTP 导出等能力，并提供完整的示例工程。

## 文档

- [Armino SMP SDK（BK AVDK SMP）](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/index.html)

## 硬件

本方案硬件使用 BK7259 Dashboard V1.0 开发板（EVB），采用 BK7259 主控芯片与 BK3515N 蓝牙从控芯片，集成 1024×600 MIPI DSI 显示屏（EK79007AD）、摄像头、SD 卡存储，以及复位（RESET）、配网复位（K1）等按键。BK7259 内置 NPU，支持端侧 AI 推理。

- [BK7259 Dashboard V1.0 开发板硬件资料](https://docs.bekencorp.com/arminodoc/bk_dashboard/bk7259/zh_CN/v4.0.1/hw-reference/index.html)
- [BK7259 Datasheet](https://docs.bekencorp.com/spec/BK7259/BK7259_Datasheet.pdf)

开发套件购买链接：即将上架。

## 版本策略

本方案采用“维护分支 + 发布标签（Tag）”的版本管理方式：

- `release/v4.0.1` 为持续维护分支，用于该版本系列的功能迭代和问题修复，始终包含最新代码，但其中可能包含尚未完成完整发布测试的改动。
- `release/v4.0.1.x` 为正式发布标签，例如 `release/v4.0.1.1`、`release/v4.0.1.2`。每个标签均经过完整的测试与发布流程，可作为量产版本使用。

建议方案代码选择 `release/v4.0.1.x` 系列中版本号最大的标签进行开发和量产，Armino SMP SDK 使用 `release/v4.0.1.12` 版本。使用前请确认所选方案标签与该 SDK 版本兼容。

<!-- readme-get-started:code -->
## 获取代码

BK7259 边缘 AI 解决方案和 Armino SMP SDK 均发布至 GitHub。

BK7259 边缘 AI 解决方案：

- GitHub：[https://github.com/bekencorp/bk_solution_edge_ai](https://github.com/bekencorp/bk_solution_edge_ai)

Armino SMP SDK：

- GitHub：[https://github.com/bekencorp/bk_avdk_smp/tree/release/v4.0.1.12](https://github.com/bekencorp/bk_avdk_smp/tree/release/v4.0.1.12)

GitHub 可公开访问。

**Windows 用户注意**：使用 Git for Windows 获取代码时，建议在克隆前关闭自动换行符转换，避免脚本或源文件被转换为 CRLF 而导致编译失败。Linux、macOS 和 WSL 环境无需执行。

```bash
git config --global core.autocrlf false
```

如果代码已经克隆，修改该配置不会自动恢复文件，建议设置后重新克隆。

以下命令以 GitHub 为代码源，Armino SMP SDK 使用 `release/v4.0.1.12` 标签，BK7259 边缘 AI 解决方案使用 `release/v4.0.1.1` 标签。

```bash
mkdir -p ~/armino && cd ~/armino

# Armino SMP SDK
git clone --branch release/v4.0.1.12 https://github.com/bekencorp/bk_avdk_smp.git

# BK7259 边缘 AI 解决方案
git clone --branch release/v4.0.1.1 https://github.com/bekencorp/bk_solution_edge_ai.git
```

<!-- readme-get-started:env -->
## 编译环境安装

Armino SMP 支持本地编译和 Docker 编译，可根据开发平台选择以下方式部署编译环境。

### Linux 本地编译

进入 SDK 目录并运行环境安装脚本：

```bash
# 安装脚本位于 bk_avdk_smp 仓库内
cd ~/armino/bk_avdk_smp
sudo bash tools/env_tools/setup/armino_env_setup.sh
```

### Windows 本地编译

下载并安装 [Armino Bash](https://dl.bekencorp.com/tools/arminosdk/WindowsInstaller/Armino-Bash-Setup_0.3.0.exe)。

### Docker 编译

Docker 编译镜像为 [bekencorp/armino-idk](https://hub.docker.com/r/bekencorp/armino-idk/tags)，请选择 `1.5` 或更高版本的镜像标签，支持 Windows / Linux / macOS。

详细安装步骤请参阅 [环境部署及编译](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/get-started/index.html#smp-get-started-env)。

<!-- readme-get-started:build -->
## 编译工程

> **说明**：部分工程在仓库中已附带预编译固件，路径为 `projects/<project>/package/all-app.bin`。如果仅想体验这些工程，可跳过下面的编译步骤，直接在“烧录固件”章节使用该文件。

以 `face_recognition` 工程为例（位于 `projects/face_recognition`），通过 `SDK_DIR` 指向 Armino SMP SDK 后本地编译：

```bash
cd ~/armino/bk_solution_edge_ai/projects/face_recognition
make bk7259 SDK_DIR=~/armino/bk_avdk_smp        # 也可先 export SDK_DIR 再执行 make bk7259
```

也支持 Docker 编译（Linux / macOS 用 `./dbuild.sh`，Windows PowerShell 用 `.\dbuild.ps1`）：

```bash
cd ~/armino/bk_solution_edge_ai/projects/face_recognition
export SDK_DIR=~/armino/bk_avdk_smp
./dbuild.sh make bk7259
```

编译成功后，用于烧录的固件文件位于以下路径（相对于 `bk_solution_edge_ai/` 仓库根目录）：

```text
projects/face_recognition/build/bk7259/face_recognition/package/all-app.bin
```

<!-- readme-get-started:flash -->
## 烧录固件

### 烧录主控

可选择以下任一方式烧录 BK7259 主控固件：

- 下载并使用 [BKFIL 本地烧录工具](https://dl.bekencorp.com/tools/bkfil/v4)
- 使用 [BKFIL 网页烧录工具](https://connect.aclsemi.com/)

烧录时请选择上一节编译生成的 `all-app.bin` 固件文件。

主控详细烧录流程请参阅 [烧录代码](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/get-started/index.html#smp-get-started-flash)。

## 参考工程
以下列出参考工程，可点击查看工程详细说明。<!-- readme-projects:begin -->
- [camera_display](/projects/camera_display/)
- [dashboard](/projects/dashboard/)
- [face_recognition](/projects/face_recognition/)
<!-- readme-projects:end -->
## BEKEN 相关资源

- [BEKEN 官网](https://www.bekencorp.com/)
- [ARMINO 开发者论坛](https://armino.bekencorp.com/)
- [BEKEN 文档中心](https://docs.bekencorp.com/)
- 微信视频号: 博通集成电路
