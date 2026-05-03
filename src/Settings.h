#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace WalkSpeedTuner::Settings {

    struct ConfigDocument {
        bool          enabled            = true;
        float         boost_pct          = 0.0f;
        bool          suppress_in_combat = true;
        // Defaults: Alt+WheelUp / Alt+WheelDown.
        // 0x108 = kMouseBase + kWheelUp (8); 0x109 = kMouseBase + kWheelDown (9).
        // 0x02 = kModAlt.
        std::uint32_t boost_up_keycode   = 0x108;
        std::uint8_t  boost_up_mods      = 0x02;
        std::uint32_t boost_down_keycode = 0x109;
        std::uint8_t  boost_down_mods    = 0x02;
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
