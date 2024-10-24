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
// cl_parse.c  -- parse a message received from the server

#include <client/client.hpp>

const char* svc_strings[256] =
{
	"svc_bad",

	"svc_muzzleflash",
	"svc_muzzleflash2",
	"svc_temp_entity",
	"svc_drawtext",
	"svc_event",

	"svc_disconnect",
	"svc_reconnect",
	"svc_sound",
	"svc_print",
	"svc_stufftext",
	"svc_serverdata",
	"svc_configstring",
	"svc_spawnbaseline",
	"svc_centerprint",
	"svc_download",
	"svc_playerinfo",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_frame",

};
/*
======================
CL_RegisterSounds
======================
*/
void CL_RegisterSounds()
{
	int32_t  i;

	S_BeginRegistration();
	CL_RegisterTEntSounds();
	for (i = 1; i < MAX_SOUNDS; i++)
	{
		if (!cl.configstrings[CS_SOUNDS + i][0])
			break;
		cl.sound_precache[i] = S_RegisterSound(cl.configstrings[CS_SOUNDS + i]);
	}
	S_EndRegistration();
}



/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData()
{
	char*	str;
	int32_t i;

	Com_DPrintf("Serverdata packet received.\n");
	//
	// wipe the client_state_t struct
	//
	CL_ClearState();
	cls.state = ca_connected;

	// parse protocol version number
	i = MSG_ReadInt(&net_message);
	cls.server_protocol = i;

	if (i != PROTOCOL_VERSION)
		Com_Error(ERR_DROP,
			"Server returned protocol version %i, not %i\nEither your engine is incompatible with the server, or you need to re-record the demo file you're trying to play.", i, PROTOCOL_VERSION);

	cl.servercount = MSG_ReadInt(&net_message);
	cl.attractloop = MSG_ReadByte(&net_message);

	// game directory
	str = MSG_ReadString(&net_message);
	strncpy(cl.gamedir, str, sizeof(cl.gamedir) - 1);

	// set gamedir
	if ((*str && (!game_asset_path->string
		|| strcmp(game_asset_path->string, str))
			|| *game_asset_path->string))
	{
		Cvar_Set("game", str);
	}

	// parse player entity number
	cl.playernum = MSG_ReadShort(&net_message);

	// get the full level name
	str = MSG_ReadString(&net_message);

	// seperate the printfs so the server message can have a color
	Com_Printf("\n\n\n\n");
	Com_Printf("%s\n", str);

	// need to prep refresh at next oportunity
	cl.refresh_prepped = false;
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline()
{
	entity_state_t* es;
	uint32_t 		bits;
	int32_t 		newnum;
	entity_state_t	nullstate;

	memset(&nullstate, 0, sizeof(nullstate));

	newnum = CL_ParseEntityBits(&bits);
	es = &cl_entities[newnum].baseline;
	CL_ParseDelta(&nullstate, es, newnum, bits);
}


/*
================
CL_LoadClientinfo

================
*/
void CL_LoadClientinfo(clientinfo_t* ci, const char* s)
{
	int32_t i;
	
	char* t;
	char  model_name[MAX_QPATH];
	char  skin_name[MAX_QPATH];
	char  model_filename[MAX_QPATH];
	char  skin_filename[MAX_QPATH];
	char  weapon_filename[MAX_QPATH];

	strncpy(ci->cinfo, s, sizeof(ci->cinfo));
	ci->cinfo[sizeof(ci->cinfo) - 1] = 0;

	// isolate the player's name
	strncpy(ci->name, s, sizeof(ci->name));
	ci->name[sizeof(ci->name) - 1] = 0;

	t = (char*)strstr(s, "\\"); // NO

	if (t)
	{
		ci->name[t - s] = 0;
		s = t + 1;
	}

	if (cl_noskins->value || *s == 0)
	{
		snprintf(model_filename, sizeof(model_filename), "players/male/tris.md2");
		snprintf(weapon_filename, sizeof(weapon_filename), "players/male/weapon.md2");
		snprintf(skin_filename, sizeof(skin_filename), "players/male/grunt.tga");
		snprintf(ci->iconname, sizeof(ci->iconname), "/players/male/grunt_i.tga");
		ci->model = re.RegisterModel(model_filename);
		memset(ci->weaponmodel, 0, sizeof(ci->weaponmodel));
		ci->weaponmodel[0] = re.RegisterModel(weapon_filename);
		ci->skin = re.RegisterSkin(skin_filename);
		ci->icon = re.RegisterPic(ci->iconname);
	}
	else
	{
		// isolate the model name
		strcpy(model_name, s);
		t = strstr(model_name, "/");
		if (!t)
			t = strstr(model_name, "\\");
		if (!t)
			t = model_name;

		*t = 0;

		// isolate the skin name
		strcpy(skin_name, s + strlen(model_name) + 1);

		// model file
		snprintf(model_filename, sizeof(model_filename), "players/%s/tris.md2", model_name);
		ci->model = re.RegisterModel(model_filename);
		if (!ci->model)
		{
			strcpy(model_name, "male");
			snprintf(model_filename, sizeof(model_filename), "players/male/tris.md2");
			ci->model = re.RegisterModel(model_filename);
		}

		// skin file
		snprintf(skin_filename, sizeof(skin_filename), "players/%s/%s.tga", model_name, skin_name);

		 
		ci->skin = re.RegisterSkin(skin_filename);

		// if we don't have the skin and the model wasn't male,
		// see if the male has it (this is for CTF's skins)
		if (!ci->skin && Q_stricmp(model_name, "male"))
		{
			// change model to male
			strcpy(model_name, "male");
			snprintf(model_filename, sizeof(model_filename), "players/male/tris.md2");
			ci->model = re.RegisterModel(model_filename);

			// see if the skin exists for the male model
			snprintf(skin_filename, sizeof(skin_filename), "players/%s/%s.tga", model_name, skin_name);
			ci->skin = re.RegisterSkin(skin_filename);
		}

		// if we still don't have a skin, it means that the male model didn't have
		// it, so default to grunt
		if (!ci->skin) {
			// see if the skin exists for the male model
			snprintf(skin_filename, sizeof(skin_filename), "players/%s/grunt.tga", model_name, skin_name);
			ci->skin = re.RegisterSkin(skin_filename);
		}

		// weapon file
		for (i = 0; i < num_cl_weaponmodels; i++) {
			snprintf(weapon_filename, sizeof(weapon_filename), "players/%s/%s", model_name, cl_weaponmodels[i]);
			ci->weaponmodel[i] = re.RegisterModel(weapon_filename);
			if (!ci->weaponmodel[i] && strcmp(model_name, "cyborg") == 0) {
				// try male
				snprintf(weapon_filename, sizeof(weapon_filename), "players/male/%s", cl_weaponmodels[i]);
				ci->weaponmodel[i] = re.RegisterModel(weapon_filename);
			}
			if (!cl_vwep->value)
				break; // only one when vwep is off
		}

		// icon file
		snprintf(ci->iconname, sizeof(ci->iconname), "/players/%s/%s_i.tga", model_name, skin_name);
		ci->icon = re.RegisterPic(ci->iconname);
	}

	// must have loaded all data types to be valud
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponmodel[0])
	{
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = NULL;
		ci->weaponmodel[0] = NULL;
		return;
	}
}

