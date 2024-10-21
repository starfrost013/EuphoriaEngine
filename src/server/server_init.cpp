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

#include "server.hpp"

server_static_t	svs;				// persistant server info
server_t		sv;					// local server

extern const char* master_base;
extern const char* master_alternative;

/*
================
SV_FindIndex
================
*/
int32_t SV_FindIndex(const char* name, int32_t start, int32_t max, bool create)
{
	int32_t i;

	if (!name || !name[0])
		return 0;

	for (i = 1; i < max && sv.configstrings[start + i][0]; i++)
		if (!strcmp(sv.configstrings[start + i], name))
			return i;

	if (!create)
		return 0;

	if (i == max)
		Com_Error(ERR_DROP, "*Index: overflow");

	strncpy(sv.configstrings[start + i], name, sizeof(sv.configstrings[i]));

	if (sv.state != ss_loading)
	{	
		// send the update to everyone
		SZ_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, svc_configstring);
		MSG_WriteShort(&sv.multicast, start + i);
		MSG_WriteString(&sv.multicast, name);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}

	return i;
}


int32_t SV_ModelIndex(const char* name)
{
	return SV_FindIndex(name, CS_MODELS, MAX_MODELS, true);
}

int32_t SV_SoundIndex(const char* name)
{
	return SV_FindIndex(name, CS_SOUNDS, MAX_SOUNDS, true);
}

