/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018-2019 Krzysztof Kondrak
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
// gl_rmain.c
#include "gl_local.hpp"
#include <cctype>

void R_Clear();

viddef_t	vid;

refimport_t	ri;

model_t* r_worldmodel;

float		gldepthmin, gldepthmax;

gl_config_t	gl_config;

image_t* r_notexture;		// use for bad textures
image_t* r_particletexture;	// little dot for particles

entity_t* currententity;
model_t* currentmodel;

cplane_t	frustum[4];

int32_t		r_visframecount;	// bumped when going to a new PVS
int32_t		r_framecount;		// used for dlight push checking

int32_t		c_brush_polys, c_alias_polys;

float		v_blend[4];			// final blending color

void GL_Strings_f(void);

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float r_world_matrix[16];
float r_base_world_matrix[16];

//
// screen size info
//
refdef_t r_newrefdef;

int32_t	r_viewcluster;
int32_t r_viewcluster2;
int32_t r_oldviewcluster;
int32_t r_oldviewcluster2;

cvar_t* r_norefresh;
cvar_t* r_drawentities;
cvar_t* r_drawworld;
cvar_t* r_speeds;
cvar_t* r_fullbright;
cvar_t* r_novis;
cvar_t* r_nocull;
cvar_t* r_lerpmodels;
cvar_t* r_lefthand;
cvar_t* r_scaled_assets_basewidth;

cvar_t* r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t* gl_vertex_arrays;

cvar_t* gl_particle_min_size;
cvar_t* gl_particle_max_size;
cvar_t* gl_particle_size;
cvar_t* gl_particle_att_a;
cvar_t* gl_particle_att_b;
cvar_t* gl_particle_att_c;

cvar_t* gl_vsync;
cvar_t* gl_ext_compiled_vertex_array;

cvar_t* gl_drawbuffer;
cvar_t* gl_lightmap;
cvar_t* gl_shadows;
cvar_t* r_width;
cvar_t* r_height;
cvar_t* gl_dynamic;
cvar_t* gl_modulate;
cvar_t* gl_round_down;
cvar_t* gl_picmip;
cvar_t* gl_skymip;
cvar_t* gl_showtris;
cvar_t* gl_clear;
cvar_t* gl_cull;
cvar_t* gl_polyblend;
cvar_t* gl_flashblend;
cvar_t* gl_playermip;
cvar_t* gl_drawdistance;
cvar_t* gl_skyboxepsilon;
cvar_t* gl_texturemode;
cvar_t* gl_texturealphamode;
cvar_t* gl_texturesolidmode;
cvar_t* gl_lockpvs;

cvar_t* vid_borderless;
cvar_t* vid_fullscreen;
cvar_t* vid_gamma;
cvar_t* vid_ref;

