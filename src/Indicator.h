#pragma once

namespace WalkSpeedTuner::Indicator {

    // Registers the HUD render callback with SKSEMenuFramework. No-op if
    // SKSEMenuFramework isn't installed — the indicator simply won't draw.
    void Install();

    // (Re)starts the fade-in / hold / fade-out timer. Called when the boost
    // changes via the hotkey (or the MCM Preview button).
    void Ping();

    void SetEnabled(bool enabled);
    void SetPosition(int corner);          // anchor: 0=BR 1=BC 2=TC 3=TL
    void SetOffset(int off_x, int off_y);  // px nudge from the anchor
    void SetScale(float scale);            // text size vs. base UI font

    bool  GetEnabled();
    int   GetPosition();
    int   GetOffsetX();
    int   GetOffsetY();
    float GetScale();

}
