#include "PCH.h"

#include "WalkSpeedHook.h"

#include <atomic>
#include <chrono>

#include <spdlog/spdlog.h>

#include "HookLogic.h"

// Cache-defeat tickle and graph-variable sync patterns adapted from
// DanjelPiDev/TES5-DynamicSpeedController (Apache-2.0).

namespace WalkSpeedTuner::WalkSpeedHook {

    namespace {

        std::atomic<bool>    g_enabled{ true };
        std::atomic<float>   g_boost_pct{ 0.0f };
        std::atomic<bool>    g_suppress_in_combat{ true };
        std::atomic<bool>    g_sync_animation{ false };
        std::atomic<bool>    g_first_boost_logged{ false };
        // ns-since-steady-epoch. std::atomic<steady_clock::time_point> isn't
        // guaranteed lock-free on MSVC; int64 is.
        std::atomic<std::int64_t> g_last_tickle_ns{ 0 };

        bool g_installed = false;

        using GetActorValue_t = float (*)(RE::ActorValueOwner*, RE::ActorValue);
        REL::Relocation<GetActorValue_t> g_orig_GetActorValue;

        using Update_t = void (*)(RE::Actor*, float);
        REL::Relocation<Update_t> g_orig_Update;

        inline std::int64_t NowNs() {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        // Snapshot live state into a BoostInputs for the pure-function gate.
        // `base` is filled by the caller (only the read hook needs it).
        HookLogic::BoostInputs SnapshotInputs(RE::Actor* actor) {
            auto* st = actor ? actor->AsActorState() : nullptr;
            return HookLogic::BoostInputs{
                /*base*/                0.0f,
                g_boost_pct.load(std::memory_order_relaxed),
                g_enabled.load(std::memory_order_relaxed),
                st && st->IsWalking(),
                actor && actor->IsInCombat(),
                g_suppress_in_combat.load(std::memory_order_relaxed),
            };
        }

        float HookGetActorValue(RE::ActorValueOwner* self, RE::ActorValue av) {
            // Tail-call form for the 99% non-SpeedMult path — lets the
            // compiler emit a jmp instead of call+ret.
            if (av != RE::ActorValue::kSpeedMult) return g_orig_GetActorValue(self, av);

            const float base = g_orig_GetActorValue(self, av);

            if (!g_enabled.load(std::memory_order_relaxed)) return base;
            if (g_boost_pct.load(std::memory_order_relaxed) <= 0.0f) return base;

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) return base;

            auto in = SnapshotInputs(player);
            in.base = base;
            const float boosted = HookLogic::ComputeBoosted(in);

            if (boosted != base &&
                !g_first_boost_logged.exchange(true, std::memory_order_relaxed)) {
                spdlog::info("[Hook] first boost: base={:.1f} +{:.1f} = {:.1f}",
                             base, in.boost_pct, boosted);
            }
            return boosted;
        }

        // Animation graph variable names — pack-dependent. DSC's multi-name
        // fallback covers vanilla + most major packs. We probe all four on the
        // first call and remember which one stuck; subsequent calls write only
        // the winning name (4× → 1× per frame).
        const RE::BSFixedString kAnimVarNames[] = {
            RE::BSFixedString{ "fAnimSpeedMult" },
            RE::BSFixedString{ "AnimSpeedMult" },
            RE::BSFixedString{ "AnimSpeed" },
            RE::BSFixedString{ "SpeedMult" },
        };
        constexpr int kAnimVarCount = sizeof(kAnimVarNames) / sizeof(kAnimVarNames[0]);

        // -2 = nothing worked, give up. -1 = haven't probed yet. ≥0 = winner.
        // Main-thread only (driven by HookUpdate which is the player's
        // per-frame Update vfunc), so plain bool/int — no atomics needed.
        int   s_winning_idx = -1;
        float s_last_written_rate = -1.0f;

        void WriteAnimRate(RE::Actor* actor, float scale) {
            if (scale == s_last_written_rate) return;            // no-op guard

            if (s_winning_idx >= 0) {
                actor->SetGraphVariableFloat(kAnimVarNames[s_winning_idx], scale);
                s_last_written_rate = scale;
                return;
            }
            if (s_winning_idx == -2) return;                     // gave up

            for (int i = 0; i < kAnimVarCount; ++i) {
                if (actor->SetGraphVariableFloat(kAnimVarNames[i], scale)) {
                    s_winning_idx = i;
                    s_last_written_rate = scale;
                    spdlog::info("[Hook] anim-graph variable '{}' accepted writes",
                                 kAnimVarNames[i].c_str());
                    return;
                }
            }
            s_winning_idx = -2;
            spdlog::warn("[Hook] no known anim-graph variable accepted writes; "
                         "skating may be visible");
        }

