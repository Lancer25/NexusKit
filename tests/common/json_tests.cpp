#include <catch2/catch_test_macros.hpp>

#include <nexus/common/json.h>
#include <nexus/core/status.h>

TEST_CASE("parse_json reads typed object fields") {
    const auto parsed = nexus::common::parse_json(R"({"name":"camera","index":7,"enabled":true})");

    REQUIRE(parsed.ok());
    REQUIRE(parsed.value().is_object());

    const auto name = parsed.value().at("name");
    REQUIRE(name.ok());
    REQUIRE(name.value().as_string().ok());
    CHECK(name.value().as_string().value() == "camera");

    const auto index = parsed.value().at("index");
    REQUIRE(index.ok());
    REQUIRE(index.value().as_int64().ok());
    CHECK(index.value().as_int64().value() == 7);

    const auto enabled = parsed.value().at("enabled");
    REQUIRE(enabled.ok());
    REQUIRE(enabled.value().as_bool().ok());
    CHECK(enabled.value().as_bool().value());
}

TEST_CASE("parse_json reports invalid input") {
    const auto parsed = nexus::common::parse_json(R"({"name":)");

    REQUIRE_FALSE(parsed.ok());
    CHECK(parsed.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("JsonValue builds and serializes objects") {
    auto root = nexus::common::JsonValue::object();

    REQUIRE(root.set("name", nexus::common::JsonValue::string("microphone")).ok());
    REQUIRE(root.set("index", nexus::common::JsonValue::integer(3)).ok());
    REQUIRE(root.set("enabled", nexus::common::JsonValue::boolean(false)).ok());

    CHECK(root.dump() == R"({"enabled":false,"index":3,"name":"microphone"})");
}

TEST_CASE("JsonValue rejects object lookup on non objects") {
    const auto value = nexus::common::JsonValue::string("camera");
    const auto result = value.at("name");

    REQUIRE_FALSE(result.ok());
    CHECK(result.status().code() == nexus::StatusCode::kFailedPrecondition);
}
