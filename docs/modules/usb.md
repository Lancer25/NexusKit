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

The initial module supports HID-backed USB device discovery, path-based open, input/output reports, feature reports, and USB-level hotplug events. UVC camera discovery, USB audio discovery, and device-class-specific abstractions are planned follow-up work. HID-level hotplug is not planned; use `UsbHotplugMonitor` for device arrival/removal notifications.

## Report Id and Feature Report Conventions

`nexus_usb` delegates to `nexus_hid` and follows the same HID report id and feature report conventions. See the [nexus_hid](hid.md) module documentation for report id rules and feature report behavior. In summary:

- All report buffers include a 1-byte report id prefix (0 for devices without report ids).
- `send_feature_report` requires the report id as the first byte; `get_feature_report` takes an explicit `report_id` parameter.

## Backend

The current backend is `nexus_hid`, which itself uses hidapi privately. Public `nexus_usb` headers do not expose hidapi or HID implementation types.

## Diagnostics

Enumeration, open, report I/O, and close events are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.

## Error Model

- Backend enumeration failures propagate their original `nexus::Status`.
- An empty device list is a successful result.
- Empty paths, zero-length reports, and zero `max_bytes` for read or feature report calls return `StatusCode::kInvalidArgument`.
- Operations on closed device handles return `StatusCode::kFailedPrecondition`.
- Device open and report I/O failures propagate their backend `nexus::Status`.
- Feature report transfers that complete with fewer bytes than requested do not error; the returned buffer is resized to the actual byte count.

## Hotplug Monitor

`UsbHotplugMonitor` provides real-time USB device arrival and removal notifications. It runs a platform-specific background monitor and delivers events via a callback invoked from a background thread.

### Usage

```cpp
auto monitor = nexus::usb::UsbHotplugMonitor::start(
    [](const nexus::usb::UsbHotplugEvent& event) {
        if (event.arrived) {
            // USB device plugged in
            // event.device contains vendor_id and product_id
        } else {
            // USB device removed
        }
    });

if (monitor.ok()) {
    // Monitor is running; callbacks fire on device hotplug events
    auto& m = monitor.value();
    // ...
    m.stop();  // Stop when done
}
```

The `nexus_example_usb_hotplug` executable demonstrates both enumeration and
bounded watch mode:

```powershell
nexus_example_usb_hotplug --list
nexus_example_usb_hotplug --watch --seconds 30
```

### Callback Contract

- The callback is invoked from a background thread. Implementations must be thread-safe.
- An empty callback is rejected with `StatusCode::kInvalidArgument`.
- Callback exceptions are caught and logged by the monitor so user-code failures do not escape the backend event loop.
- Hotplug events carry limited device information (VID/PID only). For full metadata, call `enumerate_devices()` after receiving an event.
- The callback should return quickly to avoid blocking the monitor thread.

### Platform Backends

- **Windows**: Hidden message-only window (`HWND_MESSAGE`) + `RegisterDeviceNotificationW` with `GUID_DEVINTERFACE_USB_DEVICE`. `WM_DEVICECHANGE` is processed in a background message loop.
- **Linux**: `libudev` monitor via `udev_monitor_new_from_netlink` with a `usb` subsystem filter. A background thread polls with `poll()` on the udev file descriptor.

### Error Model

- `start()` returns `kInternal` when the platform backend cannot be initialized (e.g., missing `libudev` on Linux).
- `stop()` requests the background monitor to exit and waits for the thread to finish. It is idempotent and safe to call on a stopped monitor.
