#pragma once

#include "quakedef.h"

ui_t*	ui[MAX_UI_COUNT];	// Global Ui defines
ui_t*	current_ui;			// Pointer to current_UI 
int		ui_count = 0;		// UI count

ui_t* UI_GetUI(char* name);

void UI_Init(void)
{
	// Allocate all the memory at once (speed :D)
	*ui = Hunk_AllocName(sizeof(ui_t) * MAX_UI_COUNT, "UI");

	if (!ui)
	{
		Sys_Error("Failed to allocate hunk space for UI!");
	}

}

ui_t* UI_GetUI(char* name)
{
	for (int ui_number = 0; ui_number < MAX_UI_COUNT; ui_number++)
	{
		if (!stricmp(ui[ui_number]->name, name)) return ui[ui_number];
	}

	return NULL;
}

void UI_Start(char* name)
{
	ui_t* new_ui = UI_GetUI(name);

	if (new_ui) // we already created this ui so don't bother
	{
		current_ui = new_ui;
		return; 
	}
	else
	{
		memset(&new_ui, 0x00, sizeof(ui_t));
		*new_ui->name = name;

		if (ui_count >= MAX_UI_ELEMENTS)
		{
			Con_Warning("Attempted to add a new UI when there are >= MAX_UI_COUNT UIs!");
			return;
		}

		// put it in its proper place
		memcpy(ui[ui_count], new_ui, sizeof(ui_t));
		ui_count++;
	}
}

void UI_AddButton(const char* on_click, const char* texture, float size, float position)
{
	ui_element_t new_button;

	memset(&new_button, 0x00, sizeof(ui_element_t));

	// note: "none" can be used to not call a function on click
	// it should never be null, so that's a Sys_Error

	if (on_click == NULL) Sys_Error("UI_AddButton: on_click was NULL!");
	if (texture == NULL) Sys_Error("UI_AddButton: texture was NULL!");
	
	// suppress warning
	if (on_click != NULL) strcpy(new_button.on_click, on_click);
	if (texture != NULL) strcpy(new_button.texture, texture);

	new_button.size = size;
	new_button.position = position;

	if (current_ui->element_count >= MAX_UI_ELEMENTS)
	{
		Con_Warning("Attempted to add UI element to UI with >= MAX_UI_ELEMENTS elements!");
		return;
	}

	current_ui->elements[current_ui->element_count] = new_button;
	current_ui->element_count++;
}

void UI_End(char* name)
{
	if (!UI_GetUI(name))
	{
		Host_Error("Tried to end a UI that does not exist!");
		return;
	}

	// current_ui is NULLPTR when no ui is set
	current_ui = NULL;
}