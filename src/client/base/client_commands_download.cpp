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
// client_commands_demo.c -- download code

#include <client/client.hpp>

int32_t precache_check; // for autodownload of precache items
int32_t precache_spawncount;
int32_t precache_tex;
int32_t precache_model_skin;

uint8_t* precache_model; // used for skin checking in alias models

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)

// Suffixes for skybox names.
static const char* env_suf[6] = { "rt", "bk", "lf", "ft", "up", "dn" };

extern cvar_t* allow_download;
extern cvar_t* allow_download_players;
extern cvar_t* allow_download_models;
extern cvar_t* allow_download_sounds;
extern cvar_t* allow_download_maps;

void CL_DownloadFileName(char* dest, int32_t destlen, char* fn)
{
	if (strncmp(fn, "players", 7) == 0)
		snprintf(dest, destlen, "%s/%s", game_asset_path->string, fn);
	else
		snprintf(dest, destlen, "%s/%s", FS_Gamedir(), fn);
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload()
{
	int32_t 	size, percent;
	char	name[MAX_OSPATH];
	int32_t 	r;

	// read the data
	size = MSG_ReadShort(&net_message);
	percent = MSG_ReadByte(&net_message);
	if (size == -1)
	{
		Com_Printf("Server does not have this file.\n");
		if (cls.download)
		{
			// if here, we tried to resume a file but the server said no
			fclose(cls.download);
			cls.download = NULL;
		}
		CL_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);

		FS_CreatePath(name);

		cls.download = fopen(name, "wb");
		if (!cls.download)
		{
			net_message.readcount += size;
			Com_Printf("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload();
			return;
		}
	}

	fwrite(net_message.data + net_message.readcount, 1, size, cls.download);
	net_message.readcount += size;

	if (percent != 100)
	{

		cls.downloadpercent = percent;

		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		SZ_Print(&cls.netchan.message, "nextdl");
	}
	else
	{
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

		//		Com_Printf ("100%%\n");

		fclose(cls.download);

		// rename the temp file to it's final name
		CL_DownloadFileName(oldn, sizeof(oldn), cls.downloadtempname);
		CL_DownloadFileName(newn, sizeof(newn), cls.downloadname);
		r = rename(oldn, newn);
		if (r)
			Com_Printf("failed to rename.\n");

		cls.download = NULL;
		cls.downloadpercent = 0;

		// get another file if needed

		CL_RequestNextDownload();
	}
}

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
bool CL_CheckOrDownloadFile(char* filename)
{
	FILE* fp;
	char	name[MAX_OSPATH];

	if (strstr(filename, ".."))
	{
		Com_Printf("Refusing to download a path with ..\n");
		return true;
	}

	if (FS_LoadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		return true;
	}

	strcpy(cls.downloadname, filename);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension(cls.downloadname, cls.downloadtempname);
	strcat(cls.downloadtempname, ".tmp");

	// check to see if we already have a tmp for this file, if so, try to resume
	// open the file if not opened yet
	CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);

	//	FS_CreatePath (name);

	fp = fopen(name, "r+b");
	if (fp)
	{ // it exists
		int32_t len;
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		cls.download = fp;

		// give the server an offset to start the download
		Com_Printf("Resuming %s\n", cls.downloadname);
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message,
			va("download %s %i", cls.downloadname, len));
	}
	else
	{
		Com_Printf("Downloading %s\n", cls.downloadname);
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message,
			va("download %s", cls.downloadname));
	}

	cls.downloadnumber++;

	return false;
}

/*
===============
CL_Download_f

Request a download from the server
===============
*/
void CL_Download_f()
{
	char filename[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: download <filename>\n");
		return;
	}

	snprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));

	if (strstr(filename, ".."))
	{
		Com_Printf("Refusing to download a path with ..\n");
		return;
	}

	if (FS_LoadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		Com_Printf("File already exists.\n");
		return;
	}

	strcpy(cls.downloadname, filename);
	Com_Printf("Downloading %s\n", cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension(cls.downloadname, cls.downloadtempname);
	strcat(cls.downloadtempname, ".tmp");

	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
	MSG_WriteString(&cls.netchan.message,
		va("download %s", cls.downloadname));

	cls.downloadnumber++;
}


