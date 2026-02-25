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
 * main.c — WinBolo client entry point.
 *
 * Phase B5: Raylib keyboard input wired to tank controls.
 * Phase B6: Pre-game player name entry screen.
 *
 * NOTE: compiled WITHOUT /FI winbolo_platform.h to avoid the
 * raylib.h / wingdi.h Rectangle clash.  game_loop.h is the bridge;
 * it exposes only C standard types so this file stays raylib-safe.
 */

#include <string.h>
#include "raylib.h"
#include "game_loop.h"

/* Hard-coded test map — replace with a file picker in a future phase. */
#define TEST_MAP \
    "C:\\Users\\marys\\Projects\\bolo\\winbolo115-src\\winbolo\\src\\gui\\win32\\Everard Island.map"

#define PLAYER_NAME_MAX 20   /* WinBolo effective player-name limit */

/*
 * Show a simple name-entry screen and fill buf (size PLAYER_NAME_MAX+1).
 * Returns when the user presses Enter (confirms) or Escape (uses default).
 * Must be called inside an active Raylib window.
 */
static void enterPlayerName(char *buf, int maxLen)
{
    int len = 0;
    buf[0] = '\0';

    while (!WindowShouldClose()) {
        /* Collect typed characters */
        int ch = GetCharPressed();
        while (ch > 0) {
            if (ch >= 32 && ch < 127 && len < maxLen - 1) {
                buf[len++] = (char)ch;
                buf[len]   = '\0';
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && len > 0)
            buf[--len] = '\0';

        if (IsKeyPressed(KEY_ENTER) && len > 0) break;
        if (IsKeyPressed(KEY_ESCAPE)) {
            strcpy(buf, "Player");
            break;
        }

        /* Draw the entry screen */
        BeginDrawing();
        ClearBackground((Color){0, 0, 0, 255});

        DrawText("WinBolo", 390, 180, 40, (Color){255, 220, 0, 255});
        DrawText("Enter your player name:", 320, 270, 20, (Color){200, 200, 200, 255});

        /* Name input box */
        DrawRectangle(300, 310, 430, 44, (Color){30, 30, 30, 255});
        DrawRectangleLines(300, 310, 430, 44, (Color){255, 220, 0, 255});
        DrawText(buf, 312, 322, 24, (Color){255, 255, 255, 255});

        /* Blinking cursor */
        if ((int)(GetTime() * 2) % 2 == 0) {
            int tw = MeasureText(buf, 24);
            DrawText("|", 312 + tw, 322, 24, (Color){255, 220, 0, 255});
        }

        DrawText("ENTER to start   ESC for default (Player)",
                 248, 380, 16, (Color){120, 120, 120, 255});
        EndDrawing();
    }

    if (len == 0) strcpy(buf, "Player");
}

int main(void)
{
    /* Window size = SCREEN_SIZE_X/Y * zoom (see render_bridge.c ZOOM default=2).
     * 515*2=1030, 325*2=650 — crisp on 1920x1200 and above. */
    InitWindow(1030, 650, "WinBolo");
    InitAudioDevice();
    SetTargetFPS(50);   /* original screenGameTick fires at 50 Hz (100 Hz timer, alternating) */

    /* Collect player name before starting the game engine */
    char playerName[PLAYER_NAME_MAX + 1];
    enterPlayerName(playerName, sizeof(playerName));

    int mapLoaded = boloInit(TEST_MAP, playerName);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0, 0, 0, 255});

        if (mapLoaded) {
            boloUpdate();   /* screenUpdate(redraw) → frontEndDrawMainScreen() */
        } else {
            DrawText("Map load FAILED: " TEST_MAP, 10, 10, 12, (Color){255, 80, 80, 255});
        }

        EndDrawing();

        if (mapLoaded) {
            /* Poll keyboard — Phase B5 input.
             * Arrow keys (or WASD) → movement; Space/LCtrl → fire.
             * Precedence mirrors the original input.c logic. */
            int fwd  = IsKeyDown(KEY_UP)           || IsKeyDown(KEY_W);
            int bwd  = IsKeyDown(KEY_DOWN)         || IsKeyDown(KEY_S);
            int lft  = IsKeyDown(KEY_LEFT)         || IsKeyDown(KEY_A);
            int rgt  = IsKeyDown(KEY_RIGHT)        || IsKeyDown(KEY_D);
            int fire = IsKeyDown(KEY_SPACE)        || IsKeyDown(KEY_LEFT_CONTROL);

            /* Build tankButton ordinal (0-8). */
            int tb;
            if      (fwd && rgt) tb = 6; /* TRIGHTACCEL */
            else if (fwd && lft) tb = 5; /* TLEFTACCEL  */
            else if (bwd && lft) tb = 7; /* TLEFTDECEL  */
            else if (bwd && rgt) tb = 8; /* TRIGHTDECEL */
            else if (fwd)        tb = 3; /* TACCEL      */
            else if (bwd)        tb = 4; /* TDECEL      */
            else if (lft)        tb = 1; /* TLEFT       */
            else if (rgt)        tb = 2; /* TRIGHT      */
            else                 tb = 0; /* TNONE       */

            boloTick(tb, fire);

            /* Gunsight range: [ = shorter, ] = longer. */
            if (IsKeyPressed(KEY_LEFT_BRACKET))  boloGunsightRange(0);
            if (IsKeyPressed(KEY_RIGHT_BRACKET)) boloGunsightRange(1);

            /* B = send LGM to gunsight tile (captures bases, builds terrain). */
            if (IsKeyPressed(KEY_B)) boloManMove(0); /* BsTrees = generic */
        }
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}