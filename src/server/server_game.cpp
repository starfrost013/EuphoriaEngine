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

// sv_game.c -- the server's interface to the game dll

#include "server.hpp"

game_export_t* ge;

/*
===============
PF_Unicast

Sends the contents of the mutlicast buffer to a single client
===============
*/
void PF_Unicast(edict_t* ent, bool reliable)
{
	int32_t 	p;
	client_t* client;

	if (!ent)
		return;

	if (!ent->inuse)
		return;

	if (!ent->client)
		Sys_Error("Tried to unicast data to a non-client?!");

	p = NUM_FOR_EDICT(ent);
	if (p < 1 || p > sv_maxclients->value)
		return;

	client = svs.clients + (p - 1);

	if (reliable)
		SZ_Write(&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
	else
		SZ_Write(&client->datagram, sv.multicast.data, sv.multicast.cursize);

	SZ_Clear(&sv.multicast);
}


/*
===============
PF_dprintf

Debug print to server console
===============
*/
void PF_dprintf(const char* fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start(argptr, fmt);
	vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	Com_Printf("%s", msg);
}


/*
===============
PF_cprintf

Print to a single client
===============
*/
void PF_cprintf(edict_t* ent, int32_t level, const char* fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int32_t 		n = 0;

	if (ent)
	{
		n = NUM_FOR_EDICT(ent);
		if (n < 1 || n > sv_maxclients->value)
			Com_Error(ERR_DROP, "cprintf to a non-client");
	}

	va_start(argptr, fmt);
	vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	if (ent)
		SV_ClientPrintf(svs.clients + (n - 1), level, "%s", msg);
	else
		Com_Printf("%s", msg);
}

void PF_Text_Draw(edict_t* ent, const char* font, int32_t x, int32_t y, const char* text, ...)
{
	MSG_WriteByte(&sv.multicast, svc_drawtext);
	MSG_WriteString(&sv.multicast, font);
	MSG_WriteShort(&sv.multicast, x);
	MSG_WriteShort(&sv.multicast, y);
	MSG_WriteString(&sv.multicast, text);

	// text updates probably shouldnt be reliable if they are happening all the time as they are intended to
	PF_Unicast(ent, false);
}


/*
===============
PF_centerprintf

centerprint to a single client
===============
*/
void PF_centerprintf(edict_t* ent, const char* fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int32_t 	n;

	n = NUM_FOR_EDICT(ent);
	if (n < 1 || n > sv_maxclients->value)
		return;	// Com_Error (ERR_DROP, "centerprintf to a non-client");

	va_start(argptr, fmt);
	vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	MSG_WriteByte(&sv.multicast, svc_centerprint);
	MSG_WriteString(&sv.multicast, msg);
	PF_Unicast(ent, true);
}


/*
===============
PF_error

Abort the server with a game error
===============
*/
void PF_error(const char* fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start(argptr, fmt);
	vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	Com_Error(ERR_DROP, "Game Error: %s", msg);
}


/*
=================
PF_setmodel

Also sets mins and maxs for inline bmodels
=================
*/
void PF_setmodel(edict_t* ent, const char* name)
{
	int32_t 	i;
	cmodel_t* mod;

	if (!name)
		Com_Error(ERR_DROP, "PF_setmodel: NULL");

	i = SV_ModelIndex(name);

	//	ent->model = name;
	ent->s.modelindex = i;

	// if it is an inline model, get the size information for it
	if (name[0] == '*')
	{
		mod = Map_LoadInlineModel(name);
		VectorCopy3(mod->mins, ent->mins);
		VectorCopy3(mod->maxs, ent->maxs);
		SV_LinkEdict(ent);
	}

}

/*
===============
PF_Configstring

===============
*/
void PF_Configstring(int32_t index, const char* val)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_Error(ERR_DROP, "configstring: bad index %i\n", index);

	if (!val)
		val = "";

	// change the string in sv
	strcpy(sv.configstrings[index], val);

	if (sv.state != ss_loading)
	{	// send the update to everyone
		SZ_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, svc_configstring);
		MSG_WriteShort(&sv.multicast, index);
		MSG_WriteString(&sv.multicast, val);

		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}
}

