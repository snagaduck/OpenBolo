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
 * Returns: 0=Tutorial  1=Practice (single player)  2=TCP/IP  3=Quit */
static int launcherScreen(void)
{
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
    static const int NUM_SEL = 3; /* Tutorial, Practice, TCP/IP are selectable */
    static const int NUM_OPT = 5;

    while (!WindowShouldClose()) {
        /* Input */
        if (IsKeyPressed(KEY_UP)   && sel > 0)         sel--;
        if (IsKeyPressed(KEY_DOWN) && sel < NUM_SEL-1)  sel++;
        if (IsKeyPressed(KEY_ENTER)) return sel;
        if (IsKeyPressed(KEY_ESCAPE)) return 3; /* Quit */

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
    return 3; /* window closed = quit */
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

/* networkModeScreen — shown after the user picks TCP/IP.
 * Returns 0=Host, 1=Join, 2=Back. */
static int networkModeScreen(void)
{
    int sel = 0;
    static const char *labels[] = { "Host a new game", "Join an existing game" };
    static const int optY[] = { 260, 320 };

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_UP)   && sel > 0) sel--;
        if (IsKeyPressed(KEY_DOWN) && sel < 1)  sel++;
        if (IsKeyPressed(KEY_ENTER))  return sel;
        if (IsKeyPressed(KEY_ESCAPE)) return 2; /* Back */

        BeginDrawing();
        ClearBackground(COL_BG);

        drawText("TCP/IP Network", 370, 100, 32, COL_GOLD);
        DrawRectangle(150, 150, 730, 1, (Color){80, 80, 80, 255});

        for (int i = 0; i < 2; i++) {
            int cx = 250, cy = optY[i] + 10;
            Color tc = (i == sel) ? COL_WHITE : COL_GRAY;
            Color cc = (i == sel) ? COL_GOLD  : COL_GRAY;
            if (i == sel) {
                DrawCircle(cx, cy, 6, COL_GOLD);
                DrawCircleLines((float)cx, (float)cy, 8, COL_GOLD);
            } else {
                DrawCircleLines((float)cx, (float)cy, 7, cc);
            }
            drawText(labels[i], 278, optY[i], 22, tc);
        }

        drawText("Up/Down to select   Enter = OK   Esc = Back", 280, 500, 14, COL_DIM);
        EndDrawing();
    }
    return 2;
}

/* enterPort — number entry for a UDP port.
 * Returns the entered port, or 0 if the user pressed Escape. */
