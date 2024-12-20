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
// net_wins.c

#include "winsock.h"
#include <common/common.hpp>

#define	MAX_LOOPBACK 4

typedef struct loopmsg_s
{
	uint8_t	data[MAX_MSGLEN];
	int32_t datalen;
} loopmsg_t;

typedef struct loopback_s
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int32_t 	get, send;
} loopback_t;

cvar_t* net_shownet;
static cvar_t* noudp;

loopback_t	loopbacks[2];
int32_t 	ip_sockets[2];

const char* Net_ErrorString();

//=============================================================================

void NetadrToSockadr(netadr_t* a, struct sockaddr* s)
{
	memset(s, 0, sizeof(*s));

	if (a->type == NA_BROADCAST)
	{
		((struct sockaddr_in*)s)->sin_family = AF_INET;
		((struct sockaddr_in*)s)->sin_port = a->port;
		((struct sockaddr_in*)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if (a->type == NA_IP)
	{
		((struct sockaddr_in*)s)->sin_family = AF_INET;
		((struct sockaddr_in*)s)->sin_addr.s_addr = *(int*)&a->ip;
		((struct sockaddr_in*)s)->sin_port = a->port;
	}
}

void SockadrToNetadr(struct sockaddr* s, netadr_t* a)
{
	if (s->sa_family == AF_INET)
	{
		a->type = NA_IP;
		*(int32_t*)&a->ip = ((struct sockaddr_in*)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in*)s)->sin_port;
	}
}


bool Net_CompareAdr(netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return TRUE;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
			return true;
		return false;
	}

	return TRUE;
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
bool Net_CompareBaseAdr(netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return TRUE;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return true;
		return false;
	}

	return TRUE;
}

