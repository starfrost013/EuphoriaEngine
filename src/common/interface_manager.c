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

client_api_t client;
server_api_t server;
sys_api_t sys;
