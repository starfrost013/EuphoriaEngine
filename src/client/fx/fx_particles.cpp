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
// fx.c -- entity effects parsing and management

#include <client/client.hpp>

void CL_LogoutEffect(vec3_t org, int32_t type);
void CL_ItemRespawnParticles(vec3_t org);

static vec3_t avelocities[NUM_VERTEX_NORMALS];

/*
==============
CL_ParseMuzzleFlash
==============
*/
void CL_ParseMuzzleFlash()
{
	vec3_t		fv = { 0 }, rv = { 0 };
	cdlight_t*	dl;
	int32_t 	i, weapon;
	centity_t*	pl;
	int32_t 	silenced;
	float		volume;
	char		soundname[64] = { 0 };

	i = MSG_ReadShort(&net_message);
	if (i < 1 || i >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_ParseMuzzleFlash: bad entity %d", i);

	weapon = MSG_ReadByte(&net_message);
	silenced = weapon & MZ_SILENCED;
	weapon &= ~MZ_SILENCED;

	pl = &cl_entities[i];

	dl = CL_AllocDlight(i);
	VectorCopy3(pl->current.origin, dl->origin);
	AngleVectors(pl->current.angles, fv, rv, NULL);
	VectorMA3(dl->origin, 18, fv, dl->origin);
	VectorMA3(dl->origin, 16, rv, dl->origin);
	if (silenced)
		dl->radius = 100 + (rand() & 31);
	else
		dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = cl.time; // + 0.1;

	if (silenced)
		volume = 0.2;
	else
		volume = 1;

	switch (weapon)
	{
	case MZ_BLASTER:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_HYPERBLASTER:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_MACHINEGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case MZ_SHOTGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/shotgr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_SSHOTGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_CHAINGUN1:
		dl->radius = 200 + (rand() & 31);
		dl->color[0] = 1; dl->color[1] = 0.25; dl->color[2] = 0;
		snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case MZ_CHAINGUN2:
		dl->radius = 225 + (rand() & 31);
		dl->color[0] = 1; dl->color[1] = 0.5f; dl->color[2] = 0;
		dl->die = cl.time + 0.1f;	// long delay
		snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.05);
		break;
	case MZ_CHAINGUN3:
		dl->radius = 250 + (rand() & 31);
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 0.1f;	// long delay
		snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.033);
		snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.066);
		break;
	case MZ_RAILGUN:
		dl->color[0] = 0.5; dl->color[1] = 0.5; dl->color[2] = 1.0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_ROCKET:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0.2;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_GRENADE:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_LOGIN:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 1.0f;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect(pl->current.origin, weapon);
		break;
	case MZ_LOGOUT:
		dl->color[0] = 1; dl->color[1] = 0; dl->color[2] = 0;
		dl->die = cl.time + 1.0f;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect(pl->current.origin, weapon);
		break;
	}
}


/*
==============
CL_ParseMuzzleFlash2
==============
*/
void CL_ParseMuzzleFlash2()
{
	int32_t 	ent;
	vec3_t		origin = { 0 };
	int32_t 	flash_number;
	cdlight_t*	dl;
	vec3_t		forward, right;
	char		soundname[64] = { 0 };

	ent = MSG_ReadShort(&net_message);
	if (ent < 1 || ent >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_ParseMuzzleFlash2: bad entity");

	flash_number = MSG_ReadByte(&net_message);

	// locate the origin
	AngleVectors(cl_entities[ent].current.angles, forward, right, NULL);
	origin[0] = cl_entities[ent].current.origin[0] + forward[0] * monster_flash_offset[flash_number][0] + right[0] * monster_flash_offset[flash_number][1];
	origin[1] = cl_entities[ent].current.origin[1] + forward[1] * monster_flash_offset[flash_number][0] + right[1] * monster_flash_offset[flash_number][1];
	origin[2] = cl_entities[ent].current.origin[2] + forward[2] * monster_flash_offset[flash_number][0] + right[2] * monster_flash_offset[flash_number][1] + monster_flash_offset[flash_number][2];

	dl = CL_AllocDlight(ent);
	VectorCopy3(origin, dl->origin);
	dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = cl.time;	// + 0.1;

	switch (flash_number)
	{

	case MZ2_SOLDIER_MACHINEGUN_1:
	{
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		color4_t particle_color2 = { 0, 0, 0, 255 };
		CL_ParticleEffect(origin, vec3_origin, particle_color2, 40);
		CL_SmokeAndFlash(origin);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
		break;
	}	
		

	case MZ2_SOLDIER_BLASTER_1:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SOLDIER_SHOTGUN_1:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		CL_SmokeAndFlash(origin);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
		break;

	}
}

/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

cparticle_t* active_particles, * free_particles;

cparticle_t	particles[MAX_PARTICLES];
int32_t 	cl_numparticles = MAX_PARTICLES;


/*
===============
CL_ClearParticles
===============
*/
void CL_ClearParticles()
{
	int32_t i;

	if (cl_numparticles <= 0)
		return;

	free_particles = &particles[0];
	active_particles = NULL;

	for (i = 0; i < cl_numparticles; i++)
		particles[i].next = &particles[i + 1];
	particles[cl_numparticles - 1].next = NULL;
}


/*
===============
CL_ParticleEffect

Wall impact puffs
===============
*/
void CL_ParticleEffect(vec3_t org, vec3_t dir, color4_t color, int32_t count)
{
	int32_t 		i, j;
	cparticle_t*	p;
	float			d;

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		p->color[0] = color[0] - rand() % 80;
		p->color[1] = color[1] - rand() % 80;
		p->color[2] = color[2] - rand() % 80;
		p->color[3] = 255;

		d = rand() & 31;
		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;

		p->alphavel = -1.0f / (0.5f + frand() * 0.3f);
	}
}


