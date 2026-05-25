# Phase 13B Audio And Camera Probe Examples Design

## Goal

Add low-risk audio and camera examples that show device enumeration without starting capture, playback, or camera streaming.

## Scope

This phase adds `nexus_example_audio_probe` and `nexus_example_camera_probe`. The audio example lists input/output devices and default input/output devices. The camera example lists camera devices and the default camera device. Both examples treat unavailable backends and missing devices as reportable probe results rather than hard runtime failures.

## Requirements

- Examples must include only NexusKit public headers.
- Examples must not include WASAPI, PulseAudio, libuvc, V4L2, or other backend-private headers.
- Example targets must link `nexus::audio` or `nexus::camera` only.
- `examples/CMakeLists.txt` must gate each example with `if(TARGET nexus::audio)` or `if(TARGET nexus::camera)`.
- Examples must not start audio capture, audio playback, or camera capture by default.
- README and module docs must mention the examples.
- The examples contract test must protect the public-header and CMake gating contracts.

## Non-Goals

- No audio playback, audio capture, camera frame capture, or camera control mutation.
- No new audio or camera API.
- No CI runtime requirement for hardware-backed paths; build-only coverage is enough for this phase.

## Verification

- Add failing contract tests before creating the examples.
- Run the contract tests after implementation.
- Run release-doc and public-comment checks.
- Attempt build smoke when dependency fetching allows it.
