#include "PCH.h"

#include "Indicator.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>

#include <spdlog/spdlog.h>

#include "SKSEMenuFramework.h"

#include "HookLogic.h"
#include "WalkSpeedHook.h"

using namespace ImGuiMCP;

namespace WalkSpeedTuner::Indicator {

    namespace {

        std::atomic<bool>  g_enabled{ true };
        std::atomic<int>   g_position{ 0 };
        std::atomic<int>   g_off_x{ 0 };
        std::atomic<int>   g_off_y{ 0 };
        std::atomic<float> g_scale{ 1.6f };
        // ns-since-steady-epoch of the last Ping(). std::atomic<time_point>
        // isn't guaranteed lock-free on MSVC; int64 is.
        std::atomic<std::int64_t> g_last_ping_ns{ 0 };

        bool g_installed = false;

        // Indicator look: a single signed percentage, no panel, with a soft
        // drop-shadow so it stays legible on any background. Text size is the
        // runtime g_scale setting; the shadow offset stays fixed.
        constexpr float kShadowOffsetPx = 2.0f;

        // Per-frame HUD callback (SKSEMenuFramework render thread), invoked
        // inside an already-active ImGui frame. The idle-frame path must stay
        // cheap — it runs every frame whether or not the indicator is showing.
        void __stdcall RenderHud() {
            if (!g_enabled.load(std::memory_order_relaxed)) return;
            if (!WalkSpeedHook::GetEnabled()) return;

            const std::int64_t elapsed =
                HookLogic::NowNs() - g_last_ping_ns.load(std::memory_order_relaxed);
            const float alpha = HookLogic::IndicatorAlpha(elapsed);
            if (alpha <= 0.0f) return;  // not in a fade window — skip drawing

            auto* vp   = ImGui::GetMainViewport();
            auto* dl   = ImGui::GetForegroundDrawList();
            auto* font = ImGui::GetFont();
            if (!vp || !dl || !font) return;

            const float boost = WalkSpeedHook::GetBoostPercent();
            char text[16];
            std::snprintf(text, sizeof(text), "%+.0f%%", boost);

            // Text size is the user's scale setting times the base UI font.
            // Glyph metrics scale linearly, so the scaled extent is just
            // CalcTextSize() * scale.
            const float scale    = g_scale.load(std::memory_order_relaxed);
            const float fontSize = ImGui::GetFontSize() * scale;
            ImVec2 baseSize;
            ImGui::CalcTextSize(&baseSize, text, nullptr, false, -1.0f);
            const ImVec2 size{ baseSize.x * scale, baseSize.y * scale };

            // Anchor + nudge from the unit-tested pure helper; apply the pivot
            // to the scaled text size to get the top-left draw point.
            const auto p = HookLogic::IndicatorPos(
                g_position.load(std::memory_order_relaxed),
                g_off_x.load(std::memory_order_relaxed),
                g_off_y.load(std::memory_order_relaxed),
                vp->WorkPos.x, vp->WorkPos.y, vp->WorkSize.x, vp->WorkSize.y);
            const float x = p.x - size.x * p.pivot_x;
            const float y = p.y - size.y * p.pivot_y;

            // Subtle, desaturated tint — never neon. The sign in the text
            // already conveys direction; colour is a calm reinforcement.
            const int   a      = static_cast<int>(alpha * 255.0f);
            const ImU32 main   = boost > 0.0f ? IM_COL32(150, 200, 140, a)   // faster
                               : boost < 0.0f ? IM_COL32(224, 178, 110, a)   // slower
                                              : IM_COL32(220, 218, 210, a);  // vanilla
            const ImU32 shadow = IM_COL32(0, 0, 0, static_cast<int>(alpha * 168.0f));

            ImGui::ImDrawListManager::AddText(dl, font, fontSize,
                ImVec2{ x + kShadowOffsetPx, y + kShadowOffsetPx }, shadow, text);
            ImGui::ImDrawListManager::AddText(dl, font, fontSize,
                ImVec2{ x, y }, main, text);
        }

    }

    void Install() {
        if (g_installed) return;
        if (!SKSEMenuFramework::IsInstalled()) {
            spdlog::info("[Indicator] SKSEMenuFramework not installed — HUD indicator unavailable");
            return;
        }
        SKSEMenuFramework::AddHudElement(&RenderHud);
        g_installed = true;
        spdlog::info("[Indicator] installed");
    }

    void Ping() {
        g_last_ping_ns.store(HookLogic::NowNs(), std::memory_order_relaxed);
    }

    void SetEnabled(bool enabled) {
        g_enabled.store(enabled, std::memory_order_relaxed);
    }

    void SetPosition(int corner) {
        g_position.store(std::clamp(corner, 0, 3), std::memory_order_relaxed);
    }

    void SetOffset(int off_x, int off_y) {
        g_off_x.store(off_x, std::memory_order_relaxed);
        g_off_y.store(off_y, std::memory_order_relaxed);
    }

    void SetScale(float scale) {
        g_scale.store(std::clamp(scale, HookLogic::kIndicatorScaleMin,
                                 HookLogic::kIndicatorScaleMax),
                      std::memory_order_relaxed);
    }

    bool  GetEnabled()  { return g_enabled.load(std::memory_order_relaxed); }
    int   GetPosition() { return g_position.load(std::memory_order_relaxed); }
    int   GetOffsetX()  { return g_off_x.load(std::memory_order_relaxed); }
    int   GetOffsetY()  { return g_off_y.load(std::memory_order_relaxed); }
    float GetScale()    { return g_scale.load(std::memory_order_relaxed); }

}
