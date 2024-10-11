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
#include <client/client.hpp>

// cl_loadout.c: Clientside loadout system (April 12, 2024)

// CVars
cvar_t* cl_loadout_fade;					// Determines if the LoadoutUI will fade after being toggled.
cvar_t* cl_loadout_fade_time;				// Determines the time (in milliseconds) that Loadout UI will take to fade after being toggled.

// Globals
bool	loadout_is_displayed = false;
int32_t	loadout_last_displayed_time = 0;

// Functions only used within this file
void Loadout_UpdateUI();

// Initialises the loadout system
void Loadout_Init()
{
	cl_loadout_fade = Cvar_Get("cl_loadout_fade", "1", CVAR_ARCHIVE);
	cl_loadout_fade_time = Cvar_Get("cl_loadout_fade_time", "7000", CVAR_ARCHIVE);
}

void Loadout_Add()
{
	if (cl.loadout.num_items >= LOADOUT_MAX_ITEMS)
	{
		Com_Printf("ERROR: Tried to create loadout item %d, max %d...", LOADOUT_MAX_ITEMS);
		return;
	}

	// we can guarantee msg_readstring won't be called again while this function runs
	char* str = MSG_ReadString(&net_message);

	// see if we already have it
	for (int32_t item_num = 0; item_num < cl.loadout.num_items; item_num++)
	{
		loadout_entry_t current_loadout_entry = cl.loadout.items[item_num];

		// if we already have it...don't bother adding it 
		if (current_loadout_entry.item_name != NULL
			&& !strncmp(str, current_loadout_entry.item_name, LOADOUT_MAX_STRLEN))
		{
			//autoswitching is governed by server
			// we have to read this otherwise the protocol gets out of sync with the message and the game dies
			MSG_ReadString(&net_message);
			MSG_ReadByte(&net_message);
			MSG_ReadShort(&net_message); 
			return;
		}
	}

	strncpy(cl.loadout.items[cl.loadout.num_items].item_name, str, LOADOUT_MAX_STRLEN);
	str = MSG_ReadString(&net_message);
	strncpy(cl.loadout.items[cl.loadout.num_items].icon, str, LOADOUT_MAX_STRLEN);

	cl.loadout.items[cl.loadout.num_items].type = (loadout_entry_type)MSG_ReadByte(&net_message);
	cl.loadout.items[cl.loadout.num_items].amount = MSG_ReadShort(&net_message);

	cl.loadout.num_items++;
	Loadout_UpdateUI();
}

void Loadout_Update()
{
	// do not interrupt teamui
	if (current_ui == NULL || !strncmp(current_ui->name, "TeamUI", 7))
		return;
	
	// if the client is NOT FUCKING RUNNING, don't do anything
	if (cls.state != ca_active)
		return;

	// if the user wants the UI faded out...
	if (cl_loadout_fade->value)
	{
		int32_t current_ms = Sys_Milliseconds();

		// and there has been enough time to fade it out
		if ((current_ms - loadout_last_displayed_time) > cl_loadout_fade_time->value)
		{
			UI_SetEnabled("LoadoutUI", false);
			UI_SetActivated("LoadoutUI", false);
		}
	}
}

// this function updates the loadout ui.
// it's only called when it needs to actually change for performance
void Loadout_UpdateUI()
{
	// make sure the loadout is displayed
	loadout_is_displayed = true;
	loadout_last_displayed_time = Sys_Milliseconds();

	// this is so items arent seen to "skip" if we lose one of the items in the middle
	int32_t item_num_visual = 0; 

	for (int32_t item_num = 0; item_num < cl.loadout.num_items; item_num++)
	{
		loadout_entry_t* loadout_entry_ptr = &cl.loadout.items[item_num];

		// turn on the parts of the UI...
		char loadout_ui_name_buffer[MAX_UI_STRLEN] = { 0 };

		if (item_num <= 9
			&& loadout_entry_ptr->item_name[0]
			&& loadout_entry_ptr->type == loadout_entry_type_weapon) // TEMP - what do we do for item 11+?
		{
			snprintf(loadout_ui_name_buffer, MAX_UI_STRLEN, "LoadoutUI_Option%d", item_num_visual);

			// let it be visible and set the offer
			UI_SetInvisible("LoadoutUI", loadout_ui_name_buffer, false);
			UI_SetImage("LoadoutUI", loadout_ui_name_buffer, loadout_entry_ptr->icon);

			item_num_visual++;
		}
	}

	// set the text's visibility based on if there is a current item
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Text", cl.loadout.client_current_item == NULL);

	// if there is a selected item, set the text to that item's description
	if (cl.loadout.client_current_item)
		UI_SetText("LoadoutUI", "LoadoutUI_Text", cl.loadout.client_current_item->item_name);

}

// Removes the item with the name item_name.
void Loadout_Remove(char* item_name)
{
	// see if we already have it
	for (int32_t item_num = 0; item_num < cl.loadout.num_items; item_num++)
	{
		loadout_entry_t* current_loadout_item_ptr = &cl.loadout.items[item_num];

		// if we already have it...don't bother adding it 
		if (current_loadout_item_ptr->item_name != NULL
			&& !strncmp(item_name, current_loadout_item_ptr->item_name, LOADOUT_MAX_STRLEN))
		{
			// if its the last item also decrement num_items (we just ignore null ones otherwise)
			if (item_num == (cl.loadout.num_items))
				cl.loadout.num_items--;

			// delete the item
			memset(&cl.loadout.items[item_num], 0x00, sizeof(loadout_entry_t));
			return;
		}
	}


	cl.loadout.num_items--;
	Loadout_UpdateUI();
}

void Loadout_Clear()
{
	// kill the UI items
	memset(&cl.loadout.items, 0x00, sizeof(cl.loadout.items));

	// set num_items and current item to 0
	cl.loadout.num_items = 0;
	cl.loadout.client_current_item = 0;
#
	// turn off the UI bits manually...
	// I FUCKING HATE UI PROGRAMMING! FUCK THIS!
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Text", true);
		
	char loadout_ui_name_buffer[MAX_UI_STRLEN] = { 0 };

	for (int32_t ui_ctrl_num = 0; ui_ctrl_num <= 9; ui_ctrl_num++)
	{
		snprintf(loadout_ui_name_buffer, MAX_UI_STRLEN, "LoadoutUI_Option%d", ui_ctrl_num);
		UI_SetInvisible("LoadoutUI", loadout_ui_name_buffer, true);
	}
}

void Loadout_SetCurrent(int32_t index)
{
	if (index < 0
		|| index > 9)
	{
		Com_Error(ERR_DROP, "Tried to set current client loadout item to invalid value %d!", index);
	}

	// valid value but we just don't have that many items
	if (index >= cl.loadout.num_items)
		return;

	cl.loadout.client_current_item = &cl.loadout.items[index];

	Loadout_UpdateUI();
}