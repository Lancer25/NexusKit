# NexusKit Phase 1 Skeleton Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create the initial NexusKit open-source C++17 project skeleton with CMake, documentation, tests, and a minimal `nexus_core` module.

**Architecture:** Phase 1 establishes the new project without preserving legacy APIs. It creates the standard repository layout, CMake package structure, `nexus::core` target, `Status`/`Result<T>` foundations, Catch2-based tests, and first-pass documentation. Old source files remain untouched during this phase.

**Tech Stack:** C++17, CMake 3.24+, MSVC 2022/v143, GCC/Clang, CTest, Catch2 via FetchContent.

---

## Scope

This plan implements only Phase 1 from the NexusKit design. It does not build FFmpeg, OpenSSL, hidapi, PortAudio, USB, HID, media, network, or screen modules. Those are separate follow-up plans.

## File Structure

Create:

- `CMakeLists.txt`: top-level build entry, project options, output directories, install/export setup, and subdirectory wiring.
- `CMakePresets.json`: standard configure presets for Windows and Linux.
- `cmake/NexusKitConfig.cmake.in`: package config template for installed consumers.
- `include/nexus/core/version.h`: version constants and `version_string()`.
- `include/nexus/core/export.h`: platform export macros for compiled `nexus_core` symbols.
- `include/nexus/core/status.h`: `nexus::Status`, `nexus::StatusCode`, and helpers.
- `include/nexus/core/result.h`: `nexus::Result<T>` value-or-status type.
- `src/core/CMakeLists.txt`: `nexus_core` library target and install rules for public headers.
- `src/core/version.cpp`: implementation of `version_string()`.
- `tests/CMakeLists.txt`: Catch2 integration and test registration.
- `tests/core/status_result_tests.cpp`: unit tests for `Status` and `Result<T>`.
- `examples/CMakeLists.txt`: examples entry point.
- `examples/core_status/CMakeLists.txt`: example target definition.
- `examples/core_status/main.cpp`: minimal `nexus::Status`/`Result<T>` usage example.
- `README.md`: project overview and quick start.
- `docs/build.md`: build instructions, proxy notes, output layout.
- `docs/architecture.md`: module boundaries for the new architecture.
- `docs/modules/core.md`: `nexus_core` module notes.
- `LICENSE`: MIT license placeholder with the current year and project name.
- `CONTRIBUTING.md`: contributor workflow.
- `CHANGELOG.md`: initial changelog.

Modify:

- No legacy source files.
- Existing design documents remain unchanged.

## Task 1: Top-Level CMake Skeleton

**Files:**
- Create: `CMakeLists.txt`
- Create: `CMakePresets.json`
- Create: `cmake/NexusKitConfig.cmake.in`

- [ ] **Step 1: Create a failing configure baseline**

Run:

```powershell
cmake --preset windows-msvc-debug
```

Expected: FAIL because `CMakePresets.json` does not exist.

- [ ] **Step 2: Add the top-level CMake project**

Create `CMakeLists.txt` with:

```cmake
cmake_minimum_required(VERSION 3.24)

project(NexusKit
    VERSION 0.1.0
    DESCRIPTION "A cross-platform C++17 component library for systems, devices, networking, media, and screen capture."
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(NEXUS_BUILD_SHARED "Build NexusKit libraries as shared libraries" OFF)
option(NEXUS_BUILD_TESTS "Build NexusKit tests" ON)
option(NEXUS_BUILD_EXAMPLES "Build NexusKit examples" ON)
option(NEXUS_BUILD_DOCS "Build NexusKit documentation targets" OFF)

option(NEXUS_BUILD_DEPS "Build third-party dependencies from source" ON)
option(NEXUS_BUILD_FFMPEG "Build FFmpeg from source" OFF)
option(NEXUS_BUILD_OPENSSL "Build OpenSSL from source" OFF)
option(NEXUS_BUILD_HIDAPI "Build hidapi from source" OFF)
option(NEXUS_BUILD_PORTAUDIO "Build PortAudio from source" OFF)

option(NEXUS_ENABLE_COMMON "Enable nexus_common" OFF)
option(NEXUS_ENABLE_NET "Enable nexus_net" OFF)
option(NEXUS_ENABLE_USB "Enable nexus_usb" OFF)
option(NEXUS_ENABLE_HID "Enable nexus_hid" OFF)
option(NEXUS_ENABLE_MEDIA "Enable nexus_media" OFF)
option(NEXUS_ENABLE_SCREEN "Enable nexus_screen" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NEXUS_BUILD_SHARED)
    set(NEXUS_LIBRARY_TYPE SHARED)
else()
    set(NEXUS_LIBRARY_TYPE STATIC)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(src/core)

if(NEXUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NEXUS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NexusKitConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusKitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NexusKitConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NexusKit"
)

install(
    EXPORT NexusKitTargets
    NAMESPACE nexus::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NexusKit"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/NexusKitConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/NexusKitConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NexusKit"
)
```

