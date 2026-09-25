<!-- markdownlint-disable MD024 MD060 -->

# AC.Syndicate.PatchFix

Assassin's Creed Syndicate plugin of [PatchFix](../../../README.md). See the main README for installation, diagnostics and building.

## Features

- **Controller prompt override** — force PlayStation or Xbox button prompts regardless of connected controller
- **DualShock 4 v2 fix** — recognize DS4v2 (PID 0x09CC) for correct PlayStation prompts
- **Camera smoothing toggle** — disable the smoothing applied to look input so the camera tracks the mouse directly
- **Platform specs fix** — stub DxDiag COM initialization to prevent a startup freeze/deadlock
- **FPS unlock** — remove or adjust the built-in frame rate cap
- **Resolution fix** — filter non-standard aspect ratio resolutions (e.g., 4096x2160 / 256:135) from the display mode list
- **Aspect ratio fix** — correct the stretched image at ultrawide resolutions in fullscreen and borderless
- **Language unlock** — all languages available regardless of purchase region
- **Hot-reload** — edit the INI file while the game is running, changes apply immediately
- **Per-hook toggles** — enable or disable individual fixes at runtime

## Installation

Place `AC.Syndicate.PatchFix.asi` and `AC.Syndicate.PatchFix.ini` into `<path-to-game>/plugins/`, next to an ASI loader. Releases are tagged `ac-syndicate/vX.Y.Z`.

## Configuration

All settings are in `AC.Syndicate.PatchFix.ini`. Changes are picked up automatically while the game is running.

### \[Input\]

| Key          | Default       | Values                  | Description |
|--------------|---------------|-------------------------| ----------- |
| `PromptType` | `PlayStation` | `Xbox`, `PlayStation`   | Force controller button prompt type. Useful when Steam Input remapping causes wrong prompts. |
| `DisableCameraSmoothing` | `true` | `true`, `false` | Disable the camera smoothing applied to look input, so the camera follows the mouse directly instead of lerping toward it. |

### \[Display\]

| Key           | Default | Values | Description |
|---------------|---------| ------ | ----------- |
| `AspectRatio` | `0`     | `0` = auto, a ratio like `21:9` or `32:9`, or a decimal like `2.389` | Aspect ratio used for rendering. Auto derives it from the resolution the game applies, which is correct on every setup tested; override only if the detected value is wrong. |

### \[FPS\]

| Key      | Default | Values | Description |
|----------|---------| ------ | ----------- |
| `Target` | `0`     | `0` = uncapped, any positive value (e.g., `60`) | FPS cap. `0` removes the frame limiter. Capping to 60 mitigates the London Drift perk not counting drifts at high FPS. |

### \[Language\]

| Key          | Default | Values | Description |
|--------------|---------| ------ | ----------- |
| `UnlockAll`  | `false` (shipped INI sets `true`) | `true`, `false` | Make all languages available regardless of purchase region. Requires language data files to be present. |
| `IncludeLocTest` | `false` | `true`, `false` | Include the internal LocTest language in the unlock list. Only useful for development/testing. |

### \[Hooks\]

Toggle individual hooks. Accepts `true`/`false`, `yes`/`no`, `on`/`off`, `1`/`0`.

| Key                | Default | Description |
|--------------------|---------| ----------- |
| `PlatformSpecsFix` | `true`  | Stub DxDiag COM init to prevent startup freeze |
| `DS4v2Fix`         | `true`  | DualShock 4 v2 controller recognition |
| `PromptOverride`   | `false` | Force controller prompt type. Off by default because `DS4v2Fix` already handles DS4 detection; enable it when Steam Input remapping interferes. |
| `CameraSmoothing`  | `true`  | Camera smoothing disable |
| `ResolutionFix`    | `true`  | Filter non-standard aspect ratio resolutions |
| `AspectRatioFix`   | `true`  | Correct the render aspect ratio at ultrawide resolutions |
| `FPSUnlock`        | `true`  | FPS cap removal / custom cap |
| `LanguageUnlock`   | `true`  | Language unlock |

## How It Works

### Platform Specs Fix

At startup, the engine calls `Display_EnumerateAllModes` to gather hardware capabilities for submission to Ubisoft's platform analytics. This function initializes a DxDiag COM provider, which on some systems deadlocks or hangs for 30+ seconds. Since the game doesn't use the enumerated data for rendering, the patch stubs the function with a `ret` instruction, eliminating the startup freeze entirely.

### DualShock 4 v2 Fix

The game's HID input classification recognizes the DualShock 4 v1 by its USB product ID (`0x05C4`) but has no check for the v2 revision (`0x09CC`). When a DS4 v2 is connected, it falls through the classification logic and gets treated as a generic gamepad, showing Xbox button prompts instead of PlayStation ones.

The patch hooks the PID comparison and extends it to accept both `0x05C4` and `0x09CC`. If neither matches, the original skip logic executes unchanged.

