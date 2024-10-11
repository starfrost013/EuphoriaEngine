/*
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

// cl_ui_eventhandling.c : Split UI Event Handling from UI Code (December 27, 2023)
#include <client/client.hpp>

bool UI_SetEventOnClickDown(const char* ui_name, const char* control_name, void (*func)(int32_t btn, int32_t x, int32_t y))
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("WARNING: Tried to set UI click-down event callback for unknown control %s for UI %s!", control_name, ui_name);
		return false;
	}

	if (!func)
	{
		Com_Printf("WARNING: Tried to set UI click-down event callback for control %s on UI %s to null!", control_name, ui_name);
		return false;
	}

	ui_control_ptr->on_click_down = func;
	return true;
}

bool UI_SetEventOnClickUp(const char* ui_name, const char* control_name, void (*func)(int32_t btn, int32_t x, int32_t y))
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("WARNING: Tried to set cUI click-up event callback for unknown control %s for UI %s!", control_name, ui_name);
		return false;
	}

	if (!func)
	{
		Com_Printf("WARNING: Tried to set UI click-up event callback for control %s on UI %s to null!", control_name, ui_name);
		return false;
	}

	ui_control_ptr->on_click_up = func;
	return true;
}

bool UI_SetEventOnKeyDown(const char* ui_name, const char* control_name, void (*func)(int32_t btn))
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("WARNING: Tried to set UI key-down event callback for unknown control %s for UI %s!", control_name, ui_name);
		return false;
	}

	if (!func)
	{
		Com_Printf("WARNING: Tried to set UI key-down event callback for control %s on UI %s to null!", control_name, ui_name);
		return false;
	}

	ui_control_ptr->on_key_down = func;
	return true;
}

bool UI_SetEventOnKeyUp(const char* ui_name, const char* control_name, void (*func)(int32_t btn))
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("WARNING: Tried to set UI key-up event callback for unknown control %s for UI %s!", control_name, ui_name);
		return false;
	}

	if (!func)
	{
		Com_Printf("WARNING: Tried to set UI key-up event callback for control %s on UI %s to null!", control_name, ui_name);
		return false;
	}

	ui_control_ptr->on_key_up = func;
	return true;
}

bool UI_SetEventOnUpdate(const char* ui_name, void (*func)())
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (!ui_ptr)
	{
		Com_Printf("WARNING: Tried to set UI clicked event callback for unknown UI %s", ui_name);
		return false;
	}

	if (!func)
	{
		Com_Printf("WARNING: Tried to set UI clicked event callback for UI %s to null!", ui_name);
		return false;
	}

	ui_ptr->on_update = func;
	return true;
}

bool UI_SetEventOnUpdateControl(const char* ui_name, const char* control_name, void (*func)())
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("WARNING: Tried to set UI update event callback handler for unknown control %s for UI %s!", control_name, ui_name);
		return false;
	}

	if (!func)
	{
		Com_Printf("WARNING: Tried to set UI update event callback for control %s on UI %s to null!", control_name, ui_name);
		return false;
	}

	ui_control_ptr->on_update = func;
	return true;
}

// This mechanism is used to prevent a situation where under the following conditions:
// - A UI control has an event where the firing is dependent on the position of the control (e.g. a mouse event)
// - This event handler changes the active UI
// - A UI control in the UI being switched to is in the same position as the UI firing the event
//
// both events will fire
// This is an issue caused by the status of the UI changing during this function. Therefore this function has been redesigned
// to store the events being fired in a list and then fire them separately.

#define MAX_UI_FIRE_EVENT	16

void UI_FireEventOnClickUp(int32_t btn, int32_t x, int32_t y)
{
	ui_control_t* fire_event[MAX_UI_FIRE_EVENT] = { 0 };
	int32_t fire_event_num = 0;

	for (int32_t ui_num = 0; ui_num < num_uis; ui_num++)
	{
		ui_t* ui_ptr = &ui_list[ui_num];

		// find the UIs that we need to fire the event on
		for (int32_t ui_control_num = 0; ui_control_num < ui_ptr->num_controls; ui_control_num++)
		{
			ui_control_t* ui_control_ptr = &ui_ptr->controls[ui_control_num];

			// transform [0,1] coordinate system used by the ui engine to the actual xy screen coordinates
			float final_pos_x = ui_control_ptr->position_x * r_width->value;
			float final_pos_y = ui_control_ptr->position_y * r_height->value;

			if (!ui_ptr->passive && !ui_ptr->activated)
				continue;

			if (ui_control_ptr->focused)
				ui_control_ptr->focused = false;

			// Handle focus changes for key events (events we need to fire on are below)
			if (x >= final_pos_x
				&& y >= final_pos_y
				&& x <= final_pos_x + ((ui_control_ptr->size_x) * vid_hudscale->value)
				&& y <= final_pos_y + ((ui_control_ptr->size_y) * vid_hudscale->value)
				&& ui_ptr->activated
				&& fire_event_num <= MAX_UI_FIRE_EVENT)
			{
				fire_event[fire_event_num] = ui_control_ptr;
				fire_event_num++;
			}
		}
	}

	// now fire the events
	for (int32_t fire_event_id = 0; fire_event_id < fire_event_num; fire_event_id++)
	{
		ui_control_t* ui_control_ptr = fire_event[fire_event_id];

		ui_control_ptr->focused = true;

		// if the UI has an onclickdown event handler, call it
		if (ui_control_ptr->on_click_up)
		{
			// YES this is terribly inefficient because you have to check 8 billion uis. I dont care. Fuck UI
			ui_control_ptr->on_click_up(btn, x, y);
		}
	}
}

void UI_FireEventOnClickDown(int32_t btn, int32_t x, int32_t y)
{
	ui_control_t* fire_event[MAX_UI_FIRE_EVENT] = { 0 };
	int32_t fire_event_num = 0;

	for (int32_t ui_num = 0; ui_num < num_uis; ui_num++)
	{
		ui_t* ui_ptr = &ui_list[ui_num];

		// find the UIs that we need to fire the event on
		for (int32_t ui_control_num = 0; ui_control_num < ui_ptr->num_controls; ui_control_num++)
		{
			ui_control_t* ui_control_ptr = &ui_ptr->controls[ui_control_num];

			float final_pos_x = ui_control_ptr->position_x * r_width->value;
			float final_pos_y = ui_control_ptr->position_y * r_height->value;

			// Handle focus changes for key events (ui elements we actually focused are below)
			if (!ui_ptr->passive && !ui_ptr->activated)
				continue;

			if (ui_control_ptr->focused)
				ui_control_ptr->focused = false;

			if (x >= final_pos_x
				&& y >= final_pos_y
				&& x <= final_pos_x + ((ui_control_ptr->size_x) * vid_hudscale->value)
				&& y <= final_pos_y + ((ui_control_ptr->size_y) * vid_hudscale->value)
				&& ui_ptr->activated
				&& fire_event_num <= MAX_UI_FIRE_EVENT)
			{
				fire_event[fire_event_num] = ui_control_ptr;
				fire_event_num++;
			}
		}
	}

	// now fire the events
	for (int32_t fire_event_id = 0; fire_event_id < fire_event_num; fire_event_id++)
	{
		ui_control_t* ui_control_ptr = fire_event[fire_event_id];

		ui_control_ptr->focused = true;

		// if the UI has an onclickdown event handler, call it
		if (ui_control_ptr->on_click_down)
		{
			// YES this is terribly inefficient because you have to check 8 billion uis. I dont care. Fuck UI
			ui_control_ptr->on_click_down(btn, x, y);
		}
	}
}

void UI_FireEventOnKeyDown(int32_t btn)
{
	for (int32_t ui_num = 0; ui_num < num_uis; ui_num++)
	{
		ui_t* ui_ptr = &ui_list[ui_num];

		for (int32_t ui_control_num = 0; ui_control_num < ui_ptr->num_controls; ui_control_num++)
		{
			ui_control_t* ui_control_ptr = &ui_ptr->controls[ui_control_num];

			// checking for focusing is the choice of each individual UI
			if (ui_control_ptr->on_key_down)
				ui_control_ptr->on_key_down(btn);
		}		
	}
}

void UI_FireEventOnKeyUp(int32_t btn)
{
	for (int32_t ui_num = 0; ui_num < num_uis; ui_num++)
	{
		ui_t* ui_ptr = &ui_list[ui_num];

		for (int32_t ui_control_num = 0; ui_control_num < ui_ptr->num_controls; ui_control_num++)
		{
			ui_control_t* ui_control_ptr = &ui_ptr->controls[ui_control_num];

			// checking for focusing is the choice of each individual UI
			if (ui_control_ptr->on_key_up)
				ui_control_ptr->on_key_up(btn);
		}
	}
}

void UI_FireEventOnUpdate(ui_t* ui_ptr)
{
	if (ui_ptr->on_update)
		ui_ptr->on_update();
}

void UI_FireEventOnUpdateControl(ui_control_t* ui_control_ptr)
{
	if (ui_control_ptr->on_update)
		ui_control_ptr->on_update();
}