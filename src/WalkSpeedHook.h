#pragma once

namespace WalkSpeedTuner::WalkSpeedHook {

    bool Install();

    void SetEnabled(bool enabled);
    void SetBoostPercent(float pct);
    void SetSuppressInCombat(bool suppress);
    void SetLimits(float lower, float upper);

    void TickleNow();

    bool  GetEnabled();
    float GetBoostPercent();
    bool  GetSuppressInCombat();
    float GetMinLimit();
    float GetMaxLimit();

}
