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

#include <stdio.h>
#include "global.h"        /* bool, BYTE, UNLIMITED_GAME_TIME, etc. */
#include "backend.h"       /* screenLoadMap, screenGameTick, screenUpdate,
                              gameOpen, TNONE, redraw */
#include "network.h"       /* netSetType, netSingle, netSetup, netGetStatus */
#include "servernet.h"     /* serverNetCreate */
#include "servercore.h"    /* serverCoreCreate */
#include "threads.h"       /* threadsCreate */
#include "servertransport.h" /* serverTransportListenUDP */
#include "netclient.h"     /* netClientUdpCheck */

/* Defined in win32stubs.c — keeps gameFrontGetPlayerName() in sync. */
extern void gameFrontSetPlayerName(const char *name);

/* net_log — append a diagnostic line to net_debug.log next to the exe.
 * Written before/after every blocking network call so a post-crash read
 * shows exactly where execution stopped. */
static void net_log(const char *msg)
{
    FILE *f = fopen("net_debug.log", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

int boloInit(const char *mapFile, const char *playerName)
{
    /* Truncate log at the start of each new game attempt */
    { FILE *f = fopen("net_debug.log", "w"); if (f) fclose(f); }

    const char *name = (playerName && playerName[0]) ? playerName : "Player";
    net_log("boloInit: start");
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
        net_log("boloInit: screenLoadMap OK");
    } else {
        net_log("boloInit: screenLoadMap FAILED");
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
    static int firstFrame = 1;
    if (firstFrame) {
        char msg[128];
        firstFrame = 0;
        sprintf(msg, "boloUpdate: first frame netStatus=%d threadsCtx=%d",
                (int)netGetStatus(), (int)threadsGetContext());
        net_log(msg);
    }
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

int boloHost(const char *mapFile, unsigned short port, const char *playerName)
{
    /* Truncate log so this run's entries are always visible. */
    { FILE *f = fopen("net_debug.log", "w"); if (f) fclose(f); }

    const char *name = (playerName && playerName[0]) ? playerName : "Player";
    net_log("boloHost: start");
    gameFrontSetPlayerName(name);

    /* ── Server side init ──────────────────────────────────────────────── */
    /* serverCoreCreate initialises pb, bs, splrs etc. in servercore.c.
     * These are dereferenced (as pointers) by serverNetMakeInfoRespsonse
     * during the netSetup join handshake — crash if called while NULL. */
    net_log("boloHost: calling serverCoreCreate");
    if (!serverCoreCreate((char *)mapFile, gameOpen, FALSE, 0,
                           UNLIMITED_GAME_TIME)) {
        net_log("boloHost: serverCoreCreate FAILED");
        return 0;
    }
    net_log("boloHost: serverCoreCreate OK");

    net_log("boloHost: calling serverNetCreate");
    if (!serverNetCreate(port, "", aiNone, "", 0, FALSE, "", 0)) {
        net_log("boloHost: serverNetCreate FAILED");
        return 0;
    }
    net_log("boloHost: serverNetCreate OK");

    /* threadsCreate builds the mutex used by threadsWaitForMutex /
     * threadsReleaseMutex inside serverNetUDPPacketArrive.
     *
     * Pass FALSE (client context) so screencReCalc() → needScreenReCalc=TRUE
     * fires correctly.  screenReCalc() skips when threadsGetContext()==TRUE
     * (server context), which would leave the view buffer unpopulated and
     * produce a blank black tile viewport.
     *
     * Server-side functions in servercore.c / servernet.c access their data
     * directly and do not rely on the context flag, so FALSE is safe here. */
    net_log("boloHost: calling threadsCreate");
    if (!threadsCreate(FALSE)) {
        net_log("boloHost: threadsCreate FAILED");
        return 0;
    }
    net_log("boloHost: threadsCreate OK");

    /* ── Client side init ──────────────────────────────────────────────── */
    /* screenSetup allocates mymp, mypb, mybs, myss etc. in screen.c.
     * These are written to by the download handlers (screenSetMapNetRun,
     * screenSetBaseNetData, …) — crash if called while NULL. */
    screenSetup(gameOpen, FALSE, 0, UNLIMITED_GAME_TIME);
    net_log("boloHost: screenSetup OK — calling netSetup");

    if (!netSetup(netUdp, port, "127.0.0.1", port, "",
                  TRUE, "", 0, FALSE, FALSE, FALSE, "")) {
        net_log("boloHost: netSetup FAILED");
        return 0;
    }
    net_log("boloHost: netSetup OK — returning 1");
    return 1;
}

int boloJoin(const char *ip, unsigned short port, const char *playerName)
{
    /* Truncate the net log at the start of each join attempt.
     * enet_debug_<PID>.log is per-process and never needs truncating here. */
    { FILE *f = fopen("net_debug.log", "w"); if (f) fclose(f); }

    const char *name = (playerName && playerName[0]) ? playerName : "Player";
    net_log("boloJoin: start");
    gameFrontSetPlayerName(name);

    /* screenSetup allocates mymp, mypb, mybs, myss etc. in screen.c.
     * These are written to by the download handlers before netRunning. */
    screenSetup(gameOpen, FALSE, 0, UNLIMITED_GAME_TIME);
    net_log("boloJoin: screenSetup OK — calling netSetup");

    if (!netSetup(netUdp, 0, (char *)ip, port, "",
                  FALSE, "", 0, FALSE, FALSE, FALSE, "")) {
        net_log("boloJoin: netSetup FAILED");
        return 0;
    }
    net_log("boloJoin: netSetup OK — returning 1");
    return 1;
}

int boloNetStatus(void)
{
    return (int)netGetStatus();
}

void boloNetPoll(void)
{
    serverTransportListenUDP(); /* poll ENet server events (non-blocking) */
    netClientUdpCheck();        /* poll ENet client receive (non-blocking) */
}

/* boloLogMsg — thin wrapper so main.c (which can't include bolo headers)
 * can append a line to the diagnostic log. */
void boloLogMsg(const char *msg)
{
    net_log(msg);
}

/* boloNetPostConnect — called once after connectingScreen() returns RUNNING.
 * Applies the client-side view settings that boloInit() would normally set,
 * without resetting the map data that the download state machine installed. */
void boloNetPostConnect(void)
{
    screenSetGunsight(TRUE);    /* gunsight defaults off */
    screenSetAutoScroll(TRUE);  /* center viewport on gunsight */
    screenForceStatusUpdate();  /* push initial shells/mines/armour/trees to HUD */
}