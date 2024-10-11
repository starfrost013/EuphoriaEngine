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

// sv_master.c - Master server 

#include "server.hpp"

netadr_t	master_adr[MAX_MASTERS];	// address of group servers

const char* master_base = "servers.zombono.com";
const char* master_alternative = "servers-alt.zombono.com"; // temp

// The master protocol version
// The regular protocol version is still used, but I want older versions of the game to still be supported to some extent (hidden by default)
#define	MASTER_PROTOCOL_VERSION		1
/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS		300
#define HEARTBEAT_SECONDS_DEBUG	2		// Used on debug builds when sv_debug_heartbeat is used

// move these to header if needed


/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies
===============
*/
char* SV_StatusString()
{
	char		player[1024];
	static char	status[MAX_MSGLEN - 16];
	int32_t 	i;
	client_t*	cl;
	int32_t 	status_length;
	int32_t 	player_length;
	int32_t		num_clients = 0;

	// get the number of clients
	for (i = 0; i < sv_maxclients->value; i++)
	{
		cl = &svs.clients[i];

		if (cl->state == cs_connected || cl->state == cs_spawned)
			num_clients++;
	}

	snprintf(status, (MAX_MSGLEN - 16), "\\%d", num_clients);
	status_length = (int32_t)strlen(status);
	strncpy(status + status_length, Cvar_Serverinfo(), (MAX_MSGLEN - 16 - status_length));
	status_length = (int32_t)strlen(status);
	strncat(status + status_length, "\n", (MAX_MSGLEN - 16 - status_length));

	status_length = (int32_t)strlen(status);

	for (i = 0; i < sv_maxclients->value; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == cs_connected || cl->state == cs_spawned)
		{
			snprintf(player, sizeof(player), "%i %i \"%s\"\n",
				cl->edict->client->ps.stats[STAT_FRAGS], cl->ping, cl->name);
			player_length = (int32_t)strlen(player);
			if (status_length + player_length >= sizeof(status))
				break;		// can't hold any more
			strcpy(status + status_length, player);
			status_length += player_length;
		}
	}

	return status;
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
void Master_SvcStatus()
{
	Netchan_OutOfBandPrint(NS_SERVER, net_from, "print\n%s", SV_StatusString());
}

/*
================
SVC_Ack

================
*/
void Master_SvcAck()
{
	Com_Printf("Ping acknowledge from %s\n", Net_AdrToString(net_from));
}

/*
================
SVC_Info

Responds with int16_t info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
void Master_SvcInfo()
{
	char	string[64];
	int32_t i, count;
	int32_t version;

	if (sv_maxclients->value == 1)
		return;		// ignore in single player

	version = atoi(Cmd_Argv(1));

	if (version != PROTOCOL_VERSION)
		snprintf(string, sizeof(string), "%s: wrong version\n", hostname->string, sizeof(string));
	else
	{
		count = 0;
		for (i = 0; i < sv_maxclients->value; i++)
			if (svs.clients[i].state >= cs_connected)
				count++;

		snprintf(string, sizeof(string), "%16s %8s %2i/%2i\n", hostname->string, sv.name, count, (int32_t)sv_maxclients->value);
	}

	Netchan_OutOfBandPrint(NS_SERVER, net_from, "info\n%s", string);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void Master_SvcPing()
{
	Netchan_OutOfBandPrint(NS_SERVER, net_from, "ack");
}

void Netservices_MasterHeartbeatLegacy()
{
	char* string;
	int32_t i;

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!dedicated || !dedicated->value)
		return;		// only dedicated servers send heartbeats

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!public_server || !public_server->value)
		return;		// a private dedicated game

	// check for time wraparound
	if (svs.last_heartbeat > svs.realtime)
		svs.last_heartbeat = svs.realtime;

#if DEBUG
	if (sv_debug_heartbeat->value)
	{
		if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS_DEBUG * 1000)
			return;		// not time to send yet
	}
	else
	{
		if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS * 1000)
			return;		// not time to send yet
	}
#elif PLAYTEST
	if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS * 1000)
		return;		// not time to send yet
#endif

	svs.last_heartbeat = svs.realtime;

	// send the same string that we would give for a status OOB command
	string = SV_StatusString();

	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
	{
		if (master_adr[i].port)
		{
			Com_Printf("Sending heartbeat to %s\n", Net_AdrToString(master_adr[i]));
			Netchan_OutOfBandPrint(NS_SERVER, master_adr[i], "heartbeat\n%s", string);
		}
	}

}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown()
{
	int32_t i;

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!dedicated || !dedicated->value)
		return;		// only dedicated servers send heartbeats

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!public_server || !public_server->value)
		return;		// a private dedicated game

	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
	{
		if (master_adr[i].port)
		{
			if (i > 0)
				Com_Printf("Sending heartbeat to %s\n", Net_AdrToString(master_adr[i]));

			Netchan_OutOfBandPrint(NS_SERVER, master_adr[i], "shutdown");
		}
	}

}

