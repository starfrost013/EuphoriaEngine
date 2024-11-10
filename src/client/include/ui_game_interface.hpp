/*
Euphoria Game Engine
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

//
// ui_game_interface.hpp : Holds the GameUI export interface
//

#pragma once
#include <cstdint>
#include <shared/shared.hpp>

// Required for GameUI DLL to link
// Don't include ui.hpp 
typedef struct ui_s ui_t;
typedef struct ui_control_s ui_control_t;

// Engine Interface
typedef struct engine_ui_api_s
{
	uint32_t version;

	// UI: Init
	bool (*UI_Add)(const char* name, bool (*on_create)());

	// UI: Init Controls
	bool (*UI_AddText)(const char* ui_name, const char* name, const char* text,
		float position_x, float position_y);															// Draws text.
	bool (*UI_AddImage)(const char* ui_name, const char* name, const char* image_path, float position_x, float position_y,
		int32_t size_x, int32_t size_y);																// Draws an image.
	bool (*UI_AddSlider)(const char* ui_name, const char* name, float position_x, float position_y,
		int32_t size_x, int32_t size_y, int32_t value_min, int32_t value_max);							// Draws a slider.
	bool (*UI_AddCheckbox)(const char* ui_name, const char* name, float position_x, float position_y,
		int32_t size_x, int32_t size_y, bool checked);													// Draws a checkbox.
	bool (*UI_AddBox)(const char* ui_name, const char* name, float position_x, float position_y,
		int32_t size_x, int32_t size_y, color4_t color);												// Draws a regular ole box.

	// UI: Update Properties 
	bool (*UI_SetPosition)(const char* ui_name, const char* control_name, float x, float y);			// Updates a UI control's position.
	bool (*UI_SetSize)(const char* ui_name, const char* control_name, int32_t x, int32_t y);			// Updates a UI control's size.
	bool (*UI_SetText)(const char* ui_name, const char* control_name, const char* text);				// Updates a UI control's text.
	bool (*UI_SetFont)(const char* ui_name, const char* control_name, const char* font);				// Updates a UI control's font.
	bool (*UI_SetImage)(const char* ui_name, const char* control_name, const char* image_path);			// Updates a UI control's image.
	bool (*UI_SetInvisible)(const char* ui_name, const char* control_name, bool invisible);				// Updates a UI control's invisibility.
	bool (*UI_SetImageOnHover)(const char* ui_name, const char* control_name, const char* image_path);	// Updates a UI control's on-hover image.
	bool (*UI_SetImageOnClick)(const char* ui_name, const char* control_name, const char* image_path);	// Updates a UI control's on-click image.
	bool (*UI_SetColorOnHover)(const char* ui_name, const char* control_name, color4_t color);			// Updates a UI control's on-hover colour.
	bool (*UI_SetColorOnClick)(const char* ui_name, const char* control_name, color4_t color);			// Updates a UI control's on-click colour.
	bool (*UI_SetImageIsStretched)(const char* ui_name, const char* control_name, bool is_stretched);	// Updates a UI control's stretched status.

	bool (*UI_SetPassive)(const char* ui_name, bool passive);											// Sets a UI to passive (does not capture the mouse).
	bool (*UI_SetStackable)(const char* ui_name, bool stackable);										// Allows a UI to be pushed to the UI stack.
	bool (*UI_UseScaledAssets)(const char* ui_name, const char* control_name, bool use_scaled_assets);	// Allows an image to automatically scale.

	// UI: Enable/Disable
	bool (*UI_SetEnabled)(const char* name, bool enabled);												// Sets a UI's enabled (visible) state.
	bool (*UI_SetActivated)(const char* name, bool activated);											// Sets a UI's active (tangible) state.

	// UI: Set Event Handler

	// Events for entire UIs
	bool (*UI_SetEventOnUpdate)(const char* ui_name, void (*func)());									// Sets a UI's on_update handler

	// Events for UI controls
	bool (*UI_SetEventOnClickDown)(const char* ui_name, const char* control_name,
		void (*func)(int32_t btn, int32_t x, int32_t y));												// Sets a UI's on_click_down handler.
	bool (*UI_SetEventOnKeyDown)(const char* ui_name, const char* control_name,
		void (*func)(int32_t btn));																		// Sets a UI's on_key_down handler.
	bool (*UI_SetEventOnClickUp)(const char* ui_name, const char* control_name,
		void (*func)(int32_t btn, int32_t x, int32_t y));												// Sets a UI's on_click_up handler.
	bool (*UI_SetEventOnKeyUp)(const char* ui_name, const char* control_name,
		void (*func)(int32_t btn));																		// Sets a UI's on_key_up handler.
	bool (*UI_SetEventOnUpdateControl)(const char* ui_name, const char* control_name, void (*func)());	// Sets a UI control's on_update handler.

	// UI: Clear
	void (*UI_Clear)(const char* name);																	// Removes all the controls in a UI.

	// UI: Stack
	// UI_PUSH IS NOT RECOMMENDED TO CALL - PUSHES A UI TO THE STACK! Used for internal purposes.
	void (*UI_Push)();																					// Pushes a UI to the UI stack.
	void (*UI_Pop)();																					// Pops a UI from the UI stack.

	// UI: Internal
	ui_t* (*UI_GetUI)(const char* name);																// Returns a pointer so NULL can be indicated for failure
	ui_control_t* (*UI_GetControl)(const char* ui_name, const char* name);								// Gets the control with name name in the ui UI.
	void (*UI_Reset)();																					// INTERNAL, resets all UI states to their defaults. Provided for your use.																													// INTERNAL, deletes all UIs, and restarts the UI system

} engine_ui_api_t;

engine_ui_api_t* UI_InitInterface();