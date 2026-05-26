# Phase 13D Core Log Common Lifecycle Comments Design

## Problem

Core, log, and common headers already document most public functions and fields,
but public lifecycle methods are inconsistent. Constructors, destructors, move
operations, copy operations, and deleted operations define important API
semantics, yet the current public comments checker intentionally skips many of
them.

## Scope

- Add a focused contract test for lifecycle declarations in core, log, and
  common public headers.
- Add concise `///` comments to tracked public lifecycle declarations.
- Keep runtime behavior, ABI, and exported symbols unchanged.
- Keep public headers platform-neutral and free of private backend headers.

## Non-Goals

- Do not expand this phase to media, screen, net, USB, HID, audio, or camera.
- Do not change class ownership models.
- Do not refactor implementation files.

## Contract

- Public constructors, destructors, and assignment operators in tracked
  core/log/common classes have nearby preceding `///` comments.
- Deleted copy operations are documented when they are public API surface.
- The contract test reports the header, line, class, and lifecycle declaration
  when documentation is missing.

## Verification

- Add the contract test and confirm it fails on existing missing comments.
- Add comments and confirm the contract test passes.
- Run the unified documentation check and whitespace check before commit.
