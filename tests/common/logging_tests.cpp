#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

#include <nexus/common/logging.h>
#include <nexus/log/logger.h>

namespace {

std::filesystem::path test_log_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / "nexus_common_tests" / name;
    std::filesystem::create_directories(path.parent_path());
    std::filesystem::remove(path);
    return path;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

} // namespace

TEST_CASE("diagnostic_log writes message through nexus::log") {
    const auto log_path = test_log_path("common_diagnostic.log");

    nexus::log::LoggerOptions options;
    options.level = nexus::log::Level::debug;
    auto logger = nexus::log::create_file_logger("common_diag", log_path, options);
    REQUIRE(logger.ok());

    nexus::log::clear_default_logger();
    nexus::log::set_default_logger(logger.value());

    nexus::common::diagnostic_log(nexus::log::Level::info, "test message");

    nexus::log::default_logger().flush();
    nexus::log::clear_default_logger();

    const auto contents = read_file(log_path);
    CHECK(contents.find("test message") != std::string::npos);
}
