# NexusKit Phase 7B Windows DXGI Capture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the first real `nexus_screen` backend on Windows using DXGI Desktop Duplication to capture the primary display as BGRA frames.

**Architecture:** Keep public `nexus_screen` headers platform-neutral. Move Windows-specific capture code into private implementation files compiled only on Windows, and keep the existing unavailable backend for non-Windows builds or unsupported runtime environments.

**Tech Stack:** C++17, CMake, Catch2, Windows DXGI 1.2 / D3D11 APIs (`dxgi1_2.h`, `d3d11.h`), NexusKit `Status`/`Result`.

---

### Task 1: Backend Capability Tests

**Files:**
- Modify: `tests/screen/screen_tests.cpp`

- [ ] **Step 1: Add Windows-aware backend tests**

Update the existing backend test so it expects availability on Windows and graceful unavailability elsewhere:

```cpp
TEST_CASE("Screen capturer reports backend availability") {
    const auto backend = nexus::screen::screen_backend_info();

    CHECK(backend.name == "screen");
    CHECK_FALSE(backend.description.empty());

#if defined(_WIN32)
    CHECK(backend.available);
#else
    CHECK_FALSE(backend.available);
#endif
}
```

- [ ] **Step 2: Add Windows capture smoke test**

Add a smoke test that captures only when the backend reports available:

```cpp
TEST_CASE("Screen capturer captures primary display when backend is available") {
    const auto backend = nexus::screen::screen_backend_info();
    auto capturer = nexus::screen::ScreenCapturer::create();
    REQUIRE(capturer.ok());

    if (!backend.available || !capturer.value().is_available()) {
        const auto frame = capturer.value().capture_primary();
        REQUIRE_FALSE(frame.ok());
        CHECK(frame.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

    const auto frame = capturer.value().capture_primary();
    REQUIRE(frame.ok());
    CHECK(frame.value().width > 0);
    CHECK(frame.value().height > 0);
    CHECK(frame.value().pixel_format == nexus::screen::ScreenPixelFormat::bgra);
    CHECK(frame.value().data.size() ==
        static_cast<std::size_t>(frame.value().width) *
        static_cast<std::size_t>(frame.value().height) * 4u);
}
```

- [ ] **Step 3: Run tests to verify failure**

Run:

```powershell
cmake --build build/phase7a-check --target nexus_screen_tests --config Debug
build\phase7a-check\bin\Debug\nexus_screen_tests.exe
```

Expected on Windows: test fails because `screen_backend_info().available` is still false and the current capturer is unavailable.

### Task 2: Windows DXGI Backend Build Wiring

**Files:**
- Modify: `src/screen/CMakeLists.txt`
- Create: `src/screen/screen_win_dxgi.cpp`
- Modify: `src/screen/screen.cpp`

- [ ] **Step 1: Split generic fallback from Windows backend**

Keep `src/screen/screen.cpp` as the cross-platform public method file and fallback storage implementation for non-Windows builds. Add conditional compilation so Windows can use a DXGI-backed storage implementation without exposing Windows headers publicly.

- [ ] **Step 2: Add Windows source and libraries**

In `src/screen/CMakeLists.txt`, include `screen_win_dxgi.cpp` only when `WIN32`, and link private Windows libraries:

```cmake
if(WIN32)
    target_sources(nexus_screen PRIVATE screen_win_dxgi.cpp)
    target_link_libraries(nexus_screen PRIVATE d3d11 dxgi)
    target_compile_definitions(nexus_screen PRIVATE NEXUS_SCREEN_WITH_DXGI)
endif()
```

- [ ] **Step 3: Build to verify wiring**

Run:

```powershell
cmake --build build/phase7a-check --target nexus_screen_tests --config Debug
```

Expected: build compiles or fails only on missing backend symbols that Task 3 will implement.

### Task 3: DXGI Backend Implementation

**Files:**
- Modify: `src/screen/screen.cpp`
- Create/Modify: `src/screen/screen_win_dxgi.cpp`

- [ ] **Step 1: Add private backend factory seam**

Introduce a private function implemented differently per platform:

```cpp
namespace nexus::screen::detail {
std::unique_ptr<ScreenCapturerStorage> create_screen_capturer_storage(
    const ScreenCaptureOptions& options);
ScreenBackendInfo query_screen_backend_info();
}
```

`screen.cpp` should call these helpers from `screen_backend_info()` and `ScreenCapturer::create()`.

- [ ] **Step 2: Implement COM RAII helpers**

