#pragma once

namespace WalkSpeedTuner::WalkSpeedHook {

    bool Install();

    void SetEnabled(bool enabled);
    void SetBoostPercent(float pct);
    void SetSuppressInCombat(bool suppress);
    void SetSyncAnimation(bool sync);

    void TickleNow();

    bool  GetEnabled();
    float GetBoostPercent();
    bool  GetSuppressInCombat();
    bool  GetSyncAnimation();

    bool        IsBoostActiveRightNow();
    const char* InactiveReason();

}
