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
// client_api.c: Client interface object for euphoriacommon import implementation
// October 3, 2024

#pragma once 
#include <client/include/client_api.h>
#include <client/client.h>


void Interface_SetClient()
{
	client.api_version = CLIENT_API_VERSION;
	client.CL_Drop = CL_Drop;
	client.CL_ForwardCmdToServer = Cmd_ForwardToServer;
	client.CL_Frame = CL_Frame;
	client.CL_Init = CL_Init;
	client.CL_Shutdown = CL_Shutdown;
}

client_api_t Interface_GetClient()
{
	return client; 
}