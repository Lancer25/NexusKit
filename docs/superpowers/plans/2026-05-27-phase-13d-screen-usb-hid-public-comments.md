# Phase 13D Screen USB HID Public Comments Plan

## Goal

Close the remaining small public-comment gaps in screen, USB, and HID headers
without changing behavior.

## Steps

1. Extend `tests/scripts/test_public_header_contracts.py` to track
   `ScreenPixelFormat`, `UsbTransport`, and `HidDevice` lifecycle declarations.
2. Run the contract test and confirm it fails on the current missing comments.
3. Add focused `///` comments to the reported declarations.
4. Run public header contract tests, the unified documentation check, and
   whitespace checks.
5. Commit and push to `master`.
