#include <catch2/catch_test_macros.hpp>

#include "Settings.h"

using WalkSpeedTuner::Settings::ConfigDocument;
using WalkSpeedTuner::Settings::DocumentToJsonString;
using WalkSpeedTuner::Settings::ParseDocumentFromJson;

TEST_CASE("missing JSON returns defaults", "[settings]") {
    auto d = ParseDocumentFromJson("{}");
    REQUIRE(d.enabled == true);
    REQUIRE(d.boost_pct == 0.0f);
    REQUIRE(d.suppress_in_combat == true);
    REQUIRE(d.boost_up_keycode == 0x108u);   // Alt+WheelUp
    REQUIRE(d.boost_up_mods == 0x02u);
    REQUIRE(d.boost_down_keycode == 0x109u); // Alt+WheelDown
    REQUIRE(d.boost_down_mods == 0x02u);
    REQUIRE(d.reset_keycode == 0x102u);      // Alt+MMB
    REQUIRE(d.reset_mods == 0x02u);
}

TEST_CASE("malformed JSON returns defaults", "[settings]") {
    auto d = ParseDocumentFromJson("not valid json {");
    REQUIRE(d.enabled == true);
    REQUIRE(d.boost_pct == 0.0f);
    REQUIRE(d.suppress_in_combat == true);
}

TEST_CASE("empty string returns defaults", "[settings]") {
    auto d = ParseDocumentFromJson("");
    REQUIRE(d.boost_pct == 0.0f);
}

TEST_CASE("clamps boost_pct above the default upper limit", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": 999.0})");
    REQUIRE(d.boost_pct == 140.0f);
}

TEST_CASE("accepts boost_pct exactly at the default upper limit", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": 140.0})");
    REQUIRE(d.boost_pct == 140.0f);
}

TEST_CASE("accepts negative boost_pct", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": -10.0})");
    REQUIRE(d.boost_pct == -10.0f);
}

TEST_CASE("clamps boost_pct below the default lower limit", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": -999.0})");
    REQUIRE(d.boost_pct == -20.0f);
}

TEST_CASE("non-numeric boost_pct ignored, default kept", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": "hello"})");
    REQUIRE(d.boost_pct == 0.0f);
}

TEST_CASE("integer 0 accepted as false bool", "[settings]") {
    // After v1.4 removed sync_animation, both remaining bool fields default
    // to true — so we can only meaningfully test the int-0 → false coercion
    // direction. Hits both bool reader paths.
    auto d = ParseDocumentFromJson(R"({"enabled": 0, "suppress_in_combat": 0})");
    REQUIRE(d.enabled == false);
    REQUIRE(d.suppress_in_combat == false);
}

TEST_CASE("partial JSON keeps defaults for missing fields", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": 25.0})");
    REQUIRE(d.boost_pct == 25.0f);
    REQUIRE(d.enabled == true);
    REQUIRE(d.suppress_in_combat == true);
}

TEST_CASE("round-trip preserves all fields", "[settings]") {
    ConfigDocument original{
        .enabled            = false,
        .boost_pct          = 35.0f,
        .min_limit          = -30.0f,
        .max_limit          = 120.0f,
        .suppress_in_combat = false,
        .boost_up_keycode   = 0x1B,
        .boost_up_mods      = 0,
        .boost_down_keycode = 0x1B,
        .boost_down_mods    = 4,
        .reset_keycode      = 0x108,
        .reset_mods         = 1,
        .show_indicator     = false,
        .indicator_position = 2,
        .indicator_offset_x = -40,
        .indicator_offset_y = 75,
        .indicator_scale    = 2.5f,
    };
    auto json   = DocumentToJsonString(original);
    auto parsed = ParseDocumentFromJson(json);
    REQUIRE(parsed.enabled == original.enabled);
    REQUIRE(parsed.boost_pct == original.boost_pct);
    REQUIRE(parsed.min_limit == original.min_limit);
    REQUIRE(parsed.max_limit == original.max_limit);
    REQUIRE(parsed.suppress_in_combat == original.suppress_in_combat);
    REQUIRE(parsed.boost_up_keycode == original.boost_up_keycode);
    REQUIRE(parsed.boost_up_mods == original.boost_up_mods);
    REQUIRE(parsed.boost_down_keycode == original.boost_down_keycode);
    REQUIRE(parsed.boost_down_mods == original.boost_down_mods);
    REQUIRE(parsed.reset_keycode == original.reset_keycode);
    REQUIRE(parsed.reset_mods == original.reset_mods);
    REQUIRE(parsed.show_indicator == original.show_indicator);
    REQUIRE(parsed.indicator_position == original.indicator_position);
    REQUIRE(parsed.indicator_offset_x == original.indicator_offset_x);
    REQUIRE(parsed.indicator_offset_y == original.indicator_offset_y);
    REQUIRE(parsed.indicator_scale == original.indicator_scale);
}

