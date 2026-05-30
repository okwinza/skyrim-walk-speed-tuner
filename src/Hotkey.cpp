#include "PCH.h"

#include "Hotkey.h"

#include <atomic>
#include <cstdio>

#include <spdlog/spdlog.h>

#include "SKSEMenuFramework.h"

#include "HookLogic.h"
#include "Indicator.h"
#include "Settings.h"
#include "WalkSpeedHook.h"

namespace WalkSpeedTuner::Hotkey {

    namespace {

        std::atomic<std::uint32_t> g_up_key{ 0 };
        std::atomic<std::uint32_t> g_down_key{ 0 };
        std::atomic<std::uint32_t> g_reset_key{ 0 };
        std::atomic<std::uint8_t>  g_up_mods{ 0 };
        std::atomic<std::uint8_t>  g_down_mods{ 0 };
        std::atomic<std::uint8_t>  g_reset_mods{ 0 };

        std::atomic<bool> g_ctrl_down{ false };
        std::atomic<bool> g_alt_down{ false };
        std::atomic<bool> g_shift_down{ false };

        std::atomic<int> g_capturing_target{ static_cast<int>(Target::kNone) };

        // Diagnostic latches — fire once each, then go silent.
        std::atomic<bool> g_first_event_logged{ false };
        std::atomic<bool> g_first_action_logged{ false };

        // Debounce: the wheel callback path empirically fires twice per
        // physical wheel tick (0–2 ms apart, see v1.2.4 log lines 12–22),
        // so each "click" produces 2× the configured step. Real wheel ticks
        // are ≥16 ms apart; a 10 ms window cleanly separates them.
        std::atomic<std::int64_t> g_last_fire_ns{ 0 };
        inline constexpr std::int64_t kFireDebounceNs = 10'000'000;  // 10 ms

        SKSEMenuFramework::Model::InputEvent* g_handle = nullptr;

        // DX scan codes for modifiers (left/right both fold into the same logical mod).
        constexpr std::uint32_t kLCtrl  = 0x1D;
        constexpr std::uint32_t kRCtrl  = 0x9D;
        constexpr std::uint32_t kLShift = 0x2A;
        constexpr std::uint32_t kRShift = 0x36;
        constexpr std::uint32_t kLAlt   = 0x38;
        constexpr std::uint32_t kRAlt   = 0xB8;
        constexpr std::uint32_t kEsc    = 0x01;

        bool IsModifierScanCode(std::uint32_t code) {
            return code == kLCtrl || code == kRCtrl ||
                   code == kLShift || code == kRShift ||
                   code == kLAlt   || code == kRAlt;
        }

        std::uint8_t CurrentModMask() {
            std::uint8_t m = 0;
            if (g_ctrl_down.load(std::memory_order_relaxed))  m |= kModCtrl;
            if (g_alt_down.load(std::memory_order_relaxed))   m |= kModAlt;
            if (g_shift_down.load(std::memory_order_relaxed)) m |= kModShift;
            return m;
        }