- [ ] **Step 3: Add CMake presets**

Create `CMakePresets.json` with:

```json
{
  "version": 6,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 24,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "windows-msvc-debug",
      "displayName": "Windows MSVC Debug",
      "generator": "Visual Studio 17 2022",
      "architecture": "x64",
      "binaryDir": "${sourceDir}/build/windows-msvc-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_INSTALL_PREFIX": "${sourceDir}/build/install/windows-msvc-debug",
        "NEXUS_BUILD_TESTS": "ON",
        "NEXUS_BUILD_EXAMPLES": "ON"
      }
    },
    {
      "name": "windows-ninja-debug",
      "displayName": "Windows Ninja Debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/windows-ninja-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_INSTALL_PREFIX": "${sourceDir}/build/install/windows-ninja-debug",
        "NEXUS_BUILD_TESTS": "ON",
        "NEXUS_BUILD_EXAMPLES": "ON"
      }
    },
    {
      "name": "linux-debug",
      "displayName": "Linux Debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/linux-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_INSTALL_PREFIX": "${sourceDir}/build/install/linux-debug",
        "NEXUS_BUILD_TESTS": "ON",
        "NEXUS_BUILD_EXAMPLES": "ON"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "windows-msvc-debug",
      "configurePreset": "windows-msvc-debug"
    },
    {
      "name": "windows-ninja-debug",
      "configurePreset": "windows-ninja-debug"
    },
    {
      "name": "linux-debug",
      "configurePreset": "linux-debug"
    }
  ],
  "testPresets": [
    {
      "name": "windows-msvc-debug",
      "configurePreset": "windows-msvc-debug",
      "output": {
        "outputOnFailure": true
      }
    },
    {
      "name": "windows-ninja-debug",
      "configurePreset": "windows-ninja-debug",
      "output": {
        "outputOnFailure": true
      }
    },
    {
      "name": "linux-debug",
      "configurePreset": "linux-debug",
      "output": {
        "outputOnFailure": true
      }
    }
  ]
}
```

- [ ] **Step 4: Add package config template**

Create `cmake/NexusKitConfig.cmake.in` with:

```cmake
@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/NexusKitTargets.cmake")

check_required_components(NexusKit)
```

- [ ] **Step 5: Run configure and verify the next expected failure**

Run:

```powershell
cmake --preset windows-msvc-debug
```

Expected: FAIL because `src/core` does not exist yet. This confirms the root project and preset are being read.

## Task 2: Minimal nexus_core API

**Files:**
- Create: `include/nexus/core/version.h`
- Create: `include/nexus/core/export.h`
- Create: `include/nexus/core/status.h`
- Create: `include/nexus/core/result.h`
- Create: `src/core/CMakeLists.txt`
- Create: `src/core/version.cpp`

- [ ] **Step 1: Add public version header**

Create `include/nexus/core/version.h` with:

```cpp
#pragma once

#include <string>

#include <nexus/core/export.h>

namespace nexus::core {

constexpr int version_major = 0;
constexpr int version_minor = 1;
constexpr int version_patch = 0;

NEXUS_CORE_API std::string version_string();

} // namespace nexus::core
```

- [ ] **Step 2: Add export macro header**

Create `include/nexus/core/export.h` with:

```cpp
#pragma once

#if defined(_WIN32) && defined(NEXUS_CORE_SHARED)
#if defined(NEXUS_CORE_BUILDING_LIBRARY)
#define NEXUS_CORE_API __declspec(dllexport)
#else
#define NEXUS_CORE_API __declspec(dllimport)
#endif
#else
#define NEXUS_CORE_API
#endif
```

- [ ] **Step 3: Add Status API**

Create `include/nexus/core/status.h` with:

