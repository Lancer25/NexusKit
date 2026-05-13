#include <nexus/common/json.h>

#include <exception>
#include <utility>

#include <nlohmann/json.hpp>

namespace nexus::common {

namespace detail {

class JsonStorage {
public:
    JsonStorage() = default;
    explicit JsonStorage(nlohmann::json value) : value(std::move(value)) {}

    nlohmann::json value;
};

} // namespace detail

namespace {

Status failed_precondition(std::string message) {
    return Status(StatusCode::kFailedPrecondition, std::move(message));
}

Status type_mismatch(std::string expected) {
    return failed_precondition("JSON value is not " + expected);
}

std::shared_ptr<detail::JsonStorage> make_storage(nlohmann::json value) {
    return std::make_shared<detail::JsonStorage>(std::move(value));
}

} // namespace

JsonValue::JsonValue() : storage_(make_storage(nullptr)) {}

JsonValue::JsonValue(const JsonValue& other)
    : storage_(make_storage(other.storage_->value)) {}

JsonValue::JsonValue(JsonValue&& other) noexcept
    : storage_(std::move(other.storage_)) {
    if (!other.storage_) {
        other.storage_ = make_storage(nullptr);
    }
}

JsonValue& JsonValue::operator=(const JsonValue& other) {
    if (this != &other) {
        storage_ = make_storage(other.storage_->value);
    }
    return *this;
}

JsonValue& JsonValue::operator=(JsonValue&& other) noexcept {
    if (this != &other) {
        storage_ = std::move(other.storage_);
        if (!other.storage_) {
            other.storage_ = make_storage(nullptr);
        }
    }
    return *this;
}

JsonValue::~JsonValue() = default;

JsonValue JsonValue::object() {
    return JsonValue(make_storage(nlohmann::json::object()));
}

JsonValue JsonValue::array() {
    return JsonValue(make_storage(nlohmann::json::array()));
}

JsonValue JsonValue::string(std::string value) {
    return JsonValue(make_storage(std::move(value)));
}

JsonValue JsonValue::integer(std::int64_t value) {
    return JsonValue(make_storage(value));
}

JsonValue JsonValue::boolean(bool value) {
    return JsonValue(make_storage(value));
}

bool JsonValue::is_null() const {
    return storage_->value.is_null();
}

bool JsonValue::is_object() const {
    return storage_->value.is_object();
}

bool JsonValue::is_array() const {
    return storage_->value.is_array();
}

bool JsonValue::is_string() const {
    return storage_->value.is_string();
}

bool JsonValue::is_integer() const {
    return storage_->value.is_number_integer();
}

bool JsonValue::is_boolean() const {
    return storage_->value.is_boolean();
}

Status JsonValue::set(std::string key, JsonValue value) {
    if (!storage_->value.is_object()) {
        return failed_precondition("JSON value is not an object");
    }

    storage_->value[std::move(key)] = value.storage_->value;
    return Status::ok_status();
}

Result<JsonValue> JsonValue::at(std::string_view key) const {
    if (!storage_->value.is_object()) {
        return failed_precondition("JSON value is not an object");
    }

    const auto iterator = storage_->value.find(std::string(key));
    if (iterator == storage_->value.end()) {
        return Status::not_found("JSON object does not contain key: " + std::string(key));
    }

    return JsonValue(make_storage(*iterator));
}

Result<std::string> JsonValue::as_string() const {
    if (!storage_->value.is_string()) {
        return type_mismatch("a string");
    }

    return storage_->value.get<std::string>();
}

Result<std::int64_t> JsonValue::as_int64() const {
    if (!storage_->value.is_number_integer()) {
        return type_mismatch("an integer");
    }

    return storage_->value.get<std::int64_t>();
}

Result<bool> JsonValue::as_bool() const {
    if (!storage_->value.is_boolean()) {
        return type_mismatch("a boolean");
    }

    return storage_->value.get<bool>();
}

std::string JsonValue::dump() const {
    return storage_->value.dump();
}

JsonValue::JsonValue(std::shared_ptr<detail::JsonStorage> storage)
    : storage_(std::move(storage)) {}

Result<JsonValue> parse_json(std::string_view text) {
    try {
        return JsonValue(make_storage(nlohmann::json::parse(text.begin(), text.end())));
    } catch (const nlohmann::json::parse_error& error) {
        return Status::invalid_argument(error.what());
    } catch (const std::exception& error) {
        return Status::internal(error.what());
    }
}

} // namespace nexus::common
