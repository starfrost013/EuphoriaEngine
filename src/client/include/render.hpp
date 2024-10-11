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

#include <client/client.hpp>
#include <cinttypes>

// render_2d.h

#pragma once

void Render_UpdateScreen();

void Render2D_Init();

void Render2D_CenterPrint(char* str);
void Render2D_BeginLoadingPlaque();
void Render2D_EndLoadingPlaque();

void Render2D_DebugGraph(float value, int32_t r, int32_t g, int32_t b, int32_t a);

void Render2D_TouchPics();

void Render2D_RunConsole();
void Render2D_DrawCrosshair();

void Render2D_AddNetgraph();

extern float	scr_con_current;
extern float	scr_conlines;		// lines of console to display

extern int32_t 	sb_lines;

extern cvar_t* scr_viewsize;
extern cvar_t* crosshair;

extern vrect_t	scr_vrect;		// position of render window

extern char		crosshair_pic[MAX_QPATH];
extern int32_t 	crosshair_width, crosshair_height;

void Render2D_AddDirtyPoint(int32_t x, int32_t y);
void Render2D_DirtyScreen();

//
// cl_render_3d.c
//

extern int32_t 			gun_frame;
extern struct model_s*	gun_model;
extern const char*		camera_type_names[];

void Render3D_Init();
void Render3D_PrepRefresh();
void Render3D_RenderView();
void Render3D_AddEntity(entity_t* ent);
void Render3D_AddParticle(vec3_t org, color4_t color);
void Render3D_AddLight(vec3_t org, float intensity, float r, float g, float b);
void Render3D_AddLightStyle(int32_t style, float r, float g, float b);

// Debug stuff for cl_showinfo
extern int32_t r_numdlights;
extern int32_t r_numentities;
extern int32_t r_numparticles;