```cpp
#pragma once

#include <string>
#include <utility>

namespace nexus {

enum class StatusCode {
    kOk = 0,
    kCancelled,
    kInvalidArgument,
    kNotFound,
    kAlreadyExists,
    kPermissionDenied,
    kResourceExhausted,
    kFailedPrecondition,
    kUnavailable,
    kInternal,
    kUnknown
};

class Status {
public:
    Status() = default;

    Status(StatusCode code, std::string message)
        : code_(code), message_(std::move(message)) {}

    static Status ok_status() {
        return Status();
    }

    static Status invalid_argument(std::string message) {
        return Status(StatusCode::kInvalidArgument, std::move(message));
    }

    static Status not_found(std::string message) {
        return Status(StatusCode::kNotFound, std::move(message));
    }

    static Status internal(std::string message) {
        return Status(StatusCode::kInternal, std::move(message));
    }

    bool ok() const {
        return code_ == StatusCode::kOk;
    }

    StatusCode code() const {
        return code_;
    }

    const std::string& message() const {
        return message_;
    }

private:
    StatusCode code_ = StatusCode::kOk;
    std::string message_;
};

} // namespace nexus
```

- [ ] **Step 4: Add Result API**

Create `include/nexus/core/result.h` with:

```cpp
#pragma once

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include <nexus/core/status.h>

namespace nexus {

template <typename T>
class Result {
public:
    Result(const T& value) : storage_(value) {}
    Result(T&& value) : storage_(std::move(value)) {}
    Result(Status status) : storage_(std::move(status)) {
        if (std::get<Status>(storage_).ok()) {
            throw std::invalid_argument("Result cannot hold an OK status without a value");
        }
    }

    bool ok() const {
        return std::holds_alternative<T>(storage_);
    }

    const T& value() const& {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::get<T>(storage_);
    }

    T& value() & {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::get<T>(storage_);
    }

    T&& value() && {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::move(std::get<T>(storage_));
    }

    const Status& status() const {
        if (ok()) {
            static const Status ok_status = Status::ok_status();
            return ok_status;
        }
        return std::get<Status>(storage_);
    }

private:
    std::variant<T, Status> storage_;
};

} // namespace nexus
```

- [ ] **Step 5: Add core implementation**

Create `src/core/version.cpp` with:

```cpp
#include <nexus/core/version.h>

namespace nexus::core {

std::string version_string() {
    return "0.1.0";
}

} // namespace nexus::core
```

- [ ] **Step 6: Add core CMake target**

Create `src/core/CMakeLists.txt` with:

```cmake
add_library(nexus_core ${NEXUS_LIBRARY_TYPE}
    version.cpp
)

add_library(nexus::core ALIAS nexus_core)

target_include_directories(nexus_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_core PUBLIC cxx_std_17)

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_core
        PUBLIC NEXUS_CORE_SHARED
        PRIVATE NEXUS_CORE_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_core PROPERTIES
    OUTPUT_NAME nexus_core
    EXPORT_NAME core
)

install(
    TARGETS nexus_core
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/nexus"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
```

- [ ] **Step 7: Run configure again**

Run:

```powershell
cmake --preset windows-msvc-debug
```

Expected: FAIL because `tests` or `examples` does not exist yet while both options are enabled.

## Task 3: Tests with Catch2 and CTest

**Files:**
- Create: `tests/CMakeLists.txt`
- Create: `tests/core/status_result_tests.cpp`

- [ ] **Step 1: Add tests CMake**

Create `tests/CMakeLists.txt` with:

```cmake
include(FetchContent)

if(NEXUS_BUILD_DEPS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4
        GIT_CONFIG http.sslBackend=openssl
    )

    FetchContent_MakeAvailable(Catch2)
else()
    find_package(Catch2 3 REQUIRED CONFIG)
endif()

add_executable(nexus_core_tests
    core/status_result_tests.cpp
)

target_link_libraries(nexus_core_tests
    PRIVATE
        nexus::core
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(nexus_core_tests)
```

- [ ] **Step 2: Add core tests**

Create `tests/core/status_result_tests.cpp` with:

