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
// GL_RSURF.C: surface-related refresh code
#include <cassert>

#include "gl_local.hpp"
#include <cctype>

static vec3_t	modelorg;		// relative to 

msurface_t* r_alpha_surfaces;

#define DYNAMIC_LIGHT_WIDTH  128
#define DYNAMIC_LIGHT_HEIGHT 128

#define LIGHTMAP_BYTES 4

#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

#define	MAX_LIGHTMAPS	256

int32_t		c_visible_lightmaps;
int32_t		c_visible_textures;

#define GL_LIGHTMAP_FORMAT GL_RGBA

typedef struct
{
	int32_t		internal_format;
	int32_t		current_lightmap_texture;

	msurface_t* lightmap_surfaces[MAX_LIGHTMAPS];

	int32_t		allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	uint8_t		lightmap_buffer[BLOCK_WIDTH * BLOCK_HEIGHT];
} gllightmapstate_t;

static gllightmapstate_t gl_lms;

void	LM_InitBlock(void);
void	LM_UploadBlock(bool dynamic);
bool	LM_AllocBlock(int32_t w, int32_t h, int32_t* x, int32_t* y);

extern void R_SetCacheState(msurface_t* surf);
extern void R_BuildLightMap(msurface_t* surf, uint8_t* dest, int32_t stride);

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t* R_TextureAnimation(mtexinfo_t* tex)
{
	int32_t		c;

	if (!tex->next)
		return tex->image;

	c = currententity->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}

/*
================
DrawGLPoly
================
*/
void DrawGLPoly(glpoly_t* p)
{
	int32_t	i;
	float* v;

	glBegin(GL_POLYGON);
	v = p->verts[0];
	for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
	{
		glTexCoord2f(v[3], v[4]);
		glVertex3fv(v);
	}
	glEnd();
}

/*
================
DrawGLFlowingPoly -- version of DrawGLPoly that handles scrolling textures
================
*/
void DrawGLFlowingPoly(msurface_t* fa)
{
	int32_t		i;
	float* v;
	glpoly_t* p;
	float	scroll;

	p = fa->polys;

	scroll = -64.0f * ((r_newrefdef.time / 40.0f) - (int32_t)(r_newrefdef.time / 40.0f));
	if (scroll == 0.0f)
		scroll = -64.0f;

	glBegin(GL_POLYGON);
	v = p->verts[0];
	for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
	{
		glTexCoord2f((v[3] + scroll), v[4]);
		glVertex3fv(v);
	}
	glEnd();
}

/*
** R_DrawTriangleOutlines
*/
void R_DrawTriangleOutlines()
{
	int32_t			i, j;
	glpoly_t* p;

	if (!gl_showtris->value)
		return;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glColor4f(1, 1, 1, 1);

	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		msurface_t* surf;

		for (surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain)
		{
			p = surf->polys;
			for (; p; p = p->chain)
			{
				for (j = 2; j < p->numverts; j++)
				{
					glBegin(GL_LINE_STRIP);
					glVertex3fv(p->verts[0]);
					glVertex3fv(p->verts[j - 1]);
					glVertex3fv(p->verts[j]);
					glVertex3fv(p->verts[0]);
					glEnd();
				}
			}
		}
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
}

/*
** DrawGLPolyChain
*/
void DrawGLPolyChain(glpoly_t* p, float soffset, float toffset)
{
	if (soffset == 0 && toffset == 0)
	{
		for (; p != 0; p = p->chain)
		{
			float* v;
			int32_t j;

			glBegin(GL_POLYGON);
			v = p->verts[0];
			for (j = 0; j < p->numverts; j++, v += VERTEXSIZE)
			{
				glTexCoord2f(v[5], v[6]);
				glVertex3fv(v);
			}
			glEnd();
		}
	}
	else
	{
		for (; p != 0; p = p->chain)
		{
			float* v;
			int32_t j;

			glBegin(GL_POLYGON);
			v = p->verts[0];
			for (j = 0; j < p->numverts; j++, v += VERTEXSIZE)
			{
				glTexCoord2f(v[5] - soffset, v[6] - toffset);
				glVertex3fv(v);
			}
			glEnd();
		}
	}
}

