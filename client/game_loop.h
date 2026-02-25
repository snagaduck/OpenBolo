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

#endif /* GAME_LOOP_H */