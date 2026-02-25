/*
 * $Id$
 *
 * Copyright (c) 1998-2008 John Morrison.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#ifndef _BRAINS_H
#define _BRAINS_H

/*
 * MODERNIZATION NOTES (new-project/include/brain.h):
 *
 * 1. Original brain.h included <winsock.h> INSIDE a #pragma pack(push,8) scope.
 *    Windows SDK headers included under a non-default pack state cause C_ASSERT
 *    struct-size failures. Fixed: socket includes moved to the top, at default packing.
 *
 * 2. Replaced <winsock.h> (Winsock 1) with <winsock2.h> (Winsock 2) for consistency
 *    with the rest of the server build.
 *
 * 3. Original used #pragma pack(8) / #pragma pack(1) without push/pop, leaving
 *    pack(1) active after inclusion. Fixed: proper push(8) / pop pair.
 */

/* ---------------------------------------------------------------
 * Step 1: Platform socket headers at DEFAULT packing — must come
 * before any #pragma pack directive so SDK C_ASSERT checks see
 * the expected struct sizes.
 * --------------------------------------------------------------- */
#ifndef NETWORK_H
/* Only define socket types if network.h hasn't done it already */
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>   /* was <winsock.h>; winsock2 must precede windows.h */
#  include <ws2tcpip.h>
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned long  u_long;
typedef u_char  NIBBLE;
typedef u_char  BYTE;
typedef u_short WORD;
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#endif

typedef struct {
  struct in_addr serveraddress;
  unsigned short serverport;
  u_long start_time;
} GAMEID;
#endif /* !NETWORK_H */

/* ---------------------------------------------------------------
 * Step 2: Pack-sensitive game struct definitions.
 * push saves current default; pop restores it afterward.
 * --------------------------------------------------------------- */
#pragma pack(push, 8)
#include "global.h"

#define local static
#define export
#define import extern

/* jm */
typedef BYTE Boolean;
#define TRUE 1
#define FALSE 0
/* end jm */

/* The various accessible tank control functions */
enum
	{
	KEY_faster=0, KEY_slower, KEY_turnleft, KEY_turnright,
	KEY_morerange, KEY_lessrange, KEY_shoot, KEY_dropmine,
	KEY_TankView, KEY_PillView
	};
#define setkey(CONTROLVECTOR, COMMAND) (CONTROLVECTOR |= (1<<COMMAND))
#define testkey(CONTROLVECTOR, COMMAND) ((CONTROLVECTOR & (1<<COMMAND)) != 0)

typedef BYTE TERRAIN;
enum
	{
	BBUILDING=0, BRIVER, BSWAMP, BCRATER, BROAD, BFOREST, BRUBBLE, BGRASS,
	BHALFBUILDING, BBOAT, BDEEPSEA, BREFBASE_T, BPILLBOX_T,
	TERRAIN_UNKNOWN,
	NUM_TERRAINS,
	TERRAIN_MASK     = 0x0F,
	TERRAIN_TANK_VIS = 0x10,
	TERRAIN_PILL_VIS = 0x20,
	TERRAIN_UNUSED   = 0x40,
	TERRAIN_MINE     = 0x80
	};

#define is_wet(A) ((A) == RIVER || (A) == BOAT || (A) == DEEPSEA)

typedef BYTE BUILDMODE;
enum
	{
	BUILDMODE_FARM=1, BUILDMODE_ROAD,
	BUILDMODE_BUILD, BUILDMODE_PBOX, BUILDMODE_MINE
	};

typedef struct
	{
	MAP_X x;
	MAP_Y y;
	BUILDMODE action;
	} BuildInfo;

/* Farming gets you 4 tree units.
 Roads, bridges and buildings take 2 units, boats take 20
 Placing a pillbox takes 4 units, repairing takes proportionately less */

