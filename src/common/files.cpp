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

#include "common.hpp"

// define this to dissalow any data but the demo pak file
//#define	NO_ADDONS

// if a packfile directory differs from this, it is assumed to be hacked
// Full version

// This is for quake2, if you ship a game, change this...
#define	PAK0_CHECKSUM	0x40e614e0

/*
=============================================================================

GAME FILESYSTEM

=============================================================================
*/

cvar_t* game_basedir;

//
// in memory
//

typedef struct
{
	char	name[MAX_QPATH];
	int32_t filepos, filelen;
} packfile_t;

typedef struct pack_s
{
	char		filename[MAX_OSPATH];
	FILE* handle;
	int32_t 	numfiles;
	packfile_t* files;
} pack_t;

typedef struct filelink_s
{
	struct filelink_s* next;
	char* from;
	int32_t 	fromlength;
	char* to;
} filelink_t;

filelink_t* fs_links;

typedef struct searchpath_s
{
	char	filename[MAX_OSPATH];
	pack_t* pack;		// only one of filename / pack will be used
	struct searchpath_s* next;
} searchpath_t;

searchpath_t* fs_searchpaths;
searchpath_t* fs_base_searchpaths;	// without gamedirs

/*

All of Quake's data access is through a hierarchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

*/

/*
================
FS_filelength
================
*/
int32_t FS_filelength(FILE* f)
{
	int32_t 	pos;
	int32_t 	end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void	FS_CreatePath(char* path)
{
	char* ofs;

	for (ofs = path + 1; *ofs; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_Mkdir(path);
			*ofs = '/';
		}
	}
}

/*
==============
FS_FCloseFile

For some reason, other dll's can't just cal fclose()
on files returned by FS_FOpenFile...
==============
*/
void FS_FCloseFile(FILE* f)
{
	fclose(f);
}

/*
===========
FS_FOpenFile

Finds the file in the search path.
returns filesize and an open FILE *
Used for streaming data out of either a pak file or
a seperate file.
===========
*/
int32_t file_from_pak = 0;


int32_t FS_FOpenFile(const char* filename, FILE** file)
{
	searchpath_t* search;
	char		netpath[MAX_OSPATH];
	pack_t* pak;
	int32_t 	 i;
	filelink_t* link;

	file_from_pak = 0;

	// check for links first
	for (link = fs_links; link; link = link->next)
	{
		if (!strncmp(filename, link->from, link->fromlength))
		{
			snprintf(netpath, sizeof(netpath), "%s%s", link->to, filename + link->fromlength);
			*file = fopen(netpath, "rb");
			if (*file)
			{
				Com_DPrintf("link file: %s\n", netpath);
				return FS_filelength(*file);
			}
			return -1;
		}
	}

	//
	// search through the path, one element at a time
	//
	for (search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (search->pack)
		{
			// look through all the pak file elements
			pak = search->pack;
			for (i = 0; i < pak->numfiles; i++)
				if (!Q_strcasecmp(pak->files[i].name, filename))
				{	// found it!
					file_from_pak = 1;
					Com_DPrintf("PackFile: %s : %s\n", pak->filename, filename);
					// open a new file on the pakfile
					*file = fopen(pak->filename, "rb");
					if (!*file)
						Com_Error(ERR_FATAL, "Couldn't reopen %s", pak->filename);
					fseek(*file, pak->files[i].filepos, SEEK_SET);
					return pak->files[i].filelen;
				}
		}
		else
		{
			// check a file in the directory tree

			snprintf(netpath, sizeof(netpath), "%s/%s", search->filename, filename);
#ifndef _WIN32
			// some expansion packs use backslashes in file paths which works only on Windows
			char* np = netpath;
			while (*np++) *np = *np == '\\' ? '/' : *np;
#endif
			* file = fopen(netpath, "rb");
			if (!*file)
				continue;

			Com_DPrintf("FindFile: %s\n", netpath);

			return FS_filelength(*file);
		}

	}

	Com_DPrintf("FindFile: can't find %s\n", filename);

	*file = NULL;
	return -1;
}


