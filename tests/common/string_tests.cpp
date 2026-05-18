#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

#include <nexus/common/string.h>

TEST_CASE("starts_with matches prefix") {
    CHECK(nexus::common::starts_with("hello world", "hello"));
    CHECK(nexus::common::starts_with("hello", "hello"));
    CHECK_FALSE(nexus::common::starts_with("hello", "world"));
    CHECK_FALSE(nexus::common::starts_with("hi", "hello"));
}

TEST_CASE("starts_with handles empty") {
    CHECK(nexus::common::starts_with("hello", ""));
    CHECK(nexus::common::starts_with("", ""));
    CHECK_FALSE(nexus::common::starts_with("", "a"));
}

TEST_CASE("ends_with matches suffix") {
    CHECK(nexus::common::ends_with("hello world", "world"));
    CHECK(nexus::common::ends_with("hello", "hello"));
    CHECK_FALSE(nexus::common::ends_with("hello", "world"));
    CHECK_FALSE(nexus::common::ends_with("hello", "longer-suffix"));
}

TEST_CASE("ends_with handles empty") {
    CHECK(nexus::common::ends_with("hello", ""));
    CHECK(nexus::common::ends_with("", ""));
    CHECK_FALSE(nexus::common::ends_with("", "a"));
}

TEST_CASE("contains finds substring") {
    CHECK(nexus::common::contains("hello world", "lo wo"));
    CHECK(nexus::common::contains("hello", "ell"));
    CHECK_FALSE(nexus::common::contains("hello", "world"));
}

TEST_CASE("contains handles empty") {
    CHECK(nexus::common::contains("hello", ""));
    CHECK_FALSE(nexus::common::contains("", "a"));
}

TEST_CASE("contains_space finds whitespace") {
    CHECK(nexus::common::contains_space("hello world"));
    CHECK(nexus::common::contains_space(" leading"));
    CHECK(nexus::common::contains_space("trailing "));
    CHECK(nexus::common::contains_space("tab\tchar"));
    CHECK(nexus::common::contains_space("newline\nchar"));
    CHECK_FALSE(nexus::common::contains_space("nospace"));
    CHECK_FALSE(nexus::common::contains_space(""));
}

TEST_CASE("equals_ignore_case compares case-insensitively") {
    CHECK(nexus::common::equals_ignore_case("hello", "HELLO"));
    CHECK(nexus::common::equals_ignore_case("Hello World", "hello world"));
    CHECK(nexus::common::equals_ignore_case("", ""));
    CHECK_FALSE(nexus::common::equals_ignore_case("hello", "world"));
    CHECK_FALSE(nexus::common::equals_ignore_case("hello", "hello!"));
}

TEST_CASE("trim removes leading and trailing whitespace") {
    CHECK(nexus::common::trim("  hello  ") == "hello");
    CHECK(nexus::common::trim("\t\nvalue\r\n") == "value");
    CHECK(nexus::common::trim("no-space") == "no-space");
    CHECK(nexus::common::trim("   ") == "");
    CHECK(nexus::common::trim("") == "");
}

TEST_CASE("trim_left removes leading whitespace only") {
    CHECK(nexus::common::trim_left("  hello  ") == "hello  ");
    CHECK(nexus::common::trim_left("no-space") == "no-space");
    CHECK(nexus::common::trim_left("") == "");
}

TEST_CASE("trim_right removes trailing whitespace only") {
    CHECK(nexus::common::trim_right("  hello  ") == "  hello");
    CHECK(nexus::common::trim_right("no-space") == "no-space");
    CHECK(nexus::common::trim_right("") == "");
}

TEST_CASE("split divides string by delimiter") {
    const auto parts = nexus::common::split("a,b,c", ',');
    REQUIRE(parts.size() == 3);
    CHECK(parts[0] == "a");
    CHECK(parts[1] == "b");
    CHECK(parts[2] == "c");
}

