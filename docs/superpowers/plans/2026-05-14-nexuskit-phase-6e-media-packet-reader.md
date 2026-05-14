# NexusKit Phase 6E: Media Packet Reader

## Goal

Add the first demuxing API to `nexus_media`: a move-only reader that opens a media file and reads encoded packet data without exposing FFmpeg types.

## Scope

- Add `nexus::media::MediaReader` as a move-only RAII handle.
- Add `nexus::media::MediaPacket` for stream index, timestamps, duration, key-frame flag, and packet bytes.
- Validate empty and missing paths before backend code.
- Return a clear failed-precondition status when no FFmpeg backend is available.
- Implement FFmpeg-backed packet reading with private ownership.
- Extend tests and media docs.

## Verification

- Add failing reader tests before implementation.
- Verify default no-FFmpeg tests still pass.
- Verify user-provided FFmpeg tests can open the generated WAV fixture and read one packet.
- Run the full configured build/test set and install/export checks.
