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
// sys_win.c: Windows specific code

#include <common/common.hpp>
#include <client/client.hpp>
#include "platform_win32.hpp"
#include "win32_resource.h"

#include <cerrno>
#include <cfloat>
#include <cstdio>

#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <win32/win32_conproc.hpp>

// to make euphoriads compile
#ifdef DEDICATED_ONLY
bool			app_active;
#else
#include <client/include/ui_game_interface.hpp>
#endif

static HANDLE	hinput, houtput;

uint32_t		sys_msg_time;

#define	MAX_NUM_ARGVS 128

int32_t argc;
char*	argv[MAX_NUM_ARGVS];

int32_t Sys_MsgboxV(const char* title, uint32_t buttons, const char* text, va_list args);


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

#define MAX_MSGBOX_TEXT_LENGTH 1024

#ifdef _MSC_VER
// this gets rid of RetardCompiler warnings
__declspec(noreturn) void Sys_Error(const char* error, ...)
#else
__attribute__((noreturn)) void Sys_Error(const char* error, ...)
#endif
{
	CL_Shutdown();
	Common_Shutdown();

	va_list args;

	va_start(args, error);

	Sys_MsgboxV("Fatal Error", MB_OK, error, args);
	va_end(args);

	// shut down QHOST hooks if necessary 
	DeinitConProc();

	exit(1);
}

// returns a value indicating which buttons were pressed
int32_t Sys_Msgbox(const char* title, uint32_t buttons, const char* text, ...)
{
	va_list	args;

	char text_processed[1024] = { 0 };

	va_start(args, text_processed);

	vsnprintf(text_processed, 1024, text, args);
	va_end(args);

	const char* title_processed_ptr = title;
	const char* text_processed_ptr = text_processed;

	if (localisation_initialised)
	{
		title_processed_ptr = Localisation_ProcessString(title);
		text_processed_ptr = Localisation_ProcessString(text);
	}

	return MessageBox(NULL, text_processed_ptr, title_processed_ptr, buttons);
}

int32_t Sys_MsgboxV(const char* title, uint32_t buttons, const char* text, va_list args)
{
	char text_processed[MAX_MSGBOX_TEXT_LENGTH] = { 0 };

	vsnprintf(text_processed, MAX_MSGBOX_TEXT_LENGTH, text, args);

	return MessageBox(NULL, text_processed, title, buttons);
}

void Sys_Quit()
{
	timeEndPeriod(1);

	CL_Shutdown();
	Netservices_Shutdown();
	Common_Shutdown();
	if (dedicated && dedicated->value)
		FreeConsole();
	else if (debug_console->value)
		FreeConsole();

	// shut down QHOST hooks if necessary
	DeinitConProc();

	exit(0);
}
/*
================
Sys_SetDPIAwareness

================
*/
typedef enum { dpi_unaware = 0, dpi_system_aware = 1, dpi_monitor_aware = 2 } dpi_awareness;
typedef BOOL(WINAPI* SetProcessDPIAwareFunc)();
typedef HRESULT(WINAPI* SetProcessDPIAwarenessFunc)(dpi_awareness value);

void Sys_SetDPIAwareness()
{
	HMODULE hShcore = LoadLibraryA("Shcore.dll");
	HMODULE hUser32 = LoadLibraryA("user32.dll");
	SetProcessDPIAwarenessFunc setProcDPIAwareness = (SetProcessDPIAwarenessFunc)(hShcore ? GetProcAddress(hShcore, "SetProcessDpiAwareness") : NULL);
	SetProcessDPIAwareFunc setProcDPIAware = (SetProcessDPIAwareFunc)(hUser32 ? GetProcAddress(hUser32, "SetProcessDPIAware") : NULL);

	if (setProcDPIAwareness) /* Windows 8.1+ */
		setProcDPIAwareness(dpi_monitor_aware);
	else if (setProcDPIAware) /* Windows Vista-8.0 */
		setProcDPIAware();

	if (hShcore)
		FreeLibrary(hShcore);
	if (hUser32)
		FreeLibrary(hUser32);
}

//================================================================