/*
===============
CL_ParticleEffect2

Same as CL_ParticleEffect, but that varies the colour (making it darker by a random amount)
and this doesn't.
===============
*/
void CL_ParticleEffect2(vec3_t org, vec3_t dir, color4_t color, int32_t count)
{
	int32_t 		i, j;
	cparticle_t*	p;
	float			d;

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		VectorCopy3(color, p->color);

		d = rand() & 7;
		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;

		p->alphavel = -1.0f / (0.5f + frand() * 0.3f);
	}
}


/*
===============
CL_TeleporterParticles
===============
*/
void CL_TeleporterParticles(entity_state_t* ent)
{
	int32_t 		i, j;
	cparticle_t*	p;

	for (i = 0; i < 8; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		p->color[0] = 255;
		p->color[1] = 255;
		p->color[2] = 83;
		p->color[3] = 255;

		for (j = 0; j < 2; j++)
		{
			p->org[j] = ent->origin[j] - 16 + (rand() & 31);
			p->vel[j] = crand() * 14;
		}

		p->org[2] = ent->origin[2] - 8 + (rand() & 7);
		p->vel[2] = 80 + (rand() & 7);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;

		p->alphavel = -0.5;
	}
}


/*
===============
CL_LogoutEffect

===============
*/
void CL_LogoutEffect(vec3_t org, int32_t type)
{
	int32_t 		i, j;
	cparticle_t*	p;

	for (i = 0; i < 500; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		// alpha always 255 for this effect
		p->color[3] = 255;

		// color based on what you're actually doing
		if (type == MZ_LOGIN)
		{
			// green (old palette colour 208)
			p->color[0] = 0;
			p->color[1] = 255;
			p->color[2] = 0;
		}
		else if (type == MZ_LOGOUT)
		{
			// red (old palette colour 64)
			p->color[0] = 166;
			p->color[1] = 59;
			p->color[2] = 43;
		}
		else
		{
			// yellow (old pallete colour 224)
			p->color[0] = 254;
			p->color[1] = 171;
			p->color[2] = 7;
		}

		// apply pre-existing adjustment (208 -> 208-214, 64 -> 64-70, 224 -> 224-230)
		// but only if we wouldn't go into the negatives
		if (p->color[0] > 80) p->color[0] -= rand() % 80; else if (p->color[0] > 0) p->color[0] -= rand() % (int32_t)p->color[0];
		if (p->color[1] > 80) p->color[1] -= rand() % 80; else if (p->color[1] > 0)  p->color[1] -= rand() % (int32_t)p->color[1];
		if (p->color[2] > 80) p->color[2] -= rand() % 80; else if (p->color[2] > 0)  p->color[2] -= rand() % (int32_t)p->color[2];

		p->org[0] = org[0] - 16 + frand() * 32;
		p->org[1] = org[1] - 16 + frand() * 32;
		p->org[2] = org[2] - 24 + frand() * 56;

		for (j = 0; j < 3; j++)
			p->vel[j] = crand() * 20;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;

		p->alphavel = -1.0f / (1.0f + frand() * 0.3f);
	}
}


