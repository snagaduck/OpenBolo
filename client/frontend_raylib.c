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
 * frontend_raylib.c — Phase B3 frontend implementation.
 *
 * Compiled WITH /FI winbolo_platform.h — full bolo type visibility.
 * Raylib drawing is delegated to render_bridge.c (compiled without /FI)
 * to avoid the wingdi.h::Rectangle() / raylib.h::Rectangle clash.
 *
 * Phase B2: tile grid rendering.
 * Phase B3: HUD state storage + status icons, bars, messages, man status.
 *
 * Zoom model: all coordinates passed to render_bridge use zoom=1 values;
 * render_bridge multiplies by its internal zoom factor automatically.
 */

/* Guard against IDB_TILES being undefined — we use _X/_Y coords only. */
#ifndef IDB_TILES
#  define IDB_TILES 0
#endif

#include <string.h>

/* MESSAGE_STRING_SIZE is defined in draw.h which pulls in <ddraw.h>.
 * Redeclare the value directly to avoid DirectX headers. */
#ifndef MESSAGE_STRING_SIZE
#  define MESSAGE_STRING_SIZE 68
#endif

#include "frontend.h"       /* all frontEnd* prototypes + bolo types */
#include "backend.h"        /* screenGetPos, screenIsMine, MAIN_SCREEN_SIZE_* */
#include "tilenum.h"        /* tile-type numeric IDs */
#include "tiles.h"          /* _X/_Y pixel offsets in tiles.bmp */
#include "positions.h"      /* MAIN_OFFSET_*, STATUS_*, MESSAGE_*, MAN_STATUS_* */
#include "render_bridge.h"

/* ------------------------------------------------------------------ */
/* Tile source-rect lookup table (B2)                                  */
/* Maps tile-type byte (0-254) → (srcX, srcY) in tiles.bmp.           */
/* ------------------------------------------------------------------ */
static int  tileSrcX[256];
static int  tileSrcY[256];
static int  tilesReady = 0;

