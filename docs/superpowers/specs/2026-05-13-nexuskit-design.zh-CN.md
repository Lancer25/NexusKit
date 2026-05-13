# NexusKit 设计文档

日期：2026-05-13

## 1. 项目目标

NexusKit 是一个面向 Windows 和 Linux 的全新跨平台 C++17 组件库。当前 HikCommonDll 代码库只作为产品经验和功能参考，不作为旧项目兼容改造的约束。公共 API、模块命名、目录结构、依赖管理、文档和测试都应遵循现代开源 C++ 项目的规范。

项目需要提供以下可复用组件：

- 平台工具、文件、时间、字符串、线程、错误和系统辅助能力。
- 异步日志。
- JSON、XML、加密和通用数据工具。
- TCP、UDP、HTTP 和 WebSocket 网络通信。
- USB、UVC、热插拔和音频设备访问。
- HID 设备访问。
- 基于本地编译 FFmpeg 的音视频处理。
- 桌面和窗口采集。

NexusKit 必须可以作为独立项目长期维护。构建系统以 CMake 作为唯一入口，支持 Windows 上的 MSVC 2022，以及 Linux 上的 GCC 或 Clang。

## 2. 非目标

NexusKit 不保留旧 CommonUtils、HidTools、UsbTools、MediaTools 或 WebSocket 的公共 API。旧函数名、私有 SDK 概念、输出路径、工程文件和模块边界都可以废弃。

项目不依赖 hpr、hlog、HCNetUtils、HCUSBSDK、media_tube、SCE 等私有库。它们有价值的设计经验会被新的 NexusKit 模块或开源依赖替代。

Visual Studio solution 文件和 QMake 工程文件不再作为一等构建系统。迁移期间它们可以暂时保留，但 CMake 是权威构建入口。

## 3. 仓库结构

目标目录结构如下：

```text
NexusKit/
  CMakeLists.txt
  CMakePresets.json
  LICENSE
  README.md
  CHANGELOG.md
  CONTRIBUTING.md
  cmake/
    modules/
    deps/
  docs/
    architecture.md
    build.md
    modules/
  external/
    recipes/
  include/
    nexus/
      core/
      log/
      common/
      net/
      usb/
      hid/
      media/
      screen/
  src/
    core/
    log/
    common/
    net/
    usb/
    hid/
    media/
    screen/
  tests/
  examples/
  tools/
  build/
```

`include/nexus` 只存放稳定公共头文件，`src` 存放实现细节。三方库构建配方统一放在 `external/recipes` 或 `cmake/deps`，业务模块代码中不应散落临时下载依赖的逻辑。

## 4. 构建系统

构建系统基于 CMake，目标标准为 C++17。它必须支持：

- MSVC 2022 和 v143 工具集。
- Linux 上的 GCC 和 Clang。
- NexusKit 静态库和动态库构建。
- 模块可选构建。
- 三方库源码本地构建。
- 可安装的 CMake package export，让用户可以通过 `find_package(NexusKit CONFIG REQUIRED)` 集成。
- 统一输出到 `build/`，安装产物位于 `build/install`，分发产物位于 `build/dist`。

初始 CMake 选项：

```cmake
NEXUS_BUILD_SHARED
NEXUS_BUILD_TESTS
NEXUS_BUILD_EXAMPLES
NEXUS_BUILD_DOCS

NEXUS_BUILD_DEPS
NEXUS_BUILD_FFMPEG
NEXUS_BUILD_OPENSSL
NEXUS_BUILD_HIDAPI
NEXUS_BUILD_PORTAUDIO

NEXUS_ENABLE_COMMON
NEXUS_ENABLE_NET
NEXUS_ENABLE_USB
NEXUS_ENABLE_HID
NEXUS_ENABLE_MEDIA
NEXUS_ENABLE_SCREEN
```

每个模块必须暴露一个 `nexus::<module>` CMake target，并有对应的底层 target `nexus_<module>`，例如 `nexus::core` 和 `nexus_core`。

