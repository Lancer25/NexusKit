# NexusKit Phase 5D: USB Discovery Facade

## Goal

Add the first `nexus_usb` module as a stable USB-oriented discovery facade.

## Scope

- Add public USB export and discovery headers.
- Add `nexus_usb` CMake target gated by `NEXUS_ENABLE_USB`.
- Require `NEXUS_ENABLE_HID=ON` for the initial HID-backed discovery implementation.
- Map `nexus_hid` device metadata into `nexus_usb` device metadata.
- Keep hidapi and HID backend types out of public USB headers.
- Route USB discovery diagnostics through `nexus_log`.

## Verification

- Add failing USB discovery tests first.
- Build and run targeted USB tests.
- Run full configure, build, test, install, export, and diff checks before committing.