int32_t SV_ImageIndex(const char* name)
{
	return SV_FindIndex(name, CS_IMAGES, MAX_IMAGES, true);
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline()
{
	edict_t*	svent;
	int32_t		entnum;

	for (entnum = 1; entnum < game->num_edicts; entnum++)
	{
		svent = EDICT_NUM(entnum);
		if (!svent->inuse)
			continue;
		if (!svent->s.modelindex && !svent->s.sound && !svent->s.effects)
			continue;
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		VectorCopy3(svent->s.origin, svent->s.old_origin);
		sv.baselines[entnum] = svent->s;
	}
}


/*
=================
SV_CheckForSavegame
=================
*/
void SV_CheckForSavegame()
{
	char		name[MAX_OSPATH];
	FILE* f;
	int32_t 		i;

	if (sv_noreload->value)
		return;

	if (Cvar_VariableValue("gamemode"))
		return;

	snprintf(name, sizeof(name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
	f = fopen(name, "rb");
	if (!f)
		return;		// no savegame

	fclose(f);

	SV_ClearWorld();

	// get configstrings and areaportals
	SV_ReadLevelFile();

	if (!sv.loadgame)
	{	// coming back to a level after being in a different
		// level, so run it for ten seconds

		// rlava2 was sending too many lightstyles, and overflowing the
		// reliable data. temporarily changing the server state to loading
		// prevents these from being passed down.
		server_state_t		previousState;

		previousState = sv.state;
		sv.state = ss_loading;
		for (i = 0; i < 100; i++)
			game->Game_RunFrame();

		sv.state = previousState;
	}
}


/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.

================
*/
void SV_SpawnServer(char* server, char* spawnpoint, server_state_t serverstate, bool attractloop, bool loadgame)
{
	int32_t 		i;
	uint32_t		checksum;

	if (attractloop)
		Cvar_Set("paused", "0");

	Com_Printf("------- Server Initialization -------\n");

	Com_DPrintf("SpawnServer: %s\n", server);

#ifdef PLAYTEST
	if (strstr(server, "coop") != NULL)
	{
		Com_Printf("Nuh uh! No spoilers here!\n");
		return;
	}
#endif
	if (sv.demofile)
		fclose(sv.demofile);

	svs.spawncount++;		// any partially connected client will be
	// restarted
	sv.state = ss_dead;
	Com_SetServerState(sv.state);

	// wipe the entire per-level structure
	memset(&sv, 0, sizeof(sv));
	svs.realtime = 0;
	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	// save name for levels that don't set message
	strcpy(sv.configstrings[CS_NAME], server);

	// add player physics information to configstrings
	sprintf(sv.configstrings[CS_PHYS_STOPSPEED], "%g", sv_stopspeed->value);
	sprintf(sv.configstrings[CS_PHYS_MAXSPEED_PLAYER], "%g", sv_maxspeed_player->value);
	sprintf(sv.configstrings[CS_PHYS_MAXSPEED_DIRECTOR], "%g", sv_maxspeed_director->value);
	sprintf(sv.configstrings[CS_PHYS_DUCKSPEED], "%g", sv_duckspeed->value);
	sprintf(sv.configstrings[CS_PHYS_ACCELERATE_PLAYER], "%g", sv_accelerate_player->value);
	sprintf(sv.configstrings[CS_PHYS_ACCELERATE_DIRECTOR], "%g", sv_accelerate_director->value);
	sprintf(sv.configstrings[CS_PHYS_ACCELERATE_AIR], "%g", sv_airaccelerate->value);
	sprintf(sv.configstrings[CS_PHYS_ACCELERATE_WATER], "%g", sv_wateraccelerate->value);
	sprintf(sv.configstrings[CS_PHYS_FRICTION], "%g", sv_friction->value);
	sprintf(sv.configstrings[CS_PHYS_FRICTION_WATER], "%g", sv_waterfriction->value);

	phys_stopspeed = sv_stopspeed->value;
	phys_maxspeed_player = sv_maxspeed_player->value;
	phys_maxspeed_director = sv_maxspeed_director->value;
	phys_duckspeed = sv_duckspeed->value;
	phys_accelerate_player = sv_accelerate_player->value;
	phys_accelerate_director = sv_accelerate_director->value;
	phys_airaccelerate = sv_airaccelerate->value;
	phys_wateraccelerate = sv_wateraccelerate->value;
	phys_friction = sv_friction->value;
	phys_waterfriction = sv_waterfriction->value;

	SZ_Init(&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));

	strcpy(sv.name, server);

	// leave slots at start for clients only
	for (i = 0; i < sv_maxclients->value; i++)
	{
		// needs to reconnect
		if (svs.clients[i].state > cs_connected)
			svs.clients[i].state = cs_connected;
		svs.clients[i].lastframe = -1;
	}

	sv.time = 1000;

	strcpy(sv.name, server);
	strcpy(sv.configstrings[CS_NAME], server);

	if (serverstate != ss_game)
	{
		sv.models[1] = Map_Load("", false, &checksum);	// no real map
	}
	else
	{
		snprintf(sv.configstrings[CS_MODELS + 1], sizeof(sv.configstrings[CS_MODELS + 1]),
			"maps/%s.bsp", server);
		sv.models[1] = Map_Load(sv.configstrings[CS_MODELS + 1], false, &checksum);
	}
	snprintf(sv.configstrings[CS_MAPCHECKSUM], sizeof(sv.configstrings[CS_MAPCHECKSUM]),
		"%i", checksum);

	//
	// clear physics interaction links
	//
	SV_ClearWorld();

	for (i = 1; i < Map_NumInlineModels(); i++)
	{
		snprintf(sv.configstrings[CS_MODELS + 1 + i], sizeof(sv.configstrings[CS_MODELS + 1 + i]),
			"*%i", i);
		sv.models[i + 1] = Map_LoadInlineModel(sv.configstrings[CS_MODELS + 1 + i]);
	}

	//
	// spawn the rest of the entities on the map
	//	

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;
	Com_SetServerState(sv.state);

	// load and spawn all other entities
	game->Game_SpawnEntities(sv.name, Map_GetEntityString(), spawnpoint);

	// run two frames to allow everything to settle
	game->Game_RunFrame();
	game->Game_RunFrame();

	// all precaches are complete
	sv.state = serverstate;
	Com_SetServerState(sv.state);

	// create a baseline for more efficient communications
	SV_CreateBaseline();

	// check for a savegame
	SV_CheckForSavegame();

	// set serverinfo variable
	Cvar_FullSet("mapname", sv.name, CVAR_SERVERINFO | CVAR_NOSET);

	Com_Printf("-------------------------------------\n");
}

#define ZOMBONO_MASTER_LENGTH	32

/*
==============
SV_InitGame

A brand new game has been started
==============
*/
void SV_InitGame()
{
	int32_t 	i;
	edict_t* ent;
	char	zombono_master[ZOMBONO_MASTER_LENGTH];

	if (svs.initialized)
	{
		// cause any connected clients to reconnect
		SV_Shutdown("Server restarted\n", true);
		SV_ShutdownGameLibraries();
	}
	else
	{
		// make sure the client is down
		CL_Drop();
#ifndef DEDICATED_ONLY
		Render2D_BeginLoadingPlaque();
#endif
	}

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars();

	svs.initialized = true;

	// init clients

	if (sv_maxclients->value <= 1)
		Cvar_FullSet("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);
	else if (sv_maxclients->value > MAX_CLIENTS)
		Cvar_FullSet("sv_maxclients", va("%i", MAX_CLIENTS), CVAR_SERVERINFO | CVAR_LATCH);

	// remove if we decide to not have that 6 level singleplayer campaign#
	/*
	if (Cvar_VariableValue("singleplayer"))
	{
		Cvar_FullSet("sv_maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH);
	}
	*/

	svs.spawncount = rand();
	svs.clients = (client_t*)Memory_ZoneMalloc(sizeof(client_t) * sv_maxclients->value);
	svs.num_client_entities = sv_maxclients->value * UPDATE_BACKUP * 64;
	svs.client_entities = (entity_state_t*)Memory_ZoneMalloc(sizeof(entity_state_t) * svs.num_client_entities);

	// init network stuff
	Net_Config((sv_maxclients->value > 1));

	// heartbeats will always be sent to the id master
	svs.last_heartbeat = -99999;		// send immediately
	snprintf(zombono_master, sizeof(zombono_master), "%s:%i", master_base, PORT_MASTER);

	if (!Net_StringToAdr(zombono_master, &master_adr[0]))
	{
		snprintf(zombono_master, sizeof(zombono_master), "%s:%i", master_alternative, PORT_MASTER);

		if (!Net_StringToAdr(zombono_master, &master_adr[0]))
		{
			Com_Printf("Warning: Failed to contact both base and alternative master servers!\n");
		}
	}

	Com_DPrintf("Master server is %s (TODO: MAKE CVAR!!!)\n", zombono_master);

	// init game
	SV_InitGameLibraries();

	for (i = 0; i < sv_maxclients->value; i++)
	{
		ent = EDICT_NUM(i + 1);
		ent->s.number = i + 1;
		svs.clients[i].edict = ent;
		memset(&svs.clients[i].lastcmd, 0, sizeof(svs.clients[i].lastcmd));
	}
}


/*
======================
SV_Map

  the full syntax is:

  map [*]<map>$<startspot>+<nextserver>

command from the console or game dll
Map can also be a .dm2 file
======================
*/
void SV_Map(bool attractloop, char* levelstring, bool loadgame)
{
	char	level[MAX_QPATH];
	char* ch;
	int32_t 	l;
	char	spawnpoint[MAX_QPATH];

	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	if (sv.state == ss_dead && !sv.loadgame)
		SV_InitGame();	// the game is just starting

	strcpy(level, levelstring);

	// if there is a + in the map, set nextserver to the remainder
	ch = strstr(level, "+");
	if (ch)
	{
		*ch = 0;
		Cvar_Set("nextserver", va("gamemap \"%s\"", ch + 1));
	}
	else
		Cvar_Set("nextserver", "");

	// if there is a $, use the remainder as a spawnpoint
	ch = strstr(level, "$");
	if (ch)
	{
		*ch = 0;
		strcpy(spawnpoint, ch + 1);
	}
	else
		spawnpoint[0] = 0;

	// skip the end-of-unit flag if necessary
	l = (int32_t)strlen(level);
	if (level[0] == '*')
	{
		memmove(level, level + 1, l);
		--l;
	}

	// demo file
	if (l > 4 && !strcmp(level + l - 4, ".dm2"))
	{
		Render2D_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand("changing\n");
		SV_SpawnServer(level, spawnpoint, ss_demo, attractloop, loadgame);
	}
	else // map
	{
		Render2D_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand("changing\n");
		SV_SendClientMessages();
		SV_SpawnServer(level, spawnpoint, ss_game, attractloop, loadgame);
		Cbuf_CopyToDefer();
	}

	SV_BroadcastCommand("reconnect\n");
}