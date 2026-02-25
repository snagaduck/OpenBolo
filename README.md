# OpenBolo

Cross-platform remake of WinBolo (1998) using Raylib and CMake.

## About

WinBolo is a multiplayer tank game originally written by John Morrison in 1998,
itself based on Stuart Cheshire's classic *Bolo* (1987) for the Macintosh.
The original source was released under the GPL in 2008.

OpenBolo modernizes WinBolo 1.15 with a CMake build system and a Raylib
rendering/audio layer, replacing the original DirectX 5 and Win32 GDI
dependencies. The game engine (physics, networking, map format, protocol) is
preserved verbatim so OpenBolo remains compatible with existing WinBolo maps
and, eventually, original WinBolo clients.

## Status

| Phase | Description | Status |
|-------|-------------|--------|
| A — Compile | CMake build, MSVC, server + client targets | ✅ Complete |
| B — Platform layer | Raylib rendering, audio, input, HUD, player name UI | ✅ Complete |
| C — Networking | IPv6, TLS, GameNetworkingSockets evaluation | Planned |
| D — Code quality | Tests, warnings, platform abstraction cleanup | Planned |
| E — Features | Widescreen, WinBoloNet replacement, v1.17 fixes | Planned |

The game is currently playable in single-player mode: drive, shoot, capture
pillboxes and bases, deploy LGMs, and hear sound effects — all via the keyboard.

## Building

### Requirements

- CMake 3.20+
- A C11 compiler (MSVC 2019+, GCC, or Clang)
- Windows 7+ or Linux (Windows is the primary target for now)
- An internet connection on first build (CMake fetches Raylib 5.5 via FetchContent)

### Windows (MSVC)

```
cmake -B build
cmake --build build --config Debug --target winbolo-client
build\client\Debug\winbolo-client.exe
```

Or open the folder in Visual Studio 2019+ and use the CMake integration.

### Linux

Linux support is planned (Phase B7). The server target already has POSIX
socket and SDL mutex stubs; the client Raylib layer needs a Linux CMake pass.

## Controls

| Key | Action |
|-----|--------|
| Up / W | Accelerate |
| Down / S | Decelerate |
| Left / A | Turn anti-clockwise |
| Right / D | Turn clockwise |
| Space | Fire shell |
| Tab | Quick-drop mine (visible to nearby tanks) |
| 1 – 5 | Select LGM build mode (Farm · Road · Building · Pillbox · Mine) |
| B | Send LGM to gunsight tile with selected mode |
| [ / ] | Increase / decrease gunsight range |
| ; | Cycle pillbox view (owned pillboxes) |
| Return | Return to tank view |

> The tank has momentum — releasing accelerate does not stop it instantly.
> Use decelerate to brake, and plan your approach to avoid coasting into mines.

See [docs/MANUAL.md](docs/MANUAL.md) for full gameplay documentation.

## Project Layout

```
OpenBolo/
├── CMakeLists.txt          — root build (server + client)
├── include/                — platform compat headers (force-included)
│   ├── winbolo_platform.h  — winsock2/windows/mmsystem include order fix
│   ├── brain.h             — fixed pragma pack (push/pop)
│   ├── network.h           — wrapper: reset pack after bad pop,name,1
│   └── servernet.h         — same fix
├── src/                    — original WinBolo 1.15 source (GPL)
│   ├── bolo/               — game engine (physics, maps, tanks, bullets…)
│   ├── server/             — dedicated server + embedded server core
│   ├── zlib/               — embedded zlib
│   ├── lzw/                — embedded LZW
│   ├── winbolonet/         — WinBoloNet tracker HTTP client
│   ├── gui/win32/          — original Win32 GUI helpers + assets (tiles.bmp)
│   └── bsd/                — BSD string compat headers
├── client/                 — OpenBolo Raylib platform layer (new)
│   ├── main.c              — Raylib window, game loop, player name screen
│   ├── game_loop.c/.h      — bridge: bolo engine ↔ main.c (no Win32 types)
│   ├── frontend_raylib.c   — implements all frontEnd* callbacks (HUD, tiles)
│   ├── render_bridge.c/.h  — Raylib draw calls (compiled without /FI)
│   ├── positions.h         — HUD layout constants (zoom=1 pixel coords)
│   ├── win32stubs.c        — stubs for excluded DirectX/WinMain symbols
│   └── preferences_stub.c  — Windows INI path helper
├── server/                 — standalone server CMake config
└── sounds/                 — 24 WAV sound effects
```

## Architecture Notes

**Two-texture render pipeline**: `render_bridge.c` builds two GPU textures from
`tiles.bmp` — a padded atlas (`g_tiles`, point-filtered) for terrain tiles, and a
dilated copy (`g_icons`, bilinear) for sprites and HUD icons that cross tile
boundaries.

**Rectangle clash workaround**: `wingdi.h` declares `Rectangle()` as a GDI
function; Raylib defines `Rectangle` as a struct. Files that include `raylib.h`
(`main.c`, `render_bridge.c`) are compiled as separate OBJECT libraries without
the `/FI winbolo_platform.h` force-include.

**Embedded server**: WinBolo clients always host their own server instance.
`servercore.c`, `servernet.c`, and friends are compiled into the client
executable.

## Credits

- **WinBolo / LinBolo** — John Morrison, 1998–2008 (GPL v2+) — [winbolo.com](http://www.winbolo.com/) · [winbolo.net](http://www.winbolo.net/)
- **Bolo** — [Stuart Cheshire](https://github.com/StuartCheshire), 1987–1995 — the original multiplayer tank game for the Macintosh that WinBolo is based on ([Wikipedia](https://en.wikipedia.org/wiki/Bolo_(1987_video_game)))
- **Raylib** — Ramon Santamaria ([@raysan5](https://github.com/raysan5)), zlib license

## License

OpenBolo is distributed under the GNU General Public License v3.0.
See [LICENSE](LICENSE) for the full text.

This project is a derivative of WinBolo, which is licensed under GPL v2 or later
(Copyright © 1998–2008 John Morrison). GPLv3 is compatible with "GPLv2 or later".
