#include <catch2/catch_test_macros.hpp>

#include "HookLogic.h"

using WalkSpeedTuner::HookLogic::BumpBoost;

TEST_CASE("BumpBoost up from zero", "[bump]") {
    REQUIRE(BumpBoost(0.0f, +10.0f, 75.0f) == 10.0f);
}

TEST_CASE("BumpBoost up at max saturates", "[bump]") {
    REQUIRE(BumpBoost(75.0f, +10.0f, 75.0f) == 75.0f);
}

TEST_CASE("BumpBoost up just below max clamps to max", "[bump]") {
    REQUIRE(BumpBoost(70.0f, +10.0f, 75.0f) == 75.0f);
}

TEST_CASE("BumpBoost down at zero saturates", "[bump]") {
    REQUIRE(BumpBoost(0.0f, -10.0f, 75.0f) == 0.0f);
}

TEST_CASE("BumpBoost down from max", "[bump]") {
    REQUIRE(BumpBoost(75.0f, -10.0f, 75.0f) == 65.0f);
}

TEST_CASE("BumpBoost down just above zero saturates", "[bump]") {
    REQUIRE(BumpBoost(5.0f, -10.0f, 75.0f) == 0.0f);
}

TEST_CASE("BumpBoost step zero is identity", "[bump]") {
    REQUIRE(BumpBoost(30.0f, 0.0f, 75.0f) == 30.0f);
}
