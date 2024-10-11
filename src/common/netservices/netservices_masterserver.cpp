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
// netservices_masterserver.c : Netservices master server

#include "netservices.hpp"

// Defines
#define URL_QUERY_STRING_LENGTH	512

// Globals
master_server_entry_t master_server_entries[MAX_MASTER_SERVER_ENTRIES];

// Functions only defined in this translation unit are also defined here if they end up being called earlier in the file than when they are declared

// Called on JSON receive.
size_t Netservices_OnMasterHeartbeatRecv(char* ptr, size_t size, size_t nmemb, char* userdata)
{
	return size;
}

void Netservices_MasterHeartbeat()
{
	char heartbeat_query_string[URL_QUERY_STRING_LENGTH] = { 0 };

	Netservices_AddCurlObject(SERVICE_BASE_URL_HEARTBEAT, true, http_method_post, Netservices_OnMasterHeartbeatRecv, "?heartbeat_id=");
}

void Netservices_MasterAddServer(master_server_entry_t entry)
{

}

void Netservices_MasterDeleteServer()
{

}

void Netservices_MasterGetServer()
{

}

void Netservices_MasterListServers()
{

}

void Netservices_IdToString(char* str)
{

}