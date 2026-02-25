/*
 * OpenBolo — cross-platform remake of WinBolo (1998)
 * Copyright (C) 2026 OpenBolo Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * game_loop.h — Raylib-safe bridge between main.c and the bolo game engine.
 *
 * main.c is compiled WITHOUT /FI winbolo_platform.h to dodge the
 * raylib.h / wingdi.h Rectangle clash.  This header exposes the three
 * entry points main.c needs using only C standard types — no Windows
 * headers, no bolo typedefs.
 */

#ifndef GAME_LOOP_H
#define GAME_LOOP_H

/* Append a diagnostic line to net_debug.log — lets main.c log without
 * pulling in bolo or Windows headers. */
void boloLogMsg(const char *msg);

/* Returns 1 on success, 0 if the map file could not be loaded.
 * playerName is stored for gameFrontGetPlayerName() calls and passed
 * to the game engine.  Pass NULL or "" to use the default "Player". */
int  boloInit(const char *mapFile, const char *playerName);

/* Run one 20 Hz game-logic tick.
 *   tankButtonOrdinal: 0=TNONE 1=TLEFT 2=TRIGHT 3=TACCEL 4=TDECEL
 *                      5=TLEFTACCEL 6=TRIGHTACCEL 7=TLEFTDECEL 8=TRIGHTDECEL
 *   shoot: 1 if fire key is down, 0 otherwise */
void boloTick(int tankButtonOrdinal, int shoot);

/* Trigger a screen update (wraps screenUpdate → frontEndDrawMainScreen). */
void boloUpdate(void);

/* Adjust gunsight range. increase=1 lengthens, increase=0 shortens. */
void boloGunsightRange(int increase);

/* Send the LGM (engineer) to the current gunsight tile.
 * buildType: 0=BsTrees 1=BsRoad 2=BsBuilding 3=BsPillbox 4=BsMine
 * Use 0 for base capture — LGM auto-captures any base he reaches. */
void boloManMove(int buildType);

/* Quick-drop a mine under the tank (Tab key).
 * The mine is visible to all nearby tanks. */
void boloLayMine(void);

/* Cycle to the next owned pillbox view (semicolon key).
 * Has no effect if you own no pillboxes. */
void boloPillView(void);

/* Return the viewport to the player's own tank (Return key). */
void boloTankView(void);

/* Navigate between adjacent pillboxes while in pill view.
 * horz/vert: -1 = left/up, 0 = no change, 1 = right/down. */
void boloPillViewNav(int horz, int vert);

/* Returns 1 if currently in pillbox view, 0 if in tank view. */
int boloInPillView(void);

/* ── Phase C: ENet multiplayer ──────────────────────────────────────────── */

/* Host a new game.  Loads mapFile into the embedded server, binds the
 * server to port, and joins as the first client.  Call before the game loop;
 * poll boloNetStatus() until BOLO_NET_RUNNING, then call boloNetPostConnect().
 * Returns 1 on success, 0 on failure. */
int  boloHost(const char *mapFile, unsigned short port, const char *playerName);

/* Join an existing game at ip:port.  Call before the game loop; then poll
 * boloNetStatus() each frame until it returns BOLO_NET_RUNNING or
 * BOLO_NET_FAILED. */
int  boloJoin(const char *ip, unsigned short port, const char *playerName);

/* Returns the current network status as one of the BOLO_NET_* constants. */
int  boloNetStatus(void);

/* Pump ENet events without drawing.  Safe to call outside BeginDrawing/EndDrawing.
 * Use this in connecting/loading screens that need network progress but must not
 * trigger game rendering. */
void boloNetPoll(void);

/* Apply post-download client settings (gunsight, auto-scroll, HUD status).
 * Call once after connectingScreen() returns 1 (BOLO_NET_RUNNING). */
void boloNetPostConnect(void);

/* Values returned by boloNetStatus() — mirrors the engine's netStatus enum. */
#define BOLO_NET_JOINING         0
#define BOLO_NET_RUNNING         1
#define BOLO_NET_START_DOWNLOAD  2
#define BOLO_NET_BASE_DOWNLOAD   3
#define BOLO_NET_PILL_DOWNLOAD   4
#define BOLO_NET_MAP_DOWNLOAD    5
#define BOLO_NET_TIME_DOWNLOAD   6
#define BOLO_NET_FAILED          7

#endif /* GAME_LOOP_H */