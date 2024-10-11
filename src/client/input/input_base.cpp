/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018-2019 Krzysztof Kondrak
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
// cl_input.c  -- builds an intended movement command to send to the server

#include <client/client.hpp>

cvar_t* cl_nodelta;
cvar_t* input_mouse_enabled;

extern uint32_t	sys_frame_time;
uint32_t		frame_msec;
uint32_t		old_sys_frame_time;
extern bool		mouse_initialized;

extern void Input_MLookDown();
extern void Input_MLookUp();


// ==============
// INITIALISATION
// ==============

void Input_Init()
{
	// mouse variables
	input_mouse_enabled = Cvar_Get("input_mouse_enabled", "1", CVAR_ARCHIVE);

	Cmd_AddCommand("+mlook", Input_MLookDown);
	Cmd_AddCommand("-mlook", Input_MLookUp);

	Input_StartupMouse();
}

// ========= 
// Activation, Mainloop, Shutdown
// =========

/*
===========
Input_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void Input_Activate(bool activated)
{
	mouse_active = !activated;		// force a new window check or turn off
}

/*
===========
Input_Shutdown
===========
*/
void Input_Shutdown()
{
	Input_MouseDeactivate();
}

/*
==================
Input_Frame

Called every frame, even if not generating commands
==================
*/
void Input_Frame()
{
	if (!mouse_initialized)
		return;

	if (!input_mouse_enabled)
	{
		Input_MouseDeactivate();
		return;
	}

	if (!cl.refresh_prepped
		|| cls.input_dest == key_console
		|| cls.input_dest == key_menu)
	{
		// temporarily deactivate if in windowed
		if (Cvar_VariableValue("vid_borderless") == 0
			&& Cvar_VariableValue("vid_fullscreen") == 0)
		{
			Input_MouseDeactivate();
			return;
		}
	}

	Input_MouseActivate();
}

/*
===========
Input_Move
 Don't remove this function - more input types are intended to be added
===========
*/
void Input_Move(usercmd_t* cmd)
{
	Input_MouseMove(cmd);
}



/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack1, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition


Key_Event (int32_t key, bool down, uint32_t time);

  +mlook src time

===============================================================================
*/


kbutton_t	input_left, input_right, input_forward, input_back;
kbutton_t	input_moveleft, input_moveright;
kbutton_t	input_sprint, input_use, input_attack1, input_attack2;
kbutton_t	input_up, input_down;

int32_t 	input_impulse;


void Input_KeyDown(kbutton_t* b)
{
	int32_t 	k;
	char* c;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Com_Printf("Three keys down for a button!\n");
		return;
	}

	if (b->state & 1)
		return;		// still down

	// save timestamp
	c = Cmd_Argv(2);
	b->downtime = atoi(c);
	if (!b->downtime)
		b->downtime = sys_frame_time - 100;

	b->state |= 1 + 2;	// down + impulse down
}

