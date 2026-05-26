# Phase 13D Screen USB HID Public Comments Design

## Problem

The staged public header documentation sweep has covered most lifecycle and
enum-value declarations. Two small gaps remain in nearby device-facing modules:
`ScreenPixelFormat::unknown` lacks a value-level comment, and `HidDevice`
lifecycle declarations are not guarded by the lifecycle documentation contract.

## Scope

- Track `ScreenPixelFormat` and `UsbTransport` in the enum value documentation
  contract.
- Track `HidDevice` lifecycle declarations in the lifecycle documentation
  contract.
- Add concise `///` comments to the missing declarations reported by the
  contract test.
- Keep API shape, ABI, and runtime behavior unchanged.

## Non-Goals

- Do not change HID, USB, or screen implementation files.
- Do not add HID hotplug support.
- Do not broaden this phase to field or method coverage.

## Contract

- Tracked screen/USB enum values have nearby preceding `///` comments.
- Tracked HID lifecycle declarations have nearby preceding `///` comments.
- The public header contract test reports header, line, and declaration details
  when comments are missing.

## Verification

- Extend the contract test and confirm it fails on existing gaps.
- Add comments and confirm the contract test passes.
- Run the unified documentation check and whitespace check before commit.