/*
===============
CL_ItemRespawnParticles

===============
*/
void CL_ItemRespawnParticles(vec3_t org)
{
	int32_t 		i, j;
	cparticle_t*	p;

	for (i = 0; i < 64; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		p->color[0] = 96; // no change between 212-214
		p->color[1] = 167 - (rand() % 44);
		p->color[2] = 47 + (rand() % 4);
		p->color[3] = 255;

		p->org[0] = org[0] + crand() * 8;
		p->org[1] = org[1] + crand() * 8;
		p->org[2] = org[2] + crand() * 8;

		for (j = 0; j < 3; j++)
			p->vel[j] = crand() * 8;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 0.2f;

		p->alphavel = -1.0f / (1.0f + frand() * 0.3f);
	}
}


/*
===============
CL_ExplosionParticles
===============
*/
void CL_ExplosionParticles(vec3_t org)
{
	int32_t 		i, j;
	cparticle_t*	p;

	for (i = 0; i < 256; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		p->color[0] = 255 - (rand() & 80);
		p->color[1] = 172 - (rand() & 80);
		p->color[2] = 8 - (rand() & 8);
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;

		p->alphavel = -0.8f / (0.5f + frand() * 0.3f);
	}
}

// Maybe make this configurable?
#define NUM_LIGHTNING_PARTICLES	1024

//
// CL_LightningParticles: Lightning particles for the tangfuslicator
//
void CL_LightningParticles(vec3_t start, float velocity, vec3_t angles)
{
	cparticle_t* particle;

	vec3_t		 particle_movement = { 0 };

	for (int32_t i = 0; i < NUM_LIGHTNING_PARTICLES; i++)
	{
		// we're out of particles
		if (!free_particles)
			return;
		particle = free_particles;
		free_particles = particle->next;
		particle->next = active_particles;
		active_particles = particle;
		particle->time = cl.time + rand() % 500;

		// all colours are whitish
		particle->color[0] = 100 + rand() % 30;
		particle->color[1] = 100 + rand() % 30;
		particle->color[2] = 225 + rand() % 30;
		particle->color[3] = 235 + rand() % 20;

		particle->org[0] = start[0] + 3 + rand() % 5; //3-8 units forward
		particle->org[1] = start[1] + 2 + rand() % 2; //2-4 units side
		particle->org[2] = start[2] + 1 + rand() % 2; //1-3 units high

		particle->vel[0] = (angles[0] * velocity) + rand() % 5; // 100-105
		particle->vel[1] = (angles[1] * velocity) + rand() % 5; // 100-105
		particle->vel[2] = (angles[2] * velocity) + rand() % 5; // 100-105

		particle->accel[0] = (angles[0]) * 5 + rand() % 5;
		particle->accel[1] = (angles[1]) * 5 + rand() % 5;
		particle->accel[2] = (angles[2]) * 5 + rand() % 5;

		particle->lifetime = 4500;
	}
}

void CL_LightningParticlesAttachedToEntity(vec3_t start, vec3_t angles)
{
	CL_LightningParticles(start, 0, angles);
}

/*
===============
CL_BlasterParticles

Wall impact puffs
===============
*/
void CL_BlasterParticles(vec3_t org, vec3_t dir)
{
	int32_t 		i, j;
	cparticle_t*	p;
	float			d;
	int32_t 		count;

	count = 40;
	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		p->color[0] = 255 - (rand() & 80);
		p->color[1] = 172 - (rand() & 80);
		p->color[2] = 8 - (rand() & 8);
		p->color[3] = 255;

		d = rand() & 15;
		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = dir[j] * 30 + crand() * 40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;

		p->alphavel = -1.0f / (0.5f + frand() * 0.3f);
	}
}


/*
===============
CL_BlasterTrail

===============
*/
void CL_BlasterTrail(vec3_t start, vec3_t end)
{
	vec3_t			move = { 0 };
	vec3_t			vec = { 0 };
	float			len;
	int32_t 		j;
	cparticle_t*	p;
	int32_t 		dec;

	VectorCopy3(start, move);
	VectorSubtract3(end, start, vec);
	len = VectorNormalize3(vec);

	dec = 5;
	VectorScale3(vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear3(p->accel);

		p->time = cl.time;

		p->alphavel = -1.0f / (0.3f + frand() * 0.2f);

		p->color[0] = 255 - (rand() & 80);
		p->color[1] = 172 - (rand() & 80);
		p->color[2] = 8 - (rand() & 8);
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd3(move, vec, move);
	}
}

