# WinBolo Modernization Plan

## Session Log

### Session 1 (2026-02-23)
- Explored full codebase — 7 components identified (winbolo, tracker, wbn, wome, brains, jbolo, sounds)
- Decided on: CMake build system, Raylib for graphics/audio, Windows-first target
- Deferred: Valve GNS networking (Phase C), Java port (jbolo skipped entirely)
- Installed: Visual Studio 2026 Community + C++ workload, CMake Tools + C/C++ VS Code extensions
- Created: `new-project/` scaffold with CMakeLists.txt files and preferences stub

### Session 2 (2026-02-23)
- Opened project in Visual Studio 2026 (folder mode, CMake detected automatically)
- Fixed pragma pack cascade: `brain.h` (push/pop), `network.h` + `servernet.h` wrappers (bad `pop,name,1`)
- Fixed Windows header ordering: `winbolo_platform.h` force-included via `/FI` (winsock2 before windows.h, mmsystem.h restored)
- Fixed linker: removed client-only bolo files from server build (matched original Makefile link list)
- Added `winmm.lib` to server link for multimedia timer API
- Established direct CMake/MSBuild command-line access — no more manual copy-paste of errors
- **`winbolo-server.exe` builds and runs cleanly, prints usage, exits with code 0**

### Session 3 (2026-02-24)
- Implemented Phase A2: client build with Raylib 5.5 (FetchContent)
- Key discoveries:
  - WinBolo client always embeds a full server — `servercore.c`, `servernet.c` etc. must be in the client build
  - `bolo/screen.c` is the main client screen layer (missed initially)
  - Raylib `Rectangle` vs `wingdi.h::Rectangle()` clash — solved by compiling `main.c` as OBJECT library without /FI
  - `global.h` defines `typedef BYTE bool` — never use `<stdbool.h>` in this codebase
  - `bolo/serverfrontend.c` only has 4 stubs; the rest come from real `server/servercore.c` + `server/servernet.c`
- **`winbolo-client.exe` builds and opens a Raylib 800×600 window (OpenGL 3.3), runs at 20 FPS**

### Session 6 (2026-02-24)
- Implemented Phase B3: HUD and Overlays
- Zoom model: all HUD coords are zoom=1 (positions.h values); `render_bridge` multiplies by `g_zoom` (default 2)
- Window resized to 1030×650 (515×2 × 325×2) — crisp on modern screens, easy to change
- `background.bmp` (515×325) loaded as chrome background; tiles drawn at (MAIN_OFFSET_X=81, MAIN_OFFSET_Y=18) offset
- `render_bridge` expanded: `renderSetZoom`, `renderStatusIcon`, `renderLoadBackground`, `renderDrawBackground`, `renderDrawBar`, `renderDrawText`, `renderDrawManStatus`
- `frontend_raylib.c`: all setter stubs → static globals; `drawHUD()` draws icons/bars/messages/man-compass each frame
- Key fix: `MESSAGE_STRING_SIZE` defined locally (68) — draw.h includes `<ddraw.h>` and can't be pulled in
- **background.bmp chrome + tile viewport + status panels + bars + messages all render correctly**

### Session 7 (2026-02-25)
- Implemented Phase B5: Keyboard Input
- `win32stubs.c` → `input_bridge.c`: Raylib `IsKeyDown()` polls for tank Arrow/WASD movement, shoot (Space), LGM (B), gunsight range (`[`/`]`)
- `boloTick()` in `game_loop.c` receives packed `tankButton` ordinal + `shoot` flag from main loop
- Tank movement, shooting, and LGM all work via keyboard
- **Player can drive tank, shoot pillboxes, and fire LGM**

### Session 8 (2026-02-25)
- Gunsight and transparency fixes:
  - `screenSetGunsight(TRUE)` in `boloInit` — gunsight was off by default
  - `screenSetAutoScroll(TRUE)` in `boloInit` — viewport centers on gunsight instead of tank
  - Fixed transparency: WinBolo key color is pure lime green `{0,255,0,255}` (NOT top-left pixel of tiles.bmp which is brownish 128,95,64)
