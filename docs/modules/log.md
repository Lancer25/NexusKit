# nexus_log

`nexus_log` provides NexusKit's logging API. The public interface is intentionally small and does not expose spdlog types.

## Current API

- `nexus::log::Level`: logging severity.
- `nexus::log::LoggerOptions`: file logger configuration.
- `nexus::log::Logger`: copyable logger handle with level-specific methods.
- `nexus::log::create_file_logger`: creates a file-backed logger and returns `nexus::Result<Logger>`.
- `nexus::log::set_default_logger`: installs a process-wide default logger for NexusKit diagnostics.
- `nexus::log::default_logger`: returns the current default logger handle, or an invalid logger when unset.
- `nexus::log::clear_default_logger`: clears the process-wide diagnostic logger.
- `nexus::log::write`: writes a diagnostic event to the default logger when configured.

## Diagnostics

NexusKit modules use the default logger for internal diagnostics. If no default logger is installed, diagnostic writes are silent no-ops. Applications keep control of log destination and level by creating a logger and installing it during startup.

## Backend

The initial backend is spdlog. NexusKit uses it as an implementation detail so future modules can depend on `nexus::log` without taking a public dependency on spdlog headers.
