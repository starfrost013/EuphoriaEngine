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
// cmodel.c -- model loading

#include "common.hpp"

typedef struct
{
	cplane_t		*plane;
	int32_t 		children[2];		// negative numbers are leafs
} cnode_t;

typedef struct
{
	cplane_t		*plane;
	mapsurface_t	*surface;
} cbrushside_t;

typedef struct
{
	int32_t 		contents;
	int32_t 		cluster;
	int32_t 		area;
	uint32_t		firstleafbrush;
	uint32_t		numleafbrushes;
} cleaf_t;

typedef struct
{
	int32_t 		contents;
	int32_t 		numsides;
	int32_t 		firstbrushside;
	int32_t 		checkcount;		// to avoid repeated testings
} cbrush_t;

typedef struct
{
	int32_t 	numareaportals;
	int32_t 	firstareaportal;
	int32_t 	floodnum;			// if two areas have equal floodnums, they are connected
	int32_t 	floodvalid;
} carea_t;

int32_t 		checkcount;

char			map_name[MAX_QPATH];

int32_t 		numbrushsides;
cbrushside_t	map_brushsides[MAX_MAP_BRUSHSIDES];

int32_t 		numtexinfo;
mapsurface_t	map_surfaces[MAX_MAP_TEXINFO];

int32_t 		numplanes;
cplane_t		map_planes[MAX_MAP_PLANES+6];		// extra for box hull

int32_t 		numnodes;
cnode_t			map_nodes[MAX_MAP_NODES+6];		// extra for box hull

int32_t 		numleafs = 1;	// allow leaf funcs to be called without a map
cleaf_t			map_leafs[MAX_MAP_LEAFS];
int32_t 		emptyleaf, solidleaf;

int32_t 		numleafbrushes;
uint32_t		map_leafbrushes[MAX_MAP_LEAFBRUSHES];

int32_t 		numcmodels;
cmodel_t		map_cmodels[MAX_MAP_MODELS];

int32_t 		numbrushes;
cbrush_t		map_brushes[MAX_MAP_BRUSHES];

int32_t 		numvisibility;
uint8_t			map_visibility[MAX_MAP_VISIBILITY];
dvis_t*			map_vis = (dvis_t *)map_visibility;

int32_t 		numentitychars;
char			map_entitystring[MAX_MAP_ENTSTRING];

int32_t 		numareas = 1;
carea_t			map_areas[MAX_MAP_AREAS];

int32_t 		numareaportals;
dareaportal_t	map_areaportals[MAX_MAP_AREAPORTALS];

int32_t 		numclusters = 1;

mapsurface_t	nullsurface;

int32_t 		floodvalid;

bool			portalopen[MAX_MAP_AREAPORTALS];

cvar_t*			map_noareas;

void	Map_InitBoxHull ();
void	Map_FloodAreaConnections ();


int32_t 	c_pointcontents;
int32_t 	c_traces, c_brush_traces;


/*
===============================================================================

					MAP LOADING

===============================================================================
*/

uint8_t* map_base;

/*
=================
Map_LoadSubmodels
=================
*/
void Map_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in;
	cmodel_t	*out;
	int32_t 	i, j, count;

	in = (dmodel_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadSubmodels: incorrect submodel lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no models");
	if (count > MAX_MAP_MODELS)
		Com_Error (ERR_DROP, "Map has too many models");

	numcmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &map_cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->headnode = LittleInt (in->headnode);
	}
}


/*
=================
Map_LoadSurfaces
=================
*/
void Map_LoadSurfaces (lump_t *l)
{
	texinfo_t*		in;
	mapsurface_t*	out;
	int32_t 		i, count;

	in = (texinfo_t *)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadSurfaces: incorrect surface lump size");
	count = l->filelen / sizeof(*in);
	if (count < 1)
		Com_Error (ERR_DROP, "Map with no surfaces");
	if (count > MAX_MAP_TEXINFO)
		Com_Error (ERR_DROP, "Map has too many surfaces");

	numtexinfo = count;
	out = map_surfaces;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		strncpy (out->c.name, in->texture, sizeof(out->c.name)-1);
		strncpy (out->rname, in->texture, sizeof(out->rname)-1);
		out->c.flags = LittleInt (in->flags);
		out->c.value = LittleInt (in->value);
	}
}


