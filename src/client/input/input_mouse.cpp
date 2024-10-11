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
// cl_input_mouse.c: The mouse input method

#include <client/client.hpp>

// mouse variables

bool mlooking;

void Input_MLookDown()
{ 
	mlooking = true; 
}

void Input_MLookUp()
{
	mlooking = false;
}

int32_t mouse_buttons;
bool	mouse_active;	// false when not focus app
bool	mouse_initialized;
int32_t window_center_x, window_center_y;

/*
===========
Input_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void Input_MouseActivate()
{
	if (!mouse_initialized)
		return;

	if (!input_mouse_enabled->value)
	{
		mouse_active = false;
		return;
	}
	if (mouse_active)
		return;

	// this is how the old code worked
	re.EnableCursor(false);
	mouse_active = true;

	// position is relative to top-left corner of window
	window_center_x = r_width->value / 2;
	window_center_y = r_height->value / 2;
}


/*
===========
Input_DeactivateMouse

Called when the window loses focus
===========
*/
void Input_MouseDeactivate()
{
	if (!mouse_initialized)
		return;
	if (!mouse_active)
		return;

	re.EnableCursor(true);
	mouse_active = false;
}

/*
===========
Input_StartupMouse
===========
*/
void Input_StartupMouse()
{
	cvar_t* cv;

	cv = Cvar_Get("input_initmouse", "1", CVAR_NOSET);
	if (!cv->value)
		return;

	mouse_initialized = true;
	mouse_buttons = 5;
}

/*
===========
Input_MouseMove
===========
*/
void Input_MouseMove(usercmd_t* cmd)
{
	// Add mouse acceleration to predicted viewangles
	// Replaced godawful method made by a retard on April 18, 2024
	if (!mouse_active)
		return;

	// THIS HACK IS ONLY FOR V0.0.10
	// MUST BE REMOVED BY V0.0.11
	if (intro_running)
		return;

	float x_pos = (last_mouse_pos_x - window_center_x) * sensitivity->value;
	float y_pos = (last_mouse_pos_y - window_center_y) * sensitivity->value;

	if ((lookstrafe->value)
		&& mlooking)
	{
		cmd->sidemove += m_side->value * x_pos;
	}
	else
	{
		cl.viewangles[YAW] -= m_yaw->value * x_pos;
	}

	if ((freelook->value)
		&& mlooking)
	{
		cmd->sidemove += m_side->value * y_pos;
	}
	else
	{
		cl.viewangles[PITCH] += m_pitch->value * y_pos;
	}

	if (cl.viewangles[YAW] <= -180) cl.viewangles[YAW] += 360;
	if (cl.viewangles[YAW] >= 180) cl.viewangles[YAW] -= 360;

	// pitch is clamped elsewhere 

	re.SetCursorPosition(window_center_x, window_center_y);
	last_mouse_pos_x = window_center_x;
	last_mouse_pos_y = window_center_y;
}