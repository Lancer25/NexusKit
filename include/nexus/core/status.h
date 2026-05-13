#pragma once

#include <string>
#include <utility>

namespace nexus {

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

class Status {
public:
    Status() = default;

    Status(StatusCode code, std::string message)
        : code_(code), message_(std::move(message)) {}

    static Status ok_status() {
        return Status();
    }

    static Status invalid_argument(std::string message) {
        return Status(StatusCode::kInvalidArgument, std::move(message));
    }

    static Status not_found(std::string message) {
        return Status(StatusCode::kNotFound, std::move(message));
    }

    static Status internal(std::string message) {
        return Status(StatusCode::kInternal, std::move(message));
    }

    bool ok() const {
        return code_ == StatusCode::kOk;
    }

    StatusCode code() const {
        return code_;
    }

    const std::string& message() const {
        return message_;
    }

private:
    StatusCode code_ = StatusCode::kOk;
    std::string message_;
};

} // namespace nexus