/*
=================
Map_LoadNodes

=================
*/
void Map_LoadNodes (lump_t *l)
{
	dnode_t*	in;
	int32_t		child;
	cnode_t*	out;
	int32_t 	i, j, count;
	
	in = (dnode_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadNodes: incorrect nodes lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map has no nodes");
	if (count > MAX_MAP_NODES)
		Com_Error (ERR_DROP, "Map has too many nodes");

	out = map_nodes;

	numnodes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->plane = map_planes + LittleInt(in->planenum);
		for (j=0 ; j<2 ; j++)
		{
			child = LittleInt (in->children[j]);
			out->children[j] = child;
		}
	}

}

/*
=================
Map_LoadBrushes

=================
*/
void Map_LoadBrushes (lump_t *l)
{
	dbrush_t*	in;
	cbrush_t*	out;
	int32_t 	i, count;
	
	in = (dbrush_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadBrushes: incorrect brushes lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_BRUSHES)
		Com_Error (ERR_DROP, "Map has too many brushes");

	out = map_brushes;

	numbrushes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->firstbrushside = LittleInt(in->firstside);
		out->numsides = LittleInt(in->numsides);
		out->contents = LittleInt(in->contents);
	}

}

/*
=================
Map_LoadLeafs
=================
*/
void Map_LoadLeafs (lump_t *l)
{
	int32_t i;
	cleaf_t* out;
	dleaf_t* in;
	int32_t count;
	
	in = (dleaf_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadLeafs: incorrect leafs lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no leafs");
	// need to save space for box planes
	if (count > MAX_MAP_PLANES)
		Com_Error (ERR_DROP, "Map has too many planes");

	out = map_leafs;	
	numleafs = count;
	numclusters = 0;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->contents = LittleInt (in->contents);
		out->cluster = LittleInt (in->cluster);
		out->area = LittleInt (in->area);
		out->firstleafbrush = LittleIntUnsigned (in->firstleafbrush);
		out->numleafbrushes = LittleIntUnsigned (in->numleafbrushes);

		if (out->cluster >= numclusters)
			numclusters = out->cluster + 1;
	}

	if (map_leafs[0].contents != CONTENTS_SOLID)
		Com_Error (ERR_DROP, "Map leaf 0 is not CONTENTS_SOLID");
	solidleaf = 0;
	emptyleaf = -1;
	for (i=1 ; i<numleafs ; i++)
	{
		if (!map_leafs[i].contents)
		{
			emptyleaf = i;
			break;
		}
	}
	if (emptyleaf == -1)
		Com_Error (ERR_DROP, "Map does not have an empty leaf");
}

/*
=================
Map_LoadPlanes
=================
*/
void Map_LoadPlanes (lump_t *l)
{
	int32_t		i, j;
	cplane_t*	out;
	dplane_t*	in;
	int32_t 	count;
	int32_t 	bits;
	
	in = (dplane_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadPlanes: incorrect planes lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no planes");
	// need to save space for box planes
	if (count > MAX_MAP_PLANES)
		Com_Error (ERR_DROP, "Map has too many planes");

	out = map_planes;	
	numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleInt (in->type);
		out->signbits = bits;
	}
}

/*
=================
Map_LoadLeafBrushes
=================
*/
void Map_LoadLeafBrushes (lump_t *l)
{
	int32_t 		i;
	uint32_t	*out;
	uint32_t 	*in;
	int32_t 		count;
	
	in = (uint32_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadLeafBrushes: incorrect leafbrushes lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no planes");
	// need to save space for box planes
	if (count > MAX_MAP_LEAFBRUSHES)
		Com_Error (ERR_DROP, "Map has too many leafbrushes");

	out = map_leafbrushes;
	numleafbrushes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
		*out = LittleInt (*in);
}

/*
=================
Map_LoadBrushSides
=================
*/
void Map_LoadBrushSides (lump_t *l)
{
	int32_t 		i, j;
	cbrushside_t	*out;
	dbrushside_t 	*in;
	int32_t 		count;
	uint32_t	num;

	in = (dbrushside_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadBrushSides: incorrect brushsides lump size");
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > MAX_MAP_BRUSHSIDES)
		Com_Error (ERR_DROP, "Map has too many planes");

	out = map_brushsides;	
	numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = LittleIntUnsigned (in->planenum);
		out->plane = &map_planes[num];
		j = LittleInt (in->texinfo);
		if (j >= numtexinfo)
			Com_Error (ERR_DROP, "Bad brushside texinfo");
		out->surface = &map_surfaces[j];
	}
}

