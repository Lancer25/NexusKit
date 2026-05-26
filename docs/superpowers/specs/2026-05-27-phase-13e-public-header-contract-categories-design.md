# Phase 13E Public Header Contract Categories Design

## Problem

`check_public_header_contracts.py` now runs the focused public header contracts,
but failure messages are a flat list. When the unified documentation gate fails,
callers should be able to see which contract failed without inferring it from
the message text.

## Scope

- Prefix public header contract messages with stable contract category names.
- Keep existing path and line details in each message.
- Keep checker rules, public headers, ABI, and runtime behavior unchanged.
- Keep scripts ASCII-friendly where practical.

## Non-Goals

- Do not add new public header rules.
- Do not change C++ implementation files.
- Do not change the outer `check_docs.py` result group name.

## Contract

- `check_repo()` returns messages prefixed with one of:
  - `callback-typedefs:`
  - `enum-values:`
  - `lifecycle:`
  - `dash-punctuation:`
- The command-line checker prints the same categorized messages.

## Verification

- Update checker tests to expect categorized messages and confirm they fail.
- Implement categorization and confirm tests pass.
- Run the unified documentation check and whitespace check before commit.