        const char* KeyName(std::uint32_t code) {
            switch (code) {
                // Mouse buttons + wheel (extended encoding)
                case 0x100: return "LMB";
                case 0x101: return "RMB";
                case 0x102: return "MMB";
                case 0x103: return "Mouse4";
                case 0x104: return "Mouse5";
                case 0x105: return "Mouse6";
                case 0x106: return "Mouse7";
                case 0x107: return "Mouse8";
                case 0x108: return "WheelUp";
                case 0x109: return "WheelDown";

                case 0x01: return "Esc";
                case 0x02: return "1"; case 0x03: return "2"; case 0x04: return "3";
                case 0x05: return "4"; case 0x06: return "5"; case 0x07: return "6";
                case 0x08: return "7"; case 0x09: return "8"; case 0x0A: return "9";
                case 0x0B: return "0";
                case 0x0C: return "-"; case 0x0D: return "=";
                case 0x0E: return "Backspace"; case 0x0F: return "Tab";
                case 0x10: return "Q"; case 0x11: return "W"; case 0x12: return "E";
                case 0x13: return "R"; case 0x14: return "T"; case 0x15: return "Y";
                case 0x16: return "U"; case 0x17: return "I"; case 0x18: return "O";
                case 0x19: return "P";
                case 0x1A: return "["; case 0x1B: return "]";
                case 0x1C: return "Enter";
                case 0x1E: return "A"; case 0x1F: return "S"; case 0x20: return "D";
                case 0x21: return "F"; case 0x22: return "G"; case 0x23: return "H";
                case 0x24: return "J"; case 0x25: return "K"; case 0x26: return "L";
                case 0x27: return ";"; case 0x28: return "'"; case 0x29: return "`";
                case 0x2B: return "\\";
                case 0x2C: return "Z"; case 0x2D: return "X"; case 0x2E: return "C";
                case 0x2F: return "V"; case 0x30: return "B"; case 0x31: return "N";
                case 0x32: return "M";
                case 0x33: return ","; case 0x34: return "."; case 0x35: return "/";
                case 0x39: return "Space";
                case 0x3A: return "CapsLock";
                case 0x3B: return "F1"; case 0x3C: return "F2"; case 0x3D: return "F3";
                case 0x3E: return "F4"; case 0x3F: return "F5"; case 0x40: return "F6";
                case 0x41: return "F7"; case 0x42: return "F8"; case 0x43: return "F9";
                case 0x44: return "F10"; case 0x57: return "F11"; case 0x58: return "F12";
                case 0xC7: return "Home"; case 0xCF: return "End";
                case 0xC9: return "PgUp"; case 0xD1: return "PgDn";
                case 0xC8: return "Up";   case 0xD0: return "Down";
                case 0xCB: return "Left"; case 0xCD: return "Right";
                case 0xD2: return "Insert"; case 0xD3: return "Delete";
                default: return nullptr;
            }
        }

        // True when any Skyrim menu is on screen (inventory, magic, map,
        // console, dialogue, main menu, etc.). HUD-only gameplay returns
        // false. Used to short-circuit action-mode hotkey processing so
        // vanilla Alt+wheel can navigate menus without bumping the boost.
        // MCM capture mode runs above this gate (via continue) and is
        // unaffected.
        bool AnyGameMenuOpen() {
            const auto* ui = RE::UI::GetSingleton();
            return ui && ui->IsShowingMenus();
        }

        // Logs any pair of the three hotkeys bound to the same chord. The
        // matcher checks up, then down, then reset, so on a clash the
        // earlier-listed action wins and the later one goes dead.
        void LogChordCollisions() {
            struct Bind { std::uint32_t key; std::uint8_t mods; const char* label; };
            const Bind binds[] = {
                { g_up_key.load(std::memory_order_relaxed),
                  g_up_mods.load(std::memory_order_relaxed),    "boost_up" },
                { g_down_key.load(std::memory_order_relaxed),
                  g_down_mods.load(std::memory_order_relaxed),  "boost_down" },
                { g_reset_key.load(std::memory_order_relaxed),
                  g_reset_mods.load(std::memory_order_relaxed), "reset" },
            };
            for (int i = 0; i < 3; ++i) {
                for (int k = i + 1; k < 3; ++k) {
                    if (binds[i].key != 0 &&
                        binds[i].key == binds[k].key && binds[i].mods == binds[k].mods) {
                        spdlog::warn("[Hotkey] WARN {} and {} share chord '{}' — only {} fires",
                                     binds[i].label, binds[k].label,
                                     ChordName(binds[i].key, binds[i].mods), binds[i].label);
                    }
                }
            }
        }