TEST_CASE("clamps keycode above 0x1FF", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_up_keycode": 99999})");
    REQUIRE(d.boost_up_keycode == 0x1FFu);
}

TEST_CASE("accepts mouse-encoded keycode (Ctrl+WheelUp)", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_up_keycode": 264, "boost_up_mods": 1})");
    REQUIRE(d.boost_up_keycode == 0x108u);
    REQUIRE(d.boost_up_mods == 0x01u);
}

TEST_CASE("v1.1 keyboard JSON loads unchanged in v1.2+", "[settings]") {
    // v1.1 JSONs that bound keyboard 0x1B (the ']' key) must still parse to 0x1B,
    // not be silently rebound to mouse-LMB or anything else.
    auto d = ParseDocumentFromJson(R"({"boost_up_keycode": 27})");
    REQUIRE(d.boost_up_keycode == 27u);
}

TEST_CASE("clamps mods above 0x07", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_up_mods": 99})");
    REQUIRE(d.boost_up_mods == 0x07u);
}

TEST_CASE("default chords are Alt+Wheel", "[settings]") {
    auto d = ParseDocumentFromJson("{}");
    REQUIRE(d.boost_up_keycode == 0x108u);
    REQUIRE(d.boost_up_mods == 0x02u);
    REQUIRE(d.boost_down_keycode == 0x109u);
    REQUIRE(d.boost_down_mods == 0x02u);
}

TEST_CASE("negative keycode clamps to zero", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_up_keycode": -5})");
    REQUIRE(d.boost_up_keycode == 0u);
}

TEST_CASE("default doc round-trips cleanly", "[settings]") {
    ConfigDocument d{};
    auto parsed = ParseDocumentFromJson(DocumentToJsonString(d));
    REQUIRE(parsed.enabled == d.enabled);
    REQUIRE(parsed.boost_pct == d.boost_pct);
    REQUIRE(parsed.suppress_in_combat == d.suppress_in_combat);
    REQUIRE(parsed.boost_up_keycode == d.boost_up_keycode);
    REQUIRE(parsed.boost_up_mods == d.boost_up_mods);
}

TEST_CASE("serialized output is non-empty and contains keys", "[settings]") {
    ConfigDocument d{};
    auto json = DocumentToJsonString(d);
    REQUIRE(!json.empty());
    REQUIRE(json.find("\"enabled\"") != std::string::npos);
    REQUIRE(json.find("\"boost_pct\"") != std::string::npos);
    REQUIRE(json.find("\"suppress_in_combat\"") != std::string::npos);
    REQUIRE(json.find("\"boost_up_keycode\"") != std::string::npos);
    REQUIRE(json.find("\"boost_down_keycode\"") != std::string::npos);
}

TEST_CASE("legacy sync_animation field silently ignored", "[settings]") {
    // Pre-release JSONs from v1.0–v1.3 carried `sync_animation`. v1.4 removed
    // the field; the parser must not error out — unknown keys are dropped.
    auto d = ParseDocumentFromJson(R"({"boost_pct": 20.0, "sync_animation": true})");
    REQUIRE(d.boost_pct == 20.0f);
    // (no assertion on sync_animation — the field doesn't exist anymore)
}

