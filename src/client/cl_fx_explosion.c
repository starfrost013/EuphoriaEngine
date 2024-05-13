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
// cl_fx_explosion.c - explod!
#include "client.h"

/*
=================
CL_AddExplosions
=================
*/
void CL_AddExplosions(void)
{
	entity_t* ent;
	int32_t 		i;
	explosion_t* ex;
	float		frac;
	int32_t 		f;

	memset(&ent, 0, sizeof(ent));

	for (i = 0, ex = cl_explosions; i < MAX_EXPLOSIONS; i++, ex++)
	{
		if (ex->type == ex_free)
			continue;
		frac = (cl.time - ex->start) / 100.0;
		f = floor(frac);

		ent = &ex->ent;

		switch (ex->type)
		{
		case ex_mflash:
			if (f >= ex->frames - 1)
				ex->type = ex_free;
			break;
		case ex_misc:
			if (f >= ex->frames - 1)
			{
				ex->type = ex_free;
				break;
			}
			ent->alpha = 1.0 - frac / (ex->frames - 1);
			break;
		case ex_flash:
			if (f >= 1)
			{
				ex->type = ex_free;
				break;
			}
			ent->alpha = 1.0;
			break;
		case ex_poly:
			if (f >= ex->frames - 1)
			{
				ex->type = ex_free;
				break;
			}

			ent->alpha = (16.0 - (float)f) / 16.0;

			if (f < 10)
			{
				ent->skinnum = (f >> 1);
				if (ent->skinnum < 0)
					ent->skinnum = 0;
			}
			else
			{
				ent->flags |= RF_TRANSLUCENT;
				if (f < 13)
					ent->skinnum = 5;
				else
					ent->skinnum = 6;
			}
			break;
		case ex_poly2:
			if (f >= ex->frames - 1)
			{
				ex->type = ex_free;
				break;
			}

			ent->alpha = (5.0 - (float)f) / 5.0;
			ent->skinnum = 0;
			ent->flags |= RF_TRANSLUCENT;
			break;
		default:
			break;
		}

		if (ex->type == ex_free)
			continue;
		if (ex->light)
		{
			V_AddLight(ent->origin, ex->light * ent->alpha,
				ex->lightcolor[0], ex->lightcolor[1], ex->lightcolor[2]);
		}

		VectorCopy(ent->origin, ent->oldorigin);

		if (f < 0)
			f = 0;
		ent->frame = ex->baseframe + f + 1;
		ent->oldframe = ex->baseframe + f;
		ent->backlerp = 1.0 - cl.lerpfrac;

		V_AddEntity(ent);
	}
}

/*
=================
CL_AllocExplosion
=================
*/
explosion_t* CL_AllocExplosion(void)
{
	int32_t 	i;
	int32_t 	time;
	int32_t 	index;

	for (i = 0; i < MAX_EXPLOSIONS; i++)
	{
		if (cl_explosions[i].type == ex_free)
		{
			memset(&cl_explosions[i], 0, sizeof(cl_explosions[i]));
			return &cl_explosions[i];
		}
	}
	// find the oldest explosion
	time = cl.time;
	index = 0;

	for (i = 0; i < MAX_EXPLOSIONS; i++)
		if (cl_explosions[i].start < time)
		{
			time = cl_explosions[i].start;
			index = i;
		}
	memset(&cl_explosions[index], 0, sizeof(cl_explosions[index]));
	return &cl_explosions[index];
}

/*
=================
CL_SmokeAndFlash
=================
*/
void CL_SmokeAndFlash(vec3_t origin)
{
	explosion_t* ex;

	ex = CL_AllocExplosion();
	VectorCopy(origin, ex->ent.origin);
	ex->type = ex_misc;
	ex->frames = 4;
	ex->ent.flags = RF_TRANSLUCENT;
	ex->start = cl.frame.servertime - 100;
	ex->ent.model = cl_mod_smoke;

	ex = CL_AllocExplosion();
	VectorCopy(origin, ex->ent.origin);
	ex->type = ex_flash;
	ex->ent.flags = RF_FULLBRIGHT;
	ex->frames = 2;
	ex->start = cl.frame.servertime - 100;
	ex->ent.model = cl_mod_flash;
}
