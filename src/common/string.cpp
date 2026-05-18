#include <nexus/common/string.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <clocale>
#include <cwchar>
#endif

namespace nexus::common {

bool equals_ignore_case(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> split(std::string_view value, char delimiter) {
    std::vector<std::string> result;
    std::size_t start = 0;
    std::size_t pos = value.find(delimiter);
    while (pos != std::string_view::npos) {
        result.emplace_back(value.substr(start, pos - start));
        start = pos + 1;
        pos = value.find(delimiter, start);
    }
    result.emplace_back(value.substr(start));
    return result;
}

std::string wide_to_utf8(std::wstring_view value) {
    if (value.empty()) {
        return {};
    }
#if defined(_WIN32)
    const auto size = WideCharToMultiByte(CP_UTF8, 0, value.data(),
        static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(),
        static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
#else
    std::setlocale(LC_ALL, "en_US.UTF-8");
    const auto* src = value.data();
    std::mbstate_t state{};
    const auto len = std::wcsrtombs(nullptr, &src, 0, &state);
    if (len == static_cast<std::size_t>(-1)) {
        return {};
    }
    std::string result(len, '\0');
    src = value.data();
    state = {};
    std::wcsrtombs(result.data(), &src, len, &state);
    return result;
#endif
}

std::wstring utf8_to_wide(std::string_view value) {
    if (value.empty()) {
        return {};
    }
#if defined(_WIN32)
    const auto size = MultiByteToWideChar(CP_UTF8, 0, value.data(),
        static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(),
        static_cast<int>(value.size()), result.data(), size);
    return result;
#else
    std::setlocale(LC_ALL, "en_US.UTF-8");
    const auto* src = value.data();
    std::mbstate_t state{};
    const auto len = std::mbsrtowcs(nullptr, &src, 0, &state);
    if (len == static_cast<std::size_t>(-1)) {
        return {};
    }
    std::wstring result(len, L'\0');
    src = value.data();
    state = {};
    std::mbsrtowcs(result.data(), &src, len, &state);
    return result;
#endif
}

static bool is_unreserved_uri_character(unsigned char c) {
    return std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~';
}

std::string percent_encode(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (unsigned char c : value) {
        if (is_unreserved_uri_character(c)) {
            result.push_back(static_cast<char>(c));
        } else {
            constexpr char hex[] = "0123456789ABCDEF";
            result.push_back('%');
            result.push_back(hex[(c >> 4) & 0x0f]);
            result.push_back(hex[c & 0x0f]);
        }
    }
    return result;
}

static int hex_digit_value(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

std::string percent_decode(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = hex_digit_value(static_cast<unsigned char>(value[i + 1]));
            const int lo = hex_digit_value(static_cast<unsigned char>(value[i + 2]));
            if (hi >= 0 && lo >= 0) {
                result.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        result.push_back(value[i]);
    }
    return result;
}

} // namespace nexus::common