cvar_t* game_name;
cvar_t* game_asset_path;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustum
=================
*/
bool R_CullBox(vec3_t mins, vec3_t maxs)
{
	int32_t i;

	if (r_nocull->value)
		return false;

	for (i = 0; i < 4; i++)
		if (BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity(entity_t* e)
{
	glTranslatef(e->origin[0], e->origin[1], e->origin[2]);

	glRotatef(e->angles[1], 0, 0, 1);
	glRotatef(-e->angles[0], 0, 1, 0);
	glRotatef(-e->angles[2], 1, 0, 0);
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel()
{
	vec3_t	shadelight;
	int32_t	i;

	if (currententity->flags & RF_FULLBRIGHT)
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint(currententity->origin, shadelight);

	glPushMatrix();
	R_RotateForEntity(currententity);

	glDisable(GL_TEXTURE_2D);
	glColor3fv(shadelight);

	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0, 0, -16);
	for (i = 0; i <= 4; i++)
		glVertex3f(16.0f * cosf(i * M_PI / 2), 16 * sinf(i * M_PI / 2), 0);
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0, 0, 16);
	for (i = 4; i >= 0; i--)
		glVertex3f(16.0f * cosf(i * M_PI / 2), 16 * sinf(i * M_PI / 2), 0);
	glEnd();

	glColor3f(1, 1, 1);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList()
{
	int32_t	i;

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// solid

		if (currententity->flags & RF_BEAM)
		{
			R_DrawBeam(currententity);
		}
		else
		{
			currentmodel = currententity->model;
			if (!currentmodel)
			{
				R_DrawNullModel();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_brush:
				R_DrawBrushModel(currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	glDepthMask(0);		// no z writes
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if (currententity->flags & RF_BEAM)
		{
			R_DrawBeam(currententity);
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_brush:
				R_DrawBrushModel(currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	glDepthMask(1);		// back to writing

}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles()
{
	int32_t i;
	uint8_t color[4] = { 0 };
	const particle_t* p;

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	glPointSize(gl_particle_size->value);

	glBegin(GL_POINTS);
	for (i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++)
	{
		glColor4f(p->color[0] / 255.0f, p->color[1] / 255.0f, p->color[2] / 255.0f, p->color[3] / 255.0f);

		glVertex3fv(p->origin);
	}
	glEnd();

	glDisable(GL_BLEND);
	glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
	glDepthMask(GL_TRUE);
	glEnable(GL_TEXTURE_2D);
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend()
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

	glLoadIdentity();

	// FIXME: get rid of these
	glRotatef(-90, 1, 0, 0);	    // put Z going up
	glRotatef(90, 0, 0, 1);	    // put Z going up

	glColor4fv(v_blend);

	glBegin(GL_QUADS);

	glVertex3f(10, 100, 100);
	glVertex3f(10, -100, 100);
	glVertex3f(10, -100, -100);
	glVertex3f(10, 100, -100);
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);

	glColor4f(1, 1, 1, 1);
}

//=======================================================================

int32_t SignbitsForPlane(cplane_t* out)
{
	int32_t	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j = 0; j < 3; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1 << j;
	}
	return bits;
}


void R_SetFrustum()
{
	int32_t	i;
	int32_t	base = 90;

	// HACK: silly hack to not cull outside the box 
	// TODO: This code is fundamentally broken and does not care about aspect ratio, but it's hacked to work.

	if ((float)r_newrefdef.width / (float)r_newrefdef.height > 1.4f) // 16:9 widescreen
		base = 75;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector(frustum[0].normal, vup, vpn, -(base - r_newrefdef.fov_x / 2));
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector(frustum[1].normal, vup, vpn, base - r_newrefdef.fov_x / 2);
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector(frustum[2].normal, vright, vpn, base - r_newrefdef.fov_y / 2);
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector(frustum[3].normal, vright, vpn, -(base - r_newrefdef.fov_y / 2));

	for (i = 0; i < 4; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct3(r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane(&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame()
{
	int32_t i;
	mleaf_t* leaf;

	r_framecount++;

	// build the transformation matrix for the given view angles
	VectorCopy3(r_newrefdef.vieworigin, r_origin);

	AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

	// current viewcluster
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf(r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy3(r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel);
			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy3(r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel);
			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i = 0; i < 4; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0.3, 0.3, 0.3, 1);
		glScissor(r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // changed from eye-searing hot pink
		glDisable(GL_SCISSOR_TEST);
	}
}


void GL_SetPerspective(GLdouble fovy, GLdouble aspect,
	GLdouble zNear, GLdouble zFar)
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL()
{
	float	screenaspect;
	int32_t	x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = (int32_t)floor(r_newrefdef.x * vid.width / vid.width);
	x2 = (int32_t)ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = (int32_t)floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = (int32_t)ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	glViewport(x, y2, w, h);

	//
	// set up projection matrix
	//
	screenaspect = (float)r_newrefdef.width / r_newrefdef.height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GL_SetPerspective(r_newrefdef.fov_y, screenaspect, 4, gl_drawdistance->value + gl_skyboxepsilon->value);

	glCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(-90, 1, 0, 0);	    // put Z going up
	glRotatef(90, 0, 0, 1);	    // put Z going up
	glRotatef(-r_newrefdef.viewangles[2], 1, 0, 0);
	glRotatef(-r_newrefdef.viewangles[0], 0, 1, 0);
	glRotatef(-r_newrefdef.viewangles[1], 0, 0, 1);
	glTranslatef(-r_newrefdef.vieworigin[0], -r_newrefdef.vieworigin[1], -r_newrefdef.vieworigin[2]);

	glGetFloatv(GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull->value)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
void R_Clear()
{
	// gets rid of the hall of mirrors effect

	if (gl_clear->value)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	else
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	gldepthmin = 0;
	gldepthmax = 1;
	glDepthFunc(GL_LEQUAL);

	glDepthRange(gldepthmin, gldepthmax);
}

void R_Flash(void)
{
	R_PolyBlend();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView(refdef_t* fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		ri.Sys_Error(ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights();

	R_SetupFrame();

	R_SetFrustum();

	R_SetupGL();

	R_MarkLeaves();	// done here so we know if we're in water

	R_DrawWorld();

	R_DrawEntitiesOnList();

	R_RenderDlights();

	R_DrawParticles();

	R_DrawAlphaSurfaces();

	R_Flash();

	if (r_speeds->value)
	{
		ri.Con_Printf(PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys,
			c_alias_polys,
			c_visible_textures,
			c_visible_lightmaps);
	}
}


void R_SetGL2D()
{
	// set 2D virtual screen size
	glViewport(0, 0, vid.width, vid.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, vid.width, vid.height, 0, -99999, 99999);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glColor4f(1, 1, 1, 1);
}

/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel()
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint(r_newrefdef.vieworigin, shadelight);

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

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame(refdef_t* fd)
{
	R_RenderView(fd);
	R_SetLightLevel();
	R_SetGL2D();
}

void R_Register(void)
{
	r_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0);
	r_scaled_assets_basewidth = ri.Cvar_Get("r_scaled_assets_basewidth", "1920", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);

	gl_particle_min_size = ri.Cvar_Get("gl_particle_min_size", "2", CVAR_ARCHIVE);
	gl_particle_max_size = ri.Cvar_Get("gl_particle_max_size", "40", CVAR_ARCHIVE);
	gl_particle_size = ri.Cvar_Get("gl_particle_size", "40", CVAR_ARCHIVE);
	gl_particle_att_a = ri.Cvar_Get("gl_particle_att_a", "0.01", CVAR_ARCHIVE);
	gl_particle_att_b = ri.Cvar_Get("gl_particle_att_b", "0.0", CVAR_ARCHIVE);
	gl_particle_att_c = ri.Cvar_Get("gl_particle_att_c", "0.01", CVAR_ARCHIVE);

	gl_modulate = ri.Cvar_Get("gl_modulate", "1", CVAR_ARCHIVE);
	r_width = ri.Cvar_Get("r_width", "1366", CVAR_ARCHIVE);
	r_height = ri.Cvar_Get("r_height", "768", CVAR_ARCHIVE);
	gl_lightmap = ri.Cvar_Get("gl_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get("gl_shadows", "0", CVAR_ARCHIVE);
	gl_dynamic = ri.Cvar_Get("gl_dynamic", "1", 0);
	gl_round_down = ri.Cvar_Get("gl_round_down", "1", 0);
	gl_picmip = ri.Cvar_Get("gl_picmip", "0", 0);
	gl_skymip = ri.Cvar_Get("gl_skymip", "0", 0);
	gl_showtris = ri.Cvar_Get("gl_showtris", "0", 0);
	gl_clear = ri.Cvar_Get("gl_clear", "1", 0);
	gl_cull = ri.Cvar_Get("gl_cull", "1", 0);
	gl_polyblend = ri.Cvar_Get("gl_polyblend", "1", 0);
	gl_flashblend = ri.Cvar_Get("gl_flashblend", "0", 0);
	gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE);
	gl_texturealphamode = ri.Cvar_Get("gl_texturealphamode", "default", CVAR_ARCHIVE);
	gl_texturesolidmode = ri.Cvar_Get("gl_texturesolidmode", "default", CVAR_ARCHIVE);
	gl_lockpvs = ri.Cvar_Get("gl_lockpvs", "0", 0);

	gl_vertex_arrays = ri.Cvar_Get("gl_vertex_arrays", "0", CVAR_ARCHIVE);

	gl_vsync = ri.Cvar_Get("gl_vsync", "1", CVAR_ARCHIVE);
	gl_ext_compiled_vertex_array = ri.Cvar_Get("gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE);

	gl_drawbuffer = ri.Cvar_Get("gl_drawbuffer", "GL_BACK", 0);

	gl_drawdistance = ri.Cvar_Get("gl_drawdistance", "8192", CVAR_ARCHIVE);
	gl_skyboxepsilon = ri.Cvar_Get("gl_skyboxepsilon", "725", CVAR_ARCHIVE);

	vid_borderless = ri.Cvar_Get("vid_borderless", "0", CVAR_ARCHIVE);
	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);
	vid_ref = ri.Cvar_Get("vid_ref", "gl", CVAR_ARCHIVE);

	game_name = ri.Cvar_Get("game_name", "Euphoria Example", CVAR_LATCH | CVAR_SERVERINFO);
	game_asset_path = ri.Cvar_Get("game_asset_path", "examplegame", CVAR_LATCH | CVAR_SERVERINFO);

	ri.Cmd_AddCommand("imagelist", GL_ImageList_f);
	ri.Cmd_AddCommand("screenshot", GL_ScreenShot_f);
	ri.Cmd_AddCommand("modellist", Mod_Modellist_f);
	ri.Cmd_AddCommand("gl_strings", GL_Strings_f);
}

/*
==================
R_SetMode
==================
*/
bool R_SetMode()
{
	rserr_t err;
	bool fullscreen;

	fullscreen = vid_borderless->value || vid_fullscreen->value;
	vid_borderless->modified = false;
	vid_fullscreen->modified = false;

	// vid.width = r_width->value
	// vid.height = r_height->value
	err = GL_SetMode(&vid.width, &vid.height, fullscreen);

	if (err != rserr_ok)
	{
		if (err == rserr_invalid_fullscreen)
		{
			ri.Cvar_SetValue("vid_borderless", 0);
			vid_borderless->modified = false;
			vid_fullscreen->modified = false;
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n");

			if ((err = GL_SetMode(&vid.width, &vid.height, false)) == rserr_ok)
				return true;
		}
		else if (err == rserr_invalid_mode)
		{
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n");
		}

		// try setting it back to something safe
		if ((err = GL_SetMode(&vid.width, &vid.height, false)) != rserr_ok)
		{
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}

	return true;
}

/*
===============
R_Init
===============
*/
bool R_Init()
{
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int32_t	err;
	int32_t	j;
	extern float r_turbsin[256];

	for (j = 0; j < 256; j++)
	{
		r_turbsin[j] *= 0.5;
	}

	ri.Con_Printf(PRINT_ALL, "ref_gl version: " REF_VERSION"\n");

	R_Register();

	// initialise GLAD and GLFW
	if (!GL_Init())
		return false;

	// create the window and set up the context
	if (!R_SetMode())
	{
		ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n");
		return false;
	}

	GL_SetTextureMode(gl_texturemode->string);

	strcpy(renderer_buffer, gl_config.renderer_string);
	strlwr(renderer_buffer);

	strcpy(vendor_buffer, gl_config.vendor_string);
	strlwr(vendor_buffer);

	gl_config.renderer = GL_RENDERER_OTHER;

	/*
	** grab extensions
	*/
	if (strstr(gl_config.extensions_string, "GL_EXT_compiled_vertex_array"))
	{
		ri.Con_Printf(PRINT_ALL, "...enabling GL_EXT_compiled_vertex_array\n");
	}
	else
	{
		ri.Con_Printf(PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n");
	}

	GL_SetDefaultState();

	GL_InitImages();
	Mod_Init();
	R_InitParticleTexture();
	Draw_InitLocal();

	err = glGetError();
	if (err != GL_NO_ERROR)
		ri.Con_Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);

	return true;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown()
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("gl_strings");

	Mod_FreeAll();

	GL_ShutdownImages();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GL_Shutdown();
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame()
{
	// hunk_alloc called from renderer but ocmpiled from both engine and renderer so if we use an ordinary variable
	// it will always show up as 0 when used from engine
#if DEBUG
	ri.Cvar_SetValue("hunk_total", hunk_total_size);
	ri.Cvar_SetValue("hunk_count", hunk_count);
#endif

	/*
	** change modes if necessary
	*/
	if (vid_borderless->modified
		|| vid_fullscreen->modified)
	{	// FIXME: only restart if CDS is required
		cvar_t* ref;

		ref = ri.Cvar_Get("vid_ref", "gl", 0);
		ref->modified = true;
	}

	if (r_width->modified
		|| r_height->modified)
	{
		GL_SetResolution((int32_t)r_width->value, (int32_t)r_height->value);
		// tell the client/server about it
		ri.Vid_ChangeResolution();
		r_width->modified = false;
		r_height->modified = false;
	}

	// make the gamma not modified
	// TODO: test if this dies
	if (vid_gamma->modified)
		vid_gamma->modified = false;

	GL_BeginFrame();

	/*
	** go into 2D mode
	*/
	glViewport(0, 0, vid.width, vid.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, vid.width, vid.height, 0, -99999, 99999);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glColor4f(1, 1, 1, 1);

	/*
	** draw buffer stuff
	*/
	if (gl_drawbuffer->modified)
	{
		gl_drawbuffer->modified = false;

		if (Q_stricmp(gl_drawbuffer->string, "GL_FRONT") == 0)
			glDrawBuffer(GL_FRONT);
		else
			glDrawBuffer(GL_BACK);
	}

	/*
	** texturemode stuff
	*/
	if (gl_texturemode->modified)
	{
		GL_SetTextureMode(gl_texturemode->string);
		gl_texturemode->modified = false;
	}

	if (gl_texturealphamode->modified)
	{
		GL_SetTextureAlphaMode(gl_texturealphamode->string);
		gl_texturealphamode->modified = false;
	}

	if (gl_texturesolidmode->modified)
	{
		GL_SetTextureSolidMode(gl_texturesolidmode->string);
		gl_texturesolidmode->modified = false;
	}

	//
	// clear screen if desired
	//
	R_Clear();
}

/*
=============
R_EndWorldRenderpass
=============
*/
void R_EndWorldRenderpass(void)
{
	// old OpenGL renderer has only one pass
}

/*
=============
R_SetPalette
=============
*/
uint32_t r_rawpalette[256];

#define NUM_BEAM_SEGS 6
/*
** R_DrawBeam
*/
void R_DrawBeam(entity_t* e)
{


	int32_t	i;
	float	r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize3(normalized_direction) == 0)
		return;

	PerpendicularVector(perpvec, normalized_direction);
	VectorScale3(perpvec, e->frame / 2.0f, perpvec);

	for (i = 0; i < 6; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec, (360.0 / NUM_BEAM_SEGS) * i);
		VectorAdd3(start_points[i], origin, start_points[i]);
		VectorAdd3(start_points[i], direction, end_points[i]);
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	r = 255;
	g = 255;
	b = 255;

	/*
	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;
	*/

	r *= 1 / 255.0F;
	g *= 1 / 255.0F;
	b *= 1 / 255.0F;

	glColor4f(r, g, b, e->alpha);

	glBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i < NUM_BEAM_SEGS; i++)
	{
		glVertex3fv(start_points[i]);
		glVertex3fv(end_points[i]);
		glVertex3fv(start_points[(i + 1) % NUM_BEAM_SEGS]);
		glVertex3fv(end_points[(i + 1) % NUM_BEAM_SEGS]);
	}
	glEnd();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

//===================================================================


void			R_BeginRegistration(const char* map);
struct model_s* R_RegisterModel(const char* name);
struct image_s* R_RegisterSkin(const char* name);
void			R_SetSky(char* name, float rotate, vec3_t axis);
void			R_EndRegistration();

void			R_RenderFrame(refdef_t* fd);

struct image_s* Draw_FindPic(const char* name);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

Returns the functions exported to other dlls
@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI(refimport_t rimp)
{
	refexport_t	re;

	ri = rimp;

	re.api_version = REF_API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.LoadPic = Load_Pic;
	re.DrawPicRegion = Draw_PicRegion;
	re.DrawPicStretch = Draw_PicStretch;
	re.DrawFontChar = Draw_FontChar;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen = Draw_FadeScreen;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GL_EndFrame;
	re.EndWorldRenderpass = R_EndWorldRenderpass;

	re.SetKeyPressedProc = GL_SetKeyPressedProc;
	re.SetMousePressedProc = GL_SetMousePressedProc;
	re.SetMouseScrollProc = GL_SetMouseScrollProc;
	re.SetMouseMovedProc = GL_SetMouseMovedProc;
	re.SetWindowFocusProc = GL_SetWindowFocusProc;
	re.SetWindowIconifyProc = GL_SetWindowIconifyProc;
	re.EnableCursor = GL_EnableCursor;
	re.GetCursorPosition = GL_GetCursorPosition;
	re.SetCursorPosition = GL_SetCursorPosition;
	re.SetWindowPosition = GL_SetWindowPosition;

	Swap_Init();

	return re;
}


#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
#ifdef _MSC_VER
// this gets rid of RetardCompiler warnings
__declspec(noreturn) void Sys_Error(const char* error, ...)
#else
__attribute__((noreturn)) void Sys_Error(const char* error, ...)
#endif
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	vsnprintf(text, 1024, error, argptr);
	va_end(argptr);

	ri.Sys_Error(ERR_FATAL, "%s", text);
}

void Com_Printf(const char* fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	vsnprintf(text, 1024, fmt, argptr);
	va_end(argptr);

	ri.Con_Printf(PRINT_ALL, "%s", text);
}

#endif
