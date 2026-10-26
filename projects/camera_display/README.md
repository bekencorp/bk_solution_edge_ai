# camera_display

* [中文](./README_CN.md)

The firmware boots straight into the camera display.

## Development Board Guide

This guide applies to Dashboard V1.0 hardware equipped with a 1024×600 display.

> **Note**: The board requires a DC 12V power supply to boot normally.

### Project-Specific Configuration

| Setting | Configuration |
| --- | --- |
| SoC | BK7259 |
| LCD | EK79007AD MIPI, 1024×600 |
| Camera capture | ISP MP 1280×720 |
| camera display | Full-screen camera preview, GPU flexa rotated 180° and scaled to 1024×600 |

### Jumper Configuration

Before powering on, verify that the following jumpers are connected:

| Purpose | Jumper connections |
| --- | --- |
| BK7259 flashing and serial logs | `P11 ↔ RXD`, `P10 ↔ TXD` |

### Interface Usage

- Use the **USB-UART** interface for BK7259 flashing and serial logs.

## Boot Flow

1. `bk_init()` + media service init.
2. Display hardware init, boot AVI animation plays.
3. The camera display starts immediately after boot (`dashcam_assitview_direct_start()`), no LVGL page is shown.

## Keys

The Dashboard V1.0 key-to-GPIO mapping is unchanged. In this build, short presses only post key events to an internal worker that logs them; no UI action is bound. Double and long press actions are unbound.

| Key | GPIO | Short press | Double / long press |
| --- | --- | --- | --- |
| Up | GPIO32 | Log only | Unbound |
| Down | GPIO27 | Log only | Unbound |
| Left | GPIO31 | Log only | Unbound |
| Right | GPIO29 | Log only | Unbound |
| Middle | GPIO30 | Log only | Unbound |