#define NEUTRAL_PLAYER 0xFF
#define FORESTVISUAL 0x30
#define MINRANGE 2
#define MAXRANGE 14
#define MAX_PILL_ARMOUR 15
#define MAX_BASE_SHELLS 90
#define MAX_BASE_MINES  90
#define MAX_BASE_ARMOUR 90
#define ARMOUR_COST 5
#define BASE_RESIST_SHELLS (ARMOUR_COST)
#define BASE_RESIST_TANKS (ARMOUR_COST*2)
#define MIN_BASE_ARMOUR (BASE_RESIST_TANKS + ARMOUR_COST - 1)

typedef WORD WORLD_X, WORLD_Y;

#ifndef UCHAR36_DEFINED
#define UCHAR36_DEFINED
  typedef struct { u_char c[36]; } u_char36;
#endif

enum { GameType_open=1, GameType_tournament, GameType_strict_tment };

#define GAMEINFO_HIDDENMINES 0x80
#define GAMEINFO_ALLMINES_VISIBLE 0xC0

typedef struct
	{
	u_char36 mapname;
	GAMEID gameid;
	BYTE gametype;
	BYTE hidden_mines;
	BYTE allow_AI;
	BYTE assist_AI;
	long start_delay;
	long time_limit;
	} GAMEINFO;

typedef u_short OBJECT;
enum
	{
	OBJECT_TANK=0,
	OBJECT_SHOT,
	OBJECT_PILLBOX,
	OBJECT_REFBASE,
	OBJECT_BUILDMAN,
	OBJECT_PARACHUTE
	};

#define OBJECT_HOSTILE 1
#define OBJECT_NEUTRAL 2

typedef struct
	{
	OBJECT object;
	WORLD_X x;
	WORLD_Y y;
	WORD idnum;
	BYTE direction;
	BYTE info;
	} ObjectInfo;

#define pillbox_strength direction
#define refbase_strength direction

typedef struct
	{
	u_short sender;
	PlayerBitMap *receivers;
	u_char *message;
	} MessageInfo;

#define CURRENT_BRAININFO_VERSION 3
enum { BRAIN_OPEN=0, BRAIN_CLOSE, BRAIN_THINK, BRAIN_MENU=200 };

typedef struct
	{
	u_short BoloVersion;
	u_short InfoVersion;
	void *userdata;
	u_short PrefsVRefNum;
	u_char *PrefsFileName;
	u_short operation;
	u_short menu_item;

	u_short max_players;
	u_short max_pillboxes;
	u_short max_refbases;
	u_short player_number;
	u_short num_players;
	u_char36 **playernames;
	PlayerBitMap *allies;

	WORLD_X tankx;
	WORLD_Y tanky;

	BYTE direction;
	BYTE speed;
	BYTE inboat;
	BYTE hidden;
	BYTE shells;
	BYTE mines;
	BYTE armour;
	BYTE trees;

	BYTE carriedpills;
	BYTE carriedbases;
	WORD padding2;

	BYTE gunrange;
	BYTE reload;
	BYTE newtank;
	BYTE tankobstructed;

	ObjectInfo *base;
	BYTE base_shells;
	BYTE base_mines;
	BYTE base_armour;
	BYTE padding3;

	BYTE man_status;
	BYTE man_direction;
	WORLD_X man_x;
	WORLD_Y man_y;
	BYTE manobstructed;
	BYTE padding4;

	WORD *pillview;
	MAP_Y view_top;
	MAP_X view_left;
	BYTE  view_height;
	BYTE  view_width;
	TERRAIN *viewdata;

	WORD padding5;
	u_short num_objects;
	ObjectInfo *objects;

	MessageInfo *message;

	u_long *holdkeys;
	u_long *tapkeys;
	BuildInfo *build;
	PlayerBitMap *wantallies;
	PlayerBitMap *messagedest;
	u_char *sendmessage;

	const TERRAIN *theWorld;
	GAMEINFO gameinfo;

	} BrainInfo;

#pragma pack(pop)
#endif /* _BRAINS_H */
