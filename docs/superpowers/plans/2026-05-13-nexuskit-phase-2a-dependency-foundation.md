# NexusKit Phase 2A Dependency Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move third-party dependency fetching into a reusable CMake dependency layer and make Catch2 the first dependency recipe.

**Architecture:** Phase 2A does not add production modules yet. It creates a `cmake/deps` layer with shared Git fetch configuration, a Catch2 recipe, and dependency documentation. Tests consume Catch2 through the recipe instead of declaring FetchContent directly.

**Tech Stack:** CMake 3.24+, C++17, FetchContent, Git over proxy `127.0.0.1:7897`, Catch2 v3.5.4, CTest.

---

## Scope

This plan implements Phase 2A only:

- Create reusable dependency CMake helpers.
- Move Catch2 fetching/finding into `cmake/deps/Catch2.cmake`.
- Keep `NEXUS_BUILD_DEPS=ON/OFF` semantics.
- Document dependency recipe conventions.
- Verify existing `nexus_core` tests still pass.

This plan does not add spdlog, nlohmann_json, pugixml, hidapi, OpenSSL, PortAudio, or FFmpeg. Those belong to later Phase 2 plans.

## File Structure

Create:

- `cmake/deps/NexusGit.cmake`: shared Git options for CMake dependency recipes.
- `cmake/deps/Catch2.cmake`: Catch2 dependency recipe.
- `external/recipes/README.md`: third-party recipe policy and version table.

Modify:

- `CMakeLists.txt`: add the dependency helper path and include `NexusGit`.
- `tests/CMakeLists.txt`: replace inline FetchContent logic with `include(Catch2)`.
- `docs/build.md`: document dependency recipe behavior.
- `docs/architecture.md`: mention the dependency layer.
- `docs/superpowers/specs/2026-05-13-nexuskit-design.md`: note that Phase 2 uses `cmake/deps` recipes.
- `docs/superpowers/specs/2026-05-13-nexuskit-design.zh-CN.md`: same note in Chinese.

## Task 1: Dependency Helper Path and Git Settings

**Files:**
- Create: `cmake/deps/NexusGit.cmake`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing configure check**

Run:

```powershell
cmake -S . -B build/phase2a-check -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF
```

Expected: configure currently succeeds, but Catch2 fetch logic still lives in `tests/CMakeLists.txt`. This is the baseline before refactoring.

- [ ] **Step 2: Add dependency module path to top-level CMake**

Modify `CMakeLists.txt` after the `include(CMakePackageConfigHelpers)` line:

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps")

include(NexusGit)
```

- [ ] **Step 3: Create shared Git dependency settings**

Create `cmake/deps/NexusGit.cmake` with:

```cmake
include_guard(GLOBAL)

set(NEXUS_GIT_SSL_BACKEND "openssl" CACHE STRING "Git TLS backend used by FetchContent dependency clones")
set(NEXUS_GIT_CONFIG_ARGS "http.sslBackend=${NEXUS_GIT_SSL_BACKEND}" CACHE STRING "Git -c style config entries passed to FetchContent")

function(nexus_print_dependency_mode dependency_name mode)
    message(STATUS "Nexus dependency ${dependency_name}: ${mode}")
endfunction()
```

- [ ] **Step 4: Reconfigure and expect no behavior change yet**

Run:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
cmake -S . -B build/phase2a-check -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF
```

Expected: configure succeeds. Catch2 may still be declared by `tests/CMakeLists.txt`.

## Task 2: Catch2 Dependency Recipe

**Files:**
- Create: `cmake/deps/Catch2.cmake`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Create Catch2 recipe**

Create `cmake/deps/Catch2.cmake` with:

```cmake
include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(Catch2 "FetchContent v3.5.4")

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG abb467ecd60fae9a727afca033c1eb5d20af2c12
        GIT_CONFIG ${NEXUS_GIT_CONFIG_ARGS}
    )

    FetchContent_MakeAvailable(Catch2)
else()
    nexus_print_dependency_mode(Catch2 "find_package")
    find_package(Catch2 3 REQUIRED CONFIG)
endif()
```

