# Phase 14C Opaque Identifier Contract Design

## Problem

Several public module structs expose opaque backend identifiers such as display
ids, window ids, device ids, and backend paths. These values are useful inside a
running process, but callers should not treat them as stable persisted
identifiers unless a specific API says so. The API style guide already calls
out opaque ids, but the public-header checker does not protect that lifecycle
wording.

## Scope

- Extend `scripts/check_public_header_contracts.py` with a focused opaque
  identifier field contract.
- Apply it to public `std::string` fields whose Doxygen block declares them
  `Opaque` and whose name contains `id` or `path`.
- Require the Doxygen block to say either `Do not persist` or `Use this to open`.
- Add a real-repository public header contract test.

## Non-Goals

- Do not require all `id` or `path` fields to be opaque in this phase.
- Do not change runtime behavior or public API signatures.
- Do not infer stability from implementation files.
- Do not apply this to option fields such as `device_id` unless their comment
  explicitly declares them opaque.

## Contract

- A public opaque identifier/path field must document whether callers should
  avoid persistence or use the value only to open the current backend resource.
- `python scripts/check_public_header_contracts.py --dir .` reports missing
  opaque identifier lifecycle wording under `opaque-identifiers`.

## Verification

- Add a focused failing checker test for an opaque `id` field missing lifecycle
  wording.
- Implement the checker and add it to `check_repo`.
- Confirm the real repository passes the new check.
- Run focused contract tests, docs test discovery, public header checks,
  unified docs checks, text checks, and whitespace checks before commit.
