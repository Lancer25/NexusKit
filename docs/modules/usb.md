# nexus_usb

`nexus_usb` contains USB-facing discovery and basic device I/O utilities. The initial implementation is a facade over `nexus_hid`, giving applications a stable USB-oriented model while keeping backend details private.

## Current API

- `nexus::usb::UsbTransport`: transport used to discover or access a device. The first transport is `hid`.
- `nexus::usb::UsbEnumerationFilter`: optional vendor/product filter. A zero value means no filter for that field.
- `nexus::usb::UsbDeviceInfo`: UTF-8 device metadata and transport information.
- `nexus::usb::enumerate_devices`: returns a `nexus::Result<std::vector<UsbDeviceInfo>>`.
- `nexus::usb::UsbDevice`: move-only RAII device handle backed by the selected transport.
- `UsbDevice::open`: opens a device described by `UsbDeviceInfo`.
- `UsbDevice::write`: writes an output report.
- `UsbDevice::read`: reads an input report, optionally with a timeout.
- `UsbDevice::send_feature_report`: writes a feature report.
- `UsbDevice::get_feature_report`: reads a feature report.

## Scope

The initial module supports HID-backed USB device discovery, path-based open, input/output reports, and feature reports. UVC camera discovery, USB audio discovery, hotplug events, and device-class-specific abstractions are planned follow-up work.

## Backend

The current backend is `nexus_hid`, which itself uses hidapi privately. Public `nexus_usb` headers do not expose hidapi or HID implementation types.

## Diagnostics

Enumeration, open, report I/O, and close events are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.

## Error Model

- Backend enumeration failures propagate their original `nexus::Status`.
- An empty device list is a successful result.
- Empty paths and zero-length reports return `StatusCode::kInvalidArgument`.
- Operations on closed device handles return `StatusCode::kFailedPrecondition`.
- Device open and report I/O failures propagate their backend `nexus::Status`.
