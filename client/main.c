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
 * Phase B6+: Launcher screen (Tutorial / Practice / Network).
 *
 * NOTE: compiled WITHOUT /FI winbolo_platform.h to avoid the
 * raylib.h / wingdi.h Rectangle clash.  game_loop.h is the bridge;
 * it exposes only C standard types so this file stays raylib-safe.
 */

#include <string.h>
#include "raylib.h"
#include "game_loop.h"

/*
 * g_font — Courier New loaded as a TTF.  Matches the original WinBolo
 * FONT_NAME "Courier New" from gui/font.h.  Raylib falls back silently
 * to its built-in bitmap font if the system path is not found (Linux).
 */
static Font g_font;

/* drawText — thin wrapper used by all pre-game screens so they share
 * the loaded TTF.  Drop-in replacement for drawText(). */
static void drawText(const char *text, int x, int y, int size, Color col)
{
    DrawTextEx(g_font, text, (Vector2){(float)x, (float)y},
               (float)size, 1.0f, col);
}

/* Hard-coded test map — replace with a file picker in a future phase. */
#define TEST_MAP \
    "C:\\Users\\marys\\Projects\\bolo\\winbolo115-src\\winbolo\\src\\gui\\win32\\Everard Island.map"

#define PLAYER_NAME_MAX 20   /* WinBolo effective player-name limit */

/* Color palette shared across all pre-game screens */
#define COL_BG      (Color){  0,   0,   0, 255}  /* black background     */
#define COL_GOLD    (Color){255, 220,   0, 255}  /* golden yellow        */
#define COL_WHITE   (Color){255, 255, 255, 255}  /* option text          */
#define COL_GRAY    (Color){200, 200, 200, 255}  /* body text            */
#define COL_DIM     (Color){120, 120, 120, 255}  /* hints / grayed items */
#define COL_DARK    (Color){ 30,  30,  30, 255}  /* input box fill       */

/* launcherScreen — shows the Network Selection dialog.
 * Returns: 0=Tutorial  1=Practice (single player)  2=Quit */
static int launcherScreen(void)
{
    /* 0=Tutorial  1=Practice — network options 2-4 are non-selectable */
    int sel = 1; /* default: Practice */

    /* Option labels and y positions */
    static const char *labels[] = {
        "Tutorial  (Instruction for first-time player)",
        "Practice  (Single Player; No Network)",
        "TCP/IP",
        "Local Network  (Broadcast Search)",
        "Internet  (Tracker Search)"
    };
    static const int optY[] = { 185, 225, 280, 315, 350 };
    static const int NUM_SEL = 2; /* only first two are selectable */
    static const int NUM_OPT = 5;

    while (!WindowShouldClose()) {
        /* Input */
        if (IsKeyPressed(KEY_UP)   && sel > 0)       sel--;
        if (IsKeyPressed(KEY_DOWN) && sel < NUM_SEL-1) sel++;
        if (IsKeyPressed(KEY_ENTER)) return sel;
        if (IsKeyPressed(KEY_ESCAPE)) return 2; /* Quit */

        /* Draw */
        BeginDrawing();
        ClearBackground(COL_BG);

        /* Title */
        drawText("OpenBolo", 415, 40, 40, COL_GOLD);

        /* Subtitle */
        drawText("Welcome to OpenBolo, the multiplayer tank game.",
                 215, 110, 16, COL_GRAY);
        drawText("Please choose a game type from the list below:",
                 230, 132, 16, COL_GRAY);

        /* Horizontal rule */
        DrawRectangle(150, 162, 730, 1, (Color){80, 80, 80, 255});

        /* Options */
        for (int i = 0; i < NUM_OPT; i++) {
            int cx = 175;
            int cy = optY[i] + 9;
            int isSelectable = (i < NUM_SEL);
            int isSelected   = (i == sel);
            Color textCol    = isSelectable ? (isSelected ? COL_WHITE : COL_GRAY) : COL_DIM;
            Color circleCol  = isSelectable ? (isSelected ? COL_GOLD  : COL_GRAY) : COL_DIM;

            if (isSelected) {
                /* Filled dot */
                DrawCircle(cx, cy, 6, COL_GOLD);
                DrawCircleLines((float)cx, (float)cy, 8, COL_GOLD);
            } else {
                /* Hollow dot */
                DrawCircleLines((float)cx, (float)cy, 7, circleCol);
            }
            drawText(labels[i], 198, optY[i], 20, textCol);
        }

        /* Phase C note */
        drawText("Network options will be available in Phase C.",
                 215, 385, 14, COL_DIM);

        /* Horizontal rule */
        DrawRectangle(150, 540, 730, 1, (Color){80, 80, 80, 255});

        /* OK / Quit buttons */
        DrawRectangle(360, 558, 120, 38, COL_DARK);
        DrawRectangleLines(360, 558, 120, 38, COL_GOLD);
        drawText("OK", 407, 568, 20, COL_GOLD);

        DrawRectangle(550, 558, 120, 38, COL_DARK);
        DrawRectangleLines(550, 558, 120, 38, COL_DIM);
        drawText("Quit", 591, 568, 20, COL_DIM);

        drawText("Up/Down to select   Enter = OK   Esc = Quit",
                 285, 608, 14, COL_DIM);

        EndDrawing();
    }
    return 2; /* window closed = quit */
}

