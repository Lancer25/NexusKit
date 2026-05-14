# NexusKit Phase 5B: HID Device I/O

## Goal

Extend `nexus_hid` from enumeration to a usable RAII device handle with report I/O.

## Scope

- Add `nexus::hid::HidDevice` as a move-only handle.
- Support `open_path`, `is_open`, `close`, output reports, input reports, and feature reports.
- Validate empty paths and zero-length reports before touching the backend.
- Report closed-handle operations with `StatusCode::kFailedPrecondition`.
- Route lifecycle and I/O diagnostics through `nexus_log`.

## Verification

- Add failing tests for the desired public API before implementation.
- Build and run targeted HID tests.
- Run full configure, build, test, install, export, and diff checks before committing.
