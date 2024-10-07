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
// server_api.h: Server interface object for euphoriacommon import
// September 30, 2024

#pragma once

#include <stdint.h>
#include <stdbool.h>


#define SERVER_API_VERSION	1

typedef struct server_api_s
{
	int32_t	api_version;

	void	(*SV_Init)();
	void	(*SV_Shutdown)();
	void	(*SV_Frame)(int32_t msec);
	void	(*SV_ShutdownGame)();

} server_api_t;

extern server_api_t server;

void ServerAPI_Init();
server_api_t ServerAPI_Get();