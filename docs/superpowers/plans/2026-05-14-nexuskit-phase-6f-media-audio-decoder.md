# NexusKit Phase 6F: Media Audio Decoder

## Goal

Add the first decode API to `nexus_media`: a move-only decoder that opens a media file and returns decoded frame data without exposing FFmpeg types.

## Scope

- Add `nexus::media::MediaFrame` for decoded frame metadata and bytes.
- Add `nexus::media::MediaDecoder` as a move-only RAII handle.
- Validate empty and missing paths before backend code.
- Return a clear failed-precondition status when no FFmpeg backend is available.
- Implement FFmpeg-backed audio decoding for generated PCM WAV fixtures.
- Extend tests and media docs.

## Verification

- Add failing decoder tests before implementation.
- Verify default no-FFmpeg tests still pass.
- Verify user-provided FFmpeg tests can open the generated WAV fixture and read one decoded audio frame.
- Run the full configured build/test set and install/export checks.
