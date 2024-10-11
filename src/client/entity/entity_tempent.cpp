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
// cl_tent.c -- client side temporary entities

#include <client/client.hpp>

extern void CL_TeleportParticles(vec3_t org);

void CL_BlasterParticles(vec3_t org, vec3_t dir);
void CL_ExplosionParticles(vec3_t org);

struct sfx_s* cl_sfx_ric1;
struct sfx_s* cl_sfx_ric2;
struct sfx_s* cl_sfx_ric3;
struct sfx_s* cl_sfx_lashit;
struct sfx_s* cl_sfx_spark5;
struct sfx_s* cl_sfx_spark6;
struct sfx_s* cl_sfx_spark7;
struct sfx_s* cl_sfx_railg;
struct sfx_s* cl_sfx_rockexp;
struct sfx_s* cl_sfx_grenexp;
struct sfx_s* cl_sfx_watrexp;
struct sfx_s* cl_sfx_plasexp;
struct sfx_s* cl_sfx_footsteps[4];

struct model_s* cl_mod_explode;
struct model_s* cl_mod_smoke;
struct model_s* cl_mod_flash;
struct model_s* cl_mod_explo4;
struct model_s* cl_mod_plasmaexplo;

/*
=================
CL_RegisterTEntSounds
=================
*/
void CL_RegisterTEntSounds()
{
	int32_t i;
	char	name[MAX_QPATH];

	cl_sfx_ric1 = S_RegisterSound("world/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound("world/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound("world/ric3.wav");
	cl_sfx_lashit = S_RegisterSound("weapons/lashit.wav");
	cl_sfx_spark5 = S_RegisterSound("world/spark5.wav");
	cl_sfx_spark6 = S_RegisterSound("world/spark6.wav");
	cl_sfx_spark7 = S_RegisterSound("world/spark7.wav");
	cl_sfx_railg = S_RegisterSound("weapons/railgf1a.wav");
	cl_sfx_rockexp = S_RegisterSound("weapons/rocklx1a.wav");
	cl_sfx_grenexp = S_RegisterSound("weapons/grenlx1a.wav");
	cl_sfx_watrexp = S_RegisterSound("weapons/xpld_wat.wav");

	S_RegisterSound("player/land1.wav");

	S_RegisterSound("player/fall2.wav");
	S_RegisterSound("player/fall1.wav");

	for (i = 0; i < 4; i++)
	{
		snprintf(name, sizeof(name), "player/step%i.wav", i + 1);
		cl_sfx_footsteps[i] = S_RegisterSound(name);
	}
}

/*
=================
CL_RegisterTEntModels
=================
*/
void CL_RegisterTEntModels()
{
	cl_mod_explode = re.RegisterModel("models/objects/explode/tris.md2");
	cl_mod_smoke = re.RegisterModel("models/objects/smoke/tris.md2");
	cl_mod_flash = re.RegisterModel("models/objects/flash/tris.md2");
	cl_mod_explo4 = re.RegisterModel("models/objects/r_explode/tris.md2");

	re.RegisterModel("models/objects/laser/tris.md2");
	re.RegisterModel("models/objects/grenade2/tris.md2");
	re.RegisterModel("models/weapons/v_machn/tris.md2");
	re.RegisterModel("models/weapons/v_handgr/tris.md2");
	re.RegisterModel("models/weapons/v_shotg2/tris.md2");
	re.RegisterModel("models/objects/gibs/bone/tris.md2");
	re.RegisterModel("models/objects/gibs/sm_meat/tris.md2");
	re.RegisterModel("models/objects/gibs/bone2/tris.md2");

	re.RegisterPic("w_machinegun");
	re.RegisterPic("a_bullets");
	re.RegisterPic("i_health");
	re.RegisterPic("a_grenades");
}

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts()
{
	memset(cl_beams, 0, sizeof(cl_beams));
	memset(cl_explosions, 0, sizeof(cl_explosions));
	memset(cl_playerbeams, 0, sizeof(cl_playerbeams));
}


/*
=================
CL_ParseTEnt
=================
*/
static color4_t splash_color[] =
{
	{ 0, 0, 0, 0 },
	{ 254, 171, 7, 255 },
	{ 118, 123, 207, 255 },
	{ 123, 95, 74, 255 },
	{ 0, 255, 0, 255 },
	{ 254, 171, 7, 255 },
	{ 156, 31, 1, 255 },
};

void CL_ParseTEnt()
{
	int32_t 		type;
	vec3_t			pos = { 0 }, pos2 = { 0 }, pos3 = { 0 }, dir = { 0 };
	explosion_t* ex;
	color4_t		color;
	int32_t			cnt = 0, r = 0, ent = 0, i1 = 0, i2 = 0, i3 = 0;
	float			f1 = 0.0f, f2 = 0.0f, f3 = 0.0f;

	color4_t legacy_colour_0 = { 0, 0, 0, 255 };
	color4_t legacy_colour_b0 = { 118, 123, 207, 255 };
	color4_t legacy_colour_d0 = { 0, 255, 0, 255 };
	color4_t legacy_colour_df = { 254, 191, 15, 255 };
	color4_t legacy_colour_e0 = { 254, 171, 7, 255 };
	color4_t legacy_colour_e8 = { 156, 31, 1, 255 };

	type = MSG_ReadByte(&net_message);

	switch (type)
	{
	case TE_GENERIC:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		MSG_ReadColor(&net_message, color);
		i1 = MSG_ReadInt(&net_message);
		MSG_ReadPos(&net_message, pos2);
		i2 = MSG_ReadInt(&net_message);
		MSG_ReadPos(&net_message, pos3);
		i3 = MSG_ReadInt(&net_message);
		f1 = MSG_ReadFloat(&net_message);

		CL_GenericParticleEffect(pos, dir, color, i1, pos2, i2, pos3, i3, f1);
		break;

	case TE_BLOOD:			// bullet hitting flesh
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		CL_ParticleEffect(pos, dir, legacy_colour_e8, 60);
		break;

	case TE_GUNSHOT:			// bullet hitting wall
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		if (type == TE_GUNSHOT)
			CL_ParticleEffect(pos, dir, legacy_colour_0, 40);
		else
			CL_ParticleEffect(pos, dir, legacy_colour_e0, 6);

		if (type != TE_SPARKS)
		{
			CL_SmokeAndFlash(pos);

			// impact sound
			cnt = rand() & 15;
			if (cnt == 1)
				S_StartSound(pos, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (cnt == 2)
				S_StartSound(pos, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else if (cnt == 3)
				S_StartSound(pos, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}

		break;

	case TE_SHIELD_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		CL_ParticleEffect(pos, dir, legacy_colour_b0, 40);

		//FIXME : replace or remove this sound
		S_StartSound(pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_SHOTGUN:			// bullet hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		CL_ParticleEffect(pos, dir, legacy_colour_0, 20);
		CL_SmokeAndFlash(pos);
		break;

	case TE_SPLASH:			// bullet hitting water
		cnt = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		r = MSG_ReadByte(&net_message);
		if (r > 6)
		{

			color[0] = legacy_colour_0[0];
			color[1] = legacy_colour_0[1];
			color[2] = legacy_colour_0[2];
			color[3] = legacy_colour_0[3];
		}
		else
		{
			color[0] = splash_color[r][0];
			color[1] = splash_color[r][1];
			color[2] = splash_color[r][2];
			color[3] = splash_color[r][3];
		}
		CL_ParticleEffect(pos, dir, color, cnt);

		if (r == SPLASH_SPARKS)
		{
			r = rand() & 3;
			if (r == 0)
				S_StartSound(pos, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0);
			else if (r == 1)
				S_StartSound(pos, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0);
			else
				S_StartSound(pos, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0);
		}
		break;

	case TE_LASER_SPARKS:
		cnt = MSG_ReadShort(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		MSG_ReadColor(&net_message, color);

		CL_ParticleEffect2(pos, dir, color, cnt);
		break;

	case TE_BLASTER:			// blaster hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		CL_BlasterParticles(pos, dir);

		ex = CL_AllocExplosion();
		VectorCopy3(pos, ex->ent.origin);
		ex->ent.angles[0] = acosf(dir[2]) / M_PI * 180.0f;
		// PMM - fixed to correct for pitch of 0
		if (dir[0])
			ex->ent.angles[1] = atan2f(dir[1], dir[0]) / M_PI * 180.0f;
		else if (dir[1] > 0)
			ex->ent.angles[1] = 90;
		else if (dir[1] < 0)
			ex->ent.angles[1] = 270;
		else
			ex->ent.angles[1] = 0;

		ex->type = ex_misc;
		ex->ent.flags = RF_FULLBRIGHT | RF_TRANSLUCENT;
		ex->start = cl.frame.servertime - 100;
		ex->light = 150;
		ex->lightcolor[0] = 1;
		ex->lightcolor[1] = 1;
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;
		S_StartSound(pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_RAILTRAIL:			// railgun effect
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);
		CL_RailTrail(pos, pos2);
		S_StartSound(pos2, 0, 0, cl_sfx_railg, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION2:
	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
		MSG_ReadPos(&net_message, pos);

		ex = CL_AllocExplosion();
		VectorCopy3(pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = cl.frame.servertime - 100;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.model = cl_mod_explo4;
		ex->frames = 19;
		ex->baseframe = 30;
		ex->ent.angles[1] = rand() % 360;
		CL_ExplosionParticles(pos);
		if (type == TE_GRENADE_EXPLOSION_WATER)
			S_StartSound(pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound(pos, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION1:
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
		MSG_ReadPos(&net_message, pos);

		ex = CL_AllocExplosion();
		VectorCopy3(pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = cl.frame.servertime - 100;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		ex->ent.model = cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		CL_ExplosionParticles(pos);
		if (type == TE_ROCKET_EXPLOSION_WATER)
			S_StartSound(pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound(pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_BUBBLETRAIL:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);
		CL_BubbleTrail(pos, pos2);
		break;

	case TE_TELEPORT:
		MSG_ReadPos(&net_message, pos);

		CL_TeleportParticles(pos);
		break;

	case TE_LIGHTNING:
		MSG_ReadPos(&net_message, pos);
		f1 = MSG_ReadFloat(&net_message);
		MSG_ReadDir(&net_message, dir);

		CL_LightningParticles(pos, f1, dir);
		break;

	case TE_SMOKE:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		MSG_ReadColor(&net_message, color);
		i1 = MSG_ReadInt(&net_message);
		i2 = MSG_ReadInt(&net_message);

		CL_ParticleSmokeEffect(pos, dir, color, i1, i2);
		break;

	case TE_STEAM:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		MSG_ReadColor(&net_message, color);
		i1 = MSG_ReadInt(&net_message);
		i2 = MSG_ReadInt(&net_message);

		CL_ParticleSteamEffect(pos, dir, color, i1, i2);
		break;

	case TE_FLAME:
		ent = MSG_ReadShort(&net_message);
		MSG_ReadPos(&net_message, pos);

		CL_FlameEffects((centity_t*)ent, pos);
		break;

	case TE_FLASHLIGHT:
		ent = MSG_ReadShort(&net_message);
		MSG_ReadPos(&net_message, pos);

		CL_Flashlight(ent, pos);
		break;

	case TE_FLASH: // e.g. Flashbang
		ent = MSG_ReadShort(&net_message);
		MSG_ReadPos(&net_message, pos);
		i1 = MSG_ReadInt(&net_message);
		MSG_ReadColor(&net_message, color);

		CL_ColorFlash(ent, pos, i1, pos2);
		break;

	default:
		Com_Error(ERR_DROP, "CL_ParseTEnt: bad type %d", type);
	}
}

/*
=================
CL_AddTEnts
=================
*/
void CL_AddTEnts()
{
	CL_AddBeams();
	CL_AddPlayerBeams();
	CL_AddExplosions();
}
