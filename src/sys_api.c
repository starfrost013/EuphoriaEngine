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
#include <sys_api.h>
#include <common/common.h>

// sys_api.h: Provides system-specific APIs, so euphoriacommon can use them
// September 21, 2024

sys_api_t sys;

void SystemAPI_Init()
{
	sys.api_version = SYS_API_VERSION;

	sys.Sys_ConsoleInput = Sys_ConsoleInput;
	sys.Sys_ConsoleOutput = sys.Sys_ConsoleOutput;
	sys.Sys_Error = Sys_Error; 
	sys.Sys_Init = Sys_Init;
	sys.Sys_Milliseconds = Sys_Milliseconds;
	sys.Sys_MillisecondsGet = Sys_MillisecondsGet;
	sys.Sys_Msgbox = Sys_Msgbox;
	sys.Sys_Nanoseconds = Sys_Nanoseconds;
	sys.Sys_NanosecondsGet = Sys_NanosecondsGet;
	sys.Sys_Quit = Sys_Quit;

	sys.Net_Init = Net_Init;
	sys.Net_AdrToString = Net_AdrToString;
	sys.Net_SendPacket = Net_SendPacket;
}

sys_api_t SystemAPI_Get()
{
	return sys;
}