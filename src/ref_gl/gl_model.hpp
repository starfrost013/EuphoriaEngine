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

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/

//
// in memory representation
//
typedef struct mvertex_s
{
	vec3_t		position;
} mvertex_t;

typedef struct mmodel_s
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	float		radius;
	int32_t		headnode;
	int32_t		visleafs;		// not including the solid leaf 0
	int32_t		firstface, numfaces;
} mmodel_t;

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWTURB		0x10
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	uint32_t	v[2];
	uint32_t	cachededgeoffset;
} medge_t;

typedef struct mtexinfo_s
{
	float				vecs[2][4];
	int32_t				flags;
	int32_t				numframes;
	struct mtexinfo_s*	next;		// animation chain
	image_t*			image;
} mtexinfo_t;

#define	VERTEXSIZE	7

typedef struct glpoly_s
{
	struct	glpoly_s*	next;
	struct	glpoly_s*	chain;
	int32_t				numverts;
	int32_t				flags;					// for SURF_UNDERWATER (not needed anymore?)
	float				verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
} glpoly_t;

typedef struct msurface_s
{
	int32_t				visframe;		// should be drawn when node is crossed

	cplane_t*			plane;
	int32_t				flags;

	int32_t				firstedge;	// look up in model->surfedges[], negative numbers
	int32_t				numedges;	// are backwards edges
	
	short				texturemins[2];
	short				extents[2];

	int32_t				light_s, light_t;	// gl lightmap coordinates
	int32_t				dlight_s, dlight_t; // gl lightmap coordinates for dynamic lightmaps

	glpoly_t*			polys;				// multiple if warped
	struct msurface_s*	texturechain;
	struct msurface_s*	lightmapchain;

	mtexinfo_t	*texinfo;
	
// lighting info
	int32_t				dlightframe;
	int32_t				dlightbits;

	int32_t				lightmaptexturenum;
	uint8_t				styles[MAXLIGHTMAPS];
	float				cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	uint8_t*			samples;		// [numstyles*surfsize]
} msurface_t;

typedef struct mnode_s
{
// common with leaf
	int32_t			contents;		// -1, to differentiate from leafs
	int32_t			visframe;		// node needs to be traversed if current
	
	float			minmaxs[6];		// for bounding box culling

	struct mnode_s*	parent;

// node specific
	cplane_t*		plane;
	struct mnode_s*	children[2];	

	uint32_t		firstsurface;
	uint32_t		numsurfaces;
} mnode_t;

typedef struct mleaf_s
{
// common with node
	int32_t			contents;		// wil be a negative contents number
	int32_t			visframe;		// node needs to be traversed if current

	float			minmaxs[6];		// for bounding box culling

	struct mnode_s*	parent;

// leaf specific
	int32_t			cluster;
	int32_t			area;

	msurface_t**	firstmarksurface;
	int32_t			nummarksurfaces;
} mleaf_t;


//===================================================================

//
// Whole model
//

typedef enum { mod_bad, mod_brush, mod_sprite, mod_alias } modtype_t;

typedef struct model_s
{
	char			name[MAX_QPATH];

	int32_t			registration_sequence;

	modtype_t		type;
	int32_t			numframes;
	
	int32_t			flags;

//
// volume occupied by the model graphics
//		
	vec3_t			mins, maxs;
	float			radius;

//
// solid volume for clipping 
//
	bool			clipbox;
	vec3_t			clipmins, clipmaxs;

//
// brush model
//
	int32_t			firstmodelsurface, nummodelsurfaces;
	int32_t			ightmap;		// only for submodels

	int32_t			numsubmodels;
	mmodel_t*		submodels;

	int32_t			numplanes;
	cplane_t		*planes;

	int32_t			numleafs;		// number of visible leafs, not counting 0
	mleaf_t*		leafs;

	int32_t			numvertexes;
	mvertex_t*		vertexes;

	int32_t			numedges;
	medge_t*		edges;

	int32_t			numnodes;
	int32_t			firstnode;
	mnode_t*		nodes;

	int32_t			numtexinfo;
	mtexinfo_t*		texinfo;

	int32_t			numsurfaces;
	msurface_t*		surfaces;

	int32_t			numsurfedges;
	int32_t*		surfedges;

	int32_t			nummarksurfaces;
	msurface_t**	marksurfaces;

	dvis_t*			vis;

	uint8_t*		lightdata;

	// for alias models and skins
	image_t*		skins[MAX_MD2SKINS];

	int32_t			extradatasize;
	void*			extradata;
} model_t;

//============================================================================

void		Mod_Init();
model_t*	Mod_ForName(const char *name, bool crash);
mleaf_t*	Mod_PointInLeaf(vec3_t p, model_t *model);
uint8_t*	Mod_ClusterPVS(int32_t cluster, model_t *model);

void		Mod_Modellist_f();

void*		Memory_HunkBegin(int32_t maxsize);
void*		Memory_HunkAlloc(int32_t size);
int32_t		Memory_HunkEnd();
void		Memory_HunkFree(void *base);

void		Mod_FreeAll();
void		Mod_Free(model_t *mod);
