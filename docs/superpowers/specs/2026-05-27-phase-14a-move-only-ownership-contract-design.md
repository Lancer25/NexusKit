# Phase 14A Move-Only Ownership Contract Design

## Problem

Several NexusKit public classes are move-only RAII handles. Their behavior after
copy/move operations is part of the public API contract. Existing checks verify
lifecycle methods have comments, but they do not require the class-level comment
to state that copy is disabled and move transfers ownership.

## Scope

- Extend `scripts/check_public_header_contracts.py` with a focused check for
  exported move-only classes.
- Require class-level Doxygen comments for move-only classes to mention both:
  - `Copy is deleted`
  - `move transfers ownership`
- Keep existing callback, timeout, enum, lifecycle, and punctuation checks
  unchanged.
- Update real public headers only where the new check reveals missing wording.

## Non-Goals

- Do not require the exact moved-from state wording in this phase.
- Do not inspect implementation files.
- Do not change public API signatures or runtime behavior.
- Do not apply this rule to value types or shared-handle types.

## Contract

- A public class comment that declares the type `Move-only` must also state that
  copy is deleted and move transfers ownership.
- `python scripts/check_public_header_contracts.py --dir .` reports missing
  move-only ownership semantics under a separate category.

## Verification

- Add a focused failing test for a move-only public class whose comment omits
  ownership semantics.
- Implement the checker.
- Fix any real public header comments it exposes.
- Run docs test discovery, public header contract checks, unified docs checks,
  text checks, and whitespace checks before commit.