void Input_KeyUp(kbutton_t* b)
{
	int32_t 	k;
	char* c;
	uint32_t	uptime;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	// save timestamp
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if (uptime)
		b->msec += uptime - b->downtime;
	else
		b->msec += 10;

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void Input_UpDown() { Input_KeyDown(&input_up); }
void Input_UpUp() { Input_KeyUp(&input_up); }
void Input_DownDown() { Input_KeyDown(&input_down); }
void Input_DownUp() { Input_KeyUp(&input_down); }
void Input_LeftDown() { Input_KeyDown(&input_left); }
void Input_LeftUp() { Input_KeyUp(&input_left); }
void Input_RightDown() { Input_KeyDown(&input_right); }
void Input_RightUp() { Input_KeyUp(&input_right); }
void Input_ForwardDown() { Input_KeyDown(&input_forward); }
void Input_ForwardUp() { Input_KeyUp(&input_forward); }
void Input_BackDown() { Input_KeyDown(&input_back); }
void Input_BackUp() { Input_KeyUp(&input_back); }
void Input_MoveleftDown() { Input_KeyDown(&input_moveleft); }
void Input_MoveleftUp() { Input_KeyUp(&input_moveleft); }
void Input_MoverightDown() { Input_KeyDown(&input_moveright); }
void Input_MoverightUp() { Input_KeyUp(&input_moveright); }

void Input_SprintDown() { Input_KeyDown(&input_sprint); }
void Input_SprintUp() { Input_KeyUp(&input_sprint); }

void Input_Attack1Down() { Input_KeyDown(&input_attack1); }
void Input_Attack1Up() { Input_KeyUp(&input_attack1); }
void Input_Attack2Down() { Input_KeyDown(&input_attack2); }
void Input_Attack2Up() { Input_KeyUp(&input_attack2); }

void Input_UseDown() { Input_KeyDown(&input_use); }
void Input_UseUp() { Input_KeyUp(&input_use); }

void Input_Impulse() { input_impulse = atoi(Cmd_Argv(1)); }

/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState(kbutton_t* key)
{
	float		val;
	int32_t 		msec;

	key->state &= 1;		// clear impulses

	msec = key->msec;
	key->msec = 0;

	if (key->state)
	{	// still down
		msec += sys_frame_time - key->downtime;

		key->downtime = sys_frame_time;
	}

	val = (float)msec / frame_msec;
	if (val < 0)
		val = 0;
	if (val > 1)
		val = 1;

	return val;
}

//==========================================================================

cvar_t* cl_upspeed;
cvar_t* cl_forwardspeed;
cvar_t* cl_sidespeed;

cvar_t* cl_yawspeed;
cvar_t* cl_pitchspeed;

cvar_t* cl_run;

cvar_t* cl_anglespeedkey;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles()
{
	float speed;

	if (input_sprint.state & 1)
		speed = cls.frametime * cl_anglespeedkey->value;
	else
		speed = cls.frametime;

	cl.viewangles[YAW] -= speed * cl_yawspeed->value * CL_KeyState(&input_right);
	cl.viewangles[YAW] += speed * cl_yawspeed->value * CL_KeyState(&input_left);
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove(usercmd_t* cmd)
{
	CL_AdjustAngles();

	memset(cmd, 0, sizeof(*cmd));

	VectorCopy3(cl.viewangles, cmd->angles);

	cmd->sidemove += cl_sidespeed->value * CL_KeyState(&input_moveright);
	cmd->sidemove -= cl_sidespeed->value * CL_KeyState(&input_moveleft);

	cmd->upmove += cl_upspeed->value * CL_KeyState(&input_up);
	cmd->upmove -= cl_upspeed->value * CL_KeyState(&input_down);

	cmd->forwardmove += cl_forwardspeed->value * CL_KeyState(&input_forward);
	cmd->forwardmove -= cl_forwardspeed->value * CL_KeyState(&input_back);

	//
	// adjust for speed key / running
	//
	if ((input_sprint.state & 1) ^ (int32_t)(cl_run->value))
	{
		cmd->forwardmove *= 2;
		cmd->sidemove *= 2;
		cmd->upmove *= 2;
	}
}

void CL_ClampPitch()
{
	float	pitch;

	pitch = SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
	if (pitch > 180)
		pitch -= 360;

	if (cl.viewangles[PITCH] + pitch < -360)
		cl.viewangles[PITCH] += 360; // wrapped
	if (cl.viewangles[PITCH] + pitch > 360)
		cl.viewangles[PITCH] -= 360; // wrapped

	if (cl.viewangles[PITCH] + pitch > 89)
		cl.viewangles[PITCH] = 89 - pitch;
	if (cl.viewangles[PITCH] + pitch < -89)
		cl.viewangles[PITCH] = -89 - pitch;
}

/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove(usercmd_t* cmd)
{
	int32_t ms;
	int32_t i;

	//
	// figure button bits
	//	
	if (input_attack1.state & 3)
		cmd->buttons |= BUTTON_ATTACK1;
	input_attack1.state &= ~2;

	if (input_attack2.state & 3)
		cmd->buttons |= BUTTON_ATTACK2;
	input_attack2.state &= ~2;

	if (input_use.state & 3)
		cmd->buttons |= BUTTON_USE;
	input_use.state &= ~2;

	if (anykeydown && cls.input_dest == key_game)
		cmd->buttons |= BUTTON_ANY;

	// send milliseconds of time to apply the move
	ms = cls.frametime * 1000;
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = ms;

	CL_ClampPitch();
	for (i = 0; i < 3; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);

	cmd->impulse = input_impulse;
	input_impulse = 0;

	// send the ambient light level at the player's current position
	cmd->lightlevel = (uint8_t)cl_lightlevel->value;
}

/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd()
{
	usercmd_t	cmd;

	frame_msec = sys_frame_time - old_sys_frame_time;

	if (frame_msec < 1)
		frame_msec = 1;
	if (frame_msec > 200)
		frame_msec = 200;

	// get basic movement from keyboard
	CL_BaseMove(&cmd);

	// allow mice or other external controllers to add to the move
	Input_Move(&cmd);

	CL_FinishMove(&cmd);

	old_sys_frame_time = sys_frame_time;

	return cmd;
}

/*
============
CL_InitInput
============
*/
void CL_InitInput()
{
	Cmd_AddCommand("+jump", Input_UpDown);
	Cmd_AddCommand("-jump", Input_UpUp);
	Cmd_AddCommand("+crouch", Input_DownDown);
	Cmd_AddCommand("-crouch", Input_DownUp);
	Cmd_AddCommand("+left", Input_LeftDown);
	Cmd_AddCommand("-left", Input_LeftUp);
	Cmd_AddCommand("+right", Input_RightDown);
	Cmd_AddCommand("-right", Input_RightUp);
	Cmd_AddCommand("+forward", Input_ForwardDown);
	Cmd_AddCommand("-forward", Input_ForwardUp);
	Cmd_AddCommand("+back", Input_BackDown);
	Cmd_AddCommand("-back", Input_BackUp);
	Cmd_AddCommand("+moveleft", Input_MoveleftDown);
	Cmd_AddCommand("-moveleft", Input_MoveleftUp);
	Cmd_AddCommand("+moveright", Input_MoverightDown);
	Cmd_AddCommand("-moveright", Input_MoverightUp);
	Cmd_AddCommand("+sprint", Input_SprintDown);
	Cmd_AddCommand("-sprint", Input_SprintUp);
	Cmd_AddCommand("+attack1", Input_Attack1Down);
	Cmd_AddCommand("-attack1", Input_Attack1Up);
	Cmd_AddCommand("+attack2", Input_Attack2Down);
	Cmd_AddCommand("-attack2", Input_Attack2Up);
	Cmd_AddCommand("+use", Input_UseDown);
	Cmd_AddCommand("-use", Input_UseUp);
	Cmd_AddCommand("impulse", Input_Impulse);

	cl_nodelta = Cvar_Get("cl_nodelta", "0", 0);
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd()
{
	sizebuf_t	buf;
	uint8_t		data[128];
	int32_t 	i;
	usercmd_t*	cmd, * oldcmd;
	usercmd_t	nullcmd;
	int32_t 	checksumIndex;

	memset(&buf, 0, sizeof(buf));

	// build a command even if not connected

	// save this command off for prediction
	i = cls.netchan.outgoing_sequence & (CMD_BACKUP - 1);
	cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	*cmd = CL_CreateCmd();

	cl.cmd = *cmd;

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
		return;

	if (cls.state == ca_connected)
	{
		if (cls.netchan.message.cursize || curtime - cls.netchan.last_sent > 1000)
			Netchan_Transmit(&cls.netchan, 0, buf.data);
		return;
	}

	// send a userinfo update if needed
	if (userinfo_modified)
	{
		CL_FixUpGender();
		userinfo_modified = false;
		MSG_WriteByte(&cls.netchan.message, clc_userinfo);
		MSG_WriteString(&cls.netchan.message, Cvar_Userinfo());
	}

	SZ_Init(&buf, data, sizeof(data));

	// begin a client move command
	MSG_WriteByte(&buf, clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	MSG_WriteByte(&buf, 0);

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if (cl_nodelta->value || !cl.frame.valid || cls.demowaiting)
		MSG_WriteInt(&buf, -1);	// no compression
	else
		MSG_WriteInt(&buf, cl.frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (cls.netchan.outgoing_sequence - 2) & (CMD_BACKUP - 1);
	cmd = &cl.cmds[i];
	memset(&nullcmd, 0, sizeof(nullcmd));
	MSG_WriteDeltaUsercmd(&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence - 1) & (CMD_BACKUP - 1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd(&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & (CMD_BACKUP - 1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd(&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = Com_BlockSequenceCRCByte(
		buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		cls.netchan.outgoing_sequence);

	//
	// deliver the message
	//
	Netchan_Transmit(&cls.netchan, buf.cursize, buf.data);
}
