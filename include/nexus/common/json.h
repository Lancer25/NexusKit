#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <nexus/common/export.h>
#include <nexus/core/result.h>
#include <nexus/core/status.h>

/// JSON parsing and value manipulation.
///
/// The current backend is nlohmann_json, included only from the implementation
/// file.  Users of `nexus_common` do not need to link against nlohmann_json.
namespace nexus::common {

namespace detail {
class JsonStorage;
}

/// A copyable JSON value handle backed by a shared storage.
///
/// Supports the JSON types: null, object, array, string, integer, and boolean.
/// Copy and move are shallow and cheap (shared pointer semantics).
///
/// Object field access: use `set(key, value)` to write and `at(key)` to read.
/// Typed accessors `as_string()`, `as_int64()`, and `as_bool()` extract values;
/// they return `kFailedPrecondition` when the value is not the expected type.
///
/// Serialize with `dump()` which returns compact JSON text.
class NEXUS_COMMON_API JsonValue {
public:
    /// Constructs a null JSON value.
    JsonValue();
    JsonValue(const JsonValue& other);
    JsonValue(JsonValue&& other) noexcept;
    JsonValue& operator=(const JsonValue& other);
    JsonValue& operator=(JsonValue&& other) noexcept;
    ~JsonValue();

    /// Creates an empty JSON object.
    static JsonValue object();
    /// Creates an empty JSON array.
    static JsonValue array();
    /// Creates a JSON string value.
    static JsonValue string(std::string value);
    /// Creates a JSON integer value.
    static JsonValue integer(std::int64_t value);
    /// Creates a JSON boolean value.
    static JsonValue boolean(bool value);

    /// True when this value is JSON null.
    bool is_null() const;
    /// True when this value is a JSON object.
    bool is_object() const;
    /// True when this value is a JSON array.
    bool is_array() const;
    /// True when this value is a JSON string.
    bool is_string() const;
    /// True when this value is a JSON integer.
    bool is_integer() const;
    /// True when this value is a JSON boolean.
    bool is_boolean() const;

    /// Sets an object field.
    ///
    /// @return `kFailedPrecondition` when the value is not an object.
    Status set(std::string key, JsonValue value);

    /// Reads an object field by key.
    ///
    /// @return The field value, or `kFailedPrecondition` if the value is not
    /// an object, or `kNotFound` if the key is absent.
    Result<JsonValue> at(std::string_view key) const;

    /// Extracts the value as a string.
    ///
    /// @return The string, or `kFailedPrecondition` if the value is not a
    /// JSON string.
    Result<std::string> as_string() const;

    /// Extracts the value as a signed 64-bit integer.
    ///
    /// @return The integer, or `kFailedPrecondition` if the value is not a
    /// JSON integer.
    Result<std::int64_t> as_int64() const;

    /// Extracts the value as a boolean.
    ///
    /// @return The boolean, or `kFailedPrecondition` if the value is not a
    /// JSON boolean.
    Result<bool> as_bool() const;

    /// Serializes this value as compact JSON text.
    std::string dump() const;

private:
    friend NEXUS_COMMON_API Result<JsonValue> parse_json(std::string_view text);

    explicit JsonValue(std::shared_ptr<detail::JsonStorage> storage);

    std::shared_ptr<detail::JsonStorage> storage_;
};

/// Parses JSON text into a `JsonValue`.
///
/// @return The parsed value on success, or `StatusCode::kInvalidArgument` for
/// malformed input.
NEXUS_COMMON_API Result<JsonValue> parse_json(std::string_view text);

} // namespace nexus::common
