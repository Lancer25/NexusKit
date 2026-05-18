# Implementation Plan: nexus_audio — Audio Capture (Phase 12A)

## Overview

Add a new `nexus_audio` optional module providing cross-platform audio input device enumeration and PCM audio frame capture via WASAPI (Windows) and PulseAudio (Linux).

## Stages

### Stage 1: Module skeleton, types, and CMake

- Create `include/nexus/audio/export.h`, `types.h`, `capturer.h`
- Create `src/audio/CMakeLists.txt` with platform backend selection
- Add `NEXUS_ENABLE_AUDIO` option to root `CMakeLists.txt` (default OFF)
- Add `nexus_audio` library target and `nexus::audio` alias
- Link `nexus::core`, `nexus::common`
- Windows: link `ole32.lib`
- Linux: find PulseAudio via pkg-config, link `libpulse-simple`
- Stub out `AudioCapturer` with all methods returning `kUnavailable` when no backend

### Stage 2: Windows WASAPI backend

- Implement `detail::AudioCapturerStorage` in `src/audio/wasapi_capturer.cpp`
- Device enumeration via `IMMDeviceEnumerator::EnumAudioEndpoints(eCapture)`
- Default device via `IMMDeviceEnumerator::GetDefaultAudioEndpoint(eCapture, eConsole)`
- Capture via `IAudioClient::Initialize` (shared mode, PCM int16) → `IAudioCaptureClient::GetBuffer`
- Internal capture thread using `nexus::common::Thread`
- Device property extraction: friendly name via `IPropertyStore`, format via `IAudioClient::GetMixFormat`

### Stage 3: Linux PulseAudio backend

- Implement `detail::AudioCapturerStorage` in `src/audio/pulse_capturer.cpp`
- Device enumeration via `pa_context_get_source_info_list` with threaded mainloop
- Default device via `pa_context_get_server_info` → `default_source_name`
- Capture via `pa_simple_new` + blocking `pa_simple_read` on capture thread
- Device property extraction from `pa_source_info`

### Stage 4: Tests

- `tests/audio/audio_tests.cpp` with Catch2
- Test: backend availability reporting
- Test: enumerate input devices returns non-empty list (when backend available)
- Test: default input device returns valid device
- Test: create with default options
- Test: start/stop capture lifecycle
- Test: captured frames have valid PCM data

### Stage 5: Documentation

- Add `docs/modules/audio.md` module reference
- Update `docs/architecture.md` with nexus_audio dependency graph
- Update `docs/iteration.md` completed work
- Update `CHANGELOG.md`
- Update `README.md` module table
