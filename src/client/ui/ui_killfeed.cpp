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
// ui_killfeed.c - Killfeed UI (August 6, 2024)
// In the future, all of these will be handled using scripts...

#include <client/client.hpp>

killfeed_entry_t killfeed_entries[MAX_KILLFEED_ENTRIES] = { 0 }; // initialise to zero
int32_t killfeed_entry_count = 0;

cvar_t* ui_killfeed_entry_time;

bool UI_KillFeedUICreate()
{
	// this doesn't do anything other than set the update event
	UI_SetPassive("KillFeedUI", true);
	UI_SetEventOnUpdate("KillFeedUI", Killfeed_Update);

	return true;
}

#define TEMP_NAME_BUF_SIZE	48										// Size of the name buffer for the temporary UI elements
#define KILLFEEDUI_NAME_BOX "KillFeedUI_KillFeedEntry%dBox"			// Format string for UI box elements
#define KILLFEEDUI_NAME_TEXT "KillFeedUI_KillFeedEntry%dText"		// Format string for UI text elements
#define KILLFEEDUI_NAME_ICON "KillFeedUI_KillFeedEntry%dIcon"		// Format string for UI icon elements

void Killfeed_Add()
{
	char* msg_string;
	// for the ui names, we need to come up with a unique string identifier for the UI 
	// this is required to remove the killfeed entries when we're done with them
	char name_buf[TEMP_NAME_BUF_SIZE] = { 0 };

	int32_t current_entry_num = killfeed_entry_count;

	// Overwrite the oldest entry if we have reached the maximum number of entries.
	// The draw code separate
	if (killfeed_entry_count >= MAX_KILLFEED_ENTRIES)
		current_entry_num = 0; // use a tempvar so the others still get dranw

	killfeed_entry_t* killfeed_entry = &(killfeed_entries[current_entry_num]);

	// get the kill type	

	killfeed_entry->inuse = true;
	killfeed_entry->time = Sys_Milliseconds();

	msg_string = MSG_ReadString(&net_message);
	strncpy(killfeed_entry->death_string, msg_string, MAX_DEATHSTRING_LENGTH);

	msg_string = MSG_ReadString(&net_message);

	// "none" sent for no icons
	strncpy(killfeed_entry->icon_path, msg_string, MAX_QPATH);

	//when we make UI_AddBox be consistent with the *SetColor UI functions a color4_t
	//color4_t killfeed_colour = { 32, 32, 32, 180 };

	// the positions get moved around later so just put them offscreen. the UI elements will literally last less than a frame in their initial positions
	// (see UI_KillFeedUpdate below)
	snprintf(name_buf, TEMP_NAME_BUF_SIZE, KILLFEEDUI_NAME_BOX, current_entry_num);

	color4_t color = { 32, 32, 32, 180 };

	UI_AddBox("KillFeedUI", name_buf, -0.1f, -0.1f, 189, 16, color);
	memset(name_buf, 0x00, sizeof(name_buf));

	snprintf(name_buf, TEMP_NAME_BUF_SIZE, KILLFEEDUI_NAME_TEXT, killfeed_entry_count);

	UI_AddText("KillFeedUI", name_buf, killfeed_entry->death_string, -0.1f, -0.1f);
	memset(name_buf, 0x00, sizeof(name_buf));
	// Icon not implemented yet

	killfeed_entry_count++;
}

void Killfeed_Update()
{
	if (killfeed_entry_count <= 0)
		return;

	// for the ui names, we need to come up with a unique string identifier for the UI 
	// this is required to remove the killfeed entries when we're done with them

	char name_buf[TEMP_NAME_BUF_SIZE] = { 0 };

	// start x,y coord
	float x = 0.8025f;
	float y = 0.05f;
	int32_t size_x = 0, size_y = 0;

	// enable the killfeed ui if it's not already enabled
	UI_SetEnabled("KillFeedUI", true);
	UI_SetActivated("KillFeedUI", true);

	// newest first
	for (int32_t entry_num = killfeed_entry_count; entry_num >= 0; entry_num--)
	{
		killfeed_entry_t* killfeed_entry_ptr = &killfeed_entries[entry_num];

		// if not in use, ignore
		if (!killfeed_entry_ptr->inuse)
			continue;

		// move the box
		snprintf(name_buf, TEMP_NAME_BUF_SIZE, KILLFEEDUI_NAME_BOX, entry_num);
		UI_SetPosition("KillFeedUI", name_buf, x, y);

		// do some massaging so it looks good
		snprintf(name_buf, TEMP_NAME_BUF_SIZE, KILLFEEDUI_NAME_TEXT, entry_num);

		// position it in the middle
		// we use system font for this
		Text_GetSize(cl_system_font->string, &size_x, &size_y, name_buf);

		UI_SetPosition("KillFeedUI", name_buf, x + ((160.0f/UI_SCALE_BASE_X)/2) - (((float)size_x/2)/UI_SCALE_BASE_X), y + 0.0075f);

		y += 0.06f;
	}
	
	// get rid of any we don't need
	// separate loop to prevent issues where the count changes

	for (int32_t entry_num = 0; entry_num < killfeed_entry_count; entry_num++)
	{
		killfeed_entry_t* killfeed_entry_ptr = &killfeed_entries[entry_num];

		// if not in use, ignore
		if (!killfeed_entry_ptr->inuse)
			continue;

		if (Sys_Milliseconds() > killfeed_entry_ptr->time + ui_killfeed_entry_time->value)
		{
			killfeed_entry_ptr->inuse = false;
			memset(killfeed_entry_ptr, 0x00, sizeof(killfeed_entry_t)); // to be safe

			// remove the ui elements for this ui
			// or just set them invis?

			// text is longer than box
			// IF YOU CHANGE THIS, REMEMBER TO ADD A MEMSET

			snprintf(name_buf, TEMP_NAME_BUF_SIZE, KILLFEEDUI_NAME_BOX, entry_num);
			UI_SetInvisible("KillFeedUI", name_buf, true);

			// do some massaging so it looks good
			snprintf(name_buf, TEMP_NAME_BUF_SIZE, KILLFEEDUI_NAME_TEXT, entry_num);
			UI_SetInvisible("KillFeedUI", name_buf, true);
		}


	}	
}