# NexusKit Phase 7A Screen Skeleton Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the initial `nexus_screen` module boundary, public capture API, build target, tests, and documentation.

**Architecture:** Phase 7A intentionally does not implement platform screen capture. It provides stable public types and a move-only `ScreenCapturer` facade whose default backend reports clear unavailable/precondition statuses until Windows/Linux implementations are added.

**Tech Stack:** C++17, CMake, Catch2, NexusKit `Status`/`Result`.

---

### Task 1: Public API and Tests

**Files:**
- Create: `include/nexus/screen/export.h`
- Create: `include/nexus/screen/screen.h`
- Create: `tests/screen/screen_tests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing screen tests**

Add tests for the intended API:

```cpp
#include <catch2/catch_test_macros.hpp>

#include <nexus/screen/screen.h>

TEST_CASE("Screen capturer reports backend availability") {
    const auto backend = nexus::screen::screen_backend_info();

    CHECK(backend.name == "screen");
    CHECK_FALSE(backend.available);
    CHECK_FALSE(backend.description.empty());
}

TEST_CASE("Screen capturer creates closed unavailable facade") {
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());
    CHECK_FALSE(capturer.value().is_available());

    const auto frame = capturer.value().capture_primary();
    REQUIRE_FALSE(frame.ok());
    CHECK(frame.status().code() == nexus::StatusCode::kFailedPrecondition);
}
```

- [ ] **Step 2: Wire tests and verify compile failure**

Add a `nexus_screen_tests` block to `tests/CMakeLists.txt`, then run:

```powershell
cmake -S . -B build/phase7a-red -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF -DNEXUS_ENABLE_SCREEN=ON -DNEXUS_ENABLE_LOG=OFF -DNEXUS_ENABLE_COMMON=OFF -DNEXUS_ENABLE_NET=OFF -DNEXUS_ENABLE_HID=OFF -DNEXUS_ENABLE_USB=OFF -DNEXUS_ENABLE_MEDIA=OFF
cmake --build build/phase7a-red --target nexus_screen_tests --config Debug
```

Expected: compile/configure failure because the `nexus_screen` target and headers do not exist.

### Task 2: Module Build Target and Skeleton Implementation

**Files:**
- Create: `src/screen/CMakeLists.txt`
- Create: `src/screen/screen.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add `nexus_screen` target**

Create a static/shared-aware module target matching existing NexusKit patterns. Link publicly to `nexus::core`, install target as `screen`, and define `NEXUS_SCREEN_SHARED` / `NEXUS_SCREEN_BUILDING_LIBRARY` for shared builds.

- [ ] **Step 2: Add top-level screen subdirectory**

In root `CMakeLists.txt`, add:

```cmake
if(NEXUS_ENABLE_SCREEN)
    add_subdirectory(src/screen)
endif()
```

- [ ] **Step 3: Implement minimal skeleton**

Implement `screen_backend_info`, `ScreenCapturer::create`, `ScreenCapturer::is_available`, `ScreenCapturer::capture_primary`, and `ScreenCapturer::close`. `capture_primary` returns `StatusCode::kFailedPrecondition` with message `"screen capture backend is not available"`.

- [ ] **Step 4: Run screen tests**

Run:

```powershell
cmake --build build/phase7a-red --target nexus_screen_tests --config Debug
build\phase7a-red\bin\Debug\nexus_screen_tests.exe
```

Expected: tests pass.

### Task 3: Documentation and Verification

**Files:**
- Create: `docs/modules/screen.md`
- Modify: `README.md`
- Modify: `docs/architecture.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update docs**

Document the Phase 7A scope, API, unavailable backend behavior, and planned Windows/Linux capture backends.

- [ ] **Step 2: Run final verification**

Run:

```powershell
cmake -S . -B build/phase7a-check -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF -DNEXUS_ENABLE_SCREEN=ON -DNEXUS_ENABLE_LOG=OFF -DNEXUS_ENABLE_COMMON=OFF -DNEXUS_ENABLE_NET=OFF -DNEXUS_ENABLE_HID=OFF -DNEXUS_ENABLE_USB=OFF -DNEXUS_ENABLE_MEDIA=OFF -DCMAKE_INSTALL_PREFIX=E:/Cloudview/HikCommonDll/build/install/phase7a-check
cmake --build build/phase7a-check --target nexus_screen_tests --config Debug
build\phase7a-check\bin\Debug\nexus_screen_tests.exe
cmake --install build/phase7a-check --config Debug
```

Expected: configure, build, tests, and install pass.
