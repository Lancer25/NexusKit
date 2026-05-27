# Phase 14C Screen Visible Window Contract Design

## Problem

`nexus_screen` window capture returns currently visible desktop pixels. It does
not provide occlusion-free or minimized-window capture. The public header and
module docs already explain this, but the contract is subtle enough that future
edits could accidentally weaken the wording.

## Scope

- Extend `scripts/check_public_header_contracts.py` with a focused
  screen-window documentation contract.
- Require `ScreenWindow` comments to state that enumerated windows are visible,
  minimized windows are excluded, and cross-display spanning windows are
  excluded.
- Require `ScreenCapturer::capture_window` comments to state that capture uses
  visible desktop pixels and includes occluding windows.
- Add a real-repository public header contract test.

## Non-Goals

- Do not change screen capture behavior.
- Do not implement occlusion-free or minimized-window capture.
- Do not broaden this rule to every screen method in this phase.

## Contract

- `ScreenWindow` must document the visible-window enumeration filter.
- `capture_window` must document that the returned frame is the current visible
  desktop result, including occlusion.
- `python scripts/check_public_header_contracts.py --dir .` reports missing
  screen window semantics under `screen-visible-window`.

## Verification

- Add a focused failing checker test for missing `ScreenWindow` and
  `capture_window` visible-window wording.
- Implement the checker and add it to `check_repo`.
- Confirm the real repository passes the new check.
- Run focused contract tests, docs test discovery, public header checks,
  unified docs checks, text checks, and whitespace checks before commit.