## 5. 依赖策略

NexusKit 以可复现方式构建或管理开源依赖。Git 和源码下载使用的本地代理为 `127.0.0.1:7897`，该配置需要写入 `docs/build.md`。

主要依赖：

| 依赖 | 用途 | 策略 |
| --- | --- | --- |
| spdlog | 日志后端 | 从源码获取/构建 |
| OpenSSL | 加密和 TLS | `NEXUS_BUILD_OPENSSL=ON` 时从源码构建 |
| hidapi | HID 访问 | 从源码构建 |
| asio | TCP/UDP 异步 I/O | 源码集成 |
| cpp-httplib | HTTP 客户端/服务端 | 源码集成 |
| websocketpp | WebSocket | 基于 standalone asio 源码集成 |
| FFmpeg | 音视频处理 | 本地源码编译 |
| PortAudio | 跨平台音频 I/O | 本地源码编译 |
| nlohmann_json | JSON 工具 | 源码集成 |
| pugixml | XML 工具 | 从源码构建 |
| Catch2 | 测试 | 源码集成 |
| libudev | Linux USB 热插拔 | 优先系统包检测 |
| PulseAudio/libpulse | Linux 音频 | 优先系统包检测 |
| X11/Xrandr/Xinerama | Linux 屏幕采集 | 优先系统包检测 |

依赖集成通过 `cmake/deps` 下的 CMake 配方统一管理。每个配方必须遵守 `NEXUS_BUILD_DEPS`，在 `external/recipes` 中记录来源和版本，并在通过 Git 克隆源码时使用 `NexusGit.cmake` 提供的共享 Git 配置。

FFmpeg 是一等依赖。项目需要提供独立 FFmpeg 构建配方，并记录 NASM/YASM、Perl、MSYS2 或其他平台相关前置工具。如果 Windows 最终需要基于 MSYS2 构建 FFmpeg，CMake 必须检测缺失前置条件，并给出清晰错误，而不是让构建在深层步骤中失败。

## 6. 模块架构

### nexus_core

`nexus_core` 提供共享基础能力：结果类型、状态码、错误、平台检测、文件系统辅助、时间工具、线程原语、字符串工具、动态库加载，以及小型操作系统资源 RAII 封装。

该模块应只依赖 C++17 标准库和最小平台 SDK。

### nexus_log

`nexus_log` 基于 spdlog 提供同步和异步日志能力。公共 API 不应直接暴露 spdlog 类型，除非有明确必要。它应支持控制台 sink、滚动文件 sink、日志级别、命名 logger，以及可行范围内的结构化上下文字段。

### nexus_common

`nexus_common` 包含从旧 CommonUtils 模块迁移或重写的高层工具：JSON、XML、加密辅助、字节缓冲、UUID 生成、数学辅助和安全转换工具。

旧项目内置的 jsoncpp/tinyxml 代码应替换为 nlohmann_json 和 pugixml。加密能力通过小而可测试的 OpenSSL 封装实现。

### nexus_net

`nexus_net` 替代 HCNetUtils，提供 TCP、UDP、HTTP 和 WebSocket 抽象。API 围绕明确的连接/会话对象、清晰的生命周期方法、异步事件回调和 RAII 清理设计。

初始实现优先支持 HTTP 和 WebSocket，因为旧 WebSocket 模块依赖缺失的 HCNetUtils。

### nexus_usb

`nexus_usb` 替代旧 UsbTools 和 HCUSBSDK 依赖。它提供 USB 枚举、设备描述符、热插拔监听、UVC 辅助和 USB 音频相关发现能力。Windows 实现可根据场景使用 SetupAPI、DirectShow、Media Foundation、WASAPI 和 WinUSB。Linux 实现可使用 libudev、V4L2、ALSA/PulseAudio 和系统 API。

旧 USB 代码可用于参考设备解析逻辑，但平台实现应在必要时重写，以提升正确性和资源管理质量。