TEST_CASE("indicator fields default correctly", "[settings]") {
    auto d = ParseDocumentFromJson("{}");
    REQUIRE(d.show_indicator == true);
    REQUIRE(d.indicator_position == 0);   // bottom-right
    REQUIRE(d.indicator_offset_x == 0);
    REQUIRE(d.indicator_offset_y == 0);
}

TEST_CASE("clamps indicator_position out of range", "[settings]") {
    REQUIRE(ParseDocumentFromJson(R"({"indicator_position": 9})").indicator_position == 3);
    REQUIRE(ParseDocumentFromJson(R"({"indicator_position": -1})").indicator_position == 0);
}

TEST_CASE("indicator offsets accept negatives and clamp extremes", "[settings]") {
    // The signed reader must keep negatives intact, not floor them to 0.
    REQUIRE(ParseDocumentFromJson(R"({"indicator_offset_x": -120})").indicator_offset_x == -120);
    REQUIRE(ParseDocumentFromJson(R"({"indicator_offset_y": 99999})").indicator_offset_y == 2000);
    REQUIRE(ParseDocumentFromJson(R"({"indicator_offset_x": -99999})").indicator_offset_x == -2000);
}

TEST_CASE("v1.4 JSON without indicator keys loads with indicator defaults", "[settings]") {
    // A config written by v1.4 has no indicator_* keys; they must fall back to
    // defaults rather than tripping the parser.
    auto d = ParseDocumentFromJson(R"({"enabled": true, "boost_pct": 25.0})");
    REQUIRE(d.boost_pct == 25.0f);
    REQUIRE(d.show_indicator == true);
    REQUIRE(d.indicator_position == 0);
}

TEST_CASE("limit fields default correctly", "[settings]") {
    auto d = ParseDocumentFromJson("{}");
    REQUIRE(d.min_limit == -20.0f);
    REQUIRE(d.max_limit == 140.0f);
}

TEST_CASE("clamps min_limit to its slider range", "[settings]") {
    REQUIRE(ParseDocumentFromJson(R"({"min_limit": -999.0})").min_limit == -50.0f);
    REQUIRE(ParseDocumentFromJson(R"({"min_limit": 99.0})").min_limit == 0.0f);
    REQUIRE(ParseDocumentFromJson(R"({"min_limit": -35.0})").min_limit == -35.0f);
}

TEST_CASE("clamps max_limit to its slider range", "[settings]") {
    REQUIRE(ParseDocumentFromJson(R"({"max_limit": 999.0})").max_limit == 150.0f);
    REQUIRE(ParseDocumentFromJson(R"({"max_limit": -9.0})").max_limit == 0.0f);
    REQUIRE(ParseDocumentFromJson(R"({"max_limit": 75.0})").max_limit == 75.0f);
}

TEST_CASE("boost_pct is clamped to the parsed limits", "[settings]") {
    // Limits are parsed first, then boost_pct is clamped to them.
    auto hi = ParseDocumentFromJson(R"({"max_limit": 50.0, "boost_pct": 200.0})");
    REQUIRE(hi.boost_pct == 50.0f);
    auto lo = ParseDocumentFromJson(R"({"min_limit": -5.0, "boost_pct": -30.0})");
    REQUIRE(lo.boost_pct == -5.0f);
}

TEST_CASE("indicator_scale defaults and clamps to its slider range", "[settings]") {
    REQUIRE(ParseDocumentFromJson("{}").indicator_scale == 1.6f);
    REQUIRE(ParseDocumentFromJson(R"({"indicator_scale": 9.0})").indicator_scale == 3.0f);
    REQUIRE(ParseDocumentFromJson(R"({"indicator_scale": 0.2})").indicator_scale == 1.0f);
    REQUIRE(ParseDocumentFromJson(R"({"indicator_scale": 2.0})").indicator_scale == 2.0f);
}
