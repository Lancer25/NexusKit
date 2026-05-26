#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <nexus/core/result.h>
#include <nexus/log/export.h>

/// Diagnostic logging built on spdlog.
///
/// Backend headers are private; users do not need to link against spdlog.
namespace nexus::log {

/// Log severity levels.
enum class Level {
    /// Most verbose diagnostic messages.
    trace,
    /// Debug diagnostics useful during development.
    debug,
    /// Informational events for normal operation.
    info,
    /// Recoverable problem or suspicious condition.
    warn,
    /// Error that prevents the requested operation.
    error,
    /// Severe failure that may require immediate attention.
    critical,
    /// Logging disabled.
    off
};

/// Options for `create_file_logger`.
struct LoggerOptions {
    /// Minimum level to log.  Messages below this level are dropped.
    Level level{Level::info};
    /// When true, truncate an existing log file.  When false, append.
    bool truncate{true};
};

namespace detail {
class LoggerBackend;
} // namespace detail

/// A named logger backed by a spdlog sink.
///
/// Logger instances are copyable (shared backend).  A default-constructed
/// Logger has no backend; `valid()` returns false and log calls are no-ops.
///
/// All log methods are thread-safe (delegate to spdlog's internal locking).
class NEXUS_LOG_API Logger {
public:
    /// Constructs an invalid logger (no backend).
    Logger();
    explicit Logger(std::shared_ptr<detail::LoggerBackend> backend);

    /// True when this logger has a valid backend.
    bool valid() const noexcept;
    /// Logger name set at creation time.
    std::string name() const;

    /// Logs a message at the given level.
    void log(Level level, std::string_view message) const;
    /// Convenience for `log(Level::trace, message)`.
    void trace(std::string_view message) const;
    /// Convenience for `log(Level::debug, message)`.
    void debug(std::string_view message) const;
    /// Convenience for `log(Level::info, message)`.
    void info(std::string_view message) const;
    /// Convenience for `log(Level::warn, message)`.
    void warn(std::string_view message) const;
    /// Convenience for `log(Level::error, message)`.
    void error(std::string_view message) const;
    /// Convenience for `log(Level::critical, message)`.
    void critical(std::string_view message) const;

    /// Flushes buffered log output to the underlying sink.
    void flush() const;

private:
    std::shared_ptr<detail::LoggerBackend> backend_;
};

/// Creates a file-based logger.
///
/// @param name Logger name used in log formatting.
/// @param path Output file path.
/// @param options Severity level and truncation mode.
/// @return A valid Logger on success.
/// @retval kInvalidArgument when `name` or `path` is empty.
/// @retval kInternal when the file cannot be opened.
NEXUS_LOG_API Result<Logger> create_file_logger(
    std::string name,
    const std::filesystem::path& path,
    LoggerOptions options = {});

/// Sets the global default logger used by `write()`.
NEXUS_LOG_API void set_default_logger(Logger logger);

/// Returns the current global default logger.
///
/// Returns an invalid logger if none has been set.
NEXUS_LOG_API Logger default_logger();

/// Clears the global default logger.
NEXUS_LOG_API void clear_default_logger();

/// Writes a message through the global default logger.
///
/// If no default logger is set, this call is a no-op.
NEXUS_LOG_API void write(Level level, std::string_view message);

} // namespace nexus::log