- Base capture fix: `netGetType()==netSingle` required in `tank.c:1503`; added `netSetType(netSingle)` to `boloInit`; added `netSetType()` to `network.c` + `network.h`
- HUD status bars at startup: bars were invisible until first shot; added `screenForceStatusUpdate()` to `screen.c` / `backend.h`, called from `boloInit`
- Sprite rendering overhaul (alpha dilation + padded atlas):
  - **Alpha dilation**: transparent pixels get RGB filled from nearest opaque 4-connected neighbour (2 passes); prevents bilinear from bleeding black at sprite edges
  - **Padded atlas** (`g_tiles`): each 16×16 tile slot → 18×18 (1px border extrusion all sides + corners); prevents bilinear bleeding between adjacent tiles. Point filter keeps terrain crisp.
  - **Original sheet** (`g_icons`): dilated but unpadded; used for sprites/icons that span tile-column/row boundaries (shell sprites cross row 4/5 boundary; status icons cross column boundaries)
  - `TO_PAD_X/Y` macros convert original pixel coords to padded atlas coords
  - `renderTile`/`renderMine` use `g_tiles`; `renderSprite`/`renderStatusIcon` use `g_icons`

### Session 9 (2026-02-25)
- HUD tanks panel fix: player's own tank icon was always blank
  - Root cause: `frontEndStatusTank` only called from `players.c` during network events (never in solo play)
  - Extended `screenForceStatusUpdate()` to also push pill/base/tank states at init:
    - Loops all pills → `frontEndStatusPillbox(i, pillsGetAllianceNum(&mypb, i))`
    - Loops all bases → `frontEndStatusBase(i, basesGetStatusNum(&mybs, i))`
    - Calls `frontEndStatusTank(playerSelf+1, tankSelf)` for the player's own slot
- **All HUD panels (tanks, pills, bases) now populated at startup**
- Player name message bar fix: name was blank until first action
  - Root cause: same init pattern — `frontEndMessages` never called at startup
  - Extended `screenForceStatusUpdate()` once more: `playersGetPlayerName` + `frontEndMessages(pname, "")`
- Font spacing attempt: tried `DrawTextEx(GetFontDefault(), ...)` for wider spacing — silently blanked all text, reverted to `DrawText`
- **Player name visible at startup**

### Session 5 (2026-02-24)
- Implemented Phase B2: tile rendering
- `render_bridge.c/.h` bridge (compiled without /FI) — `renderLoadTiles` / `renderTile` / `renderMine`
- `frontend_raylib.c` — `initTileLookup()` table + full `frontEndDrawMainScreen()` implementation
- `screenSetInStartFind(FALSE)` fix in `game_loop.c` — clears the `inStart` flag that was blocking rendering
- CMakeLists: `winbolo-render-bridge` OBJECT lib + post-build `tiles.bmp` copy
- **tiles.bmp (496×160) loads as Texture ID 3; Everard Island map tiles render in the window**

### Session 4 (2026-02-24)
- Implemented Phase B1: game loop wiring
- Key discoveries:
  - `netStat` initialises to `netRunning` in `network.c` — no network setup needed for solo test
  - `screenUpdate()` calls `frontEndDrawMainScreen()` only when `netStatus == netRunning|netFailed` and `inStart == FALSE`
  - `screenLoadMap()` is the one-shot init: calls `screenSetup()` + `mapRead()` + `screenSetupTank()`
  - `tankButton` first value is `TNONE` (no input); `gameOpen = 1` is the default game type
  - `game_loop.c/.h` bridge pattern: compiled WITH /FI, exposes only `int`/`void` API so `main.c` stays raylib-safe
  - **Root cause of B1 crash**: `screenGetGameType()` was missing from `backend.h`. On x64 MSVC, the implicit
    `int` return truncated the 64-bit pointer to 32 bits, so `gameTypeGetItems()` received a garbage pointer
    and crashed on `*gmeType`. Fix: added `gameType *screenGetGameType();` declaration to `backend.h`.
- Test map hard-coded to `Everard Island.map` (ships with WinBolo source)
- **`frontEndDrawMainScreen()` now gets called every frame by the game engine**
- **Resume at: Phase B2 — Tile Rendering**

---

## Project Goal
Modernize WinBolo 1.15 (a ~1998–2008 multiplayer tank game) into a maintainable,
cross-platform project using modern tooling, while preserving the original game logic.

