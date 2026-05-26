# Phase 14A Net Callback Threading Contract Design

## Problem

The net module exposes many async callback typedefs. Existing checks ensure
those typedefs have Doxygen comments, but they do not require the comments to
state callback threading or synchronous-completion behavior. That information is
part of the public contract for async APIs.

## Scope

- Extend `scripts/check_public_header_contracts.py` with a focused check for
  net callback typedef comments.
- Require each tracked net `*Handler` typedef comment block to mention callback
  invocation threading or synchronous completion behavior.
- Keep existing callback typedef, enum value, lifecycle, and dash punctuation
  checks unchanged.
- Do not change public API signatures or implementation behavior.

## Non-Goals

- Do not audit non-net modules in this phase.
- Do not enforce exact wording.
- Do not require every method comment to repeat the threading contract when the
  callback typedef already carries it.
- Do not change C++ code.

## Contract

- A net `using *Handler = ...` declaration must have nearby Doxygen comments.
- That comment block must include at least one threading/synchronous invocation
  cue such as `thread`, `synchronous`, or `synchronously`.
- `python scripts/check_public_header_contracts.py --dir .` reports missing
  callback threading contracts under a separate category.

## Verification

- Add a focused failing test with a callback typedef that has Doxygen but no
  threading contract.
- Implement the checker.
- Run docs test discovery, public header contract checks, unified docs checks,
  text checks, and whitespace checks before commit.