/*
=================
Map_LoadAreas
=================
*/
void Map_LoadAreas (lump_t *l)
{
	int32_t i;
	carea_t* out;
	darea_t* in;
	int32_t count;

	in = (darea_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadAreas: incorrect areas lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_AREAS)
		Com_Error (ERR_DROP, "Map has too many areas");

	out = map_areas;
	numareas = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->numareaportals = LittleInt (in->numareaportals);
		out->firstareaportal = LittleInt (in->firstareaportal);
		out->floodvalid = 0;
		out->floodnum = 0;
	}
}

/*
=================
Map_LoadAreaPortals
=================
*/
void Map_LoadAreaPortals (lump_t *l)
{
	int32_t 		i;
	dareaportal_t*	out;
	dareaportal_t* 	in;
	int32_t 		count;

	in = (dareaportal_t*)(void *)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "Map_LoadAreaPortals: incorrect areaportals lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_AREAS)
		Com_Error (ERR_DROP, "Map has too many areas");

	out = map_areaportals;
	numareaportals = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->portalnum = LittleInt (in->portalnum);
		out->otherarea = LittleInt (in->otherarea);
	}
}

/*
=================
Map_LoadVisibility
=================
*/
void Map_LoadVisibility (lump_t *l)
{
	int32_t 	i;

	numvisibility = l->filelen;
	if (l->filelen > MAX_MAP_VISIBILITY)
		Com_Error (ERR_DROP, "Map has too large visibility lump (add more detail brushes)");

	if (l->filelen > MAX_MAP_VIS_WARNING)
	{
		Com_DPrintf("Warning: Very large visibility lump (0x%X bytes, max 0x%X). Add more detail brushes\nto exclude complicated geometry from VIS, or use less wide open spaces.", l->filelen, MAX_MAP_VISIBILITY);
	}

	memcpy (map_visibility, map_base + l->fileofs, l->filelen);

	map_vis->numclusters = LittleInt (map_vis->numclusters);
	for (i=0 ; i<map_vis->numclusters ; i++)
	{
		map_vis->bitofs[i][0] = LittleInt (map_vis->bitofs[i][0]);
		map_vis->bitofs[i][1] = LittleInt (map_vis->bitofs[i][1]);
	}
}


/*
=================
Map_LoadEntityString
=================
*/
void Map_LoadEntityString (lump_t *l)
{
	numentitychars = l->filelen;
	if (l->filelen > MAX_MAP_ENTSTRING)
		Com_Error (ERR_DROP, "Map has too large entity lump");

	memcpy (map_entitystring, map_base + l->fileofs, l->filelen);
}



