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
 * game_loop.c — Phase B1 game loop bridge.
 *
 * Compiled WITH /FI winbolo_platform.h so it has full Windows + bolo
 * type visibility.  Exposes a minimal C-standard API (game_loop.h) for
 * main.c to call without pulling in Windows or bolo headers.
 *
 * Call sequence each frame:
 *   boloTick()   — 20 Hz game logic (screenGameTick)
 *   boloUpdate() — render callback (screenUpdate → frontEndDrawMainScreen)
 */

#include "global.h"        /* bool, BYTE, UNLIMITED_GAME_TIME, etc. */
#include "backend.h"       /* screenLoadMap, screenGameTick, screenUpdate,
                              gameOpen, TNONE, redraw */
#include "network.h"       /* netSetType, netSingle, netSetup, netGetStatus */
#include "servernet.h"     /* serverNetCreate */
#include "servertransport.h" /* serverTransportListenUDP */
#include "netclient.h"     /* netClientUdpCheck */

/* Defined in win32stubs.c — keeps gameFrontGetPlayerName() in sync. */
extern void gameFrontSetPlayerName(const char *name);

int boloInit(const char *mapFile, const char *playerName)
{
    const char *name = (playerName && playerName[0]) ? playerName : "Player";
    gameFrontSetPlayerName(name);

    int ok = (int)screenLoadMap((char *)mapFile,
                                gameOpen,
                                FALSE,               /* hiddenMines */
                                0,                   /* srtDelay    */
                                UNLIMITED_GAME_TIME, /* gmeLen      */
                                (char *)name,
                                FALSE);              /* wantFree    */
    if (ok) {
        /* networkGameType initialises to netNone, so tankCreate does not
         * clear inStart.  Clear it explicitly so screenUpdate proceeds to
         * call frontEndDrawMainScreen instead of frontEndDrawDownload. */
        screenSetInStartFind(FALSE);
        screenSetGunsight(TRUE);    /* showSight defaults FALSE; force it on */
        screenSetAutoScroll(TRUE);  /* center viewport on gunsight, not tank */
        netSetType(netSingle);      /* enable single-player base/pill capture */
        screenForceStatusUpdate();  /* push initial shells/mines/armour/trees to HUD */
    }
    return ok;
}

void boloTick(int tbOrdinal, int shoot)
{
    tankButton tb = (tankButton)tbOrdinal;
    /* Mirror the original alternating-tick pattern:
     *   screenKeysTick fires at the key-update rate (input processing),
     *   screenGameTick fires at the full game-logic rate.
     * Calling both here each frame is correct for 20 Hz; the engine
     * guards internally if the game has not yet started. */
    screenKeysTick(tb, FALSE);
    screenGameTick(tb, (bool)shoot, FALSE);
}

void boloUpdate(void)
{
    serverTransportListenUDP(); /* poll ENet server events (non-blocking) */
    netClientUdpCheck();        /* poll ENet client receive (non-blocking) */
    screenUpdate(redraw);
}

void boloGunsightRange(int increase)
{
    screenGunsightRange((bool)increase);
}

void boloManMove(int buildType)
{
    screenManMoveAtGunsight((buildSelect)buildType);
}

/* Track whether the viewport is on a pillbox or on the player's tank.
 * screenPillView/screenTankView manage the engine's own inPillView flag;
 * we mirror it here so main.c (no bolo headers) can query the state. */
static int g_inPillView = 0;

void boloLayMine(void)
{
    screenTankLayMine();
}

void boloPillView(void)
{
    screenPillView(0, 0);
    g_inPillView = 1;
}

void boloTankView(void)
{
    screenTankView();
    g_inPillView = 0;
}

void boloPillViewNav(int horz, int vert)
{
    screenPillView(horz, vert);
}

int boloInPillView(void)
{
    return g_inPillView;
}

/* ── Phase C: ENet multiplayer ──────────────────────────────────────────── */

int boloHost(unsigned short port, const char *playerName)
{
    const char *name = (playerName && playerName[0]) ? playerName : "Player";
    gameFrontSetPlayerName(name);

    /* Start the embedded server on the requested port. */
    if (!serverNetCreate(port, "", aiNone, "", 0, FALSE, "", 0)) {
        return 0;
    }

    /* Connect the local client to the local server (usCreate=TRUE). */
    if (!netSetup(netUdp, port, "127.0.0.1", port, "",
                  TRUE, "", 0, FALSE, FALSE, FALSE, "")) {
        return 0;
    }
    return 1;
}

int boloJoin(const char *ip, unsigned short port, const char *playerName)
{
    const char *name = (playerName && playerName[0]) ? playerName : "Player";
    gameFrontSetPlayerName(name);

    /* Connect to an existing remote server (usCreate=FALSE). */
    if (!netSetup(netUdp, 0, (char *)ip, port, "",
                  FALSE, "", 0, FALSE, FALSE, FALSE, "")) {
        return 0;
    }
    return 1;
}

int boloNetStatus(void)
{
    return (int)netGetStatus();
}