# NexusKit Phase 6A: Media FFmpeg Probe

## Goal

Add the first `nexus_media` target and prove it can link against the FFmpeg backend through a small public version probe API.

## Scope

- Add `nexus::media` / `nexus_media` as an optional CMake module behind `NEXUS_ENABLE_MEDIA`.
- Require configured FFmpeg CMake targets when `nexus_media` is enabled.
- Add a public `nexus/media/media.h` header with a backend version summary API.
- Keep FFmpeg headers and ownership details out of public NexusKit headers.
- Add focused tests that exercise the public API.
- Update README, architecture, and module documentation.

## Verification

- Add failing media tests before implementation.
- Build and run targeted media tests.
- Run the full configured build/test set for the phase build directory.
- Install to a phase prefix and verify headers/libraries are exported.
