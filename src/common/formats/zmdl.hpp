#pragma once
/*
Euphoria Game Engine
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

#include <cstdint>

//
// Zmdl.hpp - ZOmbono Model (even though it's dead) skeletal animation format
// October 14, 2024
//

#define ZMDL_HEADER		"ZMDL"
#define ZMDL_VERSION	1

typedef enum zmdl_section_id
{
	// total length 
	// etc
	zmdl_section_settings = 0,

	// tree
	//    bonecontroller
	//			|
	//			|
	//			|
	//      bone pivot
	//			|
	//			|
	//			|
	//		  vertex
	zmdl_section_skeleton = 1,

	zmdl_section_tris = 2,

	zmdl_section_textures = 3,

	// <list of keyframes> 
	// <position>
	// <delta of animation from previous keyframe>
	zmdl_section_animation = 4,


} zmdl_section_id;

// ZMDL_HEADER
typedef struct zmdl_header_s
{
	char		magic[4];
	uint8_t		version;
} zmdl_header_t;