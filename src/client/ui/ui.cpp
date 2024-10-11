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
// cl_ui.c -- ZombonoUI (December 9, 2023)
// In the future, all of these will be handled using scripts...

#include <client/client.hpp>

// Globals
// see client.h for explanations on what these are 
int32_t num_uis;																					
ui_t	ui_list[MAX_UIS];
ui_t*	ui_stack[MAX_UIS];				// The optional UI stack
int32_t	ui_stack_top = -1;				// The top of the UI stack, starts at -1

bool	ui_active = false;				// This is so we know to turn on the mouse cursor when a UI is being displayed and turn it off when it isn't before
bool	ui_initialised = false;

// Current UI. A non-null value is enforced.
// Editing functions apply to this UI, as well as functions that are run on UI elements.
// You can only access UI elements through the current UI.
ui_t*	current_ui;

// Functions defined only in this file

// Utility
bool UI_AddControl(ui_t* ui, char* control_name, float position_x, float position_y, int32_t size_x, int32_t size_y);// Shared function that adds controls.

// Draw functions
void UI_DrawText(ui_control_t* text);															// Draws a text control.
void UI_DrawImage(ui_control_t* image);															// Draws an image control.
void UI_DrawSlider(ui_control_t* slider);														// Draws a slider control.
void UI_DrawCheckbox(ui_control_t* checkbox);													// Draws a checkbox control.
void UI_DrawBox(ui_control_t* box);																// Draws a box control.
void UI_DrawSpinControl(ui_control_t* spin_control);											// Draws a spin control.
void UI_DrawEntry(ui_control_t* entry);															// Draws an entry control.

// Utility functions
void UI_SliderDoSlide(ui_control_t* slider, int32_t dir);										// 'Slides' a slider control.
void UI_SpinControlDoEnter(ui_control_t* spin_control);											// Handles pressing ENTER on a spincontrol.
void UI_SpinControlDoSlide(ui_control_t* spin_control, int32_t dir);							// 'Slides' a spin control.
bool UI_EntryOnKeyDown(ui_control_t* entry, int32_t key);										// Handles a key being pressed on an entry control.

// Stack functions
// Since a UI should never get destroyed at the same time that a state change is occurring, we don't have to check the stack.
// Hopefully.
void UI_Push();																					// Pushes the UI stack
void UI_Pop();																					// Pops the UI stack

void UI_InitCvars()
{
	// init UI cvars
	ui_killfeed_entry_time = Cvar_Get("ui_killfeed_entry_time", "7500", 0);
}

bool UI_Init()
{
	UI_InitCvars();

	// they are not statically initalised here because UI gets reinit'd on vidmode change so we need to wipe everything clean
	memset(&ui_list, 0x00, sizeof(ui_t) * num_uis); // only clear the uis that actually exist
	num_uis = 0;
	bool successful;

	Com_Printf("UI_Init: running UI creation scripts\n");
	successful = UI_AddUI("TeamUI", UI_TeamUICreate);
	if (successful) successful = UI_AddUI("TeamWavesUI", UI_TeamWavesUICreate);	
	if (successful) successful = UI_AddUI("LeaderboardUI", UI_LeaderboardUICreate);
	if (successful) successful = UI_AddUI("BamfuslicatorUI", UI_BamfuslicatorUICreate);
	if (successful) successful = UI_AddUI("TimeUI", UI_TimeUICreate);
	if (successful) successful = UI_AddUI("ScoreUI", UI_ScoreUICreate);
	if (successful) successful = UI_AddUI("LoadoutUI", UI_LoadoutUICreate);
	if (successful) successful = UI_AddUI("MainMenuUI", UI_MainMenuUICreate);
	if (successful) successful = UI_AddUI("MainMenuQuickstartUI", UI_MainMenuQuickstartUICreate);
	if (successful) successful = UI_AddUI("MainMenuBrowseServersUI", UI_MainMenuBrowseServersUICreate);
	if (successful) successful = UI_AddUI("MainMenuSettingsUI", UI_MainMenuSettingsUICreate);
	if (successful) successful = UI_AddUI("MainMenuZombieTVUI", UI_MainMenuZombieTVUICreate);
	if (successful) successful = UI_AddUI("MainMenuQuitUI", UI_MainMenuQuitUICreate);
	if (successful) successful = UI_AddUI("SettingsControlsUI", UI_SettingsControlsUICreate);
	if (successful) successful = UI_AddUI("SettingsGameUI", UI_SettingsGameUICreate);
	if (successful) successful = UI_AddUI("SettingsGraphicsUI", UI_SettingsGraphicsUICreate);
	if (successful) successful = UI_AddUI("SettingsSoundUI", UI_SettingsSoundUICreate);
	if (successful) successful = UI_AddUI("KillFeedUI", UI_KillFeedUICreate);
	ui_initialised = successful;
	return successful;
}

