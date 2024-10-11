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
// fx_beam.c - beams and player beams
#include <client/client.hpp>

beam_t		cl_beams[MAX_BEAMS];

//PMM - added this for player-linked beams.  Currently only used by the plasma beam
beam_t		cl_playerbeams[MAX_BEAMS];

/*
=================
CL_AddBeams
=================
*/
void CL_AddBeams()
{
	int32_t 		i, j;
	beam_t* b;
	vec3_t		dist, org;
	float		d;
	entity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	float		model_length;

	// update beams
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl.playernum + 1)	// entity 0 is the world
		{
			VectorCopy3(cl.refdef.vieworigin, b->start);
			b->start[2] -= 22;	// adjust for view height
		}
		VectorAdd3(b->start, b->offset, org);

		// calculate pitch and yaw
		VectorSubtract3(b->end, org, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2f(dist[1], dist[0]) * 180.0f / M_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0)
				yaw += 360;

			forward = sqrtf(dist[0] * dist[0] + dist[1] * dist[1]);
			pitch = (atan2f(dist[2], forward) * -180.0f / M_PI);
			if (pitch < 0)
				pitch += 360.0;
		}

		// add new entities for the beams
		d = VectorNormalize3(dist);

		memset(&ent, 0, sizeof(ent));

		// you can put hacks in here for different entities lol

		model_length = 30.0;

		steps = ceilf(d / model_length);
		len = (d - model_length) / (steps - 1);

		while (d > 0)
		{
			VectorCopy3(org, ent.origin);
			ent.model = b->model;

			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = rand() % 360;

			// Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			Render3D_AddEntity(&ent);

			for (j = 0; j < 3; j++)
				org[j] += dist[j] * len;
			d -= model_length;
		}
	}
}

extern cvar_t* hand;

/*
=================
ROGUE - draw player locked beams
CL_AddPlayerBeams
=================
*/
void CL_AddPlayerBeams()
{
	int32_t 		i, j;
	beam_t* b;
	vec3_t			dist = { 0 }, org = { 0 };
	float			d;
	entity_t		ent;
	float			yaw, pitch;
	float			forward;
	float			len, steps;
	int32_t 		framenum = 0;
	float			model_length;

	float			hand_multiplier;

	if (hand)
	{
		if (hand->value == 2)
			hand_multiplier = 0;
		else if (hand->value == 1)
			hand_multiplier = -1;
		else
			hand_multiplier = 1;
	}
	else
	{
		hand_multiplier = 1;
	}


	// update beams
	for (i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl.playernum + 1)	// entity 0 is the world
		{
			VectorCopy3(cl.refdef.vieworigin, b->start);
			b->start[2] -= 22;	// adjust for view height
		}
		VectorAdd3(b->start, b->offset, org);

		// calculate pitch and yaw
		VectorSubtract3(b->end, org, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2f(dist[1], dist[0]) * 180 / M_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0)
				yaw += 360;

			forward = sqrtf(dist[0] * dist[0] + dist[1] * dist[1]);
			pitch = (atan2f(dist[2], forward) * -180.0 / M_PI);
			if (pitch < 0)
				pitch += 360.0;
		}

		// add new entities for the beams
		d = VectorNormalize3(dist);

		memset(&ent, 0, sizeof(ent));

		model_length = 30.0;

		steps = ceilf(d / model_length);
		len = (d - model_length) / (steps - 1);

		// special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if (d <= model_length)
		{
			VectorCopy3(b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
//			for (j=0 ; j<3 ; j++)
//				ent.origin[j] -= dist[j]*10.0;
			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT;
			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = rand() % 360;
			Render3D_AddEntity(&ent);
			return;
		}
		while (d > 0)
		{
			VectorCopy3(org, ent.origin);
			ent.model = b->model;

			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = rand() % 360;

			//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			Render3D_AddEntity(&ent);

			for (j = 0; j < 3; j++)
				org[j] += dist[j] * len;
			d -= model_length;
		}
	}
}


/*
=================
CL_ParseBeam
=================
*/
int32_t CL_ParseBeam(struct model_s* model)
{
	int32_t 	ent;
	vec3_t	start, end;
	beam_t* b;
	int32_t 	i;

	ent = MSG_ReadShort(&net_message);

	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// override any beam with the same entity
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy3(start, b->start);
			VectorCopy3(end, b->end);
			VectorClear3(b->offset);
			return ent;
		}

	// find a free beam
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy3(start, b->start);
			VectorCopy3(end, b->end);
			VectorClear3(b->offset);
			return ent;
		}
	}
	Com_Printf("beam list overflow!\n");
	return ent;
}

/*
=================
CL_ParseBeam2
=================
*/
int32_t CL_ParseBeam2(struct model_s* model)
{
	int32_t 	ent;
	vec3_t	start, end, offset;
	beam_t* b;
	int32_t 	i;

	ent = MSG_ReadShort(&net_message);

	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);
	MSG_ReadPos(&net_message, offset);

	//	Com_Printf ("end- %f %f %f\n", end[0], end[1], end[2]);

	// override any beam with the same entity

	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy3(start, b->start);
			VectorCopy3(end, b->end);
			VectorCopy3(offset, b->offset);
			return ent;
		}

	// find a free beam
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy3(start, b->start);
			VectorCopy3(end, b->end);
			VectorCopy3(offset, b->offset);
			return ent;
		}
	}
	Com_Printf("beam list overflow!\n");
	return ent;
}

/*
=================
CL_ParsePlayerBeam
  - adds to the cl_playerbeam array instead of the cl_beams array
=================
*/
int32_t CL_ParsePlayerBeam(struct model_s* model)
{
	int32_t 	ent;
	vec3_t	start, end, offset;
	beam_t* b;
	int32_t 	i;

	ent = MSG_ReadShort(&net_message);

	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// revmoed hardcoded positions (these were used for network optimisation)
	MSG_ReadPos(&net_message, offset);

	//	Com_Printf ("end- %f %f %f\n", end[0], end[1], end[2]);

	// override any beam with the same entity
	// For player beams, we only want one per player (entity) so..
	for (i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++)
	{
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy3(start, b->start);
			VectorCopy3(end, b->end);
			VectorCopy3(offset, b->offset);
			return ent;
		}
	}

	// find a free beam
	for (i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 100;		// PMM - this needs to be 100 to prevent multiple heatbeams
			VectorCopy3(start, b->start);
			VectorCopy3(end, b->end);
			VectorCopy3(offset, b->offset);
			return ent;
		}
	}
	Com_Printf("beam list overflow!\n");
	return ent;
}

/*
=================
CL_ParseLightning
=================
*/
int32_t CL_ParseLightning(struct model_s* model)
{
	int32_t 	srcEnt, destEnt;
	vec3_t	start, end;
	beam_t* b;
	int32_t 	i;

	srcEnt = MSG_ReadShort(&net_message);
	destEnt = MSG_ReadShort(&net_message);

	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// override any beam with the same source AND destination entities
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
		if (b->entity == srcEnt && b->dest_entity == destEnt)
		{
			//			Com_Printf("%d: OVERRIDE  %d -> %d\n", cl.time, srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy3(start, b->start);
			VectorCopy3(end, b->end);
			VectorClear3(b->offset);
			return srcEnt;
		}

	// find a free beam
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			//			Com_Printf("%d: NORMAL  %d -> %d\n", cl.time, srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy3(start, b->start);
			VectorCopy3(end, b->end);
			VectorClear3(b->offset);
			return srcEnt;
		}
	}
	Com_Printf("beam list overflow!\n");
	return srcEnt;
}

