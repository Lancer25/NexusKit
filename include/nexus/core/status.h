#pragma once

#include <string>
#include <utility>

namespace nexus {

/// Standard error codes for recoverable failures.
///
/// Functions return `Status` or `Result<T>` for errors; exceptions are reserved
/// for programming bugs (null dereference, invalid Result access, etc.).
enum class StatusCode {
    kOk = 0,
    kCancelled,
    kInvalidArgument,
    kNotFound,
    kAlreadyExists,
    kPermissionDenied,
    kResourceExhausted,
    kFailedPrecondition,
    kUnavailable,
    kInternal,
    kUnknown
};

/// Lightweight success-or-error object for functions that return no value.
///
/// An ok Status has code `kOk` and an empty message.  An error Status has a
/// non-ok code and a diagnostic message.
///
/// Status is copyable and movable.  Pass by value, const reference, or move.
class Status {
public:
    Status() = default;

    /// Constructs an error Status from a code and message.
    ///
    /// @param code Must not be `kOk` — callers should use `ok_status()` for
    /// the success case.
    /// @param message Human-readable diagnostic.
    Status(StatusCode code, std::string message)
        : code_(code), message_(std::move(message)) {}

    /// Returns an ok Status (code `kOk`, empty message).
    static Status ok_status() {
        return Status();
    }

    /// Returns a `kInvalidArgument` error with the given message.
    static Status invalid_argument(std::string message) {
        return Status(StatusCode::kInvalidArgument, std::move(message));
    }

    /// Returns a `kNotFound` error with the given message.
    static Status not_found(std::string message) {
        return Status(StatusCode::kNotFound, std::move(message));
    }

    /// Returns a `kInternal` error with the given message.
    static Status internal(std::string message) {
        return Status(StatusCode::kInternal, std::move(message));
    }

    /// True when the Status represents success (`kOk`).
    bool ok() const {
        return code_ == StatusCode::kOk;
    }

    /// The status code of this result.
    StatusCode code() const {
        return code_;
    }

    /// Diagnostic message. Empty for ok Statuses.
    const std::string& message() const {
        return message_;
    }

private:
    StatusCode code_ = StatusCode::kOk;
    std::string message_;
};

} // namespace nexus
