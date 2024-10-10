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
// interface_manager.c: Manages loading interfaces to initialise the various engine subsystems (e.g. client, server, sys...)
// October 7, 2024

#include "common.h"
#include <client/include/client_api.h>
#include <server/server_api.h>
#include <sys_api.h>
#ifdef _WIN32
#include <windows.h>
HMODULE engine_module;
#else
#include <dlfcn.h>
void* engine_module;
#endif

client_api_t client;
server_api_t server;
sys_api_t sys;

#define BINARY_ENGINE "EuphoriaEngine"
#define BINARY_DEDICATED_SERVER "EuphoriaDS"

#define INTERFACE_NAME_GET_SYS "Interface_GetSys"
#define INTERFACE_NAME_GET_CLIENT "Interface_GetClient"
#define INTERFACE_NAME_GET_SERVER "Interface_GetServer"

void* Interface_GetSysProc;
void* Interface_GetClientProc;
void* Interface_GetServerProc;

// Loads a library
bool Interface_LoadLibrary(char* name)
{
	// THIS MUST BE CALLED AFTER EUPHORIAENGINE LOADS EUPHORIACOMMON
#ifdef _WIN32
	engine_module = GetModuleHandle(name);
#else
	engine_module = dlopen(name, RTLD_LAZY);
#endif

	return (engine_module != NULL);
}

// is_dedicated_server_binary = engine DEDICATED_ONLY
bool Interface_Init(bool is_dedicated_server_binary)
{
	bool succeeded = false;

	if (is_dedicated_server_binary)
		succeeded = Interface_LoadLibrary(BINARY_DEDICATED_SERVER);
	else
		succeeded = Interface_LoadLibrary(BINARY_ENGINE);

	if (!succeeded)
		Com_Error(ERR_FATAL, "Couldn't find either the engine or the dedicated server to bind to.");

	Interface_SetSys();
	Interface_SetClient();
	Interface_SetServer();

	return true; 
}

void Interface_SetSys()
{
#ifdef WIN32
	Interface_GetSysProc = GetProcAddress(engine_module, INTERFACE_NAME_GET_SYS);
#else
	Interface_GetSysProc = dlsym(engine_module, INTERFACE_NAME_GET_SYS);
#endif

	if (!Interface_GetSysProc)
		Com_Error(ERR_FATAL, "Interface_GetSys not found, incorrect engine version");

	Com_Printf("Loading interface: sys\n");

	if (sys.api_version != SYS_API_VERSION)
		Com_Error(ERR_FATAL, "Incorrect sys interface version %d (%d expected)", client.api_version, CLIENT_API_VERSION);

}

// Loads the interface objects.
// Takes a boolean to see if we are using the dedicated server binary.
void Interface_SetClient()
{
#ifdef WIN32
	Interface_GetClientProc = GetProcAddress(engine_module, INTERFACE_NAME_GET_CLIENT);
#else
	Interface_GetClientProc = dlsym(engine_module, INTERFACE_NAME_GET_CLIENT);
#endif

	if (!Interface_GetClientProc)
		Com_Error(ERR_FATAL, "Interface_GetClient not found, incorrect engine version");

	Com_Printf("Loading interface: client\n");

	if (client.api_version != CLIENT_API_VERSION)
		Com_Error(ERR_FATAL, "Incorrect client interface version %d (%d expected)", client.api_version, CLIENT_API_VERSION);
}

void Interface_SetServer()
{
#ifdef WIN32
	Interface_GetServerProc = GetProcAddress(engine_module, INTERFACE_NAME_GET_SERVER);
#else
	Interface_GetServerProc = dlsym(engine_module, INTERFACE_NAME_GET_SERVER);
#endif

	if (!Interface_GetServerProc)
		Com_Error(ERR_FATAL, "Interface_GetServer not found, incorrect engine version");

	Com_Printf("Loading interface: server\n");

	if (server.api_version != SERVER_API_VERSION)
		Com_Error(ERR_FATAL, "Incorrect server interface version %d (%d expected)", server.api_version, SERVER_API_VERSION);
}