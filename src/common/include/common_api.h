/*
Euphoria Game Engine 
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

#pragma once
#include <common/common.h>

// common_api.h: Provides common API stuff for initing euphoriacommon.dll - SHARED across client, server, gamedll
// September 21, 2024

#define COMMON_API_VERSION		1

// Functions exported from euphoriacommon.dll
typedef struct common_api_export_s
{
	int32_t		api_version;													// the api version

	// Init & Shutdown
	void		(*EuphoriaCommon_Init);											// Initialise the common API - this calls Eupho
	void		(*EuphoriaCommon_Shutdown);										// Shut down the common API
		
	// Command
	int32_t		(*Cmd_Argc)();
	char*		(*Cmd_Argv)(int32_t n);
	char*		(*Cmd_Args)();	// concatenation of all argv >= 1
	void		(*Cmd_TokenizeString)(char* text, bool macro_expand);
	void		(*Cmd_AddCommand)(char* cmd_name, xcommand_t function);
	void		(*Cmd_RemoveCommand)(char* cmd_name);	
	char*		(*Cmd_CompleteCommand)(char* partial);
	bool		(*Cmd_Exists)(char* cmd_name);
	void		(*Cmd_ExecuteString)(char* text);

	// Common State
	int32_t		(*Com_GetServerState)();										// Get the server state
	void		(*Com_SetServerState)(int32_t state);							// Set the server state.

	// Common Misc
	void		(*Com_Printf)(char* msg, ...);									// Print a message to the console
	void		(*Com_DPrintf)(char* msg, ...);									// Print a message to the console (debug builds only)
	void		(*Com_Error)(int32_t code, char* fmt, ...);						// Error handling
	void		(*Com_Quit)();													// Quit Common

	// CPUID (CPUID information available via cvars)
	
	bool		(*CPUID_IsDefectiveCPU)();										// Is the CPU a potentially defective Intel part?

	// CVar
	cvar_t*		(*Cvar_Get)(char* var_name, char* var_value, int32_t flags);	// Get and optionally create a cvar.
	cvar_t*		(*Cvar_Set)(char* var_name, char* value);						// Set a cvar
	cvar_t*		(*Cvar_ForceSet)(char* var_name, char* value);					// Force-set a cvar
	cvar_t*		(*Cvar_FullSet)(char* var_name, char* value, int32_t flags);	// Set a cvar including flags

	// Gameinfo is handled with cvars

	// JSON
	void		(*JSON_OpenStream)(JSON_stream* json, FILE* stream);			// Open a JSON stream
	enum JSON_type (*JSON_Next)(JSON_stream* json);								// Get the next JSON object
	const char* (*JSON_GetError)(JSON_stream* json);							// Get the most recent error from a JSON stream
	const char* (*JSON_GetString)(JSON_stream* json, size_t* length);			// Get a string from a JSON object
	double		(*JSON_GetNumber)(JSON_stream* json);							// Get a number from a JSON object
	void		(*JSON_Skip)(JSON_stream* json);								// Skip an object within a JSON stream
	size_t		(*JSON_GetLineNumber)(JSON_stream* json);						// Get the current line within a JSON stream
	size_t		(*JSON_GetLinePosition)(JSON_stream* json);							// Get the current position within a JSON stream
	void		(*JSON_Reset)(JSON_stream* json);								// Reset a JSON stream
	void		(*JSON_CloseStream)(JSON_stream* json);							// Close a JSON stream

	// Localisation
	localisation_entry_t* (*Localisation_GetString)(char* key);					// Get a localisation string
	char*		(*Localisation_ProcessString)(char* value);						// Process a string for localisation entries.

	// CMod / Map
	cmodel_t*	(*Map_Load)(char* name, bool clientload, uint32_t* checksum);	// Start loading a map
	cmodel_t*	(*Map_LoadInlineModel)(char* name);	// *1, *2, etc

	int32_t		(*Map_GetNumClusters)();
	int32_t		(*Map_NumInlineModels)();
	char*		(*Map_GetEntityString)();

	// creates a clipping hull for an arbitrary box
	int32_t		(*Map_HeadnodeForBox)(vec3_t mins, vec3_t maxs);

	// returns an ORed contents mask
	int32_t		(*Map_PointContents)(vec3_t p, int32_t headnode);
	int32_t		(*Map_TransformedPointContents)(vec3_t p, int32_t headnode, vec3_t origin, vec3_t angles);

	trace_t		(*Map_BoxTrace)(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int32_t headnode, int32_t brushmask);
	trace_t		(*Map_TransformedBoxTrace)(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int32_t headnode, int32_t brushmask, vec3_t origin, vec3_t angles);

	uint8_t*	(*Map_ClusterPVS)(int32_t cluster);
	uint8_t*	(*Map_ClusterPHS)(int32_t cluster);

	// call with topnode set to the headnode, returns with topnode
	// set to the first node that splits the box
	int32_t		(*Map_BoxLeafnums)(vec3_t mins, vec3_t maxs, int32_t* list, int32_t listsize, int32_t* topnode);

	int32_t		(*Map_GetLeafContents)(int32_t leafnum);
	int32_t		(*Map_GetLeafCluster)(int32_t leafnum);
	int32_t		(*Map_LeafArea)(int32_t leafnum);

	void		(*Map_SetAreaPortalState)(int32_t portalnum, bool open);
	bool		(*Map_AreasConnected)(int32_t area1, int32_t area2);

	int32_t		(*Map_WriteAreaBits)(uint8_t* buffer, int32_t area);
	bool		(*Map_HeadnodeVisible)(int32_t headnode, uint8_t* visbits);

	void		(*Map_WritePortalState)(FILE* f);
	void		(*Map_ReadPortalState)(FILE* f);
	int32_t 	(*Map_PointLeafnum)(vec3_t p);
	char*		(*Map_GetCurrentName)();										// Get the current map name.

	// Memory allocation (Zone)
	void		(*Memory_ZoneFree)(void* ptr);									// Free zone allocated memory
	void*		(*Memory_ZoneMalloc)(int32_t size);								// Zone Allocate memory
	void*		(*Memory_ZoneMallocTagged)(int32_t size, int32_t tag);			// Zone Allocate memory with a tag
	void		(*Memory_ZoneFreeTags)(int32_t tag);							// Zone Free memory

	// Network Channel
	void		(*Netchan_Transmit)(netchan_t* chan, int32_t length, uint8_t* data);
	void		(*Netchan_Process)(netchan_t* chan, sizebuf_t* msg);				
	
	// Network Messaging
	void		(*MSG_BeginReading)(sizebuf_t* msg);

	int32_t		(*MSG_ReadChar)();
	int32_t		(*MSG_ReadByte)();
	int32_t		(*MSG_ReadShort)();
	int32_t		(*MSG_ReadInt)();
	float		(*MSG_ReadFloat)();
	char*		(*MSG_ReadString)(char* s);
	void		(*MSG_ReadPos)(vec3_t vector);									// some fractional bits
	void		(*MSG_ReadDir)(vec3_t dir);
	void		(*MSG_ReadColor)(color4_t color);
	float		(*MSG_ReadAngle)(float f);


	void		(*MSG_WriteChar)(int32_t c);
	void		(*MSG_WriteByte)(int32_t c);
	void		(*MSG_WriteShort)(int32_t c);
	void		(*MSG_WriteInt)(int32_t c);
	void		(*MSG_WriteFloat)(float f);
	void		(*MSG_WriteString)(char* s);
	void		(*MSG_WritePos)(vec3_t pos);									// some fractional bits
	void		(*MSG_WriteDir)(vec3_t dir);
	void		(*MSG_WriteColor)(color4_t color);
	void		(*MSG_WriteAngle)(float f);
	void		(*MSG_WriteDeltaUsercmd)(sizebuf_t* buf, usercmd_t* from, 
				usercmd_t* cmd);
	void		(*MSG_WriteDeltaEntity)(entity_state_t* from, 
				entity_state_t* to, sizebuf_t* msg, bool force, bool newentity);

	// Pmove
	void		(*Player_Move)(pmove_t* pmove);									// Player movement

	// Miscellaneous crap
	void		(*Info_Print)(char* s);

	int16_t		(*BigShort) (int16_t l);
	int16_t		(*LittleShort) (int16_t l);
	uint16_t	(*BigShortUnsigned) (int16_t l);
	uint16_t	(*LittleShortUnsigned) (int16_t l);
	int32_t		(*BigInt) (int32_t l);
	int32_t		(*LittleInt) (int32_t l);
	uint32_t	(*BigIntUnsigned) (int32_t l);
	uint32_t	(*LittleIntUnsigned) (int32_t l);
	float		(*BigFloat) (float l);
	float		(*LittleFloat) (float l);

	// System-specific Services

} common_api_export_t;

// the instance of the common_api_export
extern common_api_export_t common;

void CommonAPI_Init();
common_api_export_t CommonAPI_Get();