# camera_display

* [English](./README.md)

固件启动后直接进入 camera display。

## 开发板指南

本指南适用于配备 1024×600 显示屏的 Dashboard V1.0 硬件。

> **注意**：开发板需使用 DC 12V 电源供电才能正常启动。

### 工程特定配置

| 项目 | 配置 |
| --- | --- |
| SoC | BK7259 |
| LCD | EK79007AD MIPI，1024×600 |
| 摄像头采集 | ISP MP 1280×720 |
| camera display | 全屏摄像头预览，GPU flexa 旋转 180° 并缩放至 1024×600 |

### 跳线配置

上电前请确认以下跳线已连接：

| 用途 | 跳线连接 |
| --- | --- |
| BK7259 烧录与串口日志 | `P11 ↔ RXD`，`P10 ↔ TXD` |

### 接口使用

- 使用 **USB-UART** 接口进行 BK7259 烧录与串口日志输出。

## 启动流程

1. `bk_init()` + media service 初始化。
2. 显示硬件初始化，播放开机 AVI 动画。
3. 启动后立即进入 camera display（`dashcam_assitview_direct_start()`），不显示 LVGL 页面。

## 按键

Dashboard V1.0 按键与 GPIO 映射保持不变。本固件中，短按仅向内部 worker 投递按键事件并打印日志，不绑定任何 UI 行为；双击与长按未绑定。

| 按键 | GPIO | 短按 | 双击 / 长按 |
| --- | --- | --- | --- |
| 上 | GPIO32 | 仅打日志 | 未绑定 |
| 下 | GPIO27 | 仅打日志 | 未绑定 |
| 左 | GPIO31 | 仅打日志 | 未绑定 |
| 右 | GPIO29 | 仅打日志 | 未绑定 |
| 中 | GPIO30 | 仅打日志 | 未绑定 |
