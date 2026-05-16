#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "HookLogic.h"

using WalkSpeedTuner::HookLogic::IndicatorAlpha;
using WalkSpeedTuner::HookLogic::IndicatorPos;
using WalkSpeedTuner::HookLogic::ScreenPoint;
using WalkSpeedTuner::HookLogic::kIndFadeInNs;
using WalkSpeedTuner::HookLogic::kIndHoldNs;
using WalkSpeedTuner::HookLogic::kIndFadeOutNs;

// ---- IndicatorAlpha — fade envelope ----------------------------------

TEST_CASE("alpha is zero before the ping", "[indicator]") {
    REQUIRE(IndicatorAlpha(-1) == 0.0f);
}

TEST_CASE("alpha is zero at the start of fade-in", "[indicator]") {
    REQUIRE(IndicatorAlpha(0) == 0.0f);
}

TEST_CASE("alpha ramps within the fade-in window", "[indicator]") {
    const float a = IndicatorAlpha(kIndFadeInNs / 2);
    REQUIRE(a > 0.0f);
    REQUIRE(a < 1.0f);
}

TEST_CASE("alpha is full during the hold", "[indicator]") {
    REQUIRE(IndicatorAlpha(kIndFadeInNs) == 1.0f);
    REQUIRE(IndicatorAlpha(kIndFadeInNs + kIndHoldNs / 2) == 1.0f);
}

TEST_CASE("alpha decreases during fade-out", "[indicator]") {
    const std::int64_t base = kIndFadeInNs + kIndHoldNs;
    const float early = IndicatorAlpha(base + kIndFadeOutNs / 4);
    const float late  = IndicatorAlpha(base + (kIndFadeOutNs * 3) / 4);
    REQUIRE(early > late);
    REQUIRE(early <= 1.0f);
    REQUIRE(late  >= 0.0f);
}

TEST_CASE("alpha is zero past the total window", "[indicator]") {
    REQUIRE(IndicatorAlpha(kIndFadeInNs + kIndHoldNs + kIndFadeOutNs) == 0.0f);
}

TEST_CASE("alpha is zero for a first-frame huge elapsed", "[indicator]") {
    // g_last_ping_ns defaults to 0, so before any Ping() the elapsed time is
    // NowNs() - 0 — an enormous value. It must read as "not showing".
    REQUIRE(IndicatorAlpha(9'000'000'000'000'000LL) == 0.0f);
}

// ---- IndicatorPos — anchor + nudge -----------------------------------
// Viewport: origin (0,0), size 1000 x 800, default margin 16.

TEST_CASE("bottom-right anchor sits at the lower-right inset", "[indicator]") {
    const ScreenPoint p = IndicatorPos(0, 0, 0, 0.0f, 0.0f, 1000.0f, 800.0f);
    REQUIRE(p.x == 1000.0f - 16.0f);
    REQUIRE(p.y == 800.0f - 16.0f);
    REQUIRE(p.pivot_x == 1.0f);
    REQUIRE(p.pivot_y == 1.0f);
}

TEST_CASE("top-left anchor sits at the upper-left inset", "[indicator]") {
    const ScreenPoint p = IndicatorPos(3, 0, 0, 0.0f, 0.0f, 1000.0f, 800.0f);
    REQUIRE(p.x == 16.0f);
    REQUIRE(p.y == 16.0f);
    REQUIRE(p.pivot_x == 0.0f);
    REQUIRE(p.pivot_y == 0.0f);
}

TEST_CASE("centered anchors use a 0.5 horizontal pivot", "[indicator]") {
    const ScreenPoint bc = IndicatorPos(1, 0, 0, 0.0f, 0.0f, 1000.0f, 800.0f);
    REQUIRE(bc.x == 500.0f);
    REQUIRE(bc.pivot_x == 0.5f);
    REQUIRE(bc.pivot_y == 1.0f);
    const ScreenPoint tc = IndicatorPos(2, 0, 0, 0.0f, 0.0f, 1000.0f, 800.0f);
    REQUIRE(tc.x == 500.0f);
    REQUIRE(tc.pivot_y == 0.0f);
}

TEST_CASE("offset shifts the draw point", "[indicator]") {
    const ScreenPoint p = IndicatorPos(3, 30, 40, 0.0f, 0.0f, 1000.0f, 800.0f);
    REQUIRE(p.x == 16.0f + 30.0f);
    REQUIRE(p.y == 16.0f + 40.0f);
}

TEST_CASE("extreme offset is clamped into the work rect", "[indicator]") {
    const ScreenPoint p = IndicatorPos(3, 100000, 100000, 0.0f, 0.0f, 1000.0f, 800.0f);
    REQUIRE(p.x == 1000.0f - 16.0f);
    REQUIRE(p.y == 800.0f - 16.0f);
}

TEST_CASE("unknown corner falls back to bottom-right", "[indicator]") {
    const ScreenPoint p = IndicatorPos(99, 0, 0, 0.0f, 0.0f, 1000.0f, 800.0f);
    REQUIRE(p.x == 1000.0f - 16.0f);
    REQUIRE(p.y == 800.0f - 16.0f);
    REQUIRE(p.pivot_x == 1.0f);
}

TEST_CASE("non-zero viewport origin offsets the anchor", "[indicator]") {
    const ScreenPoint p = IndicatorPos(3, 0, 0, 100.0f, 50.0f, 1000.0f, 800.0f);
    REQUIRE(p.x == 100.0f + 16.0f);
    REQUIRE(p.y == 50.0f + 16.0f);
}
