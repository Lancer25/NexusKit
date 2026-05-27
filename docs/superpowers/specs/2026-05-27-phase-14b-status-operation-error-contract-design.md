# Phase 14B Status Operation Error Contract Design

## Problem

`Status` returning public operations also carry recoverable failure semantics.
The current public-header checker protects `Result<T>` declarations, but a small
set of non-idempotent `Status` operations can still omit expected error status
codes in their Doxygen comments.

## Scope

- Extend `scripts/check_public_header_contracts.py` with a focused public
  `Status` operation documentation contract.
- Require public `Status` declarations to document at least one failure status
  via `@retval`, `StatusCode::...`, or a backtick status token such as
  `kFailedPrecondition`.
- Exclude idempotent lifecycle methods and status factory helpers where the
  method name already communicates success/error construction semantics.
- Update only public header comments exposed by the new checker.

## Non-Goals

- Do not change public API signatures or implementation behavior.
- Do not require error-code comments for `close()` or `stop()` in this phase.
- Do not infer exact status codes from implementation files.
- Do not scan private class sections or private helper declarations.

## Contract

- A public non-excluded `Status` declaration must have a nearby Doxygen block.
- That block must mention at least one expected failure status code.
- `python scripts/check_public_header_contracts.py --dir .` reports missing
  Status operation error semantics under `status-error-contracts`.

## Verification

- Add a focused failing checker test for a public `Status` operation without
  error status documentation.
- Implement the checker with the same public/private class-section awareness as
  the `Result<T>` checker.
- Fix real public headers reported by the checker.
- Add a real-repository public header contract unit test.
- Run focused contract tests, docs test discovery, public header checks,
  unified docs checks, text checks, and whitespace checks before commit.
