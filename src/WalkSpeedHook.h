#pragma once

namespace WalkSpeedTuner::WalkSpeedHook {

    bool Install();

    void SetEnabled(bool enabled);
    void SetBoostPercent(float pct);
    void SetSuppressInCombat(bool suppress);

    void TickleNow();

    bool  GetEnabled();
    float GetBoostPercent();
    bool  GetSuppressInCombat();

}