bool UI_AddUI(const char* control_name, bool (*on_create)())
{
	Com_DPrintf("Creating UI: %s\n", control_name);
	current_ui = &ui_list[num_uis];

	if (num_uis > MAX_UIS)
	{
		Sys_Error("Tried to create a UI when there are more than %d UIs!", MAX_UIS);
		return false; 
	}

	num_uis++;

	if (strlen(control_name) > MAX_UI_STRLEN)
	{
		Sys_Error("Tried to create a UI with control_name more than %d characters!", MAX_UI_STRLEN);
		return false;
	}

	strcpy(current_ui->name, control_name);

	if (on_create == NULL)
	{
		Sys_Error("Tried to create a UI with no creation function!");
		return false;
	}

	current_ui->on_create = on_create;

	if (!on_create())
	{
		Sys_Error("UI create function failed for UI %s", control_name); // should these be fatal?
		return false; 
	}

	return true;
}

// return a pointer to a UI
ui_t* UI_GetUI(const char* control_name)
{
	for (int32_t ui_num = 0; ui_num < num_uis; ui_num++)
	{
		ui_t* ui_ptr = &ui_list[ui_num];

		if (!stricmp(ui_ptr->name, control_name))
		{
			return ui_ptr;
		}
	}

	Com_Printf("UI_GetUI: Couldn't find UI: %s\n", control_name);
	return NULL;
}

// get a UI control on the UI ui
ui_control_t* UI_GetControl(const char* ui_name, const char* control_name)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	// message already printed in check above
	if (!ui_ptr)
		return NULL;

	for (int32_t ui_control_num = 0; ui_control_num < ui_ptr->num_controls; ui_control_num++)
	{
		ui_control_t* ui_control_ptr = &ui_ptr->controls[ui_control_num];

		if (!stricmp(ui_control_ptr->name, control_name))
			return ui_control_ptr;

	}

	return NULL;
}

bool UI_AddControl(ui_t* ui_ptr, const char* control_name, float position_x, float position_y, int32_t size_x, int32_t size_y)
{
	if (ui_ptr->num_controls >= CONTROLS_PER_UI)
	{
		Sys_Error("Tried to add too many controls to the UI %s, max %d", ui_ptr->name, ui_ptr->num_controls);
		return false;
	}

	ui_control_t* current_control = &ui_ptr->controls[ui_ptr->num_controls];
	ui_ptr->num_controls++;

	strcpy(current_control->name, control_name);

	current_control->position_x = position_x;
	current_control->position_y = position_y;
	current_control->size_x = size_x;
	current_control->size_y = size_y;
	 
	return true;
}

bool UI_AddText(const char* ui_name, const char* control_name, const char* text, float position_x, float position_y)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (!ui_ptr)
		return false;

	ui_control_t* ui_control_ptr = &ui_ptr->controls[ui_ptr->num_controls];

	// not recommended to buffer overflow
	if (strlen(text) > MAX_UI_STRLEN)
	{
		Com_Printf("Tried to set UI control text %s to %s - too long (max length %d)!\n", control_name, text, MAX_UI_STRLEN);
		return false;
	}

	strcpy(ui_control_ptr->text, text);

	if (!ui_control_ptr->font
		|| strlen(ui_control_ptr->font) == 0)
	{
		Text_GetSize(cl_system_font->string, &ui_control_ptr->size_x, &ui_control_ptr->size_y, text);
	}
	else
	{
		Text_GetSize(ui_control_ptr->font, &ui_control_ptr->size_x, &ui_control_ptr->size_y, text);
	}

	// set default colour to white
	ui_control_ptr->color[0] = 255;
	ui_control_ptr->color[1] = 255;
	ui_control_ptr->color[2] = 255;
	ui_control_ptr->color[3] = 255;

	// by default hover and click colour are the same as the normal colour so set them

	VectorCopy4(ui_control_ptr->color, ui_control_ptr->color_on_hover);
	VectorCopy4(ui_control_ptr->color, ui_control_ptr->color_on_click);

	ui_control_ptr->type = ui_control_text;
	return UI_AddControl(ui_ptr, control_name, position_x, position_y, 0, 0);
}

