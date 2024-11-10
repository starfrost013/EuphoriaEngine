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
// ui.hpp: UI Header
//

#pragma once
#include <cstdint>
#include <shared/shared.hpp>

//
// ui/*.c
// This is like the Zombono-Q1 UI system, except not broken and not having the server depend on stuff only the client can possibly know.
// This time the server can ONLY tell the client to draw a predefined (on the client side) UI.
//

#define CONTROLS_PER_UI			256				// The maximum number of controls per UI.
#define MAX_UIS					32				// The maximum number of UIs
#define MAX_UI_NAMELEN			96				// The maximum length of a UI name
#define MAX_UI_STRLEN			256				// The maximum string length for various UI elements.

#define MAX_UIS_STACKED			256				// The maximum number of stacked UIs. Same as the maximum number of UIs because they just get stacked.

#define UI_SCALE_BASE_X			960.0f			// Horizontal resolution used as the base for UI scaling.
#define UI_SCALE_BASE_Y			480.0f			// Vertical resolution used as the base for UI scaling.

#define MAX_ENTRY_TEXT_LENGTH	128				// Maximum length of text in an entry control

#define MAX_TABCONTROL_TABS		16				// Maximum number of tabs in a tab control

typedef enum ui_control_type_e
{
	ui_control_text = 0,									// Simple text.
	ui_control_image = 1,									// An image.
	ui_control_slider = 2,									// A slider between different values.
	ui_control_checkbox = 3,								// A checkable box.
	ui_control_box = 4,										// A simple box.
	ui_control_spin = 5,									// A "spinnable" set of options.
	ui_control_entry = 6,									// A textbox that can have text entered into it.
} ui_control_type;

//forward declaration
typedef struct ui_s ui_t;

typedef struct ui_control_s
{
	// general
	ui_control_type	type;								// Type of this UI control.
	float 			position_x;							// UI control position (x-component).
	float			position_y;							// UI control position (y-component).
	char			font[MAX_QPATH];
	int32_t 		size_x;								// UI control size (x-component).
	int32_t  		size_y;								// UI control size (y-component).
	char			name[MAX_UI_NAMELEN];				// UI control name (for code)
	bool			invisible;							// Is this UI control invisible?
	bool			focused;							// Is this UI control focused?
	bool			hovered;							// Is the mouse hovering over this UI control?
	// text
	char			text[MAX_UI_STRLEN];				// Text UI control: Text to display.
	// image
	char			image_path[MAX_QPATH];				// Image path to display for Image controls
	char			image_path_on_hover[MAX_QPATH];		// Image path when the UI control has been hovered over.
	char			image_path_on_click[MAX_QPATH];		// Image path when the UI control clicked on.
	bool			image_is_stretched;					// Is this UI control's image stretched?
	bool			use_scaled_assets;					// Does this image autoscale?
	// slider (also used by spin control)
	float	 		value_min;							// Slider UI control: minimum value
	float	 		value_max;							// Slider UI control: maximum value
	float			value_current;						// Slider UI control: current value
	// checkbox
	bool			checked;							// Checkbox UI control: Is it checked?
	// box
	color4_t		color;								// The color of this UI element.
	color4_t		color_on_hover;						// The hover colour of this UI element.
	color4_t		color_on_click;						// The pressed colour of this UI element.
	// spin control
	char** item_names;							// Spin control: Item names (pointer to string arrays)
	// entry
	int32_t			cursor_position;					// Entry: The position of the cursor
	int32_t			cursor_last_visible;				// Entry: Last visible cursor position
	char			entry_text_buffer[MAX_ENTRY_TEXT_LENGTH];// Entry: Maximum text length

	// events
	void			(*on_click_down)(int32_t btn, int32_t x, int32_t y);	// C function to call on click starting with X and Y coordinates.
	void			(*on_click_up)(int32_t btn, int32_t x, int32_t y);		// C function to call on click starting with X and Y coordinates.
	void			(*on_key_down)(int32_t btn);							// C function to call on a key starting to bressed.
	void			(*on_key_up)(int32_t btn);								// C function to call on a key stopping being pressed. 
	void			(*on_update)();											// C function to call every client frame.
} ui_control_t;

