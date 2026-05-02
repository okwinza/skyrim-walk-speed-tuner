#include "PCH.h"

#include "MCM.h"

#include <spdlog/spdlog.h>

#include "SKSEMenuFramework.h"

#include "HookLogic.h"
#include "Settings.h"
#include "WalkSpeedHook.h"

using namespace ImGuiMCP;

namespace WalkSpeedTuner::MCM {

    namespace {

        Settings::ConfigDocument g_edit{};

        void WrappedTooltip(const char* text) {
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(450.0f);
                ImGui::TextUnformatted(text);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        void CommitEdit() {
            Settings::Save(g_edit);
        }

        void __stdcall RenderTab() {
            bool changed = false;

            // ---- Master enable ----
            ImGui::SeparatorText("Enable");
            changed |= ImGui::Checkbox("Walk-speed boost enabled", &g_edit.enabled);
            WrappedTooltip(
                "Master toggle. When off, the hook short-circuits and behaves "
                "exactly like vanilla Skyrim. Your slider value is preserved "
                "for when you re-enable it.");

            // ---- Boost slider ----
            ImGui::SeparatorText("Boost");
            ImGui::BeginDisabled(!g_edit.enabled);
            changed |= ImGui::SliderFloat("Walk boost (%)",
                                          &g_edit.boost_pct,
                                          0.0f, HookLogic::kMaxBoostPct, "%.0f%%");
            WrappedTooltip(
                "Additive percentage added to your SpeedMult while you are in "
                "walk gait (toggled by Caps Lock or the 'Always Walk' key).\n\n"
                "Example: at 30%, your SpeedMult goes from 100 to 130 while "
                "walking, making you walk 30%% faster.\n\n"
                "0 disables the boost without disabling the mod. Range: "
                "0%%-75%%. Changes apply within ~25 ms (the engine cache is "
                "tickled when you move the slider).");
            ImGui::SameLine();
            if (ImGui::SmallButton("Set 25%")) {
                g_edit.boost_pct = 25.0f;
                changed = true;
            }
            WrappedTooltip(
                "Quick preset: a comfortable middle-ground boost that makes "
                "walking feel purposeful without crossing into jog territory.");
            ImGui::EndDisabled();

            // ---- Suppression ----
            ImGui::SeparatorText("Suppression");
            ImGui::BeginDisabled(!g_edit.enabled);
            changed |= ImGui::Checkbox("Suppress during combat",
                                       &g_edit.suppress_in_combat);
            WrappedTooltip(
                "When on, the boost reverts to 0 while you are in combat — "
                "you walk at vanilla speed during fights. Default on for "
                "hardmode-friendly behavior. Combat state uses the engine's "
                "own flag.");
            ImGui::EndDisabled();

            // ---- Animation sync ----
            ImGui::SeparatorText("Animation");
            ImGui::BeginDisabled(!g_edit.enabled);
            changed |= ImGui::Checkbox("Sync animation rate to boost",
                                       &g_edit.sync_animation);
            WrappedTooltip(
                "When on, the walking animation plays faster in proportion "
                "to your boost so the legs visibly keep pace with the body — "
                "no skating. Costs one extra graph-variable write per frame "
                "while boost is active.\n\n"
                "Leave off if you don't see skating at your boost level "
                "(small boosts under 25%% usually look fine without this). "
                "Turn on if your feet visibly slide across the ground at "
                "higher boosts.");
            ImGui::EndDisabled();

            // ---- Live status ----
            ImGui::SeparatorText("Status");
            const bool active = WalkSpeedHook::IsBoostActiveRightNow();
            if (active) {
                ImGui::TextColored(ImVec4{ 0.30f, 0.85f, 0.30f, 1.0f },
                    "Boost active: +%.0f%% (walking)",
                    WalkSpeedHook::GetBoostPercent());
            } else {
                ImGui::TextColored(ImVec4{ 0.70f, 0.70f, 0.70f, 1.0f },
                    "Boost inactive: %s", WalkSpeedHook::InactiveReason());
            }

            // ---- About ----
            if (ImGui::CollapsingHeader("About")) {
                ImGui::Text("Walk Speed Tuner v1.0.0");
                ImGui::TextDisabled("Read hook: VTABLE_PlayerCharacter[5] slot 0x01");
                ImGui::TextDisabled("Update hook: VTABLE_PlayerCharacter[0] slot 0xAD");
                ImGui::TextDisabled("Log: Documents\\My Games\\Skyrim Special Edition"
                                    "\\SKSE\\WalkSpeedTuner.log");
                ImGui::TextDisabled("Config: Data\\SKSE\\Plugins\\WalkSpeedTuner.json");
                ImGui::Spacing();
                ImGui::TextWrapped(
                    "Modifies SpeedMult only while you are in walk gait. "
                    "Never writes any persistent actor value — fully save-"
                    "clean. If you uninstall the mod, your saves contain "
                    "zero residue.");
            }

            if (changed) CommitEdit();
        }

    }

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            spdlog::info("[MCM] SKSEMenuFramework not installed — JSON config remains canonical");
            return;
        }
        g_edit = Settings::CurrentDocument();
        SKSEMenuFramework::SetSection("Walk Speed Tuner");
        SKSEMenuFramework::AddSectionItem("Settings", &RenderTab);
        spdlog::info("[MCM] registered");
    }

}