/*
==================
Map_Load

Loads in the map and all submodels
==================
*/
cmodel_t* Map_Load(const char *name, bool clientload, uint32_t *checksum)
{
	uint32_t		*buf;
	int32_t 		i;
	dheader_t		header;
	int32_t 		length;
	static uint32_t	last_checksum;

	map_noareas = Cvar_Get ("map_noareas", "0", 0);

	if (  !strcmp (map_name, name) && (clientload || !Cvar_VariableValue ("flushmap")) )
	{
		*checksum = last_checksum;
		if (!clientload)
		{
			memset (portalopen, 0, sizeof(portalopen));
			Map_FloodAreaConnections ();
		}
		return &map_cmodels[0];		// still have the right version
	}

	// free old stuff
	numplanes = 0;
	numnodes = 0;
	numleafs = 0;
	numcmodels = 0;
	numvisibility = 0;
	numentitychars = 0;
	map_entitystring[0] = 0;
	map_name[0] = 0;

	if (!name || !name[0])
	{
		numleafs = 1;
		numclusters = 1;
		numareas = 1;
		*checksum = 0;
		return &map_cmodels[0];
	}

	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buf);
	if (!buf)
		Com_Error (ERR_DROP, "Couldn't load %s", name);

	last_checksum = LittleInt (Com_BlockChecksum (buf, length));
	*checksum = last_checksum;

	header = *(dheader_t *)buf;
	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int32_t *)&header)[i] = LittleInt ( ((int32_t *)&header)[i]);

	if (header.version != ZBSP_VERSION)
		Com_Error (ERR_DROP, "Map_Load: %s has wrong version number (%i should be %i)"
		, name, header.version, ZBSP_VERSION);

	map_base = (uint8_t *)buf;

	// load into heap
	Map_LoadSurfaces (&header.lumps[LUMP_TEXINFO]);
	Map_LoadLeafs (&header.lumps[LUMP_LEAFS]);
	Map_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES]);
	Map_LoadPlanes (&header.lumps[LUMP_PLANES]);
	Map_LoadBrushes (&header.lumps[LUMP_BRUSHES]);
	Map_LoadBrushSides (&header.lumps[LUMP_BRUSHSIDES]);
	Map_LoadSubmodels (&header.lumps[LUMP_MODELS]);
	Map_LoadNodes (&header.lumps[LUMP_NODES]);
	Map_LoadAreas (&header.lumps[LUMP_AREAS]);
	Map_LoadAreaPortals (&header.lumps[LUMP_AREAPORTALS]);
	Map_LoadVisibility (&header.lumps[LUMP_VISIBILITY]);
	Map_LoadEntityString (&header.lumps[LUMP_ENTITIES]);

	FS_FreeFile (buf);

	Map_InitBoxHull ();

	memset (portalopen, 0, sizeof(portalopen));
	Map_FloodAreaConnections ();

	strcpy (map_name, name);

	return &map_cmodels[0];
}

/*
==================
Map_InlineModel
==================
*/
cmodel_t* Map_LoadInlineModel(const char* name)
{
	int32_t num;

	if (!name || name[0] != '*')
		Com_Error (ERR_DROP, "Map_InlineModel: bad name");
	num = atoi (name+1);
	if (num < 1 || num >= numcmodels)
		Com_Error (ERR_DROP, "Map_InlineModel: bad number");

	return &map_cmodels[num];
}

int32_t Map_GetNumClusters()
{
	return numclusters;
}

int32_t Map_NumInlineModels()
{
	return numcmodels;
}

char* Map_GetEntityString()
{
	return map_entitystring;
}

int32_t Map_GetLeafContents (int32_t leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		Com_Error (ERR_DROP, "Map_LeafContents: bad number");

	return map_leafs[leafnum].contents;
}

int32_t Map_GetLeafCluster (int32_t leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		Com_Error (ERR_DROP, "Map_LeafCluster: bad number");

	return map_leafs[leafnum].cluster;
}

int32_t Map_LeafArea (int32_t leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		Com_Error (ERR_DROP, "Map_LeafArea: bad number");

	return map_leafs[leafnum].area;
}

//=======================================================================


cplane_t*	box_planes;
int32_t 	box_headnode;
cbrush_t*	box_brush;
cleaf_t*	box_leaf;

