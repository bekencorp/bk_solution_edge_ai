# dashboard

* [English](./README.md)

## 开发板指南

本指南适用于配备 1024×600 显示屏的 Dashboard V1.0 硬件，对应 `dashboard` 工程。

> **注意**：开发板需使用 DC 12V 电源供电才能正常启动。

### 工程特定配置

| 项目 | 工程配置 |
| --- | --- |
| SoC | BK7259 |
| LCD | EK79007AD MIPI，1024×600 |
| 启动背景 | SD 卡上的 `/sd0/home_bg_tech_1024_600.jpg` |
| 投屏 JPEG | 输入/输出 1024×600，旋转 180° |
| Dashcam 采集与录像 | ISP MP 1280×720；H.264 25 FPS 录像 |
| Assist View | 输出 1024×600，旋转 180° 并缩放至屏幕 |
| 录像回放 | 输出 1024×600，旋转 180° |
| 分辨率相关资源 | 文件名含 `1024x600` 的背景图，以及按该分辨率适配的布局、图标与字体 |

本工程的固件与 UI 资源仅面向 1024×600 显示屏，请勿与其他分辨率工程的固件或生成资源混用。

### 跳线配置

上电前请确认以下跳线已连接：

| 用途 | 跳线连接 |
| --- | --- |
| BK7259 烧录与串口日志 | `P11 ↔ RXD`，`P10 ↔ TXD` |
| BK3515NS 烧录 | `BT_P0 ↔ RXD`，`BT_P1 ↔ TXD` |
| BK7259 与 BK3515NS 串口通信 | `P71 ↔ BT_RST`，`P0 ↔ BT_P0`，`P1 ↔ BT_P1` |

### 接口使用

- 使用 **USB-UART** 接口进行 BK7259 烧录与串口日志输出。
- 使用 **USB-UART** 接口进行 BK3515NS 烧录。按上表切换相关跳线，烧录完成后请断开烧录连线。
- 使用板载 **USB** 接口进行 MTP。连接 PC 后，通过串口 CLI 执行 `ap_cmd mtp start`，将 SD 卡以媒体设备形式暴露给 PC。

## 功能概览

本工程面向 Dashboard V1.0 1024×600 硬件，提供主页导航、Dashcam 循环录像与回放、本地 SD 卡音乐播放、通讯录、Assist View、蓝牙通话控制、配网以及 SD 卡 MTP 导出。

- 录像文件存储在 `/sd0/dashcam`。

## 页面

| 页面 | 用途 | 进入方式 |
| --- | --- | --- |
| 主页 | 默认页，用于选择 Dashcam、OTA、通讯录或音乐播放器 | 开机后默认显示；在功能页双击中键返回 |
| Dashcam | 浏览 SD 卡已完成录像并播放所选片段；列表显示录像时间与文件大小 | 主页用左/右键选中 Dashcam，再短按中键 |
| OTA | 显示 OTA 升级状态与进度 | 主页用左/右键选中 OTA，再短按中键 |
| 通讯录 | 浏览联系人与最近通话，通过蓝牙 HFP 拨打电话 | 主页用左/右键选中通讯录，再短按中键 |
| 音乐播放器 | 扫描并播放 `/sd0/Music` 下的本地音频 | 主页用左/右键选中音乐播放器，再短按中键 |
| Assist View | 全屏实时摄像头画面 | 主页长按左键或右键；双击中键退出返回主页 |

## 按键

Dashboard V1.0 按键与 GPIO 映射如下：

| 按键 | GPIO | 动作 | 功能 |
| --- | --- | --- | --- |
| 上 | GPIO32 | 短按 | 在通讯录或音乐播放器播放列表中选择上一项 |
| 上 | GPIO32 | 长按 | 进入经典蓝牙配对模式，使手机可再次发现并配对本设备 |
| 下 | GPIO27 | 短按 | 在通讯录或音乐播放器播放列表中选择下一项 |
| 下 | GPIO27 | 长按 | 清除已保存的配网信息并重启；仅主页生效 |
| 左 | GPIO31 | 短按 | 选择上一项；切换通讯录列表；切换音乐播放器控件或播放列表 |
| 左 | GPIO31 | 长按 | 打开 Assist View；仅主页生效 |
| 右 | GPIO29 | 短按 | 选择下一项；切换通讯录列表；切换音乐播放器控件或播放列表 |
| 右 | GPIO29 | 长按 | 打开 Assist View；仅主页生效 |
| 中 | GPIO30 | 短按 | 确认当前焦点项：打开页面、播放录像或曲目、执行音乐控制、拨打电话或执行通话操作 |
| 中 | GPIO30 | 双击 | 从功能页返回主页；退出 Assist View 返回主页 |
| 中 | GPIO30 | 长按 | 切换 ANCS BLE 广播（默认关闭）。开启后手机可发现 `BK_DASHBOARD_*` 并配对接收通知；再次长按停止 |

### 页面按键行为

