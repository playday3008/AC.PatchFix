<!-- markdownlint-disable MD024 MD060 -->

# AC.PatchFix

ASI plugin framework for Assassin's Creed games that patches game binaries at runtime. All games are built from a single codebase with shared hook infrastructure and per-game specializations.

## Supported Games

| Game | Plugin | Status |
|------|--------|--------|
| Assassin's Creed Rogue | `AC.Rogue.PatchFix.asi` | Implemented |
| Assassin's Creed Syndicate | `AC.Syndicate.PatchFix.asi` | Implemented |

## Features

### Rogue

- **Ultrawide and non-standard aspect ratio support** — 21:9, 32:9, 16:10, 4:3, 5:4, and any custom ratio
- **FOV correction** — Vert+ and Hor+ modes with adjustable multiplier
- **Multi-monitor / triple-screen detection** — configurable threshold and manual override
- **FPS unlock** — remove the ~64 FPS cap entirely or set a custom target (e.g., 120, 144)
- **UI scaling** — configurable horizontal and vertical stretch to fill pillarbox/letterbox areas
- **Language unlock** — all languages available regardless of purchase region
- **UI language override** — force any language independent of system/registry settings
- **Hot-reload** — edit the INI file while the game is running, changes apply immediately
- **Per-hook toggles** — enable or disable individual fixes at runtime

### Syndicate

- **Controller prompt override** — force PlayStation or Xbox button prompts regardless of connected controller
- **DualShock 4 v2 fix** — recognize DS4v2 (PID 0x09CC) for correct PlayStation prompts
- **Platform specs fix** — stub DxDiag COM initialization to prevent a startup freeze/deadlock
- **FPS unlock** — remove or adjust the built-in frame rate cap
- **Resolution fix** — filter non-standard aspect ratio resolutions (e.g., 4096x2160 / 17:9) from the display mode list
- **Language unlock** — all languages available regardless of purchase region
- **Hot-reload** — edit the INI file while the game is running, changes apply immediately
- **Per-hook toggles** — enable or disable individual fixes at runtime

### Diagnostics

All games include a built-in diagnostics subsystem:

- **Crash reports** with stack traces written to the game directory
- **Minidump generation** for detailed post-mortem analysis
- **Crash journal** tracking hook installation state at time of failure

If you experience a crash, check the game directory for `.log`, `.journal` and `.dmp` (if present) files and include them in bug reports.

## Installation

### Prerequisites

An ASI loader is required. Install one of the following into the game directory:

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)

### Steps