typedef struct ui_s
{
	int32_t 		num_controls;				// Number of controls in the UI.
	char			name[MAX_UI_STRLEN];		// The name of this UI	
	bool			(*on_create)();				// A function to call when creating this UI.
	void			(*on_update)();				// C function to call every client frame.
	bool			enabled;					// True if the UI is currently being drawn.
	bool			activated;					// True if the UI is currently interactable.
	bool			passive;					// True if the UI is "passive" (does not capture mouse) - it will still receive events!
	bool			stackable;					// True if the UI is stackable
	ui_control_t	controls[CONTROLS_PER_UI];	// Control list.
} ui_t;

extern ui_t		ui_list[MAX_UIS];		// The list of UIs.
extern ui_t*	ui_stack_list[MAX_UIS];	// The list of stacked UIs
extern ui_t*	current_ui;				// The current UI being displayed
extern int32_t 	num_uis;				// The current number of UIs
extern bool		ui_active;				// Is a UI active - set in UI_SetActive so we don't have to search through every single UI type
extern bool		ui_initialised;			// Determines if the UI engine has been initialised or not.

// UI: Init
bool UI_Init();
bool UI_Add(const char* name, bool (*on_create)());

// UI: Init Controls
bool UI_AddText(const char* ui_name, const char* name, const char* text, float position_x, float position_y);			// Draws text.
bool UI_AddImage(const char* ui_name, const char* name, const char* image_path, float position_x, float position_y,
	int32_t size_x, int32_t size_y);																// Draws an image.
bool UI_AddSlider(const char* ui_name, const char* name, float position_x, float position_y,
	int32_t size_x, int32_t size_y, int32_t value_min, int32_t value_max);							// Draws a slider.
bool UI_AddCheckbox(const char* ui_name, const char* name, float position_x, float position_y,
	int32_t size_x, int32_t size_y, bool checked);													// Draws a checkbox.
bool UI_AddBox(const char* ui_name, const char* name, float position_x, float position_y,
	int32_t size_x, int32_t size_y, color4_t color);												// Draws a regular ole box.

// UI: Update Properties 
bool UI_SetPosition(const char* ui_name, const char* control_name, float x, float y);									// Updates a UI control's position.
bool UI_SetSize(const char* ui_name, const char* control_name, int32_t x, int32_t y);									// Updates a UI control's size.
bool UI_SetText(const char* ui_name, const char* control_name, const char* text);										// Updates a UI control's text.
bool UI_SetFont(const char* ui_name, const char* control_name, const char* font);										// Updates a UI control's font.
bool UI_SetImage(const char* ui_name, const char* control_name, const char* image_path);								// Updates a UI control's image.
bool UI_SetInvisible(const char* ui_name, const char* control_name, bool invisible);									// Updates a UI control's invisibility.
bool UI_SetImageOnHover(const char* ui_name, const char* control_name, const char* image_path);							// Updates a UI control's on-hover image.
bool UI_SetImageOnClick(const char* ui_name, const char* control_name, const char* image_path);							// Updates a UI control's on-click image.
bool UI_SetColorOnHover(const char* ui_name, const char* control_name, color4_t color);									// Updates a UI control's on-hover colour.
bool UI_SetColorOnClick(const char* ui_name, const char* control_name, color4_t color);									// Updates a UI control's on-click colour.
bool UI_SetImageIsStretched(const char* ui_name, const char* control_name, bool is_stretched);							// Updates a UI control's stretched status.

bool UI_SetPassive(const char* ui_name, bool passive);																	// Sets a UI to passive (does not capture the mouse).
bool UI_SetStackable(const char* ui_name, bool stackable);																// Allows a UI to be pushed to the UI stack.
bool UI_UseScaledAssets(const char* ui_name, const char* control_name, bool use_scaled_assets);							// Allows an image to automatically scale.

// UI: Enable/Disable
bool UI_SetEnabled(const char* name, bool enabled);																// Sets a UI's enabled (visible) state.
bool UI_SetActivated(const char* name, bool activated);															// Sets a UI's active (tangible) state.

// UI: Set Event Handler

// Events for entire UIs
bool UI_SetEventOnUpdate(const char* ui_name, void (*func)());													// Sets a UI's on_update handler

