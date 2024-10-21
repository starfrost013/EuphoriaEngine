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
// client_commands.c -- client commands

#include <client/client.hpp>

extern void SV_ShutdownGameLibraries();


/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer()
{
	char* cmd;

	cmd = Cmd_Argv(0);
	if (cls.state <= ca_connected || *cmd == '-' || *cmd == '+')
	{
		Com_Printf("Unknown command \"%s\"\n", cmd);
		return;
	}

	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
	SZ_Print(&cls.netchan.message, cmd);

	if (Cmd_Argc() > 1)
	{
		SZ_Print(&cls.netchan.message, " ");
		SZ_Print(&cls.netchan.message, Cmd_Args());
	}
}

void CL_Setenv_f(void)
{
	int32_t argc = Cmd_Argc();

	if (argc > 2)
	{
		char buffer[1000];
		int32_t i;

		strcpy(buffer, Cmd_Argv(1));
		strcat(buffer, "=");

		for (i = 2; i < argc; i++)
		{
			strcat(buffer, Cmd_Argv(i));
			strcat(buffer, " ");
		}

		putenv(buffer);
	}
	else if (argc == 2)
	{
		char* env = getenv(Cmd_Argv(1));

		if (env)
		{
			Com_Printf("%s=%s\n", Cmd_Argv(1), env);
		}
		else
		{
			Com_Printf("%s undefined\n", Cmd_Argv(1), env);
		}
	}
}


/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f()
{
	if (cls.state != ca_connected && cls.state != ca_active)
	{
		Com_Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	// don't forward the first argument
	if (Cmd_Argc() > 1)
	{
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		SZ_Print(&cls.netchan.message, Cmd_Args());
	}
}


/*
==================
CL_Pause_f
==================
*/
void CL_Pause_f()
{
	// never pause in multiplayer
	if (Cvar_VariableValue("sv_maxclients") > 1 || !Com_GetServerState())
	{
		Cvar_SetValue("paused", 0);
		return;
	}

	Cvar_SetValue("paused", !cl_paused->value);
}

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f()
{
	CL_Disconnect();
	Com_Quit();
}


/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f()
{
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	Render2D_BeginLoadingPlaque();
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Com_Printf("\nChanging map...\n");
}

/*
=================
CL_PingServers_f
=================
*/
void CL_PingServers_f()
{
	int32_t 	i;
	netadr_t	adr;
	char		name[PLAYER_NAME_LENGTH];
	const char*	adrstring;
	cvar_t*		noudp;

	Net_Config(true);		// allow remote

	// send a broadcast packet
	Com_Printf("pinging broadcast...\n");

	noudp = Cvar_Get("noudp", "0", CVAR_NOSET);
	if (!noudp->value)
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint(NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}
	// send a packet to each address book entry
	for (i = 0; i < 16; i++)
	{
		snprintf(name, sizeof(name), "adr%i", i);
		adrstring = Cvar_VariableString(name);
		if (!adrstring || !adrstring[0])
			continue;

		Com_Printf("pinging %s...\n", adrstring);
		if (!Net_StringToAdr(adrstring, &adr))
		{
			Com_Printf("Bad address: %s\n", adrstring);
			continue;
		}
		if (!adr.port)
			adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint(NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}
}


/*
=================
CL_Skins_f

Load or download any custom player skins and models
=================
*/
void CL_Skins_f()
{
	int32_t 	i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS + i][0])
			continue;
		Com_Printf("client %i: %s\n", i, cl.configstrings[CS_PLAYERSKINS + i]);
		Render_UpdateScreen();
		CL_ParseClientinfo(i);
	}
}


/*
==============
CL_FixUpGender_f
==============
*/
void CL_FixUpGender()
{
	char* p;
	char sk[80];

	if (gender_auto->value)
	{
		if (gender->modified)
		{
			// was set directly, so don't override the user
			gender->modified = false;
			return;
		}

		strncpy(sk, skin->string, sizeof(sk) - 1);
		if ((p = strchr(sk, '/')) != NULL)
			*p = 0;
		if (Q_stricmp(sk, "male") == 0)
			Cvar_Set("gender", "male");
		else if (Q_stricmp(sk, "female") == 0)
			Cvar_Set("gender", "female");
		else if (Q_stricmp(sk, "other") == 0)
			Cvar_Set("gender", "other");
		else
			Cvar_Set("gender", "none");

		gender->modified = false;
	}
}

