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
 * render_bridge.c — Raylib tile/HUD rendering implementation.
 *
 * Compiled WITHOUT /FI winbolo_platform.h to avoid the
 * raylib.h / wingdi.h::Rectangle() clash.
 *
 * Zoom model: all public API takes zoom=1 coordinates.
 * Internally every coordinate and size is multiplied by g_zoom before
 * being passed to Raylib, so the window can run at any integer scale.
 *
 * Tile sheet strategy (two GPU textures from one BMP):
 *   g_tiles — padded atlas (each 16×16 slot gets 1px edge extrusion),
 *             TEXTURE_FILTER_BILINEAR.  Used by renderTile / renderMine.
 *             Prevents bilinear from bleeding into adjacent tiles.
 *   g_icons — original sheet with alpha-dilated transparent pixels,
 *             TEXTURE_FILTER_BILINEAR.  Used by renderSprite / renderStatusIcon.
 *             These callers have sprites that span tile-column/row boundaries
 *             so they cannot use the padded atlas safely.
 *
 * Alpha dilation: before building either texture, transparent pixels
 * (formerly the green key color) have their RGB filled from the nearest
 * opaque neighbour.  This prevents bilinear from bleeding black at
 * sprite edges.
 */

#include <math.h>
#include <stdio.h>    /* snprintf */
#include <stdlib.h>   /* calloc, free */
#include "raylib.h"
#include "render_bridge.h"

/* ---- Globals --------------------------------------------- */
static int       g_zoom        = 2;   /* default: 2x (1030x650 window) */

static Texture2D g_tiles;             /* padded atlas — renderTile / renderMine  */
static int       g_tilesLoaded = 0;

static Texture2D g_icons;             /* original sheet — renderSprite / renderStatusIcon */
static int       g_iconsLoaded = 0;

static Texture2D g_bg;
static int       g_bgLoaded    = 0;

/* Tile/icon dimensions (zoom=1) */
#define TILE_W   16
#define TILE_H   16
#define ICON_W   12
#define ICON_H   12

/* Mine overlay source coords in original (non-padded) sheet */
#define MINE_SRC_X (19 * TILE_W)
#define MINE_SRC_Y (3  * TILE_H)

/* Original chrome size (matches background.bmp) */
#define CHROME_W 515
#define CHROME_H 325

/* ---- Padded atlas layout --------------------------------- */
/* tiles.bmp is 496×160 = 31 columns × 10 rows of 16×16 tiles. */
#define TILE_COLS    31
#define TILE_ROWS    10
#define PAD          1                          /* 1px border each side */
#define PAD_SLOT_W   (TILE_W + 2 * PAD)        /* 18 */
#define PAD_SLOT_H   (TILE_H + 2 * PAD)        /* 18 */
#define PAD_ATLAS_W  (TILE_COLS * PAD_SLOT_W)  /* 558 */
#define PAD_ATLAS_H  (TILE_ROWS * PAD_SLOT_H)  /* 180 */

/*
 * Convert original pixel coordinate → padded-atlas coordinate.
 * col = x / TILE_W,  inner_offset = x % TILE_W
 * padded_x = col * PAD_SLOT_W + PAD + inner_offset
 *
 * This works for any x that lies inside a single tile column.
 * Do NOT use for sprites that span tile boundaries.
 */
#define TO_PAD_X(x)  ((x) / TILE_W * PAD_SLOT_W + PAD + (x) % TILE_W)
#define TO_PAD_Y(y)  ((y) / TILE_H * PAD_SLOT_H + PAD + (y) % TILE_H)

/* ---- Static helpers -------------------------------------- */

/*
 * Alpha-dilate: for each transparent pixel (alpha==0), fill its RGB
 * from the average of its opaque 4-connected neighbours.  Alpha stays 0.
 * Two passes to reach diagonal corners.
 * Prevents bilinear from bleeding black at sprite/terrain edges.
 */
static void alphaDilate(unsigned char *rgba, int w, int h)
{
    static const int dx[4] = {-1, 1, 0, 0};
    static const int dy[4] = { 0, 0,-1, 1};
    int pass, y, x, d;
    for (pass = 0; pass < 2; pass++) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                unsigned char *p = rgba + (y * w + x) * 4;
                if (p[3] != 0) continue;          /* opaque — skip */
                int r = 0, g = 0, b = 0, n = 0;
                for (d = 0; d < 4; d++) {
                    int nx = x + dx[d], ny = y + dy[d];
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
                    unsigned char *q = rgba + (ny * w + nx) * 4;
                    if (q[3] > 0) { r += q[0]; g += q[1]; b += q[2]; n++; }
                }
                if (n) {
                    p[0] = (unsigned char)(r / n);
                    p[1] = (unsigned char)(g / n);
                    p[2] = (unsigned char)(b / n);
                }
            }
        }
    }
}