/*
================
CL_ParseClientinfo

Load the skin, icon, and model for a client
================
*/
void CL_ParseClientinfo(int32_t player)
{
	char* s;
	clientinfo_t* ci;

	s = cl.configstrings[player + CS_PLAYERSKINS];

	ci = &cl.clientinfo[player];

	CL_LoadClientinfo(ci, s);
}


/*
================
CL_ParseConfigString
================
*/
void CL_ParseConfigString()
{
	int32_t 	i;
	char* s;
	char	olds[MAX_QPATH];

	i = MSG_ReadShort(&net_message);
	if (i < 0 || i >= MAX_CONFIGSTRINGS)
		Com_Error(ERR_DROP, "configstring > MAX_CONFIGSTRINGS");
	s = MSG_ReadString(&net_message);

	strncpy(olds, cl.configstrings[i], sizeof(olds));
	olds[sizeof(olds) - 1] = 0;

	strcpy(cl.configstrings[i], s);

	// do something apropriate 

	if (i >= CS_LIGHTS && i < CS_LIGHTS + MAX_LIGHTSTYLES)
		CL_SetLightstyle(i - CS_LIGHTS);
	else if (i == CS_CDTRACK)
	{
		if (cl.refresh_prepped)
		{
			int32_t track = atoi(cl.configstrings[CS_CDTRACK]);
			Miniaudio_Play(track, true);
		}
	}
	else if (i >= CS_MODELS && i < CS_MODELS + MAX_MODELS)
	{
		if (cl.refresh_prepped)
		{
			cl.model_draw[i - CS_MODELS] = re.RegisterModel(cl.configstrings[i]);
			if (cl.configstrings[i][0] == '*')
				cl.model_clip[i - CS_MODELS] = Map_LoadInlineModel(cl.configstrings[i]);
			else
				cl.model_clip[i - CS_MODELS] = NULL;
		}
	}
	else if (i >= CS_SOUNDS && i < CS_SOUNDS + MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.sound_precache[i - CS_SOUNDS] = S_RegisterSound(cl.configstrings[i]);
	}
	else if (i >= CS_IMAGES && i < CS_IMAGES + MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.image_precache[i - CS_IMAGES] = re.RegisterPic(cl.configstrings[i]);
	}
	else if (i >= CS_PLAYERSKINS && i < CS_PLAYERSKINS + MAX_CLIENTS)
	{
		if (cl.refresh_prepped && strcmp(olds, s))
			CL_ParseClientinfo(i - CS_PLAYERSKINS);
	}
}