```cpp
#include <stdexcept>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/core/version.h>

TEST_CASE("Status defaults to ok") {
    const nexus::Status status;

    REQUIRE(status.ok());
    REQUIRE(status.code() == nexus::StatusCode::kOk);
    REQUIRE(status.message().empty());
}

TEST_CASE("Status stores failure code and message") {
    const auto status = nexus::Status::invalid_argument("bad input");

    REQUIRE_FALSE(status.ok());
    REQUIRE(status.code() == nexus::StatusCode::kInvalidArgument);
    REQUIRE(status.message() == "bad input");
}

TEST_CASE("Result stores a value") {
    const nexus::Result<int> result(42);

    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
    REQUIRE(result.status().ok());
}

TEST_CASE("Result stores an error") {
    const nexus::Result<int> result(nexus::Status::not_found("missing"));

    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().code() == nexus::StatusCode::kNotFound);
    REQUIRE(result.status().message() == "missing");
}

TEST_CASE("Result rejects ok status without value") {
    REQUIRE_THROWS_AS(nexus::Result<int>(nexus::Status::ok_status()), std::invalid_argument);
}

TEST_CASE("Version string matches initial version") {
    REQUIRE(nexus::core::version_string() == "0.1.0");
}
```

- [ ] **Step 3: Run configure and expect examples failure**

Run:

```powershell
cmake --preset windows-msvc-debug
```

Expected: FAIL because `examples` does not exist yet.

## Task 4: Example Program

**Files:**
- Create: `examples/CMakeLists.txt`
- Create: `examples/core_status/CMakeLists.txt`
- Create: `examples/core_status/main.cpp`

- [ ] **Step 1: Add examples root**

Create `examples/CMakeLists.txt` with:

```cmake
add_subdirectory(core_status)
```

- [ ] **Step 2: Add core status example target**

Create `examples/core_status/CMakeLists.txt` with:

```cmake
add_executable(nexus_example_core_status
    main.cpp
)

target_link_libraries(nexus_example_core_status
    PRIVATE
        nexus::core
)
```

- [ ] **Step 3: Add example source**

Create `examples/core_status/main.cpp` with:

```cpp
#include <iostream>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/core/version.h>

namespace {

nexus::Result<int> parse_demo_value(bool valid) {
    if (!valid) {
        return nexus::Status::invalid_argument("demo value is invalid");
    }
    return 7;
}

} // namespace

int main() {
    std::cout << "NexusKit " << nexus::core::version_string() << '\n';

    const auto result = parse_demo_value(true);
    if (!result.ok()) {
        std::cerr << result.status().message() << '\n';
        return 1;
    }

    std::cout << "value=" << result.value() << '\n';
    return 0;
}
```

- [ ] **Step 4: Configure, build, and run tests**

Run:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Expected:

```text
100% tests passed
```

If `FetchContent` fails because network is blocked, rerun configure after setting the proxy:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
cmake --preset windows-msvc-debug
```

Expected: CMake downloads Catch2 through the proxy.

## Task 5: Project Documentation

**Files:**
- Create: `README.md`
- Create: `docs/build.md`
- Create: `docs/architecture.md`
- Create: `docs/modules/core.md`
- Create: `LICENSE`
- Create: `CONTRIBUTING.md`
- Create: `CHANGELOG.md`

- [ ] **Step 1: Add README**

Create `README.md` with:

```markdown
# NexusKit

NexusKit is a cross-platform C++17 component library for Windows and Linux. It provides reusable modules for platform utilities, logging, common data helpers, networking, USB/HID, media processing, and screen capture.

The project is a new implementation inspired by earlier internal component-library experience. It does not preserve legacy APIs or private SDK dependencies.

## Status

NexusKit is in early development. The first milestone establishes the CMake project skeleton, `nexus_core`, tests, examples, and documentation.

## Requirements

- CMake 3.24 or newer
- C++17 compiler
- MSVC 2022 on Windows, or GCC/Clang on Linux
- Git for dependency downloads

## Quick Start

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

If dependency downloads need the local proxy:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
cmake --preset windows-msvc-debug
```

## Modules

- `nexus_core`: status/result types, versioning, platform foundations.
- `nexus_log`: asynchronous logging.
- `nexus_common`: JSON, XML, crypto, and common data utilities.
- `nexus_net`: TCP, UDP, HTTP, and WebSocket.
- `nexus_usb`: USB, UVC, hotplug, and device discovery.
- `nexus_hid`: HID access.
- `nexus_media`: FFmpeg-based audio/video processing.
- `nexus_screen`: desktop and window capture.
```

- [ ] **Step 2: Add build documentation**

Create `docs/build.md` with:

```markdown
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

## Install Layout

Install prefixes default to `build/install/<preset>`.
Runtime artifacts are produced under `build/<preset>/bin`.
Libraries are produced under `build/<preset>/lib`.
```

- [ ] **Step 3: Add architecture documentation**

Create `docs/architecture.md` with:

