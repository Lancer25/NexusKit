# Phase 14C Screen Region Contract Design

## Problem

`ScreenCaptureRegion` defines the coordinate system and bounds contract for
`ScreenCapturer::capture_region`.  The public header documents the behavior,
but there is no automated guard to keep future edits from weakening the
contract.

## Goals

- Keep `ScreenCaptureRegion` documented as display-relative.
- Require the empty `display_id` primary-display behavior to stay documented.
- Require positive crop dimensions to stay documented.
- Avoid API, ABI, or backend behavior changes.

## Non-Goals

- Do not change capture-region validation logic.
- Do not add platform-specific screen types to public headers.
- Do not add runtime tests for monitor geometry.

## Acceptance Criteria

- A public header contract test fails when `ScreenCaptureRegion` omits the
  coordinate, primary-display, or positive-dimension wording.
- The real repository passes the new check.
- Existing docs, UTF-8, and public header contract checks continue to pass.
