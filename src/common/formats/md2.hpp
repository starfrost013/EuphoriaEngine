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

.MD2 triangle model file format

========================================================================
*/

#define IDALIASHEADER	(('2'<<24)+('P'<<16)+('D'<<8)+'I')
#define ALIAS_VERSION	8

#define	MAX_TRIANGLES	8192
#define MAX_VERTS		8192
#define MAX_FRAMES		512
#define MAX_MD2SKINS	32
#define	MAX_SKINNAME	64

typedef struct dstvert_s
{
	short	s;
	short	t;
} dstvert_t;

typedef struct dtriangle_s
{
	short	index_xyz[3];
	short	index_st[3];
} dtriangle_t;

typedef struct dtrivertx_s
{
	uint8_t	v[3];			// scaled byte to fit in frame mins/maxs
	uint8_t	lightnormalindex;
} dtrivertx_t;

#define DTRIVERTX_V0   0
#define DTRIVERTX_V1   1
#define DTRIVERTX_V2   2
#define DTRIVERTX_LNI  3
#define DTRIVERTX_SIZE 4

typedef struct daliasframe_s
{
	float		scale[3];	// multiply byte verts by this
	float		translate[3];	// then add this
	char		name[16];	// frame name from grabbing
	dtrivertx_t	verts[1];	// variable sized
} daliasframe_t;


// the glcmd format:
// a positive integer starts a tristrip command, followed by that many
// vertex structures.
// a negative integer starts a trifan command, followed by -x vertexes
// a zero indicates the end of the command list.
// a vertex consists of a floating point s, a floating point t,
// and an integer vertex index.


typedef struct dmdl_s
{
	int32_t 		ident;
	int32_t 		version;

	int32_t 		skinwidth;
	int32_t 		skinheight;
	int32_t 		framesize;		// byte size of each frame

	int32_t 		num_skins;
	int32_t 		num_xyz;
	int32_t 		num_st;			// greater than num_xyz for seams
	int32_t 		num_tris;
	int32_t 		num_glcmds;		// dwords in strip/fan command list
	int32_t 		num_frames;

	int32_t 		ofs_skins;		// each skin is a MAX_SKINNAME string
	int32_t 		ofs_st;			// byte offset from start for stverts
	int32_t 		ofs_tris;		// offset for dtriangles
	int32_t 		ofs_frames;		// offset for first frame
	int32_t 		ofs_glcmds;
	int32_t 		ofs_end;		// end of file

} dmdl_t;
