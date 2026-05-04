#include "PCH.h"

#include "MCM.h"

#include <spdlog/spdlog.h>

#include "SKSEMenuFramework.h"

#include "HookLogic.h"
#include "Hotkey.h"
#include "Settings.h"
#include "WalkSpeedHook.h"

using namespace ImGuiMCP;

namespace WalkSpeedTuner::MCM {

    namespace {

        Settings::ConfigDocument g_edit{};

        // Last-seen chord state, used to detect when the InputEvent callback
        // (capture mode) wrote a new binding directly into the Hotkey atomics.
        // MCM only triggers CommitEdit on widget changes by default; without
        // this watcher, captured bindings live only in memory and never reach
        // the JSON, so they're lost on game exit.
        bool          g_chord_watcher_init = false;
        std::uint32_t g_last_up_key    = 0;
        std::uint8_t  g_last_up_mods   = 0;
        std::uint32_t g_last_down_key  = 0;
        std::uint8_t  g_last_down_mods = 0;

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

        bool ConfirmModal(const char* popup_id, const char* title, const char* body) {
            bool confirmed = false;
            if (ImGui::BeginPopupModal(popup_id, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%s", title);
                ImGui::Spacing();
                ImGui::TextWrapped("%s", body);
                ImGui::Spacing();
                if (ImGui::Button("Confirm", ImVec2(120, 0))) {
                    confirmed = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            return confirmed;
        }

        void RenderHotkeyRow(const char* label, Hotkey::Target target,
                             std::uint32_t* edit_keycode, std::uint8_t* edit_mods) {
            ImGui::PushID(label);

            const auto capturing = Hotkey::CapturingTarget();
            const bool this_capturing = (capturing == target);

            const std::string display = this_capturing
                ? std::string("Press a key (or Ctrl/Alt/Shift + key)...")
                : Hotkey::ChordName(*edit_keycode, *edit_mods);

            ImGui::Text("%-8s", label);
            ImGui::SameLine(100.0f);
            ImGui::TextDisabled("%s", display.c_str());
            ImGui::SameLine(360.0f);

            if (ImGui::Button(this_capturing ? "Cancel" : "Set", ImVec2(70, 0))) {
                if (this_capturing) Hotkey::CancelCapture();
                else                Hotkey::BeginCapture(target);
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear", ImVec2(70, 0))) {
                *edit_keycode = 0;
                *edit_mods    = 0;
                CommitEdit();
            }

            ImGui::PopID();
        }

        void __stdcall RenderTab() {
            // boost_pct can change externally between MCM opens (via the
            // hotkey path). Sync from the Hook atomic each render so the
            // slider always reflects the current truth. No watcher needed —
            // hotkey already calls Settings::Persist(), so the JSON is fresh
            // whenever MCM opens.
            g_edit.boost_pct = WalkSpeedHook::GetBoostPercent();

            // The InputEvent callback writes captured chords directly to the
            // Hotkey atomics, not into g_edit. Re-sync each render so the
            // displayed chord updates immediately after a capture completes.
            const auto cur_up_k   = Hotkey::GetBoostUpKey();
            const auto cur_up_m   = Hotkey::GetBoostUpMods();
            const auto cur_down_k = Hotkey::GetBoostDownKey();
            const auto cur_down_m = Hotkey::GetBoostDownMods();

            g_edit.boost_up_keycode   = cur_up_k;
            g_edit.boost_up_mods      = cur_up_m;
            g_edit.boost_down_keycode = cur_down_k;
            g_edit.boost_down_mods    = cur_down_m;

            // Detect external chord change (capture-mode write from the
            // InputEvent callback). Force CommitEdit so the new binding hits
            // the JSON. Skip on the first render after Register so we don't
            // spuriously commit at startup.
            bool external_chord_change = false;
            if (g_chord_watcher_init) {
                external_chord_change =
                    cur_up_k   != g_last_up_key   || cur_up_m   != g_last_up_mods ||
                    cur_down_k != g_last_down_key || cur_down_m != g_last_down_mods;
            }
            g_last_up_key    = cur_up_k;
            g_last_up_mods   = cur_up_m;
            g_last_down_key  = cur_down_k;
            g_last_down_mods = cur_down_m;
            g_chord_watcher_init = true;

            if (external_chord_change) {
                spdlog::info("[MCM] hotkey chord changed externally — committing JSON");
            }

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
                "0%%-120%%. Changes apply within ~25 ms (the engine cache is "
                "tickled when you move the slider).");
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

            // ---- Hotkeys ----
            ImGui::SeparatorText("Hotkeys");
            ImGui::BeginDisabled(!g_edit.enabled);
            RenderHotkeyRow("Boost +", Hotkey::Target::kBoostUp,
                            &g_edit.boost_up_keycode, &g_edit.boost_up_mods);
            RenderHotkeyRow("Boost -", Hotkey::Target::kBoostDown,
                            &g_edit.boost_down_keycode, &g_edit.boost_down_mods);
            ImGui::EndDisabled();
            ImGui::TextDisabled(
                "Step: %.0f%%. Bind a key or Ctrl/Alt/Shift + key. Hotkeys "
                "are ignored while this menu is open. ESC during capture "
                "cancels.", HookLogic::kHotkeyStepPct);

            // ---- Danger zone ----
            ImGui::SeparatorText("Danger zone");
            if (ImGui::Button("Reset All to defaults", ImVec2(180, 0))) {
                ImGui::OpenPopup("Confirm##reset_all");
            }
            WrappedTooltip(
                "Restores all settings to defaults (boost = 0, suppress in "
                "combat = on, hotkeys reset to Alt+WheelUp / Alt+WheelDown) "
                "and rewrites the JSON config.\n\nNote: this mod never writes "
                "to your save — uninstalling the DLL leaves zero residue. "
                "The Reset button is convenience only.");
            if (ConfirmModal("Confirm##reset_all",
                             "Reset Walk Speed Tuner?",
                             "Restore all MCM settings to defaults and "
                             "rewrite the JSON config. Cannot be undone.")) {
                Settings::Save(Settings::ConfigDocument{});
                g_edit = Settings::CurrentDocument();
            }

            if (changed || external_chord_change) CommitEdit();
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