        bool __stdcall Callback(RE::InputEvent* head) {
            if (!head) return false;

            // One-shot lifecycle confirmation: prove the callback is firing.
            if (!g_first_event_logged.exchange(true, std::memory_order_relaxed)) {
                spdlog::info("[Hotkey] callback received first event — input pipeline alive");
            }

            const bool mcm_open = SKSEMenuFramework::IsAnyBlockingWindowOpened();
            const auto up_k    = g_up_key.load(std::memory_order_relaxed);
            const auto down_k  = g_down_key.load(std::memory_order_relaxed);
            const auto reset_k = g_reset_key.load(std::memory_order_relaxed);
            const bool any_bound = (up_k != 0) || (down_k != 0) || (reset_k != 0);

            for (auto* ev = head; ev; ev = ev->next) {
                auto* btn = ev->AsButtonEvent();
                if (!btn) continue;
                const auto dev = btn->device.get();
                if (dev != RE::INPUT_DEVICE::kKeyboard && dev != RE::INPUT_DEVICE::kMouse) continue;

                const auto rawCode  = btn->GetIDCode();
                const auto encoded  = HookLogic::EncodeKeycode(static_cast<int>(dev), rawCode);
                const bool pressed  = btn->IsPressed();
                const bool downEdge = btn->IsDown();

                if (dev == RE::INPUT_DEVICE::kKeyboard) {
                    switch (rawCode) {
                        case kLCtrl: case kRCtrl:
                            g_ctrl_down.store(pressed, std::memory_order_relaxed);  break;
                        case kLAlt: case kRAlt:
                            g_alt_down.store(pressed, std::memory_order_relaxed);   break;
                        case kLShift: case kRShift:
                            g_shift_down.store(pressed, std::memory_order_relaxed); break;
                        default: break;
                    }
                }

                if (!downEdge) continue;
                if (dev == RE::INPUT_DEVICE::kKeyboard && IsModifierScanCode(rawCode)) continue;

                const std::uint8_t mods = CurrentModMask();

                if (mcm_open) {
                    const auto target = g_capturing_target.load(std::memory_order_relaxed);
                    if (target == static_cast<int>(Target::kNone)) continue;

                    // ESC during capture cancels (keyboard only).
                    if (dev == RE::INPUT_DEVICE::kKeyboard && rawCode == kEsc) {
                        g_capturing_target.store(static_cast<int>(Target::kNone),
                                                 std::memory_order_relaxed);
                        spdlog::info("[Hotkey] capture canceled (ESC)");
                        continue;
                    }

                    // LMB/RMB clicks during capture are MCM-interaction events
                    // (clicking buttons, sliders, etc.), not the user's "press
                    // a key to bind" gesture. Discard them.
                    if (encoded == HookLogic::kMouseBase + 0 ||
                        encoded == HookLogic::kMouseBase + 1) {
                        continue;
                    }

                    const char* tname =
                        target == static_cast<int>(Target::kBoostUp)   ? "boost_up"   :
                        target == static_cast<int>(Target::kBoostDown) ? "boost_down" : "reset";
                    spdlog::info("[Hotkey] capture completed: target={} encoded=0x{:X} mods=0x{:X}",
                                 tname, encoded, mods);
                    if (target == static_cast<int>(Target::kBoostUp)) {
                        SetBoostUpKey(encoded, mods);
                    } else if (target == static_cast<int>(Target::kBoostDown)) {
                        SetBoostDownKey(encoded, mods);
                    } else {
                        SetResetKey(encoded, mods);
                    }
                    g_capturing_target.store(static_cast<int>(Target::kNone),
                                             std::memory_order_relaxed);
                    continue;
                }

                // Vanilla menu (inventory, map, magic, system, console,
                // dialogue, main menu, etc.) is up — let the user navigate
                // it with their normal Alt+wheel without bumping the boost.
                if (AnyGameMenuOpen()) continue;

                if (!any_bound) continue;

                const HookLogic::HotkeyConfig cfg{
                    up_k,    g_up_mods.load(std::memory_order_relaxed),
                    down_k,  g_down_mods.load(std::memory_order_relaxed),
                    reset_k, g_reset_mods.load(std::memory_order_relaxed),
                };

                // One-shot: prove that action-mode events are reaching the
                // matcher (i.e. mcm_open isn't stuck-true and any_bound passed).
                if (!g_first_action_logged.exchange(true, std::memory_order_relaxed)) {
                    spdlog::info("[Hotkey] first action-mode event: encoded=0x{:X} mods=0x{:X} "
                                 "(bound up='{}', down='{}', reset='{}')",
                                 encoded, mods,
                                 ChordName(cfg.up_key,    cfg.up_mods),
                                 ChordName(cfg.down_key,  cfg.down_mods),
                                 ChordName(cfg.reset_key, cfg.reset_mods));
                }

                const auto match = HookLogic::MatchHotkey(encoded, mods, cfg);

                // Debounce gate: ignore matches that arrive within 10 ms of
                // the previous fire. Suppresses the wheel double-fire (2
                // events per physical tick at 0–2 ms apart) without losing
                // real ticks (≥16 ms apart per v1.2.4 log).
                if (match != HookLogic::HotkeyAction::kNone) {
                    const auto now  = HookLogic::NowNs();
                    const auto last = g_last_fire_ns.load(std::memory_order_relaxed);
                    if (now - last < kFireDebounceNs) {
                        spdlog::debug("[Hotkey] debounced (gap {} ns < {} ns)",
                                      now - last, kFireDebounceNs);
                        continue;
                    }
                    g_last_fire_ns.store(now, std::memory_order_relaxed);
                }

                // Every action ends the same way — set the boost, persist it,
                // pop the indicator. boost+/- commit a bumped value; reset
                // commits 0.
                const auto commit_boost = [&](float value, const char* label) {
                    spdlog::info("[Hotkey] {} fired (encoded=0x{:X} mods=0x{:X})",
                                 label, encoded, mods);
                    WalkSpeedHook::SetBoostPercent(value);
                    Settings::Persist();
                    Indicator::Ping();
                };
                const auto bumped = [](float step) {
                    return HookLogic::BumpBoost(WalkSpeedHook::GetBoostPercent(), step,
                                                WalkSpeedHook::GetMinLimit(),
                                                WalkSpeedHook::GetMaxLimit());
                };

                switch (match) {
                    case HookLogic::HotkeyAction::kBoostUp:
                        commit_boost(bumped(+HookLogic::kHotkeyStepPct), "boost+");
                        break;
                    case HookLogic::HotkeyAction::kBoostDown:
                        commit_boost(bumped(-HookLogic::kHotkeyStepPct), "boost-");
                        break;
                    case HookLogic::HotkeyAction::kReset:
                        commit_boost(0.0f, "reset");
                        break;
                    case HookLogic::HotkeyAction::kNone:
                        // Helpful diagnostic: key matches a bound chord but
                        // modifier mask doesn't. Tells the user "you bound
                        // Ctrl+J but you're pressing plain J" (or vice versa).
                        if ((encoded == cfg.up_key    && cfg.up_key    != 0) ||
                            (encoded == cfg.down_key  && cfg.down_key  != 0) ||
                            (encoded == cfg.reset_key && cfg.reset_key != 0)) {
                            spdlog::info("[Hotkey] key matches but modifier doesn't: "
                                         "encoded=0x{:X} mods=0x{:X} "
                                         "(need up='{}', down='{}' or reset='{}')",
                                         encoded, mods,
                                         ChordName(cfg.up_key,    cfg.up_mods),
                                         ChordName(cfg.down_key,  cfg.down_mods),
                                         ChordName(cfg.reset_key, cfg.reset_mods));
                        }
                        break;
                }
            }
            return false;
        }

    }

