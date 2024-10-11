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
// client_commands_demo.c -- demo code

#include <client/client.hpp>

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage()
{
	int32_t 	len, swlen;

	// the first eight bytes are just packet sequencing stuff
	len = net_message.cursize - 8;
	swlen = LittleInt(len);
	fwrite(&swlen, 4, 1, cls.demofile);
	fwrite(net_message.data + 8, len, 1, cls.demofile);
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f()
{
	int32_t len;

	if (!cls.demorecording)
	{
		Com_Printf("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	fwrite(&len, 4, 1, cls.demofile);
	fclose(cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
	Com_Printf("Stopped demo.\n");
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
void CL_Record_f()
{
	char	name[MAX_OSPATH];
	char	buf_data[MAX_MSGLEN];
	sizebuf_t	buf;
	int32_t 	i;
	int32_t 	len;
	entity_state_t* ent;
	entity_state_t	nullstate;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("record <demoname>\n");
		return;
	}

	if (cls.demorecording)
	{
		Com_Printf("Already recording.\n");
		return;
	}

	if (cls.state != ca_active)
	{
		Com_Printf("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	snprintf(name, sizeof(name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf("recording to %s.\n", name);
	FS_CreatePath(name);
	cls.demofile = fopen(name, "wb");
	if (!cls.demofile)
	{
		Com_Printf("ERROR: couldn't open.\n");
		return;
	}
	cls.demorecording = true;

	// don't start saving messages until a non-delta compressed message is received
	cls.demowaiting = true;

	//
	// write out messages to hold the startup information
	//
	SZ_Init(&buf, (uint8_t*)buf_data, sizeof(buf_data));

	// send the serverdata
	MSG_WriteByte(&buf, svc_serverdata);
	MSG_WriteInt(&buf, PROTOCOL_VERSION);
	MSG_WriteInt(&buf, 0x10000 + cl.servercount);
	MSG_WriteByte(&buf, 1);	// demos are always attract loops
	MSG_WriteString(&buf, cl.gamedir);
	MSG_WriteShort(&buf, cl.playernum);

	MSG_WriteString(&buf, cl.configstrings[CS_NAME]);

	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (cl.configstrings[i][0])
		{
			if (buf.cursize + strlen(cl.configstrings[i]) + 32 > buf.maxsize)
			{	// write it out
				len = LittleInt(buf.cursize);
				fwrite(&len, 4, 1, cls.demofile);
				fwrite(buf.data, buf.cursize, 1, cls.demofile);
				buf.cursize = 0;
			}

			MSG_WriteByte(&buf, svc_configstring);
			MSG_WriteShort(&buf, i);
			MSG_WriteString(&buf, cl.configstrings[i]);
		}

	}

	// baselines
	memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_EDICTS; i++)
	{
		ent = &cl_entities[i].baseline;
		if (!ent->modelindex)
			continue;

		if (buf.cursize + 64 > buf.maxsize)
		{	// write it out
			len = LittleInt(buf.cursize);
			fwrite(&len, 4, 1, cls.demofile);
			fwrite(buf.data, buf.cursize, 1, cls.demofile);
			buf.cursize = 0;
		}

		MSG_WriteByte(&buf, svc_spawnbaseline);
		MSG_WriteDeltaEntity(&nullstate, &cl_entities[i].baseline, &buf, true, true);
	}

	MSG_WriteByte(&buf, svc_stufftext);
	MSG_WriteString(&buf, "precache\n");

	// write it to the demo file

	len = LittleInt(buf.cursize);
	fwrite(&len, 4, 1, cls.demofile);
	fwrite(buf.data, buf.cursize, 1, cls.demofile);

	// the rest of the demo file will be individual frames
}