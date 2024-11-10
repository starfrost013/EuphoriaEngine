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
// ui_game_interface.cpp
// Loads the GameUI Interface.
// 1 November 2024
#include <client/include/ui.hpp>
#include <client/include/ui_game_interface.hpp>

engine_ui_api_t engine_api;

engine_ui_api_t* UI_InitInterface()
{
	//UI Core functions
	engine_api.UI_Add = UI_Add;
	engine_api.UI_AddBox = UI_AddBox;
	engine_api.UI_AddCheckbox = UI_AddCheckbox;
	engine_api.UI_AddImage = UI_AddImage;
	engine_api.UI_AddSlider = UI_AddSlider;
	engine_api.UI_AddText = UI_AddText;

	// Clear stuff
	engine_api.UI_Clear = UI_Clear;

	// UI: Getting UI objects
	engine_api.UI_GetControl = UI_GetControl;
	engine_api.UI_GetUI = UI_GetUI;

	// Stack and misc stuff
	engine_api.UI_Pop = UI_Pop;
	engine_api.UI_Push = UI_Push;
	engine_api.UI_Reset = UI_Reset;

	// Event API
	engine_api.UI_SetEventOnClickDown = UI_SetEventOnClickDown;
	engine_api.UI_SetEventOnClickUp = UI_SetEventOnClickUp;
	engine_api.UI_SetEventOnKeyDown = UI_SetEventOnKeyDown;
	engine_api.UI_SetEventOnKeyUp = UI_SetEventOnKeyUp;
	engine_api.UI_SetEventOnUpdate = UI_SetEventOnUpdate;
	engine_api.UI_SetEventOnUpdateControl = UI_SetEventOnUpdateControl;
	
	// UI Control Property API
	engine_api.UI_SetActivated = UI_SetActivated;
	engine_api.UI_SetColorOnClick = UI_SetColorOnClick;
	engine_api.UI_SetColorOnHover = UI_SetColorOnHover;
	engine_api.UI_SetEnabled = UI_SetEnabled;
	engine_api.UI_SetFont = UI_SetFont;
	engine_api.UI_SetImage = UI_SetImage;
	engine_api.UI_SetImageIsStretched = UI_SetImageIsStretched;
	engine_api.UI_SetImageOnClick = UI_SetImageOnClick;
	engine_api.UI_SetImageOnHover = UI_SetImageOnHover;
	engine_api.UI_SetInvisible = UI_SetInvisible;
	engine_api.UI_SetPassive = UI_SetPassive;
	engine_api.UI_SetPosition = UI_SetPosition;
	engine_api.UI_SetSize = UI_SetSize;
	engine_api.UI_SetStackable = UI_SetStackable;
	engine_api.UI_SetText = UI_SetText;
	engine_api.UI_UseScaledAssets = UI_UseScaledAssets;

	return &engine_api;
}