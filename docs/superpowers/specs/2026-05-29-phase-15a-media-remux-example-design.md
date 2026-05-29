# Phase 15A Media Remux Example Design

## Problem

`nexus_media` includes a high-level `remux_file` API, but the examples only
show backend status and metadata probing.  Users do not yet have a minimal
copy-stream workflow they can run or adapt.

## Goals

- Add a `nexus_example_media_remux` executable under `examples/media_remux`.
- Use only public `nexus_media` APIs and avoid FFmpeg/private dependency names.
- Gate the example with `if(TARGET nexus::media)`.
- Document the example in README and `docs/modules/media.md`.

## Non-Goals

- Do not add a manual encoder/muxer example in this phase.
- Do not run the example in CI, because it needs a caller-provided input media
  file.
- Do not change `remux_file` behavior.

## Acceptance Criteria

- The examples contract test requires `examples/media_remux` source, CMake
  target, public `nexus_media` include, and top-level CMake registration.
- The example accepts `input` and `output` paths, calls
  `nexus::media::remux_file`, and reports failures through public `Status`
  messages.
- Script, docs, UTF-8, and CMake example-target verification pass.
