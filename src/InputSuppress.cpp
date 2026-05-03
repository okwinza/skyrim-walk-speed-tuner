#include "PCH.h"

#include "InputSuppress.h"

#include <spdlog/spdlog.h>

#include "Hotkey.h"

// Surgical vfunc hook on PlayerInputHandler::ProcessButton (vfunc 0x04) of
// ThirdPersonState and FirstPersonState. When the inbound ButtonEvent
// matches a currently-bound mouse-wheel chord, we skip calling the original
// — vanilla camera zoom doesn't fire. Everything else delegates through.
//
// Same architectural pattern as TrueDirectionalMovement and SmoothCam
// (Approach 4 from the v1.3 research). Slot [1] in VTABLE_*PersonState is
// the PlayerInputHandler subobject vtable (slot [0] is TESCameraState);
// see RE/T/ThirdPersonState.h:14-17 + RE/F/FirstPersonState.h:11-14 for
// the multiple-inheritance layout.

namespace WalkSpeedTuner::InputSuppress {

    namespace {

        using ProcessButton_t = void (*)(void*, RE::ButtonEvent*, void*);

        REL::Relocation<ProcessButton_t> g_orig_TPS_ProcessButton;
        REL::Relocation<ProcessButton_t> g_orig_FPS_ProcessButton;
        bool g_installed = false;

        void HookTPSProcessButton(void* self, RE::ButtonEvent* ev, void* data) {
            if (Hotkey::ShouldSuppressForChord(ev)) {
                spdlog::debug("[InputSuppress] suppressed (3rd) for bound chord");
                return;
            }
            g_orig_TPS_ProcessButton(self, ev, data);
        }

        void HookFPSProcessButton(void* self, RE::ButtonEvent* ev, void* data) {
            if (Hotkey::ShouldSuppressForChord(ev)) {
                spdlog::debug("[InputSuppress] suppressed (1st) for bound chord");
                return;
            }
            g_orig_FPS_ProcessButton(self, ev, data);
        }

    }

    bool Install() {
        if (g_installed) return true;

        REL::Relocation<std::uintptr_t> tpsVtbl{ RE::VTABLE_ThirdPersonState[1] };
        g_orig_TPS_ProcessButton = tpsVtbl.write_vfunc(0x04, &HookTPSProcessButton);

        REL::Relocation<std::uintptr_t> fpsVtbl{ RE::VTABLE_FirstPersonState[1] };
        g_orig_FPS_ProcessButton = fpsVtbl.write_vfunc(0x04, &HookFPSProcessButton);

        g_installed = true;
        spdlog::info("[InputSuppress] installed: TPS+FPS PlayerInputHandler::ProcessButton@0x04");
        return true;
    }

}