static void initTileLookup(void)
{
    int i;
    for (i = 0; i < 256; i++) { tileSrcX[i] = 0; tileSrcY[i] = 0; }

    /* Deep sea */
    tileSrcX[DEEP_SEA_SOLID]  = DEEP_SEA_SOLID_X;  tileSrcY[DEEP_SEA_SOLID]  = DEEP_SEA_SOLID_Y;
    tileSrcX[DEEP_SEA_CORN1]  = DEEP_SEA_CORN1_X;  tileSrcY[DEEP_SEA_CORN1]  = DEEP_SEA_CORN1_Y;
    tileSrcX[DEEP_SEA_CORN2]  = DEEP_SEA_CORN2_X;  tileSrcY[DEEP_SEA_CORN2]  = DEEP_SEA_CORN2_Y;
    tileSrcX[DEEP_SEA_CORN3]  = DEEP_SEA_CORN3_X;  tileSrcY[DEEP_SEA_CORN3]  = DEEP_SEA_CORN3_Y;
    tileSrcX[DEEP_SEA_CORN4]  = DEEP_SEA_CORN4_X;  tileSrcY[DEEP_SEA_CORN4]  = DEEP_SEA_CORN4_Y;
    tileSrcX[DEEP_SEA_SIDE1]  = DEEP_SEA_SIDE1_X;  tileSrcY[DEEP_SEA_SIDE1]  = DEEP_SEA_SIDE1_Y;
    tileSrcX[DEEP_SEA_SIDE2]  = DEEP_SEA_SIDE2_X;  tileSrcY[DEEP_SEA_SIDE2]  = DEEP_SEA_SIDE2_Y;
    tileSrcX[DEEP_SEA_SIDE3]  = DEEP_SEA_SIDE3_X;  tileSrcY[DEEP_SEA_SIDE3]  = DEEP_SEA_SIDE3_Y;
    tileSrcX[DEEP_SEA_SIDE4]  = DEEP_SEA_SIDE4_X;  tileSrcY[DEEP_SEA_SIDE4]  = DEEP_SEA_SIDE4_Y;

    /* Buildings */
    tileSrcX[BUILD_SINGLE]    = BUILD_SINGLE_X;    tileSrcY[BUILD_SINGLE]    = BUILD_SINGLE_Y;
    tileSrcX[BUILD_SOLID]     = BUILD_SOLID_X;     tileSrcY[BUILD_SOLID]     = BUILD_SOLID_Y;
    tileSrcX[BUILD_CORNER1]   = BUILD_CORNER1_X;   tileSrcY[BUILD_CORNER1]   = BUILD_CORNER1_Y;
    tileSrcX[BUILD_CORNER2]   = BUILD_CORNER2_X;   tileSrcY[BUILD_CORNER2]   = BUILD_CORNER2_Y;
    tileSrcX[BUILD_CORNER3]   = BUILD_CORNER3_X;   tileSrcY[BUILD_CORNER3]   = BUILD_CORNER3_Y;
    tileSrcX[BUILD_CORNER4]   = BUILD_CORNER4_X;   tileSrcY[BUILD_CORNER4]   = BUILD_CORNER4_Y;
    tileSrcX[BUILD_L1]        = BUILD_L1_X;        tileSrcY[BUILD_L1]        = BUILD_L1_Y;
    tileSrcX[BUILD_L2]        = BUILD_L2_X;        tileSrcY[BUILD_L2]        = BUILD_L2_Y;
    tileSrcX[BUILD_L3]        = BUILD_L3_X;        tileSrcY[BUILD_L3]        = BUILD_L3_Y;
    tileSrcX[BUILD_L4]        = BUILD_L4_X;        tileSrcY[BUILD_L4]        = BUILD_L4_Y;
    tileSrcX[BUILD_T1]        = BUILD_T1_X;        tileSrcY[BUILD_T1]        = BUILD_T1_Y;
    tileSrcX[BUILD_T2]        = BUILD_T2_X;        tileSrcY[BUILD_T2]        = BUILD_T2_Y;
    tileSrcX[BUILD_T3]        = BUILD_T3_X;        tileSrcY[BUILD_T3]        = BUILD_T3_Y;
    tileSrcX[BUILD_T4]        = BUILD_T4_X;        tileSrcY[BUILD_T4]        = BUILD_T4_Y;
    tileSrcX[BUILD_HORZ]      = BUILD_HORZ_X;      tileSrcY[BUILD_HORZ]      = BUILD_HORZ_Y;
    tileSrcX[BUILD_VERT]      = BUILD_VERT_X;      tileSrcY[BUILD_VERT]      = BUILD_VERT_Y;
    tileSrcX[BUILD_VERTEND1]  = BUILD_VERTEND1_X;  tileSrcY[BUILD_VERTEND1]  = BUILD_VERTEND1_Y;
    tileSrcX[BUILD_VERTEND2]  = BUILD_VERTEND2_X;  tileSrcY[BUILD_VERTEND2]  = BUILD_VERTEND2_Y;
    tileSrcX[BUILD_HORZEND1]  = BUILD_HORZEND1_X;  tileSrcY[BUILD_HORZEND1]  = BUILD_HORZEND1_Y;
    tileSrcX[BUILD_HORZEND2]  = BUILD_HORZEND2_X;  tileSrcY[BUILD_HORZEND2]  = BUILD_HORZEND2_Y;
    tileSrcX[BUILD_CROSS]     = BUILD_CROSS_X;     tileSrcY[BUILD_CROSS]     = BUILD_CROSS_Y;
    tileSrcX[BUILD_SIDE1]     = BUILD_SIDE1_X;     tileSrcY[BUILD_SIDE1]     = BUILD_SIDE1_Y;
    tileSrcX[BUILD_SIDE2]     = BUILD_SIDE2_X;     tileSrcY[BUILD_SIDE2]     = BUILD_SIDE2_Y;
    tileSrcX[BUILD_SIDE3]     = BUILD_SIDE3_X;     tileSrcY[BUILD_SIDE3]     = BUILD_SIDE3_Y;
    tileSrcX[BUILD_SIDE4]     = BUILD_SIDE4_X;     tileSrcY[BUILD_SIDE4]     = BUILD_SIDE4_Y;
    tileSrcX[BUILD_SIDECORN1] = BUILD_SIDECORN1_X; tileSrcY[BUILD_SIDECORN1] = BUILD_SIDECORN1_Y;
    tileSrcX[BUILD_SIDECORN2] = BUILD_SIDECORN2_X; tileSrcY[BUILD_SIDECORN2] = BUILD_SIDECORN2_Y;
    tileSrcX[BUILD_SIDECORN3] = BUILD_SIDECORN3_X; tileSrcY[BUILD_SIDECORN3] = BUILD_SIDECORN3_Y;
    tileSrcX[BUILD_SIDECORN4] = BUILD_SIDECORN4_X; tileSrcY[BUILD_SIDECORN4] = BUILD_SIDECORN4_Y;
    tileSrcX[BUILD_SIDECORN5] = BUILD_SIDECORN5_X; tileSrcY[BUILD_SIDECORN5] = BUILD_SIDECORN5_Y;
    tileSrcX[BUILD_SIDECORN6] = BUILD_SIDECORN6_X; tileSrcY[BUILD_SIDECORN6] = BUILD_SIDECORN6_Y;
    tileSrcX[BUILD_SIDECORN7] = BUILD_SIDECORN7_X; tileSrcY[BUILD_SIDECORN7] = BUILD_SIDECORN7_Y;
    tileSrcX[BUILD_SIDECORN8] = BUILD_SIDECORN8_X; tileSrcY[BUILD_SIDECORN8] = BUILD_SIDECORN8_Y;
    tileSrcX[BUILD_SIDECORN9] = BUILD_SIDECORN9_X; tileSrcY[BUILD_SIDECORN9] = BUILD_SIDECORN9_Y;
    tileSrcX[BUILD_SIDECORN10]= BUILD_SIDECORN10_X;tileSrcY[BUILD_SIDECORN10]= BUILD_SIDECORN10_Y;
    tileSrcX[BUILD_SIDECORN11]= BUILD_SIDECORN11_X;tileSrcY[BUILD_SIDECORN11]= BUILD_SIDECORN11_Y;
    tileSrcX[BUILD_SIDECORN12]= BUILD_SIDECORN12_X;tileSrcY[BUILD_SIDECORN12]= BUILD_SIDECORN12_Y;
    tileSrcX[BUILD_SIDECORN13]= BUILD_SIDECORN13_X;tileSrcY[BUILD_SIDECORN13]= BUILD_SIDECORN13_Y;
    tileSrcX[BUILD_SIDECORN14]= BUILD_SIDECORN14_X;tileSrcY[BUILD_SIDECORN14]= BUILD_SIDECORN14_Y;
    tileSrcX[BUILD_SIDECORN15]= BUILD_SIDECORN15_X;tileSrcY[BUILD_SIDECORN15]= BUILD_SIDECORN15_Y;
    tileSrcX[BUILD_SIDECORN16]= BUILD_SIDECORN16_X;tileSrcY[BUILD_SIDECORN16]= BUILD_SIDECORN16_Y;
    tileSrcX[BUILD_TWIST1]    = BUILD_TWIST1_X;    tileSrcY[BUILD_TWIST1]    = BUILD_TWIST1_Y;
    tileSrcX[BUILD_TWIST2]    = BUILD_TWIST2_X;    tileSrcY[BUILD_TWIST2]    = BUILD_TWIST2_Y;
    tileSrcX[BUILD_MOST1]     = BUILD_MOST1_X;     tileSrcY[BUILD_MOST1]     = BUILD_MOST1_Y;
    tileSrcX[BUILD_MOST2]     = BUILD_MOST2_X;     tileSrcY[BUILD_MOST2]     = BUILD_MOST2_Y;
    tileSrcX[BUILD_MOST3]     = BUILD_MOST3_X;     tileSrcY[BUILD_MOST3]     = BUILD_MOST3_Y;
    tileSrcX[BUILD_MOST4]     = BUILD_MOST4_X;     tileSrcY[BUILD_MOST4]     = BUILD_MOST4_Y;

    /* River */
    tileSrcX[RIVER_END1]      = RIVER_END1_X;      tileSrcY[RIVER_END1]      = RIVER_END1_Y;
    tileSrcX[RIVER_END2]      = RIVER_END2_X;      tileSrcY[RIVER_END2]      = RIVER_END2_Y;
    tileSrcX[RIVER_END3]      = RIVER_END3_X;      tileSrcY[RIVER_END3]      = RIVER_END3_Y;
    tileSrcX[RIVER_END4]      = RIVER_END4_X;      tileSrcY[RIVER_END4]      = RIVER_END4_Y;
    tileSrcX[RIVER_SOLID]     = RIVER_SOLID_X;     tileSrcY[RIVER_SOLID]     = RIVER_SOLID_Y;
    tileSrcX[RIVER_SURROUND]  = RIVER_SURROUND_X;  tileSrcY[RIVER_SURROUND]  = RIVER_SURROUND_Y;
    tileSrcX[RIVER_SIDE1]     = RIVER_SIDE1_X;     tileSrcY[RIVER_SIDE1]     = RIVER_SIDE1_Y;
    tileSrcX[RIVER_SIDE2]     = RIVER_SIDE2_X;     tileSrcY[RIVER_SIDE2]     = RIVER_SIDE2_Y;
    tileSrcX[RIVER_ONESIDE1]  = RIVER_ONESIDE1_X;  tileSrcY[RIVER_ONESIDE1]  = RIVER_ONESIDE1_Y;
    tileSrcX[RIVER_ONESIDE2]  = RIVER_ONESIDE2_X;  tileSrcY[RIVER_ONESIDE2]  = RIVER_ONESIDE2_Y;
    tileSrcX[RIVER_ONESIDE3]  = RIVER_ONESIDE3_X;  tileSrcY[RIVER_ONESIDE3]  = RIVER_ONESIDE3_Y;
    tileSrcX[RIVER_ONESIDE4]  = RIVER_ONESIDE4_X;  tileSrcY[RIVER_ONESIDE4]  = RIVER_ONESIDE4_Y;
    tileSrcX[RIVER_CORN1]     = RIVER_CORN1_X;     tileSrcY[RIVER_CORN1]     = RIVER_CORN1_Y;
    tileSrcX[RIVER_CORN2]     = RIVER_CORN2_X;     tileSrcY[RIVER_CORN2]     = RIVER_CORN2_Y;
    tileSrcX[RIVER_CORN3]     = RIVER_CORN3_X;     tileSrcY[RIVER_CORN3]     = RIVER_CORN3_Y;
    tileSrcX[RIVER_CORN4]     = RIVER_CORN4_X;     tileSrcY[RIVER_CORN4]     = RIVER_CORN4_Y;

    /* Swamp */
    tileSrcX[SWAMP]           = SWAMP_X;           tileSrcY[SWAMP]           = SWAMP_Y;

    /* Road */
    tileSrcX[ROAD_HORZ]       = ROAD_HORZ_X;       tileSrcY[ROAD_HORZ]       = ROAD_HORZ_Y;
    tileSrcX[ROAD_VERT]       = ROAD_VERT_X;       tileSrcY[ROAD_VERT]       = ROAD_VERT_Y;
    tileSrcX[ROAD_CORNER1]    = ROAD_CORNER1_X;    tileSrcY[ROAD_CORNER1]    = ROAD_CORNER1_Y;
    tileSrcX[ROAD_CORNER2]    = ROAD_CORNER2_X;    tileSrcY[ROAD_CORNER2]    = ROAD_CORNER2_Y;
    tileSrcX[ROAD_CORNER3]    = ROAD_CORNER3_X;    tileSrcY[ROAD_CORNER3]    = ROAD_CORNER3_Y;
    tileSrcX[ROAD_CORNER4]    = ROAD_CORNER4_X;    tileSrcY[ROAD_CORNER4]    = ROAD_CORNER4_Y;
    tileSrcX[ROAD_CORNER5]    = ROAD_CORNER5_X;    tileSrcY[ROAD_CORNER5]    = ROAD_CORNER5_Y;
    tileSrcX[ROAD_CORNER6]    = ROAD_CORNER6_X;    tileSrcY[ROAD_CORNER6]    = ROAD_CORNER6_Y;
    tileSrcX[ROAD_CORNER7]    = ROAD_CORNER7_X;    tileSrcY[ROAD_CORNER7]    = ROAD_CORNER7_Y;
    tileSrcX[ROAD_CORNER8]    = ROAD_CORNER8_X;    tileSrcY[ROAD_CORNER8]    = ROAD_CORNER8_Y;
    tileSrcX[ROAD_SIDE1]      = ROAD_SIDE1_X;      tileSrcY[ROAD_SIDE1]      = ROAD_SIDE1_Y;
    tileSrcX[ROAD_SIDE2]      = ROAD_SIDE2_X;      tileSrcY[ROAD_SIDE2]      = ROAD_SIDE2_Y;
    tileSrcX[ROAD_SIDE3]      = ROAD_SIDE3_X;      tileSrcY[ROAD_SIDE3]      = ROAD_SIDE3_Y;
    tileSrcX[ROAD_SIDE4]      = ROAD_SIDE4_X;      tileSrcY[ROAD_SIDE4]      = ROAD_SIDE4_Y;
    tileSrcX[ROAD_SOLID]      = ROAD_SOLID_X;      tileSrcY[ROAD_SOLID]      = ROAD_SOLID_Y;
    tileSrcX[ROAD_CROSS]      = ROAD_CROSS_X;      tileSrcY[ROAD_CROSS]      = ROAD_CROSS_Y;
    tileSrcX[ROAD_T1]         = ROAD_T1_X;         tileSrcY[ROAD_T1]         = ROAD_T1_Y;
    tileSrcX[ROAD_T2]         = ROAD_T2_X;         tileSrcY[ROAD_T2]         = ROAD_T2_Y;
    tileSrcX[ROAD_T3]         = ROAD_T3_X;         tileSrcY[ROAD_T3]         = ROAD_T3_Y;
    tileSrcX[ROAD_T4]         = ROAD_T4_X;         tileSrcY[ROAD_T4]         = ROAD_T4_Y;
    tileSrcX[ROAD_WATER1]     = ROAD_WATER1_X;     tileSrcY[ROAD_WATER1]     = ROAD_WATER1_Y;
    tileSrcX[ROAD_WATER2]     = ROAD_WATER2_X;     tileSrcY[ROAD_WATER2]     = ROAD_WATER2_Y;
    tileSrcX[ROAD_WATER3]     = ROAD_WATER3_X;     tileSrcY[ROAD_WATER3]     = ROAD_WATER3_Y;
    tileSrcX[ROAD_WATER4]     = ROAD_WATER4_X;     tileSrcY[ROAD_WATER4]     = ROAD_WATER4_Y;
    tileSrcX[ROAD_WATER5]     = ROAD_WATER5_X;     tileSrcY[ROAD_WATER5]     = ROAD_WATER5_Y;
    tileSrcX[ROAD_WATER6]     = ROAD_WATER6_X;     tileSrcY[ROAD_WATER6]     = ROAD_WATER6_Y;
    tileSrcX[ROAD_WATER7]     = ROAD_WATER7_X;     tileSrcY[ROAD_WATER7]     = ROAD_WATER7_Y;
    tileSrcX[ROAD_WATER8]     = ROAD_WATER8_X;     tileSrcY[ROAD_WATER8]     = ROAD_WATER8_Y;
    tileSrcX[ROAD_WATER9]     = ROAD_WATER9_X;     tileSrcY[ROAD_WATER9]     = ROAD_WATER9_Y;
    tileSrcX[ROAD_WATER10]    = ROAD_WATER10_X;    tileSrcY[ROAD_WATER10]    = ROAD_WATER10_Y;
    tileSrcX[ROAD_WATER11]    = ROAD_WATER11_X;    tileSrcY[ROAD_WATER11]    = ROAD_WATER11_Y;

    /* Pillboxes */
    tileSrcX[PILL_EVIL_15]    = PILL_EVIL15_X;     tileSrcY[PILL_EVIL_15]    = PILL_EVIL15_Y;
    tileSrcX[PILL_EVIL_14]    = PILL_EVIL14_X;     tileSrcY[PILL_EVIL_14]    = PILL_EVIL14_Y;
    tileSrcX[PILL_EVIL_13]    = PILL_EVIL13_X;     tileSrcY[PILL_EVIL_13]    = PILL_EVIL13_Y;
    tileSrcX[PILL_EVIL_12]    = PILL_EVIL12_X;     tileSrcY[PILL_EVIL_12]    = PILL_EVIL12_Y;
    tileSrcX[PILL_EVIL_11]    = PILL_EVIL11_X;     tileSrcY[PILL_EVIL_11]    = PILL_EVIL11_Y;
    tileSrcX[PILL_EVIL_10]    = PILL_EVIL10_X;     tileSrcY[PILL_EVIL_10]    = PILL_EVIL10_Y;
    tileSrcX[PILL_EVIL_9]     = PILL_EVIL9_X;      tileSrcY[PILL_EVIL_9]     = PILL_EVIL9_Y;
    tileSrcX[PILL_EVIL_8]     = PILL_EVIL8_X;      tileSrcY[PILL_EVIL_8]     = PILL_EVIL8_Y;
    tileSrcX[PILL_EVIL_7]     = PILL_EVIL7_X;      tileSrcY[PILL_EVIL_7]     = PILL_EVIL7_Y;
    tileSrcX[PILL_EVIL_6]     = PILL_EVIL6_X;      tileSrcY[PILL_EVIL_6]     = PILL_EVIL6_Y;
    tileSrcX[PILL_EVIL_5]     = PILL_EVIL5_X;      tileSrcY[PILL_EVIL_5]     = PILL_EVIL5_Y;
    tileSrcX[PILL_EVIL_4]     = PILL_EVIL4_X;      tileSrcY[PILL_EVIL_4]     = PILL_EVIL4_Y;
    tileSrcX[PILL_EVIL_3]     = PILL_EVIL3_X;      tileSrcY[PILL_EVIL_3]     = PILL_EVIL3_Y;
    tileSrcX[PILL_EVIL_2]     = PILL_EVIL2_X;      tileSrcY[PILL_EVIL_2]     = PILL_EVIL2_Y;
    tileSrcX[PILL_EVIL_1]     = PILL_EVIL1_X;      tileSrcY[PILL_EVIL_1]     = PILL_EVIL1_Y;
    tileSrcX[PILL_EVIL_0]     = PILL_EVIL0_X;      tileSrcY[PILL_EVIL_0]     = PILL_EVIL0_Y;
    tileSrcX[PILL_GOOD_15]    = PILL_GOOD15_X;     tileSrcY[PILL_GOOD_15]    = PILL_GOOD15_Y;
    tileSrcX[PILL_GOOD_14]    = PILL_GOOD14_X;     tileSrcY[PILL_GOOD_14]    = PILL_GOOD14_Y;
    tileSrcX[PILL_GOOD_13]    = PILL_GOOD13_X;     tileSrcY[PILL_GOOD_13]    = PILL_GOOD13_Y;
    tileSrcX[PILL_GOOD_12]    = PILL_GOOD12_X;     tileSrcY[PILL_GOOD_12]    = PILL_GOOD12_Y;
    tileSrcX[PILL_GOOD_11]    = PILL_GOOD11_X;     tileSrcY[PILL_GOOD_11]    = PILL_GOOD11_Y;
    tileSrcX[PILL_GOOD_10]    = PILL_GOOD10_X;     tileSrcY[PILL_GOOD_10]    = PILL_GOOD10_Y;
    tileSrcX[PILL_GOOD_9]     = PILL_GOOD9_X;      tileSrcY[PILL_GOOD_9]     = PILL_GOOD9_Y;
    tileSrcX[PILL_GOOD_8]     = PILL_GOOD8_X;      tileSrcY[PILL_GOOD_8]     = PILL_GOOD8_Y;
    tileSrcX[PILL_GOOD_7]     = PILL_GOOD7_X;      tileSrcY[PILL_GOOD_7]     = PILL_GOOD7_Y;
    tileSrcX[PILL_GOOD_6]     = PILL_GOOD6_X;      tileSrcY[PILL_GOOD_6]     = PILL_GOOD6_Y;
    tileSrcX[PILL_GOOD_5]     = PILL_GOOD5_X;      tileSrcY[PILL_GOOD_5]     = PILL_GOOD5_Y;
    tileSrcX[PILL_GOOD_4]     = PILL_GOOD4_X;      tileSrcY[PILL_GOOD_4]     = PILL_GOOD4_Y;
    tileSrcX[PILL_GOOD_3]     = PILL_GOOD3_X;      tileSrcY[PILL_GOOD_3]     = PILL_GOOD3_Y;
    tileSrcX[PILL_GOOD_2]     = PILL_GOOD2_X;      tileSrcY[PILL_GOOD_2]     = PILL_GOOD2_Y;
    tileSrcX[PILL_GOOD_1]     = PILL_GOOD1_X;      tileSrcY[PILL_GOOD_1]     = PILL_GOOD1_Y;
    tileSrcX[PILL_GOOD_0]     = PILL_GOOD0_X;      tileSrcY[PILL_GOOD_0]     = PILL_GOOD0_Y;

    /* Bases */
    tileSrcX[BASE_GOOD]       = BASE_GOOD_X;       tileSrcY[BASE_GOOD]       = BASE_GOOD_Y;
    tileSrcX[BASE_NEUTRAL]    = BASE_NEUTRAL_X;    tileSrcY[BASE_NEUTRAL]    = BASE_NEUTRAL_Y;
    tileSrcX[BASE_EVIL]       = BASE_EVIL_X;       tileSrcY[BASE_EVIL]       = BASE_EVIL_Y;

    /* Forest */
    tileSrcX[FOREST]          = FOREST_X;          tileSrcY[FOREST]          = FOREST_Y;
    tileSrcX[FOREST_SINGLE]   = FOREST_SINGLE_X;   tileSrcY[FOREST_SINGLE]   = FOREST_SINGLE_Y;
    tileSrcX[FOREST_BR]       = FOREST_BR_X;       tileSrcY[FOREST_BR]       = FOREST_BR_Y;
    tileSrcX[FOREST_BL]       = FOREST_BL_X;       tileSrcY[FOREST_BL]       = FOREST_BL_Y;
    tileSrcX[FOREST_AR]       = FOREST_AR_X;       tileSrcY[FOREST_AR]       = FOREST_AR_Y;
    tileSrcX[FOREST_AL]       = FOREST_AL_X;       tileSrcY[FOREST_AL]       = FOREST_AL_Y;
    tileSrcX[FOREST_ABOVE]    = FOREST_ABOVE_X;    tileSrcY[FOREST_ABOVE]    = FOREST_ABOVE_Y;
    tileSrcX[FOREST_BELOW]    = FOREST_BELOW_X;    tileSrcY[FOREST_BELOW]    = FOREST_BELOW_Y;
    tileSrcX[FOREST_LEFT]     = FOREST_LEFT_X;     tileSrcY[FOREST_LEFT]     = FOREST_LEFT_Y;
    tileSrcX[FOREST_RIGHT]    = FOREST_RIGHT_X;    tileSrcY[FOREST_RIGHT]    = FOREST_RIGHT_Y;

    /* Crater */
    tileSrcX[CRATER]          = CRATER_X;          tileSrcY[CRATER]          = CRATER_Y;
    tileSrcX[CRATER_SINGLE]   = CRATER_SINGLE_X;   tileSrcY[CRATER_SINGLE]   = CRATER_SINGLE_Y;
    tileSrcX[CRATER_BR]       = CRATER_BR_X;       tileSrcY[CRATER_BR]       = CRATER_BR_Y;
    tileSrcX[CRATER_BL]       = CRATER_BL_X;       tileSrcY[CRATER_BL]       = CRATER_BL_Y;
    tileSrcX[CRATER_AR]       = CRATER_AR_X;       tileSrcY[CRATER_AR]       = CRATER_AR_Y;
    tileSrcX[CRATER_AL]       = CRATER_AL_X;       tileSrcY[CRATER_AL]       = CRATER_AL_Y;
    tileSrcX[CRATER_ABOVE]    = CRATER_ABOVE_X;    tileSrcY[CRATER_ABOVE]    = CRATER_ABOVE_Y;
    tileSrcX[CRATER_BELOW]    = CRATER_BELOW_X;    tileSrcY[CRATER_BELOW]    = CRATER_BELOW_Y;
    tileSrcX[CRATER_LEFT]     = CRATER_LEFT_X;     tileSrcY[CRATER_LEFT]     = CRATER_LEFT_Y;
    tileSrcX[CRATER_RIGHT]    = CRATER_RIGHT_X;    tileSrcY[CRATER_RIGHT]    = CRATER_RIGHT_Y;

    /* Misc terrain */
    tileSrcX[RUBBLE]          = RUBBLE_X;          tileSrcY[RUBBLE]          = RUBBLE_Y;
    tileSrcX[GRASS]           = GRASS_X;           tileSrcY[GRASS]           = GRASS_Y;
    tileSrcX[HALFBUILDING]    = SHOT_BUILDING_X;   tileSrcY[HALFBUILDING]    = SHOT_BUILDING_Y;

    /* Boats */
    tileSrcX[BOAT_0]          = BOAT0_X;           tileSrcY[BOAT_0]          = BOAT0_Y;
    tileSrcX[BOAT_1]          = BOAT1_X;           tileSrcY[BOAT_1]          = BOAT1_Y;
    tileSrcX[BOAT_2]          = BOAT2_X;           tileSrcY[BOAT_2]          = BOAT2_Y;
    tileSrcX[BOAT_3]          = BOAT3_X;           tileSrcY[BOAT_3]          = BOAT3_Y;
    tileSrcX[BOAT_4]          = BOAT4_X;           tileSrcY[BOAT_4]          = BOAT4_Y;
    tileSrcX[BOAT_5]          = BOAT5_X;           tileSrcY[BOAT_5]          = BOAT5_Y;
    tileSrcX[BOAT_6]          = BOAT6_X;           tileSrcY[BOAT_6]          = BOAT6_Y;
    tileSrcX[BOAT_7]          = BOAT7_X;           tileSrcY[BOAT_7]          = BOAT7_Y;

    tilesReady = 1;
}