    void Install() {
        if (g_handle) return;
        g_handle = SKSEMenuFramework::AddInputEvent(&Callback);
        spdlog::info("[Hotkey] installed");
    }

    void Uninstall() {
        if (!g_handle) return;
        delete g_handle;
        g_handle = nullptr;
        spdlog::info("[Hotkey] uninstalled");
    }

    void BeginCapture(Target t) {
        g_capturing_target.store(static_cast<int>(t), std::memory_order_relaxed);
        spdlog::info("[Hotkey] capture started for {}",
                     t == Target::kBoostUp   ? "boost_up"   :
                     t == Target::kBoostDown ? "boost_down" :
                     t == Target::kReset     ? "reset"      : "none");
    }

    void CancelCapture() {
        const auto prev = g_capturing_target.exchange(
            static_cast<int>(Target::kNone), std::memory_order_relaxed);
        if (prev != static_cast<int>(Target::kNone)) {
            spdlog::info("[Hotkey] capture canceled");
        }
    }

    Target CapturingTarget() {
        return static_cast<Target>(g_capturing_target.load(std::memory_order_relaxed));
    }

    void SetBoostUpKey(std::uint32_t code, std::uint8_t mods) {
        const auto prev_k = g_up_key.exchange(code, std::memory_order_relaxed);
        const auto prev_m = g_up_mods.exchange(mods, std::memory_order_relaxed);
        if (prev_k == code && prev_m == mods) return;

        if (code == 0) {
            spdlog::info("[Hotkey] cleared boost_up");
        } else {
            spdlog::info("[Hotkey] bound boost_up to '{}' (key=0x{:X} mods=0x{:X})",
                         ChordName(code, mods), code, mods);
            LogChordCollisions();
        }
    }