Original source: `../winbolo115-src/winbolo/src/`
New project root: this directory
**jbolo (Java port) is permanently out of scope.**

---

## Architecture Decisions (Locked In)

| Concern         | Decision         | Rationale |
|----------------|------------------|-----------|
| Build system   | CMake 3.20+      | Cross-platform, VS/Ninja/Make compatible |
| Graphics/Audio | Raylib 5.5       | C-native, zero deps, modern OpenGL, replaces DirectX 5 + SDL 1.2 |
| Networking     | Original (Phase A/B), consider GNS (Phase C) | Keep protocol compatibility first |
| Language std   | C11              | Minimal change from original K&R/C89 style |
| Platform target | Windows first, Linux second | Matches original priority |

---

## Phase A — Get It Compiling ✅ COMPLETE

Goal: compile the original source code on Windows with modern MSVC via CMake.
No functional changes — only build system + minimal stubs where needed.

### A1 — Server (no graphics dependency) ✅
- [x] Create `new-project/` structure
- [x] Root `CMakeLists.txt`
- [x] `server/CMakeLists.txt` with all source references
- [x] `server/win32/preferences_stub.c`
- [x] Fix pragma pack bugs (`brain.h`, `network.h`, `servernet.h` wrappers)
- [x] Fix winsock2/windows.h/mmsystem.h ordering via `/FI winbolo_platform.h`
- [x] Fix linker: remove client-only bolo files from server build
- [x] **`winbolo-server.exe` builds and runs — prints usage, exits 0**

### A2 — Client (Raylib replaces DirectX) ✅
- [x] `client/CMakeLists.txt` with FetchContent Raylib 5.5
- [x] DirectX files identified and excluded (draw.c, dsutil.c, gamefront.c, input.c, sound.c, winutil.c)
- [x] `client/main.c` — Raylib window + 20 FPS loop (OBJECT library, no /FI)
- [x] `client/frontend_raylib.c` — all 21 `frontend.h` stubs (empty for now)
- [x] `client/preferences_stub.c` — Windows INI path helper
- [x] `client/win32stubs.c` — stubs for excluded DirectX/WinMain symbols
- [x] Added `bolo/screen.c`, `bolo/network.c`, `server/servercore.c`, `server/servernet.c`, `server/servermessages.c`, `server/threads.c`, `server/win32/servertransport.c`
- [x] **`winbolo-client.exe` builds and opens an 800×600 Raylib window**

---

## Phase B — Platform Layer (Raylib Integration) ← CURRENT

Goal: replace the Windows-only rendering/audio/input with Raylib so the same
client code compiles on Windows and Linux.

### B1 — Game Loop Wiring ✅
- [x] Wire `main.c` game loop to `screenSetup()` / `screenUpdate()` / `screenGameTick()`
- [x] Load a `.bolo` map file at startup (`Everard Island.map`, hard-coded in `main.c`)
- [x] Get `frontEndDrawMainScreen()` called by the game engine
- [x] `game_loop.c/.h` bridge: raylib-safe API between `main.c` and bolo engine
- [x] Fix crash: add `gameType *screenGetGameType();` declaration to `backend.h`

### B2 — Tile Rendering ✅
- [x] Locate `tiles.bmp` tile sheet in source tree (`gui/win32/tiles.bmp`, 496×160 px)
- [x] `render_bridge.c/.h` — raylib tile-draw bridge (compiled without /FI to avoid Rectangle clash)
- [x] `initTileLookup()` — 256-entry table mirroring `drawSetupArrays` from `draw.c`, zoomFactor=1
- [x] `frontEndDrawMainScreen()` — 15×15 tile grid + mine overlay via `renderTile`/`renderMine`
- [x] `edgeX`/`edgeY` pixel offsets passed through (sub-tile smooth scroll ready)
- [x] Post-build CMake rule copies `tiles.bmp` next to exe
- [x] **Texture loads (496×160 R8G8B8), tiles render in window**

