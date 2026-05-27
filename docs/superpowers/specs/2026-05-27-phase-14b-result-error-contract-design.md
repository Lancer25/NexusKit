# Phase 14B Result Error Contract Design

## Problem

NexusKit public APIs use `Result<T>` for recoverable failures. The API style
guide requires expected `StatusCode` values to be documented, but the public
header contract checker does not enforce that rule. Some `Result<T>` methods
still have comments that describe the success path without documenting failure
codes.

## Scope

- Extend `scripts/check_public_header_contracts.py` with a public
  `Result<T>` documentation contract.
- Require each public `Result<T>` function or method declaration to document
  at least one error status via `@retval`, `StatusCode::...`, or a backtick
  status token such as `kInvalidArgument`.
- Update only public header comments exposed by the new checker.

## Non-Goals

- Do not require this rule for plain `Status` returns in this phase.
- Do not change public API signatures or implementation behavior.
- Do not infer exact status codes from implementation files.
- Do not scan private class sections or private helper declarations.

## Contract

- A public `Result<T>` declaration must have a nearby Doxygen block.
- That Doxygen block must mention at least one expected failure status code.
- `python scripts/check_public_header_contracts.py --dir .` reports missing
  Result error semantics under `result-error-contracts`.

## Verification

- Add a focused failing checker test for a public `Result<T>` method without
  error status documentation.
- Implement the checker with public/private class-section awareness.
- Fix real public headers reported by the checker.
- Add a real-repository public header contract unit test.
- Run focused contract tests, docs test discovery, public header checks,
  unified docs checks, text checks, and whitespace checks before commit.