/* ------------------------------------------------------------------ */
/* B3 HUD state — updated by setter functions, drawn each frame        */
/* ------------------------------------------------------------------ */
#define MAX_PILLS  16
#define MAX_BASES  16
#define MAX_TANKS  16

static pillAlliance g_pillState[MAX_PILLS];
static baseAlliance g_baseState[MAX_BASES];
static tankAlliance g_tankState[MAX_TANKS];

static BYTE g_tkShells = 0, g_tkMines = 0, g_tkArmour = 0, g_tkTrees = 0;
static BYTE g_bsShells = 0, g_bsMines = 0, g_bsArmour = 0;

static char g_msgTop[MESSAGE_STRING_SIZE];
static char g_msgBot[MESSAGE_STRING_SIZE];

static int  g_kills  = 0;
static int  g_deaths = 0;

static float g_manAngle  = 0.0f;
static int   g_manDead   = 0;
static int   g_manInTank = 1;  /* default: man starts inside tank */

/* ------------------------------------------------------------------ */
/* Status icon source coordinates — indexed by alliance enum value     */
/* ------------------------------------------------------------------ */
/* Pill icon source (srcX, srcY) indexed by pillAlliance */
static const int pillIconX[] = {
    STATUS_ITEM_DEAD_X,           /* pillDead      */
    STATUS_PILLBOX_ALLIEGOOD_X,   /* pillAllie     */
    STATUS_PILLBOX_GOOD_X,        /* pillGood      */
    STATUS_PILLBOX_NEUTRAL_X,     /* pillNeutral   */
    STATUS_PILLBOX_EVIL_X,        /* pillEvil      */
    STATUS_PILLBOX_TANKGOOD_X,    /* pillTankGood  */
    STATUS_PILLBOX_TANKALLIE_X,   /* pillTankAllie */
    STATUS_PILLBOX_TANKEVIL_X     /* pillTankEvil  */
};
static const int pillIconY[] = {
    STATUS_ITEM_DEAD_Y,
    STATUS_PILLBOX_ALLIEGOOD_Y,
    STATUS_PILLBOX_GOOD_Y,
    STATUS_PILLBOX_NEUTRAL_Y,
    STATUS_PILLBOX_EVIL_Y,
    STATUS_PILLBOX_TANKGOOD_Y,
    STATUS_PILLBOX_TANKALLIE_Y,
    STATUS_PILLBOX_TANKEVIL_Y
};

