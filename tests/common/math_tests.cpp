#include <catch2/catch_test_macros.hpp>

#include <nexus/common/math.h>

TEST_CASE("min returns smallest value") {
    CHECK(nexus::common::min({3, 1, 2}) == 1);
    CHECK(nexus::common::min({5}) == 5);
    CHECK(nexus::common::min({-5, 0, 5}) == -5);
}

TEST_CASE("max returns largest value") {
    CHECK(nexus::common::max({3, 1, 2}) == 3);
    CHECK(nexus::common::max({5}) == 5);
    CHECK(nexus::common::max({-5, 0, 5}) == 5);
}

TEST_CASE("clamp constrains value to range") {
    CHECK(nexus::common::clamp(5, 0, 10) == 5);
    CHECK(nexus::common::clamp(-1, 0, 10) == 0);
    CHECK(nexus::common::clamp(15, 0, 10) == 10);
    CHECK(nexus::common::clamp(0, 0, 10) == 0);
}

TEST_CASE("absolute_difference returns positive distance") {
    CHECK(nexus::common::absolute_difference(5u, 3u) == 2u);
    CHECK(nexus::common::absolute_difference(3u, 5u) == 2u);
    CHECK(nexus::common::absolute_difference(7u, 7u) == 0u);
    CHECK(nexus::common::absolute_difference(5, -3) == 8);
}
