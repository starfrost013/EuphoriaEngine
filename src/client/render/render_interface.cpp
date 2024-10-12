/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018-2019 Krzysztof Kondrak
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

// Main windowed and fullscreen graphics interface module. This module
// is used for all of the engine's renderers, and is used in all platforms

#include <cassert>
#include <cfloat>

#include <client/client.hpp>

// Structure containing functions exported from refresh DLL
refexport_t	re;

// Console variables that we need to access from this module
cvar_t* vid_gamma;
cvar_t* vid_ref;			// Name of Refresh DLL loaded
cvar_t* vid_xpos;			// X coordinate of window position
cvar_t* vid_ypos;			// Y coordinate of window position
cvar_t* vid_borderless;
cvar_t* vid_fullscreen;
cvar_t* vid_refresh;
cvar_t* viewsize;

// Global variables used internally by this module
#ifdef WIN32
HINSTANCE	reflib_library;		// Handle to refresh DLL 
#endif
bool		graphics_mode = false;

extern uint32_t	sys_msg_time;

/*
==========================================================================

DLL GLUE

==========================================================================
*/

#define	MAXPRINTMSG	8192
void Vid_Printf(int32_t print_level, const char* fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static bool	inupdate;

	va_start(argptr, fmt);
	vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	if (print_level == PRINT_ALL)
	{
		Com_Printf("%s", msg);
	}
	else if (print_level == PRINT_DEVELOPER)
	{
		Com_DPrintf("%s", msg);
	}
	else if (print_level == PRINT_ALERT)
	{
		// ICONWARNING
		Sys_Msgbox("Alert (PRINT_ALERT)", 0x30, msg);
	}
}

void Vid_Error(int32_t err_level, const char* fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static bool	inupdate;

	va_start(argptr, fmt);
	vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	Com_Error(err_level, "%s", msg);
}

/*
============
Vid_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void Vid_Restart_f()
{
	vid_ref->modified = true;
}

/*
** Vid_ChangeResolution
*/
void Vid_ChangeResolution()
{
	cl.force_refdef = true;		// can't use a paused refdef

	char hudscale[5];
	memset(hudscale, 0, sizeof(hudscale));

	float wscale = r_width->value / UI_SCALE_BASE_X;
	float hscale = r_height->value / UI_SCALE_BASE_Y;

	if (wscale > hscale) wscale = hscale;
	if (wscale < 1) wscale = 1;

	snprintf(hudscale, 4, "%.2f", wscale);
	vid_hudscale = Cvar_Set("hudscale", hudscale);
}

void Vid_FreeReflib()
{
	if (!FreeLibrary(reflib_library))
		Com_Error(ERR_FATAL, "Reflib FreeLibrary failed");
	memset(&re, 0, sizeof(re));
	reflib_library = NULL;
	graphics_mode = false;
}

