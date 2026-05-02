#include <catch2/catch_test_macros.hpp>

#include "HookLogic.h"

using WalkSpeedTuner::HookLogic::HotkeyAction;
using WalkSpeedTuner::HookLogic::HotkeyConfig;
using WalkSpeedTuner::HookLogic::MatchHotkey;

constexpr std::uint8_t kModCtrl  = 1 << 0;
constexpr std::uint8_t kModAlt   = 1 << 1;
constexpr std::uint8_t kModShift = 1 << 2;

TEST_CASE("zero key never matches", "[match]") {
    HotkeyConfig cfg{ 0x1B, 0, 0x1A, 0 };
    REQUIRE(MatchHotkey(0, 0, cfg) == HotkeyAction::kNone);
}

TEST_CASE("plain key matches up", "[match]") {
    HotkeyConfig cfg{ 0x1B, 0, 0, 0 };
    REQUIRE(MatchHotkey(0x1B, 0, cfg) == HotkeyAction::kBoostUp);
}

TEST_CASE("key matches but mods don't returns none", "[match]") {
    HotkeyConfig cfg{ 0x1B, 0, 0, 0 };
    REQUIRE(MatchHotkey(0x1B, kModShift, cfg) == HotkeyAction::kNone);
}

TEST_CASE("chord matches down", "[match]") {
    HotkeyConfig cfg{ 0, 0, 0x1B, kModShift };
    REQUIRE(MatchHotkey(0x1B, kModShift, cfg) == HotkeyAction::kBoostDown);
}

TEST_CASE("same key different mods routes to up vs down", "[match]") {
    HotkeyConfig cfg{ 0x1B, 0, 0x1B, kModShift };
    REQUIRE(MatchHotkey(0x1B, 0,         cfg) == HotkeyAction::kBoostUp);
    REQUIRE(MatchHotkey(0x1B, kModShift, cfg) == HotkeyAction::kBoostDown);
}

TEST_CASE("identical chord collision: up wins", "[match]") {
    HotkeyConfig cfg{ 0x1B, 0, 0x1B, 0 };
    REQUIRE(MatchHotkey(0x1B, 0, cfg) == HotkeyAction::kBoostUp);
}

TEST_CASE("unbound down doesn't fire on unrelated key", "[match]") {
    HotkeyConfig cfg{ 0x1B, 0, 0, 0 };
    REQUIRE(MatchHotkey(0x39, 0, cfg) == HotkeyAction::kNone);
}

TEST_CASE("ctrl+alt+shift triple-mod chord", "[match]") {
    const std::uint8_t triple = kModCtrl | kModAlt | kModShift;
    HotkeyConfig cfg{ 0, 0, 0x21, triple };
    REQUIRE(MatchHotkey(0x21, triple,   cfg) == HotkeyAction::kBoostDown);
    REQUIRE(MatchHotkey(0x21, kModCtrl, cfg) == HotkeyAction::kNone);
}

TEST_CASE("only down bound, up keycode 0 does not match anything", "[match]") {
    HotkeyConfig cfg{ 0, 0, 0x1A, kModAlt };
    REQUIRE(MatchHotkey(0x1A, kModAlt, cfg) == HotkeyAction::kBoostDown);
    REQUIRE(MatchHotkey(0,    0,        cfg) == HotkeyAction::kNone);
}
