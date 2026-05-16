#include <catch2/catch_test_macros.hpp>

#include "HookLogic.h"

using WalkSpeedTuner::HookLogic::BumpBoost;

// ---- Generic clamp behavior (explicit min/max) -----------------------

TEST_CASE("BumpBoost up from zero", "[bump]") {
    REQUIRE(BumpBoost(0.0f, +10.0f, 0.0f, 75.0f) == 10.0f);
}

TEST_CASE("BumpBoost up at max saturates", "[bump]") {
    REQUIRE(BumpBoost(75.0f, +10.0f, 0.0f, 75.0f) == 75.0f);
}

TEST_CASE("BumpBoost up just below max clamps to max", "[bump]") {
    REQUIRE(BumpBoost(70.0f, +10.0f, 0.0f, 75.0f) == 75.0f);
}

TEST_CASE("BumpBoost down from max", "[bump]") {
    REQUIRE(BumpBoost(75.0f, -10.0f, 0.0f, 75.0f) == 65.0f);
}

TEST_CASE("BumpBoost step zero is identity", "[bump]") {
    REQUIRE(BumpBoost(30.0f, 0.0f, 0.0f, 75.0f) == 30.0f);
}

// ---- Signed range — e.g. the default -20 % .. 140 % boost band -------

TEST_CASE("BumpBoost down from zero reaches negative", "[bump]") {
    REQUIRE(BumpBoost(0.0f, -5.0f, -20.0f, 140.0f) == -5.0f);
}

TEST_CASE("BumpBoost down saturates at the negative min", "[bump]") {
    REQUIRE(BumpBoost(-20.0f, -5.0f, -20.0f, 140.0f) == -20.0f);
}

TEST_CASE("BumpBoost down from just above min clamps to min", "[bump]") {
    REQUIRE(BumpBoost(-17.0f, -5.0f, -20.0f, 140.0f) == -20.0f);
}

TEST_CASE("BumpBoost up from the negative min", "[bump]") {
    REQUIRE(BumpBoost(-20.0f, +5.0f, -20.0f, 140.0f) == -15.0f);
}

TEST_CASE("BumpBoost up saturates at the max", "[bump]") {
    REQUIRE(BumpBoost(140.0f, +5.0f, -20.0f, 140.0f) == 140.0f);
}

TEST_CASE("BumpBoost crosses zero cleanly", "[bump]") {
    REQUIRE(BumpBoost(-5.0f, +5.0f, -20.0f, 140.0f) == 0.0f);
}
