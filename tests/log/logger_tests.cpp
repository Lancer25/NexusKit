#include <nexus/log/logger.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

std::filesystem::path test_log_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / "nexuskit-tests" / name;
    std::filesystem::create_directories(path.parent_path());
    std::filesystem::remove(path);
    return path;
}

} // namespace

TEST_CASE("File logger writes and flushes messages") {
    const auto path = test_log_path("file_logger_writes.log");

    auto logger = nexus::log::create_file_logger("file_logger_writes", path);
    REQUIRE(logger.ok());

    logger.value().info("hello from nexus log");
    logger.value().flush();

    const auto contents = read_file(path);
    REQUIRE(contents.find("hello from nexus log") != std::string::npos);
}

TEST_CASE("File logger honors level filtering") {
    const auto path = test_log_path("file_logger_filters.log");

    nexus::log::LoggerOptions options;
    options.level = nexus::log::Level::warn;

    auto logger = nexus::log::create_file_logger("file_logger_filters", path, options);
    REQUIRE(logger.ok());

    logger.value().info("filtered info message");
    logger.value().warn("visible warn message");
    logger.value().flush();

    const auto contents = read_file(path);
    REQUIRE(contents.find("filtered info message") == std::string::npos);
    REQUIRE(contents.find("visible warn message") != std::string::npos);
}

TEST_CASE("Default logger can be installed and used for diagnostics") {
    const auto path = test_log_path("default_logger_diagnostics.log");

    auto logger = nexus::log::create_file_logger("default_logger_diagnostics", path);
    REQUIRE(logger.ok());

    nexus::log::clear_default_logger();
    CHECK_FALSE(nexus::log::default_logger().valid());

    nexus::log::set_default_logger(logger.value());
    REQUIRE(nexus::log::default_logger().valid());
    CHECK(nexus::log::default_logger().name() == "default_logger_diagnostics");

    nexus::log::write(nexus::log::Level::info, "diagnostic event");
    nexus::log::default_logger().flush();

    const auto contents = read_file(path);
    REQUIRE(contents.find("diagnostic event") != std::string::npos);

    nexus::log::clear_default_logger();
}