| 当前页面或状态 | 方向键 | 中键短按 | 中键双击 |
| --- | --- | --- | --- |
| 主页 | 左/右在 Dashcam、OTA、通讯录、音乐播放器之间循环 | 打开所选页面 | 停留或返回主页 |
| Dashcam | 左/右选择上一条/下一条录像，循环 | 播放所选录像 | 返回主页 |
| OTA | 页面内无方向键操作 | 无操作 | 返回主页 |
| 通讯录 | 左/右切换 联系人/最近通话；上/下选择列表项 | 拨打所选号码 | 返回主页 |
| 音乐播放器 | 左/右切换播放控件与播放列表；上/下选择曲目 | 执行焦点控件或播放所选曲目 | 返回主页 |
| HFP 来电 | 左/右选择 接听/挂断 | 执行所选操作 | 返回主页 |
| HFP 去电或通话中 | 当前通话操作自动获取焦点 | 挂断 | 返回主页 |
| Assist View | 无操作 | 无操作 | 退出 Assist View 返回主页 |

### Dashcam 回放

1. 主页短按左键或右键选中 Dashcam。
2. 短按中键进入 Dashcam，等待录像列表加载完成。
3. 短按左键或右键选择上一条/下一条录像。
4. 短按中键播放所选录像。
5. 双击中键返回主页。

录像列表每项第一行显示录像时间，第二行显示文件大小（MB）。

### 音乐播放器

1. 将受支持的音频文件放入 SD 卡 `/sd0/Music` 目录，支持 MP3、WAV、FLAC、M4A、AAC、OGG、APE。
2. 主页用左/右键选中音乐播放器，再短按中键。
3. 左/右在播放控件与播放列表之间切换；上/下选择曲目。
4. 短按中键执行焦点控件或播放所选曲目。
5. 双击中键返回主页。

## CLI 命令

通过串口 CLI 使用 `ap_cmd dashboard <command>`：

| 命令 | 描述 |
| --- | --- |
| `ap_cmd dashboard dashcam` | 直接进入 Dashcam 页 |
| `ap_cmd dashboard phone_book` | 直接进入通讯录页 |
| `ap_cmd dashboard key_prev` | 模拟左键短按，选择上一项 |
| `ap_cmd dashboard key_next` | 模拟右键短按，选择下一项 |
| `ap_cmd dashboard key_enter` | 模拟中键短按，确认当前焦点项 |
| `ap_cmd dashboard key_home` | 模拟中键双击，返回主页 |
| `ap_cmd dashboard dashcam_count` | 显示录像数量与 SD 卡剩余空间 |
| `ap_cmd dashboard dashcam_trim` | 停止并落盘当前录像，然后删除整个 `/sd0/dashcam` 目录 |
| `ap_cmd dashboard dashcam_rec_start` | 手动开始录像 |
| `ap_cmd dashboard dashcam_rec_stop` | 手动停止并落盘录像 |
| `ap_cmd dashboard dashcam_turn_left` | 打开 Assist View |
| `ap_cmd dashboard dashcam_turn_off` | 关闭 Assist View |
| `ap_cmd dashboard np_erase [reboot]` | 清除配网信息；附加 `reboot` 立即重启 |
| `ap_cmd dashboard np_start_advertise` | 重新启动 BLE 配网广播 |

> `dashcam_trim` 会删除全部录像。再次开始录像时会重建该目录。

## FTP Server

设备联网后 FTP Server 自动启动，其 FTP 根目录为 SD 卡挂载点 `/sd0`，录像存放于 `/sd0/dashcam`。

使用 FTP 客户端连接设备当前所在网络的 IP 地址：

| 项目 | 值 |
| --- | --- |
| 协议 | FTP |
| 端口 | `21` |
| 用户名 | `bk7259` |
| 密码 | `123456` |
| 录像目录 | `/sd0/dashcam` |

通过目录浏览或文件下载即可取回录像。FTP 使用明文鉴权，请仅在可信局域网内使用。

## SD 卡 MTP

`mtp` CLI 在启动时已注册，但 USB MTP 设备不会自动启动。

1. 通过 USB 将开发板连接至 PC。
2. 在串口 CLI 执行 `ap_cmd mtp start`。
3. PC 将开发板枚举为 MTP 媒体设备，可浏览或拷贝 `/sd0` 下的文件，包括 `/sd0/dashcam` 中的录像。
4. 完成后执行 `ap_cmd mtp stop`。

支持的 MTP 命令：

| 命令 | 描述 |
| --- | --- |
| `ap_cmd mtp start` | 挂载 SD 卡并启动 MTP USB 设备 |
| `ap_cmd mtp stop` | 停止 MTP USB 设备 |
| `ap_cmd mtp status` | 显示 MTP 状态 |
| `ap_cmd mtp ls [path]` | 通过串口 CLI 列出 SD 卡文件；默认路径为 `/sd0` |

> 请在 PC 端文件拷贝完成后再停止 MTP，否则 USB 传输会被中断。