bool UI_AddImage(const char* ui_name, const char* control_name, const char* image_path, float position_x, float position_y, int32_t size_x, int32_t size_y)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (!ui_ptr)
	{
		// message already printed
		return false;
	}

	ui_control_t* ui_control = &ui_ptr->controls[ui_ptr->num_controls];

	// not recommended to buffer overflow
	if (strlen(image_path) > MAX_UI_STRLEN)
	{
		Com_Printf("The UI control %s's image path %s is too long (max length %d)!\n", control_name, image_path, MAX_UI_STRLEN);
		return false;
	}

	strcpy(ui_control->image_path, image_path);

	ui_control->type = ui_control_image;

	return UI_AddControl(ui_ptr, control_name, position_x, position_y, size_x, size_y);
}

bool UI_AddSlider(const char* ui_name, const char* control_name, float position_x, float position_y, int32_t size_x, int32_t size_y, int32_t value_min, int32_t value_max)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (!ui_ptr)
	{
		// message already printed
		return false;
	}

	ui_control_t* ui_control = &ui_ptr->controls[ui_ptr->num_controls];
	ui_control->type = ui_control_slider;

	ui_control->value_min = value_min;
	ui_control->value_max = value_max;

	return UI_AddControl(ui_ptr, control_name, position_x, position_y, size_x, size_y);
}

bool UI_AddCheckbox(const char* ui_name, const char* control_name, float position_x, float position_y, int32_t size_x, int32_t size_y, bool checked)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (!ui_ptr)
	{
		// message already printed
		return false;
	}

	ui_control_t* ui_control = &ui_ptr->controls[ui_ptr->num_controls];

	ui_control->checked = checked;
	ui_control->type = ui_control_checkbox;

	return UI_AddControl(ui_ptr, control_name, position_x, position_y, size_x, size_y);
}

bool UI_AddBox(const char* ui_name, const char* control_name, float position_x, float position_y, int32_t size_x, int32_t size_y, color4_t color)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (!ui_ptr)
		return false; // message already printed

	ui_control_t* ui_control_ptr = &ui_ptr->controls[ui_ptr->num_controls];

	VectorCopy4(color, ui_control_ptr->color);

	// by default hover and click colour are the same as the normal colour so set them

	VectorCopy4(ui_control_ptr->color, ui_control_ptr->color_on_hover);
	VectorCopy4(ui_control_ptr->color, ui_control_ptr->color_on_click);

	ui_control_ptr->type = ui_control_box;

	return UI_AddControl(ui_ptr, control_name, position_x, position_y, size_x, size_y);
}

bool UI_SetEnabled(const char* ui_name, bool enabled)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (ui_ptr != NULL)
	{
		// otherwise enable the ui
		ui_ptr->enabled = enabled;

		// disable the current UI if the UI is stacked and not the top of the stack
		if (current_ui
			&& current_ui->stackable
			&& ui_ptr->stackable
			&& ui_ptr->enabled)
		{
			UI_SetActivated(current_ui->name, false);
			UI_SetEnabled(current_ui->name, false);
		}

		current_ui = (ui_ptr->enabled) ? ui_ptr : NULL;

		// don't bother popping on disable because multiple UIs can be enabled at a time
		if (ui_ptr->stackable
			&& ui_ptr->enabled)
		{
			UI_Push();
		}
	}

	return false;
}

bool UI_SetActivated(const char* ui_name, bool activated)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	// if this UI is not passive, determine if any other UI is activated, and turn it off if it is
	if (!ui_ptr->passive)
	{
		for (int32_t ui_num = 0; ui_num < num_uis; ui_num++)
		{
			ui_t* ui_ptr = &ui_list[ui_num];

			if (ui_ptr->activated)
				ui_ptr->activated = false;
		}
	}

	if (ui_ptr != NULL)
	{
		ui_ptr->activated = activated;
		ui_active = activated;

		// if the UI requires the mouse....
		if (!ui_ptr->passive)
		{
			// turn on the mouse cursor, bit hacky - we basically take over the mouse pointer when a UI is active
			Input_Activate(!activated);
		}

		current_ui = ui_ptr;
		return true; 
	}

	return false;
}

// set UI ui_name passivity to passive
bool UI_SetPassive(const char* ui_name, bool passive)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (ui_ptr != NULL)
	{
		ui_ptr->passive = passive; 
		return true;
	}

	return false;
}

