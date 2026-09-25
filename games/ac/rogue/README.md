<!-- markdownlint-disable MD024 MD060 -->

# AC.Rogue.PatchFix

Assassin's Creed Rogue plugin of [PatchFix](../../../README.md). See the main README for installation, diagnostics and building.

## Features

- **Ultrawide and non-standard aspect ratio support** — 21:9, 32:9, 16:10, 4:3, 5:4, and any custom ratio
- **FOV correction** — Vert+ and Hor+ modes with adjustable multiplier
- **Multi-monitor / triple-screen detection** — configurable threshold and manual override
- **FPS unlock** — remove the ~64 FPS cap entirely or set a custom target (e.g., 120, 144)
- **Full display mode list** — every refresh rate the monitor reports is selectable, instead of one entry per resolution collapsed to 60 Hz
- **Settings menu crash fix** — the display mode lookup is bounds checked, so a saved mode that is no longer in the list no longer faults
- **UI scaling** — configurable horizontal and vertical stretch to fill pillarbox/letterbox areas
- **Sprint camera lean fix** — the lean is scaled by frame time, so it no longer over-rolls at high frame rates
- **Language unlock** — all languages available regardless of purchase region
- **UI language override** — force any language independent of system/registry settings
- **Hot-reload** — edit the INI file while the game is running, changes apply immediately
- **Per-hook toggles** — enable or disable individual fixes at runtime

## Installation

Place `AC.Rogue.PatchFix.asi` and `AC.Rogue.PatchFix.ini` into `<path-to-game>/plugins/`, next to an ASI loader. Releases are tagged `ac-rogue/vX.Y.Z`.

## Configuration

All settings are in `AC.Rogue.PatchFix.ini`. Changes are picked up automatically while the game is running.

### \[Display\]

| Key            | Default | Values | Description |
|----------------|---------| ------ | ----------- |
| `AspectRatio`  | `0`     | `0` (auto-detect), ratio like `21:9` or `32:9`, decimal like `2.333` | Override aspect ratio. Auto reads from game resolution. |
| `MultiMonitor` | `Auto`  | `Auto`, `Single`, `Multi` (alias: `Triple`) | Multi-monitor detection mode. Auto flags as multi when aspect ratio >= 4.0. |

### \[UI\]

| Key                 | Default | Range         | Description |
|---------------------|---------|---------------| ----------- |
| `StretchHorizontal` | `0.0`   | `0.0` - `1.0` | Fill pillarbox area on wider-than-16:9 displays. `0.0` = no stretch, `1.0` = fill screen. |
| `StretchVertical`   | `0.0`   | `0.0` - `1.0` | Fill letterbox area on narrower-than-16:9 displays. `0.0` = no stretch, `1.0` = fill screen. |

### \[FOV\]

| Key          | Default | Values | Description |
|--------------|---------| ------ | ----------- |
| `Mode`       | `Auto`  | `Auto`, `VertPlus`, `HorPlus` | Auto = Hor+ when narrower than 16:9, Vert+ when wider. |
| `Multiplier` | `1.0`   | any positive float | Additional FOV multiplier. `1.0` = default, `>1.0` = wider, `<1.0` = narrower. |

### \[FPS\]

| Key      | Default | Values | Description |
|----------|---------| ------ | ----------- |
| `Target` | `0`     | `0` = uncapped, any positive value (e.g., `60`, `120`, `144`) | FPS cap. `0` removes the frame limiter entirely. |

### \[CameraLean\]

| Key            | Default | Values | Description |
|----------------|---------| ------ | ----------- |
| `ReferenceFPS` | `30`    | `0` = no lean, any positive value | Frame rate the sprint camera lean was tuned for. The game adds the lean once per frame with no time step, so above this rate the camera over-rolls before snapping back. At this value the fix changes nothing; the game shipped capped at 30. |

### \[Input\]

| Key               | Default | Range          | Description |
|-------------------|---------|----------------| ----------- |
| `SmoothingFactor` | `0.1`   | `0.02` - `1.0` | Blend coefficient of the engine's look-input lag, cancelled by pre-emphasising the mouse delta. `1.0` = no compensation. Lower until the camera stops when the mouse stops; too low overshoots. Below `0.02` is treated as off. Per-frame value, so retune if the feel changes with `[FPS] Target`. |

### \[Language\]

| Key          | Default | Values | Description |
|--------------|---------| ------ | ----------- |
| `UnlockAll`  | `false` (shipped INI sets `true`) | `true`, `false` | Make all languages available regardless of purchase region. Requires language data files to be present. |
| `UILanguage` | `None`  | `None`, `English`, `French`, `Spanish`, `Polish`, `German`, `ChineseTrad`, `Hungarian`, `Italian`, `Japanese`, `Czech`, `Korean`, `Russian`, `Dutch`, `Danish`, `Norwegian`, `Swedish`, `Portuguese`, `Brazil`, `Finnish`, `Arabic`, `Mexican`, or index `1`-`21` | Override UI language. |

