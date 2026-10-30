BK7259 Edge AI Solution Introduction
===========================================

:link_to_translation:`zh_CN:[中文]`

Overview
---------------------------------

The BK7259 Edge AI Solution is an on-device AI reference design from Beken, built on the BK7259 SoC with its embedded NPU and the Armino SMP (BK AVDK SMP) v4.0.x architecture. It uses a BK7259 + BK3515N dual-chip design, centers on offline face detection and recognition, and ships with a smart-dashboard reference implementation for the Dashboard V1.0 (1024×600) board. It provides:

- Offline face detection and recognition: SCRFD detection plus Arcface matching, running on the U65 NPU with TFLM;
- MIPI DSI + DPU + LVGL display, boot AVI playback, and full-screen camera preview;
- Phone JPEG casting, plus Wi-Fi / BLE image transmission;
- BLE / Wi-Fi provisioning;
- Dashcam loop recording and playback, local music playback, and Bluetooth calls;
- Classic Bluetooth audio, navigation voice, and multi-controller Bluetooth;
- SD card storage, MTP export, and complete example projects.

.. note::

   For environment setup, source download, build, flashing, and BK3515N companion flashing, see :doc:`../get-started/index`.
   For companion software download and usage, see :doc:`../software/index`.
   For each example project's features and usage, see :doc:`../projects/index`.

Design Philosophy
---------------------------------

The BK7259 Edge AI Solution splits work across a main SoC and a Bluetooth companion:

- **BK7259 main SoC**: face detection and recognition inference (NPU), display UI, image transmission, provisioning, on-device A/V, and storage;
- **BK3515N companion**: classic Bluetooth and related companion-side wireless features, working with the main SoC for the full Bluetooth experience;
- **Armino SMP**: SoC, RTOS, drivers, and network stacks; this repository implements the AI application, dashboard application, and example projects on top.

Core Features
---------------------------------

1. On-device face detection and recognition
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

- SCRFD face detection with 320×320 BGRA input and int8 quantization;
- Arcface embedding with 512-dimensional output and a cosine-similarity threshold of 0.70;
- VG-Lite GPU five-point affine alignment, with TFLM inference running on the U65 NPU.

2. Dashboard and camera display
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

- 1024×600 MIPI DSI + DPU;
- LVGL dashboard UI, boot AVI playback, and full-screen camera preview.

3. Casting and image transmission
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

- Phone JPEG casting;
- Wi-Fi and BLE image transmission.

4. Wireless connectivity
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

- BLE / Wi-Fi provisioning;
- Classic Bluetooth audio, navigation voice, and multi-controller Bluetooth.

5. Storage and peripherals
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

- **Display**: 1024×600 MIPI DSI (EK79007AD);
- **Input**: reset (RESET) and provisioning-reset (K1) buttons;
- **Storage**: SD card with MTP export;
- **Companion**: BK3515N Bluetooth.

Development-board hardware materials are in :doc:`../hw-reference/index`.

System Architecture
---------------------------------

.. rubric:: Relationship between the BK7259 Edge AI Solution and BK AVDK SMP

This repository and the underlying BK AVDK SMP (Armino SMP SDK v4.0.x) split the work as follows:

#. **Scope**: this is a scenario-specific solution on top of BK AVDK SMP, focused on on-device AI inference, dashboard UI, image transmission, provisioning, and Bluetooth audio; the SoC, RTOS, drivers, and network stacks live in BK AVDK SMP.
#. **Code boundary**: this repository (``bk_solution_edge_ai``) provides solution and application code (``components/``, ``projects/``, mounted at the repo root); hardware drivers, RTOS, memory management, and Wi-Fi / BLE stacks live in SMP.

On BK7259, Armino SMP uses an AP (application processor) + CP (communication processor) split:

- **AP**: face recognition inference, LVGL dashboard, image transmission, A/V, and application logic;
- **CP**: Wi-Fi, BLE, networking stacks, and low-power management.

Software stack (illustrative)::

    Application (face recognition / LVGL dashboard / casting / music / phone)
            ↓
    Services (TFLM inference / image tx / audio / provisioning / Dashcam)
            ↓
    OS (RTOS)
            ↓
    Hardware (BK7259 AP+CP + U65 NPU + BK3515N)

Typical Use Cases
---------------------------------

1. **Offline face recognition**: local face detection, alignment, and matching without network access;
2. **Smart dashboard**: LVGL display, boot animation, and physical-key interaction;
3. **Dashcam record and playback**: loop recording and local playback;
4. **Phone casting and image transmission**: JPEG casting and Wi-Fi / BLE image tx;
5. **In-vehicle entertainment**: local music, Bluetooth calls and contacts.

Technical Highlights
---------------------------------

1. **On-device AI with the NPU**: face detection and recognition run entirely on the device, with models loaded from the SD card and no network dependency.
2. **Complete solution paired with SMP**: this repo focuses on AI and dashboard application; drivers / RTOS / stacks ship with BK AVDK SMP.
3. **Dual-chip reference**: BK7259 main SoC + BK3515N companion covers AI inference, display, and companion-side Bluetooth.
4. **Multiple runnable example projects**: face recognition, smart dashboard, and camera preview; see :doc:`../projects/index`.
