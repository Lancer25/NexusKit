#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <nexus/common/export.h>
#include <nexus/core/result.h>
#include <nexus/core/status.h>

namespace nexus::common {

namespace detail {
class JsonStorage;
}

class NEXUS_COMMON_API JsonValue {
public:
    JsonValue();
    JsonValue(const JsonValue& other);
    JsonValue(JsonValue&& other) noexcept;
    JsonValue& operator=(const JsonValue& other);
    JsonValue& operator=(JsonValue&& other) noexcept;
    ~JsonValue();

    static JsonValue object();
    static JsonValue array();
    static JsonValue string(std::string value);
    static JsonValue integer(std::int64_t value);
    static JsonValue boolean(bool value);

    bool is_null() const;
    bool is_object() const;
    bool is_array() const;
    bool is_string() const;
    bool is_integer() const;
    bool is_boolean() const;

    Status set(std::string key, JsonValue value);
    Result<JsonValue> at(std::string_view key) const;
    Result<std::string> as_string() const;
    Result<std::int64_t> as_int64() const;
    Result<bool> as_bool() const;
    std::string dump() const;

private:
    friend NEXUS_COMMON_API Result<JsonValue> parse_json(std::string_view text);

    explicit JsonValue(std::shared_ptr<detail::JsonStorage> storage);

    std::shared_ptr<detail::JsonStorage> storage_;
};

NEXUS_COMMON_API Result<JsonValue> parse_json(std::string_view text);

} // namespace nexus::common
