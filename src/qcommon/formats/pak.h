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

#pragma once
#include <stdint.h>

//
// pak.h : The PAK file format
//

/*
========================================================================

The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#define IDPAKHEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')
#define MAX_PAKFILE		256

typedef struct
{
	char	name[MAX_PAKFILE];
	int32_t 	filepos, filelen;
} dpackfile_t;

typedef struct
{
	int32_t 	ident;		// == IDPAKHEADER
	int32_t 	dirofs;
	int32_t 	dirlen;
} dpackheader_t;

#define	MAX_FILES_IN_PACK	4096