### \[Hooks\]

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
| `ModeIndexGuard`   | `true`  | Bounds check the display mode index the settings menu reads |
| `FullModeList`     | `true`  | List every mode the monitor reports instead of one per resolution at 60 Hz |
| `CameraLean`       | `true`  | Scale the sprint camera lean by frame time so it does not over-roll at high frame rates |
| `MouseSmoothing`   | `true`  | Cancel the look-input lag that keeps the camera drifting after the mouse stops (needs `GameState`) |

## How It Works

### Language Region Lock

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

### Game ID Detection

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

### Viewport and FOV

The game is built around a fixed 16:9 (1280x720 base) viewport. The rendering pipeline uses hardcoded values of 16/9 (~1.778) and its reciprocal 9/16 (0.5625) when fitting the 3D scene into the window. Any non-16:9 display gets black bars.

The patch intercepts two points in the ratio calculation: where the game loads the inverse aspect ratio (9/16), substituting the actual display reciprocal, and where it multiplies by the aspect ratio (16/9), substituting the real width/height ratio. A coordinate-transform hook adjusts UI element positioning so mouse input and HUD elements map correctly.

For FOV, the game writes a single value to a camera structure. Without correction, non-16:9 displays get vertical cropping (Vert-) on ultrawide or horizontal cropping on narrow displays. Three modes are available:

- **Auto** — Hor+ when narrower than 16:9, Vert+ when wider
- **Vert+** — vertical FOV stretches/compresses with the aspect ratio
- **Hor+** — horizontal FOV stays constant; vertical view expands on wider screens

The Hor+ correction uses `atan(0.768 * (16/9) / current_aspect) / atan(0.768)`, where `0.768` is the game's base zoom factor. A configurable multiplier is applied on top.

### FPS Unlock

The game's frame pacing is controlled by a `FrameTiming` struct that selects between several timing modes: fixed (mode 0), adaptive (1), vsync (2), and averaged (3). The engine's constructor selects vsync mode and nothing writes the field afterwards.

In vsync mode the frame deadline is `current_time + trunc(ticks_per_ms * period)`, where `period` is a millisecond constant read by a `mulss` inside `UpdateFrameTiming`. That constant has a single reference in the binary, so the patch retunes it rather than changing the timing mode: `1000 / target` for a custom cap, or zero to uncap, which leaves the deadline at "now" so the wait loop never waits. The pending deadline is cleared alongside it, since it was computed from the previous period and would otherwise stall the loop once.

Capping also raises the process timer resolution to 1 ms so the engine's own frame wait can pace accurately; the game never does this itself. Uncapped there is no wait at all, so the resolution is left alone. Both paths are hot-reloadable — changing the INI value re-applies the patch without restarting.

### Display Mode List

The renderer builds its display mode list by keeping one entry per resolution, preferring whichever mode sits within 1 Hz of 60. On a high refresh rate panel that discards every native refresh entry, and a saved mode whose refresh rate is gone no longer resolves to an index. The settings code stores `-1` for that miss, and the mode getter indexes its array with no bounds check, so the read lands roughly 4 GB past the array and faults.

Two hooks address this:

- **FullModeList** replaces the list builder. It re-enumerates through `IDXGIOutput::GetDisplayModeList` with `DXGI_ENUM_MODES_SCALING`, drops anything below the engine's own 800x600 floor, sorts by width, height and refresh rate so the engine's binary search still works, grows the vector through the game's own reserve routine when needed, and publishes the full set.
- **ModeIndexGuard** clamps the index passed to the mode getter to the last valid entry, mirroring the clamp the mode setter already performs on the same value. When the list is empty it zeroes the out parameters and returns instead.

### Mouse Smoothing

Unlike Syndicate, Rogue has no smoothing flag to flip. The mouse delta leaves DirectInput unfiltered, is written raw into the virtual right stick, and reaches the player controller unfiltered; the lag is applied further downstream by the camera's data-driven damping, so there is no constant in the executable to change.

**MouseSmoothing** hooks the input update where the delta is live, after the cursor-mode filter and before the consumers, and pre-emphasises it to invert a first-order lag: feeding `estimate + (raw - estimate) / factor` makes a downstream `state += factor * (input - state)` land on `raw`. When the output has to be clamped, the shortfall is carried into the next frame so the integrated motion still matches the mouse. The hook only runs while `GameState` reports in-game, because menus drive the hardware cursor from the same buffer.

Two caveats. Every consumer of the right stick sees the pre-emphasised delta, not only the camera, so anything without a lag of its own (gesture recognition, the flick detector) receives amplified counts. And `SmoothingFactor` is a per-frame coefficient; if the engine's blend scales with frame time, the value that cancels it changes with frame rate.

## Known Limitations

- **menus, cutscenes, and loading screens stay at 16:9** — engine limitation; stretching these breaks mouse input
