# Phase 13E Public Header Contract Checker Design

## Problem

Public header contracts for callback typedef comments, tracked enum value
comments, lifecycle comments, and public-header dash punctuation currently live
only in unittest code. CI runs those tests, but the local unified documentation
gate (`python scripts/check_docs.py --dir .`) does not execute the same
contracts.

## Scope

- Add `scripts/check_public_header_contracts.py` as a reusable checker module
  and command-line entry point.
- Reuse that checker from the existing unittest file instead of duplicating
  contract logic.
- Add the checker to `scripts/check_docs.py` so the unified local command covers
  the same public header contracts as CI.
- Keep public headers and runtime behavior unchanged.

## Non-Goals

- Do not broaden the contract rules in this phase.
- Do not change C++ implementation files.
- Do not run CMake builds from documentation checks.

## Contract

- `python scripts/check_public_header_contracts.py --dir .` reports all public
  header contract failures and exits non-zero.
- `python scripts/check_docs.py --dir .` includes a `public-header-contracts`
  result group.
- Existing unittest coverage remains, but delegates to the checker module.

## Verification

- Add tests that fail before the checker is implemented.
- Implement the checker and confirm tests pass.
- Run the unified documentation check and whitespace check before commit.
