# NexusKit Phase 6D: User FFmpeg Probe Verification

## Goal

Allow `nexus_media` to use a user-provided FFmpeg install prefix and verify the real FFmpeg-backed media probe path.

## Scope

- Teach `cmake/deps/FFmpeg.cmake` to create `FFmpeg::*` imported targets from `NEXUS_FFMPEG_INSTALL_DIR` even when `NEXUS_BUILD_FFMPEG=OFF`.
- Include the FFmpeg dependency recipe when `NEXUS_ENABLE_MEDIA=ON`, so media can discover user-provided FFmpeg targets.
- Add a media test that writes a tiny PCM WAV fixture and probes it when the FFmpeg backend is available.
- Copy FFmpeg runtime DLLs beside media tests on Windows when the backend is enabled.
- Update build documentation for user-provided FFmpeg usage.

## Verification

- Add the conditional FFmpeg-backed probe test before implementation.
- Verify the default no-FFmpeg media tests still pass.
- Configure/build/run a media test configuration that points `NEXUS_FFMPEG_INSTALL_DIR` at an existing FFmpeg prefix.
- Run the full default phase build/test set and install/export checks before committing.