bool UI_SetStackable(const char* ui_name, bool stackable)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (ui_ptr != NULL)
	{
		ui_ptr->stackable = stackable;
		return true;
	}

	return false;
}

bool UI_UseScaledAssets(const char* ui_name, const char* control_name, bool use_scaled_assets)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (ui_control_ptr)
	{
		ui_control_ptr->use_scaled_assets = use_scaled_assets;
		return true;
	}

	return false;
}

// Updates a UI control's position.
bool UI_SetPosition(const char* ui_name, const char* control_name, float x, float y)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Couldn't find UI control %s to set position to %f, %f!\n", control_name, x, y);
		return false;
	}

	ui_control_ptr->position_x = x;
	ui_control_ptr->position_y = y;

	return true;
}

// Updates a UI control's size.
bool UI_SetSize(const char* ui_name, const char* control_name, int32_t x, int32_t y)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Couldn't find UI control %s to set position to %d, %d!\n", control_name, x, y);
		return false;
	}

	ui_control_ptr->size_x = x;
	ui_control_ptr->size_y = y;

	return true;
}

bool UI_SetText(const char* ui_name, const char* control_name, const char* text)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Couldn't find UI control %s to set text to %s!\n", control_name, text);
		return false;
	}

	if (strlen(text) > MAX_UI_STRLEN)
	{
		Com_Printf("UI text for control %s, %s, was too long (max %d)\n", control_name, text, MAX_UI_STRLEN);
		return false;
	}

	if (strlen(ui_control_ptr->font) == 0)
		Text_GetSize(cl_system_font->string, &ui_control_ptr->size_x, &ui_control_ptr->size_y, text);
	else
		Text_GetSize(ui_control_ptr->font, &ui_control_ptr->size_x, &ui_control_ptr->size_y, text);

	strcpy(ui_control_ptr->text, text);

	return true; 
}

bool UI_SetFont(const char* ui_name, const char* control_name, const char* font)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Couldn't find UI control %s to set text to %s!\n", control_name, font);
		return false;
	}

	if (strlen(font) > MAX_UI_STRLEN)
	{
		Com_Printf("UI text for control %s, %s, was too long (max %d)\n", control_name, font, MAX_UI_STRLEN);
		return false;
	}

	strcpy(ui_control_ptr->font, font);

	//re-evaluate the size of the UI control, now that the font changed
	//todo - doesn't take into account varargs
	if (!Text_GetSize(ui_control_ptr->font, &ui_control_ptr->size_x, &ui_control_ptr->size_y, ui_control_ptr->text))
		Com_Printf("Warning: Failed to get size of text for font %s\n", font);

	return true;
}


bool UI_SetImage(const char* ui_name, const char* control_name, const char* image_path)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Tried to set NULL UI control image path %s to %s!\n", control_name, image_path);
		return false;
	}

	if (strlen(image_path) > MAX_UI_STRLEN)
	{
		Com_Printf("UI image path for control %s, %s, was too long (max %d)\n", control_name, image_path, MAX_UI_STRLEN);
		return false;
	}

	strcpy(ui_control_ptr->image_path, image_path);

	return true;
}

bool UI_SetInvisible(const char* ui_name, const char* control_name, bool invisible)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Tried to set NULL UI control image on hover path %s to %b!\n", control_name, invisible);
		return false;
	}

	ui_control_ptr->invisible = invisible;

	return true;
}

bool UI_SetImageOnHover(const char* ui_name, const char* control_name, const char* image_path)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Tried to set NULL UI control image on hover path %s to %s!\n", control_name, image_path);
		return false;
	}

	if (strlen(image_path) > MAX_UI_STRLEN)
	{
		Com_Printf("UI image on hover path for control %s, %s, was too long (max %d)\n", control_name, image_path, MAX_UI_STRLEN);
		return false;
	}

	strcpy(ui_control_ptr->image_path_on_hover, image_path);

	return true;
}

bool UI_SetImageOnClick(const char* ui_name, const char* control_name, const char* image_path)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Tried to set NULL UI control image on click path %s to %s!\n", control_name, image_path);
		return false;
	}

	if (strlen(image_path) > MAX_UI_STRLEN)
	{
		Com_Printf("UI image on click path for control %s, %s, was too long (max %d)\n", control_name, image_path, MAX_UI_STRLEN);
		return false;
	}

	strcpy(ui_control_ptr->image_path_on_click, image_path);

	return true;
}