char* Net_AdrToString(netadr_t a)
{
	static char s[64];

	if (a.type == NA_LOOPBACK)
		snprintf(s, sizeof(s), "loopback");
	else if (a.type == NA_IP)
		snprintf(s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
	return s;
}


/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/

bool Net_StringToSockaddr(const char* s, struct sockaddr* sadr)
{
	struct hostent* h;
	char* colon;
	char	copy[128];

	memset(sadr, 0, sizeof(*sadr));

	((struct sockaddr_in*)sadr)->sin_family = AF_INET;

	((struct sockaddr_in*)sadr)->sin_port = 0;

	strcpy(copy, s);

	// strip off a trailing :port if present
	for (colon = copy; *colon; colon++)
	{
		if (*colon == ':')
		{
			*colon = 0;
			((struct sockaddr_in*)sadr)->sin_port = htons((int16_t)atoi(colon + 1));
		}
	}


	if (copy[0] >= '0' && copy[0] <= '9')
	{
		*(int*)&((struct sockaddr_in*)sadr)->sin_addr = inet_addr(copy);
	}
	else
	{
		if (!(h = gethostbyname(copy)))
			return 0;
		*(int*)&((struct sockaddr_in*)sadr)->sin_addr = *(int*)h->h_addr_list[0];
	}

	return true;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
bool Net_StringToAdr(const char* s, netadr_t* a)
{
	struct sockaddr sadr;

	if (!strcmp(s, "localhost"))
	{
		memset(a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	if (!Net_StringToSockaddr(s, &sadr))
		return false;

	SockadrToNetadr(&sadr, a);

	return true;
}


bool Net_IsLocalAddress(netadr_t adr)
{
	return adr.type == NA_LOOPBACK;
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

bool Net_GetLoopPacket(netsrc_t sock, netadr_t* net_from, sizebuf_t* net_message)
{
	int32_t 	i;
	loopback_t* loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK - 1);
	loop->get++;

	memcpy(net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	memset(net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	return true;

}


void Net_SendLoopPacket(netsrc_t sock, int32_t length, void* data, netadr_t to)
{
	int32_t 	i;
	loopback_t* loop;

	loop = &loopbacks[sock ^ 1];

	i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

	memcpy(loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================

bool Net_GetPacket(netsrc_t sock, netadr_t* net_from, sizebuf_t* net_message)
{
	int32_t 	ret;
	struct sockaddr from;
	int32_t 	fromlen;
	int32_t 	net_socket;
	int32_t 	err;

	if (Net_GetLoopPacket(sock, net_from, net_message))
		return true;

	net_socket = ip_sockets[sock];

	if (!net_socket)
		return false;

	fromlen = sizeof(from);
	ret = recvfrom(net_socket, (char*)net_message->data, net_message->maxsize
		, 0, (struct sockaddr*)&from, &fromlen);

	SockadrToNetadr(&from, net_from);

	if (ret == -1)
	{
		err = WSAGetLastError();

		if (err == WSAEWOULDBLOCK)
			return false;
		if (err == WSAEMSGSIZE) {
			Com_Printf("Warning:  Oversize packet from %s\n",
				Net_AdrToString(*net_from));
			return false;
		}

		if (dedicated->value)	// let dedicated servers continue after errors
			Com_Printf("NET_GetPacket: %s from %s\n", Net_ErrorString(),
				Net_AdrToString(*net_from));
		else
			Com_Error(ERR_DROP, "NET_GetPacket: %s from %s", Net_ErrorString(), Net_AdrToString(*net_from));

		return false;
	}

	if (ret == net_message->maxsize)
	{
		Com_Printf("Oversize packet from %s\n", Net_AdrToString(*net_from));
		return false;
	}

	net_message->cursize = ret;
	return true;
}

//=============================================================================

void Net_SendPacket(netsrc_t sock, int32_t length, void* data, netadr_t to)
{
	int32_t 	ret;
	struct sockaddr	addr;
	int32_t 	net_socket;

	if (to.type == NA_LOOPBACK)
	{
		Net_SendLoopPacket(sock, length, data, to);
		return;
	}

	if (to.type == NA_BROADCAST)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
			return;
	}
	else if (to.type == NA_IP)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
			return;
	}
	else
		Com_Error(ERR_FATAL, "Net_SendPacket: bad address type");

	NetadrToSockadr(&to, &addr);

	ret = sendto(net_socket, (const char*)data, length, 0, &addr, sizeof(addr));

	if (ret == -1)
	{
		int32_t err = WSAGetLastError();

		// wouldblock is silent
		if (err == WSAEWOULDBLOCK)
			return;

		// some PPP links dont allow broadcasts
		if ((err == WSAEADDRNOTAVAIL) && ((to.type == NA_BROADCAST)))
			return;

		if (dedicated->value)	// let dedicated servers continue after errors
		{
			Com_Printf("Net_SendPacket ERROR: %s to %s\n", Net_ErrorString(),
				Net_AdrToString(to));
		}
		else
		{
			if (err == WSAEADDRNOTAVAIL)
			{
				Com_DPrintf("Net_SendPacket Warning: %s : %s\n",
					Net_ErrorString(), Net_AdrToString(to));
			}
			else
			{
				Com_Error(ERR_DROP, "Net_SendPacket ERROR: %s to %s\n",
					Net_ErrorString(), Net_AdrToString(to));
			}
		}
	}
}


//=============================================================================


/*
====================
NET_Socket
====================
*/
int32_t Net_IPSocket(char* net_interface, int32_t port)
{
	int32_t 			newsocket;
	struct sockaddr_in	address;
	u_long		 		_true = 1;
	int32_t 			i = 1;
	int32_t 			err;

	if ((newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		err = WSAGetLastError();
		if (err != WSAEAFNOSUPPORT)
			Com_Printf("WARNING: UDP_OpenSocket: socket: %s", Net_ErrorString());
		return 0;
	}

	// make it non-blocking
	if (ioctlsocket(newsocket, FIONBIO, &_true) == -1)
	{
		Com_Printf("WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", Net_ErrorString());
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof(i)) == -1)
	{
		Com_Printf("WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", Net_ErrorString());
		return 0;
	}

	if (!net_interface || !net_interface[0] || !stricmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		Net_StringToSockaddr(net_interface, (struct sockaddr*)&address);

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((int16_t)port);

	address.sin_family = AF_INET;

	if (bind(newsocket, (const sockaddr*)&address, sizeof(address)) == -1)
	{
		Com_Printf("WARNING: UDP_OpenSocket: bind: %s\n", Net_ErrorString());
		closesocket(newsocket);
		return 0;
	}

	return newsocket;
}


/*
====================
NET_OpenIP
====================
*/
void Net_OpenIP()
{
	cvar_t* ip;
	int32_t port;
	int32_t dedicated;

	ip = Cvar_Get("ip", "localhost", CVAR_NOSET);

	dedicated = Cvar_VariableValue("dedicated");

	if (!ip_sockets[NS_SERVER])
	{
		port = Cvar_Get("ip_hostport", "0", CVAR_NOSET)->value;
		if (!port)
		{
			port = Cvar_Get("hostport", "0", CVAR_NOSET)->value;
			if (!port)
			{
				port = Cvar_Get("port", va("%i", PORT_SERVER), CVAR_NOSET)->value;
			}
		}
		ip_sockets[NS_SERVER] = Net_IPSocket(ip->string, port);
		if (!ip_sockets[NS_SERVER] && dedicated)
			Com_Error(ERR_FATAL, "Couldn't allocate dedicated server IP port");
	}


	// dedicated servers don't need client ports
	if (dedicated)
		return;

	if (!ip_sockets[NS_CLIENT])
	{
		port = Cvar_Get("ip_clientport", "0", CVAR_NOSET)->value;
		if (!port)
		{
			port = Cvar_Get("clientport", va("%i", PORT_CLIENT), CVAR_NOSET)->value;
			if (!port)
				port = PORT_ANY;
		}
		ip_sockets[NS_CLIENT] = Net_IPSocket(ip->string, port);
		if (!ip_sockets[NS_CLIENT])
			ip_sockets[NS_CLIENT] = Net_IPSocket(ip->string, PORT_ANY);
	}
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void Net_Config(bool multiplayer)
{
	int32_t 	i;
	static	bool	old_config;

	if (old_config == multiplayer)
		return;

	old_config = multiplayer;

	if (!multiplayer)
	{	// shut down any existing sockets
		for (i = 0; i < 2; i++)
		{
			if (ip_sockets[i])
			{
				closesocket(ip_sockets[i]);
				ip_sockets[i] = 0;
			}
		}
	}
	else
	{	// open sockets
		if (!noudp->value)
			Net_OpenIP();
	}
}

// sleeps msec or until net socket is ready
void Net_Sleep(int32_t msec)
{
	struct timeval timeout;
	fd_set	fdset;
	extern cvar_t* dedicated;
	int32_t i;

	if (!dedicated || !dedicated->value)
		return; // we're not a server, just run full speed

	FD_ZERO(&fdset);
	i = 0;
	if (ip_sockets[NS_SERVER]) {
		FD_SET(ip_sockets[NS_SERVER], &fdset); // network socket
		i = ip_sockets[NS_SERVER];
	}

	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = (msec % 1000) * 1000;
	select(i + 1, &fdset, NULL, NULL, &timeout);
}

//===================================================================


static WSADATA		winsockdata;

/*
====================
NET_Init
====================
*/
void Net_Init()
{
	WORD	wVersionRequested;
	int32_t 	r;

	wVersionRequested = MAKEWORD(1, 1);

	r = WSAStartup(MAKEWORD(1, 1), &winsockdata);

	if (r)
		Com_Error(ERR_FATAL, "Winsock initialization failed.");

	Com_Printf("Winsock Initialized\n");

	noudp = Cvar_Get("noudp", "0", CVAR_NOSET);

	net_shownet = Cvar_Get("net_shownet", "0", 0);
}


/*
====================
NET_Shutdown
====================
*/
void Net_Shutdown()
{
	Net_Config(false);	// close sockets

	WSACleanup();
}


/*
====================
NET_ErrorString
====================
*/
const char* Net_ErrorString()
{
	int32_t 	code;

	code = WSAGetLastError();
	switch (code)
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}