/*
** R_BlendLightMaps
**
** This routine takes all the given light mapped surfaces in the world and
** blends them into the framebuffer.
*/
void R_BlendLightmaps()
{
	int32_t			i;
	msurface_t* surf, * newdrawsurf = 0;

	// don't bother if we're set to fullbright
	if (r_fullbright->value)
		return;
	if (!r_worldmodel->lightdata)
		return;

	// don't bother writing Z
	glDepthMask(0);

	/*
	** set the appropriate blending mode unless we're only looking at the
	** lightmaps.
	*/
	if (!gl_lightmap->value)
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);

	if (currentmodel == r_worldmodel)
		c_visible_lightmaps = 0;

	/*
	** render static lightmaps first
	*/
	for (i = 1; i < MAX_LIGHTMAPS; i++)
	{
		if (gl_lms.lightmap_surfaces[i])
		{
			if (currentmodel == r_worldmodel)
				c_visible_lightmaps++;
			GL_Bind(gl_state.lightmap_textures + i);

			for (surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain)
			{
				if (surf->polys)
					DrawGLPolyChain(surf->polys, 0, 0);
			}
		}
	}

	/*
	** render dynamic lightmaps
	*/
	if (gl_dynamic->value)
	{
		LM_InitBlock();

		GL_Bind(gl_state.lightmap_textures + 0);

		if (currentmodel == r_worldmodel)
			c_visible_lightmaps++;

		newdrawsurf = gl_lms.lightmap_surfaces[0];

		for (surf = gl_lms.lightmap_surfaces[0]; surf != 0; surf = surf->lightmapchain)
		{
			int32_t		smax, tmax;
			uint8_t* base;

			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			if (LM_AllocBlock(smax, tmax, &surf->dlight_s, &surf->dlight_t))
			{
				base = gl_lms.lightmap_buffer;
				base += (surf->dlight_t * BLOCK_WIDTH + surf->dlight_s) * LIGHTMAP_BYTES;

				R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
			}
			else
			{
				msurface_t* drawsurf;

				// upload what we have so far
				LM_UploadBlock(true);

				// draw all surfaces that use this lightmap
				for (drawsurf = newdrawsurf; drawsurf != surf; drawsurf = drawsurf->lightmapchain)
				{
					if (drawsurf->polys)
						DrawGLPolyChain(drawsurf->polys,
							(drawsurf->light_s - drawsurf->dlight_s) * (1.0f / 128.0f),
							(drawsurf->light_t - drawsurf->dlight_t) * (1.0f / 128.0f));
				}

				newdrawsurf = drawsurf;

				// clear the block
				LM_InitBlock();

				// try uploading the block now
				if (!LM_AllocBlock(smax, tmax, &surf->dlight_s, &surf->dlight_t))
				{
					ri.Sys_Error(ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed (dynamic)\n", smax, tmax);
				}

				base = gl_lms.lightmap_buffer;
				base += (surf->dlight_t * BLOCK_WIDTH + surf->dlight_s) * LIGHTMAP_BYTES;

				R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
			}
		}

		/*
		** draw remainder of dynamic lightmaps that haven't been uploaded yet
		*/
		if (newdrawsurf)
			LM_UploadBlock(true);

		for (surf = newdrawsurf; surf != 0; surf = surf->lightmapchain)
		{
			if (surf->polys)
				DrawGLPolyChain(surf->polys, (surf->light_s - surf->dlight_s) * (1.0f / 128.0f), (surf->light_t - surf->dlight_t) * (1.0f / 128.0f));
		}
	}

	/*
	** restore state
	*/
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(1);
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly(msurface_t* fa)
{
	int32_t			maps;
	image_t* image;
	bool is_dynamic = false;

	c_brush_polys++;

	image = R_TextureAnimation(fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{
		GL_Bind(image->texnum);

		// warp texture, no lightmaps
		GL_TexEnv(GL_MODULATE);
		glColor4f(gl_state.inverse_intensity,
			gl_state.inverse_intensity,
			gl_state.inverse_intensity,
			1.0F);
		EmitWaterPolys(fa);
		GL_TexEnv(GL_REPLACE);

		return;
	}
	else
	{
		GL_Bind(image->texnum);

		GL_TexEnv(GL_REPLACE);
	}

	if (fa->texinfo->flags & SURF_FLOWING)
		DrawGLFlowingPoly(fa);
	else
		DrawGLPoly(fa->polys);

	/*
	** check for lightmap modification
	*/
	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (r_newrefdef.lightstyles[fa->styles[maps]].white != fa->cached_light[maps])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ((fa->dlightframe == r_framecount))
	{
	dynamic:
		if (gl_dynamic->value)
		{
			if (!(fa->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		if ((fa->styles[maps] >= 32 || fa->styles[maps] == 0) && (fa->dlightframe != r_framecount))
		{
			uint32_t	temp[34 * 34];
			int32_t		smax, tmax;

			smax = (fa->extents[0] >> 4) + 1;
			tmax = (fa->extents[1] >> 4) + 1;

			R_BuildLightMap(fa, (uint8_t*)temp, smax * 4);
			R_SetCacheState(fa);

			GL_Bind(gl_state.lightmap_textures + fa->lightmaptexturenum);

			glTexSubImage2D(GL_TEXTURE_2D, 0,
				fa->light_s, fa->light_t,
				smax, tmax,
				GL_LIGHTMAP_FORMAT,
				GL_UNSIGNED_BYTE, temp);

			fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
			gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = gl_lms.lightmap_surfaces[0];
			gl_lms.lightmap_surfaces[0] = fa;
		}
	}
	else
	{
		fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
		gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}


/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces()
{
	msurface_t* s;
	float		inverse_intensity;

	//
	// go back to the world matrix
	//
	glLoadMatrixf(r_world_matrix);

	glEnable(GL_BLEND);
	GL_TexEnv(GL_MODULATE);

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	inverse_intensity = gl_state.inverse_intensity;

	for (s = r_alpha_surfaces; s; s = s->texturechain)
	{
		GL_Bind(s->texinfo->image->texnum);
		c_brush_polys++;
		if (s->texinfo->flags & SURF_TRANS33)
			glColor4f(inverse_intensity, inverse_intensity, inverse_intensity, 0.33);
		else if (s->texinfo->flags & SURF_TRANS66)
			glColor4f(inverse_intensity, inverse_intensity, inverse_intensity, 0.66);
		else
			glColor4f(inverse_intensity, inverse_intensity, inverse_intensity, 1);
		if (s->flags & SURF_DRAWTURB)
			EmitWaterPolys(s);
		else if (s->texinfo->flags & SURF_FLOWING)			// PGM	9/16/98
			DrawGLFlowingPoly(s);							// PGM
		else
			DrawGLPoly(s->polys);
	}

	GL_TexEnv(GL_REPLACE);
	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);

	r_alpha_surfaces = NULL;
}

/*
================
DrawTextureChains
================
*/
void DrawTextureChains()
{
	int32_t		i;
	msurface_t* s;
	image_t* image;

	c_visible_textures = 0;

	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;
		if (!image->texturechain)
			continue;
		c_visible_textures++;

		for (s = image->texturechain; s; s = s->texturechain)
		{
			if (!(s->flags & SURF_DRAWTURB))
				R_RenderBrushPoly(s);
		}
	}

	GL_EnableMultitexture(false);
	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;
		s = image->texturechain;
		if (!s)
			continue;

		for (; s; s = s->texturechain)
		{
			if (s->flags & SURF_DRAWTURB)
				R_RenderBrushPoly(s);
		}

		image->texturechain = NULL;
	}

	GL_TexEnv(GL_REPLACE);
}


static void GL_RenderLightmappedPoly(msurface_t* surf)
{
	int32_t		i, nv = surf->polys->numverts;
	int32_t		map;
	float* v;
	image_t* image = R_TextureAnimation(surf->texinfo);
	bool is_dynamic = false;
	unsigned lmtex = surf->lightmaptexturenum;
	glpoly_t* p;

	for (map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++)
	{
		if (r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ((surf->dlightframe == r_framecount))
	{
	dynamic:
		if (gl_dynamic->value)
		{
			if (!(surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		uint32_t temp[DYNAMIC_LIGHT_WIDTH * DYNAMIC_LIGHT_HEIGHT];
		int32_t	 smax, tmax;

		if ((surf->styles[map] >= 32 || surf->styles[map] == 0) && (surf->dlightframe != r_framecount))
		{
			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			R_BuildLightMap(surf, (uint8_t*)temp, smax * 4);
			R_SetCacheState(surf);

			GL_MBind(GL_TEXTURE1, gl_state.lightmap_textures + surf->lightmaptexturenum);

			lmtex = surf->lightmaptexturenum;

			glTexSubImage2D(GL_TEXTURE_2D, 0,
				surf->light_s, surf->light_t,
				smax, tmax,
				GL_LIGHTMAP_FORMAT,
				GL_UNSIGNED_BYTE, temp);

		}
		else
		{
			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			R_BuildLightMap(surf, (uint8_t*)temp, smax * 4);

			GL_MBind(GL_TEXTURE1, gl_state.lightmap_textures + 0);

			lmtex = 0;

			glTexSubImage2D(GL_TEXTURE_2D, 0,
				surf->light_s, surf->light_t,
				smax, tmax,
				GL_LIGHTMAP_FORMAT,
				GL_UNSIGNED_BYTE, temp);

		}

		c_brush_polys++;

		GL_MBind(GL_TEXTURE0, image->texnum);
		GL_MBind(GL_TEXTURE1, gl_state.lightmap_textures + lmtex);

		if (surf->texinfo->flags & SURF_FLOWING)
		{
			float scroll;

			scroll = -64.0f * ((r_newrefdef.time / 40.0f) - (int32_t)(r_newrefdef.time / 40.0f));
			if (scroll == 0.0)
				scroll = -64.0f;

			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				glBegin(GL_POLYGON);
				for (i = 0; i < nv; i++, v += VERTEXSIZE)
				{
					glMultiTexCoord2f(GL_TEXTURE0, (v[3] + scroll), v[4]);
					glMultiTexCoord2f(GL_TEXTURE1, v[5], v[6]);
					glVertex3fv(v);
				}
				glEnd();
			}
		}
		else
		{
			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				glBegin(GL_POLYGON);
				for (i = 0; i < nv; i++, v += VERTEXSIZE)
				{
					glMultiTexCoord2f(GL_TEXTURE0, v[3], v[4]);
					glMultiTexCoord2f(GL_TEXTURE1, v[5], v[6]);
					glVertex3fv(v);
				}
				glEnd();
			}
		}

	}
	else
	{
		c_brush_polys++;

		GL_MBind(GL_TEXTURE0, image->texnum);
		GL_MBind(GL_TEXTURE1, gl_state.lightmap_textures + lmtex);

		if (surf->texinfo->flags & SURF_FLOWING)
		{
			float scroll;

			scroll = -64.0f * ((r_newrefdef.time / 40.0f) - (int32_t)(r_newrefdef.time / 40.0f));
			if (scroll == 0.0)
				scroll = -64.0f;

			// scroll the texture
			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				glBegin(GL_POLYGON);
				for (i = 0; i < nv; i++, v += VERTEXSIZE)
				{
					glMultiTexCoord2f(GL_TEXTURE0, (v[3] + scroll), v[4]);
					glMultiTexCoord2f(GL_TEXTURE1, v[5], v[6]);
					glVertex3fv(v);
				}
				glEnd();
			}
		}
		else
		{
			// non-flowing textures
			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				glBegin(GL_POLYGON);
				for (i = 0; i < nv; i++, v += VERTEXSIZE)
				{
					glMultiTexCoord2f(GL_TEXTURE0, v[3], v[4]);
					glMultiTexCoord2f(GL_TEXTURE1, v[5], v[6]);
					glVertex3fv(v);
				}
				glEnd();
			}
		}
	}
}

/*
=================
R_DrawInlineBModel
=================
*/
void R_DrawInlineBModel()
{
	int32_t			i, k;
	cplane_t* pplane;
	float		dot;
	msurface_t* psurf;
	dlight_t* lt;

	// calculate dynamic lighting for bmodel
	if (!gl_flashblend->value)
	{
		lt = r_newrefdef.dlights;
		for (k = 0; k < r_newrefdef.num_dlights; k++, lt++)
		{
			R_MarkLights(lt, 1 << k, currentmodel->nodes + currentmodel->firstnode);
		}
	}

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	if (currententity->flags & RF_TRANSLUCENT)
	{
		glEnable(GL_BLEND);
		glColor4f(1, 1, 1, 0.25);
		GL_TexEnv(GL_MODULATE);
	}

	//
	// draw texture
	//
	for (i = 0; i < currentmodel->nummodelsurfaces; i++, psurf++)
	{
		// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct3(modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66))
			{	// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else if (glMultiTexCoord2f && !(psurf->flags & SURF_DRAWTURB))
			{
				GL_RenderLightmappedPoly(psurf);
			}
			else
			{
				GL_EnableMultitexture(false);
				R_RenderBrushPoly(psurf);
				GL_EnableMultitexture(true);
			}
		}
	}

	if (!(currententity->flags & RF_TRANSLUCENT))
	{
		R_BlendLightmaps();
	}
	else
	{
		glDisable(GL_BLEND);
		glColor4f(1, 1, 1, 1);
		GL_TexEnv(GL_REPLACE);
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel(entity_t* e)
{
	vec3_t		mins, maxs;
	int32_t		i;
	bool		rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;
	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i = 0; i < 3; i++)
		{
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd3(e->origin, currentmodel->mins, mins);
		VectorAdd3(e->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;

	glColor3f(1, 1, 1);
	memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	VectorSubtract3(r_newrefdef.vieworigin, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy3(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct3(temp, forward);
		modelorg[1] = -DotProduct3(temp, right);
		modelorg[2] = DotProduct3(temp, up);
	}

	glPushMatrix();

	// TODO: figure out what is causing this and fix it for zombono engine
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug
	R_RotateForEntity(e);
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug

	GL_EnableMultitexture(true);
	GL_SelectTexture(GL_TEXTURE0);
	GL_TexEnv(GL_REPLACE);
	GL_SelectTexture(GL_TEXTURE1);
	GL_TexEnv(GL_MODULATE);

	R_DrawInlineBModel();
	GL_EnableMultitexture(false);

	glPopMatrix();
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode(mnode_t* node)
{
	int32_t		c, side, sidebit;
	cplane_t* plane;
	msurface_t* surf, ** mark;
	mleaf_t* pleaf;
	float		dot;
	image_t* image;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;
	if (R_CullBox(node->minmaxs, node->minmaxs + 3))
		return;

	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t*)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (!(r_newrefdef.areabits[pleaf->area >> 3] & (1 << (pleaf->area & 7))))
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;

			} while (--c);
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct3(modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNode(node->children[side]);

	// draw stuff
	for (c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
			continue;		// wrong side

		if (surf->flags & SURF_NODRAW)
			continue;		// don't draw

		if (surf->texinfo->flags & SURF_SKY)
		{	// just adds to visible sky bounds
			R_AddSkySurface(surf);
		}
		else if (surf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66))
		{	// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			if (glMultiTexCoord2f && !(surf->flags & SURF_DRAWTURB))
			{
				GL_RenderLightmappedPoly(surf);
			}
			else
			{
				// the polygon is visible, so add it to the texture
				// sorted chain
				// FIXME: this is a hack for animation
				image = R_TextureAnimation(surf->texinfo);
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode(node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld()
{
	entity_t	ent;

	if (!r_drawworld->value)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	currentmodel = r_worldmodel;

	VectorCopy3(r_newrefdef.vieworigin, modelorg);

	// auto cycle the world frame for texture animation
	memset(&ent, 0, sizeof(ent));
	ent.frame = (int32_t)(r_newrefdef.time * 2);
	currententity = &ent;

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	glColor3f(1, 1, 1);
	memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	R_ClearSkyBox();

	if (!r_fullbright->value)
	{
		// Replace texture data with texture data blended with BSP lightmap info...
		GL_EnableMultitexture(true);

		GL_SelectTexture(GL_TEXTURE0);
		GL_TexEnv(GL_REPLACE);
		GL_SelectTexture(GL_TEXTURE1);

		if (gl_lightmap->value)
			GL_TexEnv(GL_REPLACE);
		else
			GL_TexEnv(GL_MODULATE);

		//...then render the world with that data. This makes it look shiny and not garbage
		R_RecursiveWorldNode(r_worldmodel->nodes);

		GL_EnableMultitexture(false);
	}
	else
	{
		// just fullbright it
		R_RecursiveWorldNode(r_worldmodel->nodes);
	}


	/*
	** theoretically nothing should happen in the next two functions
	** if multitexture is enabled
	*/
	DrawTextureChains();
	R_BlendLightmaps();

	R_DrawSkyBox();

	R_DrawTriangleOutlines();
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves()
{
	uint8_t*	vis;
	byte		fatvis[MAX_MAP_LEAFS / 8];
	mnode_t*	node;
	int32_t		i, c;
	mleaf_t*	leaf;
	int32_t		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (gl_lockpvs->value)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i = 0; i < r_worldmodel->numleafs; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i = 0; i < r_worldmodel->numnodes; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS(r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		memcpy(fatvis, vis, (r_worldmodel->numleafs + 7) / 8);
		vis = Mod_ClusterPVS(r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs + 31) / 32;
		for (i = 0; i < c; i++)
			((int32_t*)fatvis)[i] |= ((int32_t*)vis)[i];
		vis = fatvis;
	}

	for (i = 0, leaf = r_worldmodel->leafs; i < r_worldmodel->numleafs; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster >> 3] & (1 << (cluster & 7)))
		{
			node = (mnode_t*)leaf;

			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} 
			while (node);
		}
	}
}

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

void LM_InitBlock(void)
{
	memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));
}

void LM_UploadBlock(bool dynamic)
{
	int32_t texture;
	int32_t height = 0;

	if (dynamic)
	{
		texture = 0;
	}
	else
	{
		texture = gl_lms.current_lightmap_texture;
	}

	GL_Bind(gl_state.lightmap_textures + texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (dynamic)
	{
		int32_t i;

		for (i = 0; i < BLOCK_WIDTH; i++)
		{
			if (gl_lms.allocated[i] > height)
				height = gl_lms.allocated[i];
		}

		glTexSubImage2D(GL_TEXTURE_2D,
			0,
			0, 0,
			BLOCK_WIDTH, height,
			GL_LIGHTMAP_FORMAT,
			GL_UNSIGNED_BYTE,
			gl_lms.lightmap_buffer);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D,
			0,
			gl_lms.internal_format,
			BLOCK_WIDTH, BLOCK_HEIGHT,
			0,
			GL_LIGHTMAP_FORMAT,
			GL_UNSIGNED_BYTE,
			gl_lms.lightmap_buffer);

		if (++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS)
			ri.Sys_Error(ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n");
	}
}

// returns a texture number and the position inside it
bool LM_AllocBlock(int32_t w, int32_t h, int32_t* x, int32_t* y)
{
	int32_t		i, j;
	int32_t		best, best2;

	best = BLOCK_HEIGHT;

	for (i = 0; i < BLOCK_WIDTH - w; i++)
	{
		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (gl_lms.allocated[i + j] >= best)
				break;
			if (gl_lms.allocated[i + j] > best2)
				best2 = gl_lms.allocated[i + j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return false;

	for (i = 0; i < w; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}

/*
================
GL_BuildPolygonFromSurface
================
*/
void GL_BuildPolygonFromSurface(msurface_t* fa)
{
	int32_t			i, lindex, lnumverts;
	medge_t* pedges, * r_pedge;
	int32_t			vertpage;
	float* vec;
	float		s, t;
	glpoly_t* poly;
	vec3_t		total;

	// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	VectorClear3(total);
	//
	// draw texture
	//
	poly = (glpoly_t*)Memory_HunkAlloc(sizeof(glpoly_t) + (lnumverts - 4) * VERTEXSIZE * sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0; i < lnumverts; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}
		s = DotProduct3(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct3(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		VectorAdd3(total, vec, total);
		VectorCopy3(vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct3(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

		t = DotProduct3(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;

}

/*
========================
GL_CreateSurfaceLightmap

========================
*/
void GL_CreateSurfaceLightmap(msurface_t* surf)
{
	int32_t		smax, tmax;
	uint8_t* base;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;


	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;

	//this is causing corruption on 12596 +
	if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
	{
		LM_UploadBlock(false);
		LM_InitBlock();
		if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
		{
			ri.Sys_Error(ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax);
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = gl_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState(surf);
	R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
}

uint32_t* dummy[BLOCK_HEIGHT * BLOCK_WIDTH];

/*
==================
GL_BeginBuildingLightmaps

==================
*/
void GL_BeginBuildingLightmaps(model_t* m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int32_t				i;
	memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));

	r_framecount = 1;		// no dlightcache

	GL_EnableMultitexture(true);
	GL_SelectTexture(GL_TEXTURE1);

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
	{
		gl_state.lightmap_textures = TEXNUM_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;

	gl_lms.internal_format = gl_tex_solid_format;

	/*
	** initialize the dynamic lightmap texture
	*/
	GL_Bind(gl_state.lightmap_textures + 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,
		0,
		gl_lms.internal_format,
		BLOCK_WIDTH, BLOCK_HEIGHT,
		0,
		GL_LIGHTMAP_FORMAT,
		GL_UNSIGNED_BYTE,
		dummy);
}

/*
=======================
GL_EndBuildingLightmaps
=======================
*/
void GL_EndBuildingLightmaps()
{
	LM_UploadBlock(false);
	GL_EnableMultitexture(false);
}