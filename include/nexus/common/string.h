#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

/// String manipulation and encoding utilities.
namespace nexus::common {

/// True when `value` starts with `prefix`.
inline bool starts_with(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() &&
           value.substr(0, prefix.size()) == prefix;
}

/// True when `value` ends with `suffix`.
inline bool ends_with(std::string_view value, std::string_view suffix) {
    return value.size() >= suffix.size() &&
           value.substr(value.size() - suffix.size()) == suffix;
}

/// True when `value` contains `needle`.
inline bool contains(std::string_view value, std::string_view needle) {
    return value.find(needle) != std::string_view::npos;
}

/// True when `value` contains at least one whitespace character.
inline bool contains_space(std::string_view value) {
    for (unsigned char c : value) {
        if (std::isspace(c)) {
            return true;
        }
    }
    return false;
}

/// Converts a possibly-null `const char*` to a `std::string`.
///
/// A null pointer produces an empty string.
inline std::string char_to_string(const char* value) {
    return value == nullptr ? std::string() : std::string(value);
}

/// Returns a view with leading whitespace removed.
inline std::string_view trim_left(std::string_view value) {
    const auto start = std::find_if_not(value.begin(), value.end(),
        [](unsigned char c) { return std::isspace(c); });
    return value.substr(static_cast<std::size_t>(start - value.begin()));
}

/// Returns a view with trailing whitespace removed.
inline std::string_view trim_right(std::string_view value) {
    const auto end = std::find_if_not(value.rbegin(), value.rend(),
        [](unsigned char c) { return std::isspace(c); });
    const auto count = static_cast<std::size_t>(end - value.rbegin());
    return value.substr(0, value.size() - count);
}

/// Returns a view with leading and trailing whitespace removed.
inline std::string_view trim(std::string_view value) {
    return trim_right(trim_left(value));
}

/// Case-insensitive comparison of two string views.
bool equals_ignore_case(std::string_view a, std::string_view b);

/// Splits `value` at each occurrence of `delimiter`.
std::vector<std::string> split(std::string_view value, char delimiter);

/// Converts a wide string to a UTF-8 encoded narrow string.
///
/// On Windows uses `WideCharToMultiByte`; on POSIX uses `wcsrtombs`.
std::string wide_to_utf8(std::wstring_view value);

/// Converts a UTF-8 encoded narrow string to a wide string.
///
/// On Windows uses `MultiByteToWideChar`; on POSIX uses `mbsrtowcs`.
std::wstring utf8_to_wide(std::string_view value);

/// Encodes `value` with RFC 3986 percent-encoding.
std::string percent_encode(std::string_view value);

/// Decodes an RFC 3986 percent-encoded string.
std::string percent_decode(std::string_view value);

} // namespace nexus::common