### B3 — HUD and Overlays ✅
- [x] Window resized to 1030×650 (zoom=2); zoom model built into `render_bridge` (`renderSetZoom`)
- [x] `background.bmp` (515×325) loaded and stretched to window size as chrome layer
- [x] Tile grid drawn at (MAIN_OFFSET_X=81, MAIN_OFFSET_Y=18) with sub-tile edgeX/Y scroll
- [x] All setter stubs → static globals (pill/base/tank states, bars, messages, kills, deaths, man)
- [x] `render_bridge`: `renderDrawBackground`, `renderStatusIcon`, `renderDrawBar`, `renderDrawText`, `renderDrawManStatus`
- [x] Status icons: 12×12 sprites from `tiles.bmp` placed at original `STATUS_PILLS/BASES/TANKS_*` positions
- [x] Status bars: green vertical rects at `STATUS_TANK_SHELLS/MINES/ARMOUR/TREES` positions
- [x] Messages: text at (21,272) and (21,289)
- [x] Kills/Deaths: text at (462,85) and (462,99)
- [x] Man status: compass circle at (MAN_STATUS_X+R, MAN_STATUS_Y+R), arrow or X or empty
- [x] `frontEndDrawDownload`: black rect over tile area + "Downloading..." text
- [x] CMakeLists: post-build copy `background.bmp` next to exe
- [x] **background chrome + tile viewport + status panels + bars + messages all render correctly**

### B4 — Audio ✅
- [x] Locate WAV sound files (`winbolo115-src/sounds/`, 24 WAVs)
- [x] `InitAudioDevice()` / `CloseAudioDevice()` in `main.c`
- [x] `renderLoadSounds` / `renderPlaySound` / `renderUnloadSounds` in `render_bridge.c`
- [x] `frontEndPlaySound(sndEffects value)` → `renderPlaySound((int)value)` in `frontend_raylib.c`
- [x] CMakeLists post-build `copy_directory` copies `sounds/` next to exe
- [x] **All 24 sound effects play correctly in-game**

### B5 — Input ✅
- [x] Replace Win32 keyboard/mouse (`input.c`) with Raylib `IsKeyDown()` / `IsKeyPressed()`
- [x] Wire input to tank controls via `boloTick(tankButton, shoot)` in `game_loop.c`
- [x] Arrow keys / WASD → tank movement; Space → shoot; B → LGM; `[`/`]` → gunsight range
- [x] **Player can drive tank, shoot, capture pillboxes/bases, fire LGM**

### B6 — Win32 UI Stubs → Raylib UI ✅
- [x] `gameFrontGetPlayerName`: replaced hardcoded `"Player"` with a global buffer + `gameFrontSetPlayerName()` setter in `win32stubs.c`
- [x] `boloInit()` now accepts `playerName`, calls the setter, and passes it to `screenLoadMap`
- [x] Pre-game name-entry screen in `main.c`: yellow text box, blinking cursor, Enter=confirm / ESC=default
- [x] `windowWnd` / `windowGetInstance`: NULL stubs are correct — engine never calls them in solo play
- [x] `windowsGetTicks` / `serverMainGetTicks`: `GetTickCount64()` stubs work fine — RTT/timeout logic needs only a monotonic counter
- [x] `screenForceStatusUpdate()` extended: pushes pills/bases/tank icon AND player name message at startup
- [x] **Player name flows from UI → engine → network packets; HUD fully populated at startup**

### B7 — Verify Linux Build ← NEXT
- [ ] Add Linux support to CMakeLists (conditional Raylib platform, no WIN32 stubs needed)
- [ ] Fix any `#ifdef _WIN32` gaps in the codebase

---

## Phase C — Networking Modernization

- [ ] Audit `udppackets.c`, `netpnb.c`, `netmt.c` for buffer safety
- [ ] Add IPv6 support to `servertransport.c`
- [ ] Evaluate GameNetworkingSockets (Valve GNS) — Option 1: wrap existing protocol (keeps old client compat)
- [ ] Add TLS/encryption for WinBoloNet HTTP calls (currently plain HTTP)

---

## Phase D — Code Quality & Architecture

- [ ] CMake-based test harness (Unity test framework or similar)
- [ ] Unit tests for pure logic modules (crc.c, bolo_map.c, allience.c, etc.)
- [ ] Remove all SVN `$Id$` tags
- [ ] Replace `#ifdef _WIN32` chains with a clean platform abstraction header
- [ ] Address MSVC warnings: signed/unsigned mismatches, deprecated functions
- [ ] Memory audit: scan for malloc/free imbalances