- [ ] **Step 2: Simplify tests CMake to use the recipe**

Replace the dependency block at the top of `tests/CMakeLists.txt`:

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
```

with:

```cmake
include(Catch2)
```

The full `tests/CMakeLists.txt` should become:

```cmake
include(Catch2)

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

- [ ] **Step 3: Reconfigure with dependency fetching enabled**

Run:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
cmake -S . -B build/phase2a-check -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF
```

Expected output contains:

```text
Nexus dependency Catch2: FetchContent v3.5.4
```

Expected result: configure succeeds.

- [ ] **Step 4: Verify `NEXUS_BUILD_DEPS=OFF` uses package discovery**

Run:

```powershell
cmake -S . -B build/phase2a-deps-off -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_DEPS=OFF -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF
```

Expected output contains:

```text
Nexus dependency Catch2: find_package
```

Expected result on a machine without installed Catch2: configure fails with `Could not find a package configuration file provided by "Catch2"`. This confirms it did not fetch from the network.

## Task 3: Dependency Recipe Documentation

**Files:**
- Create: `external/recipes/README.md`
- Modify: `docs/build.md`
- Modify: `docs/architecture.md`

- [ ] **Step 1: Add recipe policy**

Create `external/recipes/README.md` with:

```markdown
# Third-Party Dependency Recipes

NexusKit keeps dependency decisions centralized. CMake recipes live under `cmake/deps`, while this directory documents dependency versions, source locations, and platform notes.

## Current Recipes

| Dependency | Version | Source revision | CMake recipe | Purpose |
| --- | --- | --- | --- | --- |
| Catch2 | v3.5.4 | `abb467ecd60fae9a727afca033c1eb5d20af2c12` | `cmake/deps/Catch2.cmake` | Unit tests |

## Rules

- A dependency recipe must honor `NEXUS_BUILD_DEPS`.
- When `NEXUS_BUILD_DEPS=ON`, the recipe may fetch or build the dependency from source.
- When `NEXUS_BUILD_DEPS=OFF`, the recipe must use `find_package` or another explicit user-provided path.
- Git-based recipes should use `NEXUS_GIT_CONFIG_ARGS`.
- Git-based recipes should pin immutable commit IDs, not mutable branch or tag names.
- Production dependencies must be documented here before they are used by a module.
```

- [ ] **Step 2: Update build guide dependency section**

Replace the `## Dependency Fetching` section in `docs/build.md` with:

```markdown
## Dependency Fetching

Dependency recipes live under `cmake/deps`.

By default, `NEXUS_BUILD_DEPS=ON`, so NexusKit may fetch or build declared dependencies from source. The current Phase 2A recipe is Catch2 for tests.

To disable source fetching, configure with `-DNEXUS_BUILD_DEPS=OFF` and provide the required packages through CMake package discovery:

```powershell
cmake -S . -B build/deps-off -DNEXUS_BUILD_DEPS=OFF -DNEXUS_BUILD_TESTS=ON
```

When dependency fetching is enabled, Git-based recipes use `NEXUS_GIT_CONFIG_ARGS`. The default is:

```cmake
http.sslBackend=openssl
```
```

- [ ] **Step 3: Update architecture docs**

Add this section to `docs/architecture.md` after the opening paragraph:

```markdown
## Dependency Layer

Third-party CMake recipes live under `cmake/deps`. Module and test CMake files include dependency recipes instead of declaring `FetchContent` directly. This keeps proxy, Git, and `NEXUS_BUILD_DEPS` behavior consistent across the project.
```

## Task 4: Design Document Alignment

**Files:**
- Modify: `docs/superpowers/specs/2026-05-13-nexuskit-design.md`
- Modify: `docs/superpowers/specs/2026-05-13-nexuskit-design.zh-CN.md`

