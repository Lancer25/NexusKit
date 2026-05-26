# Phase 13D Net Lifecycle Comments Design

## Problem

The net module exposes public connection and server handles whose lifecycle
semantics matter to callers. Class-level comments describe ownership, but copy,
move, and destructor declarations are not consistently documented at the
declaration level or guarded by the focused lifecycle contract.

## Scope

- Extend the lifecycle documentation contract to net public handle classes.
- Add concise `///` comments to reported public lifecycle declarations.
- Replace touched non-ASCII punctuation in English comments with ASCII wording.
- Keep API shape, ABI, and runtime behavior unchanged.

## Non-Goals

- Do not change net implementation files.
- Do not alter callback, threading, timeout, or close semantics.
- Do not broaden this phase to non-lifecycle method coverage.

## Contract

- Tracked net public lifecycle declarations have nearby preceding `///`
  comments.
- Deleted copy operations and copyable handle operations are documented because
  they are part of the public API contract.
- The public header contract test reports header, line, class, and declaration
  details when comments are missing.

## Verification

- Extend the contract test and confirm it fails on existing net lifecycle gaps.
- Add comments and confirm the contract test passes.
- Run the unified documentation check and whitespace check before commit.
