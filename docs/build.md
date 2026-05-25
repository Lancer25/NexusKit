# Build Guide

## Windows

Use MSVC 2022:

```powershell
# Debug
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug

# Release
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release
```

Build output is written under `build/windows-msvc-debug` or `build/windows-msvc-release`.

## Linux

Use the Linux preset from a shell with CMake and Ninja installed:

```bash
# Debug
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug

# Release
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
```

## Proxy

When fetching dependencies from GitHub behind a proxy, set the environment variables:

```powershell
$env:HTTP_PROXY="http://proxy.example.com:port"
$env:HTTPS_PROXY="http://proxy.example.com:port"
```

For Git directly:

```powershell
git config --global http.proxy http://proxy.example.com:port
git config --global https.proxy http://proxy.example.com:port
```

Windows Git may need the OpenSSL TLS backend when fetching through the proxy:

```powershell
git config --global http.sslBackend openssl
```

## Dependency Fetching

Dependency recipes live under `cmake/deps`.

By default, `NEXUS_BUILD_DEPS=ON`, so NexusKit may fetch or build declared dependencies from source. The default build currently fetches only dependencies that are actually needed by enabled targets, such as Catch2 when `NEXUS_BUILD_TESTS=ON`.

To disable source fetching, configure with `-DNEXUS_BUILD_DEPS=OFF` and provide the required packages through CMake package discovery:

```powershell
cmake -S . -B build/deps-off -DNEXUS_BUILD_DEPS=OFF -DNEXUS_BUILD_TESTS=ON
```

When dependency fetching is enabled, Git-based recipes use `NEXUS_GIT_CONFIG_ARGS`. The default is:

```cmake
http.sslBackend=openssl
```

If `HTTP_PROXY` or `HTTPS_PROXY` is set in the environment, CMake copies it into `NEXUS_GIT_PROXY` and passes it to dependency clones. You can also set it explicitly:

```powershell
cmake -S . -B build/proxy -DNEXUS_GIT_PROXY=http://proxy.example.com:port
```

Production dependency recipes are available for spdlog, nlohmann_json, pugixml, asio, cpp-httplib, websocketpp, hidapi, PortAudio, OpenSSL, and FFmpeg. Lightweight dependencies are included by future modules as needed. Heavy source builds are opt-in:

```powershell
# Individual features
cmake -S . -B build/hidapi -DNEXUS_BUILD_HIDAPI=ON
cmake -S . -B build/portaudio -DNEXUS_BUILD_PORTAUDIO=ON
cmake -S . -B build/openssl -DNEXUS_BUILD_OPENSSL=ON
cmake -S . -B build/ffmpeg -DNEXUS_BUILD_FFMPEG=ON
cmake -S . -B build/camera -DNEXUS_ENABLE_CAMERA=ON -DNEXUS_BUILD_LIBUVC=ON

# Full build (all modules, FFmpeg from source, GPL x264, camera via libuvc)
cmake --preset windows-msvc-release -B build/windows-msvc-release `
  -DNEXUS_ENABLE_SCREEN=ON `
  -DNEXUS_ENABLE_USB=ON `
  -DNEXUS_ENABLE_HID=ON -DNEXUS_BUILD_HIDAPI=ON `
  -DNEXUS_ENABLE_AUDIO=ON `
  -DNEXUS_ENABLE_MEDIA=ON -DNEXUS_BUILD_FFMPEG=ON `
  -DNEXUS_ENABLE_CAMERA=ON -DNEXUS_BUILD_LIBUVC=ON `
  -DNEXUS_FFMPEG_ENABLE_GPL=ON