        void HookUpdate(RE::Actor* self, float delta) {
            g_orig_Update(self, delta);

            if (!g_sync_animation.load(std::memory_order_relaxed)) return;

            // Main-thread only — Actor::Update for PlayerCharacter is single-
            // threaded. Plain bool, no atomic.
            static bool s_was_active = false;

            // Fast bail before snapshotting expensive fields (IsWalking,
            // IsInCombat are vfunc/bit reads but still cost more than atomics).
            const bool  enabled = g_enabled.load(std::memory_order_relaxed);
            const float pct     = g_boost_pct.load(std::memory_order_relaxed);
            if (!enabled || pct <= 0.0f) {
                if (s_was_active) {
                    WriteAnimRate(self, 1.0f);
                    s_was_active = false;
                }
                return;
            }

            auto in = SnapshotInputs(self);
            const bool active = HookLogic::IsBoostActive(in);

            if (active) {
                WriteAnimRate(self, 1.0f + pct / 100.0f);
                s_was_active = true;
            } else if (s_was_active) {
                WriteAnimRate(self, 1.0f);
                s_was_active = false;
            }
        }

        void ForceSpeedRefresh(RE::Actor* actor) {
            if (!actor) return;
            const auto now  = NowNs();
            const auto last = g_last_tickle_ns.load(std::memory_order_relaxed);
            constexpr std::int64_t kThrottleNs = 25 * 1'000'000;  // 25 ms — DSC's value
            if (!HookLogic::ShouldTickle(now, last, kThrottleNs)) return;
            g_last_tickle_ns.store(now, std::memory_order_relaxed);

            if (auto* avo = actor->AsActorValueOwner()) {
                constexpr float eps = 0.10f;  // DSC kRefreshEps
                avo->ModActorValue(RE::ActorValue::kCarryWeight, +eps);
                avo->ModActorValue(RE::ActorValue::kCarryWeight, -eps);
            }
        }

    }

    bool Install() {
        if (g_installed) return true;

        REL::Relocation<std::uintptr_t> avoVtbl{ RE::VTABLE_PlayerCharacter[5] };
        g_orig_GetActorValue = avoVtbl.write_vfunc(0x01, &HookGetActorValue);

        REL::Relocation<std::uintptr_t> actorVtbl{ RE::VTABLE_PlayerCharacter[0] };
        g_orig_Update = actorVtbl.write_vfunc(0x0AD, &HookUpdate);

        g_installed = true;
        spdlog::info("[Hook] installed: GetActorValue@VTABLE[5][0x01]=0x{:X}, Update@VTABLE[0][0xAD]=0x{:X}",
                     reinterpret_cast<std::uintptr_t>(g_orig_GetActorValue.get()),
                     reinterpret_cast<std::uintptr_t>(g_orig_Update.get()));
        return true;
    }

    void SetBoostPercent(float pct) {
        const float clamped = std::clamp(pct, 0.0f, HookLogic::kMaxBoostPct);
        const float prev = g_boost_pct.exchange(clamped, std::memory_order_relaxed);
        if (std::abs(prev - clamped) <= 0.001f) return;
        spdlog::info("[Hook] boost {:.1f} -> {:.1f}; tickling", prev, clamped);
        if (auto* task = SKSE::GetTaskInterface()) {
            task->AddTask([] {
                if (auto* p = RE::PlayerCharacter::GetSingleton()) ForceSpeedRefresh(p);
            });
        }
    }

    void SetSuppressInCombat(bool suppress) {
        const bool prev = g_suppress_in_combat.exchange(suppress, std::memory_order_relaxed);
        if (prev != suppress) {
            spdlog::info("[Hook] suppress_in_combat {} -> {}", prev, suppress);
        }
    }

    void SetEnabled(bool enabled) {
        const bool prev = g_enabled.exchange(enabled, std::memory_order_relaxed);
        if (prev != enabled) {
            spdlog::info("[Hook] enabled {} -> {}; tickling", prev, enabled);
            TickleNow();
        }
    }

    void SetSyncAnimation(bool sync) {
        const bool prev = g_sync_animation.exchange(sync, std::memory_order_relaxed);
        if (prev != sync) {
            spdlog::info("[Hook] sync_animation {} -> {}", prev, sync);
        }
    }

    void TickleNow() {
        auto fn = [] {
            if (auto* p = RE::PlayerCharacter::GetSingleton()) ForceSpeedRefresh(p);
        };
        if (auto* task = SKSE::GetTaskInterface()) task->AddTask(fn);
        else fn();
    }

    bool  GetEnabled()           { return g_enabled.load(std::memory_order_relaxed); }
    float GetBoostPercent()      { return g_boost_pct.load(std::memory_order_relaxed); }
    bool  GetSuppressInCombat()  { return g_suppress_in_combat.load(std::memory_order_relaxed); }
    bool  GetSyncAnimation()     { return g_sync_animation.load(std::memory_order_relaxed); }

    bool IsBoostActiveRightNow() {
        auto* p = RE::PlayerCharacter::GetSingleton();
        if (!p) return false;
        return HookLogic::IsBoostActive(SnapshotInputs(p));
    }

    const char* InactiveReason() {
        if (!g_enabled.load(std::memory_order_relaxed)) return "disabled";
        if (g_boost_pct.load(std::memory_order_relaxed) <= 0.0f) return "boost = 0";
        auto* p = RE::PlayerCharacter::GetSingleton();
        if (!p) return "no player";
        auto* st = p->AsActorState();
        if (!st)              return "no actor state";
        if (!st->IsWalking()) return "not walking (press Caps Lock / Always Walk)";
        if (g_suppress_in_combat.load(std::memory_order_relaxed) && p->IsInCombat())
            return "in combat (suppression on)";
        return "active";
    }

}
