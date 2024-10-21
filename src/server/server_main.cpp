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

#include "server.hpp"

client_t* sv_client;			// current client

cvar_t* sv_tickrate;			// server tickrate

cvar_t* sv_paused;
cvar_t* sv_timedemo;

cvar_t* sv_enforcetime;

cvar_t* sv_msg_timeout;			// seconds without any message
cvar_t* sv_zombietime;			// seconds to sink messages after disconnect

cvar_t* rcon_password;			// password for remote server commands

cvar_t* allow_download;
cvar_t* allow_download_players;
cvar_t* allow_download_models;
cvar_t* allow_download_sounds;
cvar_t* allow_download_maps;

cvar_t* sv_stopspeed;
cvar_t* sv_maxspeed_player;
cvar_t* sv_maxspeed_director;
cvar_t* sv_duckspeed;
cvar_t* sv_accelerate_player;
cvar_t* sv_accelerate_director;
cvar_t* sv_airaccelerate;
cvar_t* sv_wateraccelerate;
cvar_t* sv_friction;
cvar_t* sv_waterfriction;
cvar_t* sv_waterspeed;

cvar_t* sv_noreload;			// don't reload level state when reentering

cvar_t* sv_maxclients;	
cvar_t* sv_showclamp;
#ifdef DEBUG
cvar_t* sv_debug_heartbeat;		// send heartbeat at a debug rate
#endif

cvar_t* hostname;
cvar_t* public_server;			// should heartbeats be sent?

cvar_t* sv_reconnect_limit;	// minimum seconds between connect messages

void Master_Shutdown();


//============================================================================


/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient(client_t* drop)
{
	// add the disconnect
	MSG_WriteByte(&drop->netchan.message, svc_disconnect);

	if (drop->state == cs_spawned)
	{
		// call the prog function for removing a client
		// this will remove the body, among other things
		game->Client_Disconnect(drop->edict);
	}

	if (drop->download)
	{
		FS_FreeFile(drop->download);
		drop->download = NULL;
	}

	drop->state = cs_zombie;		// become free in a few seconds
	drop->name[0] = 0;
}



/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/


