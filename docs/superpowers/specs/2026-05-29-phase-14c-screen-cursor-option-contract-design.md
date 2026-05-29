# Phase 14C Screen Cursor Option Contract Design

## Problem

`ScreenCaptureOptions::include_cursor` is intentionally a best-effort request.
On Windows the backend attempts cursor composition, and cursor query/render
failure must not turn an otherwise valid capture into a failure.  This public
contract is documented, but it is not protected by an automated check.

## Goals

- Keep the `include_cursor` public comment tied to Windows best-effort cursor
  composition.
- Require the comment to state that cursor composition failure does not fail
  capture.
- Avoid API, ABI, and backend behavior changes.

## Non-Goals

- Do not change cursor rendering behavior.
- Do not add Linux cursor capture support.
- Do not add backend-private types to public headers.

## Acceptance Criteria

- A public header contract test fails when `include_cursor` omits Windows,
  best-effort, or non-failing capture semantics.
- The real repository passes the new contract check.
- Existing script-level docs, UTF-8, and public header checks continue to pass.
