#include <catch2/catch_test_macros.hpp>

#include <nexus/common/xml.h>
#include <nexus/core/status.h>

TEST_CASE("parse_xml reads root attributes and child text") {
    const auto parsed = nexus::common::parse_xml(R"(<device id="cam-1"><name>Camera</name></device>)");

    REQUIRE(parsed.ok());

    const auto root = parsed.value().root();
    REQUIRE(root.ok());
    CHECK(root.value().name() == "device");

    const auto id = root.value().attribute("id");
    REQUIRE(id.ok());
    CHECK(id.value() == "cam-1");

    const auto name = root.value().child("name");
    REQUIRE(name.ok());

    const auto text = name.value().text();
    REQUIRE(text.ok());
    CHECK(text.value() == "Camera");
}

TEST_CASE("parse_xml reports invalid input") {
    const auto parsed = nexus::common::parse_xml("<device><name></device>");

    REQUIRE_FALSE(parsed.ok());
    CHECK(parsed.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("XmlDocument builds and serializes elements") {
    auto document = nexus::common::XmlDocument::create("device");
    auto root = document.root();

    REQUIRE(root.ok());
    REQUIRE(root.value().set_attribute("id", "mic-1").ok());

    auto name = root.value().append_child("name");
    REQUIRE(name.ok());
    REQUIRE(name.value().set_text("Microphone").ok());

    CHECK(document.dump() == R"(<device id="mic-1"><name>Microphone</name></device>)");
}

TEST_CASE("XmlNode reports missing attributes and children") {
    const auto parsed = nexus::common::parse_xml("<device />");
    REQUIRE(parsed.ok());

    const auto root = parsed.value().root();
    REQUIRE(root.ok());

    const auto attribute = root.value().attribute("id");
    REQUIRE_FALSE(attribute.ok());
    CHECK(attribute.status().code() == nexus::StatusCode::kNotFound);

    const auto child = root.value().child("name");
    REQUIRE_FALSE(child.ok());
    CHECK(child.status().code() == nexus::StatusCode::kNotFound);
}
