# Walk Speed Tuner

SKSE plugin for Skyrim SE/AE that lets the player walk faster without
touching anything else. Adds a configurable boost (0–110%) to your
`SpeedMult` while you are in walk gait. Sprint, run, sneak, and NPCs are
untouched.

## Architecture (the selling point)

The hook intercepts `ActorValueOwner::GetActorValue(kSpeedMult)` and returns
the boosted value when the player is walking. The underlying actor value is
**never modified**.

- Saves are byte-identical to vanilla
- Removing `WalkSpeedTuner.dll` returns the player to vanilla speed instantly
- No script-extender persistence, no co-save data, no orphan drift

A 25 ms-throttled `ModActorValue(kCarryWeight, ±0.10)` "tickle" defeats the
engine's lazy `MiddleHighProcessData` cache when the boost changes mid-game.
Pattern adapted from
[DanjelPiDev/TES5-DynamicSpeedController](https://github.com/DanjelPiDev/TES5-DynamicSpeedController)
(Apache-2.0).

## Features

- 0–110% boost on the walk gait (5% step via hotkey)
- Optional combat suppression (default on)
- Default hotkeys: **Alt + Mouse Wheel Up / Down** to bump boost ±5%
- Configurable bindings — keyboard or mouse, with Ctrl/Alt/Shift modifiers
- Bound mouse-wheel chord no longer triggers vanilla third- or first-person
  camera zoom (surgical vfunc hook on `PlayerInputHandler::ProcessButton`)
- MCM panel via [SKSEMenuFramework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)
  (soft dependency — JSON config remains canonical without it)

## Requirements

- Skyrim Special Edition / Anniversary Edition (1.6.x) with
  [SKSE64](https://skse.silverlock.org/)
- Optional: SKSEMenuFramework, for the in-game MCM panel

## Installation

Drop the contents of the release zip into your `Data/` folder, or install
the zip with your mod manager (MO2 / Vortex). The DLL lands at
`Data/SKSE/Plugins/WalkSpeedTuner.dll`.

## Configuration

In-game (with SKSEMenuFramework): open the SKSE menu → **Walk Speed Tuner**.

The panel exposes:
- **Enable** — master toggle. Off = vanilla behavior.
- **Boost** — 0–110% slider.
- **Suppression** — revert to vanilla speed during combat.
- **Hotkeys** — rebind Boost+ / Boost-. Press Set, then a key or
  Ctrl/Alt/Shift+key. ESC cancels.
- **Reset All to defaults**.

Without SKSEMenuFramework, edit
`Data/SKSE/Plugins/WalkSpeedTuner.json` directly. Defaults are written on
first run if the file is missing.

```json
{
  "enabled": true,
  "boost_pct": 0.0,
  "suppress_in_combat": true,
  "boost_up_keycode": 264,
  "boost_up_mods": 2,
  "boost_down_keycode": 265,
  "boost_down_mods": 2
}
```

Keycodes use the SkyUI / MCM-Helper extended-scan-code convention:
- `0x000–0x0FF` — keyboard DX scan codes
- `0x100–0x107` — mouse buttons (LMB, RMB, MMB, Mouse4–8)
- `0x108` — wheel up
- `0x109` — wheel down

Modifier mask is additive: `1` = Ctrl, `2` = Alt, `4` = Shift. So
`Alt+WheelUp` is `keycode = 264, mods = 2`.

## Building from source

Toolchain:
- Visual Studio 2022 with the C++23 toolset
- CMake 3.21+ and Ninja
- [vcpkg](https://github.com/microsoft/vcpkg) (manifest mode)

Set `VCPKG_ROOT` to your vcpkg checkout (or place a `vcpkg/` checkout next
to the repo), then:

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset release
cmake --build build/release
```

Or run the helper script:

```powershell
./build.ps1
```

The DLL lands at `build/release/WalkSpeedTuner.dll`.

To stage a release tree and (optionally) deploy into an MO2 mod folder:

```powershell
$env:SKYRIM_MODS_FOLDER = "C:\path\to\MO2\mods"   # optional
./deploy.ps1
```

If `SKYRIM_MODS_FOLDER` is unset, `deploy.ps1` only stages the release tree
under `release/SKSE/Plugins/`.

## Tests

Pure-function modules (parse, gate logic, throttle, hotkey matching, bump)
are covered by Catch2 unit tests with no SKSE link:

```powershell
./run-tests.ps1
```

## Releases

Tagging a commit with `v*.*.*` triggers the GitHub Actions release pipeline,
which builds the DLL, packages it as
`WalkSpeedTuner-vX.Y.Z.zip` (laid out as `SKSE/Plugins/WalkSpeedTuner.dll`),
and attaches it to a GitHub Release.

## Acknowledgments

- Cache-defeat tickle pattern from
  [TES5-DynamicSpeedController](https://github.com/DanjelPiDev/TES5-DynamicSpeedController).
- [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) for the
  reverse-engineered Skyrim runtime bindings.
- [SKSEMenuFramework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)
  for the dynamic MCM surface.
