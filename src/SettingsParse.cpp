// Pure JSON parse/serialize — no SKSE, no filesystem, no logging.
// Linked into both the plugin DLL and the test executable. Tests can't depend
// on spdlog, so this is intentionally a silent fork of NaturalFitness's
// ReadFloat/ReadBool helpers (NF's emit info/warn logs inline, which would
// pull spdlog into the test binary).

#include "Settings.h"

#include <algorithm>
#include <cstdint>

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

        std::uint32_t ReadUInt32Clamped(const json& j, const char* key,
                                        std::uint32_t def, std::uint32_t lo, std::uint32_t hi) {
            if (!j.is_object() || !j.contains(key)) return def;
            try {
                if (!j.at(key).is_number()) return def;
                const auto raw = j.at(key).get<long long>();
                if (raw < 0) return lo;
                const auto v = static_cast<std::uint32_t>(std::min<long long>(raw, hi));
                return std::clamp(v, lo, hi);
            } catch (...) {
                return def;
            }
        }

        std::uint8_t ReadUInt8Clamped(const json& j, const char* key,
                                      std::uint8_t def, std::uint8_t lo, std::uint8_t hi) {
            return static_cast<std::uint8_t>(ReadUInt32Clamped(j, key, def, lo, hi));
        }

        // Signed clamped int reader. ReadUInt32Clamped can't be reused — it
        // floors negatives to `lo` — but ReadFloatClamped is negative-safe and
        // structurally identical, so delegate to it (as ReadUInt8Clamped
        // delegates to ReadUInt32Clamped). The config ints are small and exact
        // in float, so the round-trip is lossless.
        int ReadInt32Clamped(const json& j, const char* key, int def, int lo, int hi) {
            return static_cast<int>(ReadFloatClamped(j, key, static_cast<float>(def),
                                                     static_cast<float>(lo),
                                                     static_cast<float>(hi)));
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
        // Limits must be parsed before boost_pct — boost_pct is clamped to them.
        d.min_limit          = ReadFloatClamped(j, "min_limit",          d.min_limit,
                                                HookLogic::kLowerLimitMin, HookLogic::kLowerLimitMax);
        d.max_limit          = ReadFloatClamped(j, "max_limit",          d.max_limit,
                                                HookLogic::kUpperLimitMin, HookLogic::kUpperLimitMax);
        d.boost_pct          = ReadFloatClamped(j, "boost_pct",          d.boost_pct,
                                                d.min_limit, d.max_limit);
        d.suppress_in_combat = ReadBool(j,         "suppress_in_combat", d.suppress_in_combat);
        d.boost_up_keycode   = ReadUInt32Clamped(j, "boost_up_keycode",   d.boost_up_keycode,   0, HookLogic::kKeycodeMax);
        d.boost_up_mods      = ReadUInt8Clamped(j,  "boost_up_mods",      d.boost_up_mods,      0, 0x07);
        d.boost_down_keycode = ReadUInt32Clamped(j, "boost_down_keycode", d.boost_down_keycode, 0, HookLogic::kKeycodeMax);
        d.boost_down_mods    = ReadUInt8Clamped(j,  "boost_down_mods",    d.boost_down_mods,    0, 0x07);
        d.reset_keycode      = ReadUInt32Clamped(j, "reset_keycode",      d.reset_keycode,      0, HookLogic::kKeycodeMax);
        d.reset_mods         = ReadUInt8Clamped(j,  "reset_mods",         d.reset_mods,         0, 0x07);
        d.show_indicator     = ReadBool(j,          "show_indicator",     d.show_indicator);
        d.indicator_position = ReadInt32Clamped(j,  "indicator_position", d.indicator_position, 0,     3);
        d.indicator_offset_x = ReadInt32Clamped(j,  "indicator_offset_x", d.indicator_offset_x, -2000, 2000);
        d.indicator_offset_y = ReadInt32Clamped(j,  "indicator_offset_y", d.indicator_offset_y, -2000, 2000);
        d.indicator_scale    = ReadFloatClamped(j,  "indicator_scale",    d.indicator_scale,
                                                HookLogic::kIndicatorScaleMin, HookLogic::kIndicatorScaleMax);
        return d;
    }

    std::string DocumentToJsonString(const ConfigDocument& d) {
        nlohmann::json j = {
            { "enabled",            d.enabled },
            { "boost_pct",          d.boost_pct },
            { "min_limit",          d.min_limit },
            { "max_limit",          d.max_limit },
            { "suppress_in_combat", d.suppress_in_combat },
            { "boost_up_keycode",   d.boost_up_keycode },
            { "boost_up_mods",      d.boost_up_mods },
            { "boost_down_keycode", d.boost_down_keycode },
            { "boost_down_mods",    d.boost_down_mods },
            { "reset_keycode",      d.reset_keycode },
            { "reset_mods",         d.reset_mods },
            { "show_indicator",     d.show_indicator },
            { "indicator_position", d.indicator_position },
            { "indicator_offset_x", d.indicator_offset_x },
            { "indicator_offset_y", d.indicator_offset_y },
            { "indicator_scale",    d.indicator_scale },
        };
        return j.dump(/*indent*/ 2);
    }

}