/*
===============
CL_QuadTrail
===============
*/
void CL_QuadTrail(vec3_t start, vec3_t end)
{
	vec3_t			move = { 0 };
	vec3_t			vec = { 0 };
	float			len;
	int32_t 		j;
	cparticle_t*	p;
	int32_t 		dec;

	VectorCopy3(start, move);
	VectorSubtract3(end, start, vec);
	len = VectorNormalize3(vec);

	dec = 5;
	VectorScale3(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear3(p->accel);

		p->time = cl.time;

		p->alphavel = -1.0f / (0.8f + frand() * 0.2f);

		p->color[0] = 47;
		p->color[1] = 103;
		p->color[2] = 126;
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 16;
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd3(move, vec, move);
	}
}

/*
===============
CL_DiminishingTrail

===============
*/
void CL_DiminishingTrail(vec3_t start, vec3_t end, centity_t* old, int32_t flags)
{
	vec3_t			move = { 0 };
	vec3_t			vec = { 0 };
	float			len;
	int32_t 		j;
	cparticle_t*	p;
	float			dec;
	float			orgscale;
	float			velscale;

	VectorCopy3(start, move);
	VectorSubtract3(end, start, vec);
	len = VectorNormalize3(vec);

	dec = 0.5;
	VectorScale3(vec, dec, vec);

	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;

		// drop less particles as it flies
		if ((rand() & 1023) < old->trailcount)
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;
			VectorClear3(p->accel);

			p->time = cl.time;

			if (flags & EF_GIB)
			{
				p->alphavel = -1.0f / (1.0f + frand() * 0.4f);

				p->color[0] = 154 - (rand() % 80);
				p->color[1] = 31 - (rand() % 31);
				p->color[2] = 0;


				for (j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alphavel = -1.0f / (1.0f + frand() * 0.2f);

				p->color[0] = 63 + (rand() % 108);
				p->color[1] = 63 + (rand() % 108);
				p->color[2] = 63 + (rand() % 108);

				for (j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
				}
				p->accel[2] = 20;
			}

			// always set alpha to 255 for this effect
			p->color[3] = 255;
		}

		old->trailcount -= 5;
		if (old->trailcount < 100)
			old->trailcount = 100;
		VectorAdd3(move, vec, move);
	}
}

void MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up)
{
	float d;

	// this rotate and negat guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct3(right, forward);
	VectorMA3(right, -d, forward, right);
	VectorNormalize3(right);
	VectorCrossProduct(right, forward, up);
}

/*
===============
CL_RocketTrail

===============
*/
void CL_RocketTrail(vec3_t start, vec3_t end, centity_t* old)
{
	vec3_t			move = { 0 };
	vec3_t			vec = { 0 };
	float			len;
	int32_t 		j;
	cparticle_t*	p;
	float			dec;

	// smoke
	CL_DiminishingTrail(start, end, old, EF_ROCKET);

	// fire
	VectorCopy3(start, move);
	VectorSubtract3(end, start, vec);
	len = VectorNormalize3(vec);

	dec = 1;
	VectorScale3(vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;

		if ((rand() & 7) == 0)
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;

			VectorClear3(p->accel);
			p->time = cl.time;

			p->alphavel = -1.0f / (1 + frand() * 0.2f);

			p->color[0] = 255;
			p->color[1] = 255 - (rand() % 68);
			p->color[2] = 39 - (rand() % 24);
			p->color[3] = 255;

			for (j = 0; j < 3; j++)
			{
				p->org[j] = move[j] + crand() * 5;
				p->vel[j] = crand() * 20;
			}
			p->accel[2] = -PARTICLE_GRAVITY;
		}
		VectorAdd3(move, vec, move);
	}
}

