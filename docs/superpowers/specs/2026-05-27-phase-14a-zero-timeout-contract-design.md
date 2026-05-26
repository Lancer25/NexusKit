# Phase 14A Zero Timeout Contract Design

## Problem

Several public net option structs use `std::chrono::milliseconds` timeout fields
with a default value of `0`. For these fields, `0` means "no explicit timeout",
not "fail immediately". That distinction is part of the public API contract and
should stay documented wherever the zero-default timeout field is declared.

## Scope

- Extend `scripts/check_public_header_contracts.py` with a focused public header
  check for net timeout fields defaulted to `{0}`.
- Require each matching field's Doxygen block to mention `0 means no explicit
  timeout`.
- Keep existing callback, enum, lifecycle, and punctuation checks unchanged.
- Do not change public API signatures or implementation behavior.

## Non-Goals

- Do not require non-zero timeout defaults to describe zero behavior.
- Do not audit non-net modules in this phase.
- Do not enforce exact prose beyond the required phrase.
- Do not change C++ code.

## Contract

- A tracked net field matching `std::chrono::milliseconds <name>{0};` must have
  nearby Doxygen comments.
- That comment block must contain `0 means no explicit timeout`.
- `python scripts/check_public_header_contracts.py --dir .` reports missing
  zero-timeout semantics under a separate category.

## Verification

- Add a focused failing test for a zero-default timeout field without the
  required comment.
- Implement the checker.
- Run docs test discovery, public header contract checks, unified docs checks,
  text checks, and whitespace checks before commit.