bool UI_SetColorOnHover(const char* ui_name, const char* control_name, color4_t color)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Tried to set NULL UI control hover colour %s to %f,%f,%f,%f!\n", control_name, color[0], color[1], color[2], color[3]);
		return false;
	}

	//color4_t is a vector
	VectorCopy4(color, ui_control_ptr->color_on_hover);

	return true;
}

bool UI_SetColorOnClick(const char* ui_name, const char* control_name, color4_t color)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Tried to set NULL UI control pressed colour %s to %f,%f,%f,%f!\n", control_name, color[0], color[1], color[2], color[3]);
		return false;
	}

	//color4_t is a vector
	VectorCopy4(color, ui_control_ptr->color_on_click);

	return true;
}

bool UI_SetImageIsStretched(const char* ui_name, const char* control_name, bool is_stretched)
{
	ui_control_t* ui_control_ptr = UI_GetControl(ui_name, control_name);

	if (!ui_control_ptr)
	{
		Com_Printf("Tried to set NULL UI control image is stretched %s to %d!\n", control_name, is_stretched);
		return false;
	}
	
	ui_control_ptr->image_is_stretched = is_stretched;
	return true;
}

void UI_Clear(const char* ui_name)
{
	ui_t* ui_ptr = UI_GetUI(ui_name);

	if (!ui_ptr) return; 		// message already printed

	// clear every control but not the ui's info
	// we clear every control, not just the ones that exist currently, in case more controls existed at some point (such as with Leaderboard UI)
	for (int32_t ui_control_num = 0; ui_control_num < ui_ptr->num_controls; ui_control_num++)
	{
		ui_control_t* ui_control = &ui_ptr->controls[ui_control_num];
		memset(ui_control, 0x00, sizeof(ui_control_t));
	}

	// tell everyone there are no controls
	ui_ptr->num_controls = 0;
}

// Resets all UIs
void UI_Reset()
{
	for (int32_t ui_num = 0; ui_num < num_uis; ui_num++)
	{
		ui_t* ui_ptr = &ui_list[ui_num];

		UI_SetEnabled(ui_ptr->name, false);
		UI_SetActivated(ui_ptr->name, false);
	}
}

void UI_Push()
{
	// don't bother pushing the same ui twice
	if (ui_stack[ui_stack_top] == current_ui)
	{
		Com_Printf("Warning: Tried to push the same UI twice...\n");
		return;
	}

	if (ui_stack_top >= (MAX_UIS - 1))
	{
		Sys_Error("User interface stack overflow!");
		return; // shut up compiler
	}

	ui_stack_top++;
	ui_stack[ui_stack_top] = current_ui;
}

void UI_Pop()
{
	if (ui_stack_top <= 0)
	{
		Sys_Error("User interface stack underflow!");
		return; // shut up compiler
	}

	// explicitly disable the old ui
	UI_SetEnabled(ui_stack[ui_stack_top]->name, false);
	UI_SetActivated(ui_stack[ui_stack_top]->name, false);

	ui_stack[ui_stack_top] = NULL;
	ui_stack_top--;

	UI_SetEnabled(ui_stack[ui_stack_top]->name, true);
	UI_SetActivated(ui_stack[ui_stack_top]->name, true);
}

