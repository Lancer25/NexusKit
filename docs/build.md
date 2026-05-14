# Build Guide

## Windows

Use MSVC 2022:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Build output is written under `build/windows-msvc-debug`.

## Linux

Use the Linux preset from a shell with CMake and Ninja installed:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

## Proxy

When fetching dependencies from GitHub, use the local proxy:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
```

For Git directly:

```powershell
git config --global http.proxy http://127.0.0.1:7897
git config --global https.proxy http://127.0.0.1:7897
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
cmake -S . -B build/proxy -DNEXUS_GIT_PROXY=http://127.0.0.1:7897
```

Production dependency recipes are available for spdlog, nlohmann_json, pugixml, asio, cpp-httplib, websocketpp, hidapi, PortAudio, OpenSSL, and FFmpeg. Lightweight dependencies are included by future modules as needed. Heavy source builds are opt-in:

```powershell
cmake -S . -B build/hidapi -DNEXUS_BUILD_HIDAPI=ON
cmake -S . -B build/portaudio -DNEXUS_BUILD_PORTAUDIO=ON
cmake -S . -B build/openssl -DNEXUS_BUILD_OPENSSL=ON
cmake -S . -B build/ffmpeg -DNEXUS_BUILD_FFMPEG=ON
```

OpenSSL source builds require Perl. On Windows they also require `nmake` from a Visual Studio developer prompt. FFmpeg source builds require NASM; on Windows they use MSYS2 bash with `make`, `nproc`, `nasm`, and a visible C compiler, and on Linux they require bash, make, pkg-config, and NASM.

`nexus_log` is enabled by default through `NEXUS_ENABLE_LOG=ON` and uses spdlog as a private backend.
`nexus_common` is enabled by default through `NEXUS_ENABLE_COMMON=ON` and uses nlohmann_json and pugixml as private backends for JSON and XML utilities.
`nexus_net` is enabled by default through `NEXUS_ENABLE_NET=ON` and uses cpp-httplib, standalone Asio, and websocketpp as private backends for HTTP, TCP, UDP, and WebSocket utilities. It depends on `nexus_log` for internal diagnostics, so `NEXUS_ENABLE_NET=ON` requires `NEXUS_ENABLE_LOG=ON`.
`nexus_hid` is enabled with `NEXUS_ENABLE_HID=ON` and uses hidapi as a private backend. To build hidapi from source, also set `NEXUS_BUILD_HIDAPI=ON`. It depends on `nexus_log` for internal diagnostics, so `NEXUS_ENABLE_HID=ON` requires `NEXUS_ENABLE_LOG=ON`.

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

## Install Layout

Install prefixes default to `build/install/<preset>`.
Runtime artifacts are produced under `build/<preset>/bin`.
Libraries are produced under `build/<preset>/lib`.
