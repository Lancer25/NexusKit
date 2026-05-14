# NexusKit Phase 5A: HID Enumeration

## Goal

Add the first device-side module, `nexus_hid`, backed by hidapi.

## Scope

- Add public HID export and enumeration headers.
- Add `nexus_hid` CMake target gated by `NEXUS_ENABLE_HID`.
- Build hidapi from source when `NEXUS_BUILD_HIDAPI=ON`.
- Provide stable device enumeration that does not require a specific HID device to be connected.
- Route enumeration diagnostics through `nexus_log`.

## Verification

- Add failing HID enumeration tests first.
- Build and run targeted HID tests.
- Run full configure, build, test, install, export, and diff checks before committing.
