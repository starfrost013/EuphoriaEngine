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
// client_main.c  -- client main loop, init, shutdown

#include <client/client.hpp>
#include <../game/ui/game_ui.hpp>

client_static_t	cls;
client_state_t	cl;
game_ui_api_t*	gui;		// Game UI dll interface object

centity_t		cl_entities[MAX_EDICTS];

entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

bool app_active;
bool app_minimized;

/*
================
CL_Drop

Called after an ERR_DROP was thrown
================
*/
void CL_Drop()
{
	if (cls.state == ca_uninitialized)
		return;
	if (cls.state == ca_disconnected)
		return;

	CL_Disconnect();

	// drop loading plaque unless this is the initial game start
	if (cls.disable_servercount != -1)
		Render2D_EndLoadingPlaque();	// get rid of loading plaque
}


/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and
connect.
======================
*/
void CL_SendConnectPacket()
{
	netadr_t	adr;
	int32_t 	port;

	if (!Net_StringToAdr(cls.servername, &adr))
	{
		Com_Printf("Bad server address\n");
		cls.connect_time = 0;
		return;
	}
	if (adr.port == 0)
		adr.port = BigShort(PORT_SERVER);

	port = Cvar_VariableValue("qport");
	userinfo_modified = false;

	Netchan_OutOfBandPrint(NS_CLIENT, adr, "connect %i %i %i \"%s\"\n",
		PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo());
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend()
{
	netadr_t	adr;

	// if the local server is running and we aren't
	// then connect
	if (cls.state == ca_disconnected && Com_GetServerState())
	{
		cls.state = ca_connecting;
		strncpy(cls.servername, "localhost", sizeof(cls.servername) - 1);
		// we don't need a challenge on the localhost
		CL_SendConnectPacket();
		return;
		//		cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
	}

	// resend if we haven't gotten a reply yet
	if (cls.state != ca_connecting)
		return;

	if (cls.realtime - cls.connect_time < 3000)
		return;

	if (!Net_StringToAdr(cls.servername, &adr))
	{
		Com_Printf("Bad server address\n");
		cls.state = ca_disconnected;
		return;
	}
	if (adr.port == 0)
		adr.port = BigShort(PORT_SERVER);

	cls.connect_time = cls.realtime;	// for retransmit requests

	Com_Printf("Connecting to %s...\n", cls.servername);

	Netchan_OutOfBandPrint(NS_CLIENT, adr, "getchallenge\n");
}


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState()
{
	S_StopAllSounds();
	CL_ClearEffects();
	CL_ClearTEnts();

	// wipe the entire cl structure
	memset(&cl, 0, sizeof(cl));
	memset(&cl_entities, 0, sizeof(cl_entities));

	// disable any active UIs
	UI_Reset();

	SZ_Clear(&cls.netchan.message);
}

/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Com_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect()
{
	uint8_t	final[32];

	if (cls.state == ca_disconnected)
		return;

	if (cl_timedemo && cl_timedemo->value)
	{
		int32_t time;

		time = Sys_Milliseconds() - cl.timedemo_start;
		if (time > 0)
			Com_Printf("%i frames, %3.1f seconds: %3.1f fps\n", cl.timedemo_frames,
				time / 1000.0, cl.timedemo_frames * 1000.0 / time);
	}

	VectorClear3(cl.refdef.blend);

	UI_Reset();

	cls.connect_time = 0;

	if (cls.demorecording)
		CL_Stop_f();

	// send a disconnect message to the server
	final[0] = clc_stringcmd;
	strcpy((char*)final + 1, "disconnect");
	Netchan_Transmit(&cls.netchan, (int32_t)strlen((const char*)final), final);
	Netchan_Transmit(&cls.netchan, (int32_t)strlen((const char*)final), final);
	Netchan_Transmit(&cls.netchan, (int32_t)strlen((const char*)final), final);

	CL_ClearState();

	// stop download
	if (cls.download)
	{
		fclose(cls.download);
		cls.download = NULL;
	}

	cls.state = ca_disconnected;

	UI_SetEnabled("MainMenuUI", true);
	UI_SetActivated("MainMenuUI", true);
}

/*
=================
CL_ParseStatusMessage

Handle a reply from a ping
=================
*/
void CL_ParseStatusMessage()
{
	char* s;

	s = MSG_ReadString(&net_message);

	Com_Printf("%s\n", s);
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket()
{
	char* s;
	char* c;

	MSG_BeginReading(&net_message);
	MSG_ReadInt(&net_message);	// skip the -1

	s = MSG_ReadStringLine(&net_message);

	Cmd_TokenizeString(s, false);

	c = Cmd_Argv(0);

	Com_Printf("%s: %s\n", Net_AdrToString(net_from), c);

	// server connection
	if (!strcmp(c, "client_connect"))
	{
		if (cls.state == ca_connected)
		{
			Com_Printf("Dup connect received.  Ignored.\n");
			return;
		}
		Netchan_Setup(NS_CLIENT, &cls.netchan, net_from, cls.netchan_port);
		MSG_WriteChar(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message, "new");
		cls.state = ca_connected;
		return;
	}

	// server responding to a status broadcast
	if (!strcmp(c, "info"))
	{
		CL_ParseStatusMessage();
		return;
	}

	// remote command from gui front end
	if (!strcmp(c, "cmd"))
	{
		if (!Net_IsLocalAddress(net_from))
		{
			Com_Printf("Command packet from remote host.  Ignored.\n");
			return;
		}
		s = MSG_ReadString(&net_message);
		Cbuf_AddText(s);
		Cbuf_AddText("\n");
		return;
	}
	// print command from somewhere
	if (!strcmp(c, "print"))
	{
		s = MSG_ReadString(&net_message);
		Com_Printf("%s", s);
		return;
	}

	// ping from somewhere
	if (!strcmp(c, "ping"))
	{
		Netchan_OutOfBandPrint(NS_CLIENT, net_from, "ack");
		return;
	}

	// challenge from the server we are connecting to
	if (!strcmp(c, "challenge"))
	{
		cls.challenge = atoi(Cmd_Argv(1));
		CL_SendConnectPacket();
		return;
	}

	// echo request from server
	if (!strcmp(c, "echo"))
	{
		Netchan_OutOfBandPrint(NS_CLIENT, net_from, "%s", Cmd_Argv(1));
		return;
	}

	Com_Printf("Unknown command.\n");
}


/*
=================
CL_DumpPackets

A vain attempt to help bad TCP stacks that cause problems
when they overflow
=================
*/
void CL_DumpPackets()
{
	while (Net_GetPacket(NS_CLIENT, &net_from, &net_message))
	{
		Com_Printf("dumnping a packet\n");
	}
}

/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets()
{
	while (Net_GetPacket(NS_CLIENT, &net_from, &net_message))
	{
		//	Com_Printf ("packet\n");
				//
				// remote command packet
				//
		if (*(int32_t*)net_message.data == -1)
		{
			CL_ConnectionlessPacket();
			continue;
		}

		if (cls.state == ca_disconnected || cls.state == ca_connecting)
			continue;		// dump it if not connected

		if (net_message.cursize < 8)
		{
			Com_Printf("%s: Runt packet\n", Net_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!Net_CompareAdr(net_from, cls.netchan.remote_address))
		{
			// WHY IS THIS EVEN BEING TRIGGERED
			//Com_DPrintf ("%s:sequenced packet without connection\n"
			//	,NET_AdrToString(net_from));
			continue;
		}
		if (!Netchan_Process(&cls.netchan, &net_message))
			continue;		// wasn't accepted for some reason
		CL_ParseServerMessage();
	}

	//
	// check timeout
	//
	if (cls.state >= ca_connected
		&& cls.realtime - cls.netchan.last_received > cl_timeout->value * 1000)
	{
		if (++cl.timeoutcount > 5)	// timeoutcount saves debugger
		{
			Com_Printf("\nServer connection timed out.\n");
			CL_Disconnect();
			return;
		}
	}
	else
		cl.timeoutcount = 0;

}


//=============================================================================



/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal()
{
	cls.state = ca_disconnected;
	cls.realtime = Sys_Milliseconds();

	CL_InitInput();

	CL_InitCvars();
	CL_InitCommands();
}

/*
===============
CL_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void CL_WriteConfiguration()
{
	FILE* f;
	char	path[MAX_QPATH];

	if (cls.state == ca_uninitialized)
		return;

#ifdef PLAYTEST
	snprintf(path, sizeof(path), "%s/config_playtest.cfg", FS_Gamedir());
#else
	snprintf(path, sizeof(path), "%s/config.cfg", FS_Gamedir());
#endif

	f = fopen(path, "w");

	if (!f)
	{
		Com_Printf("Couldn't write config.cfg.\n");
		return;
	}

	fprintf(f, "// generated by zombono, do not modify\n");
	Key_WriteBindings(f);
	fclose(f);

	Cvar_WriteVariables(path);
}


/*
==================
CL_FixCvarCheats

==================
*/

typedef struct cheatvar_s
{
	const char* name;
	const char* value;
	cvar_t* var;
} cheatvar_t;

cheatvar_t	cheatvars[] = {
#ifdef NDEBUG
	{"timescale", "1"},
	{"timedemo", "0"},
	{"r_drawworld", "1"},
	{"cl_testlights", "0"},
	{"r_fullbright", "0"},
	{"r_drawflat", "0"},
	{"paused", "0"},
	{"fixedtime", "0"},
	//{"gl_lightmap", "0"},
#endif
	{NULL, NULL}
};

int32_t  numcheatvars;

void CL_FixCvarCheats()
{
	int32_t 		i;
	cheatvar_t* var;

	if (!strcmp(cl.configstrings[CS_MAXCLIENTS], "1")
		|| !cl.configstrings[CS_MAXCLIENTS][0])
		return;		// single player can cheat

	// find all the cvars if we haven't done it yet
	if (!numcheatvars)
	{
		while (cheatvars[numcheatvars].name)
		{
			cheatvars[numcheatvars].var = Cvar_Get(cheatvars[numcheatvars].name,
				cheatvars[numcheatvars].value, 0);
			numcheatvars++;
		}
	}

	// make sure they are all set to the proper values
	for (i = 0, var = cheatvars; i < numcheatvars; i++, var++)
	{
		if (strcmp(var->var->string, var->value))
		{
			Cvar_Set(var->name, var->value);
		}
	}
}

//============================================================================

/*
==================
CL_SendCommand

==================
*/
void CL_SendCommand()
{
	// Kill input when a UI that captures input and is (slight hack) NOT the leaderboard is active.
	// sys_frame_time being frozen effectively breaks input

	if (cls.state == ca_active
		&& ui_active 
		&& !(ui_active 
			&& current_ui != NULL 
			&& current_ui->passive))
	{
		// disable movement
		return;
	}

	sys_frame_time = Sys_Milliseconds();

	// process console commands
	Cbuf_Execute();

	// fix any cheating cvars
	CL_FixCvarCheats();

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();
}


/*
==================
CL_Frame
==================
*/
void CL_Frame(int32_t msec)
{
	static int32_t extratime;
	static int64_t lasttimecalled;

	if (dedicated->value)
		return;

	extratime += msec;

	if (!cl_timedemo->value)
	{
		if (cls.state == ca_connected && extratime < 100)
			return;			// don't flood packets out while connecting
		if (extratime < 1000 / cl_maxfps->value)
			return;			// framerate is too high
	}


	// let the mouse activate or deactivate
	if (!cls.disable_input)
		Input_Frame();

	// decide the simulation time
	cls.frametime = extratime / 1000.0f;
	cl.time += extratime;
	cls.realtime = curtime;

	extratime = 0;

	if (cls.frametime > (1.0 / 5))
		cls.frametime = (1.0 / 5);

	// if in the debugger last frame, don't timeout
	if (msec > 5000)
		cls.netchan.last_received = Sys_Milliseconds();

	// fetch results from server
	CL_ReadPackets();

	// send a new command message to the server
	CL_SendCommand();

	// predict all unacknowledged movements
	CL_PredictMovement();

	// allow rendering DLL change
	Vid_CheckChanges();
	if (!cl.refresh_prepped && cls.state == ca_active)
		Render3D_PrepRefresh();

	// update the screen
	if (profile_all->value)
		time_before_ref = Sys_Nanoseconds();
	Render_UpdateScreen();
	if (profile_all->value)
		time_after_ref = Sys_Nanoseconds();

	// update audio
	S_Update(cl.refdef.vieworigin, cl.v_forward, cl.v_right, cl.v_up);

	Miniaudio_Update();

	// update the loadout information
	Loadout_Update();

	// advance local effects for next frame
	CL_RunDLights();
	CL_RunLightStyles();

	Render2D_RunConsole();

	cls.framecount++;

	if (cls.state == ca_active)
	{
		if (!lasttimecalled)
		{
			lasttimecalled = Sys_Nanoseconds();
			if (log_stats->value
				&& log_stats_file)
			{
				fprintf(log_stats_file, "0\n");
			}
		}
		else
		{
			int64_t now = Sys_Nanoseconds();
			float frametime_ms = (float)(now - lasttimecalled) / 1000000.0f;

			if (log_stats->value
				&& log_stats_file)
			{
				fprintf(log_stats_file, "%.2f\n", frametime_ms);
			}

			lasttimecalled = now;

			// calculate the fps
			cls.fps = 1000.0f / frametime_ms;
		}
	}
}


void App_Activate(bool active, bool minimize)
{
	app_minimized = minimize;

	Key_ClearStates();

	// we don't want to act like we're active if we're minimized
	if (active && !app_minimized)
		app_active = true;
	else
		app_active = false;

	// minimize/restore mouse-capture on demand
	if (!app_active)
	{
		Input_Activate(false);
		S_Activate(false);
	}
	else
	{
		// ZombonoUI controls this in the case a UI is active
		if (!ui_active
			|| ui_active && current_ui->passive) Input_Activate(true);
	}
}


//============================================================================
#ifndef DEDICATED_ONLY
// Initialises the gameui library
void CL_InitGameLibraries()
{
	Com_Printf("------- Loading GameUI -------\n");

	engine_ui_api_t* engine_api = UI_InitInterface();

	gui = (game_ui_api_t*)Sys_LoadGameUILibrary(engine_api);

	if (gui->version != GAME_UI_INTERFACE_VERSION)
		Sys_Error("Incorrect Game UI interface version %d (expected %d)...", gui->version, GAME_UI_INTERFACE_VERSION);
}


void CL_ShutdownGameLibraries()
{
	Com_Printf("------- Shutting down GameUI -------\n");

	Sys_UnloadGameUILibrary();
	gui = NULL;
}
#endif


/*
====================
CL_Init
====================
*/
void CL_Init()
{
	if (dedicated->value)
		return;		// nothing running on the client

	// all archived variables will now be loaded
	Con_Init();

#ifndef DEDICATED_ONLY
	// Initialise game client dll (game_ui)
	CL_InitGameLibraries();
#endif

#if defined __linux__
	S_Init();
	Vid_Init();
#else
	Vid_Init();
	S_Init();	// sound must be initialized after window is created
#endif

	Render3D_Init();

	net_message.data = net_message_buffer;
	net_message.maxsize = sizeof(net_message_buffer);

	Render2D_Init();
	cls.disable_screen = true;	// don't draw yet

	Miniaudio_Init();
	CL_InitLocal();
	Input_Init();
	if (!Font_Init()) Sys_Error("Error initialising font engine");
	if (!UI_Init()) Sys_Error("Error initialising the user interface system");
	Loadout_Init();

	FS_ExecAutoexec();
	Cbuf_Execute();

#ifdef PLAYTEST
	// HACK
	cls.disable_screen = 0;
	Cbuf_AddText("exec playtest_server.cfg");
#endif

	// now the init is done...
	if (!gui->GameUI_Create())
		Sys_Error("Error during initialisation: Failed to create game UI");

	// let's go!
	App_Activate(true, false);

	if (cl_showintro->value)
		Intro_Start();

	// put up the main menu

	UI_SetEnabled("MainMenuUI", true);
	UI_SetActivated("MainMenuUI", true);
}

/*
===============
CL_Shutdown

FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CL_Shutdown()
{
	static bool isdown = false;

	if (isdown)
	{
		printf("recursive shutdown\n");
		return;
	}
	isdown = true;

	CL_ShutdownGameLibraries();
	CL_WriteConfiguration();
	Miniaudio_Shutdown();
	S_Shutdown();
	Input_Shutdown();
	Vid_Shutdown();
}