### nexus_hid

`nexus_hid` 基于 hidapi 提供 HID 设备枚举和读写操作。它应暴露简单的 RAII 设备句柄和安全的缓冲区 API。

### nexus_media

`nexus_media` 替代旧 MediaTools 和 media_tube。它基于本地编译的 FFmpeg 提供解复用、解码、编码、重新封装、重采样、缩放、音频混合基础能力和格式转换。音频采集/播放优先使用 PortAudio，并在需要时提供可选原生后端。

媒体处理 API 应隐藏 FFmpeg 资源所有权规则，对外暴露清晰的 C++ 对象，例如 `Decoder`、`Encoder`、`Frame`、`Packet` 和 `MediaPipeline`。

### nexus_screen

`nexus_screen` 替代 SCE，提供桌面和窗口采集。Windows 初始实现可根据系统能力使用 GDI 或 Desktop Duplication。Linux 初始实现使用 X11，后续可扩展 PipeWire/Wayland。

## 7. 公共 API 风格

NexusKit API 应遵循：

- 使用 `namespace nexus::<module>`。
- 头文件采用小写 snake_case 命名。
- 使用 RAII 管理资源所有权。
- 使用 `std::string`、`std::vector`、`std::chrono`、`std::filesystem` 和 `std::function`。
- 对可恢复错误使用 `nexus::Result<T>` 和 `nexus::Status`。
- 异常只用于编程错误，或用于内部适配必须抛异常的依赖。
- 公共 API 中不暴露拥有语义的裸指针。
- 除非模块明确提供平台扩展，否则公共头文件中不包含平台头文件。

示例 include：

```cpp
#include <nexus/log/logger.h>
#include <nexus/net/websocket_client.h>
#include <nexus/hid/device.h>
#include <nexus/media/decoder.h>
#include <nexus/screen/capturer.h>
```

## 8. 错误处理

基础错误模型：

- `nexus::Status` 表示成功或失败，并携带错误码和消息。
- `nexus::Result<T>` 表示一个值或一个 `Status`。
- 错误码按模块分组。
- 原生平台错误作为可选诊断信息保留。

公共 API 应避免对可能有多种失败原因的操作返回含义模糊的布尔值。

## 9. 日志

日志由 `nexus_log` 提供，不通过散落在各模块中的全局宏实现。模块可以接收可选 logger，也可以使用命名模块 logger。日志配置优先支持代码式配置，文件配置后续再补。

旧 hlog 宏不保留。迁移旧代码时，日志调用应改写为新的 logger API。

## 10. 线程和异步模型

初始异步模型采用回调风格，并配合明确的生命周期所有权。网络、USB 热插拔、音频和屏幕模块不应创建不可控的全局线程。长时间运行的服务应表示为可以启动和停止的对象。

项目保持 C++17 兼容，不要求 C++20 coroutine。

## 11. 平台策略

平台相关实现文件应清晰拆分，例如：

```text
src/usb/device_monitor_win.cpp
src/usb/device_monitor_linux.cpp
src/screen/capturer_win.cpp
src/screen/capturer_x11.cpp
```

公共头文件应尽量保持平台无关。CMake 根据平台决定编译哪些实现文件。

Windows 平台可使用：

- SetupAPI 和 WinUSB 用于 USB。
- DirectShow 或 Media Foundation 用于 UVC 发现/采集。
- WASAPI 和 PortAudio 用于音频。
- Desktop Duplication 或 GDI 用于屏幕采集。

Linux 平台可使用：

- libudev 用于设备枚举和热插拔。
- V4L2 用于相机/UVC。
- ALSA/PulseAudio/PortAudio 用于音频。
- X11/Xrandr/Xinerama 用于屏幕采集。

## 12. 测试策略

项目应包含单元测试和集成示例。测试使用 CTest 和 Catch2。

初始测试范围：