/*
===============
CL_RailTrail

===============
*/
void CL_RailTrail(vec3_t start, vec3_t end)
{
	vec3_t			move = { 0 };
	vec3_t			vec = { 0 };
	float			len;
	int32_t 		j;
	cparticle_t* p;
	float			dec;
	vec3_t			right, up;
	int32_t 		i;
	float			d, c, s;
	vec3_t			dir;

	VectorCopy3(start, move);
	VectorSubtract3(end, start, vec);
	len = VectorNormalize3(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++)
	{
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		VectorClear3(p->accel);

		d = i * 0.1f;
		c = cosf(d);
		s = sinf(d);

		VectorScale3(right, c, dir);
		VectorMA3(dir, s, up, dir);

		p->alphavel = -1.0f / (1 + frand() * 0.2f);

		p->color[0] = 23 - (rand() % 23);
		p->color[1] = 83 - (rand() % 50);
		p->color[2] = 111 - (rand() % 50);
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + dir[j] * 3;
			p->vel[j] = dir[j] * 6;
		}

		VectorAdd3(move, vec, move);
	}

	dec = 0.75;
	VectorScale3(vec, dec, vec);
	VectorCopy3(start, move);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		VectorClear3(p->accel);

		p->alphavel = -1.0f / (0.6f + frand() * 0.2f);

		// generate 0-255 gray
		p->color[0] = 0 + (rand() & 256);
		p->color[1] = 0 + (rand() & 256);
		p->color[2] = 0 + (rand() & 256);
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 3;
			p->vel[j] = crand() * 3;
			p->accel[j] = 0;
		}

		VectorAdd3(move, vec, move);
	}
}

/*
===============
CL_BubbleTrail

===============
*/
void CL_BubbleTrail(vec3_t start, vec3_t end)
{
	vec3_t			move = { 0 };
	vec3_t			vec = { 0 };
	float			len;
	int32_t			i, j;
	cparticle_t*	p;
	float			dec;

	VectorCopy3(start, move);
	VectorSubtract3(end, start, vec);
	len = VectorNormalize3(vec);

	dec = 32;
	VectorScale3(vec, dec, vec);

	for (i = 0; i < len; i += dec)
	{
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear3(p->accel);
		p->time = cl.time;

		p->alphavel = -1.0f / (1.0f + frand() * 0.2f);

		p->color[0] = 63 + (rand() % 108);
		p->color[1] = 63 + (rand() % 108);
		p->color[2] = 63 + (rand() % 108);
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 2;
			p->vel[j] = crand() * 5;
		}
		p->vel[2] += 6;

		VectorAdd3(move, vec, move);
	}
}


/*
===============
CL_FlyParticles
===============
*/

#define	BEAMLENGTH			16
void CL_FlyParticles(vec3_t origin, int32_t count)
{
	int32_t 		i;
	cparticle_t*	p;
	float			angle;
	float			sr, sp, sy, cr, cp, cy;
	vec3_t			forward = { 0 };
	float			dist = 64;
	float			ltime;


	if (count > NUM_VERTEX_NORMALS)
		count = NUM_VERTEX_NORMALS;

	if (!avelocities[0][0])
	{
		for (i = 0; i < NUM_VERTEX_NORMALS; i++)
		{
			avelocities[i][0] = (rand() & 255) * 0.01f;
			avelocities[i][1] = (rand() & 255) * 0.01f;
			avelocities[i][2] = (rand() & 255) * 0.01f;
		}
	}


	ltime = (float)cl.time / 1000.0;
	for (i = 0; i < count; i += 2)
	{
		angle = ltime * avelocities[i][0];
		sy = sinf(angle);
		cy = cosf(angle);
		angle = ltime * avelocities[i][1];
		sp = sinf(angle);
		cp = cosf(angle);
		angle = ltime * avelocities[i][2];
		sr = sinf(angle);
		cr = cosf(angle);

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		dist = sinf(ltime + i) * 64.0f;
		p->org[0] = origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;

		VectorClear3(p->vel);
		VectorClear3(p->accel);

		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->color[3] = 255;

		p->colorvel = 0;

		p->alphavel = -100;
	}
}

void CL_FlyEffect(centity_t* ent, vec3_t origin)
{
	int32_t n;
	int32_t count;
	int32_t starttime;

	if (ent->fly_stoptime < cl.time)
	{
		starttime = cl.time;
		ent->fly_stoptime = cl.time + 60000;
	}
	else
	{
		starttime = ent->fly_stoptime - 60000;
	}

	n = cl.time - starttime;
	if (n < 20000)
		count = n * 162 / 20000.0f;
	else
	{
		n = ent->fly_stoptime - cl.time;
		if (n < 20000)
			count = n * 162 / 20000.0f;
		else
			count = 162;
	}

	CL_FlyParticles(origin, count);
}