/*
===================
Map_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void Map_InitBoxHull ()
{
	int32_t 		i;
	int32_t 		side;
	cnode_t		*c;
	cplane_t	*p;
	cbrushside_t	*s;

	box_headnode = numnodes;
	box_planes = &map_planes[numplanes];
	if (numnodes+6 > MAX_MAP_NODES
		|| numbrushes+1 > MAX_MAP_BRUSHES
		|| numleafbrushes+1 > MAX_MAP_LEAFBRUSHES
		|| numbrushsides+6 > MAX_MAP_BRUSHSIDES
		|| numplanes+12 > MAX_MAP_PLANES)
		Com_Error (ERR_DROP, "Not enough room for box tree");

	box_brush = &map_brushes[numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = numbrushsides;
	box_brush->contents = CONTENTS_MONSTER;

	box_leaf = &map_leafs[numleafs];
	box_leaf->contents = CONTENTS_MONSTER;
	box_leaf->firstleafbrush = numleafbrushes;
	box_leaf->numleafbrushes = 1;

	map_leafbrushes[numleafbrushes] = numbrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &map_brushsides[numbrushsides+i];
		s->plane = 	map_planes + (numplanes+i*2+side);
		s->surface = &nullsurface;

		// nodes
		c = &map_nodes[box_headnode+i];
		c->plane = map_planes + (numplanes+i*2);
		c->children[side] = -1 - emptyleaf;
		if (i != 5)
			c->children[side^1] = box_headnode+i + 1;
		else
			c->children[side^1] = -1 - numleafs;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear3 (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear3 (p->normal);
		p->normal[i>>1] = -1;
	}	
}


/*
===================
Map_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int32_t Map_HeadnodeForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}


/*
==================
Map_PointLeafnum_r

==================
*/
int32_t Map_PointLeafnum_r (vec3_t p, int32_t num)
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		node = map_nodes + num;
		plane = node->plane;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct3 (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

int32_t Map_PointLeafnum (vec3_t p)
{
	if (!numplanes)
		return 0;		// sound may call this without map loaded
	return Map_PointLeafnum_r (p, 0);
}

/*
=============
Map_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int32_t 	leaf_count;
int32_t		leaf_maxcount;
int32_t* 	leaf_list;
float*		map_mins;
float*		map_maxs;
int32_t 	leaf_topnode;

void MapRenderer_BoxLeafnums_r (int32_t nodenum)
{
	cplane_t	*plane;
	cnode_t		*node;
	int32_t 	s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
				Com_DPrintf ("Map_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}
	
		node = &map_nodes[nodenum];
		plane = node->plane;
		s = BOX_ON_PLANE_SIDE(map_mins, map_maxs, plane);
		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			MapRenderer_BoxLeafnums_r (node->children[0]);
			nodenum = node->children[1];
		}

	}
}

// ============================
// Map_BoxLeafnums start (at head leaf)
// ============================
int32_t MapRenderer_BoxLeafnums_headnode (vec3_t mins, vec3_t maxs, int32_t *list, int32_t listsize, int32_t headnode, int32_t *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	map_mins = mins;
	map_maxs = maxs;

	leaf_topnode = -1;

	MapRenderer_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int32_t Map_BoxLeafnums (vec3_t mins, vec3_t maxs, int32_t *list, int32_t listsize, int32_t *topnode)
{
	return MapRenderer_BoxLeafnums_headnode (mins, maxs, list,
		listsize, map_cmodels[0].headnode, topnode);
}

/*
==================
Map_PointContents
==================
*/
int32_t Map_PointContents (vec3_t p, int32_t headnode)
{
	int32_t 	l;

	if (!numnodes)	// map not loaded
		return 0;

	l = Map_PointLeafnum_r (p, headnode);

	return map_leafs[l].contents;
}

/*
==================
Map_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int32_t Map_TransformedPointContents (vec3_t p, int32_t headnode, vec3_t origin, vec3_t angles)
{
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;
	int32_t 		l;

	// subtract origin offset
	VectorSubtract3 (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && 
	(angles[0] || angles[1] || angles[2]) )
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy3 (p_l, temp);
		p_l[0] = DotProduct3 (temp, forward);
		p_l[1] = -DotProduct3 (temp, right);
		p_l[2] = DotProduct3 (temp, up);
	}

	l = Map_PointLeafnum_r (p_l, headnode);

	return map_leafs[l].contents;
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	0.03125f

vec3_t	trace_start, trace_end;
vec3_t	trace_mins, trace_maxs;
vec3_t	trace_extents;

trace_t	trace_trace;
int32_t 	trace_contents;
bool	trace_ispoint;		// optimized case

/*
================
Map_ClipBoxToBrush
================
*/
void Map_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
					  trace_t *trace, cbrush_t *brush)
{
	int32_t 		i, j;
	cplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	bool	getout, startout;
	float		f;
	cbrushside_t	*side, *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct3 (ofs, plane->normal);
			dist = plane->dist - dist;
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct3 (p1, plane->normal) - dist;
		d2 = DotProduct3 (p2, plane->normal) - dist;

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1 && enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = &(leadside->surface->c);
			trace->contents = brush->contents;
		}
	}
}

