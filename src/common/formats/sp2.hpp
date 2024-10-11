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
#include <cstdint>


/*
========================================================================

.SP2 sprite file format

========================================================================
*/

#define IDSPRITEHEADER	(('2'<<24)+('S'<<16)+('D'<<8)+'I')
// little-endian "IDS2"
#define SPRITE_VERSION	2

typedef struct dsprframe_s
{
	int32_t 	width, height;
	int32_t 	origin_x, origin_y;		// raster coordinates inside pic
	char	name[MAX_SKINNAME];		// name of tga file
} dsprframe_t;

typedef struct dsprite_s
{
	int32_t 		ident;
	int32_t 		version;
	int32_t 		numframes;
	dsprframe_t		frames[1];			// variable sized
} dsprite_t;
