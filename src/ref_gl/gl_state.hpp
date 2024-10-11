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
#include "gl_local.hpp"

// gl_state.h: Holds the OpenGL renderer state and globals

#define		MAX_TEXTURE_SIZE	2048

typedef struct gl_state_s
{
	GLFWwindow* window;					// The GLFW window

	float		inverse_intensity;
	bool		fullscreen;				// Are we in fullscreen?

	int32_t		lightmap_textures;

	int32_t		currenttextures[2];
	int32_t		currenttmu;
} gl_state_t;
