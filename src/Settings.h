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
        bool          sync_animation     = false;
        std::uint32_t boost_up_keycode   = 0;
        std::uint8_t  boost_up_mods      = 0;
        std::uint32_t boost_down_keycode = 0;
        std::uint8_t  boost_down_mods    = 0;
    };

    // Pure helpers (testable, no fs / no SKSE).
    ConfigDocument ParseDocumentFromJson(std::string_view text);
    std::string    DocumentToJsonString(const ConfigDocument& doc);

    // Side-effecting wrappers (link SKSE).
    std::filesystem::path FilePath();
    void                  Load();
    void                  Apply(const ConfigDocument& doc);
    void                  Save(const ConfigDocument& doc);
    ConfigDocument        CurrentDocument();

}
