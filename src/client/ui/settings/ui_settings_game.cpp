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
// ui_settings_game.c: Game Settings (August 17, 2024)

#include <client/client.hpp>

void UI_SettingsGameUIOnBackPressed(int32_t btn, int32_t x, int32_t y);

bool UI_SettingsGameUICreate()
{

	UI_SetStackable("SettingsGameUI", true);

	// background
	UI_AddImage("SettingsGameUI", "UI_SettingsGameUI_Background", "2d/ui/mainmenu/settingsgameui_background", 0, 0, r_width->value, r_height->value);
	UI_SetImageIsStretched("SettingsGameUI", "UI_SettingsGameUI_Background", true);
	UI_UseScaledAssets("SettingsGameUI", "UI_SettingsGameUI_Background", true);

	UI_AddImage("SettingsGameUI", "SettingsGameUI_Back", "2d/ui/global_btn_back", 0.75f, 0.87f, 256, 64);
	UI_SetImageOnHover("SettingsGameUI", "SettingsGameUI_Back", "2d/ui/global_btn_back_hover");
	UI_SetEventOnClickDown("SettingsGameUI", "SettingsGameUI_Back", UI_SettingsGameUIOnBackPressed);

	float x = 0.40f;
	float y = 0.23f;

	// Game options
	UI_AddText("SettingsGameUI", "SettingsGameUI_Header", "Game Settings", x, y);

	return true;
}

// is this enough to justify adding global UI assets?
void UI_SettingsGameUIOnBackPressed(int32_t btn, int32_t x, int32_t y)
{
	UI_Pop();
}