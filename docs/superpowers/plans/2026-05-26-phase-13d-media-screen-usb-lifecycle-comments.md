# Phase 13D Media Screen USB Lifecycle Comments Plan

## Goal

Make lifecycle semantics explicit for media, screen, and USB public RAII
classes, continuing the staged public API documentation cleanup.

## Steps

1. Add media, screen, and USB public headers to the lifecycle documentation
   contract.
2. Run the contract test and confirm it fails on missing lifecycle comments.
3. Add focused comments to the reported public lifecycle declarations.
4. Run public header contract tests, the unified documentation check, and
   whitespace checks.
5. Commit and push to `master`.