void UI_Draw()
{
// draw debug/playtest indicator
	
// this is NOT!! efficient don't do this (esp. getting the length of the string every frame and stuff) but not used in release 
#if defined(PLAYTEST) || defined(DEBUG)
	time_t		raw_time;
	struct tm*	local_time;

	time(&raw_time);
	local_time = localtime(&raw_time);
	char		time_str[128] = { 0 };
#ifdef PLAYTEST
	strftime(time_str, 128, "Playtest Build v" ENGINE_VERSION " (%b %d %Y %H:%M:%S)", local_time);
#elif !defined(NDEBUG)
	strftime(time_str, 128, "Debug Build v" ENGINE_VERSION " (%b %d %Y %H:%M:%S)", local_time);
#endif

	int32_t size_x = 0, size_y = 0;
	Text_GetSize(cl_system_font->string, &size_x, &size_y, time_str);
	// TODO: Text_GetSize
	Text_Draw(cl_system_font->string, r_width->value - size_x, 0, time_str);
	const char* prerelease_text = "^3[STRING_PRERELEASE]";
	Text_GetSize(cl_system_font->string, &size_x, &size_y, prerelease_text);
	Text_Draw(cl_system_font->string, r_width->value - size_x, 10 * vid_hudscale->value, prerelease_text);

#endif

	for (int32_t ui_num = 0; ui_num < num_uis; ui_num++)
	{
		// draw the current UI if enabled (*ACTIVE* means it's receiving input events0
		ui_t* current_ui = &ui_list[ui_num];

		UI_FireEventOnUpdate(current_ui);

		if (current_ui->enabled)
		{
			for (int32_t ui_control_num = 0; ui_control_num < current_ui->num_controls; ui_control_num++)
			{
				ui_control_t* current_ui_control = &current_ui->controls[ui_control_num];

				if (!current_ui_control->invisible)
				{
					float final_pos_x = current_ui_control->position_x * r_width->value;
					float final_pos_y = current_ui_control->position_y * r_height->value;

					// toggle UI hover images if the mouse is within a UI
					current_ui_control->hovered =
						(last_mouse_pos_x >= final_pos_x
							&& last_mouse_pos_x <= (final_pos_x + (current_ui_control->size_x * vid_hudscale->value))
							&& last_mouse_pos_y >= final_pos_y
							&& last_mouse_pos_y <= (final_pos_y + (current_ui_control->size_y * vid_hudscale->value)));

					// fire the update event
					UI_FireEventOnUpdateControl(current_ui_control);

					switch (current_ui_control->type)
					{
					case ui_control_text:
						UI_DrawText(current_ui_control);
						break;
					case ui_control_image:
						UI_DrawImage(current_ui_control);
						break;
					case ui_control_checkbox:
						UI_DrawCheckbox(current_ui_control);
						break;
					case ui_control_slider:
						UI_DrawSlider(current_ui_control);
						break;
					case ui_control_box:
						UI_DrawBox(current_ui_control);
						break;
					case ui_control_spin:
						UI_DrawSpinControl(current_ui_control);
						break;
					case ui_control_entry:
						UI_DrawEntry(current_ui_control);
						break;
					}
				}
			}
		}
	}
}

void UI_DrawText(ui_control_t* text)
{
	int32_t final_pos_x = text->position_x * r_width->value;
	int32_t final_pos_y = text->position_y * r_height->value;

	color4_t color = { 255, 255, 255, 255 };

	if (text->hovered) // allow text to disappear when hovered
	{
		VectorCopy3(text->color_on_hover, color);
	}
	else if (text->focused)
	{
		VectorCopy3(text->color_on_click, color);
	}
	// if the colour snot entirely transparent, use it
	else
	{
		VectorCopy3(text->color, color);
	}

	// initialised to 0
	// if the font is not set use the system font
	if (strlen(text->font) == 0)
	{
		Text_DrawColor(cl_system_font->string, final_pos_x, final_pos_y, color, text->text);
	}
	else
	{
		Text_DrawColor(text->font, final_pos_x, final_pos_y, color, text->text);
	}
}

void UI_DrawImage(ui_control_t* image)
{
	int32_t final_pos_x = image->position_x * r_width->value;
	int32_t final_pos_y = image->position_y * r_height->value;

	int32_t final_size_x = image->size_x;
	int32_t final_size_y = image->size_y;

	// don't scale stretched images
	if (!image->image_is_stretched)
	{
		final_size_x *= vid_hudscale->value;
		final_size_y *= vid_hudscale->value;
	}

	char* image_path = image->image_path;

	if (image->focused
		&& image->image_path_on_click != NULL
		&& strlen(image->image_path_on_click) > 0)
	{
		image_path = image->image_path_on_click;
	}
	else if (image->hovered
		&& image->image_path_on_hover != NULL
		&& strlen(image->image_path_on_hover) > 0)
	{
		image_path = image->image_path_on_hover;
	}

	if (image->image_is_stretched)
	{
		re.DrawPicStretch(final_pos_x, final_pos_y, final_size_x, final_size_y, image_path, NULL, image->use_scaled_assets);
	}
	else
	{
		re.DrawPic(final_pos_x, final_pos_y, image_path, NULL, image->use_scaled_assets);
	}

}

#define SLIDER_RANGE 10
#define RCOLUMN_OFFSET  16 * vid_hudscale->value
#define LCOLUMN_OFFSET -16 * vid_hudscale->value

void UI_SliderDoSlide(ui_control_t* slider, int32_t dir)
{
	slider->value_current += dir;

	if (slider->value_current > slider->value_max)
		slider->value_current = slider->value_max;
	else if (slider->value_current < slider->value_min)
		slider->value_current = slider->value_min;

}

