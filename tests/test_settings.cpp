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
    REQUIRE(d.sync_animation == false);
    REQUIRE(d.boost_up_keycode == 0u);
    REQUIRE(d.boost_up_mods == 0u);
    REQUIRE(d.boost_down_keycode == 0u);
    REQUIRE(d.boost_down_mods == 0u);
}

TEST_CASE("malformed JSON returns defaults", "[settings]") {
    auto d = ParseDocumentFromJson("not valid json {");
    REQUIRE(d.enabled == true);
    REQUIRE(d.boost_pct == 0.0f);
    REQUIRE(d.suppress_in_combat == true);
    REQUIRE(d.sync_animation == false);
}

TEST_CASE("empty string returns defaults", "[settings]") {
    auto d = ParseDocumentFromJson("");
    REQUIRE(d.boost_pct == 0.0f);
}

TEST_CASE("clamps boost_pct above max", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": 999.0})");
    REQUIRE(d.boost_pct == 75.0f);
}

TEST_CASE("accepts boost_pct exactly at max", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": 75.0})");
    REQUIRE(d.boost_pct == 75.0f);
}

TEST_CASE("clamps boost_pct below 0", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": -10.0})");
    REQUIRE(d.boost_pct == 0.0f);
}

TEST_CASE("non-numeric boost_pct ignored, default kept", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": "hello"})");
    REQUIRE(d.boost_pct == 0.0f);
}

TEST_CASE("integer 0/1 accepted as boolean", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"enabled": 0, "sync_animation": 1})");
    REQUIRE(d.enabled == false);
    REQUIRE(d.sync_animation == true);
}

TEST_CASE("partial JSON keeps defaults for missing fields", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_pct": 25.0})");
    REQUIRE(d.boost_pct == 25.0f);
    REQUIRE(d.enabled == true);
    REQUIRE(d.suppress_in_combat == true);
    REQUIRE(d.sync_animation == false);
}

TEST_CASE("round-trip preserves all fields", "[settings]") {
    ConfigDocument original{
        /*enabled*/ false,
        /*boost_pct*/ 35.0f,
        /*suppress_in_combat*/ false,
        /*sync_animation*/ true,
        /*boost_up_keycode*/ 0x1B,
        /*boost_up_mods*/ 0,
        /*boost_down_keycode*/ 0x1B,
        /*boost_down_mods*/ 4,
    };
    auto json   = DocumentToJsonString(original);
    auto parsed = ParseDocumentFromJson(json);
    REQUIRE(parsed.enabled == original.enabled);
    REQUIRE(parsed.boost_pct == original.boost_pct);
    REQUIRE(parsed.suppress_in_combat == original.suppress_in_combat);
    REQUIRE(parsed.sync_animation == original.sync_animation);
    REQUIRE(parsed.boost_up_keycode == original.boost_up_keycode);
    REQUIRE(parsed.boost_up_mods == original.boost_up_mods);
    REQUIRE(parsed.boost_down_keycode == original.boost_down_keycode);
    REQUIRE(parsed.boost_down_mods == original.boost_down_mods);
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

TEST_CASE("v1.1 keyboard JSON loads unchanged in v1.2", "[settings]") {
    // v1.1 JSONs that bound keyboard 0x1B (the ']' key) must still parse to 0x1B,
    // not be silently rebound to mouse-LMB or anything else.
    auto d = ParseDocumentFromJson(R"({"boost_up_keycode": 27})");
    REQUIRE(d.boost_up_keycode == 27u);
}

TEST_CASE("clamps mods above 0x07", "[settings]") {
    auto d = ParseDocumentFromJson(R"({"boost_up_mods": 99})");
    REQUIRE(d.boost_up_mods == 0x07u);
}

TEST_CASE("default chords are unbound", "[settings]") {
    auto d = ParseDocumentFromJson("{}");
    REQUIRE(d.boost_up_keycode == 0u);
    REQUIRE(d.boost_up_mods == 0u);
    REQUIRE(d.boost_down_keycode == 0u);
    REQUIRE(d.boost_down_mods == 0u);
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
    REQUIRE(parsed.sync_animation == d.sync_animation);
}

TEST_CASE("serialized output is non-empty and contains keys", "[settings]") {
    ConfigDocument d{ true, 30.0f, true, false };
    auto json = DocumentToJsonString(d);
    REQUIRE(!json.empty());
    REQUIRE(json.find("\"enabled\"") != std::string::npos);
    REQUIRE(json.find("\"boost_pct\"") != std::string::npos);
    REQUIRE(json.find("\"suppress_in_combat\"") != std::string::npos);
    REQUIRE(json.find("\"sync_animation\"") != std::string::npos);
}
