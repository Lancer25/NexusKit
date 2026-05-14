# NexusKit Phase 6C: Media Diagnostics

## Goal

Route `nexus_media` backend and probe diagnostics through the shared `nexus_log` default logger.

## Scope

- Make `nexus_media` depend on `nexus_log` for internal diagnostics.
- Log media backend availability checks and probe lifecycle events.
- Log validation failures, missing files, unavailable backend, FFmpeg parser failures, and successful probe summaries.
- Keep diagnostics silent when no default logger is installed.
- Extend media tests and docs.

## Verification

- Add a failing media diagnostic test before implementation.
- Build and run `nexus_media_tests`.
- Run the full configured phase build/test set.
- Install to a phase prefix and verify exports remain clean.
