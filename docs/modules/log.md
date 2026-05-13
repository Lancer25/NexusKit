# nexus_log

`nexus_log` provides NexusKit's logging API. The public interface is intentionally small and does not expose spdlog types.

## Current API

- `nexus::log::Level`: logging severity.
- `nexus::log::LoggerOptions`: file logger configuration.
- `nexus::log::Logger`: copyable logger handle with level-specific methods.
- `nexus::log::create_file_logger`: creates a file-backed logger and returns `nexus::Result<Logger>`.

## Backend

The initial backend is spdlog. NexusKit uses it as an implementation detail so future modules can depend on `nexus::log` without taking a public dependency on spdlog headers.
