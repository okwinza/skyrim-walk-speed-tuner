#include <catch2/catch_test_macros.hpp>

#include "HookLogic.h"

using WalkSpeedTuner::HookLogic::ShouldTickle;

constexpr std::int64_t kThrottleNs = 25 * 1'000'000;  // 25 ms

TEST_CASE("first tickle fires (last=0)", "[throttle]") {
    REQUIRE(ShouldTickle(/*now*/ 1'000'000'000, /*last*/ 0, kThrottleNs));
}

TEST_CASE("rapid second tickle suppressed", "[throttle]") {
    const std::int64_t t0 = 1'000'000'000;
    REQUIRE(ShouldTickle(t0, 0, kThrottleNs));
    REQUIRE_FALSE(ShouldTickle(t0 + 10'000'000, t0, kThrottleNs));   // 10 ms later
}

TEST_CASE("tickle fires after throttle window", "[throttle]") {
    const std::int64_t t0 = 1'000'000'000;
    REQUIRE(ShouldTickle(t0 + 26'000'000, t0, kThrottleNs));         // 26 ms later
}

TEST_CASE("exact-window edge fires (>=)", "[throttle]") {
    REQUIRE(ShouldTickle(kThrottleNs, 0, kThrottleNs));
}

TEST_CASE("just-below window suppressed", "[throttle]") {
    REQUIRE_FALSE(ShouldTickle(kThrottleNs - 1, 0, kThrottleNs));
}