void UI_DrawSlider(ui_control_t* slider)
{
	int32_t i;
	int32_t size_x = 0, size_y = 0; // emulate original code R2L drawing

	Text_GetSize(cl_system_font->string, &size_x, &size_y, slider->text);
	Text_Draw(cl_system_font->string, slider->position_x * r_width->value  + LCOLUMN_OFFSET - size_x,
		slider->position_y * r_height->value,
		slider->text);

	float range = (slider->value_current - slider->value_min) / (float)(slider->value_max - slider->value_min);

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;

	re.DrawPic(slider->position_x * r_width->value  + RCOLUMN_OFFSET, slider->position_y * r_height->value, "2d/slider_01", NULL, false);
	
	for (i = 0; i < SLIDER_RANGE; i++)
		re.DrawPic(RCOLUMN_OFFSET + slider->position_x * r_width->value + i * 8 * vid_hudscale->value + 8 * vid_hudscale->value, slider->position_y * r_height->value, "2d/slider_02", NULL, false);
	
	re.DrawPic(RCOLUMN_OFFSET + slider->position_x * r_width->value + i * 8 * vid_hudscale->value + 8 * vid_hudscale->value, slider->position_y * r_height->value, "2d/slider_03", NULL, false);
	
	re.DrawPic((int32_t)(8 * vid_hudscale->value + RCOLUMN_OFFSET  + slider->position_x * r_width->value + (SLIDER_RANGE - 1) * 8 * vid_hudscale->value * range), slider->position_y * r_height->value, "2d/slider_value", NULL, false);
}

void UI_DrawCheckbox(ui_control_t* checkbox)
{
	Com_Printf("UI: Checkboxes aren't implemented yet!\n");
}

void UI_DrawBox(ui_control_t* box)
{
	int32_t final_pos_x = box->position_x * r_width->value;
	int32_t final_pos_y = box->position_y * r_height->value;

	int32_t final_size_x = box->size_x * vid_hudscale->value;
	int32_t final_size_y = box->size_y * vid_hudscale->value;

	if (box->focused)
	{
		re.DrawFill(final_pos_x, final_pos_y, final_size_x, final_size_y, box->color_on_click);
	}
	else if (box->hovered)
	{
		re.DrawFill(final_pos_x, final_pos_y, final_size_x, final_size_y, box->color_on_hover);
	}
	else
	{
		re.DrawFill(final_pos_x, final_pos_y, final_size_x, final_size_y, box->color);
	}

}

void UI_SpinControlDoEnter(ui_control_t* spin_control)
{
	spin_control->value_current++;
	if (spin_control->item_names[(int32_t)spin_control->value_current] == 0)
		spin_control->value_current = 0;
}

void UI_SpinControlDoSlide(ui_control_t* spin_control, int32_t dir)
{
	spin_control->value_current += dir;

	if (spin_control->value_current < 0)
		spin_control->value_current = 0;
	else if (spin_control->item_names[(int32_t)spin_control->value_current] == 0)
		spin_control->value_current--;
}

void UI_DrawSpinControl(ui_control_t* spin_control)
{
	int32_t size_x = 0, size_y = 0;

	if (spin_control->text)
	{
		Text_GetSize(cl_system_font->string, &size_x, &size_y, spin_control->text);
		Text_Draw(cl_system_font->string, spin_control->position_x * r_width->value + LCOLUMN_OFFSET - size_x,
			spin_control->position_y,
			spin_control->text);
	}

	Text_Draw(cl_system_font->string, RCOLUMN_OFFSET + spin_control->position_x * r_width->value, spin_control->position_y, spin_control->item_names[(int32_t)spin_control->value_current]);
}

