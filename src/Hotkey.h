#pragma once

#include <cstdint>
#include <string>

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

}