void CL_RequestNextDownload()
{
	uint32_t	map_checksum;		// for detecting cheater maps
	char		fn[MAX_OSPATH];
	dmdl_t* pheader;

	if (cls.state != ca_connected)
		return;

	if (!allow_download->value && precache_check < ENV_CNT)
		precache_check = ENV_CNT;

	if (precache_check == CS_MODELS)
	{ // confirm map
		precache_check = CS_MODELS + 2; // 0 isn't used
		if (allow_download_maps->value)
		{
			if (!CL_CheckOrDownloadFile(cl.configstrings[CS_MODELS + 1]))
				return; // started a download
		}
	}
	if (precache_check >= CS_MODELS && precache_check < CS_MODELS + MAX_MODELS)
	{
		if (allow_download_models->value)
		{
			while (precache_check < CS_MODELS + MAX_MODELS &&
				cl.configstrings[precache_check][0])
			{
				if (cl.configstrings[precache_check][0] == '*' ||
					cl.configstrings[precache_check][0] == '#')
				{
					precache_check++;
					continue;
				}

				if (precache_model_skin == 0) {
					if (!CL_CheckOrDownloadFile(cl.configstrings[precache_check]))
					{
						precache_model_skin = 1;
						return; // started a download
					}
					precache_model_skin = 1;
				}

				// checking for skins in the model
				if (!precache_model)
				{

					FS_LoadFile(cl.configstrings[precache_check], (void**)&precache_model);
					if (!precache_model) {
						precache_model_skin = 0;
						precache_check++;
						continue; // couldn't load it
					}
					if (LittleInt(*(uint32_t*)precache_model) != IDALIASHEADER)
					{
						// not an alias model
						FS_FreeFile(precache_model);
						precache_model = 0;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
					pheader = (dmdl_t*)precache_model;
					if (LittleInt(pheader->version) != ALIAS_VERSION)
					{
						precache_check++;
						precache_model_skin = 0;
						continue; // couldn't load it
					}
				}

				pheader = (dmdl_t*)precache_model;

				while (precache_model_skin - 1 < LittleInt(pheader->num_skins))
				{
					if (!CL_CheckOrDownloadFile((char*)precache_model +
						LittleInt(pheader->ofs_skins) +
						(precache_model_skin - 1) * MAX_SKINNAME)) {
						precache_model_skin++;
						return; // started a download
					}
					precache_model_skin++;
				}

				if (precache_model)
				{
					FS_FreeFile(precache_model);
					precache_model = 0;
				}

				precache_model_skin = 0;
				precache_check++;
			}
		}
		precache_check = CS_SOUNDS;
	}

	if (precache_check >= CS_SOUNDS && precache_check < CS_SOUNDS + MAX_SOUNDS)
	{
		if (allow_download_sounds->value)
		{
			if (precache_check == CS_SOUNDS)
				precache_check++; // zero is blank

			while (precache_check < CS_SOUNDS + MAX_SOUNDS &&
				cl.configstrings[precache_check][0])
			{
				if (cl.configstrings[precache_check][0] == '*')
				{
					precache_check++;
					continue;
				}

				snprintf(fn, sizeof(fn), "sound/%s", cl.configstrings[precache_check++]);

				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}

		precache_check = CS_IMAGES;
	}

	if (precache_check >= CS_IMAGES && precache_check < CS_IMAGES + MAX_IMAGES)
	{
		if (precache_check == CS_IMAGES)
			precache_check++; // zero is blank

		while (precache_check < CS_IMAGES + MAX_IMAGES &&
			cl.configstrings[precache_check][0])
		{
			snprintf(fn, sizeof(fn), "%s.tga", cl.configstrings[precache_check++]);
			if (!CL_CheckOrDownloadFile(fn))
				return; // started a download
		}

		precache_check = CS_PLAYERSKINS;
	}

	// skins
//  are special, since a player has three things to download:
	// model, weapon model and skin
	// so precache_check is now *3
	if (precache_check >= CS_PLAYERSKINS && precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
	{
		if (allow_download_players->value)
		{
			while (precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
			{
				int32_t i, n;
				char model[MAX_QPATH], skin[MAX_QPATH], * p;

				i = (precache_check - CS_PLAYERSKINS) / PLAYER_MULT;
				n = (precache_check - CS_PLAYERSKINS) % PLAYER_MULT;

				if (!cl.configstrings[CS_PLAYERSKINS + i][0]) {
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.configstrings[CS_PLAYERSKINS + i], '\\')) != NULL)
					p++;
				else
					p = cl.configstrings[CS_PLAYERSKINS + i];
				strcpy(model, p);
				p = strchr(model, '/');

				if (!p)
					p = strchr(model, '\\');

				if (p)
				{
					*p++ = 0;
					strcpy(skin, p);
				}
				else
				{
					*skin = 0;
				}


				switch (n)
				{
				case 0: // model
					snprintf(fn, sizeof(fn), "players/%s/tris.md2", model);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 1: // weapon model
					snprintf(fn, sizeof(fn), "players/%s/weapon.md2", model);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 2: // weapon skin
					snprintf(fn, sizeof(fn), "players/%s/weapon.tga", model);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 3: // skin
					snprintf(fn, sizeof(fn), "players/%s/%s.tga", model, skin);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 4: // skin_i
					snprintf(fn, sizeof(fn), "players/%s/%s_i.tga", model, skin);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
						return; // started a download
					}
					// move on to next model
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		precache_check = ENV_CNT;
	}

	if (precache_check == ENV_CNT)
	{
		precache_check = ENV_CNT + 1;

		Map_Load(cl.configstrings[CS_MODELS + 1], true, &map_checksum);

		if (map_checksum != atoi(cl.configstrings[CS_MAPCHECKSUM]))
		{
			Com_Error(ERR_DROP, "Local map version differs from server: %i != '%s'\n",
				map_checksum, cl.configstrings[CS_MAPCHECKSUM]);
			return;
		}
	}

	if (precache_check > ENV_CNT && precache_check < TEXTURE_CNT)
	{
		if (allow_download->value && allow_download_maps->value)
		{
			while (precache_check < TEXTURE_CNT)
			{
				int32_t n = precache_check++ - ENV_CNT - 1;

				if (n & 1)
					snprintf(fn, sizeof(fn), "env/%s%s.tga",
						cl.configstrings[CS_SKY], env_suf[n / 2]);
				else
					snprintf(fn, sizeof(fn), "env/%s%s.tga",
						cl.configstrings[CS_SKY], env_suf[n / 2]);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT;
	}

	if (precache_check == TEXTURE_CNT)
	{
		precache_check = TEXTURE_CNT + 1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == TEXTURE_CNT + 1)
	{
		// from common/cmodel.c
		extern int32_t 		numtexinfo;
		extern mapsurface_t	map_surfaces[];

		if (allow_download->value && allow_download_maps->value)
		{
			while (precache_tex < numtexinfo)
			{
				char fn[MAX_OSPATH];

				sprintf(fn, "textures/%s.tga", map_surfaces[precache_tex++].rname);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT + 999;
	}

	CL_RegisterSounds();
	Render3D_PrepRefresh();

	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
	MSG_WriteString(&cls.netchan.message, va("begin %i\n", precache_spawncount));
}

/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f()
{
	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2)
	{
		uint32_t	map_checksum;		// for detecting cheater maps
		Map_Load(cl.configstrings[CS_MODELS + 1], true, &map_checksum);
		CL_RegisterSounds();
		Render3D_PrepRefresh();
		return;
	}

	precache_check = CS_MODELS;
	precache_spawncount = atoi(Cmd_Argv(1));
	precache_model = 0;
	precache_model_skin = 0;

	CL_RequestNextDownload();
}
