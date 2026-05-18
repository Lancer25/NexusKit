#include <catch2/catch_test_macros.hpp>

#include <nexus/common/status.h>
#include <nexus/core/status.h>

TEST_CASE("failed_precondition returns correct code and message") {
    const auto s = nexus::common::failed_precondition("test error");
    CHECK(s.code() == nexus::StatusCode::kFailedPrecondition);
    CHECK(s.message() == "test error");
}

TEST_CASE("unavailable returns kUnavailable with message") {
    const auto s = nexus::common::unavailable("backend not ready");
    CHECK(s.code() == nexus::StatusCode::kUnavailable);
    CHECK(s.message() == "backend not ready");
}

TEST_CASE("invalid_argument returns correct code") {
    const auto s = nexus::common::invalid_argument("bad input");
    CHECK(s.code() == nexus::StatusCode::kInvalidArgument);
    CHECK(s.message() == "bad input");
}

TEST_CASE("not_found returns correct code") {
    const auto s = nexus::common::not_found("resource missing");
    CHECK(s.code() == nexus::StatusCode::kNotFound);
    CHECK(s.message() == "resource missing");
}

TEST_CASE("internal_error returns correct code") {
    const auto s = nexus::common::internal_error("something broke");
    CHECK(s.code() == nexus::StatusCode::kInternal);
    CHECK(s.message() == "something broke");
}
