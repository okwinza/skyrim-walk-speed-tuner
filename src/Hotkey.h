#pragma once

#include <cstdint>
#include <string>

namespace RE { class ButtonEvent; }

namespace WalkSpeedTuner::Hotkey {

    enum class Target { kNone, kBoostUp, kBoostDown };

    constexpr std::uint8_t kModCtrl  = 1 << 0;
    constexpr std::uint8_t kModAlt   = 1 << 1;
    constexpr std::uint8_t kModShift = 1 << 2;

    void Install();
    void Uninstall();

    void   BeginCapture(Target t);
    void   CancelCapture();
    Target CapturingTarget();

    void          SetBoostUpKey(std::uint32_t code, std::uint8_t mods);
    void          SetBoostDownKey(std::uint32_t code, std::uint8_t mods);
    std::uint32_t GetBoostUpKey();
    std::uint8_t  GetBoostUpMods();
    std::uint32_t GetBoostDownKey();
    std::uint8_t  GetBoostDownMods();

    void ResetModifierState();

    std::string ChordName(std::uint32_t code, std::uint8_t mods);

    // Returns true if `ev` is a button event whose (key, mods) chord matches
    // a currently-bound hotkey. Used by InputSuppress to gate vanilla camera
    // zoom for wheel-bound chords. Mouse-device only — keyboard keys never
    // trigger vanilla camera, so we don't gate them here.
    bool ShouldSuppressForChord(const RE::ButtonEvent* ev);

}
