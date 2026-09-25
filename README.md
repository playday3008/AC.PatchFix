<!-- markdownlint-disable MD024 MD060 -->

# PatchFix

ASI plugin framework that patches game binaries at runtime. All games are built from a single codebase with shared hook infrastructure and per-game specializations. Each game lives in its own directory under `games/<series>/<game>/` with its own README, INI and version.

## Supported Games

| Game | Plugin | Docs | Releases |
|------|--------|------|----------|
| Assassin's Creed Rogue | `AC.Rogue.PatchFix.asi` | [README](games/ac/rogue/README.md) | [`ac-rogue/v*`](https://github.com/playday3008/AC.PatchFix/releases?q=ac-rogue) |
| Assassin's Creed Syndicate | `AC.Syndicate.PatchFix.asi` | [README](games/ac/syndicate/README.md) | [`ac-syndicate/v*`](https://github.com/playday3008/AC.PatchFix/releases?q=ac-syndicate) |

Releases up to `v3.3.0` bundled every game under one tag; later releases are tagged per game.

## Diagnostics

All games include a built-in diagnostics subsystem:

- **Crash reports** with stack traces written to the game directory
- **Minidump generation** for detailed post-mortem analysis
- **Crash journal** tracking hook installation state at time of failure
- **Debugger attach** — VMProtect's `ntdll!DbgUiRemoteBreakin` kill-stub is restored at startup, so debuggers attach normally instead of terminating the game

If you experience a crash, check the game directory for `.log`, `.journal` and `.dmp` (if present) files and include them in bug reports.

## Installation

### Prerequisites

An ASI loader is required. Install one of the following into the game directory:

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)

### Steps

1. Download the latest release for your game (see [Supported Games](#supported-games))
2. Place the `.asi` and `.ini` files into `<path-to-game>/plugins/`
3. Edit the INI file to configure the fixes (options are documented in the game's README)
4. Launch the game

The plugin version is embedded in the DLL — check the log output or file properties to verify which version is installed.

### Steam Deck / Proton

After installing the ASI loader and patch files, add the following to the game's launch options:

```shell
WINEDLLOVERRIDES="version.dll=n,b" %command%
```

Replace `version.dll` with the actual DLL name used by your ASI loader.

## Building from Source

### Prerequisites

- CMake 3.28+
- **Windows**: Visual Studio 2026 with ClangCL toolset and C++23 support
- **Linux** (cross-compile): LLVM 22 (clang-cl, lld-link, llvm-lib, llvm-rc, llvm-mt), Ninja, [msvc-wine](https://github.com/mstorsjo/msvc-wine) installed to `~/.msvc`

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

Output: one `.asi` per game in `build/bin/Release/`. To build a single game, pass its target, e.g. `cmake --build --preset wine-x64-release --target ac-rogue`.

### Adding a game

1. Create `games/<series>/<game>/` with `src/`, `include/games/<series>/<game>/`, an INI and a README. Optional `tests/*.cpp` are added to the shared test binary.
2. Add a `CMakeLists.txt` there:

   ```cmake
   project(Series.Game
       LANGUAGES CXX
       VERSION 1.0.0
   )

   patchfix_add_game(ARCH x64)
   ```

   The target is named `<series>-<game>`, the plugin `<project name>.PatchFix.asi`.
3. `add_subdirectory()` it from `games/<series>/CMakeLists.txt` (and the series from `games/CMakeLists.txt`).
4. Add `<series>/<game>` to the `game` choices in `.github/workflows/release.yml`.

## Credits

- [**@lnx00**](https://github.com/lnx00) — [game-patches](https://github.com/lnx00/game-patches), source of the Syndicate camera smoothing branch
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