- [ ] **Step 1: Update English design dependency strategy**

In `docs/superpowers/specs/2026-05-13-nexuskit-design.md`, append this paragraph after the dependency table:

```markdown
Dependency integration is centralized through CMake recipes under `cmake/deps`. Each recipe must honor `NEXUS_BUILD_DEPS`, document its source/version in `external/recipes`, and use shared Git settings from `NexusGit.cmake` when cloning from Git.
```

- [ ] **Step 2: Update Chinese design dependency strategy**

In `docs/superpowers/specs/2026-05-13-nexuskit-design.zh-CN.md`, append this paragraph after the dependency table:

```markdown
依赖集成通过 `cmake/deps` 下的 CMake 配方统一管理。每个配方必须遵守 `NEXUS_BUILD_DEPS`，在 `external/recipes` 中记录来源和版本，并在通过 Git 克隆源码时使用 `NexusGit.cmake` 提供的共享 Git 配置。
```

- [ ] **Step 3: Verify UTF-8 readability of Chinese doc**

Run:

```powershell
Get-Content -Encoding UTF8 -LiteralPath docs\superpowers\specs\2026-05-13-nexuskit-design.zh-CN.md | Select-Object -First 8
```

Expected: readable Chinese text, not mojibake.

## Task 5: Verification

**Files:**
- Read all files created or modified by Tasks 1-4.

- [ ] **Step 1: Scan new Phase 2A files for placeholders**

Run:

```powershell
$patterns = @('TB' + 'D', 'TO' + 'DO', 'FIX' + 'ME', 'Status::ok\(', 'Status ok\(', 'Hpr' + 'Compat', 'cloud' + 'view')
Get-ChildItem cmake/deps,external/recipes,tests,docs/build.md,docs/architecture.md,docs/superpowers/specs -Recurse |
    Select-String -Pattern $patterns
```

Expected: no matches.

- [ ] **Step 2: Clean Phase 2A verification build dirs**

Run:

```powershell
$root = (Resolve-Path '.').Path
$targets = @('build\phase2a-check', 'build\phase2a-deps-off')
foreach ($item in $targets) {
    $resolved = (Resolve-Path $item -ErrorAction SilentlyContinue).Path
    if ($resolved -and $resolved.StartsWith($root)) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
```

Expected: command completes without deleting anything outside the workspace.

- [ ] **Step 3: Configure default Phase 2A build**

Run:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
cmake -S . -B build/phase2a-check -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=ON
```

Expected output contains:

```text
Nexus dependency Catch2: FetchContent v3.5.4
```

Expected result: configure succeeds.

- [ ] **Step 4: Build default Phase 2A build**

Run:

```powershell
cmake --build build/phase2a-check --config Debug
```

Expected: `nexus_core`, `nexus_core_tests`, and `nexus_example_core_status` build successfully.

- [ ] **Step 5: Run tests**

Run:

```powershell
ctest --test-dir build/phase2a-check -C Debug --output-on-failure
```

Expected:

```text
100% tests passed, 0 tests failed out of 6
```

- [ ] **Step 6: Verify dependency-off behavior**

Run:

```powershell
cmake -S . -B build/phase2a-deps-off -G "Visual Studio 17 2022" -A x64 -DNEXUS_BUILD_DEPS=OFF -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=OFF
```

Expected output contains:

```text
Nexus dependency Catch2: find_package
```

Expected result on this machine if Catch2 is not installed: configure fails with a `Catch2Config.cmake` not found error, and no `_deps/catch2-src` directory is created under `build/phase2a-deps-off`.

- [ ] **Step 7: Commit if repository is initialized**

Run:

```powershell
git status --short
```

If this is a git repository, commit:

```powershell
git add CMakeLists.txt cmake/deps tests docs external/recipes
git commit -m "build: add NexusKit dependency recipe foundation"
```

If this is not a git repository, skip the commit and report that the workspace has no `.git` directory.
