# Phase 13D Audio Camera Lifecycle Comments Design

## Problem

Audio and camera expose move-only RAII classes for capture and playback. Their
class-level documentation explains ownership, but move operations and
destructors are not documented at the declaration level. A few touched comment
lines also contain punctuation that displays poorly in some Windows consoles.

## Scope

- Extend the lifecycle documentation contract to audio and camera public
  capturer/player headers.
- Add concise `///` comments to public move operations and destructors.
- Replace touched non-ASCII punctuation in English comments with ASCII wording.
- Keep API shape, ABI, and runtime behavior unchanged.

## Non-Goals

- Do not change audio or camera implementation files.
- Do not add new capture or playback behavior.
- Do not broaden this phase to field or method coverage beyond lifecycle
  declarations.

## Contract

- Public lifecycle declarations in tracked audio/camera RAII classes have
  nearby preceding `///` comments.
- The lifecycle contract continues to report header, line, class, and
  declaration details for missing comments.

## Verification

- Extend the contract test and confirm it fails on current missing comments.
- Add comments and confirm the contract test passes.
- Run the unified documentation check and whitespace check before commit.