/*
================
Sys_Init
================
*/
void Sys_Init()
{
	OSVERSIONINFO vinfo;

	timeBeginPeriod(1);

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx(&vinfo))
		Sys_Error("Couldn't get OS info");

	// VS2022 requires Windows 7
	// GetVersionEx fuckery didn't start until Windows 8.1 so it's safe
	if (vinfo.dwMajorVersion < 6
		|| (vinfo.dwMajorVersion == 6 && vinfo.dwMinorVersion < 1))
	{
		// TODO: LOCALISE THIS TEXT
		Sys_Error("%s requires Windows 7 or later!", gameinfo.name);
	}

	if (dedicated->value)
	{
		if (!AllocConsole())
			Sys_Error("Couldn't create dedicated server console");

		hinput = GetStdHandle(STD_INPUT_HANDLE);
		houtput = GetStdHandle(STD_OUTPUT_HANDLE);

		// let QHOST hook in
		InitConProc(argc, argv);
	}
	else if (debug_console->value)
	{
		AllocConsole();
		SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
		DeleteMenu(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE, MF_BYCOMMAND);
		freopen("CONOUT$", "w", stderr);
	}

	// enable DPI awareness
	Sys_SetDPIAwareness();
}


static char	console_text[256];
static int32_t console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char* Sys_ConsoleInput()
{
	INPUT_RECORD	recs[1024];
	int32_t 		dummy;
	uint64_t		ch, numread, numevents; //64bit

	if (!dedicated || !dedicated->value)
		return NULL;


	for (;; )
	{
		if (!GetNumberOfConsoleInputEvents(hinput, (LPDWORD)&numevents))
			Sys_Error("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, (LPDWORD)&numread))
			Sys_Error("Error reading console input");

		if (numread != 1)
			Sys_Error("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
				case '\r':
					WriteFile(houtput, "\r\n", 2, (LPDWORD)&dummy, NULL);

					if (console_textlen)
					{
						console_text[console_textlen] = 0;
						console_textlen = 0;
						return console_text;
					}
					break;

				case '\b':
					if (console_textlen)
					{
						console_textlen--;
						WriteFile(houtput, "\b \b", 3, (LPDWORD)&dummy, NULL);
					}
					break;

				default:
					if (ch >= ' ')
					{
						if (console_textlen < sizeof(console_text) - 2)
						{
							WriteFile(houtput, &ch, 1, (LPDWORD)&dummy, NULL);
							console_text[console_textlen] = ch;
							console_textlen++;
						}
					}

					break;

				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput(char* string)
{
	int32_t dummy;
	char	text[256];

	if (!dedicated || !dedicated->value)
	{
		if (debug_console && debug_console->value)
		{
			fputs(string, stderr);
			OutputDebugString(string);
		}
		return;
	}

	if (console_textlen)
	{
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen);
		text[console_textlen + 1] = '\r';
		text[console_textlen + 2] = 0;
		WriteFile(houtput, text, console_textlen + 2, (LPDWORD)&dummy, NULL);
	}

	WriteFile(houtput, string, (DWORD)strlen(string), (LPDWORD)&dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, (LPDWORD)&dummy, NULL);
}

/*
================
Sys_GetClipboardData

================
*/
char* Sys_GetClipboardData(void)
{
	char* data = NULL;
	char* cliptext;

	if (OpenClipboard(NULL) != 0)
	{
		HANDLE hClipboardData;

		if ((hClipboardData = GetClipboardData(CF_TEXT)) != 0)
		{
			if ((cliptext = (char*)GlobalLock(hClipboardData)) != 0)
			{
				data = (char*)malloc(GlobalSize(hClipboardData) + 1);
				strcpy(data, cliptext);
				GlobalUnlock(hClipboardData);
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
========================================================================

Euphoria game libraries

	game*.dll (e.g. gamex64.dll)		Game Server DLL
	game_ui*.dll (e.g. game_uix64.dll)	Game Client/ UI DLL

========================================================================
*/

static HINSTANCE game_server_library;
static HINSTANCE game_ui_library;

#define GAME_SERVER_CREATE_PROC "Sys_GetGameAPI"
#define GAME_UI_INIT_PROC "GameUI_Init"


#if defined _M_IX86
#define GAME_SERVER_LIBRARY_NAME "gamex86.dll"
#elif defined _M_X64
#define GAME_SERVER_LIBRARY_NAME "gamex64.dll"
#endif

#if defined _M_IX86
#define GAME_UI_LIBRARY_NAME "game_uix86.dll"
#elif defined _M_X64
#define GAME_UI_LIBRARY_NAME "game_uix64.dll"
#endif

HINSTANCE Sys_LoadLibrary(const char* name)
{
	char* path = NULL;

	char dll_name[MAX_OSPATH] = { 0 };

	while (path = FS_NextPath(path))
	{
		// to be safe...
		if (!path)
			break;

		snprintf(dll_name, MAX_OSPATH, "%s/%s", path, name);
		
		HINSTANCE library_ptr = LoadLibrary(dll_name);

		if (library_ptr)
		{
			Com_Printf("Sys_LoadLibrary: Loaded library %s\n", dll_name);
			return library_ptr;
		}
		
		// failure case is handled by the caller
	}

	return NULL;
}

/*
=================
Sys_LoadGameServerLibrary

Loads the game dll
=================
*/
void* Sys_LoadGameServerLibrary(void* parms)
{
	void* (*game_server_create_proc) (void*);

	if (game_server_library)
		Com_Error(ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	// now run through the search paths
	game_server_library = Sys_LoadLibrary(GAME_SERVER_LIBRARY_NAME);

	if (!game_server_library)
	{
		Com_Printf("Failed to load game server library %s", GAME_SERVER_LIBRARY_NAME);
		return NULL;
	}

	game_server_create_proc = (void*(*)(void*))GetProcAddress(game_server_library, GAME_SERVER_CREATE_PROC); // wtf

	if (!game_server_create_proc)
	{
		Sys_UnloadGameServerLibrary();
		return NULL;
	}

	return game_server_create_proc(parms);
}

#ifndef DEDICATED_ONLY
// void* so that common.hpp can build
void* Sys_LoadGameUILibrary(void* engine_api)
{
	UI_InitInterface();
	// void* so that we don't have to include game.hpp from here

	// todo: UI Interface
	void* (*game_ui_create_proc)(void* engine_api);

	if (game_ui_library)
		Com_Error(ERR_FATAL, "Sys_LoadGameUILibrary without Sys_UnloadingGame");

	// now run through the search paths
	game_ui_library = Sys_LoadLibrary(GAME_UI_LIBRARY_NAME);

	if (!game_ui_library)
	{
		Com_Printf("Failed to load game UI library %s", GAME_UI_LIBRARY_NAME);
		return NULL;
	}

	game_ui_create_proc = (void*(*)(void* engine_api))GetProcAddress(game_ui_library, GAME_UI_INIT_PROC); // wtf

	if (!game_ui_create_proc)
	{
		Sys_UnloadGameUILibrary();
		return NULL;
	}

	return game_ui_create_proc(engine_api);
}
#endif

/*
=================
Sys_UnloadGameLibrary
=================
*/
void Sys_UnloadGameServerLibrary()
{
	if (!FreeLibrary(game_server_library))
		Com_Error(ERR_FATAL, "FreeLibrary failed for game server library");
	game_server_library = NULL;
}

#ifndef DEDICATED_ONLY
void Sys_UnloadGameUILibrary()
{
	if (!FreeLibrary(game_ui_library))
		Com_Error(ERR_FATAL, "FreeLibrary failed for game UI library");

	game_ui_library = NULL;
}
#endif
//=======================================================================


/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine(LPSTR lpCmdLine)
{
	argc = 1;

	// bizarre stupidity inherited from earlier code, didn't port to c++ easily
	argv[0] = (char*)"exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

}

/*
==================
WinMain

The entry point on Windows platform
==================
*/

int32_t WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nCmdShow)
{
	int32_t time, oldtime, newtime;

	ParseCommandLine(lpCmdLine);

	Common_Init(argc, argv);
	oldtime = Sys_Milliseconds();

	/* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
		if ((dedicated && dedicated->value))
		{
			Sleep(1);
		}

		do
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} 
		while (time < 1);

		// always run on dedicated, only run if the app is active otherwise
#ifndef DEDICATED_ONLY
		if (app_active
			|| dedicated->value

			|| (!dedicated->value && (cls.state == ca_connected
				|| cls.state == ca_active)))
#else
		if (app_active
			|| (dedicated && dedicated->value))
#endif
		{
			Common_Frame(time);
		}


		oldtime = newtime;
	}

	// never gets here
	return TRUE;
}