/* Base icon source indexed by baseAlliance */
static const int baseIconX[] = {
    STATUS_ITEM_DEAD_X,       /* baseDead      */
    STATUS_BASE_GOOD_X,       /* baseOwnGood   */
    STATUS_BASE_ALLIEGOOD_X,  /* baseAllieGood */
    STATUS_BASE_NEUTRAL_X,    /* baseNeutral   */
    STATUS_BASE_EVIL_X        /* baseEvil      */
};
static const int baseIconY[] = {
    STATUS_ITEM_DEAD_Y,
    STATUS_BASE_GOOD_Y,
    STATUS_BASE_ALLIEGOOD_Y,
    STATUS_BASE_NEUTRAL_Y,
    STATUS_BASE_EVIL_Y
};

/* Tank icon source indexed by tankAlliance */
static const int tankIconX[] = {
    STATUS_TANK_NONE_X,   /* tankNone  */
    STATUS_TANK_SELF_X,   /* tankSelf  */
    STATUS_TANK_GOOD_X,   /* tankAllie */
    STATUS_TANK_EVIL_X    /* tankEvil  */
};
static const int tankIconY[] = {
    STATUS_TANK_NONE_Y,
    STATUS_TANK_SELF_Y,
    STATUS_TANK_GOOD_Y,
    STATUS_TANK_EVIL_Y
};

