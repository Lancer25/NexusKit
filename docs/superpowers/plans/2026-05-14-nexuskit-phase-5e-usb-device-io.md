# NexusKit Phase 5E: USB Device I/O Facade

## Goal

Extend `nexus_usb` beyond discovery so applications can open a discovered USB device and perform basic HID-backed report I/O through USB-oriented APIs.

## Scope

- Add `nexus::usb::UsbDevice` as a move-only RAII handle.
- Support `open`, `is_open`, `close`, output reports, input reports, and feature reports.
- Keep `nexus_hid` and hidapi types out of USB public headers.
- Preserve input validation and closed-handle behavior from the HID backend.
- Route USB lifecycle and I/O diagnostics through `nexus_log`.

## Verification

- Add failing USB device tests before implementation.
- Build and run targeted USB tests.
- Run full configure, build, test, install, export, and diff checks before committing.
