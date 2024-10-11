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
// fx_dlight.c - dynamic lighting
#include <client/client.hpp>

/*
==============================================================

DLIGHT MANAGEMENT

==============================================================
*/

cdlight_t		cl_dlights[MAX_DLIGHTS];

/*
================
CL_ClearDlights
================
*/
void CL_ClearDlights()
{
	memset(cl_dlights, 0, sizeof(cl_dlights));
}

/*
===============
CL_AllocDlight

===============
*/
cdlight_t* CL_AllocDlight(int32_t key)
{
	int32_t 	i;
	cdlight_t* dl;

	// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->key == key)
			{
				memset(dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset(dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset(dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

/*
===============
CL_NewDlight
===============
*/
void CL_NewDlight(int32_t key, float x, float y, float z, float radius, float time)
{
	cdlight_t* dl;

	dl = CL_AllocDlight(key);
	dl->origin[0] = x;
	dl->origin[1] = y;
	dl->origin[2] = z;
	dl->radius = radius;
	dl->die = cl.time + time;
}


/*
===============
CL_RunDLights

===============
*/
void CL_RunDLights()
{
	int32_t 		i;
	cdlight_t* dl;

	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
			continue;

		if (dl->die < cl.time)
		{
			dl->radius = 0;
			return;
		}
		dl->radius -= cls.frametime * dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
===============
CL_AddDLights

===============
*/
void CL_AddDLights()
{
	int32_t 		i;
	cdlight_t* dl;

	dl = cl_dlights;

	if (vidref_val == VIDREF_GL)
	{
		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (!dl->radius)
				continue;
			Render3D_AddLight(dl->origin, dl->radius,
				dl->color[0], dl->color[1], dl->color[2]);
		}
	}
	else
	{
		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (!dl->radius)
				continue;

			// negative light in software. only black allowed
			if ((dl->color[0] < 0) || (dl->color[1] < 0) || (dl->color[2] < 0))
			{
				dl->radius = -(dl->radius);
				dl->color[0] = 1;
				dl->color[1] = 1;
				dl->color[2] = 1;
			}
			Render3D_AddLight(dl->origin, dl->radius,
				dl->color[0], dl->color[1], dl->color[2]);
		}
	}
}
