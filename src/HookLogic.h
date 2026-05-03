#pragma once

#include <algorithm>
#include <cstdint>

namespace WalkSpeedTuner::HookLogic {

    inline constexpr float kMaxBoostPct   = 75.0f;
    inline constexpr float kHotkeyStepPct = 5.0f;

    // Extended scan-code encoding (SkyUI/MCM-Helper convention) so mouse
    // wheel/buttons don't collide with keyboard scan codes.
    inline constexpr std::uint32_t kKbBase     = 0x000;   // 0x000-0x0FF DX scan codes
    inline constexpr std::uint32_t kMouseBase  = 0x100;   // 0x100-0x109 mouse buttons + wheel
    inline constexpr std::uint32_t kKeycodeMax = 0x1FF;

    // device matches RE::INPUT_DEVICE: kKeyboard=0, kMouse=1, kGamepad=2.
    inline std::uint32_t EncodeKeycode(int device, std::uint32_t idCode) {
        if (device == 0) return kKbBase    + idCode;
        if (device == 1) return kMouseBase + idCode;
        return 0;
    }

    struct BoostInputs {
        float base;
        float boost_pct;
        bool  enabled;
        bool  walking;
        bool  in_combat;
        bool  suppress_in_combat;
    };

    inline bool IsBoostActive(const BoostInputs& in) {
        return in.enabled
            && in.boost_pct > 0.0f
            && in.walking
            && !(in.suppress_in_combat && in.in_combat);
    }

    inline float ComputeBoosted(const BoostInputs& in) {
        return IsBoostActive(in) ? (in.base + in.boost_pct) : in.base;
    }

    inline bool ShouldTickle(std::int64_t now_ns,
                             std::int64_t last_ns,
                             std::int64_t throttle_ns) {
        return (now_ns - last_ns) >= throttle_ns;
    }

    inline float BumpBoost(float current, float step, float max) {
        return std::clamp(current + step, 0.0f, max);
    }

    enum class HotkeyAction { kNone, kBoostUp, kBoostDown };

    struct HotkeyConfig {
        std::uint32_t up_key;
        std::uint8_t  up_mods;
        std::uint32_t down_key;
        std::uint8_t  down_mods;
    };

    inline HotkeyAction MatchHotkey(std::uint32_t key, std::uint8_t mods,
                                    const HotkeyConfig& cfg) {
        if (key == 0) return HotkeyAction::kNone;
        if (cfg.up_key   != 0 && key == cfg.up_key   && mods == cfg.up_mods)
            return HotkeyAction::kBoostUp;
        if (cfg.down_key != 0 && key == cfg.down_key && mods == cfg.down_mods)
            return HotkeyAction::kBoostDown;
        return HotkeyAction::kNone;
    }

}
