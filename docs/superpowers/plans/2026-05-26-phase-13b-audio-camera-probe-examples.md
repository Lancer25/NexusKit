# Phase 13B Audio And Camera Probe Examples Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add audio and camera device probe examples without requiring capture/playback hardware workflows.

**Architecture:** `examples/audio_probe` uses `AudioCapturer` and `AudioPlayer` static enumeration helpers. `examples/camera_probe` uses `CameraCapturer` static enumeration helpers. Python contract tests ensure examples stay on public NexusKit headers and gated CMake targets.

**Tech Stack:** C++17, CMake, Python unittest.

---

### Task 1: Example Contract Tests

**Files:**
- Modify: `tests/scripts/test_examples_contracts.py`

- [ ] **Step 1: Write failing tests**

Add tests that assert:

- `examples/audio_probe/main.cpp` and `CMakeLists.txt` exist.
- `examples/camera_probe/main.cpp` and `CMakeLists.txt` exist.
- Audio source includes `<nexus/audio/capturer.h>` and `<nexus/audio/player.h>`.
- Camera source includes `<nexus/camera/capturer.h>`.
- Sources do not mention backend-private names such as WASAPI, PulseAudio, libuvc, V4L2, `mmdeviceapi`, `pulse`, `uvc`, or `linux/videodev2`.
- CMake files link only `nexus::audio` or `nexus::camera`.
- `examples/CMakeLists.txt` gates with `if(TARGET nexus::audio)` and `if(TARGET nexus::camera)`.

Run:

```powershell
python -m unittest tests.scripts.test_examples_contracts -v
```

Expected before implementation: failure because the example directories do not exist.

### Task 2: Audio Probe Example

**Files:**
- Create: `examples/audio_probe/main.cpp`
- Create: `examples/audio_probe/CMakeLists.txt`
- Modify: `examples/CMakeLists.txt`

- [ ] **Step 1: Implement the example**

The example must:

- call `AudioCapturer::enumerate_input_devices()`,
- call `AudioPlayer::enumerate_output_devices()`,
- call `default_input_device()` and `default_output_device()`,
- print device names, ids, channels, and sample rates,
- return 0 for unavailable backend, missing default device, or empty device lists.

### Task 3: Camera Probe Example

**Files:**
- Create: `examples/camera_probe/main.cpp`
- Create: `examples/camera_probe/CMakeLists.txt`
- Modify: `examples/CMakeLists.txt`

- [ ] **Step 1: Implement the example**

The example must:

- call `CameraCapturer::enumerate_devices()`,
- call `CameraCapturer::default_device()`,
- print device names, ids, paths, VID, and PID,
- return 0 for unavailable backend, missing default device, or empty device lists.

### Task 4: Documentation Update

**Files:**
- Modify: `README.md`
- Modify: `docs/modules/audio.md`
- Modify: `docs/modules/camera.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Add example references**

Mention `nexus_example_audio_probe` and `nexus_example_camera_probe` in README and module docs.

- [ ] **Step 2: Run docs checks**

```powershell
python scripts/check_release_docs.py --dir .
python scripts/check_public_comments.py --dir .
```

Expected: pass.

### Task 5: Final Verification

**Files:**
- Read: all modified files.

- [ ] **Step 1: Run script tests**

```powershell
python -m unittest tests.scripts.test_check_release_docs tests.scripts.test_examples_contracts -v
```

- [ ] **Step 2: Run CMake example smoke when possible**

```powershell
cmake --preset windows-msvc-release -B build/phase13b-audio-camera-examples -DNEXUS_ENABLE_AUDIO=ON -DNEXUS_ENABLE_CAMERA=ON -DNEXUS_BUILD_EXAMPLES=ON -DNEXUS_BUILD_TESTS=OFF
cmake --build build/phase13b-audio-camera-examples --config Release --target nexus_example_audio_probe nexus_example_camera_probe
```

- [ ] **Step 3: Commit and push**

```powershell
git add examples README.md docs/modules/audio.md docs/modules/camera.md CHANGELOG.md tests/scripts/test_examples_contracts.py docs/superpowers/specs/2026-05-26-phase-13b-audio-camera-probe-examples-design.md docs/superpowers/plans/2026-05-26-phase-13b-audio-camera-probe-examples.md
git commit -m "docs: add audio and camera probe examples"
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
git push
```