/*
=================
FS_ReadFile

Properly handles partial reads
=================
*/
#define	MAX_READ	0x10000		// read in blocks of 64k
void FS_Read(void* buffer, int32_t len, FILE* f)
{
	int32_t 	block, remaining;
	int32_t 	read;
	uint8_t* buf;
	int32_t 	tries;

	buf = (uint8_t*)buffer;

	// read in chunks for progress bar
	remaining = len;
	tries = 0;
	while (remaining)
	{
		block = remaining;
		if (block > MAX_READ)
			block = MAX_READ;
		read = (int32_t)fread(buf, 1, block, f);
		if (read == 0)
		{
			Com_Error(ERR_FATAL, "FS_Read: 0 bytes read");
		}
		else if (read == -1)
			Com_Error(ERR_FATAL, "FS_Read: -1 bytes read");

		// do some progress bar thing here...

		remaining -= read;
		buf += read;
	}
}

/*
============
FS_LoadFile

Filename are reletive to the quake search path
a null buffer will just return the file length without loading
============
*/
int32_t FS_LoadFile(const char* path, void** buffer)
{
	FILE*		h;
	uint8_t*	buf;
	int32_t 	len;

	buf = NULL;	// quiet compiler warning

	// look for it in the filesystem or pack files
	len = FS_FOpenFile(path, &h);
	if (!h)
	{
		if (buffer)
			*buffer = NULL;
		return -1;
	}

	if (!buffer)
	{
		fclose(h);
		return len;
	}

	buf = (uint8_t*)Memory_ZoneMalloc(len);
	*buffer = buf;

	FS_Read(buf, len, h);

	fclose(h);

	return len;
}


/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile(void* buffer)
{
	Memory_ZoneFree(buffer);
}

/*
=================
FS_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
pack_t* FS_LoadPackFile(char* packfile)
{
	dpackheader_t	header;
	int32_t 		i;
	packfile_t*		newfiles;
	int32_t 		numpackfiles;
	pack_t*			pack;
	FILE*			packhandle;
	dpackfile_t		info[MAX_FILES_IN_PACK];
	uint32_t		checksum;

	packhandle = fopen(packfile, "rb");
	if (!packhandle)
		return NULL;

	fread(&header, 1, sizeof(header), packhandle);
	if (LittleInt(header.ident) != IDPAKHEADER)
		Com_Error(ERR_FATAL, "%s is not a packfile", packfile);
	header.dirofs = LittleInt(header.dirofs);
	header.dirlen = LittleInt(header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK)
		Com_Error(ERR_FATAL, "%s has %i files", packfile, numpackfiles);

	newfiles = (packfile_t*)Memory_ZoneMalloc(numpackfiles * sizeof(packfile_t));

	fseek(packhandle, header.dirofs, SEEK_SET);
	fread(info, 1, header.dirlen, packhandle);

	// crc the directory to check for modifications
	checksum = Com_BlockChecksum((void*)info, header.dirlen);

#ifdef NO_ADDONS
	if (checksum != PAK0_CHECKSUM)
		return NULL;
#endif
	// parse the directory
	for (i = 0; i < numpackfiles; i++)
	{
		strcpy(newfiles[i].name, info[i].name);
		newfiles[i].filepos = LittleInt(info[i].filepos);
		newfiles[i].filelen = LittleInt(info[i].filelen);
	}

	pack = (pack_t*)Memory_ZoneMalloc(sizeof(pack_t));
	strcpy(pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;

	Com_Printf("Added packfile %s (%i files)\n", packfile, numpackfiles);
	return pack;
}


/*
================
FS_AddGameDirectory

Sets game_asset_path, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
void FS_AddGameDirectory(char* dir)
{
	int32_t 		i;
	searchpath_t*	search;
	pack_t*			pak;
	char			pakfile[MAX_OSPATH];

	//
	// add the directory to the search path
	//
	search = (searchpath_t*)Memory_ZoneMalloc(sizeof(searchpath_t));
	strcpy(search->filename, dir);
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	//
	// add any pak files in the format pak0.pak pak1.pak, ...
	//
	for (i = 0; i < 10; i++)
	{
		snprintf(pakfile, sizeof(pakfile), "%s/pak%i.pak", dir, i);
		pak = FS_LoadPackFile(pakfile);
		if (!pak)
			continue;
		search = (searchpath_t*)Memory_ZoneMalloc(sizeof(searchpath_t));
		search->pack = pak;
		search->next = fs_searchpaths;
		fs_searchpaths = search;
	}


}

/*
============
FS_Gamedir

Called to find where to write a file (demos, savegames, etc)
============
*/
char* FS_Gamedir()
{
	return game_asset_path->string;
}

