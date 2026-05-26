# Phase 13D Media Screen USB Lifecycle Comments Design

## Problem

Media, screen, and USB expose move-only RAII classes for backend resources.
Their class overview comments describe ownership at a high level, but some
public constructors, deleted copy operations, move operations, and destructors
still lack declaration-level comments.

## Scope

- Extend the lifecycle documentation contract to media, screen, and USB public
  headers.
- Add concise `///` comments to public lifecycle declarations in those modules.
- Keep API shape, ABI, and runtime behavior unchanged.
- Keep platform/backend headers private.

## Non-Goals

- Do not expand this phase to audio or camera.
- Do not alter resource ownership semantics.
- Do not refactor implementation files.

## Contract

- Public lifecycle declarations in tracked media, screen, and USB RAII classes
  have nearby preceding `///` comments.
- Deleted copy operations are documented because they are part of the public
  API contract.
- The existing lifecycle contract reports missing comments with header, line,
  class, and declaration details.

## Verification

- Extend the contract test and confirm it fails on current missing comments.
- Add comments and confirm the contract test passes.
- Run the unified documentation check and whitespace check before commit.
