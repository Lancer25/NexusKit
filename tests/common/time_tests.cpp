#include <catch2/catch_test_macros.hpp>

#include <thread>

#include <nexus/common/time.h>

TEST_CASE("steady_timestamp_ms returns positive value") {
    const auto ts = nexus::common::steady_timestamp_ms();
    CHECK(ts > 0);
}

TEST_CASE("steady_timestamp_ms is monotonic") {
    const auto before = nexus::common::steady_timestamp_ms();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    const auto after = nexus::common::steady_timestamp_ms();
    CHECK(after > before);
}

TEST_CASE("system_timestamp_ms returns reasonable value") {
    const auto ts = nexus::common::system_timestamp_ms();
    CHECK(ts > 1700000000000LL);
    CHECK(ts < 3000000000000LL);
}