/*
 * Build a padded atlas: each TILE_W×TILE_H tile gets PAD pixels of
 * edge extrusion on all four sides (corners included).
 * Returns a calloc'd RGBA8 buffer of PAD_ATLAS_W×PAD_ATLAS_H×4 bytes.
 * Caller must free().
 */
static unsigned char *buildPaddedAtlas(const unsigned char *src, int sw, int sh)
{
    unsigned char *dst =
        (unsigned char *)calloc((size_t)PAD_ATLAS_W * PAD_ATLAS_H * 4, 1);
    if (!dst) return NULL;

#define SRC(px,py)  (src + ((py) * sw + (px)) * 4)
#define DST(px,py)  (dst + ((py) * PAD_ATLAS_W + (px)) * 4)
#define COPY4(d,s)  do { \
    (d)[0]=(s)[0]; (d)[1]=(s)[1]; (d)[2]=(s)[2]; (d)[3]=(s)[3]; \
} while(0)

    int row, col, tx, ty;
    for (row = 0; row < TILE_ROWS; row++) {
        for (col = 0; col < TILE_COLS; col++) {
            int sx0 = col * TILE_W, sy0 = row * TILE_H;  /* source origin */
            int dx0 = col * PAD_SLOT_W + PAD;             /* dest inner X  */
            int dy0 = row * PAD_SLOT_H + PAD;             /* dest inner Y  */

            /* Interior */
            for (ty = 0; ty < TILE_H; ty++)
                for (tx = 0; tx < TILE_W; tx++)
                    COPY4(DST(dx0+tx, dy0+ty), SRC(sx0+tx, sy0+ty));

            /* Edge extrusions */
            for (ty = 0; ty < TILE_H; ty++) {
                COPY4(DST(dx0-1,      dy0+ty), SRC(sx0,          sy0+ty)); /* left  */
                COPY4(DST(dx0+TILE_W, dy0+ty), SRC(sx0+TILE_W-1, sy0+ty)); /* right */
            }
            for (tx = 0; tx < TILE_W; tx++) {
                COPY4(DST(dx0+tx, dy0-1),      SRC(sx0+tx, sy0));           /* top    */
                COPY4(DST(dx0+tx, dy0+TILE_H), SRC(sx0+tx, sy0+TILE_H-1)); /* bottom */
            }

            /* Corner extrusions */
            COPY4(DST(dx0-1,      dy0-1),      SRC(sx0,          sy0));          /* TL */
            COPY4(DST(dx0+TILE_W, dy0-1),      SRC(sx0+TILE_W-1, sy0));          /* TR */
            COPY4(DST(dx0-1,      dy0+TILE_H), SRC(sx0,          sy0+TILE_H-1)); /* BL */
            COPY4(DST(dx0+TILE_W, dy0+TILE_H), SRC(sx0+TILE_W-1, sy0+TILE_H-1));/* BR */
        }
    }

#undef SRC
#undef DST
#undef COPY4
    return dst;
}

/* ---- Zoom ------------------------------------------------ */
void renderSetZoom(int zoom) { if (zoom >= 1) g_zoom = zoom; }
int  renderGetZoom(void)     { return g_zoom; }

/* ---- Tile sheet ------------------------------------------ */
void renderLoadTiles(const char *tilesPath)
{
    if (g_tilesLoaded) return;

    Image img = LoadImage(tilesPath);
    if (img.data == NULL) return;

    /* Ensure RGBA8 layout for direct pixel access */
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    /* Remove WinBolo green transparency key */
    Color key = {0, 255, 0, 255};
    ImageColorReplace(&img, key, BLANK);

    /* Alpha-dilate: fill transparent pixel RGB from nearest opaque neighbour
     * so bilinear blends to the correct edge colour rather than black. */
    alphaDilate((unsigned char *)img.data, img.width, img.height);

    /* g_icons: dilated original sheet.
     * Used for sprites/icons that span tile boundaries. */
    g_icons = LoadTextureFromImage(img);
    g_iconsLoaded = (g_icons.id != 0);
    if (g_iconsLoaded) SetTextureFilter(g_icons, TEXTURE_FILTER_BILINEAR);

    /* Build padded atlas from the same dilated data */
    unsigned char *padded = buildPaddedAtlas(
        (const unsigned char *)img.data, img.width, img.height);
    UnloadImage(img);

    if (padded) {
        Image padImg;
        padImg.data    = padded;
        padImg.width   = PAD_ATLAS_W;
        padImg.height  = PAD_ATLAS_H;
        padImg.mipmaps = 1;
        padImg.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        g_tiles = LoadTextureFromImage(padImg);
        free(padded);
        /* Point filter keeps terrain tiles crisp; padded atlas still prevents
         * bleed if zoom is later changed to a non-integer value. */
        if (g_tiles.id != 0) SetTextureFilter(g_tiles, TEXTURE_FILTER_POINT);
    }
    g_tilesLoaded = (g_tiles.id != 0);
}

