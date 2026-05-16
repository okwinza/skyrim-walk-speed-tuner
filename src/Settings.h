#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace WalkSpeedTuner::Settings {

    struct ConfigDocument {
        bool          enabled            = true;
        float         boost_pct          = 0.0f;
        // User-configurable bounds for boost_pct. Lower is always <= 0,
        // upper always >= 0 (see the HookLogic limit-slider constants).
        float         min_limit          = -20.0f;
        float         max_limit          = 140.0f;
        bool          suppress_in_combat = true;
        // Defaults: Alt+WheelUp / Alt+WheelDown.
        // 0x108 = kMouseBase + kWheelUp (8); 0x109 = kMouseBase + kWheelDown (9).
        // 0x02 = kModAlt.
        std::uint32_t boost_up_keycode   = 0x108;
        std::uint8_t  boost_up_mods      = 0x02;
        std::uint32_t boost_down_keycode = 0x109;
        std::uint8_t  boost_down_mods    = 0x02;
        // Quick-reset hotkey — default Alt + MMB (mouse-wheel press).
        std::uint32_t reset_keycode      = 0x102;
        std::uint8_t  reset_mods         = 0x02;
        // Indicator: a brief HUD readout that pops in on a hotkey boost change.
        bool          show_indicator     = true;
        int           indicator_position = 0;   // anchor: 0=BR 1=BC 2=TC 3=TL
        int           indicator_offset_x = 0;   // px X nudge from the anchor
        int           indicator_offset_y = 0;   // px Y nudge from the anchor
        float         indicator_scale    = 1.6f;  // text size vs. base UI font
    };

    // Pure helpers (testable, no fs / no SKSE).
    ConfigDocument ParseDocumentFromJson(std::string_view text);
    std::string    DocumentToJsonString(const ConfigDocument& doc);

    // Side-effecting wrappers (link SKSE).
    std::filesystem::path FilePath();
    void                  Load();
    void                  Apply(const ConfigDocument& doc);
    void                  Save(const ConfigDocument& doc);
    // Write-only JSON persistence (skips Apply re-invocation). Call from
    // hotkey path; atomics are already authoritative.
    void                  Persist();
    ConfigDocument        CurrentDocument();

}
