# NexusKit Phase 5C: Private Dependency Install Cleanup

## Goal

Keep NexusKit install prefixes focused on NexusKit public artifacts even when private third-party dependencies are built from source.

## Scope

- Remove hidapi pkg-config files emitted by the hidapi subproject during NexusKit installation.
- Preserve NexusKit headers, libraries, and CMake package files.
- Keep hidapi private in installed NexusKit targets.

## Verification

- Confirm the previous install prefix contains `lib/pkgconfig/hidapi.pc`.
- Reconfigure, build, test, and install with `NEXUS_ENABLE_HID=ON` and `NEXUS_BUILD_HIDAPI=ON`.
- Verify no `hidapi*.pc` files remain in the install prefix.
- Verify NexusKit CMake exports still do not expose hidapi.
