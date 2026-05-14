# nexus_hid

`nexus_hid` contains cross-platform HID utilities. It keeps hidapi out of public headers and routes internal diagnostics through `nexus_log`.

## Current API

- `nexus::hid::HidEnumerationFilter`: optional vendor/product filter. A zero value means no filter for that field.
- `nexus::hid::HidDeviceInfo`: UTF-8 device metadata returned by hidapi.
- `nexus::hid::enumerate_devices`: returns a `nexus::Result<std::vector<HidDeviceInfo>>`.

## Scope

The initial module supports device enumeration only. Opening devices, feature reports, input/output reports, hotplug tracking, and higher-level USB abstractions are planned follow-up work.

## Backend

The current backend is hidapi. NexusKit links it privately so users of `nexus_hid` depend on NexusKit headers instead of hidapi headers.

## Diagnostics

Enumeration attempts and result counts are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.

## Error Model

- HID initialization failures return `StatusCode::kInternal`.
- An empty device list is a successful result.