/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f()
{
	Com_Printf("User info settings:\n");
	Info_Print(Cvar_Userinfo());
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem so it can pick up
new parameters and flush all sounds
=================
*/
void CL_Snd_Restart_f()
{
	S_Shutdown();
	S_Init();
	CL_RegisterSounds();
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f()
{
	char* server;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: connect <server>\n");
		return;
	}

	if (Com_GetServerState())
	{	// if running a local server, kill it and reissue
		SV_Shutdown(va("Server quit\n", msg), false);
		SV_ShutdownGameLibraries();
	}
	else
	{
		CL_Disconnect();
	}

	server = Cmd_Argv(1);

	Net_Config(true);		// allow remote

	CL_Disconnect();

	cls.state = ca_connecting;
	strncpy(cls.servername, server, sizeof(cls.servername) - 1);
	cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f()
{
	char		message[1024];
	int32_t 	i;
	netadr_t	to;

	if (!rcon_client_password->string)
	{
		Com_Printf("You must set 'rcon_password' before\n"
			"issuing an rcon command.\n");
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	Net_Config(true);		// allow remote

	strcat(message, "rcon ");

	strcat(message, rcon_client_password->string);
	strcat(message, " ");

	for (i = 1; i < Cmd_Argc(); i++)
	{
		strcat(message, Cmd_Argv(i));
		strcat(message, " ");
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address->string))
		{
			Com_Printf("You must either be connected,\n"
				"or set the 'rcon_address' cvar\n"
				"to issue rcon commands\n");

			return;
		}
		Net_StringToAdr(rcon_address->string, &to);
		if (to.port == 0)
			to.port = BigShort(PORT_SERVER);
	}

	Net_SendPacket(NS_CLIENT, (int32_t)strlen(message) + 1, message, to);
}

/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f()
{
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	S_StopAllSounds();

	if (cls.state == ca_connected)
	{
		Com_Printf("reconnecting...\n");
		cls.state = ca_connected;
		MSG_WriteChar(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message, "new");
		return;
	}

	if (*cls.servername)
	{
		if (cls.state >= ca_connected)
		{
			CL_Disconnect();
			cls.connect_time = (float)cls.realtime - 1500.0f;
		}
		else
			cls.connect_time = -99999; // fire immediately

		cls.state = ca_connecting;
		Com_Printf("reconnecting...\n");
	}
}

// ==============
// CL_Disconnect_f();
// 
// Disconnects from the server...this isn't great
// ===============
void CL_Disconnect_f()
{
	Com_Error(ERR_DROP, "Disconnected from server");
}


void CL_InitCommands()
{
	//
	// register our commands
	//
	Cmd_AddCommand("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand("pause", CL_Pause_f);
	Cmd_AddCommand("pingservers", CL_PingServers_f);
	Cmd_AddCommand("skins", CL_Skins_f);

	Cmd_AddCommand("userinfo", CL_Userinfo_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);

	Cmd_AddCommand("changing", CL_Changing_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("stop", CL_Stop_f);

	Cmd_AddCommand("quit", CL_Quit_f);

	Cmd_AddCommand("connect", CL_Connect_f);
	Cmd_AddCommand("reconnect", CL_Reconnect_f);

	Cmd_AddCommand("rcon", CL_Rcon_f);

	Cmd_AddCommand("setenv", CL_Setenv_f);

	Cmd_AddCommand("precache", CL_Precache_f);

	Cmd_AddCommand("download", CL_Download_f);

	//
	// forward to server commands
	//
	// the only thing this does is allow command completion
	// to work -- all unknown commands are automatically
	// forwarded to the server
	Cmd_AddCommand("wave", NULL);
	Cmd_AddCommand("kill", NULL);
	Cmd_AddCommand("use", NULL);
	Cmd_AddCommand("drop", NULL);
	Cmd_AddCommand("say", NULL);
	Cmd_AddCommand("say_team", NULL);
	Cmd_AddCommand("info", NULL);
	Cmd_AddCommand("prog", NULL);
	Cmd_AddCommand("give", NULL);
	Cmd_AddCommand("god", NULL);
	Cmd_AddCommand("notarget", NULL);
	Cmd_AddCommand("loadout", NULL);
	Cmd_AddCommand("noclip", NULL);
	Cmd_AddCommand("invuse", NULL);
	Cmd_AddCommand("invprev", NULL);
	Cmd_AddCommand("invnext", NULL);
	Cmd_AddCommand("invdrop", NULL);
	Cmd_AddCommand("weapnext", NULL);
	Cmd_AddCommand("weapprev", NULL);
	Cmd_AddCommand("weaplast", NULL);
}