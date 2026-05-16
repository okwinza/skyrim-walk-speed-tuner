#include <catch2/catch_test_macros.hpp>

#include "HookLogic.h"

using WalkSpeedTuner::HookLogic::EncodeKeycode;
using WalkSpeedTuner::HookLogic::HotkeyAction;
using WalkSpeedTuner::HookLogic::HotkeyConfig;
using WalkSpeedTuner::HookLogic::kKbBase;
using WalkSpeedTuner::HookLogic::kMouseBase;
using WalkSpeedTuner::HookLogic::kKeycodeMax;
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

// ---- v1.2: extended scan-code encoding ---------------------------------

TEST_CASE("encoding round-trip: keyboard pass-through (v1.1 backwards-compat)", "[encoding]") {
    // Locks the invariant that v1.1 JSONs (keyboard codes 0-0xFF) work unchanged.
    REQUIRE(EncodeKeycode(0, 0x1B) == 0x1Bu);
    REQUIRE(EncodeKeycode(0, 0x00) == 0x00u);
    REQUIRE(EncodeKeycode(0, 0xFF) == 0xFFu);
}

TEST_CASE("encoding round-trip: mouse offset", "[encoding]") {
    REQUIRE(EncodeKeycode(1, 0x00) == 0x100u);  // LMB
    REQUIRE(EncodeKeycode(1, 0x02) == 0x102u);  // MMB
    REQUIRE(EncodeKeycode(1, 0x08) == 0x108u);  // wheel up
    REQUIRE(EncodeKeycode(1, 0x09) == 0x109u);  // wheel down
}

TEST_CASE("encoding rejects unknown device", "[encoding]") {
    REQUIRE(EncodeKeycode(2, 0x08) == 0u);   // gamepad — not supported in v1.2
    REQUIRE(EncodeKeycode(99, 0x42) == 0u);  // garbage device
}

TEST_CASE("disambiguation: Backspace bind doesn't fire on wheel-up", "[encoding]") {
    // cfg binds keyboard Backspace (0x0E) to Boost-.
    HotkeyConfig cfg{ 0, 0, /*down_key*/ 0x0E, 0 };
    REQUIRE(MatchHotkey(0x0E,  0, cfg) == HotkeyAction::kBoostDown);  // Backspace
    REQUIRE(MatchHotkey(0x108, 0, cfg) == HotkeyAction::kNone);        // wheel up
}

TEST_CASE("disambiguation: WheelUp bind doesn't fire on Backspace", "[encoding]") {
    HotkeyConfig cfg{ /*up_key*/ 0x108, 0, 0, 0 };
    REQUIRE(MatchHotkey(0x108, 0, cfg) == HotkeyAction::kBoostUp);
    REQUIRE(MatchHotkey(0x0E,  0, cfg) == HotkeyAction::kNone);
}

TEST_CASE("Ctrl+WheelUp chord matches", "[encoding]") {
    HotkeyConfig cfg{ /*up_key*/ 0x108, kModCtrl, 0, 0 };
    REQUIRE(MatchHotkey(0x108, kModCtrl, cfg) == HotkeyAction::kBoostUp);
    REQUIRE(MatchHotkey(0x108, 0,         cfg) == HotkeyAction::kNone);  // missing modifier
}

TEST_CASE("WheelUp without modifier and Ctrl+WheelUp route to different actions", "[encoding]") {
    HotkeyConfig cfg{ /*up*/ 0x108, 0, /*down*/ 0x108, kModCtrl };
    REQUIRE(MatchHotkey(0x108, 0,        cfg) == HotkeyAction::kBoostUp);
    REQUIRE(MatchHotkey(0x108, kModCtrl, cfg) == HotkeyAction::kBoostDown);
}

// ---- v1.8: quick-reset hotkey ------------------------------------------

TEST_CASE("reset chord matches", "[match]") {
    HotkeyConfig cfg{ 0, 0, 0, 0, /*reset*/ 0x102, kModAlt };
    REQUIRE(MatchHotkey(0x102, kModAlt, cfg) == HotkeyAction::kReset);
}

TEST_CASE("unbound reset doesn't fire", "[match]") {
    HotkeyConfig cfg{ 0x1B, 0, 0x1A, 0, /*reset*/ 0, 0 };
    REQUIRE(MatchHotkey(0x102, kModAlt, cfg) == HotkeyAction::kNone);
}

TEST_CASE("reset needs its modifier", "[match]") {
    HotkeyConfig cfg{ 0, 0, 0, 0, /*reset*/ 0x102, kModAlt };
    REQUIRE(MatchHotkey(0x102, 0, cfg) == HotkeyAction::kNone);
}

TEST_CASE("up, down, reset route to distinct actions", "[match]") {
    HotkeyConfig cfg{ 0x1B, 0, 0x1A, 0, 0x102, kModAlt };
    REQUIRE(MatchHotkey(0x1B,  0,       cfg) == HotkeyAction::kBoostUp);
    REQUIRE(MatchHotkey(0x1A,  0,       cfg) == HotkeyAction::kBoostDown);
    REQUIRE(MatchHotkey(0x102, kModAlt, cfg) == HotkeyAction::kReset);
}
