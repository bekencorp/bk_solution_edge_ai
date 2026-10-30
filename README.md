# Beken BK7259 Edge AI Solution

- [中文](./README_CN.md)

## Overview

**BK7259 Edge AI Solution** is an open-source turnkey on-device AI solution released by Beken. Built on the **BK7259** main control chip with its embedded NPU and the Armino base SDK **BK_AVDK_SMP**, it adopts a BK7259 + BK3515N dual-chip architecture. At its core is offline face detection and recognition (SCRFD detection + Arcface matching, running on the NPU with TFLM), complemented by a smart dashboard reference implementation that covers MIPI DSI + DPU + LVGL display, boot AVI playback, full-screen camera preview, Dashcam loop recording and playback, local music playback, Bluetooth phone calls, BLE / Wi-Fi provisioning, phone casting, SD card storage and MTP export, along with complete example projects.

## Documentation

- [Armino SMP SDK (BK AVDK SMP)](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/index.html)

## Hardware

The hardware for this solution uses the BK7259 Dashboard V1.0 development board (EVB), which is built around the BK7259 main control chip and the BK3515N Bluetooth secondary control chip, and integrates a 1024×600 MIPI DSI display (EK79007AD), a camera, SD card storage, and buttons such as reset and provisioning reset. The BK7259 embeds an NPU for on-device AI inference.

- [BK7259 Dashboard V1.0 development board hardware materials](https://docs.bekencorp.com/arminodoc/bk_dashboard/bk7259/en/v4.0.1/hw-reference/index.html)
- [BK7259 Datasheet](https://docs.bekencorp.com/spec/BK7259/BK7259_Datasheet.pdf)

Development kit purchase link: coming soon.

## Version Strategy

This solution uses a "maintenance branch + release tag" version management approach:

- `release/v4.0.1` is the ongoing maintenance branch, used for feature iteration and bug fixes in this version series. It always contains the latest code, but it may include changes that have not yet completed the full release testing.
- `release/v4.0.1.x` are official release tags, such as `release/v4.0.1.1` and `release/v4.0.1.2`. Each tag has gone through the complete testing and release process and can be used as a mass-production version.

For solution development and mass production, customers should choose the highest tag in the `release/v4.0.1.x` series. Use `release/v4.0.1.12` for the Armino SMP SDK. Before use, make sure the selected solution tag is compatible with this SDK version.

<!-- readme-get-started:code -->
## Get the Code

The BK7259 Edge AI Solution and the Armino SMP SDK are released on GitHub.

BK7259 Edge AI Solution:

- GitHub: [https://github.com/bekencorp/bk_solution_edge_ai](https://github.com/bekencorp/bk_solution_edge_ai)

Armino SMP SDK:

- GitHub: [https://github.com/bekencorp/bk_avdk_smp/tree/release/v4.0.1.12](https://github.com/bekencorp/bk_avdk_smp/tree/release/v4.0.1.12)

GitHub is publicly accessible.

**Note for Windows users**: When using Git for Windows to get the code, we recommend disabling automatic line-ending conversion before cloning, to avoid scripts or source files being converted to CRLF and causing build failures. This is not required on Linux, macOS, or WSL.

```bash
git config --global core.autocrlf false
```

If the code has already been cloned, changing this setting will not automatically restore the files, so we recommend re-cloning after setting it.

The following commands use GitHub as the code source, with the Armino SMP SDK using the `release/v4.0.1.12` tag and the BK7259 Edge AI Solution using the `release/v4.0.1.1` tag.

```bash
mkdir -p ~/armino && cd ~/armino

# Armino SMP SDK
git clone --branch release/v4.0.1.12 https://github.com/bekencorp/bk_avdk_smp.git

# BK7259 Edge AI Solution
git clone --branch release/v4.0.1.1 https://github.com/bekencorp/bk_solution_edge_ai.git
```

<!-- readme-get-started:env -->
## Set Up the Build Environment

Armino SMP supports local build and Docker build. Choose one of the following methods to set up the build environment based on your development platform.

### Local Build on Linux

Enter the SDK directory and run the environment setup script:

```bash
# The setup script is located in the bk_avdk_smp repository
cd ~/armino/bk_avdk_smp
sudo bash tools/env_tools/setup/armino_env_setup.sh
```

### Local Build on Windows

Download and install [Armino Bash](https://dl.bekencorp.com/tools/arminosdk/WindowsInstaller/Armino-Bash-Setup_0.3.0.exe).

### Docker Build

The Docker build image is [bekencorp/armino-idk](https://hub.docker.com/r/bekencorp/armino-idk/tags). Choose an image tag of `1.5` or higher; it supports Windows / Linux / macOS.

For detailed setup steps, see [Environment Deployment and Compilation](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/get-started/index.html#smp-get-started-env).

<!-- readme-get-started:build -->
## Build the Project

> **Note**: Some projects ship a pre-built firmware binary in the repository at `projects/<project>/package/all-app.bin`. If you only want to try these projects, you can skip the build steps below and use that file directly in the "Flash the Firmware" section.

Taking the `face_recognition` project as an example (located in `projects/face_recognition`), point `SDK_DIR` to the Armino SMP SDK and build locally:

```bash
cd ~/armino/bk_solution_edge_ai/projects/face_recognition
make bk7259 SDK_DIR=~/armino/bk_avdk_smp        # or export SDK_DIR first, then run make bk7259
```

Docker build is also supported (use `./dbuild.sh` on Linux / macOS, and `.\dbuild.ps1` on Windows PowerShell):

```bash
cd ~/armino/bk_solution_edge_ai/projects/face_recognition
export SDK_DIR=~/armino/bk_avdk_smp
./dbuild.sh make bk7259
```

After a successful build, the firmware file used for flashing is located at the following path (relative to the `bk_solution_edge_ai/` repository root):

```text
projects/face_recognition/build/bk7259/face_recognition/package/all-app.bin
```

<!-- readme-get-started:flash -->
## Flash the Firmware

### Flash the main SoC

You can flash the BK7259 main control firmware using either of the following methods:

- Download and use the [BKFIL local flashing tool](https://dl.bekencorp.com/tools/bkfil/v4)
- Use the [BKFIL web flashing tool](https://connect.aclsemi.com/)

When flashing, select the `all-app.bin` firmware file generated in the previous section.

For the main-SoC flashing process, see [Burn Code](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/en/v4.0.1/get-started/index.html#smp-get-started-flash).

## Reference Projects
The following reference projects are listed; click a name for details.<!-- readme-projects:begin -->
- [camera_display](/projects/camera_display/)
- [dashboard](/projects/dashboard/)
- [face_recognition](/projects/face_recognition/)
<!-- readme-projects:end -->
## Beken Resources

- [Beken official website](https://www.bekencorp.com/)
- [Armino developer forum](https://armino.bekencorp.com/)
- [Beken documentation center](https://docs.bekencorp.com/)
- WeChat Channels: Beken Corporation
