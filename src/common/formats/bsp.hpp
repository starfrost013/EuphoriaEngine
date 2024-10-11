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

/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define ZBSP_HEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'Z')
// little-endian "ZBSP"

#define ZBSP_VERSION	2

// maximum allocation sizes for various models
// boosted by 4x to account for 32-bit texturing
#define MAX_SP2_ALLOC			0x100000  // 1MB (from 64KB) 		
#define MAX_MD2_ALLOC			0x1000000 // 16MB (from 4)
#define MAX_BSP_ALLOC			0x4000000 // 64MB (from 16)

// Limit are based on QBISM QBSP format, with some modification for Zombono

// Zombono BSP limits
#define MAX_MAP_AREAS           256
#define MAX_MAP_AREAPORTALS     1024
#define WARN_MAP_MODELS         32768
#define MAX_MAP_MODELS          131072
#define MAX_MAP_BRUSHES         1048576
#define WARN_MAP_ENTITIES       32768
#define MAX_MAP_ENTITIES        131072
#define MAX_MAP_ENTSTRING       13631488
#define MAX_MAP_TEXINFO         1048576
#define MAX_MAP_PLANES          1048576
#define MAX_MAP_NODES           1048576
#define MAX_MAP_LEAFS           1048576
#define MAX_MAP_VERTS           4194304
#define MAX_MAP_FACES           1048576
#define MAX_MAP_LEAFFACES       1048576
#define MAX_MAP_LEAFBRUSHES     1048576
#define MAX_MAP_EDGES           1048576
#define MAX_MAP_BRUSHSIDES      4194304
#define MAX_MAP_PORTALS         1048576
#define MAX_MAP_SURFEDGES       4194304
#define MAX_MAP_LIGHTING        54525952 // 0x3400000
#define MAX_MAP_VIS_WARNING		0x400000
#define MAX_MAP_VISIBILITY      0x8000000

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================

typedef struct lump_s
{
	int32_t 	fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_AREAS			16
#define	LUMP_AREAPORTALS	17
#define	HEADER_LUMPS		18

typedef struct dheader_s
{
	int32_t 	ident;
	int32_t 	version;
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct dmodel_s
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int32_t 	headnode;
	int32_t 	firstface, numfaces;	// submodels just draw faces
	// without walking the bsp tree
} dmodel_t;


typedef struct dvertex_s
{
	float	point[3];
} dvertex_t;

// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

// planes (x&~1) and (x&~1)+1 are always opposites

typedef struct dplane_s
{
	float	normal[3];
	float	dist;
	int32_t type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// these definitions also need to be in shared.h!

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_MIST			64
#define	LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000

#define	SURF_LIGHT		0x1		// value will hold the light strength

#define	SURF_SLICK		0x2		// effects game physics

#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	// scroll towards angle
#define	SURF_NODRAW		0x80	// don't bother referencing the texture

typedef struct dnode_s
{
	int32_t 	planenum;
	int32_t 	children[2];	// negative numbers are -(leafs+1), not nodes
	float		mins[3];		// for frustum culling
	float		maxs[3];
	uint32_t	firstface;
	uint32_t	numfaces;	// counting both sides
} dnode_t;

#define TEXTURE_NAME_LENGTH			80

typedef struct texinfo_s
{
	float		vecs[2][4];					// [s/t][xyz offset]
	int32_t 	flags;						// miptex flags + overrides
	int32_t 	value;						// light emission, etc
	char		texture[TEXTURE_NAME_LENGTH];	// texture name (textures/*.tga)
	int32_t 	nexttexinfo;				// for animations, -1 = end of chain
} texinfo_t;

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct dedge_s
{
	uint32_t	v[2];		// vertex numbers
} dedge_t;

#define	MAXLIGHTMAPS	4
typedef struct dface_s
{
	uint32_t	planenum;
	int32_t 			side;

	int32_t 			firstedge;		// we must support > 64k edges
	int32_t 			numedges;
	int32_t 			texinfo;

	// lighting info
	uint8_t			styles[MAXLIGHTMAPS];
	int32_t 			lightofs;		// start of [numstyles*surfsize] samples
} dface_t;

typedef struct dleaf_s
{
	int32_t 			contents;			// OR of all brushes (not needed?)

	int32_t 			cluster;
	int32_t 			area;

	float			mins[3];			// for frustum culling
	float			maxs[3];

	uint32_t	firstleafface;
	uint32_t	numleaffaces;

	uint32_t	firstleafbrush;
	uint32_t	numleafbrushes;
} dleaf_t;

typedef struct dbrushside_s
{
	uint32_t	planenum;		// facing out of the leaf
	int32_t		texinfo;
} dbrushside_t;

typedef struct dbrush_s
{
	int32_t 		firstside;
	int32_t 		numsides;
	int32_t 		contents;
} dbrush_t;

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
typedef struct dvis_s
{
	int32_t 		numclusters;
	int32_t 		bitofs[8][2];	// bitofs[numclusters][2]
} dvis_t;

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
typedef struct dareaportal_s
{
	int32_t 	portalnum;
	int32_t 	otherarea;
} dareaportal_t;

typedef struct darea_s
{
	int32_t 	numareaportals;
	int32_t 	firstareaportal;
} darea_t;