- `Status` 和 `Result<T>`。
- 文件系统和字符串辅助工具。
- JSON/XML 解析辅助工具。
- 带已知测试向量的加密封装。
- HID 枚举冒烟测试，按硬件可用性开关。
- 使用生成媒体样本的 FFmpeg 封装测试。
- HTTP/WebSocket 网络回环测试。

依赖真实硬件的测试必须默认关闭，并通过显式选项启用。

## 13. 文档策略

文档应符合常见开源项目预期：

- `README.md`：项目概览、功能、快速开始和构建命令。
- `docs/build.md`：Windows、Linux、代理和三方库构建说明。
- `docs/architecture.md`：模块边界和依赖关系图。
- `docs/modules/*.md`：模块级 API 说明。
- `examples/`：可运行示例。
- 公共 API 稳定后可选接入 Doxygen。

文档需要说明哪些依赖会从源码构建，哪些依赖需要系统包提供。

## 14. 从旧代码迁移

旧代码是参考资料，不是兼容契约。

值得迁移或参考的内容：

- 字节缓冲、UUID、字符串转换、数学工具和部分通用工具。
- 旧 USB、HID、UVC、PulseAudio、WASAPI 和 V4L2 实现中的经验。
- MediaTools 中正确且可维护的 FFmpeg 使用模式。

应重写或废弃的内容：

- 绑定 hpr、hlog、HCNetUtils、HCUSBSDK、media_tube 和 SCE 的代码。
- 全局日志宏。
- 通过公共头文件暴露平台细节的代码。
- 可用 RAII 替代的手动资源管理。
- 旧 Visual Studio、QMake 和分散输出目录约定。
- 编码异常或已经无法帮助维护的乱码注释。

复用旧代码时，必须重新审查线程安全、资源所有权、编码问题、错误处理和平台假设。

## 15. 实施阶段

### 阶段 1：项目骨架

创建 CMake 根工程、CMake presets、标准目录、README、构建文档和初始 `nexus_core` target。即使 CI 后续再接入，也先建立可用于 CTest 的测试结构。

### 阶段 2：三方库构建配方

增加 spdlog、hidapi、OpenSSL、PortAudio 和 FFmpeg 的可复现下载/构建配方。记录代理配置和平台前置条件。

### 阶段 3：Core、Log 和 Common

实现 `nexus_core`、`nexus_log` 和一小部分 `nexus_common`。为 result/status、日志初始化和部分工具函数添加测试。

### 阶段 4：HID 和 USB

基于 hidapi 实现 `nexus_hid`。实现 Windows 和 Linux 下的 `nexus_usb` 枚举与热插拔。添加示例和硬件开关测试。

### 阶段 5：网络

优先实现 `nexus_net` 的 HTTP 和 WebSocket，再实现 TCP 和 UDP。添加回环测试和示例。

### 阶段 6：媒体

实现 FFmpeg 封装，支持基础解复用、解码、重新封装和音频处理。添加生成媒体样本和测试。集成 PortAudio 以支持简单音频 I/O。

### 阶段 7：屏幕采集

实现 Windows 和 Linux 下的 `nexus_screen`。添加帧采集示例，并提供与 `nexus_media` 的可选集成。

### 阶段 8：打包和完善

完善安装/export targets、示例、文档和分发目录。只有在新项目可用后，再清理旧工程文件。

## 16. 阶段 1 骨架验收标准

阶段 1 骨架完成时应满足：

- Windows/MSVC 2022 可以配置新工程。
- 提供 Windows/MSVC、Windows/Ninja 和 Linux/Ninja 的 CMake presets。
- `nexus_core` 可以成功构建。
- 第一批基于 Catch2 的测试可以通过 CTest 运行。
- 构建输出位于 `build/`。
- 安装/export targets 可以生成可消费的 `NexusKitConfig.cmake`。
- 项目具备 README、构建文档、架构文档和 `nexus_core` 模块文档。

`nexus_log` 和更多依赖驱动模块属于后续实施阶段。
