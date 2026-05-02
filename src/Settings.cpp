#include "PCH.h"

#include "Settings.h"

#include <fstream>
#include <sstream>
#include <system_error>

#include <spdlog/spdlog.h>

#include "WalkSpeedHook.h"

namespace WalkSpeedTuner::Settings {

    std::filesystem::path FilePath() {
        return std::filesystem::path("Data") / "SKSE" / "Plugins" / "WalkSpeedTuner.json";
    }

    void Apply(const ConfigDocument& d) {
        WalkSpeedHook::SetEnabled(d.enabled);
        WalkSpeedHook::SetBoostPercent(d.boost_pct);
        WalkSpeedHook::SetSuppressInCombat(d.suppress_in_combat);
        WalkSpeedHook::SetSyncAnimation(d.sync_animation);

        spdlog::info("[Settings] applied: enabled={} boost={:.1f}% suppress_in_combat={} sync_animation={}",
                     d.enabled, d.boost_pct, d.suppress_in_combat, d.sync_animation);
    }

    void Save(const ConfigDocument& d) {
        Apply(d);

        const auto path = FilePath();
        const auto tmp  = std::filesystem::path(path).concat(".tmp");
        const auto json = DocumentToJsonString(d);

        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            spdlog::warn("[Settings] mkdir failed: {}", ec.message());
            return;
        }

        {
            std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
            if (!ofs) {
                spdlog::warn("[Settings] could not open tmp for write: {}", tmp.string());
                return;
            }
            ofs << json;
        }

        std::filesystem::rename(tmp, path, ec);
        if (ec) {
            spdlog::warn("[Settings] atomic rename failed: {}", ec.message());
            std::filesystem::remove(tmp, ec);
            return;
        }
        spdlog::info("[Settings] wrote {}", path.string());
    }

    void Load() {
        const auto path = FilePath();

        // Open directly — avoids TOCTOU race between exists() and open().
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            spdlog::info("[Settings] {} missing or unreadable — applying defaults and writing template",
                         path.string());
            Save(ConfigDocument{});
            return;
        }

        std::ostringstream oss;
        oss << ifs.rdbuf();
        const auto doc = ParseDocumentFromJson(oss.str());
        spdlog::info("[Settings] loaded {}", path.string());
        Apply(doc);
    }

    ConfigDocument CurrentDocument() {
        // Source of truth lives in the hook's atomics; rebuild from there
        // rather than maintaining a parallel cache + mutex.
        return ConfigDocument{
            WalkSpeedHook::GetEnabled(),
            WalkSpeedHook::GetBoostPercent(),
            WalkSpeedHook::GetSuppressInCombat(),
            WalkSpeedHook::GetSyncAnimation(),
        };
    }

}
