#include "PCH.h"

#include "WalkSpeedHook.h"

#include <atomic>

#include <spdlog/spdlog.h>

#include "HookLogic.h"

// Cache-defeat tickle pattern adapted from
// DanjelPiDev/TES5-DynamicSpeedController (Apache-2.0).

namespace WalkSpeedTuner::WalkSpeedHook {

    namespace {

        std::atomic<bool>    g_enabled{ true };
        std::atomic<float>   g_boost_pct{ 0.0f };
        std::atomic<bool>    g_suppress_in_combat{ true };
        std::atomic<float>   g_min_limit{ -20.0f };
        std::atomic<float>   g_max_limit{ 140.0f };
        std::atomic<bool>    g_first_boost_logged{ false };
        // ns-since-steady-epoch. std::atomic<steady_clock::time_point> isn't
        // guaranteed lock-free on MSVC; int64 is.
        std::atomic<std::int64_t> g_last_tickle_ns{ 0 };

        bool g_installed = false;

        using GetActorValue_t = float (*)(RE::ActorValueOwner*, RE::ActorValue);
        REL::Relocation<GetActorValue_t> g_orig_GetActorValue;

        float HookGetActorValue(RE::ActorValueOwner* self, RE::ActorValue av) {
            // Tail-call form for the 99% non-SpeedMult path — lets the
            // compiler emit a jmp instead of call+ret.
            if (av != RE::ActorValue::kSpeedMult) return g_orig_GetActorValue(self, av);

            const float base = g_orig_GetActorValue(self, av);

            // Two early-returns: from this point forward we know `enabled`
            // is true and `boost_pct != 0` (it may be negative — a walk-speed
            // reduction), so the inlined BoostInputs sets `enabled = true`
            // unconditionally (the remaining gate work in IsBoostActive is the
            // walking + combat checks).
            if (!g_enabled.load(std::memory_order_relaxed)) return base;
            if (g_boost_pct.load(std::memory_order_relaxed) == 0.0f) return base;

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) return base;

            auto* st = player->AsActorState();
            const HookLogic::BoostInputs in{
                base,
                g_boost_pct.load(std::memory_order_relaxed),
                true,                                   // already gated above
                st && st->IsWalking(),
                player->IsInCombat(),
                g_suppress_in_combat.load(std::memory_order_relaxed),
                st && st->IsSprinting(),
            };
            const float boosted = HookLogic::ComputeBoosted(in);

            if (boosted != base &&
                !g_first_boost_logged.exchange(true, std::memory_order_relaxed)) {
                spdlog::info("[Hook] first boost: base={:.1f} +{:.1f} = {:.1f}",
                             base, in.boost_pct, boosted);
            }
            return boosted;
        }

        void ForceSpeedRefresh(RE::Actor* actor) {
            if (!actor) return;
            const auto now  = HookLogic::NowNs();
            const auto last = g_last_tickle_ns.load(std::memory_order_relaxed);
            constexpr std::int64_t kThrottleNs = 25 * 1'000'000;  // 25 ms — DSC's value
            if (!HookLogic::ShouldTickle(now, last, kThrottleNs)) return;
            g_last_tickle_ns.store(now, std::memory_order_relaxed);

            if (auto* avo = actor->AsActorValueOwner()) {
                constexpr float eps = 0.10f;  // DSC kRefreshEps
                using Mod = RE::ACTOR_VALUE_MODIFIER;
                avo->ModActorValue(Mod::kDamage, RE::ActorValue::kCarryWeight, +eps);
                avo->ModActorValue(Mod::kDamage, RE::ActorValue::kCarryWeight, -eps);
            }
        }

    }

    bool Install() {
        if (g_installed) return true;

        REL::Relocation<std::uintptr_t> avoVtbl{ RE::VTABLE_PlayerCharacter[5] };
        g_orig_GetActorValue = avoVtbl.write_vfunc(0x01, &HookGetActorValue);

        g_installed = true;
        spdlog::info("[Hook] installed: GetActorValue@VTABLE[5][0x01]=0x{:X}",
                     reinterpret_cast<std::uintptr_t>(g_orig_GetActorValue.get()));
        return true;
    }

    void SetBoostPercent(float pct) {
        const float clamped = std::clamp(pct,
                                         g_min_limit.load(std::memory_order_relaxed),
                                         g_max_limit.load(std::memory_order_relaxed));
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

    void SetLimits(float lower, float upper) {
        const float prev_lo = g_min_limit.exchange(lower, std::memory_order_relaxed);
        const float prev_hi = g_max_limit.exchange(upper, std::memory_order_relaxed);
        if (std::abs(prev_lo - lower) > 0.001f || std::abs(prev_hi - upper) > 0.001f) {
            spdlog::info("[Hook] limits {:.0f}..{:.0f} -> {:.0f}..{:.0f}",
                         prev_lo, prev_hi, lower, upper);
        }
        // The caller (Settings::Apply) re-applies boost_pct right after, which
        // re-clamps it through SetBoostPercent against these new bounds.
    }

    void SetEnabled(bool enabled) {
        const bool prev = g_enabled.exchange(enabled, std::memory_order_relaxed);
        if (prev != enabled) {
            spdlog::info("[Hook] enabled {} -> {}; tickling", prev, enabled);
            TickleNow();
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
    float GetMinLimit()          { return g_min_limit.load(std::memory_order_relaxed); }
    float GetMaxLimit()          { return g_max_limit.load(std::memory_order_relaxed); }

}
