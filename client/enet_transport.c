/*
 * OpenBolo — cross-platform remake of WinBolo (1998)
 * Copyright (C) 2026 OpenBolo Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * enet_transport.c — ENet replacement for:
 *   gui/win32/netclient.c        (client-side UDP transport)
 *   server/win32/servertransport.c (server-side UDP transport)
 *
 * Pure C, zero extra dependencies beyond ENet itself.
 * Compiled WITH /FI winbolo_platform.h so Windows types are available.
 *
 * Connection model
 * ----------------
 * ENet is connection-oriented; the original code is address-oriented (IP+port).
 * We bridge them with a peer table that assigns each ENet peer a "fake port"
 * (27501 + slot index) so servernet.c can correlate packets to peers without
 * any changes to the upper-layer protocol logic.
 *
 * Channel usage
 * -------------
 *   Channel 0 — all game traffic (unreliable by default)
 *   Channel 1 — join-handshake ping packets (reliable, used by netClientUdpPing)
 *
 * CRC handling
 * ------------
 * serverTransportSendUDPLast with wantCrc==TRUE appends a 2-byte CRC to the
 * buffer before sending, matching the original behaviour.
 */

#include <enet/enet.h>
#include <string.h>
#include <stdio.h>

/* Pull in bolo types (BYTE, bool, TRUE, FALSE) via the force-included platform
 * header.  global.h also defines typedef BYTE bool — fine here since this is
 * a .c file compiled in C mode where bool is not a keyword. */
#include "global.h"
#include "crc.h"

/* ── Forward declarations for upper-layer callbacks ─────────────────────── */
/* Defined in network.c — called when a packet arrives on the CLIENT side.   */
void netUdpPacketArrive(BYTE *buff, int len, unsigned short port);
/* Defined in servernet.c — called when a packet arrives on the SERVER side. */
void serverNetUDPPacketArrive(BYTE *buff, int len,
                               unsigned long addr, unsigned short port);

/* ── Peer table ─────────────────────────────────────────────────────────── */
#define ENET_MAX_PEERS 16
#define FAKE_PORT_BASE 27501u   /* fake ports: 27501 … 27516 */

typedef struct {
    ENetPeer      *peer;
    unsigned short fakePort;
    struct in_addr fakeAddr;    /* 127.0.1.N — distinguishable per slot       */
    int            active;
} ENetPeerSlot;

static ENetPeerSlot g_peers[ENET_MAX_PEERS];

static int allocPeerSlot(ENetPeer *peer)
{
    int i;
    for (i = 0; i < ENET_MAX_PEERS; i++) {
        if (!g_peers[i].active) {
            g_peers[i].peer     = peer;
            g_peers[i].fakePort = (unsigned short)(FAKE_PORT_BASE + (unsigned)i);
            /* 127.0.1.N in host byte order → store in network byte order     */
            g_peers[i].fakeAddr.s_addr = htonl(0x7f000100u | (unsigned)i);
            g_peers[i].active   = 1;
            return i;
        }
    }
    return -1;
}

static void freePeerSlot(ENetPeer *peer)
{
    int i;
    for (i = 0; i < ENET_MAX_PEERS; i++) {
        if (g_peers[i].active && g_peers[i].peer == peer) {
            g_peers[i].active = 0;
            g_peers[i].peer   = NULL;
            return;
        }
    }
}

static ENetPeerSlot *findSlotByPeer(ENetPeer *peer)
{
    int i;
    for (i = 0; i < ENET_MAX_PEERS; i++) {
        if (g_peers[i].active && g_peers[i].peer == peer)
            return &g_peers[i];
    }
    return NULL;
}

static ENetPeerSlot *findSlotByFakePort(unsigned short fakePort)
{
    int i;
    for (i = 0; i < ENET_MAX_PEERS; i++) {
        if (g_peers[i].active && g_peers[i].fakePort == fakePort)
            return &g_peers[i];
    }
    return NULL;
}

/* ── Shared init state ──────────────────────────────────────────────────── */
static int g_enetInitialized = 0;

/* enet_log — append a diagnostic line to enet_debug_<PID>.log.
 * Each process gets its own log file, so HOST and JOINER never overwrite
 * each other.  After a test run, share both files for diagnosis. */
