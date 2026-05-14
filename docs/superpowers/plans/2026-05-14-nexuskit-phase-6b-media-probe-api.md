# NexusKit Phase 6B: Media Probe API

## Goal

Add a stable `nexus_media` file probing API that can report container and stream metadata when FFmpeg is linked, while keeping default non-FFmpeg builds usable.

## Scope

- Add `nexus::media::probe_media` as the first media-file operation.
- Validate empty and missing paths before touching backend code.
- Return a clear failed-precondition status when no FFmpeg backend is available.
- Keep FFmpeg headers and resource ownership out of public headers.
- Define small public metadata structs for format and stream summaries.
- Extend media tests and docs.

## Verification

- Add failing tests before implementation.
- Build and run `nexus_media_tests`.
- Run the full configured phase build/test set.
- Install to a phase prefix and verify `nexus::media` export plus media headers.
