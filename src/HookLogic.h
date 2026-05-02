#pragma once

#include <cstdint>

namespace WalkSpeedTuner::HookLogic {

    inline constexpr float kMaxBoostPct = 75.0f;

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

}
