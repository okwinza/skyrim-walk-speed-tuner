# Walk Speed Tuner

A small SKSE plugin that lets you walk faster in Skyrim SE/AE. Adds a
configurable boost (0–120%) to your walk speed only — sprint, run, sneak,
and NPCs are untouched.

**Save-clean.** The plugin never writes to your character's actor values.
Your saves stay byte-identical to vanilla, and removing the DLL takes you
straight back to vanilla speed — no orphan effects, no co-save data, no
script residue.

## Features

- 0–120% walk-speed boost (default: off)
- **Alt + Mouse Wheel Up / Down** to bump boost ±5% on the fly
- Optional: vanilla speed during combat (default on)
- In-game settings panel (with [SKSEMenuFramework](https://www.nexusmods.com/skyrimspecialedition/mods/120352))
- Hotkeys are rebindable — keyboard or mouse, with Ctrl/Alt/Shift modifiers
- Bound mouse-wheel hotkeys won't trigger vanilla camera zoom

## Requirements

- Skyrim Special Edition / Anniversary Edition (1.6.x) with
  [SKSE64](https://skse.silverlock.org/)
- Optional: [SKSEMenuFramework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)
  for the in-game settings panel

## Installation

Download the latest `WalkSpeedTuner-vX.Y.Z.zip` from the
[Releases](https://github.com/<owner>/WalkSpeedTuner/releases) page, then:

- **Mod Organizer 2 / Vortex**: drag the zip into your manager and install.
- **Manual**: extract into your Skyrim `Data\` folder. The DLL lands at
  `Data\SKSE\Plugins\WalkSpeedTuner.dll`.

## Usage

1. Toggle walk gait — Caps Lock by default, or whatever you bound to
   "Always Walk".
2. Hold **Alt** and scroll the mouse wheel — up to walk faster, down to
   slow down. Each click changes boost by 5%, up to a maximum of 120%.

The boost only applies while you're walking. Sprint, run, and sneak speeds
are unchanged.

## Settings

**With SKSEMenuFramework**: open the SKSE menu in-game →
**Walk Speed Tuner**.

| Setting                | What it does                                                          |
| ---------------------- | --------------------------------------------------------------------- |
| Enable                 | Master toggle. Off = vanilla speed.                                   |
| Boost (%)              | 0 to 120.                                                             |
| Suppress during combat | Revert to vanilla speed during fights.                                |
| Hotkeys                | Rebind Boost+ / Boost-. Click *Set*, press a key (or modifier+key). ESC cancels. |
| Reset All              | Restore defaults (boost 0, hotkeys Alt+Wheel).                        |

**Without SKSEMenuFramework**: edit
`Data\SKSE\Plugins\WalkSpeedTuner.json` directly. The file is created on
first run with these defaults:

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

Quick reference for keycodes / modifiers (SkyUI / MCM-Helper convention):

- `264` = Mouse Wheel Up, `265` = Mouse Wheel Down
- `0–255` = keyboard scan codes
- `256–263` = mouse buttons (LMB, RMB, MMB, Mouse4–8)
- Modifier mask: `1` Ctrl, `2` Alt, `4` Shift (add for combos — e.g. Ctrl+Alt = `3`)

## Acknowledgments

Cache-refresh trick adapted from
[TES5-DynamicSpeedController](https://github.com/DanjelPiDev/TES5-DynamicSpeedController)
(Apache-2.0). Built on
[CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG).

---

# For developers

## Building from source

Tested on Windows 11 with Visual Studio 2022. Linux/macOS not supported —
this is a Skyrim mod.

### 1. Install prerequisites

- **Visual Studio 2022** (Community is fine), with these components:
  - Desktop development with C++
  - C++ CMake tools for Windows (provides CMake and Ninja)
  - MSVC v143 toolset, version **17.6 or newer** (for C++23 support)
- **Git** for Windows ([git-scm.com](https://git-scm.com/download/win))

Verify in PowerShell:

```powershell
git --version
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -property installationPath
```

### 2. Clone (with submodules)

```powershell
git clone --recursive https://github.com/okwinza/skyrim-walk-speed-tuner.git WalkSpeedTuner
cd WalkSpeedTuner
```

`--recursive` pulls [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng)
under `extern/`. Forgot the flag? Run `git submodule update --init --recursive`.
`build.ps1` will also auto-init the submodule if it's missing.

### 3. Bootstrap vcpkg

vcpkg handles transitive deps (spdlog, DirectXTK, rapidcsv, fmt, nlohmann-json,
catch2). CommonLibSSE-NG is the submodule, not a vcpkg package.

```powershell
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
```

(Or point at a system-wide vcpkg via `VCPKG_ROOT` in `.env.local` — see
step 4.)

### 4. (Optional) Configure environment

```powershell
Copy-Item .env.dist .env.local
```

Edit `.env.local` and uncomment whatever applies:

| Variable             | Purpose                                                                  |
| -------------------- | ------------------------------------------------------------------------ |
| `VCPKG_ROOT`         | Path to a system-wide vcpkg (skip if you used step 3).                   |
| `VCVARSALL`          | Override `vswhere`'s pick of `vcvarsall.bat`.                            |
| `SKYRIM_FOLDER`      | When set, every build auto-copies the DLL to `Data\SKSE\Plugins\`.       |
| `SKYRIM_MODS_FOLDER` | Lets `deploy.ps1` copy into your MO2 `mods\` directory.                  |

Variables set in your shell win over the file.

### 5. Build

```powershell
.\build.ps1
```

The DLL lands at `build\release\WalkSpeedTuner.dll`.

> First build takes 10–15 minutes while vcpkg compiles CommonLibSSE-NG
> from source. Subsequent builds finish in seconds.

If PowerShell blocks the script with an execution-policy error:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

For raw CMake, run from a **Developer PowerShell for VS 2022** so `cl.exe`
is on PATH:

```powershell
cmake --preset release
cmake --build build\release
```

### 6. Deploy

If you set `SKYRIM_FOLDER` in step 4, every build deploys automatically.
For MO2, set `SKYRIM_MODS_FOLDER` and run `.\deploy.ps1`. Or copy by hand:

```powershell
Copy-Item build\release\WalkSpeedTuner.dll "<skyrim>\Data\SKSE\Plugins\"
```

## Tests

```powershell
.\run-tests.ps1
```

Catch2 unit tests for the pure-function modules (parser, gate logic,
throttle, hotkey matcher, boost-bump). No SKSE link.

## Releases

Tagging a commit with `v*.*.*` triggers the GitHub Actions pipeline, which
builds the DLL, packages it as `WalkSpeedTuner-vX.Y.Z.zip` (laid out as
`SKSE/Plugins/WalkSpeedTuner.dll`), and attaches it to a GitHub Release
with auto-generated notes.