---

## Phase E — Feature Updates (Future / Optional)

- [ ] Replace WinBoloNet (PHP/MySQL) backend with a modern web service
- [ ] Add widescreen support (original renders at fixed resolution)
- [ ] Replace brains DLL plugin system with something cross-platform
- [ ] Incorporate v1.17 bug fixes (tank physics, Vista drawing fixes, NAT improvements, server crash fixes)

---

## Known Issues & Notes

- **`bool` type**: `global.h` defines `typedef BYTE bool`. Do NOT use `<stdbool.h>` anywhere in this codebase — it conflicts.
- **Raylib/wingdi clash**: `Rectangle()` GDI function vs Raylib `Rectangle` struct. Solved by compiling Raylib files as OBJECT library without /FI. Phase B files that include raylib.h need the same treatment.
- **`main.c` OBJECT library**: compiled WITHOUT `/FI winbolo_platform.h` — intentional. All other client sources use /FI.
- **`serverfrontend.c` duality**: `bolo/serverfrontend.c` = 4 client stubs for server calls. `server/serverfrontend.c` = stub frontend for standalone server. Client build does NOT include either — real implementations are in client files + server/*.c.
- **v1.17 source**: not available (binaries only in `linbolo117-server/` and `winbolo117-setup/`). v1.15 remains the source base; v1.17 fixes to be cherry-picked manually later.
- **Transparency key**: WinBolo uses pure lime green `RGB(0,255,0)` as its color key. Do NOT detect it from the top-left pixel of tiles.bmp (that pixel is brownish terrain, not background).
- **Shell sprites span tile rows**: shellY=79, height=4 → spans rows 4/5 (y=79–82 crosses 80=5×16). Must use `g_icons` (unpadded), NOT the padded atlas.
- **Status icons span tile columns**: STATUS_BASE_GOOD_X=440+12=452 > 28×16=448. Must use `g_icons`.
- **x64 pointer truncation trap**: if MSVC warns C4047 "differs in levels of indirection from 'int'", a pointer-returning function has no forward declaration. The implicit `int` truncates the 64-bit pointer. Fix: add the declaration to the appropriate header.
- **Solo-play init requirements** (in `boloInit`):
  - `screenSetInStartFind(FALSE)` — clears flag that blocks `frontEndDrawMainScreen`
  - `screenSetGunsight(TRUE)` — gunsight defaults off
  - `screenSetAutoScroll(TRUE)` — centers viewport on gunsight
  - `netSetType(netSingle)` — enables base/pill capture in `tank.c:1503`
  - `screenForceStatusUpdate()` — pushes initial pill/base/tank/bar states to HUD

---

## Directory Structure (new-project/)

```
new-project/
├── PLAN.md                      ← this file
├── CMakeLists.txt               ← root: project setup, both targets
├── include/
│   ├── winbolo_platform.h       ← force-included: winsock2/windows/mmsystem order
│   ├── brain.h                  ← fixed copy: push/pop pragmas
│   ├── network.h                ← wrapper: reset pack after bad pop,name,1
│   └── servernet.h              ← wrapper: same fix
├── server/
│   ├── CMakeLists.txt           ← server executable (no graphics)
│   └── win32/
│       └── preferences_stub.c
└── client/
    ├── CMakeLists.txt           ← client executable (Raylib 5.5)
    ├── main.c                   ← Raylib window + game loop (OBJECT lib, no /FI)
    ├── game_loop.h              ← raylib-safe bridge API (boloInit/Tick/Update)
    ├── game_loop.c              ← bridge impl compiled WITH /FI
    ├── frontend_raylib.c        ← full frontend impl (B2-B3: tile grid, HUD, status, man)
    ├── render_bridge.c/.h       ← Raylib draw bridge (compiled without /FI): tiles, sprites, HUD primitives
    ├── game_loop.c/.h           ← bolo engine bridge (compiled with /FI): boloInit/Tick/Update
    ├── positions.h              ← HUD layout constants (zoom=1 coords)
    ├── preferences_stub.c       ← Windows INI path helper
    └── win32stubs.c             ← stubs for DirectX/WinMain symbols
```
