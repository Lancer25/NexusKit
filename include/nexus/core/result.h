#pragma once

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include <nexus/core/status.h>

namespace nexus {

/// A value-or-error container used as the return type of fallible functions.
///
/// A `Result<T>` holds either a value of `T` (success) or a non-ok `Status`
/// (failure).  Constructing with an ok Status throws `std::invalid_argument`.
///
/// Access the value via `value()` (throws on error) and the status via
/// `status()`.  Prefer checking `ok()` before calling `value()`.
///
/// @tparam T The success value type.
template <typename T>
class Result {
public:
    /// Constructs a success Result from a value copy.
    Result(const T& value) : storage_(value) {}
    /// Constructs a success Result from a moved value.
    Result(T&& value) : storage_(std::move(value)) {}
    /// Constructs a failure Result from a non-ok Status.
    ///
    /// @throws std::invalid_argument if `status` is ok.
    Result(Status status) : storage_(std::move(status)) {
        if (std::get<Status>(storage_).ok()) {
            throw std::invalid_argument("Result cannot hold an OK status without a value");
        }
    }

    /// True when the Result holds a success value.
    bool ok() const {
        return std::holds_alternative<T>(storage_);
    }

    /// Returns a const reference to the success value.
    ///
    /// @throws std::logic_error if the Result holds an error.
    const T& value() const& {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::get<T>(storage_);
    }

    /// Returns a mutable reference to the success value.
    ///
    /// @throws std::logic_error if the Result holds an error.
    T& value() & {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::get<T>(storage_);
    }

    /// Moves the success value out of a temporary Result.
    ///
    /// @throws std::logic_error if the Result holds an error.
    T&& value() && {
        if (!ok()) {
            throw std::logic_error("Result does not contain a value");
        }
        return std::move(std::get<T>(storage_));
    }

    /// Returns the Status of this Result.
    ///
    /// Returns an ok Status when the Result holds a value, and the error
    /// Status when it holds a failure.
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
