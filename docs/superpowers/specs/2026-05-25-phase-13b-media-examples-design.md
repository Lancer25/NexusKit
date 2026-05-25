# Phase 13B Media Examples Design

## Goal

Turn the existing FFmpeg probe example into a NexusKit media API example that demonstrates the public `nexus_media` surface without exposing private FFmpeg headers to users.

## Scope

This phase replaces the direct-FFmpeg example with `nexus_example_media_probe`. The example prints FFmpeg backend availability and, when given a media path, prints container and stream metadata through `nexus::media::probe_media`.

## Requirements

- Example source must include only NexusKit public media headers, not FFmpeg headers.
- Example target must link `nexus::media`, not FFmpeg component targets directly.
- Example must be gated on `TARGET nexus::media`.
- README and module docs must refer to `nexus_example_media_probe`.
- A lightweight script test must protect the example contract.

## Non-Goals

- No new media API.
- No remux, encode, mux, decode, or conversion example in this phase.
- No heavyweight FFmpeg source build changes.

## Verification

- Add a failing script test before changing the example.
- Run the script test after the example cleanup.
- Run release docs and public comment checks.
- Run a media-enabled configure/build for the example when dependency fetching allows it.
