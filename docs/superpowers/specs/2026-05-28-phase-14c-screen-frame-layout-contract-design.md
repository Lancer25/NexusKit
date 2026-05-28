# Phase 14C Screen Frame Layout Contract Design

## Problem

`nexus_screen` returns `ScreenFrame` pixel buffers that callers use directly.
The public header documents that BGRA/RGBA frames are contiguous and sized as
`width * height * 4`, but this data-layout contract is important enough to
protect against accidental wording loss.

## Scope

- Extend `scripts/check_public_header_contracts.py` with a focused screen frame
  data-layout contract.
- Require `ScreenFrame` comments to mention contiguous pixel data and
  `data.size() == width * height * 4`.
- Add a real-repository public header contract test.

## Non-Goals

- Do not change screen capture behavior.
- Do not add stride support or new pixel formats.
- Do not broaden this rule to media frames in this phase.

## Contract

- `ScreenFrame` must document contiguous pixel storage for public screen frames.
- `ScreenFrame` must document the BGRA/RGBA byte count formula.
- `python scripts/check_public_header_contracts.py --dir .` reports missing
  screen frame layout semantics under `screen-frame-layout`.

## Verification

- Add a focused failing checker test for a `ScreenFrame` comment missing data
  layout wording.
- Implement the checker and add it to `check_repo`.
- Confirm the real repository passes the new check.
- Run focused contract tests, docs test discovery, public header checks,
  unified docs checks, text checks, and whitespace checks before commit.
