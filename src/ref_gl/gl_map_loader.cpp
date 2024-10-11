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
// models.c -- model loading and caching (OpenGL side)

#include "gl_local.hpp"

model_t* loadmodel;
int32_t	modfilelen;

float	map_radius;

void Mod_LoadSpriteModel(model_t* mod, void* buffer);
void MapRenderer_Load(model_t* mod, void* buffer);
void Mod_LoadAliasModel(model_t* mod, void* buffer);
void MapRenderer_BoxLeafnums_r(int32_t nodenum);

uint8_t	mod_novis[MAX_MAP_LEAFS / 8];

#define	MAX_MOD_KNOWN	512
model_t	mod_known[MAX_MOD_KNOWN];
int32_t	mod_numknown;

// the inline * models from the current map are kept seperate
model_t	mod_inline[MAX_MOD_KNOWN];

int32_t	registration_sequence;

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t* Mod_PointInLeaf(vec3_t p, model_t* model)
{
	mnode_t* node;
	float		d;
	cplane_t* plane;

	if (!model || !model->nodes)
		ri.Sys_Error(ERR_DROP, "Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents != -1)
			return (mleaf_t*)node;
		plane = node->plane;
		d = DotProduct3(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
uint8_t* Mod_DecompressVis(uint8_t* in, model_t* model)
{
	static byte	decompressed[MAX_MAP_LEAFS / 8];
	int32_t		c;
	uint8_t* out;
	int32_t		row;

	row = (model->vis->numclusters + 7) >> 3;
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

/*
==============
Mod_ClusterPVS
==============
*/
uint8_t* Mod_ClusterPVS(int32_t cluster, model_t* model)
{
	if (cluster == -1 || !model->vis)
		return mod_novis;
	return Mod_DecompressVis((uint8_t*)model->vis + model->vis->bitofs[cluster][DVIS_PVS],
		model);
}


//===============================================================================

/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f()
{
	int32_t	i;
	model_t* mod;
	int32_t	total;

	total = 0;
	ri.Con_Printf(PRINT_ALL, "Loaded models:\n");
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		ri.Con_Printf(PRINT_ALL, "%8i : %s\n", mod->extradatasize, mod->name);
		total += mod->extradatasize;
	}
	ri.Con_Printf(PRINT_ALL, "Total resident: %i\n", total);
}

/*
===============
Mod_Init
===============
*/
void Mod_Init()
{
	memset(mod_novis, 0xff, sizeof(mod_novis));
}


/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t* Mod_ForName(const char* name, bool crash)
{
	model_t* mod;
	uint32_t* buf;
	int32_t		i;

	if (!name[0])
		ri.Sys_Error(ERR_DROP, "Mod_ForName: NULL name");

	//
	// inline models are grabbed only from worldmodel
	//
	if (name[0] == '*')
	{
		i = atoi(name + 1);
		if (i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels)
			ri.Sys_Error(ERR_DROP, "bad inline model number");
		return &mod_inline[i];
	}

	//
	// search the currently loaded models
	//
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (!strcmp(mod->name, name))
			return mod;
	}

	//
	// find a free model slot spot
	//
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			break;	// free spot
	}
	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
			ri.Sys_Error(ERR_DROP, "mod_numknown == MAX_MOD_KNOWN");
		mod_numknown++;
	}
	strcpy(mod->name, name);

	//
	// load the file
	//
	modfilelen = ri.FS_LoadFile(mod->name, (void**)&buf);
	if (!buf)
	{
		if (crash)
			ri.Sys_Error(ERR_DROP, "Mod_NumForName: %s not found", mod->name);
		memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}

	loadmodel = mod;

	// call the apropriate loader

	switch (LittleInt(*(uint32_t*)buf))
	{
	case IDALIASHEADER:
		loadmodel->extradata = Memory_HunkBegin(MAX_MD2_ALLOC);
		Mod_LoadAliasModel(mod, buf);
		break;

	case IDSPRITEHEADER:
		loadmodel->extradata = Memory_HunkBegin(MAX_SP2_ALLOC);
		Mod_LoadSpriteModel(mod, buf);
		break;

	case ZBSP_HEADER:
		loadmodel->extradata = Memory_HunkBegin(MAX_BSP_ALLOC);
		MapRenderer_Load(mod, buf);
		break;

	default:
		ri.Sys_Error(ERR_DROP, "Mod_NumForName: unknown fileid for %s", mod->name);
		break;
	}

	loadmodel->extradatasize = Memory_HunkEnd();

	ri.FS_FreeFile(buf);

	return mod;
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

uint8_t* map_base;

/*
=================
Mod_LoadLighting
=================
*/
void MapRenderer_LoadLighting(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}

	loadmodel->lightdata = (uint8_t*)Memory_HunkAlloc(l->filelen);
	memcpy(loadmodel->lightdata, map_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVisibility
=================
*/
void MapRenderer_LoadVisibility(lump_t* l)
{
	int32_t	i;

	if (!l->filelen)
	{
		loadmodel->vis = NULL;
		return;
	}
	loadmodel->vis = (dvis_t*)Memory_HunkAlloc(l->filelen);
	memcpy(loadmodel->vis, map_base + l->fileofs, l->filelen);

	loadmodel->vis->numclusters = LittleInt(loadmodel->vis->numclusters);
	for (i = 0; i < loadmodel->vis->numclusters; i++)
	{
		loadmodel->vis->bitofs[i][0] = LittleInt(loadmodel->vis->bitofs[i][0]);
		loadmodel->vis->bitofs[i][1] = LittleInt(loadmodel->vis->bitofs[i][1]);
	}
}


/*
=================
Mod_LoadVertexes
=================
*/
void MapRenderer_LoadVertexes(lump_t* l)
{
	dvertex_t* in;
	mvertex_t* out;
	int32_t	i, count;

	in = (dvertex_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny vertexes lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mvertex_t*)Memory_HunkAlloc(count * sizeof(*out));

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs)
{
	int32_t	i;
	vec3_t	corner = { 0 };

	for (i = 0; i < 3; i++)
	{
		corner[i] = fabsf(mins[i]) > fabsf(maxs[i]) ? fabsf(mins[i]) : fabsf(maxs[i]);
	}

	return VectorLength3(corner);
}


/*
=================
Mod_LoadSubmodels
=================
*/
void MapRenderer_LoadSubmodels(lump_t* l)
{
	dmodel_t* in;
	mmodel_t* out;
	int32_t		i, j, count;

	in = (dmodel_t*)(map_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny submodels lump size in %s", loadmodel->name);
	
	count = l->filelen / sizeof(*in);
	out = (mmodel_t*)Memory_HunkAlloc(count * sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		out->radius = RadiusFromBounds(out->mins, out->maxs);
		out->headnode = LittleInt(in->headnode);
		out->firstface = LittleInt(in->firstface);
		out->numfaces = LittleInt(in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void MapRenderer_LoadEdges(lump_t* l)
{
	dedge_t* in;
	medge_t* out;
	int32_t i, count;

	in = (dedge_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny edges lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (medge_t*)Memory_HunkAlloc((count + 1) * sizeof(*out));

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->v[0] = LittleIntUnsigned(in->v[0]);
		out->v[1] = LittleIntUnsigned(in->v[1]);
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void MapRenderer_LoadTexinfo(lump_t* l)
{
	texinfo_t*	in;
	mtexinfo_t* out, *step;
	int32_t 	i, j, count;
	char		name[MAX_QPATH] = { 0 };
	int32_t		next;

	in = (texinfo_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny texinfo lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mtexinfo_t*)Memory_HunkAlloc(count * sizeof(*out));

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		out->flags = LittleInt(in->flags);
		next = LittleInt(in->nexttexinfo);
		if (next > 0)
			out->next = (mtexinfo_t*)loadmodel->texinfo + next;
		else
			out->next = NULL;
		snprintf(name, sizeof(name), "textures/%s.tga", in->texture);

		out->image = GL_FindImage(name, it_wall);
		if (!out->image)
		{
			ri.Con_Printf(PRINT_ALL, "Couldn't load %s\n", name);
			out->image = r_notexture;
		}
	}

	// count animation frames
	for (i = 0; i < count; i++)
	{
		out = &loadmodel->texinfo[i];
		out->numframes = 1;
		for (step = out->next; step && step != out; step = step->next)
			out->numframes++;
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents(msurface_t* s)
{
	float		mins[2], maxs[2], val;
	int32_t		i, j, e;
	mvertex_t* v;
	mtexinfo_t* tex;
	int32_t		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		e = loadmodel->surfedges[s->firstedge + i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j = 0; j < 2; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0; i < 2; i++)
	{
		bmins[i] = (int32_t)floorf(mins[i] / 16);
		bmaxs[i] = (int32_t)ceilf(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}


void GL_BuildPolygonFromSurface(msurface_t* fa);
void GL_CreateSurfaceLightmap(msurface_t* surf);
void GL_EndBuildingLightmaps();
void GL_BeginBuildingLightmaps(model_t* m);

/*
=================
Mod_LoadFaces
=================
*/
void MapRenderer_LoadFaces(lump_t* l)
{
	dface_t*	in;
	msurface_t* out;
	int32_t		i, count, surfnum;
	int32_t		planenum, side;
	int32_t		ti;

	in = (dface_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny faces lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t*)Memory_HunkAlloc(count * sizeof(*out));

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	currentmodel = loadmodel;

	GL_BeginBuildingLightmaps(loadmodel);

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		out->firstedge = LittleInt(in->firstedge);
		out->numedges = LittleInt(in->numedges);
		out->flags = 0;
		out->polys = NULL;

		planenum = LittleIntUnsigned(in->planenum);
		side = LittleIntUnsigned(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		ti = LittleInt(in->texinfo);
		if (ti < 0 || ti >= loadmodel->numtexinfo)
			ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: bad texinfo number");
		out->texinfo = loadmodel->texinfo + ti;

		CalcSurfaceExtents(out);

		// lighting info

		for (i = 0; i < MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];
		i = LittleInt(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;

		// set the drawing flags

		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;
			for (i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface(out);	// cut up polygon for warps
		}

		// create lightmaps and polygons
		if (!(out->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
			GL_CreateSurfaceLightmap(out);

		if (!(out->texinfo->flags & SURF_WARP))
			GL_BuildPolygonFromSurface(out);

	}

	GL_EndBuildingLightmaps();
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent(mnode_t* node, mnode_t* parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void MapRenderer_LoadNodes(lump_t* l)
{
	int32_t	 i, j, count, p;
	dnode_t* in;
	mnode_t* out;

	in = (dnode_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny nodes lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mnode_t*)Memory_HunkAlloc(count * sizeof(*out));

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleFloat(in->mins[j]);
			out->minmaxs[3 + j] = LittleFloat(in->maxs[j]);
		}

		p = LittleInt(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleIntUnsigned(in->firstface);
		out->numsurfaces = LittleIntUnsigned(in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j = 0; j < 2; j++)
		{
			p = LittleInt(in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t*)(loadmodel->leafs + (-1 - p));
		}
	}

	Mod_SetParent(loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void MapRenderer_LoadLeafs(lump_t* l)
{
	dleaf_t* in;
	mleaf_t* out;
	int32_t	 i, j, count, p;

	in = (dleaf_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny leafs lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mleaf_t*)Memory_HunkAlloc(count * sizeof(*out));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleFloat(in->mins[j]);
			out->minmaxs[3 + j] = LittleFloat(in->maxs[j]);
		}

		p = LittleInt(in->contents);
		out->contents = p;

		out->cluster = LittleInt(in->cluster);
		out->area = LittleInt(in->area);

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleIntUnsigned(in->firstleafface);
		out->nummarksurfaces = LittleIntUnsigned(in->numleaffaces);

	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void MapRenderer_LoadMarksurfaces(lump_t* l)
{
	uint32_t	i, j, count;
	int32_t*	in;
	msurface_t** out;

	in = (int32_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny marksurfaces lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t**)Memory_HunkAlloc(count * sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		j = LittleIntUnsigned(in[i]);
		if (j < 0 || j >= loadmodel->numsurfaces)
			ri.Sys_Error(ERR_DROP, "Mod_ParseMarksurfaces: bad surface number %d/%d", j, loadmodel->numsurfaces);
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void MapRenderer_LoadSurfedges(lump_t* l)
{
	int32_t	 i, count;
	int32_t* in, * out;

	in = (int32_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny surfedges lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	if (count < 1 || count >= MAX_MAP_SURFEDGES)
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: bad surfedges count in %s: %i",
			loadmodel->name, count);

	out = (int32_t*)Memory_HunkAlloc(count * sizeof(*out));

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for (i = 0; i < count; i++)
		out[i] = LittleInt(in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void MapRenderer_LoadPlanes(lump_t* l)
{
	int32_t		i, j;
	cplane_t*	out;
	dplane_t*	in;
	int32_t		count;
	int32_t		bits;

	in = (dplane_t*)(map_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBrushModel: funny planes lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (cplane_t*)Memory_HunkAlloc(count * 2 * sizeof(*out));

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleInt(in->type);
		out->signbits = bits;
	}
}

/*
=================
Mod_LoadBrushModel
=================
*/
void MapRenderer_Load(model_t* mod, void* buffer)
{
	int32_t		i;
	dheader_t* header;
	mmodel_t* bm;

	loadmodel->type = mod_brush;
	if (loadmodel != mod_known)
		ri.Sys_Error(ERR_DROP, "Loaded a brush model after the world");

	header = (dheader_t*)buffer;

	i = LittleInt(header->version);
	if (i != ZBSP_VERSION)
		ri.Sys_Error(ERR_DROP, "MapRenderer_Load: BSP %s has wrong version number (%i should be %i)", mod->name, i, ZBSP_VERSION);

	// swap all the lumps
	map_base = (uint8_t*)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int32_t*)header)[i] = LittleInt(((int32_t*)header)[i]);

	// load into heap

	MapRenderer_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
	MapRenderer_LoadEdges(&header->lumps[LUMP_EDGES]);
	MapRenderer_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
	MapRenderer_LoadLighting(&header->lumps[LUMP_LIGHTING]);
	MapRenderer_LoadPlanes(&header->lumps[LUMP_PLANES]);
	MapRenderer_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
	MapRenderer_LoadFaces(&header->lumps[LUMP_FACES]);
	MapRenderer_LoadMarksurfaces(&header->lumps[LUMP_LEAFFACES]);
	MapRenderer_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	MapRenderer_LoadLeafs(&header->lumps[LUMP_LEAFS]);
	MapRenderer_LoadNodes(&header->lumps[LUMP_NODES]);
	MapRenderer_LoadSubmodels(&header->lumps[LUMP_MODELS]);
	mod->numframes = 2;		// regular and alternate animation

	//
	// set up the submodels
	//
	for (i = 0; i < mod->numsubmodels; i++)
	{
		model_t* starmod;

		bm = &mod->submodels[i];
		starmod = &mod_inline[i];

		*starmod = *loadmodel;

		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces = bm->numfaces;
		starmod->firstnode = bm->headnode;
		if (starmod->firstnode >= loadmodel->numnodes)
			ri.Sys_Error(ERR_DROP, "Inline model %i has bad firstnode", i);

		VectorCopy3(bm->maxs, starmod->maxs);
		VectorCopy3(bm->mins, starmod->mins);
		starmod->radius = bm->radius;

		if (i == 0)
			*loadmodel = *starmod;

		starmod->numleafs = bm->visleafs;
	}

	// for later use by skybox rendering code
	map_radius = loadmodel->radius;
}

//=============================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration(const char* model)
{
	char	fullname[MAX_QPATH];
	cvar_t* flushmap;

	registration_sequence++;
	r_oldviewcluster = -1;		// force markleafs

	snprintf(fullname, sizeof(fullname), "maps/%s.bsp", model);

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	flushmap = ri.Cvar_Get("flushmap", "0", 0);

	if (strcmp(mod_known[0].name, fullname) || flushmap->value)
		Mod_Free(&mod_known[0]);

	r_worldmodel = Mod_ForName(fullname, true);

	r_viewcluster = -1;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/
struct model_s* R_RegisterModel(const char* name)
{
	model_t*	mod;
	int32_t		i;
	dsprite_t*	sprout;
	dmdl_t*		pheader;

	mod = Mod_ForName(name, false);
	if (mod)
	{
		mod->registration_sequence = registration_sequence;

		// register any images used by the models
		if (mod->type == mod_sprite)
		{
			sprout = (dsprite_t*)mod->extradata;
			for (i = 0; i < sprout->numframes; i++)
				mod->skins[i] = GL_FindImage(sprout->frames[i].name, it_sprite);
		}
		else if (mod->type == mod_alias)
		{
			pheader = (dmdl_t*)mod->extradata;
			for (i = 0; i < pheader->num_skins; i++)
				mod->skins[i] = GL_FindImage((char*)pheader + pheader->ofs_skins + i * MAX_SKINNAME, it_skin);
			mod->numframes = pheader->num_frames;
		}
		else if (mod->type == mod_brush)
		{
			for (i = 0; i < mod->numtexinfo; i++)
				mod->texinfo[i].image->registration_sequence = registration_sequence;
		}
	}

	return mod;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration
@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration()
{
	int32_t	 i;
	model_t* mod;

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			continue;

		if (mod->registration_sequence != registration_sequence)
		{	// don't need this model
			Mod_Free(mod);
		}
	}

	GL_FreeUnusedImages();
}


//=============================================================================


/*
================
Mod_Free
================
*/
void Mod_Free(model_t* mod)
{
	Memory_HunkFree(mod->extradata);
	memset(mod, 0, sizeof(*mod));
}

/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll()
{
	int32_t		i;

	for (i = 0; i < mod_numknown; i++)
	{
		if (mod_known[i].extradatasize)
			Mod_Free(&mod_known[i]);
	}
}
