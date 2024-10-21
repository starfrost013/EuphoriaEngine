/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2023-2024 starfrost

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// server.h


#pragma once

#include <common/common.hpp>
#include <../game/src/game.hpp>
//=============================================================================

#define	MAX_MASTERS	8				// max recipients for heartbeat packets

// master addresses
extern const char* master_base;
extern const char* master_alternative;

typedef enum
{
	ss_dead,			// no map loaded
	ss_loading,			// spawning level edicts
	ss_game,			// actively running
	ss_demo,			// running demo
	ss_image
} server_state_t;
// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

typedef struct
{
	server_state_t	state;			// precache commands are only valid during load

	bool			attractloop;		// running cinematics and demos for the local system only
	bool			loadgame;			// client begins should reuse existing entity

	uint32_t		time;				// always sv.framenum * (1/FRAMETIME) msec
	int32_t 		framenum;

	char			name[MAX_QPATH];			// map name
	struct cmodel_s* models[MAX_MODELS];

	char			configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];
	entity_state_t	baselines[MAX_EDICTS];

	// the multicast buffer is used to send a message to a set of clients
	// it is only used to marshall data until SV_Multicast is called
	sizebuf_t		multicast;
	uint8_t			multicast_buf[MAX_MSGLEN];

	// demo server information
	FILE*			demofile;
	bool			timedemo;		// don't time sync
} server_t;

#define EDICT_NUM(n) ((edict_t *)((uint8_t *)game->edicts + game->edict_size*(n)))
#define NUM_FOR_EDICT(e) ( ((uint8_t *)(e)-(uint8_t *)game->edicts ) / game->edict_size)


typedef enum
{
	cs_free,		// can be reused for a new connection
	cs_zombie,		// client has been disconnected, but don't reuse
	// connection for a couple seconds
	cs_connected,	// has been assigned to a client_t, but not in game yet
	cs_spawned		// client is fully in game
} client_state_t;

typedef struct
{
	int32_t 		areabytes;
	uint8_t			areabits[MAX_MAP_AREAS / 8];		// portalarea visibility bits
	player_state_t	ps;
	int32_t 		num_entities;
	int32_t 		first_entity;		// into the circular sv_packet_entities[]
	int32_t 		senttime;			// for ping calculations
} client_frame_t;

#define	LATENCY_COUNTS		16
#define	RATE_MESSAGES		10
#define PLAYER_NAME_LENGTH	80

