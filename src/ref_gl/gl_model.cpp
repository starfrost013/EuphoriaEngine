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
// gl_mesh.c: triangle model functions

#include "gl_local.hpp"

// Why does this code compile?
#define NUM_VERTEX_NORMALS	162

float	r_avertexnormals[NUM_VERTEX_NORMALS][3] = {
#include <client/include/anorms.hpp>
};

static	vec4_t	s_lerped[MAX_VERTS];

vec3_t	shadevector;
float	shadelight[3];

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16

#include "anormtab.hpp"

float* shadedots = r_avertexnormal_dots[0];

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel(model_t* mod, void* buffer)
{
	int32_t			i, j;
	dmdl_t* pinmodel, * pheader;
	dstvert_t* pinst, * poutst;
	dtriangle_t* pintri, * pouttri;
	daliasframe_t* pinframe, * poutframe;
	int32_t* pincmd, * poutcmd;
	int32_t			version;

	pinmodel = (dmdl_t*)buffer;

	version = LittleInt(pinmodel->version);
	if (version != ALIAS_VERSION)
		ri.Sys_Error(ERR_DROP, "%s has wrong version number (%i should be %i)",
			mod->name, version, ALIAS_VERSION);

	pheader = (dmdl_t*)Memory_HunkAlloc(LittleInt(pinmodel->ofs_end));

	// byte swap the header fields and sanity check
	for (i = 0; i < sizeof(dmdl_t) / 4; i++)
		((int32_t*)pheader)[i] = LittleInt(((int*)buffer)[i]);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
		ri.Sys_Error(ERR_DROP, "model %s has a skin taller than %d", mod->name,
			MAX_LBM_HEIGHT);

	if (pheader->num_xyz <= 0)
		ri.Sys_Error(ERR_DROP, "model %s has no vertices", mod->name);

	if (pheader->num_xyz > MAX_VERTS)
		ri.Sys_Error(ERR_DROP, "model %s has too many vertices", mod->name);

	if (pheader->num_st <= 0)
		ri.Sys_Error(ERR_DROP, "model %s has no st vertices", mod->name);

	if (pheader->num_tris <= 0)
		ri.Sys_Error(ERR_DROP, "model %s has no triangles", mod->name);

	if (pheader->num_frames <= 0)
		ri.Sys_Error(ERR_DROP, "model %s has no frames", mod->name);

	//
	// load base s and t vertices (not used in gl version)
	//
	pinst = (dstvert_t*)((uint8_t*)pinmodel + pheader->ofs_st);
	poutst = (dstvert_t*)((uint8_t*)pheader + pheader->ofs_st);

	for (i = 0; i < pheader->num_st; i++)
	{
		poutst[i].s = LittleShort(pinst[i].s);
		poutst[i].t = LittleShort(pinst[i].t);
	}

	//
	// load triangle lists
	//
	pintri = (dtriangle_t*)((uint8_t*)pinmodel + pheader->ofs_tris);
	pouttri = (dtriangle_t*)((uint8_t*)pheader + pheader->ofs_tris);

	for (i = 0; i < pheader->num_tris; i++)
	{
		for (j = 0; j < 3; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort(pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort(pintri[i].index_st[j]);
		}
	}

	//
	// load the frames
	//
	for (i = 0; i < pheader->num_frames; i++)
	{
		pinframe = (daliasframe_t*)((uint8_t*)pinmodel
			+ pheader->ofs_frames + i * pheader->framesize);
		poutframe = (daliasframe_t*)((uint8_t*)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy(poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j = 0; j < 3; j++)
		{
			poutframe->scale[j] = LittleFloat(pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
		}
		// verts are all 8 bit, so no swapping needed
		memcpy(poutframe->verts, pinframe->verts,
			pheader->num_xyz * sizeof(dtrivertx_t));

	}

	mod->type = mod_alias;

	//
	// load the glcmds
	//
	pincmd = (int32_t*)((uint8_t*)pinmodel + pheader->ofs_glcmds);
	poutcmd = (int32_t*)((uint8_t*)pheader + pheader->ofs_glcmds);
	for (i = 0; i < pheader->num_glcmds; i++)
		poutcmd[i] = LittleInt(pincmd[i]);


	// register all skins
	memcpy((char*)pheader + pheader->ofs_skins, (char*)pinmodel + pheader->ofs_skins,
		pheader->num_skins * MAX_SKINNAME);
	for (i = 0; i < pheader->num_skins; i++)
	{
		mod->skins[i] = GL_FindImage((char*)pheader + pheader->ofs_skins + i * MAX_SKINNAME
			, it_skin);
	}

	mod->mins[0] = -32;
	mod->mins[1] = -32;
	mod->mins[2] = -32;
	mod->maxs[0] = 32;
	mod->maxs[1] = 32;
	mod->maxs[2] = 32;
}


void GL_LerpVerts(int32_t nverts, dtrivertx_t* v, dtrivertx_t* ov, dtrivertx_t* verts, float* lerp, float move[3], float frontv[3], float backv[3])
{
	int32_t i;

	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
	{
		for (i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			float* normal = r_avertexnormals[verts[i].lightnormalindex];

			lerp[0] = move[0] + ov->v[0] * backv[0] + v->v[0] * frontv[0] + normal[0] * POWERSUIT_SCALE;
			lerp[1] = move[1] + ov->v[1] * backv[1] + v->v[1] * frontv[1] + normal[1] * POWERSUIT_SCALE;
			lerp[2] = move[2] + ov->v[2] * backv[2] + v->v[2] * frontv[2] + normal[2] * POWERSUIT_SCALE;
		}
	}
	else
	{
		for (i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			lerp[0] = move[0] + ov->v[0] * backv[0] + v->v[0] * frontv[0];
			lerp[1] = move[1] + ov->v[1] * backv[1] + v->v[1] * frontv[1];
			lerp[2] = move[2] + ov->v[2] * backv[2] + v->v[2] * frontv[2];
		}
	}

}

/*
=============
GL_DrawAliasFrameLerp

interpolates between two frames and origins
FIXME: batch lerp all vertexes
=============
*/
void GL_DrawAliasFrameLerp(dmdl_t* paliashdr, float backlerp)
{
	float 			l;
	daliasframe_t* frame, * oldframe;
	dtrivertx_t* v, * ov, * verts;
	int32_t* order;
	int32_t			count;
	float			frontlerp;
	float			alpha;
	vec3_t			move, delta, vectors[3];
	vec3_t			frontv, backv;
	int32_t			i;
	int32_t			index_xyz;
	float* lerp;

	frame = (daliasframe_t*)((uint8_t*)paliashdr + paliashdr->ofs_frames
		+ currententity->frame * paliashdr->framesize);
	verts = v = frame->verts;

	oldframe = (daliasframe_t*)((uint8_t*)paliashdr + paliashdr->ofs_frames
		+ currententity->oldframe * paliashdr->framesize);
	ov = oldframe->verts;

	order = (int32_t*)((uint8_t*)paliashdr + paliashdr->ofs_glcmds);

	if (currententity->flags & RF_TRANSLUCENT)
		alpha = currententity->alpha;
	else
		alpha = 1.0;

	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
		glDisable(GL_TEXTURE_2D);

	frontlerp = 1.0f - backlerp;

	// move should be the delta back to the previous frame * backlerp
	VectorSubtract3(currententity->oldorigin, currententity->origin, delta);
	AngleVectors(currententity->angles, vectors[0], vectors[1], vectors[2]);

	move[0] = DotProduct3(delta, vectors[0]);	// forward
	move[1] = -DotProduct3(delta, vectors[1]);	// left
	move[2] = DotProduct3(delta, vectors[2]);	// up

	VectorAdd3(move, oldframe->translate, move);

	for (i = 0; i < 3; i++)
	{
		move[i] = backlerp * move[i] + frontlerp * frame->translate[i];
	}

	for (i = 0; i < 3; i++)
	{
		frontv[i] = frontlerp * frame->scale[i];
		backv[i] = backlerp * oldframe->scale[i];
	}

	lerp = s_lerped[0];

	GL_LerpVerts(paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv);

	if (gl_vertex_arrays->value)
	{
		float colorArray[MAX_VERTS * 4];

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 16, s_lerped);	// padded for SIMD

		if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
		{
			glColor4f(shadelight[0], shadelight[1], shadelight[2], alpha);
		}
		else
		{
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(3, GL_FLOAT, 0, colorArray);

			//
			// pre light everything
			//
			for (i = 0; i < paliashdr->num_xyz; i++)
			{
				float l = shadedots[verts[i].lightnormalindex];

				colorArray[i * 3 + 0] = l * shadelight[0];
				colorArray[i * 3 + 1] = l * shadelight[1];
				colorArray[i * 3 + 2] = l * shadelight[2];
			}
		}

		if (glLockArraysEXT != 0)
			glLockArraysEXT(0, paliashdr->num_xyz);

		while (1)
		{
			// get the vertex count and primitive type
			count = *order++;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				glBegin(GL_TRIANGLE_FAN);
			}
			else
			{
				glBegin(GL_TRIANGLE_STRIP);
			}

			if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
			{
				do
				{
					index_xyz = order[2];
					order += 3;

					glVertex3fv(s_lerped[index_xyz]);

				} while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					glTexCoord2f(((float*)order)[0], ((float*)order)[1]);
					index_xyz = order[2];

					order += 3;

					glArrayElement(index_xyz);

				} while (--count);
			}
			glEnd();
		}

		if (glUnlockArraysEXT != 0)
			glUnlockArraysEXT();
	}
	else
	{
		while (1)
		{
			// get the vertex count and primitive type
			count = *order++;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				glBegin(GL_TRIANGLE_FAN);
			}
			else
			{
				glBegin(GL_TRIANGLE_STRIP);
			}

			if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
			{
				do
				{
					index_xyz = order[2];
					order += 3;

					glColor4f(shadelight[0], shadelight[1], shadelight[2], alpha);
					glVertex3fv(s_lerped[index_xyz]);

				} while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					glTexCoord2f(((float*)order)[0], ((float*)order)[1]);
					index_xyz = order[2];
					order += 3;

					// normals and vertexes come from the frame list
					l = shadedots[verts[index_xyz].lightnormalindex];

					glColor4f(l * shadelight[0], l * shadelight[1], l * shadelight[2], alpha);
					glVertex3fv(s_lerped[index_xyz]);
				} while (--count);
			}

			glEnd();
		}
	}

	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
		glEnable(GL_TEXTURE_2D);
}

