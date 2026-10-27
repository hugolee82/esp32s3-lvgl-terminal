# ESP32-S3 LVGL 智能桌面终端

基于 **ESP32-S3 + LVGL + SquareLine Studio** 的多功能桌面小终端。2.4 寸触摸屏，提供天气、音乐播放、百度语音助手（小智）、串口终端、日历时钟、小游戏等功能，界面切换带过渡动画。

![PlatformIO](https://img.shields.io/badge/PlatformIO-espressif32%406.5.0-orange)
![ESP32-S3](https://img.shields.io/badge/MCU-ESP32--S3-red)
![LVGL](https://img.shields.io/badge/LVGL-8.3.9-blue)
![UI](https://img.shields.io/badge/UI-SquareLine%201.5.0-purple)

## 功能特性

| 功能 | 说明 |
| --- | --- |
| 时钟 / 日历 | 连接 WiFi 后通过阿里云 NTP 自动对时（UTC+8），支持日历显示 |
| 天气预报 | 心知天气 API，显示实时天气 + 未来 7 天预报，联网后每小时自动刷新 |
| 音乐播放 | 读取 SD 卡根目录 MP3，支持选歌、播放/暂停、音量调节 |
| 小智语音助手 | 百度实时语音识别（ASR）+ MiniMax 大模型回答 + 百度 TTS 播报 |
| 串口终端 | 屏幕键盘输入并发送串口数据，实时显示接收内容 |
| 小游戏 | 2048、羊了个羊、消消乐、植物大战僵尸（来自 lvgl-games） |
| WiFi 管理 | 扫描附近热点、屏幕键盘输入密码、自动重连、配置保存到 NVS |
| 设置 | 屏幕亮度 PWM 调节、WiFi 配置 |
| 关于 | 项目信息与 GitHub 地址 |

界面入口：主界面可点击图标进入各功能，**下拉主界面进入设置，上滑设置页返回**。

## 硬件要求

- **开发板**：ESP32-S3 开发板，建议 **N16R8**（16MB Flash + 8MB PSRAM，OPI PSRAM）
- **屏幕**：2.4 寸 ST7789 SPI 屏，分辨率 **320×240**（竖屏 240×320，代码中旋转为横屏）
- **触摸**：CST816S 电容触摸屏（I2C）
- **存储**：TF / MicroSD 卡（FAT32，用于存放 MP3）
- **音频输出**：I2S 功放（如 MAX98357A）+ 喇叭
- **麦克风**：I2S 数字麦克风（如 INMP441）

### 引脚连接

**显示（TFT_eSPI / ST7789，SPI）**

| 信号 | GPIO |
| --- | --- |
| MOSI (SDA) | 7 |
| SCLK (SCL) | 15 |
| CS | 4 |
| DC | 6 |
| RST | 5 |
| BL（背光，PWM） | 16 |

**触摸（CST816S，I2C）**

| 信号 | GPIO |
| --- | --- |
| SDA | 3 |
| SCL | 18 |
| RST | 8 |
| IRQ | 46 |

**SD 卡（HSPI）**

| 信号 | GPIO |
| --- | --- |
| CS | 9 |
| SCK | 14 |
| MISO | 17 |
| MOSI | 10 |

**音频输出（I2S_NUM_1，ESP32-audioI2S）**

| 信号 | GPIO |
| --- | --- |
| BCLK | 40 |
| LRC (WS) | 41 |
| DOUT | 39 |

**麦克风输入（I2S_NUM_0，百度小智库默认值）**

| 信号 | GPIO |
| --- | --- |
| SCK (BCLK) | 11 |
| WS (LRC) | 13 |
| SD (DIN) | 12 |

> 引脚如有改动，请同步修改：`src/main.cpp`（触摸、分辨率）、`src/sduser.cpp`（SD）、`src/music.cpp`（I2S 输出），以及麦克风库的 `xiaozhi_ai.h`。

## 软件环境

- [PlatformIO](https://platformio.org/)（VSCode 插件或 CLI 均可）
- 平台版本：`espressif32 @ 6.5.0`，框架：Arduino
- LVGL `8.3.9`（UI 由 SquareLine Studio 1.5.0 按 LVGL 8.3.6 导出）
- 主要依赖（见 `platformio.ini`）：

```
lvgl/lvgl@^8.3.9
bodmer/TFT_eSPI@2.5.0
fbiego/CST816S@^1.1.1
bblanchon/ArduinoJson@^6.21.4
esphome/ESP32-audioI2S@^2.0.7
https://github.com/CaddonThaw/lvgl-games.git
https://github.com/CaddonThaw/baidu-xiaozhi.git
plageoj/UrlEncode@^1.0.1
gilmaimon/ArduinoWebsockets@^0.5.3
```

## 快速开始

### 1. 获取代码

```bash
git clone https://github.com/hugolee82/esp32s3-lvgl-terminal.git
cd esp32s3-lvgl-terminal
```

用 VSCode + PlatformIO 打开该目录，或直接使用 CLI。

### 2. 配置 TFT_eSPI（必做）

仓库中**不包含** TFT_eSPI 的屏幕配置（`.pio` 已被 git 忽略）。首次安装依赖后，编辑：

```
.pio/libdeps/esp32-s3-devkitc-1/TFT_eSPI/User_Setup.h
```

按下面的内容修改（只列关键项，其余保持默认）：

```cpp
#define ST7789_DRIVER
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

#define TFT_BL     16
#define TFT_BACKLIGHT_ON HIGH

#define TFT_MOSI   7
#define TFT_SCLK   15
#define TFT_CS     4
#define TFT_DC     6
#define TFT_RST    5
```

如果依赖还没下载，可以先执行一次 `pio pkg install` 再修改。

> 注意：重新安装依赖（`pio pkg update` / 删除 `.pio`）会覆盖该文件，需要重新配置。也可以在 `platformio.ini` 里用 `-D USER_SETUP_LOADED` + `build_flags` 的方式自行管理配置。

### 3. 填写 API Key

项目里用到了三组第三方服务，都需要换成你自己的：

| 服务 | 位置 | 说明 |
| --- | --- | --- |
| 心知天气 | `src/data.cpp` 顶部 `key` / `locations` | 默认 `武汉`，key 有免费额度与有效期 |
| 百度语音 | 依赖库 `baidu-xiaozhi/xiaozhi_ai.h` 中 `BAIDU_APP_ID` / `BAIDU_APP_KEY` / `BAIDU_SECRET_KEY` | 语音识别（ASR）与语音合成（TTS） |
| MiniMax | 依赖库 `baidu-xiaozhi/xiaozhi_minimax.h` 中 `MiniMaxKey` | **默认为空**，不填则语音助手无法回答 |

> `baidu-xiaozhi` 来自 `lib_deps`，直接修改 `.pio/libdeps` 下的文件在重装依赖后会丢失。建议 fork 一份库，把 `lib_deps` 换成自己的仓库地址。

### 4. 编译与烧录

`platformio.ini` 中上传口写死为 `COM7`、波特率 `921600`，请按实际情况修改：

```ini
upload_port = COM7        ; 改成你的串口（CH340 常见 COM3/COM7）
upload_speed = 921600
monitor_speed = 115200
```

然后：

```bash
pio run                   # 编译
pio run -t upload         # 烧录
pio device monitor        # 查看串口日志
```

### 5. 准备 SD 卡

- 格式化为 **FAT32**
- 把 MP3 文件放在**根目录**（文件名支持中文，UTF-8 编码）
- 音乐页进入时自动扫描根目录下的 `.mp3`（扩展名大小写均可）

## 使用说明

- **主界面**：点击图标进入天气 / 音乐 / 语音助手 / 游戏 / 串口 / 时间 / 关于；下拉呼出设置
- **设置页**：拖动滑块调背光；点击 WiFi 图标显示热点列表，选中后输入密码连接（配置会保存，下次开机自动重连）
- **天气页**：联网后自动更新，右上角按钮可手动刷新
- **音乐页**：滚轮选歌，开关控制播放/暂停，切歌按钮立即播放选中歌曲
- **小智页**：打开 "ON" 开关后对着麦克风说话，识别结果和回答会显示在文本框中，回答通过喇叭播放
- **串口页**：点击输入框弹出键盘，发送按钮输出到 `Serial`，接收区实时显示串口收到的内容
- **游戏页**：选择游戏后全屏运行，左上角返回按钮回到游戏列表

## 项目结构

```
├── platformio.ini          # PlatformIO 工程配置（开发板、分区、依赖、烧录参数）
├── include/
│   └── lv_conf.h           # LVGL 配置（16 位色、48KB 内存池、自定义字体等）
├── src/
│   ├── main.cpp            # 显示/触摸驱动、LVGL 初始化、setup/loop
│   ├── config.cpp/.h       # 全局状态结构体（WiFi、天气、音乐、串口、语音）
│   ├── task.cpp/.h         # FreeRTOS 任务与 LVGL 互斥锁
│   ├── wifiuser.cpp/.h     # WiFi 扫描、连接、状态刷新
│   ├── data.cpp/.h         # NTP 对时、心知天气请求与界面刷新
│   ├── music.cpp/.h        # SD 卡 MP3 扫描、I2S 播放、播放状态机
│   ├── sduser.cpp/.h       # SD 卡挂载（HSPI）
│   ├── xiaozhi.cpp/.h      # 语音助手状态机（对接 baidu-xiaozhi 库）
│   └── ui/                 # SquareLine Studio 导出的 UI 工程
│       └── src/
│           ├── screens/    # 各页面（Main/Set/Weather/Music/Aidoubao/...）
│           ├── fonts/      # 阿里妈妈大楷字体、16px 宋体
│           ├── images/     # 界面图片资源
│           └── ui.cpp/.h   # 页面初始化与事件回调
├── lib/                    # 预留
└── test/                   # 预留
```

## 实现要点

- **多任务划分**：LVGL 刷新跑在 Core 1；网络、音乐、串口、语音任务跑在 Core 0，通过递归互斥锁 `lvgl_mutex` 访问 UI
- **触摸防抖**：CST816S 在 I2C 读失败或抬手时上报的无效点会被过滤（`points`、坐标范围校验），避免"幽灵点击"
- **音乐状态机**：区分 `play_flag` / `start_flag` / `pause_flag`，播完自动复位开关；打开失败 3 次即停止，避免无限重试刷日志
- **语音回答播放**：TTS 音频复用音乐播放通道（`Audio.connecttohost`），回答期间暂停其他播放请求
- **省资源**：标签内容不变时不重复 `lv_label_set_text`，WiFi 列表仅在内容变化时刷新 roller

## 常见问题

**Q：编译报找不到 `lv_conf.h` 里的字体 / 颜色不对？**
确认 `platformio.ini` 保留了 `-D LV_CONF_INCLUDE_SIMPLE`，并且使用仓库内 `include/lv_conf.h`，不要改 `.pio/libdeps` 里的 LVGL 配置。

**Q：屏幕白屏 / 花屏 / 触摸没反应？**
检查 TFT_eSPI 的 `User_Setup.h` 是否按上面第 2 步配置，以及接线是否与引脚表一致。屏幕旋转在 `main.cpp` 中为 `tft.setRotation(3)`。

**Q：WiFi 连不上？**
先用手机确认热点可用；串口日志会打印失败状态码。密码错误或信号差时 `WiFi.status()` 为 6/4。

**Q：天气一直不刷新？**
查看串口 `[Weather]` 日志。常见原因是心知天气 key 过期或免费额度用完。

**Q：语音助手没反应？**
依次确认：MiniMax Key 已填写、百度三个 Key 有效、麦克风 I2S 引脚正确、`Xiaozhi` 开关为 ON。串口日志中会有 `[XIAOZHI]` 前缀的过程信息。

**Q：反复进出语音页后异常？**
当前版本退出时只删除任务，库内部 WebSocket 会话未完全释放（代码中有注释说明），必要时重启设备。

**Q：PlatformIO 报 `LockFileTimeoutError`？**
不要同时在 VSCode 插件和 CLI 里编译/上传，同一时间只保留一个操作。

## 致谢

- [LVGL](https://lvgl.io/) — 嵌入式 GUI 框架
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) — 显示驱动
- [SquareLine Studio](https://squareline.io/) — UI 设计工具
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) — 音频播放
- [lvgl-games](https://github.com/CaddonThaw/lvgl-games)、[baidu-xiaozhi](https://github.com/CaddonThaw/baidu-xiaozhi) — 小游戏与语音助手
- [心知天气](https://www.seniverse.com/)、[百度智能云](https://cloud.baidu.com/)、[MiniMax](https://www.minimax.io/) — 天气与语音服务

## 许可证

仓库暂未包含 `LICENSE` 文件。如需转载或商用，请先联系作者。第三方库与服务的授权请遵循各自协议。