/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec()
{
	const char* dir;
	char name[MAX_QPATH] = { 0 };

	dir = Cvar_VariableString("game_asset_path");
	if (*dir)
		snprintf(name, sizeof(name), "%s/%s/autoexec.cfg", game_basedir->string, dir);
	else
		snprintf(name, sizeof(name), "%s/%s/autoexec.cfg", game_basedir->string, game_asset_path->string);

	if (Sys_FindFirst(name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM))
		Cbuf_AddText("exec autoexec.cfg\n");

	Sys_FindClose();
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir(char* dir)
{
	searchpath_t* next;

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":"))
	{
		Com_Printf("Gamedir should be a single filename, not a path\n");
		return;
	}

	//
	// free up any current game dir info
	//
	while (fs_searchpaths != fs_base_searchpaths)
	{
		if (fs_searchpaths->pack)
		{
			fclose(fs_searchpaths->pack->handle);
			Memory_ZoneFree(fs_searchpaths->pack->files);
			Memory_ZoneFree(fs_searchpaths->pack);
		}
		next = fs_searchpaths->next;
		Memory_ZoneFree(fs_searchpaths);
		fs_searchpaths = next;
	}

	//
	// flush all data, so it will be forced to reload
	//
	if (dedicated && !dedicated->value)
		Cbuf_AddText("vid_restart\nsnd_restart\n");

	if (!strcmp(dir, game_asset_path->string) || (*dir == 0))
	{
		Cvar_FullSet("game", "", CVAR_LATCH | CVAR_SERVERINFO);
	}
	else
	{
		Cvar_FullSet("game_asset_path", dir, CVAR_SERVERINFO | CVAR_NOSET);
		FS_AddGameDirectory(va("%s/%s", game_basedir->string, dir));
	}
}


