// Pure JSON parse/serialize — no SKSE, no filesystem, no logging.
// Linked into both the plugin DLL and the test executable. Tests can't depend
// on spdlog, so this is intentionally a silent fork of NaturalFitness's
// ReadFloat/ReadBool helpers (NF's emit info/warn logs inline, which would
// pull spdlog into the test binary).

#include "Settings.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "HookLogic.h"

namespace WalkSpeedTuner::Settings {

    namespace {
        using nlohmann::json;

        float ReadFloatClamped(const json& j, const char* key, float def, float lo, float hi) {
            if (!j.is_object() || !j.contains(key)) return def;
            try {
                if (!j.at(key).is_number()) return def;
                return std::clamp(j.at(key).get<float>(), lo, hi);
            } catch (...) {
                return def;
            }
        }

        bool ReadBool(const json& j, const char* key, bool def) {
            if (!j.is_object() || !j.contains(key)) return def;
            try {
                if (j.at(key).is_boolean()) return j.at(key).get<bool>();
                if (j.at(key).is_number())  return j.at(key).get<int>() != 0;
                return def;
            } catch (...) {
                return def;
            }
        }
    }

    ConfigDocument ParseDocumentFromJson(std::string_view text) {
        ConfigDocument d{};
        json j;
        try {
            j = json::parse(text, /*cb*/ nullptr, /*allow_exceptions*/ true,
                            /*ignore_comments*/ true);
        } catch (...) {
            return d;
        }
        d.enabled            = ReadBool(j,         "enabled",            d.enabled);
        d.boost_pct          = ReadFloatClamped(j, "boost_pct",          d.boost_pct,
                                                0.0f, HookLogic::kMaxBoostPct);
        d.suppress_in_combat = ReadBool(j,         "suppress_in_combat", d.suppress_in_combat);
        d.sync_animation     = ReadBool(j,         "sync_animation",     d.sync_animation);
        return d;
    }

    std::string DocumentToJsonString(const ConfigDocument& d) {
        nlohmann::json j = {
            { "enabled",            d.enabled },
            { "boost_pct",          d.boost_pct },
            { "suppress_in_combat", d.suppress_in_combat },
            { "sync_animation",     d.sync_animation },
        };
        return j.dump(/*indent*/ 2);
    }

}