TEST_CASE("split handles single element") {
    const auto parts = nexus::common::split("hello", ',');
    REQUIRE(parts.size() == 1);
    CHECK(parts[0] == "hello");
}

TEST_CASE("split handles empty string") {
    const auto parts = nexus::common::split("", ',');
    REQUIRE(parts.size() == 1);
    CHECK(parts[0] == "");
}

TEST_CASE("split handles consecutive delimiters") {
    const auto parts = nexus::common::split("a,,b", ',');
    REQUIRE(parts.size() == 3);
    CHECK(parts[0] == "a");
    CHECK(parts[1] == "");
    CHECK(parts[2] == "b");
}

TEST_CASE("char_to_string handles null and non-null") {
    CHECK(nexus::common::char_to_string(nullptr) == "");
    CHECK(nexus::common::char_to_string("hello") == "hello");
    CHECK(nexus::common::char_to_string("") == "");
}

TEST_CASE("wide_to_utf8 converts ASCII wide characters") {
    const std::wstring wide = L"hello";
    const auto utf8 = nexus::common::wide_to_utf8(wide);
    CHECK(utf8 == "hello");
}

TEST_CASE("wide_to_utf8 handles empty string") {
    CHECK(nexus::common::wide_to_utf8(L"").empty());
    CHECK(nexus::common::wide_to_utf8(std::wstring_view()).empty());
}

TEST_CASE("wide_to_utf8 converts CJK characters") {
    const std::wstring wide = L"\u4E2D\u6587";
    const auto utf8 = nexus::common::wide_to_utf8(wide);
    CHECK(utf8 == "\xE4\xB8\xAD\xE6\x96\x87");
}

TEST_CASE("utf8_to_wide converts ASCII to wide") {
    const auto wide = nexus::common::utf8_to_wide("hello");
    CHECK(wide == L"hello");
}

TEST_CASE("utf8_to_wide handles empty string") {
    CHECK(nexus::common::utf8_to_wide("").empty());
    CHECK(nexus::common::utf8_to_wide(std::string_view()).empty());
}

TEST_CASE("utf8_to_wide round-trips with wide_to_utf8") {
    const std::wstring original = L"Hello \u4E2D\u6587 World";
    const auto utf8 = nexus::common::wide_to_utf8(original);
    const auto wide = nexus::common::utf8_to_wide(utf8);
    CHECK(wide == original);
}

TEST_CASE("percent_encode encodes special characters") {
    CHECK(nexus::common::percent_encode("hello world") == "hello%20world");
    CHECK(nexus::common::percent_encode("a+b=c") == "a%2Bb%3Dc");
}

TEST_CASE("percent_encode preserves unreserved characters") {
    CHECK(nexus::common::percent_encode("abc-_.~123") == "abc-_.~123");
}

TEST_CASE("percent_encode handles empty string") {
    CHECK(nexus::common::percent_encode("") == "");
}

TEST_CASE("percent_decode decodes percent-encoded strings") {
    CHECK(nexus::common::percent_decode("hello%20world") == "hello world");
    CHECK(nexus::common::percent_decode("a%2Bb%3Dc") == "a+b=c");
}

TEST_CASE("percent_decode handles mixed encoded and literal") {
    CHECK(nexus::common::percent_decode("%48%65%6C%6C%6F") == "Hello");
}

TEST_CASE("percent_decode handles malformed sequences") {
    CHECK(nexus::common::percent_decode("%XX") == "%XX");
    CHECK(nexus::common::percent_decode("%2") == "%2");
    CHECK(nexus::common::percent_decode("%") == "%");
}

TEST_CASE("percent_decode handles empty string") {
    CHECK(nexus::common::percent_decode("") == "");
}

TEST_CASE("percent_encode and percent_decode round-trip") {
    const std::string original = "Hello World! 123 & abc=def";
    const auto encoded = nexus::common::percent_encode(original);
    const auto decoded = nexus::common::percent_decode(encoded);
    CHECK(decoded == original);
}