/*
==============
Vid_LoadRefresh
==============
*/
bool Vid_LoadRefresh(char* name)
{
	refimport_t	ri = { 0 };
	GetRefAPI_t	GetRefAPI;

	if (graphics_mode)
	{
		re.Shutdown();
		Vid_FreeReflib();
	}

	Com_Printf("------- Loading %s -------\n", name);

	if ((reflib_library = LoadLibrary(name)) == 0)
	{
		Com_Printf("LoadLibrary(\"%s\") failed\n", name);

		return false;
	}

	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Con_Printf = Vid_Printf;
	ri.Sys_Error = Vid_Error;
	ri.FS_LoadFile = FS_LoadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_Gamedir = FS_Gamedir;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_SetValue = Cvar_SetValue;
	ri.Vid_ChangeResolution = Vid_ChangeResolution;
	ri.Com_Quit = Com_Quit;

	ri.JSON_CloseStream = JSON_close;
	ri.JSON_GetError = JSON_get_error;
	ri.JSON_GetLineNumber = JSON_get_lineno;
	ri.JSON_GetNumber = JSON_get_number;
	ri.JSON_GetPosition = JSON_get_position;
	ri.JSON_GetString = JSON_get_string;
	ri.JSON_Next = JSON_next;
	ri.JSON_OpenStream = JSON_open_stream;
	ri.JSON_Peek = JSON_peek;

	if ((GetRefAPI = (GetRefAPI_t)GetProcAddress(reflib_library, "GetRefAPI")) == 0)
		Com_Error(ERR_FATAL, "GetProcAddress failed on %s", name);

	re = GetRefAPI(ri);

	if (re.api_version != API_VERSION)
	{
		Vid_FreeReflib();
		Com_Error(ERR_FATAL, "%s has incompatible api_version", name);
	}

	if (re.Init() == false)
	{
		re.Shutdown();
		Vid_FreeReflib();
		return false;
	}

	Com_Printf("------------------------------------\n");
	graphics_mode = true;

	vidref_val = VIDREF_OTHER;
	if (vid_ref)
	{
		if (!strcmp(vid_ref->string, "gl"))
			vidref_val = VIDREF_GL;
	}

	// set up the event procs
	re.SetKeyPressedProc(Key_Event);
	re.SetMousePressedProc(MouseClick_Event);
	re.SetMouseMovedProc(MouseMove_Event);
	re.SetMouseScrollProc(MouseScroll_Event);
	re.SetWindowFocusProc(WindowFocus_Event);
	re.SetWindowIconifyProc(WindowIconify_Event);
	return true;
}

/*
============
Vid_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to
update the rendering DLL and/or video mode to match.
============
*/
void Vid_CheckChanges()
{
	char name[100];

	if (vid_ref->modified)
	{
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds();
	}

	while (vid_ref->modified)
	{
		/*
		** refresh has changed
		*/
		vid_ref->modified = false;
		vid_borderless->modified = true;
		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		snprintf(name, sizeof(name), "ref_%s.dll", vid_ref->string);
		if (!Vid_LoadRefresh(name))
		{
			Com_Error(ERR_FATAL, "Failed to initialise renderer. Renderer name: %s", vid_ref->string);

			/*
			** drop the console if we fail to load a refresh
			*/
			if (cls.input_dest != key_console)
			{
				Con_ToggleConsole_f();
			}
		}

		// turn off the cursor if we are switching into fullscreen mode
		if (vid_fullscreen->value
			|| vid_borderless->value)
		{
			re.EnableCursor(false);
		}

		cls.disable_screen = false;
	}

	/*
	** update our window position
	*/
	if (vid_xpos->modified || vid_ypos->modified)
	{
		if (!vid_borderless->value
			&& !vid_fullscreen->value)
		{
			re.SetWindowPosition(vid_xpos->value, vid_ypos->value);
		}

		vid_xpos->modified = false;
		vid_ypos->modified = false;
	}

	if (vid_refresh->modified)
	{
		vid_refresh->modified = false;
		cl.refresh_prepped = false;
	}
}

/*
============
Vid_Init

Initialises the video/rendering subsystem
============
*/
void Vid_Init()
{
	/* Create the video variables so we know how to start the graphics drivers */
	r_width = Cvar_Get("r_width", "1366", CVAR_ARCHIVE);
	r_height = Cvar_Get("r_height", "768", CVAR_ARCHIVE);

	vid_ref = Cvar_Get("vid_ref", "gl", CVAR_ARCHIVE);
	vid_xpos = Cvar_Get("vid_xpos", "3", CVAR_ARCHIVE);
	vid_ypos = Cvar_Get("vid_ypos", "22", CVAR_ARCHIVE);
	vid_borderless = Cvar_Get("vid_borderless", "0", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_refresh = Cvar_Get("vid_refresh", "0", CVAR_NOSET);
	vid_gamma = Cvar_Get("vid_gamma", "1", CVAR_ARCHIVE);
	viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);

	/* Add some console commands that we want to handle */
	Cmd_AddCommand("vid_restart", Vid_Restart_f);

	/* Start the graphics mode and load refresh DLL */
	Vid_CheckChanges();
}

/*
============
Vid_Shutdown
============
*/
void Vid_Shutdown()
{
	if (graphics_mode)
	{
		re.Shutdown();
		Vid_FreeReflib();
	}
}