/*
===============
CL_TeleportParticles

===============
*/
void CL_TeleportParticles(vec3_t org)
{
	int32_t 		i, j, k;
	cparticle_t*	p;
	float			vel;
	vec3_t			dir = { 0 };

	for (i = -16; i <= 16; i += 4)
	{
		for (j = -16; j <= 16; j += 4)
		{
			for (k = -16; k <= 32; k += 4)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;

				p->time = cl.time;

				// legacy colour 8-15
				p->color[0] = 107 + rand() % 148;
				p->color[1] = 107 + rand() % 148;
				p->color[2] = 107 + rand() % 148;
				p->color[3] = 255;

				p->alphavel = -1.0f / (0.3f + (rand() & 7) * 0.02f);
		
				p->org[0] = org[0] + i + (rand() & 3);
				p->org[1] = org[1] + j + (rand() & 3);
				p->org[2] = org[2] + k + (rand() & 3);

				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				VectorNormalize3(dir);
				vel = 50 + (rand() & 63);
				VectorScale3(dir, vel, p->vel);

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
		}
	}
}


/*
===============
CL_AddParticles
===============
*/
void CL_AddParticles()
{
	cparticle_t*	p, *next;
	float			time = 0, time2;
	vec3_t			org = { 0 };
	cparticle_t*	activated, *tail;

	activated = NULL;
	tail = NULL;

	for (p = active_particles; p; p = next)
	{
		bool despawn = false;

		next = p->next;

		// PMM - added INSTANT_PARTICLE handling for heat beam
		if (p->alphavel != INSTANT_PARTICLE
			&& !p->permanent)
		{
			time = (cl.time - p->time) * 0.001f;

			// failsafe to prevent particles never despawning
			if (p->alphavel == 0
				&& p->lifetime == 0)
			{
				// fade by default after 10 seconds
				p->alphavel = 0.1f;
			}

			p->color[3] = p->color[3] + time * p->alphavel;

			if (p->color[3] <= 0)
				despawn = true;

			if (p->lifetime > 0)
			{
				// did the particle expire yet?
				if (cl.time - p->time > p->lifetime)
					despawn = true;
			}
			
			if (despawn)
			{
				// faded out or expired, get rid of it
				p->next = free_particles;
				free_particles = p;
				continue;
			}
		}

		p->next = NULL;

		if (!tail)
		{
			activated = tail = p;
		}
		else
		{
			tail->next = p;
			tail = p;
		}

		if (p->color[0] > 255) p->color[0] = 255;
		if (p->color[1] > 255) p->color[1] = 255;
		if (p->color[2] > 255) p->color[2] = 255;
		if (p->color[3] > 255) p->color[3] = 255;

		time2 = time * time;

		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;

		Render3D_AddParticle(org, p->color);

		if (p->alphavel == INSTANT_PARTICLE)
		{
			p->alphavel = 0.0;
			p->color[3] = 0;
		}
	}

	active_particles = activated;
}

//=============
//=============
void CL_Flashlight(int32_t ent, vec3_t pos)
{
	cdlight_t* dl;

	dl = CL_AllocDlight(ent);
	VectorCopy3(pos, dl->origin);
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = cl.time + 100;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}

/*
======
CL_ColorFlash - flash of light
======
*/
void CL_ColorFlash(int32_t ent, vec3_t pos, int32_t intensity, color4_t color)
{
	cdlight_t* dl;

	dl = CL_AllocDlight(ent);
	VectorCopy3(pos, dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = cl.time + 100;
	dl->color[0] = color[0];
	dl->color[1] = color[1];
	dl->color[2] = color[2];
}

void CL_FlameEffects(centity_t* ent, vec3_t origin)
{
	int32_t 		n, count;
	int32_t 		j;
	cparticle_t*	p;

	count = rand() & 0xF;

	for (n = 0; n < count; n++)
	{
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear3(p->accel);
		p->time = cl.time;

		p->alphavel = -1.0f / (1 + frand() * 0.2f);
		p->color[0] = 239 - (rand() % 56);
		p->color[1] = 127 - (rand() % 68);
		p->color[2] = 1; // always 1 lmao
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + crand() * 5;
			p->vel[j] = crand() * 5;
		}
		p->vel[2] = crand() * -10;
		p->accel[2] = -PARTICLE_GRAVITY;
	}

	count = rand() & 0x7;

	for (n = 0; n < count; n++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear3(p->accel);

		p->time = cl.time;

		p->alphavel = -1.0f / (1 + frand() * 0.5f);

		p->color[0] = 0 + (rand() % 63);
		p->color[1] = 0 + (rand() % 63);
		p->color[2] = 0 + (rand() % 63);
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + crand() * 3;
		}
		p->vel[2] = 20 + crand() * 5;
	}

}


