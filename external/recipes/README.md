# Third-Party Dependency Recipes

NexusKit keeps dependency decisions centralized. CMake recipes live under `cmake/deps`, while this directory documents dependency versions, source locations, and platform notes.

## Current Recipes

| Dependency | Version | Source revision | CMake recipe | Purpose |
| --- | --- | --- | --- | --- |
| spdlog | v1.14.1 | `27cb4c76708608465c413f6d0e6b8d99a4d84302` | `cmake/deps/Spdlog.cmake` | Logging backend |
| nlohmann_json | v3.11.3 | `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03` | `cmake/deps/NlohmannJson.cmake` | JSON utilities |
| pugixml | v1.14 | `db78afc2b7d8f043b4bc6b185635d949ea2ed2a8` | `cmake/deps/Pugixml.cmake` | XML utilities |
| asio | 1.30.2 | `42a93679dc4c8c5caf3d3082542f1bfa2438271d` | `cmake/deps/Asio.cmake` | TCP/UDP async I/O |
| cpp-httplib | v0.15.3 | `5c00bbf36ba8ff47b4fb97712fc38cb2884e5b98` | `cmake/deps/CppHttplib.cmake` | HTTP client/server |
| websocketpp | 0.8.2 | `0e4241727c199e208ceb41134e840ba9968cf181` | `cmake/deps/Websocketpp.cmake` | WebSocket transport |
| hidapi | 0.14.0 | `73d292a8f4d18f7fb532b9829b9a26ca8d864419` | `cmake/deps/Hidapi.cmake` | HID device access |
| PortAudio | v19.7.0 | `147dd722548358763a8b649b3e4b41dfffbcfbb6` | `cmake/deps/PortAudio.cmake` | Audio I/O |
| OpenSSL | 3.3.1 | `243b18a4c9e2865caf7901ec4506e899cfc34d7c` | `cmake/deps/OpenSSLSource.cmake` | Crypto and TLS |
| FFmpeg | n7.0.1 | `47f70eda3e2ff003a787e512afd07b0c266f7a70` | `cmake/deps/FFmpeg.cmake` | Media processing |
| Catch2 | v3.5.4 | `abb467ecd60fae9a727afca033c1eb5d20af2c12` | `cmake/deps/Catch2.cmake` | Unit tests |

## Rules

- A dependency recipe must honor `NEXUS_BUILD_DEPS`.
- When `NEXUS_BUILD_DEPS=ON`, the recipe may fetch or build the dependency from source.
- When `NEXUS_BUILD_DEPS=OFF`, the recipe must use `find_package` or another explicit user-provided path.
- Git-based recipes should use `NEXUS_GIT_EFFECTIVE_CONFIG_ARGS`.
- Git-based recipes should pin immutable commit IDs, not mutable branch or tag names.
- Production dependencies must be documented here before they are used by a module.

## Heavy Source Builds

OpenSSL and FFmpeg are managed as `ExternalProject` builds because their upstream build systems are not CMake-first. They are opt-in at the top level:

```powershell
cmake -S . -B build/openssl -DNEXUS_BUILD_OPENSSL=ON
cmake -S . -B build/ffmpeg -DNEXUS_BUILD_FFMPEG=ON
```

On Windows, OpenSSL expects a Visual Studio developer environment with `nmake` and Perl. FFmpeg expects MSYS2 bash with `make`, `nproc`, `nasm`, and a visible C compiler. The recipes fail during configure with explicit prerequisite messages when those tools are missing.
