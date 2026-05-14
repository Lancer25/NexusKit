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

The initial module supports device enumeration, path-based open, input/output reports, and feature reports. Hotplug tracking and higher-level USB abstractions are planned follow-up work.

## Backend

The current backend is hidapi. NexusKit links it privately so users of `nexus_hid` depend on NexusKit headers instead of hidapi headers.

## Diagnostics

Enumeration, open, read, write, feature report, and close events are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.

## Error Model

- HID initialization failures return `StatusCode::kInternal`.
- An empty device list is a successful result.
- Empty paths and zero-length reports return `StatusCode::kInvalidArgument`.
- Operations on closed device handles return `StatusCode::kFailedPrecondition`.
- Device open and report I/O failures return `StatusCode::kUnavailable`.
