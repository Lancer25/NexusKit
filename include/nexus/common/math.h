#pragma once

#include <algorithm>
#include <initializer_list>

/// Simple math helpers supplementing `<algorithm>`.
namespace nexus::common {

/// Returns the smallest value in `values`.
template <typename T>
T min(std::initializer_list<T> values) {
    return std::min(values);
}

/// Returns the largest value in `values`.
template <typename T>
T max(std::initializer_list<T> values) {
    return std::max(values);
}

/// Clamps `value` to the inclusive range [lo, hi].
template <typename T>
T clamp(T value, T lo, T hi) {
    return std::max(lo, std::min(hi, value));
}

/// Unsigned distance between two values.
template <typename T>
T absolute_difference(T a, T b) {
    return a > b ? static_cast<T>(a - b) : static_cast<T>(b - a);
}

} // namespace nexus::common
