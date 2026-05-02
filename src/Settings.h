#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace WalkSpeedTuner::Settings {

    struct ConfigDocument {
        bool  enabled            = true;
        float boost_pct          = 0.0f;
        bool  suppress_in_combat = true;
        bool  sync_animation     = false;
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