```

OpenSSL source builds require Perl. On Windows they also require `nmake` from a Visual Studio developer prompt. FFmpeg source builds require NASM; on Windows they use MSYS2 bash with `make`, `nproc`, `nasm`, and a visible C compiler, and on Linux they require bash, make, pkg-config, and NASM.

`nexus_log` is enabled by default through `NEXUS_ENABLE_LOG=ON` and uses spdlog as a private backend.
`nexus_common` is enabled by default through `NEXUS_ENABLE_COMMON=ON` and uses nlohmann_json and pugixml as private backends for JSON and XML utilities.
`nexus_net` is enabled by default through `NEXUS_ENABLE_NET=ON` and uses cpp-httplib, standalone Asio, and websocketpp as private backends for HTTP, TCP, UDP, and WebSocket utilities. It depends on `nexus_log` for internal diagnostics, so `NEXUS_ENABLE_NET=ON` requires `NEXUS_ENABLE_LOG=ON`.
`nexus_hid` is enabled with `NEXUS_ENABLE_HID=ON` and uses hidapi as a private backend. To build hidapi from source, also set `NEXUS_BUILD_HIDAPI=ON`. It depends on `nexus_log` for internal diagnostics, so `NEXUS_ENABLE_HID=ON` requires `NEXUS_ENABLE_LOG=ON`.
`nexus_usb` is enabled with `NEXUS_ENABLE_USB=ON`. The initial implementation uses `nexus_hid` for HID-backed device discovery, so it requires both `NEXUS_ENABLE_LOG=ON` and `NEXUS_ENABLE_HID=ON`.
`nexus_media` is enabled with `NEXUS_ENABLE_MEDIA=ON`. The first media slice builds without FFmpeg and reports the backend as unavailable unless FFmpeg CMake targets are present. To use the source-built FFmpeg backend, also set `NEXUS_BUILD_FFMPEG=ON`. To use an existing FFmpeg install tree, leave `NEXUS_BUILD_FFMPEG=OFF` and set `NEXUS_FFMPEG_INSTALL_DIR` to a prefix containing `include/libavutil/avutil.h` plus FFmpeg libraries under `bin` on Windows or `lib` on Linux.
`nexus_screen` is enabled with `NEXUS_ENABLE_SCREEN=ON` and defaults to `OFF`. On Windows it uses DXGI Desktop Duplication through the Windows SDK (D3D11, DXGI); MSVC 2022 provides these headers and libraries by default. On Linux it uses X11 when development files are available at configure time; install `libx11-dev` (Debian/Ubuntu) or `libX11-devel` (Fedora/RHEL) to enable the X11 backend. Builds without a supported backend still compile but report the screen backend as unavailable at runtime.
`NEXUS_BUILD_TESTS` defaults to `ON` and builds the Catch2 unit-test suite. Set `-DNEXUS_BUILD_TESTS=OFF` to skip test targets and the Catch2 dependency.
`NEXUS_BUILD_EXAMPLES` defaults to `ON` and builds runnable example programs such as `nexus_example_screen_capture` and `nexus_example_media_probe`. Set `-DNEXUS_BUILD_EXAMPLES=OFF` to skip example targets.

For the Windows FFmpeg source build, the tested MSYS2 UCRT64 package set is:

```powershell
C:\msys64\usr\bin\bash.exe -lc "pacman -Sy --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf make nasm"
```

When `NEXUS_BUILD_FFMPEG=ON`, the source recipe exposes imported CMake targets for downstream modules:

```cmake
FFmpeg::avutil
FFmpeg::swresample
FFmpeg::swscale
FFmpeg::avcodec
FFmpeg::avformat
FFmpeg::avfilter
FFmpeg::avdevice
FFmpeg::FFmpeg
```

On Windows, the source-built FFmpeg DLLs and import libraries are installed under `build/<preset>/deps/ffmpeg/bin`.

### FFmpeg GPL and x264

Set `-DNEXUS_FFMPEG_ENABLE_GPL=ON` to build FFmpeg with `--enable-gpl --enable-libx264`. This enables the GPL-licensed x264 H.264 encoder as a source-built ExternalProject dependency. x264 compiles as a static library (`libx264.a`) via the MSYS2 MinGW toolchain (Windows) or native GCC (Linux). Requirements:

- **Windows**: MSYS2 UCRT64 with `bash`, `make`, `nproc`, `nasm`, and `gcc`
- **Linux**: `bash`, `make`, `pkg-config`, and `nasm`

When GPL is enabled, x264 is built first, then FFmpeg's configure step picks up the x264 pkg-config path automatically.

### Camera and libuvc

`nexus_camera` (`NEXUS_ENABLE_CAMERA=ON`) requires libuvc on Windows. Set `NEXUS_BUILD_LIBUVC=ON` to build libuvc from source via FetchContent. libuvc in turn requires libusb, which is built via FetchContent with a custom CMake recipe (`cmake/deps/LibUSB.cmake`). On MSVC, POSIX headers (`sys/time.h`, `pthread.h`) are provided as compatibility shims under `build/<preset>/_deps/libuvc-compat`.

No extra system packages are needed beyond the MSYS2 environment used for FFmpeg/x264.

## Install Layout

Install prefixes default to `build/install/<preset>`.
Runtime artifacts are produced under `build/<preset>/bin`.
Libraries are produced under `build/<preset>/lib`.

## Release Builds

Release builds use `CMAKE_BUILD_TYPE=Release` with compiler optimizations enabled (`/O2` on MSVC, `-O3` on GCC/Clang) and `NDEBUG` defined. The presets do not strip debug symbols automatically; use an explicit strip or install-strip step if a smaller redistributable package requires it.

All Debug presets have corresponding Release presets:

| Platform | Debug Preset | Release Preset |
|----------|-------------|----------------|
| Windows MSVC | `windows-msvc-debug` | `windows-msvc-release` |
| Windows Ninja | `windows-ninja-debug` | `windows-ninja-release` |
| Linux Ninja | `linux-debug` | `linux-release` |

For the MSVC multi-config generator, pass `--config Release` to build in Release mode. The build preset already includes `"configuration": "Release"`, so `cmake --build --preset windows-msvc-release` produces a Release binary without extra flags.
