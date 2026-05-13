#include <stdexcept>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/core/version.h>

TEST_CASE("Status defaults to ok") {
    const nexus::Status status;

    REQUIRE(status.ok());
    REQUIRE(status.code() == nexus::StatusCode::kOk);
    REQUIRE(status.message().empty());
}

TEST_CASE("Status stores failure code and message") {
    const auto status = nexus::Status::invalid_argument("bad input");

    REQUIRE_FALSE(status.ok());
    REQUIRE(status.code() == nexus::StatusCode::kInvalidArgument);
    REQUIRE(status.message() == "bad input");
}

TEST_CASE("Result stores a value") {
    const nexus::Result<int> result(42);

    REQUIRE(result.ok());
    REQUIRE(result.value() == 42);
    REQUIRE(result.status().ok());
}

TEST_CASE("Result stores an error") {
    const nexus::Result<int> result(nexus::Status::not_found("missing"));

    REQUIRE_FALSE(result.ok());
    REQUIRE(result.status().code() == nexus::StatusCode::kNotFound);
    REQUIRE(result.status().message() == "missing");
}

TEST_CASE("Result rejects ok status without value") {
    REQUIRE_THROWS_AS(nexus::Result<int>(nexus::Status::ok_status()), std::invalid_argument);
}

TEST_CASE("Version string matches initial version") {
    REQUIRE(nexus::core::version_string() == "0.1.0");
}