/* ------------------------------------------------------------------ */
/* Helper: return (relX, relY) of item n (1-based) inside a panel.    */
/* Mirrors the STATUS_BASE_n_X/Y layout from positions.h.             */
/* ------------------------------------------------------------------ */
static void getItemPos(int n, int *rx, int *ry)
{
    int step_x = STATUS_ITEM_GAP_X + STATUS_ITEM_SIZE_X;  /* 14 */
    int step_y = STATUS_ITEM_GAP_Y + STATUS_ITEM_SIZE_Y;  /* 21 */
    int row, col;

    if (n <= 6)       { row = 0; col = n - 1; }
    else if (n <= 8)  { row = 1; col = n - 7; }        /* cols 0,1 */
    else if (n <= 10) { row = 1; col = (n - 9) + 4; }  /* cols 4,5 */
    else              { row = 2; col = n - 11; }        /* cols 0-5 */

    *rx = 4 + col * step_x;
    *ry = 4 + row * step_y;
}

/* ------------------------------------------------------------------ */
/* drawHUD — called from frontEndDrawMainScreen every frame            */
/* ------------------------------------------------------------------ */
static void drawHUD(void)
{
    int i, relX, relY;
    char numBuf[8];

    /* --- Status icon panels ---------------------------------------- */
    /* Tanks panel (STATUS_TANKS_LEFT=335, STATUS_TANKS_TOP=18) */
    for (i = 1; i <= MAX_TANKS; i++) {
        int alliance = (int)g_tankState[i - 1];
        if (alliance < 0 || alliance > 3) alliance = 0;
        getItemPos(i, &relX, &relY);
        renderStatusIcon(tankIconX[alliance], tankIconY[alliance],
                         STATUS_TANKS_LEFT + relX,
                         STATUS_TANKS_TOP  + relY);
    }

    /* Pills panel (STATUS_PILLS_LEFT=335, STATUS_PILLS_TOP=88) */
    for (i = 1; i <= MAX_PILLS; i++) {
        int alliance = (int)g_pillState[i - 1];
        if (alliance < 0 || alliance > 7) alliance = 3; /* default neutral */
        getItemPos(i, &relX, &relY);
        renderStatusIcon(pillIconX[alliance], pillIconY[alliance],
                         STATUS_PILLS_LEFT + relX,
                         STATUS_PILLS_TOP  + relY);
    }

    /* Bases panel (STATUS_BASES_LEFT=335, STATUS_BASES_TOP=157) */
    for (i = 1; i <= MAX_BASES; i++) {
        int alliance = (int)g_baseState[i - 1];
        if (alliance < 0 || alliance > 4) alliance = 3; /* default neutral */
        getItemPos(i, &relX, &relY);
        renderStatusIcon(baseIconX[alliance], baseIconY[alliance],
                         STATUS_BASES_LEFT + relX,
                         STATUS_BASES_TOP  + relY);
    }

    /* --- Tank status bars (vertical, bottom-anchored) -------------- */
    /* Clear bar area black first (background.bmp has the frame) */
    renderDrawBar(STATUS_TANK_SHELLS, STATUS_TANK_BARS_TOP,
                  STATUS_TANK_TREES + STATUS_TANK_BARS_WIDTH - STATUS_TANK_SHELLS,
                  STATUS_TANK_BARS_HEIGHT, 0, 0, 0);

    /* All four bars are green in the original (ddpf.dwGBitMask) */
#define DRAW_VBAR(leftX, value) \
    renderDrawBar((leftX), \
                  STATUS_TANK_BARS_TOP + STATUS_TANK_BARS_HEIGHT - (BAR_TANK_MULTIPLY * (value)), \
                  STATUS_TANK_BARS_WIDTH, \
                  BAR_TANK_MULTIPLY * (value), \
                  0, 200, 0)

    DRAW_VBAR(STATUS_TANK_SHELLS, g_tkShells);
    DRAW_VBAR(STATUS_TANK_MINES,  g_tkMines);
    DRAW_VBAR(STATUS_TANK_ARMOUR, g_tkArmour);
    DRAW_VBAR(STATUS_TANK_TREES,  g_tkTrees);

#undef DRAW_VBAR

    /* --- Messages -------------------------------------------------- */
    renderDrawText(MESSAGE_TOP_LINE_X, MESSAGE_TOP_LINE_Y,
                   8, g_msgTop,  255, 255, 255);
    renderDrawText(MESSAGE_BOTTOM_LINE_X, MESSAGE_BOTTOM_LINE_Y,
                   8, g_msgBot, 255, 255, 255);

    /* --- Kills / Deaths ------------------------------------------- */
    numBuf[0] = '\0';
    /* Simple sprintf substitute: use itoa manually for kills */
    {
        int k = g_kills, d = g_deaths;
        char kb[4], db[4];
        int n;
        /* kills */
        if (k > 999) k = 999;
        n = 0; if (k == 0) { kb[n++] = '0'; }
        else { int tmp = k; char tmp2[4]; int tl=0;
               while (tmp > 0) { tmp2[tl++] = (char)('0' + tmp % 10); tmp /= 10; }
               while (tl-- > 0) kb[n++] = tmp2[tl+0]; /* reversed */
               /* simple reversal */
               { int a=0, b=n-1; while(a<b){char t=kb[a];kb[a]=kb[b];kb[b]=t;a++;b--;} }
             }
        kb[n] = '\0';
        /* deaths */
        n = 0; if (d == 0) { db[n++] = '0'; }
        else { int tmp = d; char tmp2[4]; int tl=0;
               if (d > 999) d = 999;
               while (tmp > 0) { tmp2[tl++] = (char)('0' + tmp % 10); tmp /= 10; }
               { int a=0, b=tl-1; while(a<b){char t=tmp2[a];tmp2[a]=tmp2[b];tmp2[b]=t;a++;b--;} }
               while (tl-- > 0) db[n++] = tmp2[0+n];
             }
        db[n] = '\0';
        renderDrawText(STATUS_KILLS_LEFT,  STATUS_KILLS_TOP,
                       8, kb, 255, 255, 255);
        renderDrawText(STATUS_DEATHS_LEFT, STATUS_DEATHS_TOP,
                       8, db, 255, 255, 255);
    }

    /* --- Man status compass ---------------------------------------- */
    renderDrawManStatus(MAN_STATUS_X + MAN_STATUS_RADIUS,
                        MAN_STATUS_Y + MAN_STATUS_RADIUS,
                        g_manDead, g_manInTank, g_manAngle);
}

