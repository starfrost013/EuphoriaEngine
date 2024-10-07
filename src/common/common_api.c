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
#include "include/common_api.h"

// common_api.c: Implements initialisation and shutdown for EuphoriaCommon.dll
// September 24, 2024
common_api_export_t common;

void CommonAPI_Init()
{
	Com_Printf("Initialising Common API version %d\n", COMMON_API_VERSION);

	common.api_version = COMMON_API_VERSION;

	//common.EuphoriaCommon_Init = EuphoriaCommon_Init;
	//common.EuphoriaCommon_Shutdown = EuphoriaCommon_Shutdown;
	
	// Cmd
	common.Cmd_AddCommand = Cmd_AddCommand;
	common.Cmd_Argc = Cmd_Argc;
	common.Cmd_Args = Cmd_Args;
	common.Cmd_Argv = Cmd_Argv;
	common.Cmd_CompleteCommand = Cmd_CompleteCommand;
	common.Cmd_ExecuteString = Cmd_ExecuteString;
	common.Cmd_Exists = Cmd_Exists;
	common.Cmd_RemoveCommand = Cmd_RemoveCommand;
	common.Cmd_TokenizeString = Cmd_TokenizeString;
	
	//Miscellaneous Common
	common.Com_DPrintf = Com_DPrintf;
	common.Com_Error = Com_Error;
	common.Com_GetServerState = Com_GetServerState;
	common.Com_Printf = Com_Printf;
	common.Com_Quit = Com_Quit;
	common.Com_SetServerState = Com_SetServerState;

	//Cvar
	common.Cvar_ForceSet = Cvar_ForceSet;
	common.Cvar_FullSet = Cvar_FullSet;
	common.Cvar_Get = Cvar_Get;
	common.Cvar_Set = Cvar_Set;

	// JSON API
	common.JSON_CloseStream = JSON_close;
	common.JSON_GetError = JSON_get_error;
	common.JSON_GetLineNumber = JSON_get_lineno;
	common.JSON_GetLinePosition = JSON_get_position;
	common.JSON_GetNumber = JSON_get_number;
	common.JSON_GetString = JSON_get_string;
	common.JSON_Next = JSON_next;
	common.JSON_OpenStream = JSON_open_stream;
	common.JSON_Reset = JSON_reset;
	common.JSON_Skip = JSON_skip;

	// Localisation Engine
	common.Localisation_GetString = Localisation_GetString;
	common.Localisation_ProcessString = Localisation_ProcessString;

	// Map
	common.Map_AreasConnected = Map_AreasConnected;
	common.Map_BoxLeafnums = Map_BoxLeafnums;
	common.Map_BoxTrace = Map_BoxTrace;
	common.Map_ClusterPHS = Map_ClusterPHS;
	common.Map_ClusterPVS = Map_ClusterPVS;
	common.Map_GetCurrentName = Map_GetCurrentName;
	common.Map_GetEntityString = Map_GetEntityString;
	common.Map_GetLeafCluster = Map_GetLeafCluster;
	common.Map_GetLeafContents = Map_GetLeafContents;
	common.Map_GetNumClusters = Map_GetNumClusters;
	common.Map_HeadnodeForBox = Map_HeadnodeForBox;
	common.Map_HeadnodeVisible = Map_HeadnodeVisible;
	common.Map_LeafArea = Map_LeafArea;
	common.Map_Load = Map_Load;
	common.Map_LoadInlineModel = Map_LoadInlineModel;
	common.Map_NumInlineModels = Map_NumInlineModels;
	common.Map_PointContents = Map_PointContents;
	common.Map_PointLeafnum = Map_PointLeafnum;
	common.Map_ReadPortalState = Map_ReadPortalState;
	common.Map_SetAreaPortalState = Map_SetAreaPortalState;
	common.Map_TransformedBoxTrace = Map_TransformedBoxTrace;
	common.Map_TransformedPointContents = Map_TransformedPointContents;
	common.Map_WriteAreaBits = Map_WriteAreaBits;
	common.Map_WritePortalState = Map_WritePortalState;

	// Zone memory allocator
	common.Memory_ZoneFree = Memory_ZoneFree;
	common.Memory_ZoneFreeTags = Memory_ZoneFreeTags;
	common.Memory_ZoneMalloc = Memory_ZoneMalloc;
	common.Memory_ZoneMallocTagged = Memory_ZoneMallocTagged;

	// Network Messaging (read)
	common.MSG_BeginReading = MSG_BeginReading;
	common.MSG_ReadAngle = MSG_ReadAngle;
	common.MSG_ReadByte = MSG_ReadByte;
	common.MSG_ReadChar = MSG_ReadChar;
	common.MSG_ReadColor = MSG_ReadColor;
	common.MSG_ReadDir = MSG_ReadDir;
	common.MSG_ReadFloat = MSG_ReadFloat;
	common.MSG_ReadInt = MSG_ReadInt;
	common.MSG_ReadPos = MSG_ReadPos;
	common.MSG_ReadShort = MSG_ReadShort;
	common.MSG_ReadString = MSG_ReadString;

	// Network Messaging (Write)
	common.MSG_WriteAngle = MSG_WriteAngle;
	common.MSG_WriteByte = MSG_WriteByte;
	common.MSG_WriteChar = MSG_WriteChar;
	common.MSG_WriteColor = MSG_WriteColor;
	common.MSG_WriteDir = MSG_WriteDir;
	common.MSG_WriteFloat = MSG_WriteFloat;
	common.MSG_WriteInt = MSG_WriteInt;
	common.MSG_WritePos = MSG_WritePos;
	common.MSG_WriteShort = MSG_WriteShort;
	common.MSG_WriteString = MSG_WriteString;

	// Network channels
	common.Netchan_Process = Netchan_Process;
	common.Netchan_Transmit = Netchan_Transmit;

	// Physics
	common.Player_Move = Player_Move;
	// Endianness stuff
	common.BigFloat = BigFloat;
	common.BigInt = BigInt;
	common.BigIntUnsigned = BigIntUnsigned;
	common.BigShort = BigShort;
	common.BigShortUnsigned = BigIntUnsigned;

	common.LittleFloat = LittleFloat;
	common.LittleInt = LittleInt;
	common.LittleIntUnsigned = LittleIntUnsigned;
	common.LittleShort = LittleShort;
	common.LittleShortUnsigned = LittleIntUnsigned;
}

common_api_export_t CommonAPI_Get() 
{
	return common;
}