# nexus_usb

`nexus_usb` contains USB-facing discovery utilities. The initial implementation is a facade over `nexus_hid`, giving applications a stable USB-oriented model while keeping backend details private.

## Current API

- `nexus::usb::UsbTransport`: transport used to discover or access a device. The first transport is `hid`.
- `nexus::usb::UsbEnumerationFilter`: optional vendor/product filter. A zero value means no filter for that field.
- `nexus::usb::UsbDeviceInfo`: UTF-8 device metadata and transport information.
- `nexus::usb::enumerate_devices`: returns a `nexus::Result<std::vector<UsbDeviceInfo>>`.

## Scope

The initial module supports HID-backed USB device discovery. UVC camera discovery, USB audio discovery, hotplug events, and device-class-specific abstractions are planned follow-up work.

## Backend

The current backend is `nexus_hid`, which itself uses hidapi privately. Public `nexus_usb` headers do not expose hidapi or HID implementation types.

## Diagnostics

Enumeration attempts and result counts are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.

## Error Model

- Backend enumeration failures propagate their original `nexus::Status`.
- An empty device list is a successful result.