/* ------------------------------------------------------------------ */
/* Asset paths (deployed next to exe by CMake post-build steps)        */
/* ------------------------------------------------------------------ */
#define TILES_BMP_PATH      "tiles.bmp"
#define BACKGROUND_BMP_PATH "background.bmp"
#define SOUNDS_DIR_PATH     "sounds"

/* ------------------------------------------------------------------ */
/* frontEndDrawMainScreen — B3 main render function                    */
/* ------------------------------------------------------------------ */
void frontEndDrawMainScreen(
    screen *value, screenMines *mineView, screenTanks *tks,
    screenGunsight *gs, screenBullets *sBullet, screenLgm *lgms,
    long srtDelay, bool isPillView, int edgeX, int edgeY)
{
    BYTE x, y, pos;

    if (!tilesReady) {
        int i;
        initTileLookup();
        renderLoadTiles(TILES_BMP_PATH);
        renderLoadBackground(BACKGROUND_BMP_PATH);
        renderLoadSounds(SOUNDS_DIR_PATH);
        renderSetZoom(2);  /* 2x for modern screens (1030x650) */
        /* Initialise HUD state arrays */
        for (i = 0; i < MAX_PILLS;  i++) g_pillState[i] = pillNeutral;
        for (i = 0; i < MAX_BASES;  i++) g_baseState[i] = baseNeutral;
        for (i = 0; i < MAX_TANKS;  i++) g_tankState[i] = tankNone;
        g_msgTop[0] = '\0';
        g_msgBot[0] = '\0';
    }

    /* 1. Background chrome */
    renderDrawBackground();

    /* 2. Tile grid at (MAIN_OFFSET_X, MAIN_OFFSET_Y) with sub-tile scroll */
    for (y = 0; y < MAIN_SCREEN_SIZE_Y; y++) {
        for (x = 0; x < MAIN_SCREEN_SIZE_X; x++) {
            pos = screenGetPos(value, x, y);
            renderTile(tileSrcX[pos], tileSrcY[pos],
                       MAIN_OFFSET_X + x * TILE_SIZE_X - edgeX,
                       MAIN_OFFSET_Y + y * TILE_SIZE_Y - edgeY);
            if (screenIsMine(mineView, x, y)) {
                renderMine(MAIN_OFFSET_X + x * TILE_SIZE_X - edgeX,
                           MAIN_OFFSET_Y + y * TILE_SIZE_Y - edgeY);
            }
        }
    }

    /* 3. Tank sprites — viewport-relative coords from screenTanksPrepare.
     * Handles TANK_SELF frames (0-15) and TANK_SELFBOAT frames (16-31). */
    {
        /* TANK_SELFBOAT source coords — frames 0-4 are in row 5 col 15-19,
         * frames 5-15 are in row 6 col 0-10. Matches tiles.h exactly. */
        static const int boatSrcX[16] = {
            15*TILE_SIZE_X, 16*TILE_SIZE_X, 17*TILE_SIZE_X, 18*TILE_SIZE_X,
            19*TILE_SIZE_X,
            0,   TILE_SIZE_X,  2*TILE_SIZE_X,  3*TILE_SIZE_X,  4*TILE_SIZE_X,
            5*TILE_SIZE_X, 6*TILE_SIZE_X, 7*TILE_SIZE_X, 8*TILE_SIZE_X,
            9*TILE_SIZE_X, 10*TILE_SIZE_X
        };
        static const int boatSrcY[16] = {
            5*TILE_SIZE_Y, 5*TILE_SIZE_Y, 5*TILE_SIZE_Y, 5*TILE_SIZE_Y,
            5*TILE_SIZE_Y,
            6*TILE_SIZE_Y, 6*TILE_SIZE_Y, 6*TILE_SIZE_Y, 6*TILE_SIZE_Y,
            6*TILE_SIZE_Y, 6*TILE_SIZE_Y, 6*TILE_SIZE_Y, 6*TILE_SIZE_Y,
            6*TILE_SIZE_Y, 6*TILE_SIZE_Y, 6*TILE_SIZE_Y
        };

        BYTE count = 1;
        BYTE total = screenTanksGetNumEntries(tks);
        while (count <= total) {
            BYTE mx, my, px, py, frame, playerNum;
            char playerName[260];
            int dstX, dstY;
            screenTanksGetItem(tks, count, &mx, &my, &px, &py,
                               &frame, &playerNum, playerName);
            dstX = MAIN_OFFSET_X + mx * TILE_SIZE_X + (int)(px + 2);
            dstY = MAIN_OFFSET_Y + my * TILE_SIZE_Y + (int)(py + 2);
            if (frame < 16) {
                /* TANK_SELF_0..15: row 4, col = frame */
                renderTile(frame * TILE_SIZE_X, 4 * TILE_SIZE_Y, dstX, dstY);
            } else if (frame < 32) {
                /* TANK_SELFBOAT_0..15 */
                renderTile(boatSrcX[frame - 16], boatSrcY[frame - 16], dstX, dstY);
            }
            count++;
        }
    }

    /* 4. Gunsight crosshair — drawn before shells so shells render on top.
     * gs->mapX == NO_GUNSIGHT (-1) when outside viewport or hidden. */
    if (gs->mapX != NO_GUNSIGHT) {
        renderTile(17 * TILE_SIZE_X, 4 * TILE_SIZE_Y,
                   MAIN_OFFSET_X + (int)gs->mapX * TILE_SIZE_X + (int)gs->pixelX,
                   MAIN_OFFSET_Y + (int)gs->mapY * TILE_SIZE_Y + (int)gs->pixelY);
    }

    /* 5. Shells and explosions.
     * Frame IDs (tilenum.h): 1-8 = SHELL_EXPLOSION8..1 (16x16 tiles),
     *                         9-24 = SHELL_DIR0..15 (tiny 3-4px sprites). */
    {
        /* Explosion source coords indexed by expNum 1-8 */
        static const int expX[9] = { 0,
            29*TILE_SIZE_X, 30*TILE_SIZE_X,           /* EXPLOSION1,2 */
            29*TILE_SIZE_X, 30*TILE_SIZE_X,           /* EXPLOSION3,4 */
            29*TILE_SIZE_X, 30*TILE_SIZE_X,           /* EXPLOSION5,6 */
            18*TILE_SIZE_X, 19*TILE_SIZE_X            /* EXPLOSION7,8 */
        };
        static const int expY[9] = { 0,
            3*TILE_SIZE_Y, 3*TILE_SIZE_Y,
            4*TILE_SIZE_Y, 4*TILE_SIZE_Y,
            5*TILE_SIZE_Y, 5*TILE_SIZE_Y,
            4*TILE_SIZE_Y, 4*TILE_SIZE_Y
        };

        /* Shell sprite source coords + sizes for SHELL_DIR0..15 */
        static const int shellX[16]  = { 452,455,458,452,456,460,452,456,459,452,456,459,452,456,452,456 };
        static const int shellY[16]  = {  72, 72, 72, 76, 76, 76, 79, 79, 79, 83, 83, 83, 87, 87, 90, 90 };
        static const int shellW[16]  = {   3,  3,  4,  4,  4,  4,  4,  3,  3,  3,  3,  4,  4,  4,  4,  4 };
        static const int shellH[16]  = {   4,  4,  4,  3,  3,  3,  4,  4,  4,  4,  4,  3,  3,  3,  4,  3 };

        int  total = screenBulletsGetNumEntries(sBullet);
        int  count = 1;
        while (count <= total) {
            BYTE mx, my, px, py, frame;
            screenBulletsGetItem(sBullet, count, &mx, &my, &px, &py, &frame);
            int dstX = MAIN_OFFSET_X + (int)mx * TILE_SIZE_X + (int)px;
            int dstY = MAIN_OFFSET_Y + (int)my * TILE_SIZE_Y + (int)py;
            if (frame >= 1 && frame <= 8) {
                /* Explosion: frame 1=EXPLOSION8, 2=EXPLOSION7, ..., 8=EXPLOSION1 */
                int expNum = 9 - (int)frame;
                renderTile(expX[expNum], expY[expNum], dstX, dstY);
            } else if (frame >= 9 && frame <= 24) {
                /* Shell direction: SHELL_DIR0..15 */
                int d = (int)frame - 9;
                renderSprite(shellX[d], shellY[d], shellW[d], shellH[d], dstX, dstY);
            }
            count++;
        }
    }

    /* 6. HUD overlay */
    drawHUD();

    (void)lgms;
    (void)srtDelay; (void)isPillView;
}