// These aren't safe to use at all times.
// Make sure you are reading from a real client when you use these.
int32_t PF_ReadChar() { return MSG_ReadChar(&net_message); }
int32_t PF_ReadByte() { return MSG_ReadByte(&net_message); }
int32_t PF_ReadShort() { return MSG_ReadShort(&net_message); }
int32_t PF_ReadInt() { return MSG_ReadInt(&net_message); }
float PF_ReadFloat() { return MSG_ReadFloat(&net_message); }
char* PF_ReadString() { return MSG_ReadString(&net_message); }
void PF_ReadPos(vec3_t pos) { MSG_ReadPos(&net_message, pos); }
void PF_ReadDir(vec3_t dir) { MSG_ReadDir(&net_message, dir); }
void PF_ReadColor(color4_t color) { MSG_ReadColor(&net_message, color); }
float PF_ReadAngle(float f) { return MSG_ReadAngle(&net_message); }

void PF_WriteChar(int32_t c) { MSG_WriteChar(&sv.multicast, c); }
void PF_WriteByte(int32_t c) { MSG_WriteByte(&sv.multicast, c); }
void PF_WriteShort(int32_t c) { MSG_WriteShort(&sv.multicast, c); }
void PF_WriteInt(int32_t c) { MSG_WriteInt(&sv.multicast, c); }
void PF_WriteFloat(float f) { MSG_WriteFloat(&sv.multicast, f); }
void PF_WriteString(const char* s) { MSG_WriteString(&sv.multicast, s); }
void PF_WritePos(vec3_t pos) { MSG_WritePos(&sv.multicast, pos); }
void PF_WriteDir(vec3_t dir) { MSG_WriteDir(&sv.multicast, dir); }
void PF_WriteColor(color4_t color) { MSG_WriteColor(&sv.multicast, color); }
void PF_WriteAngle(float f) { MSG_WriteAngle(&sv.multicast, f); }