/*
================
Map_TestBoxInBrush
================
*/
void Map_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1,
					  trace_t *trace, cbrush_t *brush)
{
	int32_t 		i, j;
	cplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = DotProduct3 (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct3 (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;

	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
Map_TraceToLeaf
================
*/
void Map_TraceToLeaf (int32_t leafnum)
{
	int32_t 		k;
	int32_t 		brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		Map_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
================
Map_TestInLeaf
================
*/
void Map_TestInLeaf (int32_t leafnum)
{
	int32_t 	k;
	int32_t 	brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		Map_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
==================
Map_RecursiveHullCheck
==================
*/
void Map_RecursiveHullCheck (int32_t num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	cnode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int32_t 	i;
	vec3_t		mid;
	int32_t 	side;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		Map_TraceToLeaf (-1-num);
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = map_nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct3 (plane->normal, p1) - plane->dist;
		t2 = DotProduct3 (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabsf(trace_extents[0]*plane->normal[0]) +
				fabsf(trace_extents[1]*plane->normal[1]) +
				fabsf(trace_extents[2]*plane->normal[2]);
	}

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		Map_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		Map_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0f/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0f/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;
		
	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	Map_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);


	// go past the node
	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;
		
	midf = p1f + (p2f - p1f)*frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	Map_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}

//======================================================================

/*
==================
Map_BoxTrace
==================
*/
trace_t		Map_BoxTrace (vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int32_t headnode, int32_t brushmask)
{
	int32_t 	i;

	checkcount++;		// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &(nullsurface.c);

	if (!numnodes)	// map not loaded
		return trace_trace;

	trace_contents = brushmask;
	VectorCopy3 (start, trace_start);
	VectorCopy3 (end, trace_end);
	VectorCopy3 (mins, trace_mins);
	VectorCopy3 (maxs, trace_maxs);

	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int32_t 	leafs[1024];
		int32_t 	i, numleafs;
		vec3_t	c1, c2;
		int32_t 	topnode;

		VectorAdd3 (start, mins, c1);
		VectorAdd3 (start, maxs, c2);
		for (i=0 ; i<3 ; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		numleafs = MapRenderer_BoxLeafnums_headnode (c1, c2, leafs, 1024, headnode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
			Map_TestInLeaf (leafs[i]);
			if (trace_trace.allsolid)
				break;
		}
		VectorCopy3 (start, trace_trace.endpos);
		return trace_trace;
	}

	//
	// check for point special case
	//
	if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0
		&& maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
	{
		trace_ispoint = true;
		VectorClear3 (trace_extents);
	}
	else
	{
		trace_ispoint = false;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	//
	// general sweeping through world
	//
	Map_RecursiveHullCheck (headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1)
	{
		VectorCopy3 (end, trace_trace.endpos);
	}
	else
	{
		for (i=0 ; i<3 ; i++)
			trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}
	return trace_trace;
}


/*
==================
Map_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
//#ifdef _WIN32
//#pragma optimize( "", off )
//#endif


trace_t		Map_TransformedBoxTrace (vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int32_t headnode, int32_t brushmask,
						  vec3_t origin, vec3_t angles)
{
	trace_t	trace;
	vec3_t	start_l, end_l;
	vec3_t	a;
	vec3_t	forward, right, up;
	vec3_t	temp;
	bool	rotated;

	// subtract origin offset
	VectorSubtract3 (start, origin, start_l);
	VectorSubtract3 (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && 
	(angles[0] || angles[1] || angles[2]) )
		rotated = true;
	else
		rotated = false;

	if (rotated)
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy3 (start_l, temp);
		start_l[0] = DotProduct3 (temp, forward);
		start_l[1] = -DotProduct3 (temp, right);
		start_l[2] = DotProduct3 (temp, up);

		VectorCopy3 (end_l, temp);
		end_l[0] = DotProduct3 (temp, forward);
		end_l[1] = -DotProduct3 (temp, right);
		end_l[2] = DotProduct3 (temp, up);
	}

	// sweep the box through the model
	trace = Map_BoxTrace (start_l, end_l, mins, maxs, headnode, brushmask);

	if (rotated && trace.fraction != 1.0)
	{
		// FIXME: figure out how to do this with existing angles
		VectorNegate3 (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy3 (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct3 (temp, forward);
		trace.plane.normal[1] = -DotProduct3 (temp, right);
		trace.plane.normal[2] = DotProduct3 (temp, up);
	}

	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	return trace;
}

//#ifdef _WIN32
//#pragma optimize( "", on )
//#endif

/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
Map_DecompressVis
===================
*/
void Map_DecompressVis (uint8_t *in, uint8_t *out)
{
	int32_t 	c;
	uint8_t* out_p;
	int32_t 	row;

	row = (numclusters+7)>>3;	
	out_p = out;

	if (!in || !numvisibility)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;		
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			Com_DPrintf ("warning: Vis decompression overrun\n");
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

uint8_t	pvsrow[MAX_MAP_LEAFS/8];

uint8_t	phsrow[MAX_MAP_LEAFS/8];

uint8_t* Map_ClusterPVS (int32_t cluster)
{
	if (cluster == -1)
		memset (pvsrow, 0, (numclusters+7)>>3);
	else
		Map_DecompressVis (map_visibility + map_vis->bitofs[cluster][DVIS_PVS], pvsrow);
	return pvsrow;
}

uint8_t* Map_ClusterPHS (int32_t cluster)
{
	if (cluster == -1)
		memset (phsrow, 0, (numclusters+7)>>3);
	else
		Map_DecompressVis (map_visibility + map_vis->bitofs[cluster][DVIS_PHS], phsrow);
	return phsrow;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void Map_FloodArea_r (carea_t *area, int32_t floodnum)
{
	int32_t 	i;
	dareaportal_t	*p;

	if (area->floodvalid == floodvalid)
	{
		if (area->floodnum == floodnum)
			return;
		Com_Error (ERR_DROP, "FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	p = &map_areaportals[area->firstareaportal];
	for (i=0 ; i<area->numareaportals ; i++, p++)
	{
		if (portalopen[p->portalnum])
			Map_FloodArea_r (&map_areas[p->otherarea], floodnum);
	}
}

/*
====================
FloodAreaConnections

Flood fills connections between map areas
====================
*/
void	Map_FloodAreaConnections ()
{
	int32_t i;
	carea_t	*area;
	int32_t floodnum;

	// all current floods are now invalid
	floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=1 ; i<numareas ; i++)
	{
		area = &map_areas[i];
		if (area->floodvalid == floodvalid)
			continue;		// already flooded into
		floodnum++;
		Map_FloodArea_r (area, floodnum);
	}

}

void	Map_SetAreaPortalState (int32_t portalnum, bool open)
{
	if (portalnum > numareaportals)
		Com_Error (ERR_DROP, "areaportal > numareaportals");

	portalopen[portalnum] = open;
	Map_FloodAreaConnections ();
}

bool	Map_AreasConnected (int32_t area1, int32_t area2)
{
	if (map_noareas->value)
		return true;

	if (area1 > numareas || area2 > numareas)
		Com_Error (ERR_DROP, "area > numareas");

	if (map_areas[area1].floodnum == map_areas[area2].floodnum)
		return true;
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int32_t Map_WriteAreaBits (uint8_t *buffer, int32_t area)
{
	int32_t 	i;
	int32_t 	floodnum;
	int32_t 	bytes;

	bytes = (numareas+7)>>3;

	if (map_noareas->value)
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		memset (buffer, 0, bytes);

		floodnum = map_areas[area].floodnum;
		for (i=0 ; i<numareas ; i++)
		{
			if (map_areas[i].floodnum == floodnum || !area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void	Map_WritePortalState (FILE *f)
{
	fwrite (portalopen, sizeof(portalopen), 1, f);
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void	Map_ReadPortalState (FILE *f)
{
	FS_Read (portalopen, sizeof(portalopen), f);
	Map_FloodAreaConnections ();
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
bool Map_HeadnodeVisible (int32_t nodenum, uint8_t *visbits)
{
	int32_t 	leafnum;
	int32_t 	cluster;
	cnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = map_leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &map_nodes[nodenum];
	if (Map_HeadnodeVisible(node->children[0], visbits))
		return true;
	return Map_HeadnodeVisible(node->children[1], visbits);
}

