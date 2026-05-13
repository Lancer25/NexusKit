# nexus_core

`nexus_core` contains the foundational API shared by other NexusKit modules.

## Headers

- `<nexus/core/status.h>`: `nexus::Status` and `nexus::StatusCode`.
- `<nexus/core/result.h>`: `nexus::Result<T>`.
- `<nexus/core/version.h>`: version constants and `nexus::core::version_string()`.
- `<nexus/core/export.h>`: platform export macros for compiled `nexus_core` symbols.

## Error Model

Use `nexus::Status` when a function only reports success or failure.
Use `nexus::Result<T>` when a function returns either a value or a failure.
