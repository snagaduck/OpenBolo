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
 * win32stubs.c — Phase A2 stubs for excluded DirectX / WinMain files
 *
 * The six DirectX files (draw.c, dsutil.c, gamefront.c, input.c,
 * sound.c, winutil.c) and the WinMain file (winbolo.c) are excluded
 * because they depend on DirectX 5 headers or the Win32 message loop.
 * Phase B will replace them with Raylib equivalents.
 *
 * All stubs are no-ops or return safe default values.
 */

#include <windows.h>
#include <string.h>
#include <time.h>
#include "global.h"   /* bool, BYTE, etc. */
#include "backend.h"  /* aiType enum */

/* ---- servermain.c stubs ------------------------------------- */

/* serverMainGetTicks: returns the current game tick count.
 * servermain.c (which has the standalone server's main()) is excluded
 * from the client build.  Return the system tick counter instead. */
time_t serverMainGetTicks(void)
{
    return (time_t)GetTickCount64();
}

/* ---- winutil.c stubs ---------------------------------------- */

/* Called by brainsHandler.c — returns FALSE (no Brains subdir) */
BOOL winUtilWBSubDirExist(char *dirName)
{
    return FALSE;
}

/* ---- winbolo.c stubs ---------------------------------------- */

/* Application instance — NULL until Phase B wires Raylib's HINSTANCE */
HINSTANCE windowGetInstance(void)
{
    return NULL;
}

/* Main window handle — NULL until Phase B exposes Raylib's HWND */
HWND windowWnd(void)
{
    return NULL;
}

/* Alliance request popup — stub declines (returns FALSE) */
bool windowShowAllianceRequest(void)
{
    return FALSE;
}

/* Enable/disable player-name field in the UI — no-op */
void windowAllowPlayerNameChange(bool allow)
{
}

/* Millisecond tick counter used for RTT in network.c */
time_t windowsGetTicks(void)
{
    return (time_t)GetTickCount64();
}

/* ---- gamefront.c stubs -------------------------------------- */

/* Password entered in the game-setup dialog — Phase A2: empty */
void gameFrontGetPassword(char *pword)
{
    if (pword) pword[0] = '\0';
}

/* Player name — set once at startup via gameFrontSetPlayerName(). */
static char g_playerName[64] = "Player";

void gameFrontSetPlayerName(const char *name)
{
    if (!name || !name[0]) return;
    strncpy(g_playerName, name, sizeof(g_playerName) - 1);
    g_playerName[sizeof(g_playerName) - 1] = '\0';
}

void gameFrontGetPlayerName(char *pn)
{
    if (pn) strcpy(pn, g_playerName);
}

/* AI type configuration after joining a game — no-op */
void gameFrontSetAIType(aiType ait)
{
}

/* ---- dialogAlliance.c stubs --------------------------------- */

/* Win32 dialog procedure for alliance-request dialog — returns FALSE */
BOOL CALLBACK dialogAllianceCallback(HWND hWnd, unsigned uMsg,
                                     WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

/* Update alliance dialog player name — no-op */
void dialogAllianceSetName(HWND hWnd, HWND hParentWnd,
                           char *playerName, BYTE playerNum)
{
}