/*
=================
PF_inPVS

Also checks portalareas so that doors block sight
=================
*/
bool PF_inPVS(vec3_t p1, vec3_t p2)
{
	int32_t 	leafnum;
	int32_t 	cluster;
	int32_t 	area1, area2;
	uint8_t* mask;

	leafnum = Map_PointLeafnum(p1);
	cluster = Map_GetLeafCluster(leafnum);
	area1 = Map_LeafArea(leafnum);
	mask = Map_ClusterPVS(cluster);

	leafnum = Map_PointLeafnum(p2);
	cluster = Map_GetLeafCluster(leafnum);
	area2 = Map_LeafArea(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
		return false;
	if (!Map_AreasConnected(area1, area2))
		return false;		// a door blocks sight
	return true;
}


/*
=================
PF_inPHS

Also checks portalareas so that doors block sound
=================
*/
bool PF_inPHS(vec3_t p1, vec3_t p2)
{
	int32_t 	leafnum;
	int32_t 	cluster;
	int32_t 	area1, area2;
	uint8_t* mask;

	leafnum = Map_PointLeafnum(p1);
	cluster = Map_GetLeafCluster(leafnum);
	area1 = Map_LeafArea(leafnum);
	mask = Map_ClusterPHS(cluster);

	leafnum = Map_PointLeafnum(p2);
	cluster = Map_GetLeafCluster(leafnum);
	area2 = Map_LeafArea(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
		return false;		// more than one bounce away
	if (!Map_AreasConnected(area1, area2))
		return false;		// a door blocks hearing

	return true;
}

void PF_StartSound(edict_t* entity, int32_t channel, int32_t sound_num, float volume,
	float attenuation, float timeofs)
{
	if (!entity)
		return;
	SV_StartSound(NULL, entity, channel, sound_num, volume, attenuation, timeofs);
}

//==============================================

/*
===============
SV_ShutdownGameProgs

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
void SV_ShutdownGameProgs()
{
	if (!ge)
		return;
	ge->Game_Shutdown();
	Sys_UnloadGameLibrary();
	ge = NULL;
}

/*
===============
SV_InitGameProgs

Init the game subsystem for a new map
===============
*/
void Render2D_DebugGraph(float value, int32_t r, int32_t g, int32_t b, int32_t a);

void SV_InitGameProgs()
{
	game_import_t import;

	// unload anything we have now
	if (ge)
		SV_ShutdownGameProgs();

	// load a new game dll
	import.multicast = SV_Multicast;
	import.unicast = PF_Unicast;
	import.bprintf = SV_BroadcastPrintf;
	import.dprintf = PF_dprintf;
	import.cprintf = PF_cprintf;
	import.Text_Draw = PF_Text_Draw;
	import.centerprintf = PF_centerprintf;
	import.error = PF_error;

	import.Edict_Link = SV_LinkEdict;
	import.Edict_Unlink = SV_UnlinkEdict;
	import.BoxEdicts = SV_AreaEdicts;
	import.trace = SV_Trace;
	import.pointcontents = SV_PointContents;
	import.setmodel = PF_setmodel;
	import.inPVS = PF_inPVS;
	import.inPHS = PF_inPHS;
	import.Player_Move = Player_Move;

	import.modelindex = SV_ModelIndex;
	import.soundindex = SV_SoundIndex;
	import.imageindex = SV_ImageIndex;

	import.configstring = PF_Configstring;
	import.sound = PF_StartSound;
	import.positioned_sound = SV_StartSound;

	import.ReadChar = PF_ReadChar;
	import.ReadByte = PF_ReadByte;
	import.ReadShort = PF_ReadShort;
	import.ReadInt = PF_ReadInt;
	import.ReadFloat = PF_ReadFloat;
	import.ReadString = PF_ReadString;
	import.ReadPos = PF_ReadPos;
	import.ReadDir = PF_ReadDir;
	import.ReadColor = PF_ReadColor;
	import.ReadAngle = PF_ReadAngle;

	import.WriteChar = PF_WriteChar;
	import.WriteByte = PF_WriteByte;
	import.WriteShort = PF_WriteShort;
	import.WriteInt = PF_WriteInt;
	import.WriteFloat = PF_WriteFloat;
	import.WriteString = PF_WriteString;
	import.WritePos = PF_WritePos;
	import.WriteDir = PF_WriteDir;
	import.WriteColor = PF_WriteColor;
	import.WriteAngle = PF_WriteAngle;

	import.TagMalloc = Memory_ZoneMallocTagged;
	import.TagFree = Memory_ZoneFree;
	import.FreeTags = Memory_ZoneFreeTags;

	import.Cvar_Get = Cvar_Get;
	import.Cvar_Set = Cvar_Set;
	import.Cvar_ForceSet = Cvar_ForceSet;

	import.Cmd_AddCommand = Cmd_AddCommand;
	import.Cmd_RemoveCommand = Cmd_RemoveCommand;
	import.Cmd_Argc = Cmd_Argc;
	import.Cmd_Argv = Cmd_Argv;
	import.Cmd_Args = Cmd_Args;
	import.AddCommandString = Cbuf_AddText;

	import.DebugGraph = Render2D_DebugGraph;
	import.SetAreaPortalState = Map_SetAreaPortalState;
	import.AreasConnected = Map_AreasConnected;

	ge = (game_export_t*)Sys_LoadGameLibrary(&import);

	if (!ge)
	{
		Com_Error(ERR_DROP, "failed to load game DLL");
		return;
	}

	if (ge->apiversion != GAME_API_VERSION)
		Com_Error(ERR_DROP, "game is version %i, not %i", ge->apiversion,
			GAME_API_VERSION);

	ge->Game_Init();
}

