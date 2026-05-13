#include <nexus/log/logger.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace nexus::log {
namespace {

spdlog::level::level_enum to_spdlog_level(Level level) {
    switch (level) {
    case Level::trace:
        return spdlog::level::trace;
    case Level::debug:
        return spdlog::level::debug;
    case Level::info:
        return spdlog::level::info;
    case Level::warn:
        return spdlog::level::warn;
    case Level::error:
        return spdlog::level::err;
    case Level::critical:
        return spdlog::level::critical;
    case Level::off:
        return spdlog::level::off;
    }

    return spdlog::level::info;
}

} // namespace

namespace detail {

class LoggerBackend {
public:
    explicit LoggerBackend(std::shared_ptr<spdlog::logger> logger) : logger_(std::move(logger)) {}

    spdlog::logger& logger() {
        return *logger_;
    }

    const spdlog::logger& logger() const {
        return *logger_;
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace detail

Logger::Logger() = default;

Logger::Logger(std::shared_ptr<detail::LoggerBackend> backend) : backend_(std::move(backend)) {}

bool Logger::valid() const noexcept {
    return static_cast<bool>(backend_);
}

std::string Logger::name() const {
    if (!backend_) {
        return {};
    }
    return backend_->logger().name();
}

void Logger::log(Level level, std::string_view message) const {
    if (!backend_) {
        return;
    }
    backend_->logger().log(to_spdlog_level(level), "{}", message);
}

void Logger::trace(std::string_view message) const {
    log(Level::trace, message);
}

void Logger::debug(std::string_view message) const {
    log(Level::debug, message);
}

void Logger::info(std::string_view message) const {
    log(Level::info, message);
}

void Logger::warn(std::string_view message) const {
    log(Level::warn, message);
}

void Logger::error(std::string_view message) const {
    log(Level::error, message);
}

void Logger::critical(std::string_view message) const {
    log(Level::critical, message);
}

void Logger::flush() const {
    if (!backend_) {
        return;
    }
    backend_->logger().flush();
}

Result<Logger> create_file_logger(std::string name, const std::filesystem::path& path, LoggerOptions options) {
    if (name.empty()) {
        return Status::invalid_argument("logger name cannot be empty");
    }

    if (path.empty()) {
        return Status::invalid_argument("logger path cannot be empty");
    }

    try {
        const auto parent = path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        auto logger = spdlog::basic_logger_mt(std::move(name), path.string(), options.truncate);
        logger->set_level(to_spdlog_level(options.level));
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

        return Logger(std::make_shared<detail::LoggerBackend>(std::move(logger)));
    } catch (const spdlog::spdlog_ex& error) {
        return Status::internal(error.what());
    } catch (const std::filesystem::filesystem_error& error) {
        return Status::internal(error.what());
    }
}

} // namespace nexus::log