### Controller Prompt Override

The engine determines button prompt style through `get_active_device_type`, which walks the input context chain (`InputContext` → `InputSystemState` → `DeviceManager` → active `DeviceSlot`) and returns a device type code: 0 for keyboard, 2 for Xbox, 5 for DS4/PlayStation. A downstream `== 5` check decides whether to show PlayStation or Xbox prompts.

The patch hooks `get_active_device_type` at its entry point. When a controller is active and the hook is enabled, it replaces the return value with the configured prompt type (2 for Xbox, 5 for PlayStation) and skips the rest of the function. This is hot-reloadable and respects the per-hook toggle.

### Camera Smoothing

The engine lerps the camera toward its target orientation instead of snapping to it, which reads as mouse smoothing. It already has a flag that bypasses this, checked immediately before the interpolation:

```
mov  rcx, [r13+0xA20]
cmp  byte [rcx+0x104], 0
jnz  skip_smoothing        ; patched to jmp
...
movss xmm0, [rbx+0xA0]
call  <lerp>
movss [rax+0xA0], xmm0
```

The patch rewrites the `jnz` opcode (`0x75`) to `jmp` (`0xEB`), keeping the same displacement, so the interpolation is always skipped and the camera target is written straight through. Restoring `0x75` re-enables smoothing, which makes the setting hot-reloadable.

The branch was identified by [@lnx00](https://github.com/lnx00) in [game-patches](https://github.com/lnx00/game-patches/blob/master/x64/acs-patches/src/patches/disable_camera_smoothing.rs).

### Resolution Fix

The game populates its display mode list by calling `ModeList_InsertSorted` for each resolution the display driver reports. Some monitors report non-standard aspect ratios (e.g., 4096x2160 which reduces to 256:135) that the engine doesn't handle correctly, causing rendering artifacts or broken UI.

The patch hooks the insert function and checks each mode's reduced aspect ratio against a whitelist of standard ratios: 16:9, 16:10, 8:5, 21:9, 64:27, 43:18, 32:9, 5:4, 4:3, and 3:2. Non-matching entries are silently dropped.

### Aspect Ratio Fix

At an ultrawide resolution the game renders a stretched 16:9 image in fullscreen and borderless, while windowed mode looks correct.

The renderer holds three aspect-ratio blocks, one per display topology. The mode-change handler computes the correct ratio and writes it to the block at `+0x958`, but the viewport code reads the one at `+0x928` — which only the AMD Eyefinity and NVIDIA Surround paths ever write, and both are gated off on a single monitor. It therefore keeps the hardcoded 16:9 value it was initialized with, so the viewport computes `1.7777778 / 1.7777778 = 1.0`, concludes no correction is needed, and stretches a 16:9 projection across the wider buffer.

The patch hooks the tail of the recompute and mirrors the computed ratio, with its gate bytes, into the block the viewport actually reads; `AspectRatio` overrides the value instead. It runs before the Eyefinity and Surround paths, so a multi-monitor span still has the final say.

### FPS Unlock

Syndicate's frame limiter works differently from Rogue — it uses inline code rather than a data-driven timing struct. Three locations are patched:

1. **Flag gate** — a 6-byte instruction sequence that guards the frame limiter. NOP'd to uncap, restored to re-enable.
2. **Sleep branch** — a conditional `jnb` that enters the sleep path. Changed to an unconditional `jmp` when uncapping (always sleeps 0 ms), restored to `jnb` when capping (sleeps based on remaining budget).
3. **Frame time constant** — a `float` used by `mulss` to compute the per-frame time budget. Set to `1000 / target` for a custom cap, or restored to the original value (~32.33 ms, corresponding to ~30.9 FPS stock) when uncapping.

### Language Unlock

The mechanism mirrors Rogue's bitfield approach but with structural differences. Syndicate uses three separate bitfields (menu, subtitle, audio) instead of Rogue's shared UI/subtitle pair, and its language enum has 23 entries (adding Simplified Chinese, absent in Rogue).

The patch overwrites the bitfields with the union of all languages that ship across any retail SKU: `0x007BFFFE` for menu and subtitles (21 languages) and `0x0068262E` for audio (10 languages). A mid-hook on the bitfield write instruction ensures the values persist through the engine's initialization sequence. An optional `IncludeLocTest` flag adds the internal test language to the unlock set.

Unlike Rogue, Syndicate does not require a Game ID fixup — DLC entitlements are handled through a different backend mechanism that doesn't derive the SKU from language bitfields.

## Known Limitations

- **UI and HUD scale with the render aspect** — the engine stores a single aspect ratio that both the 3D world and the UI derive from, so the UI cannot be given an aspect of its own
- **London Drift perk bug at high FPS** — the perk doesn't count drifts reliably above ~60 FPS; cap FPS to 60 as a workaround
