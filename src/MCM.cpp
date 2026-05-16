#include "PCH.h"

#include "MCM.h"

#include <algorithm>
#include <cmath>

#include <spdlog/spdlog.h>

#include "SKSEMenuFramework.h"

#include "HookLogic.h"
#include "Hotkey.h"
#include "Indicator.h"
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
        std::uint32_t g_last_up_key     = 0;
        std::uint8_t  g_last_up_mods    = 0;
        std::uint32_t g_last_down_key   = 0;
        std::uint8_t  g_last_down_mods  = 0;
        std::uint32_t g_last_reset_key  = 0;
        std::uint8_t  g_last_reset_mods = 0;

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
            const auto cur_up_k    = Hotkey::GetBoostUpKey();
            const auto cur_up_m    = Hotkey::GetBoostUpMods();
            const auto cur_down_k  = Hotkey::GetBoostDownKey();
            const auto cur_down_m  = Hotkey::GetBoostDownMods();
            const auto cur_reset_k = Hotkey::GetResetKey();
            const auto cur_reset_m = Hotkey::GetResetMods();

            g_edit.boost_up_keycode   = cur_up_k;
            g_edit.boost_up_mods      = cur_up_m;
            g_edit.boost_down_keycode = cur_down_k;
            g_edit.boost_down_mods    = cur_down_m;
            g_edit.reset_keycode      = cur_reset_k;
            g_edit.reset_mods         = cur_reset_m;

            // Detect external chord change (capture-mode write from the
            // InputEvent callback). Force CommitEdit so the new binding hits
            // the JSON. Skip on the first render after Register so we don't
            // spuriously commit at startup.
            bool external_chord_change = false;
            if (g_chord_watcher_init) {
                external_chord_change =
                    cur_up_k    != g_last_up_key    || cur_up_m    != g_last_up_mods   ||
                    cur_down_k  != g_last_down_key  || cur_down_m  != g_last_down_mods ||
                    cur_reset_k != g_last_reset_key || cur_reset_m != g_last_reset_mods;
            }
            g_last_up_key     = cur_up_k;
            g_last_up_mods    = cur_up_m;
            g_last_down_key   = cur_down_k;
            g_last_down_mods  = cur_down_m;
            g_last_reset_key  = cur_reset_k;
            g_last_reset_mods = cur_reset_m;
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
            if (ImGui::SliderFloat("Walk boost (%)", &g_edit.boost_pct,
                                   g_edit.min_limit, g_edit.max_limit,
                                   "%.0f%%")) {
                // Snap to a whole percent — the hotkey path already steps in
                // integers, so this keeps slider edits equally clean (the
                // indicator text, logs and JSON all stay integral).
                g_edit.boost_pct = std::round(g_edit.boost_pct);
                changed = true;
            }
            WrappedTooltip(
                "Percentage applied to your SpeedMult while you are in walk "
                "gait (toggled by Caps Lock or the 'Always Walk' key).\n\n"
                "Positive speeds you up, negative slows you down: +30% takes "
                "SpeedMult 100 to 130; -10% takes 100 to 90.\n\n"
                "0 disables the boost without disabling the mod. The Lower / "
                "Upper limit sliders below set how far it can go. Changes "
                "apply within ~25 ms (the engine cache is tickled when you "
                "move the slider).");

            if (ImGui::SliderFloat("Lower limit (%)", &g_edit.min_limit,
                                   HookLogic::kLowerLimitMin, HookLogic::kLowerLimitMax,
                                   "%.0f%%")) {
                g_edit.min_limit = std::round(g_edit.min_limit);
                changed = true;
            }
            WrappedTooltip("How far below normal speed you can set the boost.");

            if (ImGui::SliderFloat("Upper limit (%)", &g_edit.max_limit,
                                   HookLogic::kUpperLimitMin, HookLogic::kUpperLimitMax,
                                   "%.0f%%")) {
                g_edit.max_limit = std::round(g_edit.max_limit);
                changed = true;
            }
            WrappedTooltip("How far above normal speed you can set the boost.");

            // A tightened limit can leave boost_pct out of range — re-clamp the
            // edit buffer so the Boost slider doesn't show a stale value or fire
            // a spurious change next frame.
            g_edit.boost_pct = std::clamp(g_edit.boost_pct,
                                          g_edit.min_limit, g_edit.max_limit);
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
            RenderHotkeyRow("Reset", Hotkey::Target::kReset,
                            &g_edit.reset_keycode, &g_edit.reset_mods);
            ImGui::EndDisabled();
            ImGui::TextDisabled(
                "Step: %.0f%%. Bind a key or Ctrl/Alt/Shift + key. Hotkeys "
                "are ignored while this menu is open. ESC during capture "
                "cancels.", HookLogic::kHotkeyStepPct);

            // ---- Indicator ----
            ImGui::SeparatorText("Indicator");
            changed |= ImGui::Checkbox("Show speed indicator", &g_edit.show_indicator);
            WrappedTooltip(
                "Pops a small on-screen readout for ~2 seconds whenever you "
                "change the boost with the hotkey, then fades out. Needs "
                "SKSEMenuFramework. Off = no on-screen readout.");
            ImGui::BeginDisabled(!g_edit.show_indicator);
            {
                static const char* const kAnchorNames[] = {
                    "Bottom-right", "Bottom-center", "Top-center", "Top-left"
                };
                changed |= ImGui::Combo("Anchor", &g_edit.indicator_position,
                                        kAnchorNames, 4);
                WrappedTooltip("Screen corner the indicator is anchored to.");
                if (ImGui::SliderFloat("Size", &g_edit.indicator_scale,
                                       HookLogic::kIndicatorScaleMin,
                                       HookLogic::kIndicatorScaleMax, "%.1fx")) {
                    g_edit.indicator_scale =
                        std::round(g_edit.indicator_scale * 10.0f) / 10.0f;
                    changed = true;
                }
                WrappedTooltip("Readout text size, relative to the normal menu font.");
                changed |= ImGui::SliderInt("Nudge X", &g_edit.indicator_offset_x,
                                            -500, 500, "%d px");
                changed |= ImGui::SliderInt("Nudge Y", &g_edit.indicator_offset_y,
                                            -500, 500, "%d px");
                WrappedTooltip(
                    "Fine pixel offset from the anchored corner. Click Preview "
                    "to see exactly where the indicator lands.");
                if (ImGui::Button("Preview", ImVec2(120, 0))) {
                    Indicator::Ping();
                }
            }
            ImGui::EndDisabled();

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