1. Download the [latest release](https://github.com/playday3008/AC.Rogue-PatchFix/releases)
2. Place the `.asi` and `.ini` files for your game into `<path-to-game>/plugins/`
   - Rogue: `AC.Rogue.PatchFix.asi` + `AC.Rogue.PatchFix.ini`
   - Syndicate: `AC.Syndicate.PatchFix.asi` + `AC.Syndicate.PatchFix.ini`
3. Edit the INI file to configure the fixes
4. Launch the game

The plugin version is embedded in the DLL — check the log output or file properties to verify which version is installed.

### Steam Deck / Proton

After installing the ASI loader and patch files, add the following to the game's launch options:

```shell
WINEDLLOVERRIDES="version.dll=n,b" %command%
```

Replace `version.dll` with the actual DLL name used by your ASI loader.

## Configuration

### Rogue

All settings are in `AC.Rogue.PatchFix.ini`. Changes are picked up automatically while the game is running.

#### \[Display\]

| Key            | Default | Values | Description |
|----------------|---------| ------ | ----------- |
| `AspectRatio`  | `0`     | `0` (auto-detect), ratio like `21:9` or `32:9`, decimal like `2.333` | Override aspect ratio. Auto reads from game resolution. |
| `MultiMonitor` | `Auto`  | `Auto`, `Single`, `Multi` (alias: `Triple`) | Multi-monitor detection mode. Auto flags as multi when aspect ratio >= 4.0. |

#### \[UI\]

| Key                 | Default | Range         | Description |
|---------------------|---------|---------------| ----------- |
| `StretchHorizontal` | `0.0`   | `0.0` - `1.0` | Fill pillarbox area on wider-than-16:9 displays. `0.0` = no stretch, `1.0` = fill screen. |
| `StretchVertical`   | `0.0`   | `0.0` - `1.0` | Fill letterbox area on narrower-than-16:9 displays. `0.0` = no stretch, `1.0` = fill screen. |

#### \[FOV\]

| Key          | Default | Values | Description |
|--------------|---------| ------ | ----------- |
| `Mode`       | `Auto`  | `Auto`, `VertPlus`, `HorPlus` | Auto = Hor+ when narrower than 16:9, Vert+ when wider. |
| `Multiplier` | `1.0`   | any positive float | Additional FOV multiplier. `1.0` = default, `>1.0` = wider, `<1.0` = narrower. |

#### \[FPS\]

| Key      | Default | Values | Description |
|----------|---------| ------ | ----------- |
| `Target` | `0`     | `0` = uncapped, any positive value (e.g., `60`, `120`, `144`) | FPS cap. `0` removes the frame limiter entirely. |

#### \[Language\]

| Key          | Default | Values | Description |
|--------------|---------| ------ | ----------- |
| `UnlockAll`  | `false` | `true`, `false` | Make all languages available regardless of purchase region. Requires language data files to be present. |
| `UILanguage` | `None`  | `None`, `English`, `French`, `Spanish`, `Polish`, `German`, `ChineseTrad`, `Hungarian`, `Italian`, `Japanese`, `Czech`, `Korean`, `Russian`, `Dutch`, `Danish`, `Norwegian`, `Swedish`, `Portuguese`, `Brazil`, `Finnish`, `Arabic`, `Mexican`, or index `1`-`21` | Override UI language. |

#### \[Hooks\]

Toggle individual hooks. Accepts `true`/`false`, `yes`/`no`, `on`/`off`, `1`/`0`.

| Key                | Default | Description |
|--------------------|---------| ----------- |
| `GameState`        | `true`  | Tracks pause/unpause state for viewport fixes |
| `ViewportFitting`  | `true`  | Aspect ratio correction |
| `ViewportScaling`  | `true`  | UI coordinate scaling |
| `DisplayDetection` | `true`  | Multi-monitor detection |
| `FOVCorrection`    | `true`  | FOV adjustment |
| `FPSUnlock`        | `true`  | FPS cap removal / custom cap |
| `LanguageUnlock`   | `true`  | Language unlock and override |

### Syndicate

All settings are in `AC.Syndicate.PatchFix.ini`. Changes are picked up automatically while the game is running.

#### \[Input\]

| Key          | Default       | Values                  | Description |
|--------------|---------------|-------------------------| ----------- |
| `PromptType` | `PlayStation` | `Xbox`, `PlayStation`   | Force controller button prompt type. Useful when Steam Input remapping causes wrong prompts. |

#### \[FPS\]

| Key      | Default | Values | Description |
|----------|---------| ------ | ----------- |
| `Target` | `0`     | `0` = uncapped, any positive value (e.g., `60`) | FPS cap. `0` removes the frame limiter. Capping to 60 mitigates the London Drift perk not counting drifts at high FPS. |

#### \[Language\]

| Key          | Default | Values | Description |
|--------------|---------| ------ | ----------- |
| `UnlockAll`  | `false` | `true`, `false` | Make all languages available regardless of purchase region. Requires language data files to be present. |
| `IncludeLocTest` | `false` | `true`, `false` | Include the internal LocTest language in the unlock list. Only useful for development/testing. |

#### \[Hooks\]

Toggle individual hooks. Accepts `true`/`false`, `yes`/`no`, `on`/`off`, `1`/`0`.

| Key                | Default | Description |
|--------------------|---------| ----------- |
| `PlatformSpecsFix` | `true`  | Stub DxDiag COM init to prevent startup freeze |
| `DS4v2Fix`         | `true`  | DualShock 4 v2 controller recognition |
| `PromptOverride`   | `true`  | Force controller prompt type |
| `ResolutionFix`    | `true`  | Filter non-standard aspect ratio resolutions |
| `FPSUnlock`        | `true`  | FPS cap removal / custom cap |
| `LanguageUnlock`   | `true`  | Language unlock |

## How It Works

### Rogue

#### Language Region Lock

Assassin's Creed Rogue defines 22 language slots in its engine, but gates which ones are selectable behind a pair of bitfield values loaded at startup. Each language is assigned a bit position, and the game maintains two separate bitfields — one for subtitles and one for audio:

```c
uint32_t subtitle_languages;   // bit N set = language N available for subtitles
uint32_t audio_languages;      // bit N set = language N available for audio

// Bit assignment (1 << index):
//   English=1, French=2, Spanish=3, Polish=4, German=5,
//   ChineseTrad=6, Hungarian=7, Italian=8, Japanese=9, Czech=10,
//   Korean=11, Russian=12, Dutch=13, Danish=14, Norwegian=15,
//   Swedish=16, Portuguese=17, Brazil=18, Finnish=19, Arabic=20,
//   Mexican=21, LocTest=22
```

A worldwide Steam copy might have bits set for English, French, Spanish, German, Italian, etc. — but not Russian, Korean, or ChineseTrad. A Russian retail copy will have Russian set but not others.

Not all 22 enum values correspond to actual localization data. Eight languages (Hungarian, Czech, Danish, Norwegian, Swedish, Portuguese, Finnish, LocTest) are defined in the engine but never shipped with any retail SKU. The patch hooks the language setup routine and overwrites the bitfields with the union of all languages that actually exist across any SKU: `0x00343B7E` for menu/subtitles (14 languages) and `0x0004112E` for audio (7 languages — English, French, Spanish, German, Italian, Russian, Brazilian Portuguese). An optional `UILanguage` setting forces a specific default by writing directly to the game's global language index.

#### Game ID Detection

The game determines its regional SKU at startup by inspecting the language bitfields:

```c
uint16_t get_game_id(void) {
    if (subtitle_languages & (1 << RUSSIAN))
        return is_steam ? 0x4A3 : 0x4A2;   // RU region
    if ((subtitle_languages & (1 << KOREAN)) &&
        (subtitle_languages & (1 << CHINESE_TRAD)))
        return is_steam ? 0x67E : 0x67D;   // Asia region
    return is_steam ? 0x3A6 : 0x37F;       // Worldwide
}
```

Ubisoft's backend uses this ID to validate DLC entitlements. If the patch unlocked all language bits without fixing the game ID, a worldwide copy would see Russian in its bitfield, report as the RU SKU, and the backend would reject the user's DLC keys.

The patch solves this by snapshotting the original bitfields before overwriting them, running the region detection on the unmodified values to determine the true SKU, and replacing `GetGameId` with a stub that always returns the pre-computed correct ID.

#### Viewport and FOV

The game is built around a fixed 16:9 (1280x720 base) viewport. The rendering pipeline uses hardcoded values of 16/9 (~1.778) and its reciprocal 9/16 (0.5625) when fitting the 3D scene into the window. Any non-16:9 display gets black bars.

The patch intercepts two points in the ratio calculation: where the game loads the inverse aspect ratio (9/16), substituting the actual display reciprocal, and where it multiplies by the aspect ratio (16/9), substituting the real width/height ratio. A coordinate-transform hook adjusts UI element positioning so mouse input and HUD elements map correctly.

For FOV, the game writes a single value to a camera structure. Without correction, non-16:9 displays get vertical cropping (Vert-) on ultrawide or horizontal cropping on narrow displays. Three modes are available:

- **Auto** — Hor+ when narrower than 16:9, Vert+ when wider
- **Vert+** — vertical FOV stretches/compresses with the aspect ratio
- **Hor+** — horizontal FOV stays constant; vertical view expands on wider screens

The Hor+ correction uses `atan(0.768 * (16/9) / current_aspect) / atan(0.768)`, where `0.768` is the game's base zoom factor. A configurable multiplier is applied on top.

#### FPS Unlock

The game's frame pacing is controlled by a `FrameTiming` struct that selects between several timing modes: fixed (mode 0), adaptive (1), vsync (2), and averaged (3). The stock game runs in fixed mode, capping at roughly 64 FPS.

To uncap: the patch switches to averaged mode (3) and primes the QPC timestamps so the first frame doesn't compute a nonsensical delta. To set a custom cap: it switches to fixed mode (0) and writes the target FPS directly into the `fixed_rate` field. Both paths are hot-reloadable — changing the INI value re-applies the patch without restarting.

### Syndicate

#### Platform Specs Fix

At startup, the engine calls `Display_EnumerateAllModes` to gather hardware capabilities for submission to Ubisoft's platform analytics. This function initializes a DxDiag COM provider, which on some systems deadlocks or hangs for 30+ seconds. Since the game doesn't use the enumerated data for rendering, the patch stubs the function with a `ret` instruction, eliminating the startup freeze entirely.

#### DualShock 4 v2 Fix

The game's HID input classification recognizes the DualShock 4 v1 by its USB product ID (`0x05C4`) but has no check for the v2 revision (`0x09CC`). When a DS4 v2 is connected, it falls through the classification logic and gets treated as a generic gamepad, showing Xbox button prompts instead of PlayStation ones.

The patch hooks the PID comparison and extends it to accept both `0x05C4` and `0x09CC`. If neither matches, the original skip logic executes unchanged.

#### Controller Prompt Override

The engine determines button prompt style through `get_active_device_type`, which walks the input context chain (`InputContext` → `InputSystemState` → `DeviceManager` → active `DeviceSlot`) and returns a device type code: 0 for keyboard, 2 for Xbox, 5 for DS4/PlayStation. A downstream `== 5` check decides whether to show PlayStation or Xbox prompts.

The patch hooks `get_active_device_type` at its entry point. When a controller is active and the hook is enabled, it replaces the return value with the configured prompt type (2 for Xbox, 5 for PlayStation) and skips the rest of the function. This is hot-reloadable and respects the per-hook toggle.

#### Resolution Fix

The game populates its display mode list by calling `ModeList_InsertSorted` for each resolution the display driver reports. Some monitors report non-standard aspect ratios (e.g., 4096x2160 which reduces to 256:135) that the engine doesn't handle correctly, causing rendering artifacts or broken UI.

The patch hooks the insert function and checks each mode's reduced aspect ratio against a whitelist of standard ratios: 16:9, 16:10, 8:5, 21:9, 64:27, 43:18, 32:9, 5:4, 4:3, and 3:2. Non-matching entries are silently dropped.

#### FPS Unlock

Syndicate's frame limiter works differently from Rogue — it uses inline code rather than a data-driven timing struct. Three locations are patched:

1. **Flag gate** — a 6-byte instruction sequence that guards the frame limiter. NOP'd to uncap, restored to re-enable.
2. **Sleep branch** — a conditional `jnb` that enters the sleep path. Changed to an unconditional `jmp` when uncapping (always sleeps 0 ms), restored to `jnb` when capping (sleeps based on remaining budget).
3. **Frame time constant** — a `float` used by `mulss` to compute the per-frame time budget. Set to `1000 / target` for a custom cap, or restored to the original value (~32.33 ms, corresponding to ~30.9 FPS stock) when uncapping.

#### Language Unlock

The mechanism mirrors Rogue's bitfield approach but with structural differences. Syndicate uses three separate bitfields (menu, subtitle, audio) instead of Rogue's shared UI/subtitle pair, and its language enum has 23 entries (adding Simplified Chinese, absent in Rogue).

The patch overwrites the bitfields with the union of all languages that ship across any retail SKU: `0x007BFFFE` for menu and subtitles (21 languages) and `0x0068262E` for audio (10 languages). A mid-hook on the bitfield write instruction ensures the values persist through the engine's initialization sequence. An optional `IncludeLocTest` flag adds the internal test language to the unlock set.

Unlike Rogue, Syndicate does not require a Game ID fixup — DLC entitlements are handled through a different backend mechanism that doesn't derive the SKU from language bitfields.

## Known Limitations

- **Rogue: menus, cutscenes, and loading screens stay at 16:9** — engine limitation; stretching these breaks mouse input
- **Syndicate: London Drift perk bug at high FPS** — the perk doesn't count drifts reliably above ~60 FPS; cap FPS to 60 as a workaround

## Building from Source

### Prerequisites

- CMake 3.28+
- **Windows**: Visual Studio 2026 with ClangCL toolset and C++23 support
- **Linux** (cross-compile): LLVM 21 (clang-cl, lld-link, llvm-lib, llvm-rc, llvm-mt), Ninja, [msvc-wine](https://github.com/mstorsjo/msvc-wine) installed to `~/.msvc`

All dependencies are fetched automatically by CMake:

- [safetyhook](https://github.com/cursey/safetyhook)
- [Hooking.Patterns](https://github.com/ThirteenAG/Hooking.Patterns)
- [mINI](https://github.com/metayeti/mINI)
- [spdlog](https://github.com/gabime/spdlog)

### Windows

```sh
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
```

### Linux (cross-compile via msvc-wine)

```sh
cmake --preset wine-x64
cmake --build --preset wine-x64-release
```

Output: `build/bin/AC.Rogue.PatchFix.asi`, `build/bin/AC.Syndicate.PatchFix.asi`

## Credits

- [**@ThirteenAG**](https://github.com/ThirteenAG) — [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader), [Hooking.Patterns](https://github.com/ThirteenAG/Hooking.Patterns)
- [**@cursey**](https://github.com/cursey) — [safetyhook](https://github.com/cursey/safetyhook)
- [**@metayeti**](https://github.com/metayeti) — [mINI](https://github.com/metayeti/mINI)
- [**@gabime**](https://github.com/gabime) — [spdlog](https://github.com/gabime/spdlog)
- [**@WerWolv**](https://github.com/WerWolv) — [ImHex](https://github.com/WerWolv/ImHex)
- [**Hex-Rays**](https://hex-rays.com) — [IDA Pro](https://hex-rays.com/ida-pro)
- [**@NationalSecurityAgency**](https://github.com/NationalSecurityAgency) — [Ghidra](https://github.com/NationalSecurityAgency/ghidra)
- [**x64dbg Contributors**](https://x64dbg.com/#credits) — [x64dbg](https://x64dbg.com)

## License

[MIT](LICENSE)