// Events for UI controls
bool UI_SetEventOnClickDown(const char* ui_name, const char* control_name, void (*func)(int32_t btn, int32_t x, int32_t y));	// Sets a UI's on_click_down handler.
bool UI_SetEventOnKeyDown(const char* ui_name, const char* control_name, void (*func)(int32_t btn));							// Sets a UI's on_key_down handler.
bool UI_SetEventOnClickUp(const char* ui_name, const char* control_name, void (*func)(int32_t btn, int32_t x, int32_t y));		// Sets a UI's on_click_up handler.
bool UI_SetEventOnKeyUp(const char* ui_name, const char* control_name, void (*func)(int32_t btn));								// Sets a UI's on_key_up handler.
bool UI_SetEventOnUpdateControl(const char* ui_name, const char* control_name, void (*func)());									// Sets a UI control's on_update handler.

// UI: Event Handling
void UI_FireEventOnUpdate(ui_t* ui_ptr);																	// Fires UI on_update events.
void UI_FireEventOnClickDown(int32_t btn, int32_t x, int32_t y);											// Fires UI on_click_down events.
void UI_FireEventOnKeyDown(int32_t btn);																	// Fires UI on_key_down events.
void UI_FireEventOnClickUp(int32_t btn, int32_t x, int32_t y);												// Fires UI on_click_up events.
void UI_FireEventOnKeyUp(int32_t btn);																		// Fires UI on_key_up events.
void UI_FireEventOnUpdateControl(ui_control_t* control);													// Fires UI control on_update events.

// UI: Draw
void UI_Draw();																								// Draws a UI.

// UI: Clear
void UI_Clear(const char* name);																			// Removes all the controls in a UI.

// UI: Stack
// UI_PUSH IS NOT RECOMMENDED TO CALL - PUSHES A UI TO THE STACK! Used for internal purposes.
void UI_Push();																								// Pushes a UI to the UI stack.
void UI_Pop();																								// Pops a UI from the UI stack.

// UI: Internal
ui_t* UI_GetUI(const char* name);																			// Returns a pointer so NULL can be indicated for failure
ui_control_t* UI_GetControl(const char* ui_name, const char* name);											// Gets the control with name name in the ui UI.
void UI_Reset();																							// INTERNAL, resets all UI states to their defaults.																													// INTERNAL, deletes all UIs, and restarts the UI system


// UI: Create Scripts
// TeamUI
bool UI_TeamUICreate();													// Create the TDM-mode Team UI

// TeamWavesUI
bool UI_TeamWavesUICreate();											// Create the Waves-mode Team UI

// LeaderboardUI
bool UI_LeaderboardUICreate();
void UI_LeaderboardUIEnable(int32_t btn);								// Enables the Leaderboard UI.
void UI_LeaderboardUIDisable(int32_t btn);								// Disables the Leaderboard UI.

// Leaderboard utility functions
void UI_LeaderboardUIUpdate();

// BamfuslicatorUI
bool UI_BamfuslicatorUICreate();

// TimeUI
bool UI_TimeUICreate();

// ScoreUI
bool UI_ScoreUICreate();

// LoadoutUI
bool UI_LoadoutUICreate();

// Defines for all main menus
#define MAIN_MENU_HOVER_COLOR { 200, 200, 255, 255 }

// MainMenuUI
bool UI_MainMenuUICreate();

// MainMenuQuickstartUI
bool UI_MainMenuQuickstartUICreate();

// MainMenuBrowseServersUI
bool UI_MainMenuBrowseServersUICreate();

// MainMenuSettingsUI
bool UI_MainMenuSettingsUICreate();

// MainMenuZombieTVUI
bool UI_MainMenuZombieTVUICreate();

// MainMenuQuitUI
bool UI_MainMenuQuitUICreate();

// SettingsControlsUI
bool UI_SettingsControlsUICreate();

// SettingsGameUI
bool UI_SettingsGameUICreate();

// SettingsGraphicsUI
bool UI_SettingsGraphicsUICreate();

// SettingsSoundUI
bool UI_SettingsSoundUICreate();

// KillFeedUI
bool UI_KillFeedUICreate();

