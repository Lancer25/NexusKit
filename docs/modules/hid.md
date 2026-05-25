# nexus_hid

`nexus_hid` contains cross-platform HID utilities. It keeps hidapi out of public headers and routes internal diagnostics through `nexus_log`.

## Current API

- `nexus::hid::HidEnumerationFilter`: optional vendor/product filter. A zero value means no filter for that field.
- `nexus::hid::HidDeviceInfo`: UTF-8 device metadata returned by hidapi.
- `nexus::hid::enumerate_devices`: returns a `nexus::Result<std::vector<HidDeviceInfo>>`.
- `nexus::hid::HidDevice`: move-only RAII device handle.
- `HidDevice::open_path`: opens a device path returned by enumeration.
- `HidDevice::write`: writes an output report.
- `HidDevice::read`: reads an input report, optionally with a timeout.
- `HidDevice::send_feature_report`: writes a feature report.
- `HidDevice::get_feature_report`: reads a feature report.

## Scope

The module supports device enumeration, path-based open, input/output reports, and feature reports. HID-level hotplug is not planned; use `nexus::usb::UsbHotplugMonitor` for device arrival/removal notifications. Higher-level USB abstractions remain separate follow-up work.

## Report Id Conventions

HID reports are prefixed with a 1-byte report id as mandated by the HID specification. Every `write` and `read` buffer must include this byte.

- **Devices without report ids**: the first byte of every report is `0` (the single-report psuedo-id). Callers must still provide it in `write` and `read` return buffers will start with `0`.
- **Devices with report ids**: the first byte identifies which report descriptor the payload targets. The caller is responsible for setting the correct id in `write` and `send_feature_report`. `read` returns the report id as the first byte of the returned buffer.
- **Feature reports**: `send_feature_report` requires the report id as the first byte of the buffer, matching the `write` convention. `get_feature_report` takes an explicit `report_id` parameter - the backend prepends it to the request, and the returned buffer starts with that id byte.

## Feature Reports

Feature reports are bidirectional control transfers that are independent of the interrupt endpoint data stream used by `write` / `read`. They are commonly used for device configuration, calibration data, and vendor-specific commands.

- `send_feature_report(report)` sends a Set_Report control transfer via `hid_send_feature_report`. The buffer must include the report id as the first byte.
- `get_feature_report(report_id, max_bytes)` sends a Get_Report control transfer via `hid_get_feature_report`. The `report_id` parameter identifies the report to request. The returned buffer includes the report id as the first byte, followed by the report payload.
- Feature report transfers may block briefly while the device responds. They do not affect the input report queue - pending `read` calls are not disrupted.

## Backend

The current backend is hidapi. NexusKit links it privately so users of `nexus_hid` depend on NexusKit headers instead of hidapi headers.

## Diagnostics

Enumeration, open, read, write, feature report, and close events are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.

## Error Model

- HID initialization failures return `StatusCode::kInternal`.
- An empty device list is a successful result.
- Empty paths, zero-length reports, and zero `max_bytes` for read or feature report calls return `StatusCode::kInvalidArgument`.
- Operations on closed device handles return `StatusCode::kFailedPrecondition`.
- Device open, report I/O, and feature report failures return `StatusCode::kUnavailable`.
- Feature report transfers that complete but return fewer bytes than requested do not error — the returned buffer is resized to the actual byte count.
