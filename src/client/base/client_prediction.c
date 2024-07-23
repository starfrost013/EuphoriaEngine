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

// client_prediction.c: Client prediction

#include <client/client.h>

/*
===================
CL_CheckPredictionError
===================
*/
void CL_CheckPredictionError()
{
	int32_t frame;
	int32_t delta[3];
	int32_t i;
	int32_t len;

	if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (CMD_BACKUP - 1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract(cl.frame.playerstate.vieworigin, cl.predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear(cl.prediction_error);
	}
	else
	{
		if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]))
			Com_Printf("prediction miss on %i: %i\n", cl.frame.serverframe,
				delta[0] + delta[1] + delta[2]);

		VectorCopy(cl.frame.playerstate.vieworigin, cl.predicted_origins[frame]);

		// save for error itnerpolation
		for (i = 0; i < 3; i++)
			cl.prediction_error[i] = delta[i];
	}
}


/*
====================
CL_ClipMoveToEntities

====================
*/
void CL_ClipMoveToEntities(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t* tr)
{
	int32_t 		i, x, zd, zu;
	trace_t		trace;
	int32_t 		headnode;
	float* angles;
	entity_state_t* ent;
	int32_t 		num;
	cmodel_t* cmodel;
	vec3_t		bmins, bmaxs;

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cl.playernum + 1)
			continue;

		if (ent->solid == 31)
		{	// special value for bmodel
			cmodel = cl.model_clip[ent->modelindex];
			if (!cmodel)
				continue;
			headnode = cmodel->headnode;
			angles = ent->angles;
		}
		else
		{	// encoded bbox
			x = 8 * (ent->solid & 31);
			zd = 8 * ((ent->solid >> 5) & 31);
			zu = 8 * ((ent->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headnode = Map_HeadnodeForBox(bmins, bmaxs);
			angles = vec3_origin;	// boxes don't rotate
		}

		if (tr->allsolid)
			return;

		trace = Map_TransformedBoxTrace(start, end,
			mins, maxs, headnode, MASK_PLAYERSOLID,
			ent->origin, angles);

		if (trace.allsolid || trace.startsolid ||
			trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_s*)ent;
			if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
				*tr = trace;
		}
		else if (trace.startsolid)
			tr->startsolid = true;
	}
}


/*
================
CL_PMTrace
================
*/
trace_t	 CL_PMTrace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t	t;

	// check against world
	t = Map_BoxTrace(start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s*)1;

	// check all other solid models
	CL_ClipMoveToEntities(start, mins, maxs, end, &t);

	return t;
}

int32_t  CL_PMpointcontents(vec3_t point)
{
	int32_t 		i;
	entity_state_t* ent;
	int32_t 		num;
	cmodel_t* cmodel;
	int32_t 		contents;

	contents = Map_PointContents(point, 0);

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (ent->solid != 31) // special value for bmodel
			continue;

		cmodel = cl.model_clip[ent->modelindex];
		if (!cmodel)
			continue;

		contents |= Map_TransformedPointContents(point, cmodel->headnode, ent->origin, ent->angles);
	}

	return contents;
}


/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CL_PredictMovement()
{
	int32_t 	ack, current;
	int32_t 	frame;
	int32_t 	oldframe;
	usercmd_t*	cmd;
	pmove_t		pm;
	int32_t 	i;
	int32_t 	step;
	int32_t 	oldz;

	if (cls.state != ca_active)
		return;

	if (cl_paused->value)
		return;

	if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// just set angles
		for (i = 0; i < 3; i++)
		{
			cl.predicted_angles[i] = cl.viewangles[i] + SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[i]);
		}
		return;
	}

	ack = cls.netchan.incoming_acknowledged;
	current = cls.netchan.outgoing_sequence;

	// if we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP)
	{
		if (cl_showmiss->value)
			Com_Printf("exceeded CMD_BACKUP\n");
		return;
	}

	// copy current state to pmove
	memset(&pm, 0, sizeof(pm));
	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMpointcontents;

	phys_stopspeed = atof(cl.configstrings[CS_PHYS_STOPSPEED]);
	phys_maxspeed_player = atof(cl.configstrings[CS_PHYS_MAXSPEED_PLAYER]);
	phys_maxspeed_director = atof(cl.configstrings[CS_PHYS_MAXSPEED_DIRECTOR]);
	phys_duckspeed = atof(cl.configstrings[CS_PHYS_DUCKSPEED]);
	phys_accelerate_player = atof(cl.configstrings[CS_PHYS_ACCELERATE_PLAYER]);
	phys_accelerate_director = atof(cl.configstrings[CS_PHYS_ACCELERATE_DIRECTOR]);
	phys_airaccelerate = atof(cl.configstrings[CS_PHYS_ACCELERATE_AIR]);
	phys_wateraccelerate = atof(cl.configstrings[CS_PHYS_ACCELERATE_WATER]);
	phys_friction = atof(cl.configstrings[CS_PHYS_FRICTION]);
	phys_waterfriction = atof(cl.configstrings[CS_PHYS_FRICTION_WATER]);

	VectorCopy(cl.frame.playerstate.vieworigin, pm.vieworigin);
	pm.s = cl.frame.playerstate.pmove;

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (CMD_BACKUP - 1);
		cmd = &cl.cmds[frame];

		pm.cmd = *cmd;
		Player_Move(&pm);

		// save for debug checking
		VectorCopy(pm.vieworigin, cl.predicted_origins[frame]);
	}

	oldframe = (ack - 2) & (CMD_BACKUP - 1);
	oldz = cl.predicted_origins[oldframe][2];
	step = pm.vieworigin[2] - oldz;

	if (step > 63 && step < 160 && (pm.s.pm_flags & PMF_ON_GROUND))
	{
		cl.predicted_step = step;
		cl.predicted_step_time = cls.realtime - cls.frametime * 500;
	}

	// copy results out for rendering

	cl.predicted_origin[0] = pm.vieworigin[0];
	cl.predicted_origin[1] = pm.vieworigin[1];
	cl.predicted_origin[2] = pm.vieworigin[2];

	VectorCopy(pm.viewangles, cl.predicted_angles);
}