/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void GL_DrawAliasShadow(dmdl_t* paliashdr, int32_t posenum)
{
	dtrivertx_t* verts;
	int32_t* order;
	vec3_t			point;
	float			height, lheight;
	int32_t			count;
	daliasframe_t* frame;

	lheight = currententity->origin[2] - lightspot[2];

	frame = (daliasframe_t*)((uint8_t*)paliashdr + paliashdr->ofs_frames
		+ currententity->frame * paliashdr->framesize);
	verts = frame->verts;

	height = 0;

	order = (int32_t*)((uint8_t*)paliashdr + paliashdr->ofs_glcmds);

	height = -lheight + 1.0f;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done

		if (count < 0)
		{
			count = -count;
			glBegin(GL_TRIANGLE_FAN);
		}
		else
		{
			glBegin(GL_TRIANGLE_STRIP);
		}

		do
		{
			memcpy(point, s_lerped[order[2]], sizeof(point));

			point[0] -= shadevector[0] * (point[2] + lheight);
			point[1] -= shadevector[1] * (point[2] + lheight);
			point[2] = height;
			glVertex3fv(point);

			order += 3;

		} while (--count);

		glEnd();
	}
}

/*
** R_CullAliasModel
*/
static bool R_CullAliasModel(vec3_t bbox[8], entity_t* e)
{
	int32_t i;
	vec3_t		mins, maxs;
	dmdl_t* paliashdr;
	vec3_t		vectors[3];
	vec3_t		thismins, oldmins, thismaxs, oldmaxs;
	daliasframe_t* pframe, * poldframe;
	vec3_t angles;

	paliashdr = (dmdl_t*)currentmodel->extradata;

	if ((e->frame >= paliashdr->num_frames) || (e->frame < 0))
	{
		ri.Con_Printf(PRINT_DEVELOPER, "R_CullAliasModel %s: no such frame %d\n",
			currentmodel->name, e->frame);
		e->frame = 0;
	}
	if ((e->oldframe >= paliashdr->num_frames) || (e->oldframe < 0))
	{
		ri.Con_Printf(PRINT_DEVELOPER, "R_CullAliasModel %s: no such oldframe %d\n",
			currentmodel->name, e->oldframe);
		e->oldframe = 0;
	}

	pframe = (daliasframe_t*)((uint8_t*)paliashdr +
		paliashdr->ofs_frames +
		e->frame * paliashdr->framesize);

	poldframe = (daliasframe_t*)((uint8_t*)paliashdr +
		paliashdr->ofs_frames +
		e->oldframe * paliashdr->framesize);

	/*
	** compute axially aligned mins and maxs
	*/
	if (pframe == poldframe)
	{
		for (i = 0; i < 3; i++)
		{
			mins[i] = pframe->translate[i];
			maxs[i] = mins[i] + pframe->scale[i] * 255;
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			thismins[i] = pframe->translate[i];
			thismaxs[i] = thismins[i] + pframe->scale[i] * 255;

			oldmins[i] = poldframe->translate[i];
			oldmaxs[i] = oldmins[i] + poldframe->scale[i] * 255;

			if (thismins[i] < oldmins[i])
				mins[i] = thismins[i];
			else
				mins[i] = oldmins[i];

			if (thismaxs[i] > oldmaxs[i])
				maxs[i] = thismaxs[i];
			else
				maxs[i] = oldmaxs[i];
		}
	}

	/*
	** compute a full bounding box
	*/
	for (i = 0; i < 8; i++)
	{
		vec3_t   tmp;

		if (i & 1)
			tmp[0] = mins[0];
		else
			tmp[0] = maxs[0];

		if (i & 2)
			tmp[1] = mins[1];
		else
			tmp[1] = maxs[1];

		if (i & 4)
			tmp[2] = mins[2];
		else
			tmp[2] = maxs[2];

		VectorCopy3(tmp, bbox[i]);
	}

	/*
	** rotate the bounding box
	*/
	VectorCopy3(e->angles, angles);
	angles[YAW] = -angles[YAW];
	AngleVectors(angles, vectors[0], vectors[1], vectors[2]);

	for (i = 0; i < 8; i++)
	{
		vec3_t tmp;

		VectorCopy3(bbox[i], tmp);

		bbox[i][0] = DotProduct3(vectors[0], tmp);
		bbox[i][1] = -DotProduct3(vectors[1], tmp);
		bbox[i][2] = DotProduct3(vectors[2], tmp);

		VectorAdd3(e->origin, bbox[i], bbox[i]);
	}

	{
		int32_t p, f, aggregatemask = ~0;

		for (p = 0; p < 8; p++)
		{
			int32_t mask = 0;

			for (f = 0; f < 4; f++)
			{
				float dp = DotProduct3(frustum[f].normal, bbox[p]);

				if ((dp - frustum[f].dist) < 0)
				{
					mask |= (1 << f);
				}
			}

			aggregatemask &= mask;
		}

		if (aggregatemask)
		{
			return true;
		}

		return false;
	}
}

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel(entity_t* e)
{
	int32_t		i;
	dmdl_t* paliashdr;
	float		an;
	vec3_t		bbox[8];
	image_t* skin;

	if (!(e->flags & RF_WEAPONMODEL))
	{
		if (R_CullAliasModel(bbox, e))
			return;
	}

	if (e->flags & RF_WEAPONMODEL)
	{
		if (r_lefthand->value == 2)
			return;
	}

	paliashdr = (dmdl_t*)currentmodel->extradata;

	//
	// get model lighting information
	//
	if (currententity->flags & (RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE))
	{
		VectorClear3(shadelight);
		if (currententity->flags & RF_SHELL_RED)
			shadelight[0] = 1.0;
		if (currententity->flags & RF_SHELL_GREEN)
			shadelight[1] = 1.0;
		if (currententity->flags & RF_SHELL_BLUE)
			shadelight[2] = 1.0;
	}
	else if (currententity->flags & RF_FULLBRIGHT)
	{
		for (i = 0; i < 3; i++)
			shadelight[i] = 1.0;
	}
	else
	{
		R_LightPoint(currententity->origin, shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if (currententity->flags & RF_WEAPONMODEL)
		{
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if (shadelight[0] > shadelight[1])
			{
				if (shadelight[0] > shadelight[2])
					r_lightlevel->value = 150 * shadelight[0];
				else
					r_lightlevel->value = 150 * shadelight[2];
			}
			else
			{
				if (shadelight[1] > shadelight[2])
					r_lightlevel->value = 150 * shadelight[1];
				else
					r_lightlevel->value = 150 * shadelight[2];
			}

		}
	}

	if (currententity->flags & RF_MINLIGHT)
	{
		for (i = 0; i < 3; i++)
			if (shadelight[i] > 0.1)
				break;
		if (i == 3)
		{
			shadelight[0] = 0.1;
			shadelight[1] = 0.1;
			shadelight[2] = 0.1;
		}
	}

	if (currententity->flags & RF_GLOW)
	{	// bonus items will pulse with time
		float scale;
		float min;

		scale = 0.1f * sinf(r_newrefdef.time * 7);
		for (i = 0; i < 3; i++)
		{
			min = shadelight[i] * 0.8f;
			shadelight[i] += scale;
			if (shadelight[i] < min)
				shadelight[i] = min;
		}
	}

	shadedots = r_avertexnormal_dots[((int32_t)(currententity->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];

	an = currententity->angles[1] / 180 * M_PI;
	shadevector[0] = cosf(-an);
	shadevector[1] = sinf(-an);
	shadevector[2] = 1;
	VectorNormalize3(shadevector);

	//
	// locate the proper data
	//

	c_alias_polys += paliashdr->num_tris;

	//
	// draw all the triangles
	//
	if (currententity->flags & RF_DEPTHHACK) // hack the depth range to prevent view model from poking into walls
		glDepthRange(gldepthmin, gldepthmin + 0.3 * (gldepthmax - gldepthmin));

	if ((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F))
	{
		extern void GL_SetPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glScalef(-1, 1, 1);
		GL_SetPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / r_newrefdef.height, 4, gl_drawdistance->value + gl_skyboxepsilon->value);
		glMatrixMode(GL_MODELVIEW);

		glCullFace(GL_BACK);
	}

	glPushMatrix();
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	R_RotateForEntity(e);
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.

	// select skin
	if (currententity->skin)
		skin = currententity->skin;	// custom player skin
	else
	{
		if (currententity->skinnum >= MAX_MD2SKINS)
			skin = currentmodel->skins[0];
		else
		{
			skin = currentmodel->skins[currententity->skinnum];
			if (!skin)
				skin = currentmodel->skins[0];
		}
	}
	if (!skin)
		skin = r_notexture;	// fallback...

	GL_Bind(skin->texnum);

	// draw it

	glShadeModel(GL_SMOOTH);

	GL_TexEnv(GL_MODULATE);

	if (currententity->flags & RF_TRANSLUCENT)
	{
		glEnable(GL_BLEND);
	}

	if ((currententity->frame >= paliashdr->num_frames)
		|| (currententity->frame < 0))
	{
		ri.Con_Printf(PRINT_ALL, "R_DrawAliasModel %s: no such frame %d\n",
			currentmodel->name, currententity->frame);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ((currententity->oldframe >= paliashdr->num_frames)
		|| (currententity->oldframe < 0))
	{
		ri.Con_Printf(PRINT_ALL, "R_DrawAliasModel %s: no such oldframe %d\n",
			currentmodel->name, currententity->oldframe);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if (!r_lerpmodels->value)
		currententity->backlerp = 0;
	GL_DrawAliasFrameLerp(paliashdr, currententity->backlerp);

	GL_TexEnv(GL_REPLACE);
	glShadeModel(GL_FLAT);

	glPopMatrix();

	if ((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F))
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glCullFace(GL_FRONT);
	}

	if (currententity->flags & RF_TRANSLUCENT)
	{
		glDisable(GL_BLEND);
	}

	if (currententity->flags & RF_DEPTHHACK)
		glDepthRange(gldepthmin, gldepthmax);

	if (gl_shadows->value && !(currententity->flags & (RF_TRANSLUCENT | RF_WEAPONMODEL)))
	{
		glPushMatrix();
		R_RotateForEntity(e);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glColor4f(0, 0, 0, 0.5);
		GL_DrawAliasShadow(paliashdr, currententity->frame);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glPopMatrix();
	}

	glColor4f(1, 1, 1, 1);
}