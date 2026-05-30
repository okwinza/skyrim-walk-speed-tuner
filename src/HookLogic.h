#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

namespace WalkSpeedTuner::HookLogic {

    inline constexpr float kHotkeyStepPct = 5.0f;

    // The boost range is bounded by two user-configurable limits. These are the
    // ranges of the limit sliders themselves (the meta-bounds):
    //   lower-limit slider spans [-50, 0]; upper-limit slider spans [0, 150].
    // Because the lower limit is always <= 0 and the upper always >= 0, the
    // pair always brackets 0 — vanilla speed stays reachable.
    inline constexpr float kLowerLimitMin = -50.0f;
    inline constexpr float kLowerLimitMax =   0.0f;
    inline constexpr float kUpperLimitMin =   0.0f;
    inline constexpr float kUpperLimitMax = 150.0f;

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
        // Sprint and walk are independent bits in ActorState — both can be
        // true at once if the player holds the walk-toggle while sprinting.
        // Without this gate the boost gets multiplied into sprint speed,
        // which is what users observe as "crazy fast" sprinting when boost
        // is high. Appended (not inserted) so existing test positional inits
        // value-init sprinting to false and stay correct.
        bool  sprinting;
    };

    inline bool IsBoostActive(const BoostInputs& in) {
        return in.enabled
            && in.boost_pct != 0.0f
            && in.walking
            && !in.sprinting
            && !(in.suppress_in_combat && in.in_combat);
    }

    inline float ComputeBoosted(const BoostInputs& in) {
        return IsBoostActive(in) ? (in.base + in.boost_pct) : in.base;
    }

    // Monotonic nanoseconds since the steady-clock epoch. Shared by every
    // module that timestamps events (hook tickle, hotkey debounce, indicator
    // fade): std::atomic<time_point> isn't guaranteed lock-free on MSVC, so
    // callers store this int64 instead.
    inline std::int64_t NowNs() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    inline bool ShouldTickle(std::int64_t now_ns,
                             std::int64_t last_ns,
                             std::int64_t throttle_ns) {
        return (now_ns - last_ns) >= throttle_ns;
    }

    inline float BumpBoost(float current, float step, float min, float max) {
        return std::clamp(current + step, min, max);
    }

    enum class HotkeyAction { kNone, kBoostUp, kBoostDown, kReset };

    struct HotkeyConfig {
        std::uint32_t up_key;
        std::uint8_t  up_mods;
        std::uint32_t down_key;
        std::uint8_t  down_mods;
        std::uint32_t reset_key;
        std::uint8_t  reset_mods;
    };

    inline HotkeyAction MatchHotkey(std::uint32_t key, std::uint8_t mods,
                                    const HotkeyConfig& cfg) {
        if (key == 0) return HotkeyAction::kNone;
        if (cfg.up_key   != 0 && key == cfg.up_key   && mods == cfg.up_mods)
            return HotkeyAction::kBoostUp;
        if (cfg.down_key != 0 && key == cfg.down_key && mods == cfg.down_mods)
            return HotkeyAction::kBoostDown;
        if (cfg.reset_key != 0 && key == cfg.reset_key && mods == cfg.reset_mods)
            return HotkeyAction::kReset;
        return HotkeyAction::kNone;
    }

    // ---- Transient speed indicator (HUD readout) -----------------------

    // Fade envelope: ramp-up, hold at full opacity, ramp-down. All times are
    // nanoseconds since the last Ping().
    inline constexpr std::int64_t kIndFadeInNs  =   120'000'000;  // 0.12 s
    inline constexpr std::int64_t kIndHoldNs    = 1'600'000'000;  // 1.60 s
    inline constexpr std::int64_t kIndFadeOutNs =   400'000'000;  // 0.40 s

    // Indicator text size, as a multiplier on the base UI font — slider range.
    inline constexpr float kIndicatorScaleMin = 1.0f;
    inline constexpr float kIndicatorScaleMax = 3.0f;

    // 0..1 opacity for a given elapsed time. Out-of-window (negative, or past
    // fade-in + hold + fade-out) returns 0 so the caller can skip drawing.
    inline float IndicatorAlpha(std::int64_t elapsed_ns) {
        if (elapsed_ns < 0) return 0.0f;
        if (elapsed_ns < kIndFadeInNs)
            return static_cast<float>(elapsed_ns) / static_cast<float>(kIndFadeInNs);
        const std::int64_t after_in = elapsed_ns - kIndFadeInNs;
        if (after_in < kIndHoldNs) return 1.0f;
        const std::int64_t after_hold = after_in - kIndHoldNs;
        if (after_hold < kIndFadeOutNs)
            return 1.0f - static_cast<float>(after_hold) / static_cast<float>(kIndFadeOutNs);
        return 0.0f;
    }

    // Resolved screen-space draw point + ImGui pivot for the indicator window.
    struct ScreenPoint {
        float x;
        float y;
        float pivot_x;
        float pivot_y;
    };

    // Maps an anchor corner + X/Y pixel nudge to a draw point, given the
    // viewport work area (vx, vy = origin; vw, vh = size). Corner codes:
    //   0 = bottom-right, 1 = bottom-center, 2 = top-center, 3 = top-left.
    // Each corner's pivot grows the window inward, so a point inset by
    // `margin` stays on-screen. The final point is clamped into the work
    // area inset by `margin`.
    inline ScreenPoint IndicatorPos(int corner, int off_x, int off_y,
                                    float vx, float vy, float vw, float vh,
                                    float margin = 16.0f) {
        float x, y, px, py;
        switch (corner) {
            case 1:  // bottom-center
                x = vx + vw * 0.5f;   y = vy + vh - margin; px = 0.5f; py = 1.0f; break;
            case 2:  // top-center
                x = vx + vw * 0.5f;   y = vy + margin;      px = 0.5f; py = 0.0f; break;
            case 3:  // top-left
                x = vx + margin;      y = vy + margin;      px = 0.0f; py = 0.0f; break;
            default: // 0 = bottom-right
                x = vx + vw - margin; y = vy + vh - margin; px = 1.0f; py = 1.0f; break;
        }
        x += static_cast<float>(off_x);
        y += static_cast<float>(off_y);
        x = std::clamp(x, vx + margin, vx + vw - margin);
        y = std::clamp(y, vy + margin, vy + vh - margin);
        return ScreenPoint{ x, y, px, py };
    }

}