/*
===============
CL_GenericParticleEffect
===============
*/
void CL_GenericParticleEffect(vec3_t org, vec3_t dir, color4_t color, int32_t count, color4_t run, int32_t dirspread, vec3_t velocity, int32_t lifetime, float alphavel)
{
	int32_t 		i, j; 
	cparticle_t*	p;
	float			d;

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->lifetime = lifetime;

		VectorAdd3(color, run, p->color);

		d = rand() & dirspread;
		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];

			if (velocity == vec3_origin)
				p->vel[j] = crand() * 20;
			else
				p->vel[j] = velocity[j] + (crand() * 20);
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->color[3] = 255;
		p->alphavel = alphavel;
	}
}

/*
===============
CL_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===============
*/
void CL_ParticleSteamEffect(vec3_t org, vec3_t dir, color4_t color, int32_t count, int32_t magnitude)
{
	int32_t 		i, j;
	cparticle_t*	p;
	float			d;
	vec3_t			r, u;

	MakeNormalVectors(dir, r, u);

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		// works in most cases for now
		p->color[0] = color[0] - (rand() & 50);
		p->color[1] = color[1] - (rand() & 50);
		p->color[2] = color[2] - (rand() & 50);
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + magnitude * 0.1f * crand();
		}

		VectorScale3(dir, magnitude, p->vel);
		d = crand() * magnitude / 3;
		VectorMA3(p->vel, d, r, p->vel);
		d = crand() * magnitude / 3;
		VectorMA3(p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY / 2;

		p->alphavel = -1.0f / (0.5f + frand() * 0.3f);
	}
}

/*
===============
CL_ParticleSmokeEffect - like the steam effect, but unaffected by gravity
===============
*/
void CL_ParticleSmokeEffect(vec3_t org, vec3_t dir, color4_t color, int32_t count, int32_t magnitude)
{
	int32_t 		i, j;
	cparticle_t*	p;
	float			d;
	vec3_t			r, u;

	MakeNormalVectors(dir, r, u);

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color[0] = color[0] - rand() % 80;
		p->color[1] = color[1] - rand() % 80;
		p->color[2] = color[2] - rand() % 80;
		p->color[3] = 255;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + magnitude * 0.1f * crand();
		}
		VectorScale3(dir, magnitude, p->vel);
		d = crand() * magnitude / 3;
		VectorMA3(p->vel, d, r, p->vel);
		d = crand() * magnitude / 3;
		VectorMA3(p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = p->accel[2] = 0;

		p->alphavel = -1.0f / (0.5f + frand() * 0.3f);
	}
}


/*
==============
CL_EntityEvent

An entity has just been parsed that has an event value

the female events are there for backwards compatability
==============
*/
extern struct sfx_s* cl_sfx_footsteps[4];

void CL_EntityEvent(entity_state_t* ent)
{
	switch (ent->event)
	{
	case EV_ITEM_RESPAWN:
		S_StartSound(NULL, ent->number, CHAN_WEAPON, S_RegisterSound("items/respawn1.wav"), 1, ATTN_IDLE, 0);
		CL_ItemRespawnParticles(ent->origin);
		break;
	case EV_PLAYER_TELEPORT:
		S_StartSound(NULL, ent->number, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_IDLE, 0);
		CL_TeleportParticles(ent->origin);
		break;
	case EV_FOOTSTEP:
		if (cl_footsteps->value)
			S_StartSound(NULL, ent->number, CHAN_BODY, cl_sfx_footsteps[rand() & 3], 1, ATTN_NORM, 0);
		break;
	case EV_FALLSHORT:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALL:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("*fall2.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALLFAR:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("*fall1.wav"), 1, ATTN_NORM, 0);
		break;
	}
}


/*
==============
CL_ClearEffects

==============
*/
void CL_ClearEffects()
{
	CL_ClearParticles();
	CL_ClearDlights();
	CL_ClearLightStyles();
}