typedef struct client_s
{
	client_state_t	state;

	char			userinfo[MAX_INFO_STRING];		// name, etc

	int32_t 		lastframe;			// for delta compression
	usercmd_t		lastcmd;			// for filling in big drops

	int32_t 		commandMsec;		// every seconds this is reset, if user
	// commands exhaust it, assume time cheating

	int32_t 		frame_latency[LATENCY_COUNTS];
	int32_t 		ping;

	int32_t 		message_size[RATE_MESSAGES];	// used to rate drop packets

	edict_t*		edict;				// EDICT_NUM(clientnum+1)
	char			name[PLAYER_NAME_LENGTH];			// extracted from userinfo, high bits masked
	int32_t 		messagelevel;		// for filtering printed messages

	// The datagram is written to by sound calls, prints, temp ents, etc.
	// It can be harmlessly overflowed.
	sizebuf_t		datagram;
	uint8_t			datagram_buf[MAX_MSGLEN];

	client_frame_t	frames[UPDATE_BACKUP];	// updates can be delta'd from here

	uint8_t*		download;			// file being downloaded
	int32_t 		downloadsize;		// total bytes (can't use EOF because of paks)
	int32_t 		downloadcount;		// bytes sent

	int32_t 		lastmessage;		// sv.framenum when packet was last received
	int32_t 		lastconnect;

	int32_t 		challenge;			// challenge of this user, randomly generated

	netchan_t		netchan;
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

typedef struct
{
	netadr_t		adr;
	int32_t 		challenge;
	int32_t 		time;
} challenge_t;

typedef struct
{
	bool			initialized;				// sv_init has completed
	int32_t 		realtime;					// always increasing, no clamping, etc

	char			mapcmd[MAX_TOKEN_CHARS];	// ie: *intro.cin+base 

	int32_t 		spawncount;					// incremented each server start
	// used to check late spawns

	client_t*		clients;					// [maxclients->value];
	int32_t 		num_client_entities;		// maxclients->value*UPDATE_BACKUP*MAX_PACKET_ENTITIES
	int32_t 		next_client_entities;		// next client_entity to use
	entity_state_t* client_entities;		// [num_client_entities]

	int32_t 		last_heartbeat;

	challenge_t		challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	// serverrecord values
	FILE*			demofile;
	sizebuf_t		demo_multicast;
	uint8_t			demo_multicast_buf[MAX_MSGLEN];
} server_static_t;

//=============================================================================

extern netadr_t			net_from;
extern sizebuf_t		net_message;

extern netadr_t			master_adr[MAX_MASTERS];	// address of the master server

extern server_static_t	svs;				// persistant server info
extern server_t			sv;					// local server

extern cvar_t* sv_tickrate;			// server tickrate

extern cvar_t* hostname;
extern cvar_t* sv_paused;
extern cvar_t* sv_maxclients;
extern cvar_t* sv_noreload;			// don't reload level state when reentering

// physics parameters (override client default to prevent cheating)
extern cvar_t* sv_stopspeed;
extern cvar_t* sv_maxspeed_player;
extern cvar_t* sv_maxspeed_director;
extern cvar_t* sv_duckspeed;
extern cvar_t* sv_accelerate_player;
extern cvar_t* sv_accelerate_director;
extern cvar_t* sv_airaccelerate;
extern cvar_t* sv_wateraccelerate;
extern cvar_t* sv_friction;
extern cvar_t* sv_waterfriction;
extern cvar_t* sv_waterspeed;

// master stuff
extern cvar_t* public_server;
// development tool
extern cvar_t* sv_enforcetime;
#ifdef DEBUG
extern cvar_t* sv_debug_heartbeat;		// send heartbeats every 2 seconds instead of every 5 minutes
#endif

extern client_t* sv_client;
extern edict_t* sv_player;

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage(const char* message, bool reconnect);
void SV_DropClient(client_t* drop);

int32_t SV_ModelIndex(const char* name);
int32_t SV_SoundIndex(const char* name);
int32_t SV_ImageIndex(const char* name);

void SV_ExecuteUserCommand(char* s);
void SV_ExecuteUserEvent();
void SV_InitOperatorCommands();

void SV_UserinfoChanged(client_t* cl);

char* SV_StatusString();

//
// sv_master.c
//



// Connectionless stuff
void Master_SvcAck();
void Master_SvcInfo();
void Master_SvcPing();
void Master_SvcStatus();
void Netservices_MasterHeartbeatLegacy(); // DEPRECATED!!!!

//
// sv_init.c
//
void SV_InitGame();
void SV_Map(bool attractloop, char* levelstring, bool loadgame);


//
// sv_phys.c
//
void SV_PrepWorldFrame();

//
// sv_send.c
//
typedef enum
{ 
	RD_NONE, 
	RD_CLIENT, 
	RD_PACKET 
} redirect_t;

#define	SV_OUTPUTBUF_LENGTH	(MAX_MSGLEN - 16)

extern	char	sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect(int32_t sv_redirected, char* outputbuf);

void SV_DemoCompleted();
void SV_SendClientMessages();

void SV_Multicast(vec3_t origin, multicast_t to);
void SV_StartSound(vec3_t origin, edict_t* entity, int32_t channel, int32_t soundindex, float volume, float attenuation, float timeofs);
void SV_ClientPrintf(client_t* cl, int32_t level, const char* fmt, ...);
void SV_BroadcastPrintf(int32_t level, const char* fmt, ...);
void SV_BroadcastCommand(const char* fmt, ...);

//
// sv_user.c
//
void SV_Nextserver();
void SV_ExecuteClientMessage(client_t* cl);

//
// sv_ccmds.c
//
void SV_ReadLevelFile();
void SV_Status_f();

//
// sv_ents.c
//
void SV_WriteFrameToClient(client_t* client, sizebuf_t* msg);
void SV_RecordDemoMessage();
void SV_BuildClientFrame(client_t* client);

//
// sv_game.c
//
extern game_api_t* game;

void SV_InitGameLibraries();
void SV_ShutdownGameLibraries();

//============================================================

//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld();
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEdict(edict_t* ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEdict(edict_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

int32_t SV_AreaEdicts(vec3_t mins, vec3_t maxs, edict_t** list, int32_t maxcount, int32_t areatype);
// fills in a table of edict pointers with edicts that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// ??? does this always return the world?

//===================================================================

//
// functions that interact with everything apropriate
//
int32_t SV_PointContents(vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// Quake 2 extends this to also check entities, to allow moving liquids

// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passedict is explicitly excluded from clipping checks (normally NULL)
trace_t SV_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t* passedict, int32_t contentmask);

//
// HACK PROTECTION
//