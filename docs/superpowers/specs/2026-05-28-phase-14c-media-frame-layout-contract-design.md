# Phase 14C Media Frame Layout Contract Design

## Problem

`MediaFrame` is shared by decoder output, converter output, and encoder input.
Its `data` layout depends on audio/video type and format metadata.  The public
header already describes this, but the contract is not protected by an
automated check.

## Goals

- Keep the public `MediaFrame` layout contract documented in the platform-
  neutral public header.
- Require audio layout notes for packed and planar audio data.
- Require video layout notes that backend-native decoded bytes should be
  normalized through `convert_video_frame` when callers need RGB-family data.
- Avoid any runtime behavior or ABI changes.

## Non-Goals

- Do not change `MediaFrame` fields.
- Do not add FFmpeg types or backend-private headers to public headers.
- Do not enforce exact byte sizing for every media format.

## Acceptance Criteria

- `scripts/check_public_header_contracts.py` reports a focused error when
  `MediaFrame` lacks the required audio/video layout wording.
- The real repository passes the new public header contract check.
- Existing public header, docs, and text hygiene checks continue to pass.