/*
 * showControlsScreen — displays key bindings reference.
 * Shown before the Tutorial path; dismissed by any key press.
 */
static void showControlsScreen(void)
{
    static const char *keys[] = {
        "Up / W",  "Down / S",  "Left / A", "Right / D",
        "Space",   "Tab",       "1 - 5",    "B",
        "[ / ]",   ";",         "Return"
    };
    static const char *actions[] = {
        "Accelerate",
        "Decelerate",
        "Turn anti-clockwise",
        "Turn clockwise",
        "Fire shell",
        "Quick-drop mine (visible to enemies)",
        "Select LGM build mode (Farm/Road/Building/Pillbox/Mine)",
        "Send LGM to gunsight tile with selected mode",
        "Gunsight range shorter / longer",
        "Cycle pillbox view (owned pillboxes)",
        "Return to tank view"
    };
    static const int ROWS = 11;
    const int rowH = 38;
    const int tableY = 100;
    const int keyX = 200;
    const int actX = 440;

    /* Flush any buffered key presses from the launcher */
    while (GetKeyPressed() != 0) {}

    while (!WindowShouldClose()) {
        if (GetKeyPressed() != 0) break;

        BeginDrawing();
        ClearBackground(COL_BG);

        drawText("Controls", 430, 40, 32, COL_GOLD);
        DrawRectangle(150, 88, 730, 1, (Color){80, 80, 80, 255});

        /* Column headers */
        drawText("Key", keyX, tableY - 30, 16, COL_DIM);
        DrawRectangle(430, tableY - 30, 1, ROWS * rowH + 30, (Color){60, 60, 60, 255});
        drawText("Action", actX, tableY - 30, 16, COL_DIM);

        for (int i = 0; i < ROWS; i++) {
            int y = tableY + i * rowH;
            /* Alternate row tint */
            if (i % 2 == 0)
                DrawRectangle(150, y - 4, 730, rowH - 2, (Color){20, 20, 20, 255});
            drawText(keys[i],    keyX, y, 18, COL_GOLD);
            drawText(actions[i], actX, y, 18, COL_GRAY);
        }

        DrawRectangle(150, tableY + ROWS * rowH + 8, 730, 1, (Color){80, 80, 80, 255});

        /* "Press any key" with blinking */
        if ((int)(GetTime() * 2) % 2 == 0)
            drawText("Press any key to continue",
                     360, 605, 16, COL_DIM);

        EndDrawing();
    }
}

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

        drawText("WinBolo", 390, 180, 40, (Color){255, 220, 0, 255});
        drawText("Enter your player name:", 320, 270, 20, (Color){200, 200, 200, 255});

        /* Name input box */
        DrawRectangle(300, 310, 430, 44, (Color){30, 30, 30, 255});
        DrawRectangleLines(300, 310, 430, 44, (Color){255, 220, 0, 255});
        drawText(buf, 312, 322, 24, (Color){255, 255, 255, 255});

        /* Blinking cursor */
        if ((int)(GetTime() * 2) % 2 == 0) {
            int tw = (int)MeasureTextEx(g_font, buf, 24, 1.0f).x;
            drawText("|", 312 + tw, 322, 24, (Color){255, 220, 0, 255});
        }

        drawText("ENTER to start   ESC for default (Player)",
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

    /* Load TTF font for all pre-game screens.
     * anonymous_pro_bold.ttf is bundled in fonts/ (OFL licence, shipped with
     * Raylib examples).  Raylib silently falls back to its bitmap default if
     * the file is missing — nothing else in main.c needs to change. */
    g_font = LoadFontEx("fonts/anonymous_pro_bold.ttf", 48, NULL, 0);
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);

    /* Launcher: Tutorial / Practice / Quit */
    int launch = launcherScreen();
    if (launch == 2 || WindowShouldClose()) goto cleanup;

    /* Tutorial shows controls reference; Practice goes straight to name entry */
    if (launch == 0) showControlsScreen();
    if (WindowShouldClose()) goto cleanup;

    /* Collect player name (Tutorial uses default "Player") */
    char playerName[PLAYER_NAME_MAX + 1];
    if (launch == 0) {
        strcpy(playerName, "Player");
    } else {
        enterPlayerName(playerName, sizeof(playerName));
    }
    if (WindowShouldClose()) goto cleanup;

    int mapLoaded = boloInit(TEST_MAP, playerName);

    /* Current LGM build mode (persists until changed).
     * 0=BsTrees(farm/capture) 1=BsRoad 2=BsBuilding 3=BsPillbox 4=BsMine(invisible) */
    int buildMode = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0, 0, 0, 255});

        if (mapLoaded) {
            boloUpdate();   /* screenUpdate(redraw) → frontEndDrawMainScreen() */
        } else {
            drawText("Map load FAILED: " TEST_MAP, 10, 10, 12, (Color){255, 80, 80, 255});
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

            /* Tab = quick-drop mine under tank (visible to nearby enemies). */
            if (IsKeyPressed(KEY_TAB)) boloLayMine();

            /* ; = cycle to next owned pillbox view. */
            if (IsKeyPressed(KEY_SEMICOLON)) boloPillView();

            /* Return = return viewport to own tank. */
            if (IsKeyPressed(KEY_ENTER)) boloTankView();

            /* While in pillbox view, arrow keys navigate adjacent pillboxes
             * instead of (only) moving the tank. */
            if (boloInPillView()) {
                if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) boloPillViewNav(-1,  0);
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) boloPillViewNav( 1,  0);
                if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) boloPillViewNav( 0, -1);
                if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) boloPillViewNav( 0,  1);
            }

            /* 1-5 = select LGM build mode (persists until changed):
             *   1 Farm/capture  2 Road  3 Building  4 Pillbox  5 Invisible mine */
            if (IsKeyPressed(KEY_ONE))   buildMode = 0;
            if (IsKeyPressed(KEY_TWO))   buildMode = 1;
            if (IsKeyPressed(KEY_THREE)) buildMode = 2;
            if (IsKeyPressed(KEY_FOUR))  buildMode = 3;
            if (IsKeyPressed(KEY_FIVE))  buildMode = 4;

            /* B = send LGM to gunsight tile with current build mode. */
            if (IsKeyPressed(KEY_B)) boloManMove(buildMode);
        }
    }

cleanup:
    UnloadFont(g_font);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}