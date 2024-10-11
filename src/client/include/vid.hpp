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
// vid.h -- video driver defs

typedef struct vrect_s
{
	int32_t	x;
	int32_t y;
	int32_t	width;
	int32_t	height;
} vrect_t;

extern cvar_t* r_width;
extern cvar_t* r_height;
extern bool graphics_mode;

// Video module initialisation etc
void Vid_Init();
void Vid_Shutdown();
void Vid_CheckChanges();