# NexusKit 开发迭代记录

## 2026-05-18 — 项目初始化

- 创建 CMake 项目骨架，C++17 标准
- 实现 `nexus_core`（Status/Result 错误处理基础）
- 实现 `nexus_log`（spdlog 诊断日志，头文件私有）
- 实现 `nexus_common`（字符串、时间、二进制 I/O、线程、JSON、XML）
- 实现 `nexus_net`（HTTP/HTTPS、TCP、UDP、WebSocket 同步/异步）
- 配置 Catch2 测试框架，CMakePresets windows-msvc-debug / linux-debug
- 建立 RAII、公共头文件不暴露后端、Doxygen 注释等核心规范

## 2026-05-19 上午 — 扩展模块基础

- 实现 `nexus_screen`：DXGI 屏幕/窗口/区域采集，X11/Linux 后端
- 实现 `nexus_usb`：USB 设备枚举，HID 报告 I/O，热插拔监控
- 实现 `nexus_hid`：HID 设备枚举和特性报告
- 实现 `nexus_media`：FFmpeg 媒体探测、解码、编码、复用
- 屏幕采集支持光标叠加、显示枚举、可见窗口枚举
- 媒体模块支持 AAC/H264 编码，MP4/MPEG-PS 复用
- 添加 WAV/PPM 轻量文件写入器

## 2026-05-19 下午 — 音频/摄像头 + 发布构建

- 实现 `nexus_audio`（捕获）：WASAPI/PulseAudio PCM int16 采集
- 实现 `nexus_camera`：libuvc/V4L2 USB 摄像头采集
- WASAPI 音量/静音控制（IAudioEndpointVolume）
- PulseAudio 完整捕获后端
- UVC/V4L2 摄像头控制（亮度、对比度、曝光等）
- 添加 Release 构建预设
- 添加 x264 GPL ExternalProject 源构建
- 添加 libusb/libuvc 集成（Windows 摄像头支持）
- 修复 MediaMuxer H.264 extradata（AV_CODEC_FLAG_GLOBAL_HEADER）
- 修复 FFmpeg MinGW 运行时 DLL 缺失
- 清理 118 个 stale build/phase 目录
- 清理 116 个历史 plan/spec 文件
- 重写 README 为 0.1.0 发布就绪状态

## 2026-05-19 晚 — 审计修复

- 修复 CMakePresets 配置问题
- 修复 x264 版本锁定
- 修复缺失的导出宏和文档
- 去重清理

## 2026-05-19 晚 — README/CLAUDE 同步

- 启用 screen 模块默认配置修复
- README 与项目状态同步（模块表、构建命令）
- CLAUDE.md 状态更新为完整 245 测试

## 2026-05-20 — 优化和扩展

### 新增功能
- **TcpListener**（nexus_net）：move-only RAII TCP 监听器，异步 accept 通过 background worker 回调交付 TcpClient
- **AudioPlayer**（nexus_audio）：WASAPI IAudioRenderClient / PulseAudio pa_simple PCM int16 播放，pull 模型回调，空帧结束流
- **remux_file**（nexus_media）：FFmpeg 流拷贝 remux，不解码编码直接复制容器流（等价于 ffmpeg -c copy）
- **CameraCapturer::default_device**：静态辅助方法返回第一个摄像头设备

### 优化修复
- USB 参数验证顺序修正：空 report / max_bytes==0 在任何设备状态下返回 kInvalidArgument
- nexus_common 二进制 I/O 辅助函数添加边界检查
- Pugixml CMake 配方清理，移除未使用的 fetch 开关
- Thread 类 API 添加 Doxygen 注释

### TcpListener 技术要点
- `acceptor.open(protocol)` → `set_option(reuse_address)` → `bind(ep)` → `listen(backlog)` 顺序
- `socket->release()` / `client.storage_->socket.assign()` 跨 io_context 传递已接收 socket
- 通过 friend class 声明访问 TcpClient 私有存储
- TcpClientStorage 提取到共享内部头文件 tcp_internal.h
- MSVC C4996 抑制（release() 在 Windows 8.1 前已弃用，目标平台 Windows 10+）

### AudioPlayer 技术要点
- WASAPI render 后端：CoCreateInstance MMDeviceEnumerator → GetDefaultAudioEndpoint(eRender) → Activate → Initialize(shared, PCM int16) → GetService(IAudioRenderClient) → polling GetCurrentPadding + GetBuffer + ReleaseBuffer
- PulseAudio render 后端：pa_simple_new(PA_STREAM_PLAYBACK, PA_SAMPLE_S16LE) → pa_simple_write
- Pull 模型：回调返回 Result<AudioFrame>，空 data 表示流结束
- 2 秒缓冲区，20ms chunk 渲染循环

## 2026-05-20 — 文档同步

- 更新所有模块文档（audio、net、media、camera）与新 API 同步
- CHANGELOG 添加所有优化和扩展条目
- README/CLAUDE 测试计数更新为 262/262
- 模块表更新：TCP listener、stream-copy remux、playback、volume/mute、default device
- 创建统一的开发迭代记录（本文件）

## 测试状态

| 日期 | 测试数 | 通过率 |
|------|--------|--------|
| 2026-05-18 | 初始 | - |
| 2026-05-19 | 245 | 100% |
| 2026-05-20 | 262 | 100% |

## 模块完成度

| 模块 | 状态 | 后端 |
|------|------|------|
| nexus_core | 完成 | 纯 C++17 |
| nexus_log | 完成 | spdlog |
| nexus_common | 完成 | 纯 C++17 + 平台工具 |
| nexus_net | 完成 | Asio + cpp-httplib + websocketpp + OpenSSL |
| nexus_screen | 完成 | DXGI (Win) / X11+Xrandr (Linux) |
| nexus_usb | 完成 | HID 后端，热插拔监控 |
| nexus_hid | 完成 | HID 设备枚举 |
| nexus_media | 完成 | FFmpeg + x264 |
| nexus_audio | 完成 | WASAPI (Win) / PulseAudio (Linux) |
| nexus_camera | 完成 | libuvc (Win) / V4L2 (Linux) |
