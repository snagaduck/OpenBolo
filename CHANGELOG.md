# Changelog

All notable changes to this project will be documented in this file.

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versions correspond to development phases rather than release tags during
active modernization. Semver will be adopted once the project reaches a
stable, distributable state.

---

## [Unreleased]

### Planned
- Phase B7 — Linux build verification (conditional CMake, POSIX socket stubs)
- Phase C — Networking modernization (IPv6, TLS, GameNetworkingSockets evaluation)
- Phase D — Code quality (test harness, warning cleanup, platform abstraction)
- Phase E — Feature updates (widescreen, WinBoloNet replacement, v1.17 fixes)

---

## [Phase B] — 2026-02-25

Complete replacement of the original Win32/DirectX platform layer with Raylib.

### Added
- **B1 — Game loop wiring**: `game_loop.c/.h` bridge connects `main.c` (Raylib)
  to `screenSetup` / `screenUpdate` / `screenGameTick` without exposing Win32
  types to `main.c`. Fixed x64 pointer truncation crash caused by missing
  `screenGetGameType()` declaration in `backend.h`.
- **B2 — Tile rendering**: `render_bridge.c/.h` Raylib draw bridge (compiled
  without `/FI` to avoid `Rectangle` type clash). 256-entry tile lookup table
  mirroring `drawSetupArrays`. 15×15 tile viewport with sub-tile smooth scroll.
  Post-build CMake rule copies `tiles.bmp` to output directory.
- **B3 — HUD and overlays**: zoom model (all HUD coords at zoom=1, scaled by
  `g_zoom` in `render_bridge`). `background.bmp` chrome layer. Status icon
  panels (pills, bases, tanks). Shell/mine/armour/tree status bars. Message
  lines, kills/deaths counters, man-status compass. `frontEndDrawDownload`
  overlay.
- **B4 — Audio**: 24 WAV sound effects loaded via Raylib. `renderLoadSounds` /
  `renderPlaySound` / `renderUnloadSounds` in `render_bridge.c`.
  `InitAudioDevice` / `CloseAudioDevice` in `main.c`. CMake post-build copies
  `sounds/` to output directory.
- **B5 — Input**: Raylib `IsKeyDown` / `IsKeyPressed` poll replaces Win32
  `input.c`. Arrow keys / WASD drive tank; Space fires; B deploys LGM; `[`/`]`
  adjust gunsight range. `boloTick(tankButton, shoot)` bridge in `game_loop.c`.
- **B6 — Win32 UI stubs → Raylib UI**: Pre-game player name entry screen
  (yellow text box, blinking cursor, Enter/ESC). `gameFrontSetPlayerName`
  setter in `win32stubs.c`. `boloInit` accepts `playerName`, passes it to
  `screenLoadMap` and the HUD message bar.

### Fixed
- Gunsight defaulted to off — added `screenSetGunsight(TRUE)` to `boloInit`.
- Viewport did not auto-center on tank — added `screenSetAutoScroll(TRUE)`.
- Base/pill capture silently failed in solo play — added `netSetType(netSingle)`
  (required by `tank.c:1503`).
- HUD bars, pill/base/tank panels, and player name blank at startup — added
  `screenForceStatusUpdate()` called once after `screenLoadMap` succeeds.
- WinBolo transparency key misidentified as top-left pixel of `tiles.bmp`
  (brownish terrain); correct key is pure lime green `RGB(0, 255, 0)`.
- Bilinear bleeding between tiles — introduced padded atlas (`g_tiles`, 18×18
  slots with 1 px edge extrusion) and alpha dilation (transparent pixels filled
  from nearest opaque neighbour).

---

## [Phase A] — 2026-02-23 / 2026-02-24

Goal: compile the original WinBolo 1.15 source on Windows with modern MSVC.
No functional changes — build system and minimal stubs only.

### Added
- **A1 — Server**: CMake build for standalone `winbolo-server.exe` with no
  graphics dependency. Winsock2/windows.h/mmsystem.h ordering fixed via
  `/FI winbolo_platform.h` force-include. Pragma pack bugs fixed in `brain.h`,
  `network.h`, and `servernet.h` wrappers. `winmm.lib` linked for multimedia
  timer API.
- **A2 — Client**: CMake build for `winbolo-client.exe` with Raylib 5.5
  (FetchContent). DirectX files excluded; replaced with stub
  `frontend_raylib.c`. `main.c` compiled as OBJECT library without `/FI` to
  avoid `wingdi.h::Rectangle()` / `raylib.h::Rectangle` struct clash.
  Embedded server sources (`servercore.c`, `servernet.c`, `threads.c`,
  `servertransport.c`) included in client build — required by the bolo engine
  even in single-player mode.

### Fixed
- Winsock2/windows.h header ordering conflict resolved globally via
  `/FI winbolo_platform.h`.
- Bad `#pragma pack(pop, name, 1)` directives in `network.h` and `servernet.h`
  caused struct misalignment; fixed with header wrappers that reset pack state.
