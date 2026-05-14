#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <nexus/core/result.h>
#include <nexus/log/export.h>

namespace nexus::log {

enum class Level {
    trace,
    debug,
    info,
    warn,
    error,
    critical,
    off
};

struct LoggerOptions {
    Level level{Level::info};
    bool truncate{true};
};

namespace detail {
class LoggerBackend;
} // namespace detail

class NEXUS_LOG_API Logger {
public:
    Logger();
    explicit Logger(std::shared_ptr<detail::LoggerBackend> backend);

    bool valid() const noexcept;
    std::string name() const;

    void log(Level level, std::string_view message) const;
    void trace(std::string_view message) const;
    void debug(std::string_view message) const;
    void info(std::string_view message) const;
    void warn(std::string_view message) const;
    void error(std::string_view message) const;
    void critical(std::string_view message) const;
    void flush() const;

private:
    std::shared_ptr<detail::LoggerBackend> backend_;
};

NEXUS_LOG_API Result<Logger> create_file_logger(
    std::string name,
    const std::filesystem::path& path,
    LoggerOptions options = {});

NEXUS_LOG_API void set_default_logger(Logger logger);
NEXUS_LOG_API Logger default_logger();
NEXUS_LOG_API void clear_default_logger();
NEXUS_LOG_API void write(Level level, std::string_view message);

} // namespace nexus::log