static void enet_log(const char *msg)
{
    char fname[64];
    sprintf(fname, "enet_debug_%u.log", (unsigned)GetCurrentProcessId());
    FILE *f = fopen(fname, "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

static bool gnsEnsureInit(void)
{
    if (g_enetInitialized) return TRUE;
    if (enet_initialize() != 0) {
        fprintf(stderr, "[enet] enet_initialize() failed\n");
        enet_log("gnsEnsureInit: enet_initialize FAILED");
        return FALSE;
    }
    memset(g_peers, 0, sizeof(g_peers));
    g_enetInitialized = 1;
    enet_log("gnsEnsureInit: enet_initialize OK");
    return TRUE;
}

/* ════════════════════════════════════════════════════════════════════════════
 * SERVER SIDE  (replaces server/win32/servertransport.c)
 * ════════════════════════════════════════════════════════════════════════════ */

static ENetHost   *g_server       = NULL;
static ENetPeer   *g_lastSrvPeer  = NULL;   /* last peer to send us a packet  */
static unsigned short g_serverPort = 0;

bool serverTransportCreate(unsigned short port, char *addrToUse)
{
    ENetAddress addr;
    (void)addrToUse;  /* TODO: bind to specific IP when addrToUse != NULL     */

    if (!gnsEnsureInit()) return FALSE;

    addr.host = ENET_HOST_ANY;
    addr.port = port;
    g_serverPort = port;

    /* Up to 16 peers, 2 channels, unlimited bandwidth                        */
    g_server = enet_host_create(&addr, ENET_MAX_PEERS, 2, 0, 0);
    if (!g_server) {
        fprintf(stderr, "[enet] enet_host_create (server) failed on port %u\n",
                (unsigned)port);
        return FALSE;
    }
    return TRUE;
}

void serverTransportDestroy(void)
{
    if (g_server) {
        enet_host_destroy(g_server);
        g_server = NULL;
    }
    g_lastSrvPeer = NULL;
}

/*
 * serverTransportListenUDP — NON-BLOCKING poll (called every game tick from
 * boloUpdate).  The original blocking loop is replaced by a drain loop with
 * timeout=0.
 */
void serverTransportListenUDP(void)
{
    static unsigned srvCallCount = 0;
    ENetEvent ev;
    if (!g_server) return;

    /* Heartbeat: log every 100th call so we can confirm the server is still
     * being polled when the joiner tries to connect. */
    if (++srvCallCount % 100 == 0) {
        char hb[64];
        sprintf(hb, "serverTransportListenUDP: heartbeat count=%u", srvCallCount);
        enet_log(hb);
    }

    while (enet_host_service(g_server, &ev, 0) > 0) {
        switch (ev.type) {

        case ENET_EVENT_TYPE_CONNECT: {
            /* New client connected — assign fake port so servernet.c can      *
             * address replies back to it.                                     */
            int idx = allocPeerSlot(ev.peer);
            if (idx < 0) {
                fprintf(stderr, "[enet] peer table full — rejecting connection\n");
                enet_log("serverTransportListenUDP: CONNECT — peer table FULL, rejecting");
                enet_peer_disconnect(ev.peer, 0);
            } else {
                char cmsg[128];
                sprintf(cmsg,
                    "serverTransportListenUDP: CONNECT peer=%u slot=%d fakePort=%u",
                    (unsigned)ev.peer->address.port, idx,
                    (unsigned)g_peers[idx].fakePort);
                enet_log(cmsg);
                ev.peer->data = (void *)(size_t)idx; /* store slot index in peer */
            }
            break;
        }

        case ENET_EVENT_TYPE_RECEIVE: {
            ENetPeerSlot *slot = findSlotByPeer(ev.peer);
            {
                char rmsg[128];
                sprintf(rmsg,
                    "serverTransportListenUDP: RECEIVE peer=%u chan=%u len=%u slot=%s pktType=0x%02x",
                    (unsigned)ev.peer->address.port,
                    (unsigned)ev.channelID,
                    (unsigned)ev.packet->dataLength,
                    slot ? "OK" : "NULL",
                    ev.packet->dataLength > 0
                        ? (unsigned)(((BYTE*)ev.packet->data)[0]) : 0xFF);
                enet_log(rmsg);
            }
            if (slot) {
                g_lastSrvPeer = ev.peer;
                serverNetUDPPacketArrive(
                    (BYTE *)ev.packet->data,
                    (int)ev.packet->dataLength,
                    (unsigned long)slot->fakeAddr.s_addr,
                    slot->fakePort);
            }
            enet_packet_destroy(ev.packet);
            break;
        }

        case ENET_EVENT_TYPE_DISCONNECT:
            enet_log("serverTransportListenUDP: DISCONNECT");
            freePeerSlot(ev.peer);
            if (g_lastSrvPeer == ev.peer) g_lastSrvPeer = NULL;
            ev.peer->data = NULL;
            break;

        default:
            break;
        }
    }
}

void serverTransportSetUs(void)
{
    /* ENet manages the local address automatically; nothing to do here.      */
}

void serverTransportGetUs(struct in_addr *dest, unsigned short *port)
{
    dest->s_addr = htonl(0x7f000001u);  /* 127.0.0.1 — good enough for solo  */
    *port        = htons(g_serverPort);
}

void serverTransportSendUDPLast(BYTE *buff, int len, bool wantCrc)
{
    ENetPacket *pkt;
    BYTE        crcBuf[2048];

    if (!g_lastSrvPeer) {
        enet_log("serverTransportSendUDPLast: g_lastSrvPeer is NULL — packet dropped");
        return;
    }
    {
        char smsg[128];
        sprintf(smsg,
            "serverTransportSendUDPLast: sending len=%d wantCrc=%d to peer port=%u pktType=0x%02x",
            len, (int)wantCrc,
            (unsigned)g_lastSrvPeer->address.port,
            len > 0 ? (unsigned)buff[0] : 0xFF);
        enet_log(smsg);
    }

    if (wantCrc && len + 2 <= (int)sizeof(crcBuf)) {
        BYTE crcA, crcB;
        CRCCalcBytes(buff, len, &crcA, &crcB);
        memcpy(crcBuf, buff, (size_t)len);
        crcBuf[len]     = crcA;
        crcBuf[len + 1] = crcB;
        pkt = enet_packet_create(crcBuf, (size_t)(len + 2),
                                 ENET_PACKET_FLAG_UNSEQUENCED);
    } else {
        pkt = enet_packet_create(buff, (size_t)len,
                                 ENET_PACKET_FLAG_UNSEQUENCED);
    }

    if (pkt)
        enet_peer_send(g_lastSrvPeer, 0, pkt);
}

/* Generic send to an arbitrary peer by sockaddr_in (used by servernet.c).   */
void serverTransportSendUDP(BYTE *buff, int len, struct sockaddr_in *addr)
{
    ENetPeerSlot *slot;
    ENetPacket   *pkt;

    slot = findSlotByFakePort(ntohs(addr->sin_port));
    if (!slot) return;

    pkt = enet_packet_create(buff, (size_t)len, ENET_PACKET_FLAG_UNSEQUENCED);
    if (pkt)
        enet_peer_send(slot->peer, 0, pkt);
}

bool serverTransportSetTracker(char *address, unsigned short port)
{
    (void)address; (void)port;
    return TRUE;  /* Tracker not implemented in Phase C; stub succeeds.       */
}

void serverTransportSendUdpTracker(BYTE *buff, int len)
{
    (void)buff; (void)len;  /* Stub — tracker not used in Phase C.            */
}

void serverTransportDoChecks(void)
{
    serverTransportListenUDP();
}

/* ════════════════════════════════════════════════════════════════════════════
 * CLIENT SIDE  (replaces gui/win32/netclient.c)
 * ════════════════════════════════════════════════════════════════════════════ */

static ENetHost  *g_client      = NULL;
static ENetPeer  *g_serverConn  = NULL;   /* connection to the game server    */
static ENetAddress g_serverAddr;          /* address set by netClientSetServerAddress */
static struct in_addr g_ourAddr;
static unsigned short g_ourPort = 0;
static ENetPeer  *g_lastCliPeer = NULL;   /* last peer to send us a packet    */

bool netClientCreate(unsigned short port)
{
    ENetAddress localAddr;

    if (!gnsEnsureInit()) return FALSE;

    /* If the server is already bound to the requested port (i.e. this process
     * is both server and client, as in the Host case), we must NOT bind the
     * client to the same port — that would fail with EADDRINUSE.  Use port=0
     * (OS-assigned ephemeral port) instead.                                  */
    if (port != 0 && g_server != NULL && g_server->address.port == port) {
        port = 0;
    }

    localAddr.host = ENET_HOST_ANY;
    localAddr.port = port;

    if (port == 0) {
        g_client = enet_host_create(NULL, 1, 2, 0, 0);
    } else {
        g_client = enet_host_create(&localAddr, 1, 2, 0, 0);
    }

    if (!g_client) {
        fprintf(stderr, "[enet] enet_host_create (client) failed on port %u\n",
                (unsigned)port);
        return FALSE;
    }

    g_ourPort = port ? port : (unsigned short)g_client->address.port;
    g_ourAddr.s_addr = htonl(0x7f000001u);  /* 127.0.0.1 until SetUs is called */
    return TRUE;
}

void netClientDestroy(void)
{
    if (g_serverConn) {
        enet_peer_disconnect(g_serverConn, 0);
        g_serverConn = NULL;
    }
    if (g_client) {
        enet_host_destroy(g_client);
        g_client = NULL;
    }
    if (g_enetInitialized) {
        enet_deinitialize();
        g_enetInitialized = 0;
    }
}

void netClientCheck(void)
{
    /* no-op: actual polling happens in netClientUdpCheck() */
}

void netClientSendUdpLast(BYTE *buff, int len)
{
    ENetPacket *pkt;
    if (!g_serverConn) return;
    pkt = enet_packet_create(buff, (size_t)len, ENET_PACKET_FLAG_UNSEQUENCED);
    if (pkt)
        enet_peer_send(g_serverConn, 0, pkt);
}

void netClientSendUdpServer(BYTE *buff, int len)
{
    ENetPacket *pkt;
    if (!g_serverConn) return;
    pkt = enet_packet_create(buff, (size_t)len, ENET_PACKET_FLAG_UNSEQUENCED);
    if (pkt)
        enet_peer_send(g_serverConn, 0, pkt);
}

void netClientGetServerAddress(struct in_addr *dest, unsigned short *port)
{
    dest->s_addr = htonl(g_serverAddr.host);
    *port        = htons(g_serverAddr.port);
}

void netClientGetServerAddressString(char *dest)
{
    char buf[64];
    enet_address_get_host_ip(&g_serverAddr, buf, sizeof(buf));
    snprintf(dest, 64, "%s:%u", buf, (unsigned)g_serverAddr.port);
}

void netClientSetServerAddress(struct in_addr *src, unsigned short port)
{
    /* src->s_addr is in network byte order (from netClientGetLast / INFO_PACKET).
     * port has already been through ntohs() by the caller (netJoinInit), so it
     * is in host byte order — do NOT apply ntohs() again. */
    g_serverAddr.host = ntohl(src->s_addr);
    g_serverAddr.port = port;
}

void netClientSetServerPort(unsigned short port)
{
    g_serverAddr.port = ntohs(port);
}

void netClientSetUs(void)
{
    /* For LAN play, 127.0.0.1 is fine as the "our address" placeholder.      *
     * A real implementation would query the OS for the LAN interface IP.     */
    g_ourAddr.s_addr = htonl(0x7f000001u);
}

void netClientGetAddress(char *ip, char *dest)
{
    strncpy(dest, ip, 64);  /* stub: reverse DNS not needed for Phase C       */
    dest[63] = '\0';
}

void netClientGetUs(struct in_addr *dest, unsigned short *port)
{
    *dest = g_ourAddr;
    *port = htons(g_ourPort);
}

void netClientGetUsStr(char *dest)
{
    unsigned char *b = (unsigned char *)&g_ourAddr;
    snprintf(dest, 64, "%u.%u.%u.%u:%u",
             (unsigned)b[0], (unsigned)b[1],
             (unsigned)b[2], (unsigned)b[3],
             (unsigned)g_ourPort);
}

/*
 * netClientUdpPing — blocking connect+request+response, up to TIME_OUT ms.
 *
 * Original behaviour: send a raw UDP packet to dest:port, wait for a reply,
 * return the reply in *buff/*len.  Used during the join handshake to retrieve
 * the server's INFO_PACKET.
 *
 * ENet behaviour: create a temporary ENetHost, connect to dest:port, send the
 * packet on the reliable channel, wait for a RECEIVE event, copy the response
 * back, then keep the connection as g_serverConn for the rest of the session.
 */
bool netClientUdpPing(BYTE *buff, int *len, char *dest,
                      unsigned short port, bool wantCrc, bool addNonReliable)
{
    ENetAddress  addr;
    ENetPeer    *peer;
    ENetEvent    ev;
    enet_uint32  deadline;
    bool         got_response = FALSE;
    bool         need_connect;

    (void)addNonReliable;

    {
        char emsg[128];
        sprintf(emsg, "netClientUdpPing: ENTER dest=%s port=%u sendLen=%d wantCrc=%d",
                dest, (unsigned)port, *len, (int)wantCrc);
        enet_log(emsg);
    }

    if (!g_enetInitialized && !gnsEnsureInit()) {
        enet_log("netClientUdpPing: gnsEnsureInit FAILED");
        return FALSE;
    }

    /* Resolve destination */
    if (enet_address_set_host(&addr, dest) != 0) {
        enet_log("netClientUdpPing: enet_address_set_host FAILED");
        return FALSE;
    }
    addr.port = port;

    /* Create a client host if we don't have one yet. */
    if (!g_client) {
        g_client = enet_host_create(NULL, 1, 2, 0, 0);
        if (!g_client) {
            enet_log("netClientUdpPing: enet_host_create (client) FAILED");
            return FALSE;
        }
        enet_log("netClientUdpPing: created fresh g_client");
    }

    /* Reuse an existing connection to the same server — netJoinInit calls us
     * multiple times (info, password, name-check) on the same session.
     * Calling enet_host_connect again on a peerCount=1 host that already has
     * a connected peer returns NULL; reusing the peer avoids that. */
    need_connect = (g_serverConn == NULL ||
                    g_serverAddr.host != addr.host ||
                    g_serverAddr.port != addr.port);

    if (need_connect) {
        enet_log("netClientUdpPing: need_connect=TRUE — initiating ENet handshake");
        peer = enet_host_connect(g_client, &addr, 2, 0);
        if (!peer) {
            enet_log("netClientUdpPing: enet_host_connect returned NULL");
            return FALSE;
        }

        /* Wait for the ENet handshake (up to 3 s).
         * Pump the embedded server so it can accept the connection. */
        deadline = enet_time_get() + 3000;
        while (enet_time_get() < deadline) {
            serverTransportListenUDP();
            if (enet_host_service(g_client, &ev, 10) > 0) {
                if (ev.type == ENET_EVENT_TYPE_CONNECT) {
                    enet_log("netClientUdpPing: CONNECT event received — handshake OK");
                    break;
                }
                if (ev.type == ENET_EVENT_TYPE_DISCONNECT) {
                    enet_log("netClientUdpPing: DISCONNECT during handshake — aborting");
                    return FALSE;
                }
            }
        }
        if (enet_time_get() >= deadline) {
            enet_log("netClientUdpPing: CONNECT deadline expired (3 s) — handshake TIMEOUT");
            enet_peer_reset(peer);
            return FALSE;
        }
        /* Record so subsequent pings reuse this connection. */
        g_serverConn = peer;
        g_serverAddr = addr;
        g_ourPort    = (unsigned short)g_client->address.port;
    } else {
        enet_log("netClientUdpPing: reusing existing g_serverConn");
        peer = g_serverConn;
    }

    /* Send the ping packet reliably on channel 1.
     * When wantCrc is TRUE, append a 2-byte CRC so serverNetUDPPacketArrive
     * can validate it — matching what serverTransportSendUDPLast does.       */
    {
        BYTE        crcBuf[2048];
        const BYTE *sendBuf = buff;
        int         sendLen = *len;
        ENetPacket *pkt;

        if (wantCrc && sendLen + 2 <= (int)sizeof(crcBuf)) {
            BYTE crcA, crcB;
            CRCCalcBytes(buff, sendLen, &crcA, &crcB);
            memcpy(crcBuf, buff, (size_t)sendLen);
            crcBuf[sendLen]     = crcA;
            crcBuf[sendLen + 1] = crcB;
            sendBuf = crcBuf;
            sendLen += 2;
        }

        pkt = enet_packet_create(sendBuf, (size_t)sendLen, ENET_PACKET_FLAG_RELIABLE);
        if (!pkt || enet_peer_send(peer, 1, pkt) < 0) {
            enet_log("netClientUdpPing: enet_peer_send (request) FAILED");
            return FALSE;
        }
        {
            char smsg[80];
            sprintf(smsg, "netClientUdpPing: sent request len=%d (wantCrc=%d) on channel 1",
                    sendLen, (int)wantCrc);
            enet_log(smsg);
        }
    }
    enet_host_flush(g_client);
    enet_log("netClientUdpPing: flushed — waiting for response (6 s)");

    /* Wait for the response (up to 6 s).
     * *len is the SENT size on entry; on return it is the RECEIVED size.
     * The receive buffer (buff) is always MAX_UDPPACKET_SIZE bytes in the
     * callers (netJoinInit), so copying the full received packet is safe. */
    deadline = enet_time_get() + 6000;
    while (enet_time_get() < deadline && !got_response) {
        serverTransportListenUDP();
        if (enet_host_service(g_client, &ev, 10) > 0) {
            if (ev.type == ENET_EVENT_TYPE_RECEIVE) {
                int rcvLen = (int)ev.packet->dataLength;
                {
                    char rmsg[128];
                    sprintf(rmsg,
                        "netClientUdpPing: RECEIVE rcvLen=%d chan=%u pktType=0x%02x",
                        rcvLen, (unsigned)ev.channelID,
                        rcvLen > 0 ? (unsigned)((BYTE*)ev.packet->data)[0] : 0xFF);
                    enet_log(rmsg);
                }
                /* Record the peer so netClientGetLast() can return the correct
                 * server address to netJoinInit after the first ping.        */
                g_lastCliPeer = ev.peer;
                memcpy(buff, ev.packet->data, (size_t)rcvLen);
                *len = rcvLen;
                enet_packet_destroy(ev.packet);
                got_response = TRUE;
            } else {
                char omsg[64];
                sprintf(omsg, "netClientUdpPing: non-RECEIVE event type=%d (ignored)",
                        (int)ev.type);
                enet_log(omsg);
            }
        }
    }

    if (!got_response) {
        enet_log("netClientUdpPing: response deadline expired (6 s) — TIMEOUT");
    }
    {
        char rmsg[64];
        sprintf(rmsg, "netClientUdpPing: RETURN got_response=%d", (int)got_response);
        enet_log(rmsg);
    }
    return got_response;
}

bool netClientUdpPingServer(BYTE *buff, int *len, bool wantCrc, bool addNonReliable)
{
    char ipBuf[64];
    enet_address_get_host_ip(&g_serverAddr, ipBuf, sizeof(ipBuf));
    return netClientUdpPing(buff, len, ipBuf, g_serverAddr.port,
                            wantCrc, addNonReliable);
}

void netClientSendUdpNoWait(BYTE *buff, int len, char *dest, unsigned short port)
{
    /* Fire-and-forget: connect, send, don't wait for a response.             *
     * Used by the original code for non-critical packets to arbitrary hosts. *
     * For Phase C we send through g_serverConn if addresses match.           */
    if (g_serverConn && g_serverAddr.port == port) {
        ENetPacket *pkt = enet_packet_create(buff, (size_t)len,
                                             ENET_PACKET_FLAG_UNSEQUENCED);
        if (pkt) enet_peer_send(g_serverConn, 0, pkt);
    }
    (void)dest;
}

void netClientGetLastStr(char *dest)
{
    if (g_lastCliPeer) {
        enet_address_get_host_ip(&g_lastCliPeer->address, dest, 64);
    } else {
        strncpy(dest, "0.0.0.0", 64);
    }
}

void netClientGetLast(struct in_addr *dest, unsigned short *port)
{
    if (g_lastCliPeer) {
        dest->s_addr = htonl(g_lastCliPeer->address.host);
        *port        = htons(g_lastCliPeer->address.port);
    } else {
        dest->s_addr = 0;
        *port        = 0;
    }
}

bool netClientSetUseEvents(void) { return FALSE; }  /* not used in ENet model */

/*
 * netClientUdpCheck — non-blocking drain of all pending client-side packets.
 * Called every game tick from boloUpdate().
 */
void netClientUdpCheck(void)
{
    ENetEvent ev;
    if (!g_client) return;

    while (enet_host_service(g_client, &ev, 0) > 0) {
        switch (ev.type) {
        case ENET_EVENT_TYPE_RECEIVE:
            g_lastCliPeer = ev.peer;
            netUdpPacketArrive(
                (BYTE *)ev.packet->data,
                (int)ev.packet->dataLength,
                (unsigned short)ev.peer->address.port);
            enet_packet_destroy(ev.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            if (ev.peer == g_serverConn) g_serverConn = NULL;
            break;
        default:
            break;
        }
    }
}

bool netClientSetUdpAsync(bool on)
{
    (void)on;
    return TRUE;  /* ENet is always non-blocking when used with timeout=0     */
}

bool netClientSetTracker(char *address, unsigned short port)
{
    (void)address; (void)port;
    return TRUE;
}

void netClientSendUdpTracker(BYTE *buff, int len)
{
    (void)buff; (void)len;  /* Tracker stub — Phase C4+                       */
}
