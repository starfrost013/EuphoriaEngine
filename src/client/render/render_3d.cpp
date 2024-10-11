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
// cl_view.c -- player rendering positioning

#include <client/client.hpp>

//=============
//
// development tools for weapons
//
int32_t 		gun_frame;
struct model_s* gun_model;

//=============

cvar_t* crosshair;

extern cvar_t* vid_hudscale;
extern cvar_t* viewsize;

int32_t 		r_numdlights;
dlight_t		r_dlights[MAX_DLIGHTS];

int32_t 		r_numentities;
entity_t		r_entities[MAX_ENTITIES];

int32_t 		r_numparticles;
particle_t		r_particles[MAX_PARTICLES];

lightstyle_t	r_lightstyles[MAX_LIGHTSTYLES];

char			cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int32_t			num_cl_weaponmodels;

// some camera type names ued in various files
const char* camera_type_names[] =
{
	"CAMERA_TYPE_NORMAL",
	"CAMERA_TYPE_CHASE",
	"CAMERA_TYPE_TOPDOWN",
	"CAMERA_TYPE_FREE",
};

/*
====================
V_ClearScene

Clears the scene of all entities, dynamic lights and particles
====================
*/
void Render3D_ClearScene()
{
	r_numdlights = 0;
	r_numentities = 0;
	r_numparticles = 0;
}

/*
=====================
V_AddEntity

Adds an entity.
=====================
*/
void Render3D_AddEntity(entity_t* ent)
{
	if (r_numentities >= MAX_ENTITIES)
	{
		Com_Printf("WARNING: Too many client entities!");
		return;
	}

	r_entities[r_numentities++] = *ent;
}

/*
=====================
V_AddParticle

=====================
*/
void Render3D_AddParticle(vec3_t org, color4_t color)
{
	particle_t* p;

	if (r_numparticles >= MAX_PARTICLES)
	{
		Com_Printf("WARNING: Too many particles!");
		return;
	}

	p = &r_particles[r_numparticles++];
	VectorCopy3(org, p->origin);

	// wtf is wrong with msvc vectorcopy does nothing. KILL THIS COMPILER! DO IT NOW!
	p->color[0] = color[0];
	p->color[1] = color[1];
	p->color[2] = color[2];
	p->color[3] = color[3];
}

/*
=====================
V_AddLight

=====================
*/
void Render3D_AddLight(vec3_t org, float intensity, float r, float g, float b)
{
	dlight_t* dl;

	if (r_numdlights >= MAX_DLIGHTS)
	{
		Com_Printf("WARNING: Too many dynamic lights!");
		return;
	}

	dl = &r_dlights[r_numdlights++];
	VectorCopy3(org, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}


/*
=====================
V_AddLightStyle

=====================
*/
void Render3D_AddLightStyle(int32_t style, float r, float g, float b)
{
	lightstyle_t* ls;

	if (style < 0 || style > MAX_LIGHTSTYLES)
		Com_Error(ERR_DROP, "Bad light style %i", style);

	ls = &r_lightstyles[style];

	ls->white = r + g + b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void Render3D_PrepRefresh()
{
	char	mapname[32];
	int32_t i;
	char	name[MAX_QPATH];
	float	rotate;
	vec3_t	axis;

	if (!cl.configstrings[CS_MODELS + 1][0])
		return;		// no map loaded

	Render2D_AddDirtyPoint(0, 0);
	Render2D_AddDirtyPoint(r_width->value - 1, r_height->value - 1);

	// let the render dll load the map
	strcpy(mapname, cl.configstrings[CS_MODELS + 1] + 5);	// skip "maps/"
	mapname[strlen(mapname) - 4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	Com_Printf("Map: %s\r", mapname);
	Render_UpdateScreen();
	re.BeginRegistration(mapname);
	Com_Printf("                                     \r");

	// precache status bar pics
	Com_Printf("pics\r");
	Render_UpdateScreen();
	Render2D_TouchPics();
	Com_Printf("                                     \r");

	CL_RegisterTEntModels();

	num_cl_weaponmodels = 1;
	strcpy(cl_weaponmodels[0], "weapon.md2");

	for (i = 1; i < MAX_MODELS && cl.configstrings[CS_MODELS + i][0]; i++)
	{
		strcpy(name, cl.configstrings[CS_MODELS + i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
			Com_Printf("%s\r", name);
		Render_UpdateScreen();
		if (name[0] == '#')
		{
			// special player weapon model
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS)
			{
				strncpy(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS + i] + 1,
					sizeof(cl_weaponmodels[num_cl_weaponmodels]) - 1);
				num_cl_weaponmodels++;
			}
		}
		else
		{
			cl.model_draw[i] = re.RegisterModel(cl.configstrings[CS_MODELS + i]);
			if (name[0] == '*')
				cl.model_clip[i] = Map_LoadInlineModel(cl.configstrings[CS_MODELS + i]);
			else
				cl.model_clip[i] = NULL;
		}

		if (name[0] != '*')
			Com_Printf("                                     \r");
	}

	Com_Printf("images\r", i);
	Render_UpdateScreen();

	for (i = 1; i < MAX_IMAGES && cl.configstrings[CS_IMAGES + i][0]; i++)
	{
		cl.image_precache[i] = re.RegisterPic(cl.configstrings[CS_IMAGES + i]);
	}

	Com_Printf("                                     \r");
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS + i][0])
			continue;
		Com_Printf("client %i\r", i);
		Render_UpdateScreen();
		CL_ParseClientinfo(i);
		Com_Printf("                                     \r");
	}

	CL_LoadClientinfo(&cl.baseclientinfo, "unnamed\\male/grunt");

	// set sky textures and speed
	Com_Printf("sky\r", i);
	Render_UpdateScreen();
	rotate = (float)atof(cl.configstrings[CS_SKYROTATE]);
	sscanf(cl.configstrings[CS_SKYAXIS], "%f %f %f",
		&axis[0], &axis[1], &axis[2]);
	re.SetSky(cl.configstrings[CS_SKY], rotate, axis);
	Com_Printf("                                     \r");

	// the renderer can now free unneeded stuff
	re.EndRegistration();

	// clear any lines of console text
	Con_ClearRecentHistory();

	Render_UpdateScreen();
	cl.refresh_prepped = true;
	cl.force_refdef = true;	// make sure we have a valid refdef

	// start the cd track
	int32_t track = atoi(cl.configstrings[CS_CDTRACK]);
	Miniaudio_Play(track, true);
}

