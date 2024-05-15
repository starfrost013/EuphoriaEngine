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

struct sfx_s;

void S_Init ();
void S_Shutdown ();

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_StartSound (vec3_t origin, int32_t entnum, int32_t entchannel, struct sfx_s *sfx, float fvol,  float attenuation, float timeofs);
void S_StartLocalSound (char *s);

void S_StopAllSounds();
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);

void S_Activate (bool activated);

void S_BeginRegistration ();
struct sfx_s *S_RegisterSound (char *sample);
void S_EndRegistration ();

struct sfx_s *S_FindName (char *name, bool create);

// the sound code makes callbacks to the client for entitiy position
// information, so entities can be dynamically re-spatialized
void CL_GetEntitySoundOrigin (int32_t ent, vec3_t org);