bool UI_EntryOnKeyDown(ui_control_t* entry, int32_t key)
{
	//TODO: THIS CODE SUCKS
	//WHY DO WE DUPLICATE EVERYTHING ALREADY HANDLED IN INPUT SYSTEM????

	extern bool keydown[];

	// support pasting from the clipboard
	if ((keydown[K_V] && keydown[K_CTRL]) ||
		(((key == K_INSERT)) && keydown[K_SHIFT]))
	{
		char* cbd;

		if ((cbd = Sys_GetClipboardData()) != 0)
		{
			strncpy(entry->entry_text_buffer, cbd, strlen(entry->entry_text_buffer) - 1);

			entry->cursor_position = (int32_t)strlen(entry->entry_text_buffer);
			entry->cursor_last_visible = entry->cursor_position - entry->cursor_last_visible;
			if (entry->cursor_last_visible < 0)
				entry->cursor_last_visible = 0;

			free(cbd);
		}
		return true;
	}

	switch (key)
	{
	case K_LEFTARROW:
	case K_BACKSPACE:
		if (entry->cursor_position > 0)
		{
			memmove(&entry->entry_text_buffer[entry->cursor_position - 1], &entry->entry_text_buffer[entry->cursor_position], strlen(&entry->entry_text_buffer[entry->cursor_position]) + 1);
			entry->cursor_position--;

			if (entry->cursor_last_visible)
			{
				entry->cursor_last_visible--;
			}
		}
		break;

	case K_DELETE:
		memmove(&entry->entry_text_buffer[entry->cursor_position], &entry->entry_text_buffer[entry->cursor_position + 1], strlen(&entry->entry_text_buffer[entry->cursor_position + 1]) + 1);
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_ESCAPE:
	case K_TAB:
		return false;

	case K_SPACE:
	default:
		char processed_key = Key_VirtualToPhysical(key, (keydown[K_SHIFT])
			|| (keydown[K_CAPS_LOCK])
			&& key >= K_A
			&& key <= K_Z)[0];

		//if (!isdigit(key) && (f->generic.flags & QMF_NUMBERSONLY))
			//return false;

		if (entry->cursor_position < strlen(entry->entry_text_buffer))
		{
			entry->entry_text_buffer[entry->cursor_position++] = processed_key;
			entry->entry_text_buffer[entry->cursor_position] = 0;

			if (entry->cursor_position > entry->cursor_last_visible)
			{
				entry->cursor_last_visible++;
			}
		}
	}

	return true;
}

//TODO: Draw cursor
void UI_DrawEntry(ui_control_t* entry)
{
	int32_t i;
	int32_t name_size_x = 0, name_size_y = 0;
	int32_t text_size_x = 0, text_size_y = 0;
	char tempbuffer[128] = "";

	if (entry->text)
	{
		Text_GetSize(cl_system_font->string, &name_size_x, &name_size_y, entry->text);
		Text_Draw(cl_system_font->string, entry->position_x * r_width->value + LCOLUMN_OFFSET - name_size_x, entry->position_y * r_height->value, entry->text);
	}

	strncpy(tempbuffer, entry->entry_text_buffer + entry->cursor_last_visible, entry->cursor_last_visible);

	re.DrawPic(entry->position_x * r_width->value + 16 * vid_hudscale->value, entry->position_y * r_height->value - 4 * vid_hudscale->value, "2d/field_top_01", NULL, false);
	re.DrawPic(entry->position_x * r_width->value + 16 * vid_hudscale->value, entry->position_y * r_height->value + 4 * vid_hudscale->value, "2d/field_bottom_01", NULL, false);

	re.DrawPic(entry->position_x * r_width->value + 24 * vid_hudscale->value + entry->cursor_last_visible * 8 * vid_hudscale->value, entry->position_y * r_height->value - 4 * vid_hudscale->value, "2d/field_top_03", NULL, false);
	re.DrawPic(entry->position_x * r_width->value + 24 * vid_hudscale->value + entry->cursor_last_visible * 8 * vid_hudscale->value, entry->position_y * r_height->value + 4 * vid_hudscale->value, "2d/field_bottom_03", NULL, false);

	for (i = 0; i < entry->cursor_last_visible; i++)
	{
		re.DrawPic(entry->position_x * r_width->value + 24 * vid_hudscale->value + i * 8 * vid_hudscale->value, entry->position_y * r_height->value - 4 * vid_hudscale->value, "2d/field_top_02", NULL, false);
		re.DrawPic(entry->position_x * r_width->value + 24 * vid_hudscale->value + i * 8 * vid_hudscale->value, entry->position_y * r_height->value + 4 * vid_hudscale->value, "2d/field_bottom_02", NULL, false);
	}

	Text_GetSize(cl_system_font->string, &text_size_x, &text_size_y, tempbuffer);
	Text_Draw(cl_system_font->string, entry->position_x * r_width->value + 24 * vid_hudscale->value, (entry->position_y * r_height->value) - 1, tempbuffer); // -1 for padding

	int32_t offset;

	if (entry->cursor_last_visible)
		offset = entry->cursor_last_visible;
	else
		offset = entry->cursor_position;

	if (((int32_t)(Sys_Milliseconds() / 250)) & 1)
	{
		// 8x8 is cursor size
		re.DrawPic(entry->position_x * r_width->value + (offset + 2) + (text_size_x + 8) * vid_hudscale->value,
			entry->position_y * r_height->value,
			"2d/field_cursor_on", NULL, false);
	}
}