/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge()
{
	int32_t 	i;
	int32_t 	oldest;
	int32_t 	oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (Net_CompareBaseAdr(net_from, svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7fff;
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = curtime;
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint(NS_SERVER, net_from, "challenge %i", svs.challenges[i].challenge);
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
void SVC_DirectConnect()
{
	char		userinfo[MAX_INFO_STRING];
	netadr_t	adr;
	int32_t 	i;
	client_t* cl, * newcl;
	client_t	temp;
	edict_t* ent;
	int32_t 	edictnum;
	int32_t 	version;
	int32_t 	qport;
	int32_t 	challenge;

	adr = net_from;

	Com_DPrintf("SVC_DirectConnect ()\n");

	version = atoi(Cmd_Argv(1));
	if (version != PROTOCOL_VERSION)
	{
		Netchan_OutOfBandPrint(NS_SERVER, adr, "print\nServer is game version %s.\n", ENGINE_VERSION);
		Com_DPrintf("    rejected connect from version %i\n", version);
		return;
	}

	qport = atoi(Cmd_Argv(2));

	challenge = atoi(Cmd_Argv(3));

	strncpy(userinfo, Cmd_Argv(4), sizeof(userinfo) - 1);
	userinfo[sizeof(userinfo) - 1] = 0;

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey(userinfo, (char*)"ip", Net_AdrToString(net_from)); // horrible

	// attractloop servers are ONLY for local clients
	if (sv.attractloop)
	{
		if (!Net_IsLocalAddress(adr))
		{
			Com_Printf("Remote connect in attract loop.  Ignored.\n");
			Netchan_OutOfBandPrint(NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}

	// see if the challenge is valid
	if (!Net_IsLocalAddress(adr))
	{
		for (i = 0; i < MAX_CHALLENGES; i++)
		{
			if (Net_CompareBaseAdr(net_from, svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
					break;		// good
				Netchan_OutOfBandPrint(NS_SERVER, adr, "print\nBad challenge.\n");
				return;
			}
		}
		if (i == MAX_CHALLENGES)
		{
			Netchan_OutOfBandPrint(NS_SERVER, adr, "print\nNo challenge for address.\n");
			return;
		}
	}

	newcl = &temp;
	memset(newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (Net_CompareBaseAdr(adr, cl->netchan.remote_address)
			&& (cl->netchan.qport == qport
				|| adr.port == cl->netchan.remote_address.port))
		{
			if (!Net_IsLocalAddress(adr) && (svs.realtime - cl->lastconnect) < ((int32_t)sv_reconnect_limit->value * 1000))
			{
				Com_DPrintf("%s:reconnect rejected : too soon\n", Net_AdrToString(adr));
				return;
			}
			Com_Printf("%s:reconnect\n", Net_AdrToString(adr));
			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot
	newcl = NULL;
	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
	{
		if (cl->state == cs_free)
		{
			newcl = cl;
			break;
		}
	}
	if (!newcl)
	{
		Netchan_OutOfBandPrint(NS_SERVER, adr, "print\nServer is full.\n");
		Com_DPrintf("Rejected a connection.\n");
		return;
	}

gotnewcl:
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	sv_client = newcl;
	edictnum = (newcl - svs.clients) + 1;
	ent = EDICT_NUM(edictnum);
	newcl->edict = ent;
	newcl->challenge = challenge; // save challenge for checksumming

	// get the game a chance to reject this connection or modify the userinfo
	if (!(game->Client_Connect(ent, userinfo)))
	{
		if (*Info_ValueForKey(userinfo, "rejmsg"))
			Netchan_OutOfBandPrint(NS_SERVER, adr, "print\n%s\nConnection refused.\n",
				Info_ValueForKey(userinfo, "rejmsg"));
		else
			Netchan_OutOfBandPrint(NS_SERVER, adr, "print\nConnection refused.\n");
		Com_DPrintf("Game rejected a connection.\n");
		return;
	}

	// parse some info from the info strings
	strncpy(newcl->userinfo, userinfo, sizeof(newcl->userinfo) - 1);
	SV_UserinfoChanged(newcl);

	// send the connect packet to the client
	Netchan_OutOfBandPrint(NS_SERVER, adr, "client_connect");

	Netchan_Setup(NS_SERVER, &newcl->netchan, adr, qport);

	newcl->state = cs_connected;

	SZ_Init(&newcl->datagram, newcl->datagram_buf, sizeof(newcl->datagram_buf));
	newcl->datagram.allowoverflow = true;
	newcl->lastmessage = svs.realtime;	// don't timeout
	newcl->lastconnect = svs.realtime;
}

int32_t Rcon_Validate()
{
	if (!strlen(rcon_password->string))
		return 0;

	if (strcmp(Cmd_Argv(1), rcon_password->string))
		return 0;

	return 1;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand()
{
	int32_t i;
	char	remaining[1024] = { 0 };

	i = Rcon_Validate();

	if (i == 0)
		Com_Printf("Bad rcon from %s:\n%s\n", Net_AdrToString(net_from), net_message.data + 4);
	else
		Com_Printf("Rcon from %s:\n%s\n", Net_AdrToString(net_from), net_message.data + 4);

	Com_BeginRedirect(RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!Rcon_Validate())
	{
		Com_Printf("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i = 2; i < Cmd_Argc(); i++)
		{
			strcat(remaining, Cmd_Argv(i));
			strcat(remaining, " ");
		}

		Cmd_ExecuteString(remaining);
	}

	Com_EndRedirect();
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket()
{
	char* s;
	char* c;

	MSG_BeginReading(&net_message);
	MSG_ReadInt(&net_message);		// skip the -1 marker

	s = MSG_ReadStringLine(&net_message);

	Cmd_TokenizeString(s, false);

	c = Cmd_Argv(0);
	Com_DPrintf("Packet %s : %s\n", Net_AdrToString(net_from), c);

	if (!strcmp(c, "ping"))
		Master_SvcPing();
	else if (!strcmp(c, "ack"))
		Master_SvcAck();
	else if (!strcmp(c, "status"))
		Master_SvcStatus();
	else if (!strcmp(c, "info"))
		Master_SvcInfo();
	else if (!strcmp(c, "getchallenge"))
		SVC_GetChallenge();
	else if (!strcmp(c, "connect"))
		SVC_DirectConnect();
	else if (!strcmp(c, "rcon"))
		SVC_RemoteCommand();
	else
		Com_Printf("bad connectionless packet from %s:\n%s\n"
			, Net_AdrToString(net_from), s);
}

void SV_MasterOnReadPost()
{

}


//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings()
{
	int32_t 	i, j;
	client_t* cl;
	int32_t 	total, count;

	for (i = 0; i < sv_maxclients->value; i++)
	{
		cl = &svs.clients[i];
		if (cl->state != cs_spawned)
			continue;

		total = 0;
		count = 0;
		for (j = 0; j < LATENCY_COUNTS; j++)
		{
			if (cl->frame_latency[j] > 0)
			{
				count++;
				total += cl->frame_latency[j];
			}
		}
		if (!count)
			cl->ping = 0;
		else

			cl->ping = total / count;

		// let the game dll know about the ping
		cl->edict->client->ping = cl->ping;
	}
}


/*
===================
SV_GiveMsec

Every few frames, gives all clients an allotment of milliseconds
for their command moves.  If they exceed it, assume cheating.
===================
*/
void SV_GiveMsec()
{
	int32_t i;
	client_t* cl;

	if (sv.framenum & 15)
		return;

	for (i = 0; i < sv_maxclients->value; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == cs_free)
			continue;

		cl->commandMsec = 1800;		// 1600 + some slop
	}
}


/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets()
{
	int32_t 		i;
	client_t* cl;
	int32_t 		qport;

	while (Net_GetPacket(NS_SERVER, &net_from, &net_message))
	{
		// check for connectionless packet (0xffffffff) first
		if (*(int32_t*)net_message.data == -1)
		{
			SV_ConnectionlessPacket();
			continue;
		}

		// read the qport out of the message so we can fix up
		// stupid address translating routers
		MSG_BeginReading(&net_message);
		MSG_ReadInt(&net_message);		// sequence number
		MSG_ReadInt(&net_message);		// sequence number
		qport = MSG_ReadShort(&net_message) & 0xffff;

		// check for packets from connected clients
		for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
		{
			if (cl->state == cs_free)
				continue;
			if (!Net_CompareBaseAdr(net_from, cl->netchan.remote_address))
				continue;
			if (cl->netchan.qport != qport)
				continue;
			if (cl->netchan.remote_address.port != net_from.port)
			{
				Com_Printf("SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remote_address.port = net_from.port;
			}

			if (Netchan_Process(&cl->netchan, &net_message))
			{	// this is a valid, sequenced packet, so process it
				if (cl->state != cs_zombie)
				{
					cl->lastmessage = svs.realtime;	// don't timeout
					SV_ExecuteClientMessage(cl);
				}
			}
			break;
		}

		if (i != sv_maxclients->value)
			continue;
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.  Server frames are used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts()
{
	int32_t 	i;
	client_t* cl;
	int32_t 		droppoint;
	int32_t 		zombiepoint;

	droppoint = svs.realtime - 1000 * sv_msg_timeout->value;
	zombiepoint = svs.realtime - 1000 * sv_zombietime->value;

	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->lastmessage > svs.realtime)
			cl->lastmessage = svs.realtime;

		if (cl->state == cs_zombie
			&& cl->lastmessage < zombiepoint)
		{
			cl->state = cs_free;	// can now be reused
			continue;
		}
		if ((cl->state == cs_connected || cl->state == cs_spawned)
			&& cl->lastmessage < droppoint)
		{
			SV_BroadcastPrintf(PRINT_HIGH, "%s timed out\n", cl->name);
			SV_DropClient(cl);
			cl->state = cs_free;	// don't bother with zombie state
		}
	}
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
void SV_PrepWorldFrame()
{
	edict_t* ent;
	int32_t 	i;

	for (i = 0; i < game->num_edicts; i++, ent++)
	{
		ent = EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}

}


/*
=================
SV_RunGameFrame
=================
*/
void SV_RunGameFrame()
{
	if (profile_all->value)
		time_before_game = Sys_Nanoseconds();

	// we always need to bump framenum, even if we
	// don't run the world, otherwise the delta
	// compression can get confused when a client
	// has the "current" frame
	sv.framenum++;
	sv.time = sv.framenum * (1000*(1/sv_tickrate->value));

	// don't run if paused
	if (!sv_paused->value || sv_maxclients->value > 1)
	{
		game->Game_RunFrame();

		// never get more than one tic behind
		if (sv.time < svs.realtime)
		{
			if (sv_showclamp->value)
				Com_Printf("sv highclamp\n");
			svs.realtime = sv.time;
		}
	}

	if (profile_all->value)
		time_after_game = Sys_Nanoseconds();

}

/*
==================
SV_Frame
==================
*/
void SV_Frame(int32_t msec)
{
	time_before_game = time_after_game = 0;

	// if server is not active, do nothing
	if (!svs.initialized)
		return;

	svs.realtime += msec;

	// keep the random time dependent
	rand();

	// check timeouts
	SV_CheckTimeouts();

	// get packets from clients
	SV_ReadPackets();

	// move autonomous things around if enough time has passed
	if (!sv_timedemo->value && svs.realtime < sv.time)
	{
		// never let the time get too far off
		if (sv.time - svs.realtime > 100)
		{
			if (sv_showclamp->value)
				Com_Printf("sv lowclamp\n");
			svs.realtime = sv.time - 100;
		}
		Net_Sleep(sv.time - svs.realtime);
		return;
	}

	// update ping based on the last known frame from all clients
	SV_CalcPings();

	// give the clients some timeslices
	SV_GiveMsec();

	// let everything in the world think and move
	SV_RunGameFrame();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages();

	// save the entire world state if recording a serverdemo
	SV_RecordDemoMessage();

	// send a heartbeat to the master if needed
	Netservices_MasterHeartbeatLegacy();

	// clear teleport flags, etc for next frame
	SV_PrepWorldFrame();

}

//============================================================================


//============================================================================


/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged(client_t* cl)
{
	const char* val;
	int32_t 	i;

	// call prog code to allow overrides
	game->Client_UserinfoChanged(cl->edict, cl->userinfo);

	// name for C code
	strncpy(cl->name, Info_ValueForKey(cl->userinfo, "name"), sizeof(cl->name) - 1);
	// mask off high bit
	for (i = 0; i < sizeof(cl->name); i++)
		cl->name[i] &= 127;

	// msg command
	val = Info_ValueForKey(cl->userinfo, "msg");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}

}


//============================================================================

/*
===============
SV_Init

Only called at zombono.exe startup, not for each game
===============
*/
void SV_Init()
{
	SV_InitOperatorCommands();

	rcon_password = Cvar_Get("rcon_password", "", 0);
	Cvar_Get("skill", "1", 0);
	Cvar_Get("gamemode", "0", CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_Get("gameflags", va("%i", GF_INSTANT_ITEMS), CVAR_SERVERINFO);
	Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	Cvar_Get("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_Get("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_NOSET);
	sv_tickrate = Cvar_Get("sv_tickrate", "40", CVAR_SERVERINFO | CVAR_LATCH);
	sv_maxclients = Cvar_Get("sv_maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH);
	hostname = Cvar_Get("hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
	sv_msg_timeout = Cvar_Get("sv_msg_timeout", "125", 0);
	sv_zombietime = Cvar_Get("sv_zombietime", "2", 0);
	sv_showclamp = Cvar_Get("sv_showclamp", "0", 0);
#ifdef DEBUG
	sv_debug_heartbeat = Cvar_Get("sv_debug_heartbeat", "0", 0);
#endif
	sv_paused = Cvar_Get("paused", "0", 0);
	sv_timedemo = Cvar_Get("timedemo", "0", 0);
	sv_enforcetime = Cvar_Get("sv_enforcetime", "0", 0);
	allow_download = Cvar_Get("allow_download", "1", CVAR_ARCHIVE);
	allow_download_players = Cvar_Get("allow_download_players", "0", CVAR_ARCHIVE);
	allow_download_models = Cvar_Get("allow_download_models", "1", CVAR_ARCHIVE);
	allow_download_sounds = Cvar_Get("allow_download_sounds", "1", CVAR_ARCHIVE);
	allow_download_maps = Cvar_Get("allow_download_maps", "1", CVAR_ARCHIVE);

	sv_noreload = Cvar_Get("sv_noreload", "0", 0);

	sv_stopspeed = Cvar_Get("sv_stopspeed", "100", CVAR_SERVERINFO);
	sv_maxspeed_player = Cvar_Get("sv_maxspeed_player", "300", CVAR_SERVERINFO);
	sv_maxspeed_director = Cvar_Get("sv_maxspeed_director", "300", CVAR_SERVERINFO);
	sv_duckspeed = Cvar_Get("sv_duckspeed", "100", CVAR_SERVERINFO);
	sv_accelerate_player = Cvar_Get("sv_accelerate_player", "10", CVAR_SERVERINFO);
	sv_accelerate_director = Cvar_Get("sv_accelerate_director", "8", CVAR_SERVERINFO);
	sv_airaccelerate = Cvar_Get("sv_airaccelerate", "10", CVAR_SERVERINFO);
	sv_wateraccelerate = Cvar_Get("sv_wateraccelerate", "10", CVAR_SERVERINFO);
	sv_friction = Cvar_Get("sv_friction", "6", CVAR_SERVERINFO);
	sv_waterfriction = Cvar_Get("sv_waterfriction", "1", CVAR_SERVERINFO);
	sv_waterspeed = Cvar_Get("sv_waterspeed", "400", CVAR_SERVERINFO);


	public_server = Cvar_Get("public", "0", 0);

	sv_reconnect_limit = Cvar_Get("sv_reconnect_limit", "3", CVAR_ARCHIVE);

	SZ_Init(&net_message, net_message_buffer, sizeof(net_message_buffer));
}

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage(const char* message, bool reconnect)
{
	int32_t		i;
	client_t*	cl;

	SZ_Clear(&net_message);
	MSG_WriteByte(&net_message, svc_print);
	MSG_WriteByte(&net_message, PRINT_HIGH);
	MSG_WriteString(&net_message, message);

	if (reconnect)
		MSG_WriteByte(&net_message, svc_reconnect);
	else
		MSG_WriteByte(&net_message, svc_disconnect);

	// send it twice
	// stagger the packets to crutch operating system limited buffers

	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
		if (cl->state >= cs_connected)
			Netchan_Transmit(&cl->netchan, net_message.cursize
				, net_message.data);

	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
		if (cl->state >= cs_connected)
			Netchan_Transmit(&cl->netchan, net_message.cursize
				, net_message.data);
}



/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown(const char* finalmsg, bool reconnect)
{
	if (svs.clients)
		SV_FinalMessage(finalmsg, reconnect);

	Master_Shutdown();
	// calling this function here causes function stack to be corrupted on 64 bit builds when invoked from Com_Error()
	//SV_ShutdownGameProgs ();

	// free current level
	if (sv.demofile)
		fclose(sv.demofile);
	memset(&sv, 0, sizeof(sv));
	Com_SetServerState(sv.state);

	// free server static data
	if (svs.clients)
		Memory_ZoneFree(svs.clients);
	if (svs.client_entities)
		Memory_ZoneFree(svs.client_entities);
	if (svs.demofile)
		fclose(svs.demofile);
	memset(&svs, 0, sizeof(svs));
}