/*
====================
CalcFov
====================
*/
float Render3D_CalcFov(float fov_x, float width, float height)
{
	float a;
	float x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error(ERR_DROP, "Bad fov: %f", fov_x);

	// calculate 4:3 fov 

	x = width / tanf(fov_x / 360.0f * M_PI);

	a = atanf(height / x);

	a = a * 360.0f / M_PI;

	// adapt to current aspect ratio (i stole this code from yamagi quake2)

	a = (atanf(tanf(a / 360.0f * M_PI) * (width / height * 0.75f)) / M_PI * 360.0f);

	return a;
}

//============================================================================

// gun frame debugging functions
void Render3D_Gun_Next_f()
{
	gun_frame++;
	Com_Printf("frame %i\n", gun_frame);
}

void Render3D_Gun_Prev_f()
{
	gun_frame--;
	if (gun_frame < 0)
		gun_frame = 0;
	Com_Printf("frame %i\n", gun_frame);
}

void Render3D_Gun_Model_f()
{
	char name[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		gun_model = NULL;
		return;
	}
	snprintf(name, sizeof(name), "models/%s/tris.md2", Cmd_Argv(1));
	gun_model = re.RegisterModel(name);
}

/*
==================
V_RenderView

==================
*/
void Render3D_RenderView()
{
	extern int32_t CompareEntities(const entity_t*, const entity_t*);

	if (cls.state != ca_active)
	{
		re.EndWorldRenderpass();
		return;
	}

	if (!cl.refresh_prepped)
	{
		re.EndWorldRenderpass();
		return;			// still loading
	}

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if (cl.frame.valid && (cl.force_refdef || !cl_paused->value || viewsize->modified))
	{
		cl.force_refdef = false;
		viewsize->modified = false;

		Render3D_ClearScene();

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities();

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworigin[0] += 1.0 / 16;
		cl.refdef.vieworigin[1] += 1.0 / 16;
		cl.refdef.vieworigin[2] += 1.0 / 16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;
		cl.refdef.fov_y = Render3D_CalcFov(cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.time * 0.001f;

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;

		if (!cl_add_particles->value)
			r_numparticles = 0;

		if (!cl_add_lights->value)
			r_numdlights = 0;

		if (!cl_add_blend->value)
			VectorClear3(cl.refdef.blend);

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;
		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;

		cl.refdef.rdflags = cl.frame.playerstate.rdflags;

		// sort entities for better cache locality
		qsort(cl.refdef.entities, cl.refdef.num_entities, sizeof(cl.refdef.entities[0]), (int32_t(*)(const void*, const void*))CompareEntities);
	}

	re.RenderFrame(&cl.refdef);
	if (log_stats->value && (log_stats_file != 0))
		fprintf(log_stats_file, "%i,%i,%i,", r_numentities, r_numdlights, r_numparticles);


	Render2D_AddDirtyPoint(scr_vrect.x, scr_vrect.y);
	Render2D_AddDirtyPoint(scr_vrect.x + scr_vrect.width - 1,
		scr_vrect.y + scr_vrect.height - 1);

	Render2D_DrawCrosshair();
}

/*
=============
Render3D_Init

Initialises stuff that interfaces with the 3d rendering subsystem of the client which interfaces with the renderer
=============
*/
void Render3D_Init()
{
	Cmd_AddCommand("gun_next", Render3D_Gun_Next_f);
	Cmd_AddCommand("gun_prev", Render3D_Gun_Prev_f);
	Cmd_AddCommand("gun_model", Render3D_Gun_Model_f);

	crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
}
