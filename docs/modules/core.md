# nexus_core

`nexus_core` contains the foundational API shared by other NexusKit modules.

## Headers

- `<nexus/core/status.h>`: `nexus::Status` and `nexus::StatusCode`.
- `<nexus/core/result.h>`: `nexus::Result<T>`.
- `<nexus/core/version.h>`: version constants and `nexus::core::version_string()`.
- `<nexus/core/export.h>`: `NEXUS_CORE_API` platform export macro for compiled `nexus_core` symbols.

All public types and functions in these headers are annotated with Doxygen `///` comments following `docs/api-style.md`.

## Error Model

Use `nexus::Status` when a function only reports success or failure.
Use `nexus::Result<T>` when a function returns either a value or a failure.
