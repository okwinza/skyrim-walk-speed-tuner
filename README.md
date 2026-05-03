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

Tested on Windows 11 with Visual Studio 2022. Linux/macOS not supported —
this is a Skyrim mod.

### 1. Install prerequisites

- **Visual Studio 2022** (Community is fine, or Pro / Enterprise / Build
  Tools). In the Visual Studio Installer, enable:
  - **Desktop development with C++** workload
  - **C++ CMake tools for Windows** individual component (gives you CMake
    3.x and Ninja, both required by the preset)
  - MSVC v143 toolset version **17.6 or newer** — earlier 17.x doesn't
    have full C++23 support
- **Git** for Windows ([git-scm.com](https://git-scm.com/download/win))

Verify in a fresh PowerShell window:

```powershell
git --version
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -property installationPath
```

The second command should print your VS 2022 install path. If it errors or
returns nothing, the C++ workload isn't installed.

### 2. Clone the repo

```powershell
git clone https://github.com/<owner>/WalkSpeedTuner.git
cd WalkSpeedTuner
```

### 3. Set up vcpkg

vcpkg pulls in CommonLibSSE-NG (auto-fetched from the colorglass registry
configured in `vcpkg-configuration.json`), nlohmann_json, and Catch2
(tests only). Pick one of:

**Option A — clone vcpkg next to the repo (no config needed):**

```powershell
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
```

The build scripts find `.\vcpkg\vcpkg.exe` automatically. (`vcpkg/` is in
`.gitignore`, so it won't be tracked.)

**Option B — use a system-wide vcpkg via `VCPKG_ROOT`:** see step 4.

### 4. Configure your environment (optional)

Per-machine paths live in `.env.local` (gitignored). Copy the template:

```powershell
Copy-Item .env.dist .env.local
```

Edit `.env.local` and uncomment whichever apply. All vars are optional:

| Variable             | Purpose                                                            |
| -------------------- | ------------------------------------------------------------------ |
| `VCPKG_ROOT`         | Path to a system-wide vcpkg (skip if you used Option A in step 3). |
| `VCVARSALL`          | Override `vswhere`'s pick of `vcvarsall.bat`.                      |
| `SKYRIM_FOLDER`      | When set at configure time, every `cmake --build` copies the DLL straight to `Data\SKSE\Plugins\`. |
| `SKYRIM_MODS_FOLDER` | Lets `deploy.ps1` copy the staged release tree into your MO2 `mods\` directory. |

`_setup-env.ps1` (dot-sourced by all build scripts) reads `.env` first,
then `.env.local` (later overrides earlier). Variables already set in
your shell win over both files.

### 5. Build

```powershell
.\build.ps1
```

The helper discovers MSVC via `vswhere`, imports its environment into the
current PowerShell session, and runs CMake. The DLL lands at
`build\release\WalkSpeedTuner.dll`.

> **First build takes 10–15 minutes** while vcpkg compiles CommonLibSSE-NG
> from source. Subsequent builds finish in seconds (binary cache).

If PowerShell blocks the script with an execution-policy error, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

If you prefer raw CMake, you must run from a **Developer PowerShell for
VS 2022** (Start menu → Visual Studio 2022 folder) so that `cl.exe` is on
PATH; the preset wires the compiler by name, not full path:

```powershell
cmake --preset release
cmake --build build\release
```

### 6. Install into Skyrim

The fastest loop is to set `SKYRIM_FOLDER` in `.env.local` (step 4) — then
every `./build.ps1` automatically copies the DLL to
`Data\SKSE\Plugins\WalkSpeedTuner.dll`.

For Mod Organizer 2 users, set `SKYRIM_MODS_FOLDER` instead and run:

```powershell
.\deploy.ps1
```

This produces `<SKYRIM_MODS_FOLDER>\WalkSpeedTuner\SKSE\Plugins\WalkSpeedTuner.dll`,
which MO2 picks up as a separate mod entry you can toggle.

Or copy manually:

```powershell
Copy-Item build\release\WalkSpeedTuner.dll `
    "<skyrim>\Data\SKSE\Plugins\"
```

## Running tests

Pure-function modules (parser, gate logic, throttle, hotkey matcher,
boost-bump) are covered by Catch2 tests with no SKSE link:

```powershell
.\run-tests.ps1
```

You should see `100% tests passed, 0 tests failed`.

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
