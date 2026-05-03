#pragma once

namespace WalkSpeedTuner::InputSuppress {

    // Installs vfunc hooks on ThirdPersonState and FirstPersonState
    // PlayerInputHandler::ProcessButton (vfunc 0x04). When the inbound
    // ButtonEvent matches a currently-bound mouse-wheel hotkey chord,
    // the original is skipped — vanilla camera zoom does not fire.
    // All other input passes through unchanged.
    bool Install();

}