/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket()
{
	vec3_t  pos_v;
	float*	pos;
	int32_t channel, ent;
	int32_t sound_num;
	float 	volume;
	float 	attenuation;
	int32_t flags;
	float	ofs;

	flags = MSG_ReadByte(&net_message);
	sound_num = MSG_ReadByte(&net_message);

	if (flags & SND_VOLUME)
		volume = MSG_ReadByte(&net_message) / 255.0f;
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

	if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte(&net_message) / 64.0f;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	if (flags & SND_OFFSET)
		ofs = MSG_ReadByte(&net_message) / 1000.0f;
	else
		ofs = 0;

	if (flags & SND_ENT)
	{	// entity reletive
		channel = MSG_ReadShort(&net_message);
		ent = channel >> 3;
		if (ent > MAX_EDICTS)
			Com_Error(ERR_DROP, "CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	if (flags & SND_POS)
	{	// positioned in space
		MSG_ReadPos(&net_message, pos_v);

		pos = pos_v;
	}
	else	// use entity number
		pos = NULL;

	if (!cl.sound_precache[sound_num])
		return;

	S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume, attenuation, ofs);
}


void CL_ShowNet(const char* s)
{
	if (cl_shownet->value >= 2)
		Com_Printf("%3i:%s\n", net_message.readcount - 1, s);
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage()
{
	int32_t cmd;
	char*	s;
	// These are for multi-string messages
	char	str_tempbuf[MAX_UI_STRLEN] = { 0 };		
	char	str_tempbuf2[MAX_UI_STRLEN] = { 0 };
	int32_t i = 0;

	//
	// if recording demos, copy the message out
	//
	if (cl_shownet->value == 1)
		Com_Printf("%i ", net_message.cursize);
	else if (cl_shownet->value >= 2)
		Com_Printf("------------------\n");


	//
	// parse the message
	//
	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Error(ERR_DROP, "CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte(&net_message);

		if (cmd == -1)
		{
			CL_ShowNet("END OF MESSAGE");
			break;
		}

		if (cl_shownet->value >= 2)
		{
			if (!svc_strings[cmd])
				Com_Printf("%3i:BAD CMD %i\n", net_message.readcount - 1, cmd);
			else
				CL_ShowNet(svc_strings[cmd]);
		}

		// other commands
		switch (cmd)
		{
		default:
			Com_Error(ERR_DROP, "CL_ParseServerMessage: Illegible server message\n");
			break;

		case svc_disconnect:
			Com_Error(ERR_DISCONNECT, "Server disconnected\n");
			break;

		case svc_reconnect:
			Com_Printf("Server disconnected, reconnecting\n");
			if (cls.download) {
				//ZOID, close download
				fclose(cls.download);
				cls.download = NULL;
			}
			cls.state = ca_connecting;
			cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
			break;

		case svc_print:
			i = MSG_ReadByte(&net_message);

			if (i == PRINT_CHAT)
				S_StartLocalSound("misc/talk.wav");

			Com_Printf("%s", MSG_ReadString(&net_message));

			break;

		case svc_centerprint:
			Render2D_CenterPrint(MSG_ReadString(&net_message));
			break;

		case svc_stufftext:
			s = MSG_ReadString(&net_message);
			Com_DPrintf("stufftext: %s\n", s);
			Cbuf_AddText(s);
			break;

		case svc_serverdata:
			Cbuf_Execute();		// make sure any stuffed commands are done
			CL_ParseServerData();
			break;

		case svc_configstring:
			CL_ParseConfigString();
			break;

		case svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case svc_spawnbaseline:
			CL_ParseBaseline();
			break;

		case svc_temp_entity:
			CL_ParseTEnt();
			break;

		case svc_muzzleflash:
			CL_ParseMuzzleFlash();
			break;

		case svc_muzzleflash2:
			CL_ParseMuzzleFlash2();
			break;

		case svc_download:
			CL_ParseDownload();
			break;

		case svc_frame:
			CL_ParseFrame();
			break;
			
		case svc_event:
			// handle the event
			CL_ParseEvent();
			break;

		case svc_playerinfo:
		case svc_packetentities:
		case svc_deltapacketentities:
			Com_Error(ERR_DROP, "Out of place frame data");
			break;

		case svc_drawtext:
			s = MSG_ReadString(&net_message);
			strncpy(str_tempbuf, s, MAX_UI_STRLEN);
			int32_t x = MSG_ReadShort(&net_message);
			int32_t y = MSG_ReadShort(&net_message);
			s = MSG_ReadString(&net_message);
			strncpy(str_tempbuf2, s, MAX_UI_STRLEN);

			Text_Draw(str_tempbuf, x, y, str_tempbuf2);
		}
	}

	Render2D_AddNetgraph();

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (cls.demorecording && !cls.demowaiting)
		CL_WriteDemoMessage();

}
