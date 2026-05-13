#pragma once

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include <nexus/core/status.h>

namespace nexus {

template <typename T>
class Result {
public:
    Result(const T& value) : storage_(value) {}
    Result(T&& value) : storage_(std::move(value)) {}
    Result(Status status) : storage_(std::move(status)) {
        if (std::get<Status>(storage_).ok()) {
            throw std::invalid_argument("Result cannot hold an OK status without a value");
        }
    }

    bool ok() const {
        return std::holds_alternative<T>(storage_);
    }

    const T& value() const& {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::get<T>(storage_);
    }

    T& value() & {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::get<T>(storage_);
    }

    T&& value() && {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::move(std::get<T>(storage_));
    }

    const Status& status() const {
        if (ok()) {
            static const Status ok_status = Status::ok_status();
            return ok_status;
        }
        return std::get<Status>(storage_);
    }

private:
    std::variant<T, Status> storage_;
};

} // namespace nexus
