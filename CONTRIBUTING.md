# Contributing to OpenBolo

Thank you for your interest in contributing! This document covers how to build
the project, the coding conventions we follow, and how to submit changes.

## Getting Started

### Prerequisites

- **CMake** 3.20 or later
- **Windows**: MSVC 2019+ (Visual Studio Community is free)
- **Linux**: GCC or Clang, SDL (for server mutex), and a Raylib-compatible
  OpenGL driver (client Linux support is in progress — see Phase B7 in
  [PLAN.md](PLAN.md))
- An internet connection on first build (CMake fetches Raylib 5.5 via
  FetchContent)

### Building

```sh
# Configure
cmake -B build

# Build the client
cmake --build build --config Debug --target winbolo-client

# Build the server
cmake --build build --config Debug --target winbolo-server
```

The client executable and its assets (`tiles.bmp`, `background.bmp`, `sounds/`)
are placed in `build/client/Debug/` automatically by post-build rules.

### Running

```
build/client/Debug/winbolo-client.exe
```

Enter a player name at the startup screen (Enter to confirm, ESC for default).
The game loads `Everard Island.map` automatically for solo play.

## Code Style

The codebase mixes the original WinBolo C89/C99 style with our new C11 files.
Please follow these rules:

- **Indentation**: 2 spaces (no tabs) in new files — matches the original
  WinBolo style
- **Line endings**: LF (`\n`) — `.editorconfig` enforces this
- **Line length**: aim for 80 columns; hard limit 120
- **Comments**: block comments `/* ... */` for file/function headers;
  `//` for inline notes
- **Naming**: follow the existing convention — `camelCase` for functions and
  variables, `ALL_CAPS` for constants and macros
- **No `<stdbool.h>`**: `global.h` defines `typedef BYTE bool`. Using
  `<stdbool.h>` anywhere causes a redefinition conflict.
- **Header ordering**: always `#include "winbolo_platform.h"` (or force-include
  via `/FI`) before any other Windows headers. Files that include `raylib.h`
  must be compiled without `/FI` — see `client/CMakeLists.txt` for examples.

## What to Work On

See [PLAN.md](PLAN.md) for the full roadmap. Good first areas:

- **Phase B7** — Linux build: fix `#ifdef _WIN32` gaps, add Linux CMake support
- **Phase C** — Networking: IPv6 support in `servertransport.c`, buffer safety
  audit of `udppackets.c`
- **Phase D** — Code quality: unit tests for pure logic modules (`crc.c`,
  `bolo_map.c`, etc.)

## Submitting Changes

1. Fork the repository and create a branch from `main`:
   ```sh
   git checkout -b feature/my-change
   ```
2. Make your changes. Keep commits focused — one logical change per commit.
3. Ensure the project still builds cleanly with no new warnings.
4. Open a merge request against `main` with a clear description of what changed
   and why.

## Modifying Original Source Files

The files under `src/` are the original WinBolo 1.15 source. When modifying
them:

- Preserve the original copyright header at the top of the file.
- Add a note in the comment block describing what was changed and why.
- Keep changes minimal — prefer adding to `client/` over modifying `src/` where
  possible.

## Questions

Open an issue if you're unsure about anything. There are no silly questions.
