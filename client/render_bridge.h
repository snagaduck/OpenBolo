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
 * render_bridge.h — Raylib tile/HUD rendering bridge.
 *
 * Compiled WITHOUT /FI winbolo_platform.h to avoid the
 * raylib.h / wingdi.h::Rectangle() clash.
 *
 * All parameters use only plain C types so this header is safe to
 * include from files compiled WITH /FI (frontend_raylib.c, game_loop.c).
 *
 * Zoom scaling: all dst coordinates and sizes are passed in zoom=1 space
 * (original positions.h values).  render_bridge multiplies by the current
 * zoom factor internally, so callers never need to know about zoom.
 */

#ifndef RENDER_BRIDGE_H
#define RENDER_BRIDGE_H

/* ---- Zoom ------------------------------------------------ */
/* Set/get the integer zoom factor (default 2).
 * Call renderSetZoom() once before any draw calls.
 * Window should be (515 * zoom) x (325 * zoom). */
void renderSetZoom(int zoom);
int  renderGetZoom(void);

/* ---- Tile sheet ------------------------------------------ */
void renderLoadTiles(const char *tilesPath);
void renderUnloadTiles(void);

/* Draw one 16x16 tile from the sheet (zoom=1 coords). */
void renderTile(int srcX, int srcY, int dstX, int dstY);

/* Draw an arbitrary srcW x srcH sub-tile sprite (zoom=1 coords).
 * Used for shell projectiles which are 3-4 px wide/tall. */
void renderSprite(int srcX, int srcY, int srcW, int srcH, int dstX, int dstY);

/* Draw a 12x12 status icon from the sheet (zoom=1 coords). */
void renderStatusIcon(int srcX, int srcY, int dstX, int dstY);

/* Draw the mine overlay tile (zoom=1 coords). */
void renderMine(int dstX, int dstY);

/* ---- Background chrome ----------------------------------- */
void renderLoadBackground(const char *bgPath);
void renderUnloadBackground(void);
/* Draw the 515x325 chrome background stretched to (515*zoom)x(325*zoom). */
void renderDrawBackground(void);

/* ---- HUD primitives ------------------------------------- */

/* Filled rectangle (zoom=1 coords + size, RGBA 0-255). */
void renderDrawBar(int x, int y, int w, int h,
                   unsigned char r, unsigned char g, unsigned char b);

/* Text string at zoom=1 position; fontSize is zoom=1 pixels tall. */
void renderDrawText(int x, int y, int fontSize, const char *text,
                    unsigned char r, unsigned char g, unsigned char b);

/* LGM man-status compass at zoom=1 center (cx,cy), radius=20.
 *   inTank  != 0 → draw empty circle (man inside tank)
 *   isDead  != 0 → draw X
 *   otherwise    → draw arrow at angleDeg (0=up, clockwise) */
void renderDrawManStatus(int cx, int cy, int isDead, int inTank, float angleDeg);

/* ---- Audio ----------------------------------------------- */
/* Load all 24 WAV sound effects from the given directory.
 * Call once after InitAudioDevice(), before the game loop. */
void renderLoadSounds(const char *soundsDir);

/* Unload all sound effects (call before CloseAudioDevice). */
void renderUnloadSounds(void);

/* Play one sound effect by its sndEffects enum index (cast to int). */
void renderPlaySound(int effectIndex);

#endif /* RENDER_BRIDGE_H */