/* ------------------------------------------------------------------ */
/* B3 setter functions — store state for drawHUD()                     */
/* ------------------------------------------------------------------ */
void frontEndUpdateTankStatusBars(BYTE shells, BYTE mines, BYTE armour, BYTE trees)
{
    g_tkShells = shells;
    g_tkMines  = mines;
    g_tkArmour = armour;
    g_tkTrees  = trees;
}

void frontEndUpdateBaseStatusBars(BYTE shells, BYTE mines, BYTE armour)
{
    g_bsShells = shells;
    g_bsMines  = mines;
    g_bsArmour = armour;
}

void frontEndStatusPillbox(BYTE pillNum, pillAlliance pb)
{
    if (pillNum >= 1 && pillNum <= MAX_PILLS)
        g_pillState[pillNum - 1] = pb;
}

void frontEndStatusTank(BYTE tankNum, tankAlliance ts)
{
    if (tankNum >= 1 && tankNum <= MAX_TANKS)
        g_tankState[tankNum - 1] = ts;
}

void frontEndStatusBase(BYTE baseNum, baseAlliance bs)
{
    if (baseNum >= 1 && baseNum <= MAX_BASES)
        g_baseState[baseNum - 1] = bs;
}

void frontEndMessages(char *top, char *bottom)
{
    if (top)    { strncpy(g_msgTop, top,    MESSAGE_STRING_SIZE - 1); g_msgTop[MESSAGE_STRING_SIZE-1] = '\0'; }
    if (bottom) { strncpy(g_msgBot, bottom, MESSAGE_STRING_SIZE - 1); g_msgBot[MESSAGE_STRING_SIZE-1] = '\0'; }
}

