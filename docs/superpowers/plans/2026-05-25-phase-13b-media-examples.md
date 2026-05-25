# Phase 13B Media Examples Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the direct-FFmpeg probe example with a public NexusKit media probe example.

**Architecture:** The example lives under `examples/media_probe`, links `nexus::media`, and calls `ffmpeg_backend_info()` plus `probe_media()`. A Python unittest checks the example does not directly include FFmpeg headers or link FFmpeg targets.

**Tech Stack:** C++17, CMake, Python unittest.

---

### Task 1: Example Contract Test

**Files:**
- Create: `tests/scripts/test_examples_contracts.py`

- [ ] **Step 1: Write the failing test**

The test should assert:

- `examples/media_probe/main.cpp` exists.
- The media example source includes `<nexus/media/media.h>`.
- The media example source does not mention `libav`.
- The media example CMake links `nexus::media`.
- `examples/CMakeLists.txt` adds the example when `TARGET nexus::media`.

Run:

```powershell
python -m unittest tests.scripts.test_examples_contracts -v
```

Expected before implementation: failure because `examples/media_probe` does not exist.

### Task 2: Media Probe Example

**Files:**
- Delete: `examples/ffmpeg_probe/main.cpp`
- Delete: `examples/ffmpeg_probe/CMakeLists.txt`
- Create: `examples/media_probe/main.cpp`
- Create: `examples/media_probe/CMakeLists.txt`
- Modify: `examples/CMakeLists.txt`

- [ ] **Step 1: Implement the example**

The example must:

- print backend availability and component versions,
- print usage when invoked with more than one path,
- when given one path, call `nexus::media::probe_media`,
- print each stream index, type, codec, and available audio/video dimensions,
- return non-zero on probe failure.

- [ ] **Step 2: Re-run the example contract test**

```powershell
python -m unittest tests.scripts.test_examples_contracts -v
```

Expected: pass.

### Task 3: Docs Update

**Files:**
- Modify: `README.md`
- Modify: `docs/modules/media.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update references**

Replace `nexus_example_ffmpeg_probe` with `nexus_example_media_probe` and describe it as a NexusKit media metadata probe.

- [ ] **Step 2: Run docs checks**

```powershell
python scripts/check_release_docs.py --dir .
python scripts/check_public_comments.py --dir .
```

Expected: pass.

### Task 4: Final Verification

**Files:**
- Read: all modified files.

- [ ] **Step 1: Run script tests**

```powershell
python -m unittest tests.scripts.test_examples_contracts -v
python -m unittest tests.scripts.test_check_release_docs -v
```

- [ ] **Step 2: Run CMake example smoke when possible**

```powershell
cmake --preset windows-msvc-release -B build/phase13b-media-example -DNEXUS_ENABLE_MEDIA=ON -DNEXUS_BUILD_EXAMPLES=ON -DNEXUS_BUILD_TESTS=OFF
cmake --build build/phase13b-media-example --config Release --target nexus_example_media_probe
```

- [ ] **Step 3: Commit and push**

```powershell
git add examples README.md docs/modules/media.md CHANGELOG.md tests/scripts/test_examples_contracts.py docs/superpowers/specs/2026-05-25-phase-13b-media-examples-design.md docs/superpowers/plans/2026-05-25-phase-13b-media-examples.md
git commit -m "docs: add media probe example guidance"
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
git push
```