    void SetBoostDownKey(std::uint32_t code, std::uint8_t mods) {
        const auto prev_k = g_down_key.exchange(code, std::memory_order_relaxed);
        const auto prev_m = g_down_mods.exchange(mods, std::memory_order_relaxed);
        if (prev_k == code && prev_m == mods) return;

        if (code == 0) {
            spdlog::info("[Hotkey] cleared boost_down");
        } else {
            spdlog::info("[Hotkey] bound boost_down to '{}' (key=0x{:X} mods=0x{:X})",
                         ChordName(code, mods), code, mods);
            LogChordCollisions();
        }
    }

    void SetResetKey(std::uint32_t code, std::uint8_t mods) {
        const auto prev_k = g_reset_key.exchange(code, std::memory_order_relaxed);
        const auto prev_m = g_reset_mods.exchange(mods, std::memory_order_relaxed);
        if (prev_k == code && prev_m == mods) return;

        if (code == 0) {
            spdlog::info("[Hotkey] cleared reset");
        } else {
            spdlog::info("[Hotkey] bound reset to '{}' (key=0x{:X} mods=0x{:X})",
                         ChordName(code, mods), code, mods);
            LogChordCollisions();
        }
    }

    std::uint32_t GetBoostUpKey()    { return g_up_key.load(std::memory_order_relaxed); }
    std::uint8_t  GetBoostUpMods()   { return g_up_mods.load(std::memory_order_relaxed); }
    std::uint32_t GetBoostDownKey()  { return g_down_key.load(std::memory_order_relaxed); }
    std::uint8_t  GetBoostDownMods() { return g_down_mods.load(std::memory_order_relaxed); }
    std::uint32_t GetResetKey()      { return g_reset_key.load(std::memory_order_relaxed); }
    std::uint8_t  GetResetMods()     { return g_reset_mods.load(std::memory_order_relaxed); }

    void ResetModifierState() {
        g_ctrl_down.store(false,  std::memory_order_relaxed);
        g_alt_down.store(false,   std::memory_order_relaxed);
        g_shift_down.store(false, std::memory_order_relaxed);
    }

    std::string ChordName(std::uint32_t code, std::uint8_t mods) {
        if (code == 0) return "<unbound>";
        std::string s;
        if (mods & kModCtrl)  s += "Ctrl+";
        if (mods & kModAlt)   s += "Alt+";
        if (mods & kModShift) s += "Shift+";
        if (const char* name = KeyName(code)) {
            s += name;
        } else {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "DX0x%X", code);
            s += buf;
        }
        return s;
    }

    bool ShouldSuppressForChord(const RE::ButtonEvent* ev) {
        if (!ev) return false;
        // Don't suppress while a menu is up — vanilla menu wheel-nav must
        // pass through. PlayerInputHandler::ProcessButton on camera states
        // is typically inactive in menu mode already, so this is mainly
        // an explicit contract for future hook expansion.
        if (AnyGameMenuOpen()) return false;
        // Mouse-only: keyboard keys never trigger vanilla camera, and
        // modifier-key chords (Ctrl/Alt/Shift) couldn't match a keyboard
        // key whose code IS the modifier (since the modifier is held).
        if (ev->device.get() != RE::INPUT_DEVICE::kMouse) return false;

        const auto encoded = HookLogic::EncodeKeycode(
            static_cast<int>(ev->device.get()), ev->GetIDCode());
        const std::uint8_t mods = CurrentModMask();

        const HookLogic::HotkeyConfig cfg{
            g_up_key.load(std::memory_order_relaxed),
            g_up_mods.load(std::memory_order_relaxed),
            g_down_key.load(std::memory_order_relaxed),
            g_down_mods.load(std::memory_order_relaxed),
            g_reset_key.load(std::memory_order_relaxed),
            g_reset_mods.load(std::memory_order_relaxed),
        };
        return HookLogic::MatchHotkey(encoded, mods, cfg) != HookLogic::HotkeyAction::kNone;
    }

}