```markdown
# Architecture

NexusKit is organized as focused CMake targets with public headers under `include/nexus` and implementation files under `src`.

## Modules

- `nexus_core` is the lowest-level module and depends only on C++17 and minimal platform SDKs.
- `nexus_log` provides logging and depends on `nexus_core` and spdlog.
- `nexus_common` provides JSON, XML, crypto, and utility helpers.
- `nexus_net` provides TCP, UDP, HTTP, and WebSocket.
- `nexus_usb` provides USB and UVC device support.
- `nexus_hid` provides HID access through hidapi.
- `nexus_media` provides FFmpeg-based media processing.
- `nexus_screen` provides desktop and window capture.

## Rules

Public headers must not expose platform headers unless the API is explicitly platform-specific.
Owning resources must use RAII.
Recoverable failures should return `nexus::Status` or `nexus::Result<T>`.
```

- [ ] **Step 4: Add core module docs**

Create `docs/modules/core.md` with:

```markdown
# nexus_core

`nexus_core` contains the foundational API shared by other NexusKit modules.

## Headers

- `<nexus/core/status.h>`: `nexus::Status` and `nexus::StatusCode`.
- `<nexus/core/result.h>`: `nexus::Result<T>`.
- `<nexus/core/version.h>`: version constants and `nexus::core::version_string()`.

## Error Model

Use `nexus::Status` when a function only reports success or failure.
Use `nexus::Result<T>` when a function returns either a value or a failure.
```

- [ ] **Step 5: Add license**

Create `LICENSE` with:

```text
MIT License

Copyright (c) 2026 NexusKit contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

- [ ] **Step 6: Add contributing guide**

Create `CONTRIBUTING.md` with:

```markdown
# Contributing

NexusKit uses CMake, C++17, CTest, and focused module boundaries.

Before submitting changes:

1. Configure the project with the relevant preset.
2. Build the project.
3. Run CTest.
4. Add or update tests for behavior changes.
5. Keep public headers platform-neutral unless a platform extension is intentional.

Do not introduce private SDK dependencies.
```

- [ ] **Step 7: Add changelog**

Create `CHANGELOG.md` with:

```markdown
# Changelog

## 0.1.0 - Unreleased

- Start NexusKit as a new cross-platform C++17 component library.
- Add initial CMake project skeleton.
- Add `nexus_core` with status/result foundations.
- Add initial tests, examples, and documentation.
```

## Task 6: Final Verification

**Files:**
- Read: all files created by Tasks 1-5.

- [ ] **Step 1: Scan for placeholders**

Run:

```powershell
$patterns = @('TB' + 'D', 'TO' + 'DO', 'FIX' + 'ME', 'Hpr' + 'Compat', 'cloud' + 'view')
Get-ChildItem CMakeLists.txt,CMakePresets.json,cmake,include,src,tests,examples,docs,README.md,CONTRIBUTING.md,CHANGELOG.md,LICENSE -Recurse |
    Select-String -Pattern $patterns
```

Expected: no matches.

- [ ] **Step 2: Configure**

Run:

```powershell
cmake --preset windows-msvc-debug
```

Expected: configure succeeds and writes files under `build/windows-msvc-debug`.

- [ ] **Step 3: Build**

Run:

```powershell
cmake --build --preset windows-msvc-debug
```

Expected: `nexus_core`, `nexus_core_tests`, and `nexus_example_core_status` build successfully.

- [ ] **Step 4: Test**

Run:

```powershell
ctest --preset windows-msvc-debug
```

Expected:

```text
100% tests passed
```

- [ ] **Step 5: Install**

Run:

```powershell
cmake --install build/windows-msvc-debug
```

Expected:

```text
build/install/windows-msvc-debug/include/nexus/core/status.h
build/install/windows-msvc-debug/include/nexus/core/result.h
build/install/windows-msvc-debug/include/nexus/core/version.h
build/install/windows-msvc-debug/lib/cmake/NexusKit/NexusKitConfig.cmake
```

- [ ] **Step 6: Commit if repository is initialized**

Run:

```powershell
git status --short
```

If this is a git repository, commit:

```powershell
git add CMakeLists.txt CMakePresets.json cmake include src tests examples docs README.md CONTRIBUTING.md CHANGELOG.md LICENSE
git commit -m "feat: add NexusKit phase 1 skeleton"
```

If this is not a git repository, skip the commit and report that the workspace has no `.git` directory.