void renderUnloadTiles(void)
{
    if (g_tilesLoaded) { UnloadTexture(g_tiles); g_tilesLoaded = 0; }
    if (g_iconsLoaded) { UnloadTexture(g_icons); g_iconsLoaded = 0; }
}

/*
 * Draw one 16×16 tile from the padded atlas.
 * srcX/srcY are original-sheet coordinates (multiples of 16).
 */
void renderTile(int srcX, int srcY, int dstX, int dstY)
{
    if (!g_tilesLoaded) return;
    Rectangle src = { (float)TO_PAD_X(srcX), (float)TO_PAD_Y(srcY),
                      (float)TILE_W, (float)TILE_H };
    Rectangle dst = { (float)(dstX * g_zoom), (float)(dstY * g_zoom),
                      (float)(TILE_W * g_zoom), (float)(TILE_H * g_zoom) };
    DrawTexturePro(g_tiles, src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

/*
 * Draw an arbitrary srcW×srcH sprite from the original (unpadded) sheet.
 * Used for shell/bullet sprites which may cross tile-row boundaries.
 */
void renderSprite(int srcX, int srcY, int srcW, int srcH, int dstX, int dstY)
{
    if (!g_iconsLoaded) return;
    Rectangle src = { (float)srcX, (float)srcY, (float)srcW, (float)srcH };
    Rectangle dst = { (float)(dstX * g_zoom), (float)(dstY * g_zoom),
                      (float)(srcW * g_zoom), (float)(srcH * g_zoom) };
    DrawTexturePro(g_icons, src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

/*
 * Draw a 12×12 status icon from the original (unpadded) sheet.
 * Icons land at non-16-aligned positions and span tile-column boundaries.
 */
void renderStatusIcon(int srcX, int srcY, int dstX, int dstY)
{
    if (!g_iconsLoaded) return;
    Rectangle src = { (float)srcX, (float)srcY, (float)ICON_W, (float)ICON_H };
    Rectangle dst = { (float)(dstX * g_zoom), (float)(dstY * g_zoom),
                      (float)(ICON_W * g_zoom), (float)(ICON_H * g_zoom) };
    DrawTexturePro(g_icons, src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

/* Draw the mine overlay from the padded atlas. */
void renderMine(int dstX, int dstY)
{
    if (!g_tilesLoaded) return;
    Rectangle src = { (float)TO_PAD_X(MINE_SRC_X), (float)TO_PAD_Y(MINE_SRC_Y),
                      (float)TILE_W, (float)TILE_H };
    Rectangle dst = { (float)(dstX * g_zoom), (float)(dstY * g_zoom),
                      (float)(TILE_W * g_zoom), (float)(TILE_H * g_zoom) };
    DrawTexturePro(g_tiles, src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

/* ---- Background chrome ----------------------------------- */
void renderLoadBackground(const char *bgPath)
{
    if (g_bgLoaded) return;
    g_bg = LoadTexture(bgPath);
    g_bgLoaded = (g_bg.id != 0);
    if (g_bgLoaded) SetTextureFilter(g_bg, TEXTURE_FILTER_BILINEAR);
}

void renderUnloadBackground(void)
{
    if (g_bgLoaded) { UnloadTexture(g_bg); g_bgLoaded = 0; }
}

void renderDrawBackground(void)
{
    if (!g_bgLoaded) return;
    Rectangle src = { 0, 0, (float)CHROME_W, (float)CHROME_H };
    Rectangle dst = { 0, 0, (float)(CHROME_W * g_zoom), (float)(CHROME_H * g_zoom) };
    DrawTexturePro(g_bg, src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

/* ---- HUD primitives ------------------------------------- */
void renderDrawBar(int x, int y, int w, int h,
                   unsigned char r, unsigned char g, unsigned char b)
{
    DrawRectangle(x * g_zoom, y * g_zoom,
                  w * g_zoom, h * g_zoom,
                  (Color){r, g, b, 255});
}

void renderDrawText(int x, int y, int fontSize, const char *text,
                    unsigned char r, unsigned char gr, unsigned char b)
{
    DrawText(text, x * g_zoom, y * g_zoom, fontSize * g_zoom,
             (Color){r, gr, b, 255});
}

void renderDrawManStatus(int cx, int cy, int isDead, int inTank, float angleDeg)
{
    int   zcx    = cx * g_zoom;
    int   zcy    = cy * g_zoom;
    int   radius = 20 * g_zoom;
    Color white  = {255, 255, 255, 255};
    Color yellow = {255, 220,   0, 255};

    /* Outline circle */
    DrawCircleLines(zcx, zcy, (float)radius, white);

    if (inTank) {
        /* Man is inside tank — just the empty circle */
        return;
    }

    if (isDead) {
        /* Dead: draw an X inside the circle */
        int d = (int)(radius * 0.6f);
        DrawLine(zcx - d, zcy - d, zcx + d, zcy + d, yellow);
        DrawLine(zcx + d, zcy - d, zcx - d, zcy + d, yellow);
    } else {
        /* Direction arrow: angleDeg 0=up, clockwise.
         * Convert to standard math angle: 0=right, counter-clockwise. */
        float rad = (float)((angleDeg - 90.0) * 3.14159265 / 180.0);
        int   len = (int)(radius * 0.85f);
        int   ex  = zcx + (int)(len * cos(rad));
        int   ey  = zcy + (int)(len * sin(rad));
        DrawLine(zcx, zcy, ex, ey, yellow);
        /* Arrowhead dot */
        DrawCircle(ex, ey, (float)(2 * g_zoom), yellow);
    }
}

/* ---- Audio ------------------------------------------------ */
/*
 * Maps sndEffects enum values (0-23) to WAV filenames.
 * Order must match the sndEffects enum in backend.h exactly.
 */
#define SND_COUNT 24
static Sound       g_sounds[SND_COUNT];
static int         g_soundsLoaded = 0;

static const char *k_soundFiles[SND_COUNT] = {
    "shooting_self.wav",       /* 0  shootSelf          */
    "shooting_near.wav",       /* 1  shootNear          */
    "shot_tree_near.wav",      /* 2  shotTreeNear       */
    "shot_tree_far.wav",       /* 3  shotTreeFar        */
    "shot_building_near.wav",  /* 4  shotBuildingNear   */
    "shot_building_far.wav",   /* 5  shotBuildingFar    */
    "hit_tank_near.wav",       /* 6  hitTankNear        */
    "hit_tank_far.wav",        /* 7  hitTankFar         */
    "hit_tank_self.wav",       /* 8  hitTankSelf        */
    "bubbles.wav",             /* 9  bubbles            */
    "tank_sinking_near.wav",   /* 10 tankSinkNear       */
    "tank_sinking_far.wav",    /* 11 tankSinkFar        */
    "big_explosion_near.wav",  /* 12 bigExplosionNear   */
    "big_explosion_far.wav",   /* 13 bigExplosionFar    */
    "farming_tree_near.wav",   /* 14 farmingTreeNear    */
    "farming_tree_far.wav",    /* 15 farmingTreeFar     */
    "man_building_near.wav",   /* 16 manBuildingNear    */
    "man_building_far.wav",    /* 17 manBuildingFar     */
    "man_dying_near.wav",      /* 18 manDyingNear       */
    "man_dying_far.wav",       /* 19 manDyingFar        */
    "man_lay_mine_near.wav",   /* 20 manLayingMineNear  */
    "mine_explosion_near.wav", /* 21 mineExplosionNear  */
    "mine_explosion_far.wav",  /* 22 mineExplosionFar   */
    "shooting_far.wav"         /* 23 shootFar           */
};

void renderLoadSounds(const char *soundsDir)
{
    char path[512];
    int i;
    if (g_soundsLoaded) return;
    for (i = 0; i < SND_COUNT; i++) {
        snprintf(path, sizeof(path), "%s/%s", soundsDir, k_soundFiles[i]);
        g_sounds[i] = LoadSound(path);
    }
    g_soundsLoaded = 1;
}

void renderUnloadSounds(void)
{
    int i;
    if (!g_soundsLoaded) return;
    for (i = 0; i < SND_COUNT; i++)
        UnloadSound(g_sounds[i]);
    g_soundsLoaded = 0;
}

void renderPlaySound(int effectIndex)
{
    if (!g_soundsLoaded) return;
    if (effectIndex < 0 || effectIndex >= SND_COUNT) return;
    PlaySound(g_sounds[effectIndex]);
}