In `screen_win_dxgi.cpp`, use `Microsoft::WRL::ComPtr` from `<wrl/client.h>` or local `ComPtr` wrappers. Include:

```cpp
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
```

Use only private implementation types; no Windows headers in public headers.

- [ ] **Step 3: Create D3D11 device and duplication**

Implement `ScreenCapturerStorage` on Windows to:

1. Create a D3D11 device with `D3D11CreateDevice`.
2. Query `IDXGIDevice`.
3. Get adapter through `IDXGIDevice::GetAdapter`.
4. Enumerate primary output with `IDXGIAdapter::EnumOutputs(0, ...)`.
5. Query `IDXGIOutput1`.
6. Call `IDXGIOutput1::DuplicateOutput(device.Get(), ...)`.

If any step fails because duplication is unavailable, create a storage object whose `is_available()` returns false and whose `capture_primary()` returns `kFailedPrecondition`.

- [ ] **Step 4: Capture one primary frame**

Implement `capture_primary()` to:

1. Call `AcquireNextFrame` with a short timeout such as 500 ms.
2. Query acquired resource as `ID3D11Texture2D`.
3. Create a staging `D3D11_USAGE_STAGING` texture with `D3D11_CPU_ACCESS_READ`.
4. Copy acquired texture into staging texture.
5. Map staging texture with `D3D11_MAP_READ`.
6. Copy row-by-row into contiguous `ScreenFrame::data`.
7. Set `width`, `height`, `pixel_format = ScreenPixelFormat::bgra`, and `timestamp_ms`.
8. Always call `ReleaseFrame` after a successful `AcquireNextFrame`.

Use `DXGI_OUTDUPL_FRAME_INFO::LastPresentTime` when available; otherwise use a monotonic fallback timestamp.

- [ ] **Step 5: Map DXGI errors to NexusKit statuses**

Return:

- `kFailedPrecondition`: duplication unavailable, access lost, unsupported desktop/session state.
- `kUnavailable`: timeout/no new frame when backend exists but no frame was produced.
- `kInternal`: unexpected D3D/DXGI failures after setup.

### Task 4: Tests, Docs, and Verification

**Files:**
- Modify: `docs/modules/screen.md`
- Modify: `README.md` if status wording needs a real backend update.
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Run screen tests**

Run:

```powershell
cmake --build build/phase7a-check --target nexus_screen_tests --config Debug
build\phase7a-check\bin\Debug\nexus_screen_tests.exe
```

Expected on Windows with duplication support: smoke capture passes. If running in a session where DXGI duplication is unavailable, backend should report unavailable and the unavailable path should pass.

- [ ] **Step 2: Update docs**

Document:

- Windows DXGI Desktop Duplication is the first real backend.
- Captured primary frames are BGRA.
- Cursor composition is not included in Phase 7B.
- Multi-monitor selection and window capture are future work.
- Remote sessions, locked desktops, and unsupported GPU/session states may report unavailable.

- [ ] **Step 3: Run final verification**

Run:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
cmake -S . -B build/phase7b-dxgi -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF -DNEXUS_ENABLE_SCREEN=ON -DNEXUS_ENABLE_LOG=OFF -DNEXUS_ENABLE_COMMON=OFF -DNEXUS_ENABLE_NET=OFF -DNEXUS_ENABLE_HID=OFF -DNEXUS_ENABLE_USB=OFF -DNEXUS_ENABLE_MEDIA=OFF -DCMAKE_INSTALL_PREFIX=E:/Cloudview/HikCommonDll/build/install/phase7b-dxgi
cmake --build build/phase7b-dxgi --target nexus_screen_tests --config Debug
build\phase7b-dxgi\bin\Debug\nexus_screen_tests.exe
cmake --install build/phase7b-dxgi --config Debug
```

Expected: configure, build, tests, and install pass. Test output may take the unavailable path if the current Windows session does not support Desktop Duplication.

### Implementation Notes

- Use `DXGI_FORMAT_B8G8R8A8_UNORM` as the expected BGRA format. If the acquired texture format differs, return `kFailedPrecondition` in Phase 7B rather than adding conversion logic.
- Keep `ScreenCaptureOptions::include_cursor` accepted but not implemented. Phase 7B should ignore cursor composition and document that behavior.
- Avoid global COM initialization requirements. D3D11/DXGI device creation does not require public COM objects in the API.
- Keep all Windows headers inside `.cpp` files.
- Do not add platform headers to `include/nexus/screen/screen.h`.