void frontEndKillsDeaths(int kills, int deaths)
{
    g_kills  = kills;
    g_deaths = deaths;
}

void frontEndManStatus(bool isDead, TURNTYPE angle)
{
    g_manDead   = isDead ? 1 : 0;
    g_manInTank = 0;
    g_manAngle  = (float)angle;
}

void frontEndManClear(void)
{
    g_manInTank = 1;
}

/* ------------------------------------------------------------------ */
/* frontEndDrawDownload — called while inStart=TRUE (network loading)  */
/* ------------------------------------------------------------------ */
void frontEndDrawDownload(bool justBlack)
{
    if (justBlack) return;
    /* Draw a black rect over the tile viewport area */
    renderDrawBar(MAIN_OFFSET_X, MAIN_OFFSET_Y,
                  MAIN_SCREEN_SIZE_X * TILE_SIZE_X,
                  MAIN_SCREEN_SIZE_Y * TILE_SIZE_Y,
                  0, 0, 0);
    renderDrawText(MAIN_OFFSET_X + 4, MAIN_OFFSET_Y + 100,
                   8, "Downloading...", 255, 255, 255);
}

/* ------------------------------------------------------------------ */
/* Remaining stubs (B4/B5)                                             */
/* ------------------------------------------------------------------ */
void frontEndPlaySound(sndEffects value) { renderPlaySound((int)value); }
void frontEndGameOver(void)                                                         {}
void frontEndClearPlayer(playerNumbers value)                                       { (void)value; }
void frontEndSetPlayer(playerNumbers value, char *str)                              { (void)value; (void)str; }
void frontEndSetPlayerCheckState(playerNumbers value, bool isChecked)               { (void)value; (void)isChecked; }
void frontEndEnableRequestAllyMenu(bool enabled)                                    { (void)enabled; }
void frontEndEnableLeaveAllyMenu(bool enabled)                                      { (void)enabled; }
void frontEndShowGunsight(bool isShown)                                             { (void)isShown; }

bool frontEndTutorial(BYTE pos)
{
    (void)pos;
    return FALSE;
}