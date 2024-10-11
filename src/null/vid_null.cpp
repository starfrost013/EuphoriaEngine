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

// Vid_null.c -- null video driver to aid porting efforts (and for the dedicated server binary)
// this assumes that one of the refs is statically linked to the executable

#include "../client/client.h"

refexport_t	re;

GetRefAPI_t	GetRefAPI;

// misc stuff
cvar_t* r_width;
cvar_t* r_height;

// Global variables used internally by this module
#ifdef WIN32
HINSTANCE	reflib_library;		// Handle to refresh DLL 
#endif

/*
==========================================================================

DIRECT LINK GLUE

==========================================================================
*/

#define	MAX_PRINT_MSG	4096
void Vid_Printf(int32_t print_level, const char* fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	if (print_level == PRINT_ALL)
		Com_Printf("%s", msg);
	else
		Com_DPrintf("%s", msg);
}

void Vid_Error(int32_t err_level, const char* fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	Com_Error(err_level, "%s", msg);
}

void Vid_ChangeResolution(int32_t width, int32_t height)
{
	r_width->value = width;
	r_height->value = height;
}


void Vid_Init()
{
	/* Create the video variables so we know how to start the graphics drivers */
	r_width = Cvar_Get("r_width", "1366", CVAR_ARCHIVE);
	r_height = Cvar_Get("r_height", "768", CVAR_ARCHIVE);

	/* Start the graphics mode and load refresh DLL */
	Vid_CheckChanges();
}

void Vid_Shutdown(void)
{

}

void Vid_CheckChanges(void)
{
}

void Vid_MenuDraw(void)
{
}

void Render2D_BeginLoadingPlaque()
{

}

void Render2D_EndLoadingPlaque()
{

}

void Render2D_DebugGraph(float value, int32_t r, int32_t g, int32_t b, int32_t a)
{

}

const char* Vid_MenuKey(int32_t k)
{
	return NULL;
}