static unsigned short enterPort(unsigned short defaultPort)
{
    char buf[8];
    int len = 0;

    /* Pre-fill with default */
    TextCopy(buf, TextFormat("%d", (int)defaultPort));
    len = (int)TextLength(buf);

    while (!WindowShouldClose()) {
        int ch = GetCharPressed();
        while (ch > 0) {
            if (ch >= '0' && ch <= '9' && len < 5) {
                buf[len++] = (char)ch;
                buf[len]   = '\0';
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && len > 0) buf[--len] = '\0';
        if (IsKeyPressed(KEY_ENTER) && len > 0) {
            int p = TextToInteger(buf);
            if (p > 0 && p <= 65535) return (unsigned short)p;
        }
        if (IsKeyPressed(KEY_ESCAPE)) return 0;

        BeginDrawing();
        ClearBackground(COL_BG);
        drawText("Host — Choose Port", 360, 180, 28, COL_GOLD);
        drawText("Enter the UDP port to listen on:", 305, 260, 18, COL_GRAY);
        DrawRectangle(330, 300, 370, 44, COL_DARK);
        DrawRectangleLines(330, 300, 370, 44, COL_GOLD);
        drawText(buf, 342, 312, 24, COL_WHITE);
        if ((int)(GetTime() * 2) % 2 == 0) {
            int tw = (int)MeasureTextEx(g_font, buf, 24, 1.0f).x;
            drawText("|", 342 + tw, 312, 24, COL_GOLD);
        }
        drawText("ENTER to confirm   ESC = Back", 325, 370, 16, COL_DIM);
        EndDrawing();
    }
    return 0;
}

/* enterIPPort — IP:port entry for joining a game.
 * Returns 1 on OK, 0 on Escape/close.
 * ipBuf must be at least 64 bytes; port is filled on success. */
static int enterIPPort(char *ipBuf, int ipLen, unsigned short *port)
{
    char ipEntry[64]   = "127.0.0.1";
    char portEntry[8]  = "27500";
    int  ipFocus       = 1;   /* 1=editing IP, 0=editing Port */
    int  ipLen2        = (int)TextLength(ipEntry);
    int  portLen       = (int)TextLength(portEntry);

    while (!WindowShouldClose()) {
        int ch = GetCharPressed();
        while (ch > 0) {
            if (ipFocus) {
                if ((ch == '.' || (ch >= '0' && ch <= '9')) && ipLen2 < 15) {
                    ipEntry[ipLen2++] = (char)ch;
                    ipEntry[ipLen2]   = '\0';
                }
            } else {
                if (ch >= '0' && ch <= '9' && portLen < 5) {
                    portEntry[portLen++] = (char)ch;
                    portEntry[portLen]   = '\0';
                }
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (ipFocus && ipLen2 > 0) ipEntry[--ipLen2]     = '\0';
            else if (!ipFocus && portLen > 0) portEntry[--portLen] = '\0';
        }
        if (IsKeyPressed(KEY_TAB)) ipFocus = !ipFocus;
        if (IsKeyPressed(KEY_ENTER) && ipLen2 > 0 && portLen > 0) {
            int p = TextToInteger(portEntry);
            if (p > 0 && p <= 65535) {
                TextCopy(ipBuf, ipEntry);
                *port = (unsigned short)p;
                return 1;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) return 0;

        BeginDrawing();
        ClearBackground(COL_BG);
        drawText("Join Game", 420, 160, 32, COL_GOLD);

        drawText("Server IP:", 310, 260, 18, COL_GRAY);
        DrawRectangle(310, 286, 410, 40, COL_DARK);
        DrawRectangleLines(310, 286, 410, 40, ipFocus ? COL_GOLD : COL_GRAY);
        drawText(ipEntry, 322, 296, 22, COL_WHITE);
        if (ipFocus && (int)(GetTime() * 2) % 2 == 0) {
            int tw = (int)MeasureTextEx(g_font, ipEntry, 22, 1.0f).x;
            drawText("|", 322 + tw, 296, 22, COL_GOLD);
        }

        drawText("Port:", 310, 344, 18, COL_GRAY);
        DrawRectangle(310, 368, 180, 40, COL_DARK);
        DrawRectangleLines(310, 368, 180, 40, ipFocus ? COL_GRAY : COL_GOLD);
        drawText(portEntry, 322, 378, 22, COL_WHITE);
        if (!ipFocus && (int)(GetTime() * 2) % 2 == 0) {
            int tw = (int)MeasureTextEx(g_font, portEntry, 22, 1.0f).x;
            drawText("|", 322 + tw, 378, 22, COL_GOLD);
        }

        drawText("Tab to switch fields   ENTER = Connect   ESC = Back",
                 225, 460, 14, COL_DIM);
        EndDrawing();
    }
    return 0;
}

/* connectingScreen — spin until boloNetStatus() returns RUNNING or FAILED.
 * Returns 1 if the game is running, 0 on failure or window close. */
static int connectingScreen(void)
{
    double startTime = GetTime();
    const double timeout = 30.0;   /* 30 s connection timeout */
    static const char *statusNames[] = {
        "Joining...",
        "Running",
        "Preparing download...",
        "Downloading bases...",
        "Downloading pillboxes...",
        "Downloading map...",
        "Synchronising time...",
        "Connection failed."
    };

    while (!WindowShouldClose()) {
        boloUpdate();   /* pump ENet events so the download progresses */

        int status = boloNetStatus();
        if (status == BOLO_NET_RUNNING) return 1;
        if (status == BOLO_NET_FAILED)  break;
        if (GetTime() - startTime > timeout) break;

        if (IsKeyPressed(KEY_ESCAPE)) break;

        BeginDrawing();
        ClearBackground(COL_BG);
        drawText("Connecting...", 400, 220, 28, COL_GOLD);

        const char *statusStr = (status >= 0 && status <= 7)
                                ? statusNames[status] : "...";
        drawText(statusStr, 440, 278, 18, COL_GRAY);

        /* Animated dots */
        int dots = (int)(GetTime() * 2) % 4;
        char dotBuf[5] = "";
        for (int i = 0; i < dots; i++) dotBuf[i] = '.';
        dotBuf[dots] = '\0';
        drawText(dotBuf, 660, 278, 18, COL_GRAY);

        drawText("ESC = Cancel", 450, 400, 14, COL_DIM);
        EndDrawing();
    }
    return 0;
}

/* runGameLoop — runs the in-game loop.  mapLoaded is passed so the loop
 * can show an error message if the map failed to initialise. */
static void runGameLoop(int mapLoaded)
{
    int buildMode = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0, 0, 0, 255});

        if (mapLoaded) {
            boloUpdate();
        } else {
            drawText("Map load FAILED: " TEST_MAP, 10, 10, 12, (Color){255, 80, 80, 255});
        }

        EndDrawing();

        if (mapLoaded) {
            int fwd  = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
            int bwd  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
            int lft  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
            int rgt  = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
            int fire = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_LEFT_CONTROL);

            int tb;
            if      (fwd && rgt) tb = 6;
            else if (fwd && lft) tb = 5;
            else if (bwd && lft) tb = 7;
            else if (bwd && rgt) tb = 8;
            else if (fwd)        tb = 3;
            else if (bwd)        tb = 4;
            else if (lft)        tb = 1;
            else if (rgt)        tb = 2;
            else                 tb = 0;

            boloTick(tb, fire);

            if (IsKeyPressed(KEY_LEFT_BRACKET))  boloGunsightRange(0);
            if (IsKeyPressed(KEY_RIGHT_BRACKET)) boloGunsightRange(1);
            if (IsKeyPressed(KEY_TAB))           boloLayMine();
            if (IsKeyPressed(KEY_SEMICOLON))     boloPillView();
            if (IsKeyPressed(KEY_ENTER))         boloTankView();

            if (boloInPillView()) {
                if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) boloPillViewNav(-1,  0);
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) boloPillViewNav( 1,  0);
                if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) boloPillViewNav( 0, -1);
                if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) boloPillViewNav( 0,  1);
            }

            if (IsKeyPressed(KEY_ONE))   buildMode = 0;
            if (IsKeyPressed(KEY_TWO))   buildMode = 1;
            if (IsKeyPressed(KEY_THREE)) buildMode = 2;
            if (IsKeyPressed(KEY_FOUR))  buildMode = 3;
            if (IsKeyPressed(KEY_FIVE))  buildMode = 4;
            if (IsKeyPressed(KEY_B))     boloManMove(buildMode);
        }
    }
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

    /* Main launcher loop — allows returning to the launcher after a game. */
    while (!WindowShouldClose()) {
        int launch = launcherScreen();
        if (launch == 3 || WindowShouldClose()) break; /* Quit */

        /* Collect player name */
        char playerName[PLAYER_NAME_MAX + 1];
        if (launch == 0) {
            /* Tutorial: show controls reference, use default name */
            showControlsScreen();
            if (WindowShouldClose()) break;
            strcpy(playerName, "Player");
        } else if (launch == 1) {
            /* Practice: name entry, then solo game */
            enterPlayerName(playerName, sizeof(playerName));
            if (WindowShouldClose()) break;
        } else {
            /* TCP/IP: shared name entry used for both Host and Join */
            enterPlayerName(playerName, sizeof(playerName));
            if (WindowShouldClose()) break;
        }

        if (launch == 2) {
            /* ── TCP/IP path ───────────────────────────────────── */
            int netMode = networkModeScreen();
            if (netMode == 2 || WindowShouldClose()) continue; /* Back */

            if (netMode == 0) {
                /* Host */
                unsigned short port = enterPort(27500);
                if (!port || WindowShouldClose()) continue;

                int mapLoaded = boloInit(TEST_MAP, playerName);
                if (mapLoaded && boloHost(port, playerName)) {
                    /* Brief connecting phase while the local client joins
                     * the embedded server. */
                    if (connectingScreen()) {
                        runGameLoop(1);
                    }
                } else if (!mapLoaded) {
                    runGameLoop(0); /* show error screen */
                }
            } else {
                /* Join */
                char ip[64];
                unsigned short port = 0;
                if (!enterIPPort(ip, sizeof(ip), &port) || WindowShouldClose())
                    continue;

                if (boloJoin(ip, port, playerName)) {
                    if (connectingScreen()) {
                        runGameLoop(1);
                    }
                }
            }
        } else {
            /* ── Solo path (Tutorial / Practice) ──────────────── */
            int mapLoaded = boloInit(TEST_MAP, playerName);
            runGameLoop(mapLoaded);
        }
    }

cleanup:
    UnloadFont(g_font);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}