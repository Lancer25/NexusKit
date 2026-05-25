# Phase 13A CI And Release Checks Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add lightweight CI and release checks for docs, default builds, install layout, and downstream package consumption.

**Architecture:** A Python standard-library checker protects release-facing Markdown rules. A tiny CMake consumer project validates the installed `NexusKitConfig.cmake` and exported default targets. GitHub Actions runs these checks on Windows and Linux using the existing presets where possible.

**Tech Stack:** Python unittest, CMake, CTest, GitHub Actions, PowerShell, Bash.

---

### Task 1: Release Docs Checker

**Files:**
- Create: `scripts/check_release_docs.py`
- Create: `tests/scripts/test_check_release_docs.py`

- [ ] **Step 1: Write failing unit tests**

Create unittest coverage for:

- stale changelog heading detection,
- PowerShell continuation at the end of a fenced block,
- full-build block missing `NEXUS_ENABLE_SCREEN=ON`,
- HID-level hotplug text that does not say it is not planned.

Run:

```powershell
python -m unittest tests.scripts.test_check_release_docs -v
```

Expected before implementation: failure because `scripts.check_release_docs` does not exist.

- [ ] **Step 2: Implement the checker**

Implement `scripts/check_release_docs.py` with standard-library helpers that can validate either the real repo or temporary test roots.

- [ ] **Step 3: Verify tests and real docs**

Run:

```powershell
python -m unittest tests.scripts.test_check_release_docs -v
python scripts/check_release_docs.py --dir .
```

Expected: unit tests pass and the real docs print `Release docs checks passed.`

### Task 2: Install Consumer Smoke

**Files:**
- Create: `tests/packaging/consumer/CMakeLists.txt`
- Create: `tests/packaging/consumer/main.cpp`

- [ ] **Step 1: Add the consumer project**

Create a minimal downstream project that calls `find_package(NexusKit CONFIG REQUIRED)`, includes public headers, and links the default targets `nexus::core`, `nexus::log`, `nexus::common`, and `nexus::net`.

- [ ] **Step 2: Verify against a local install**

Run after a local install:

```powershell
cmake -S tests/packaging/consumer -B build/phase13a-consumer -DCMAKE_PREFIX_PATH="$PWD/build/phase13a-prefix"
cmake --build build/phase13a-consumer --config Release
```

Expected: configure and build succeed.

### Task 3: GitHub Actions Workflow

**Files:**
- Create: `.github/workflows/ci.yml`

- [ ] **Step 1: Add docs job**

Run:

```powershell
python scripts/check_release_docs.py --dir .
python scripts/check_public_comments.py --dir .
```

- [ ] **Step 2: Add default build matrix**

Run default configure/build/test/install on:

- `windows-latest` using `windows-msvc-release`,
- `ubuntu-latest` using `linux-release`.

- [ ] **Step 3: Add consumer smoke**

After install, configure and build `tests/packaging/consumer` with `CMAKE_PREFIX_PATH` pointing at the installed prefix.

### Task 4: Final Verification

**Files:**
- Read: all files modified in this phase.

- [ ] **Step 1: Run script tests**

```powershell
python -m unittest tests.scripts.test_check_release_docs -v
```

- [ ] **Step 2: Run docs checks**

```powershell
python scripts/check_release_docs.py --dir .
python scripts/check_public_comments.py --dir .
```

- [ ] **Step 3: Run default build/install smoke**

```powershell
cmake --preset windows-msvc-release -B build/phase13a-release -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_EXAMPLES=ON
cmake --build build/phase13a-release --config Release
ctest --test-dir build/phase13a-release -C Release --output-on-failure
cmake --install build/phase13a-release --config Release --prefix build/phase13a-prefix
cmake -S tests/packaging/consumer -B build/phase13a-consumer -DCMAKE_PREFIX_PATH="$PWD/build/phase13a-prefix"
cmake --build build/phase13a-consumer --config Release
```

- [ ] **Step 4: Commit and push**

```powershell
git add .github/workflows/ci.yml scripts/check_release_docs.py tests/scripts/test_check_release_docs.py tests/packaging/consumer/CMakeLists.txt tests/packaging/consumer/main.cpp docs/superpowers/specs/2026-05-25-phase-13a-ci-release-checks-design.md docs/superpowers/plans/2026-05-25-phase-13a-ci-release-checks.md
git commit -m "ci: add release documentation checks"
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
git push
```