/*
================
FS_Link_f

Creates a filelink_t
================
*/
void FS_Link_f()
{
	filelink_t* l, ** prev;

	if (Cmd_Argc() != 3)
	{
		Com_Printf("USAGE: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	prev = &fs_links;
	for (l = fs_links; l; l = l->next)
	{
		if (!strcmp(l->from, Cmd_Argv(1)))
		{
			Memory_ZoneFree(l->to);
			if (!strlen(Cmd_Argv(2)))
			{	// delete it
				*prev = l->next;
				Memory_ZoneFree(l->from);
				Memory_ZoneFree(l);
				return;
			}
			l->to = CopyString(Cmd_Argv(2));
			return;
		}
		prev = &l->next;
	}

	// create a new link
	l = (filelink_t*)Memory_ZoneMalloc(sizeof(*l));
	l->next = fs_links;
	fs_links = l;
	l->from = CopyString(Cmd_Argv(1));
	l->fromlength = (int32_t)strlen(l->from);
	l->to = CopyString(Cmd_Argv(2));
}

/*
** FS_ListFiles
*/
char** FS_ListFiles(char* findname, int32_t* numfiles, unsigned musthave, unsigned canthave)
{
	char* s;
	int32_t nfiles = 0;
	char** list = 0;

	s = Sys_FindFirst(findname, musthave, canthave);
	while (s)
	{
		if (s[strlen(s) - 1] != '.')
			nfiles++;
		s = Sys_FindNext(musthave, canthave);
	}
	Sys_FindClose();

	if (!nfiles)
		return NULL;

	nfiles++; // add space for a guard
	*numfiles = nfiles;

	list = (char**)malloc(sizeof(char*) * nfiles);
	memset(list, 0, sizeof(char*) * nfiles);

	s = Sys_FindFirst(findname, musthave, canthave);
	nfiles = 0;
	while (s)
	{
		if (s[strlen(s) - 1] != '.')
		{
			list[nfiles] = strdup(s);
#ifdef _WIN32
			strlwr(list[nfiles]);
#endif
			nfiles++;
		}
		s = Sys_FindNext(musthave, canthave);
	}
	Sys_FindClose();

	return list;
}

/*
** FS_Dir_f
*/
void FS_Dir_f(void)
{
	char*	path = NULL;
	char	findname[1024];
	char	wildcard[1024] = "*.*";
	char**	dirnames;
	int32_t ndirs;

	if (Cmd_Argc() > 1)
	{
		strcpy(wildcard, Cmd_Argv(1));
	}

	while ((path = FS_NextPath(path)) != NULL)
	{
		char* tmp = findname;

		snprintf(findname, sizeof(findname), "%s/%s", path, wildcard);

		while (*tmp != 0)
		{
			if (*tmp == '\\')
				*tmp = '/';
			tmp++;
		}
		Com_Printf("Directory of %s\n", findname);
		Com_Printf("----\n");

		if ((dirnames = FS_ListFiles(findname, &ndirs, 0, 0)) != 0)
		{
			int32_t i;

			for (i = 0; i < ndirs - 1; i++)
			{
				if (strrchr(dirnames[i], '/'))
					Com_Printf("%s\n", strrchr(dirnames[i], '/') + 1);
				else
					Com_Printf("%s\n", dirnames[i]);

				free(dirnames[i]);
			}
			free(dirnames);
		}
		Com_Printf("\n");
	};
}

/*
============
FS_Path_f

============
*/
void FS_Path_f()
{
	searchpath_t* s;
	filelink_t* l;

	Com_Printf("Current search path:\n");
	for (s = fs_searchpaths; s; s = s->next)
	{
		if (s == fs_base_searchpaths)
			Com_Printf("----------\n");
		if (s->pack)
			Com_Printf("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		else
			Com_Printf("%s\n", s->filename);
	}

	Com_Printf("\nLinks:\n");
	for (l = fs_links; l; l = l->next)
		Com_Printf("%s : %s\n", l->from, l->to);
}

/*
================
FS_NextPath

Allows enumerating all of the directories in the search path
================
*/
char* FS_NextPath(char* prevpath)
{
	searchpath_t* s;
	char* prev;

	if (!prevpath)
		return game_asset_path->string;

	prev = game_asset_path->string;
	for (s = fs_searchpaths; s; s = s->next)
	{
		if (s->pack)
			continue;
		if (prevpath == prev)
			return s->filename;
		prev = s->filename;
	}

	return NULL;
}


/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem()
{
	Cmd_AddCommand("path", FS_Path_f);
	Cmd_AddCommand("link", FS_Link_f);
	Cmd_AddCommand("dir", FS_Dir_f);

	//todo: get current working directory
	game_basedir = Cvar_Get("basedir", "", 0);

	//
	// start up with zombonogame by default
	//

	// slight hack, since game_basedir is now the current working directory by default, we have to do some kludging so that we try and load from the right directory

	if (strlen(game_basedir->string) == 0)
	{
		FS_AddGameDirectory(game_asset_path->string);
	}
	else
	{
		FS_AddGameDirectory(va("%s/%s", game_basedir->string, game_asset_path->string));
	}

	// any set gamedirs will be freed up to here
	fs_base_searchpaths = fs_searchpaths;

	// check for game override
	cvar_t* game_override = Cvar_Get("game", "", CVAR_LATCH | CVAR_SERVERINFO);

	if (strlen(game_override->string) > 0)
	{
		FS_SetGamedir(game_override->string);
	}
	else
	{
		FS_SetGamedir(game_asset_path->string);
		// actually make it an alias for compatibility...
		Cvar_Set(game_override->string, game_asset_path->string);
	}

}