#include <catch2/catch_test_macros.hpp>

#include "HookLogic.h"

using WalkSpeedTuner::HookLogic::BoostInputs;
using WalkSpeedTuner::HookLogic::ComputeBoosted;

TEST_CASE("returns base when disabled", "[compute]") {
    BoostInputs in{ 100.0f, 50.0f, /*enabled*/ false,
                    /*walking*/ true, /*combat*/ false, /*suppress*/ true };
    REQUIRE(ComputeBoosted(in) == 100.0f);
}

TEST_CASE("returns base when boost is zero", "[compute]") {
    BoostInputs in{ 100.0f, 0.0f, true, true, false, true };
    REQUIRE(ComputeBoosted(in) == 100.0f);
}

TEST_CASE("returns base when not walking", "[compute]") {
    BoostInputs in{ 100.0f, 30.0f, true, /*walking*/ false, false, true };
    REQUIRE(ComputeBoosted(in) == 100.0f);
}

TEST_CASE("suppresses in combat when toggle on", "[compute]") {
    BoostInputs in{ 100.0f, 30.0f, true, true, /*combat*/ true, /*suppress*/ true };
    REQUIRE(ComputeBoosted(in) == 100.0f);
}

TEST_CASE("applies in combat when suppress toggle off", "[compute]") {
    BoostInputs in{ 100.0f, 30.0f, true, true, /*combat*/ true, /*suppress*/ false };
    REQUIRE(ComputeBoosted(in) == 130.0f);
}

TEST_CASE("preserves perk-stacked base", "[compute]") {
    // base=148 simulates 100 base + 5 perk + 3 enchant + 10 shout + 30 sneak-buff
    BoostInputs in{ 148.0f, 25.0f, true, true, false, true };
    REQUIRE(ComputeBoosted(in) == 173.0f);
}

TEST_CASE("happy path", "[compute]") {
    BoostInputs in{ 100.0f, 50.0f, true, true, false, true };
    REQUIRE(ComputeBoosted(in) == 150.0f);
}

TEST_CASE("standing still in walk gait still gates on walking flag", "[compute]") {
    // walking flag is true even when stopped (Always Walk engaged but no input);
    // boost is added to AV reads, but no movement so no observable effect.
    BoostInputs in{ 100.0f, 50.0f, true, /*walking*/ true, false, true };
    REQUIRE(ComputeBoosted(in) == 150.0f);
}

TEST_CASE("zero base with boost", "[compute]") {
    BoostInputs in{ 0.0f, 30.0f, true, true, false, true };
    REQUIRE(ComputeBoosted(in) == 30.0f);
}
