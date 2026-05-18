#pragma once

#include <string>
#include <utility>

#include <nexus/core/status.h>

/// Common Status factory helpers built on `nexus::Status`.
namespace nexus::common {

/// Returns a `StatusCode::kFailedPrecondition` Status.
inline Status failed_precondition(std::string message) {
    return Status(StatusCode::kFailedPrecondition, std::move(message));
}

/// Returns a `StatusCode::kUnavailable` Status.
inline Status unavailable(std::string message) {
    return Status(StatusCode::kUnavailable, std::move(message));
}

/// Returns a `StatusCode::kInvalidArgument` Status.
inline Status invalid_argument(std::string message) {
    return Status(StatusCode::kInvalidArgument, std::move(message));
}

/// Returns a `StatusCode::kNotFound` Status.
inline Status not_found(std::string message) {
    return Status(StatusCode::kNotFound, std::move(message));
}

/// Returns a `StatusCode::kInternal` Status.
inline Status internal_error(std::string message) {
    return Status(StatusCode::kInternal, std::move(message));
}

} // namespace nexus::common
