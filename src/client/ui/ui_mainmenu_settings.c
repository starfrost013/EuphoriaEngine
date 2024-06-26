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
// cl_ui_mainmenu_settings.c: Settings menu (June 16, 2024)

#include <client/client.h>

void UI_MainMenuSettingsUIOnBackPressed(uint32_t btn, int32_t x, int32_t y);

bool UI_MainMenuSettingsUICreate()
{
	if (!ui_newmenu->value)
		return true;

	UI_SetStackable("MainMenuSettingsUI", true);
	UI_AddImage("MainMenuSettingsUI", "UI_MainMenuSettingsUI_Background", "2d/ui/mainmenusettingsui_background", 0, 0, r_width->value, r_height->value);
	UI_SetImageIsStretched("MainMenuSettingsUI", "UI_MainMenuSettingsUI_Background", true);
	UI_UseScaledAssets("MainMenuSettingsUI", "UI_MainMenuSettingsUI_Background", true);

	UI_AddImage("MainMenuSettingsUI", "MainMenuSettingsUI_Back", "2d/ui/global_btn_back", 0.1f, 0.77f, 256, 64);
	UI_SetImageOnHover("MainMenuSettingsUI", "MainMenuSettingsUI_Back", "2d/ui/global_btn_back_hover");
	UI_SetEventOnClickDown("MainMenuSettingsUI", "MainMenuSettingsUI_Back", UI_MainMenuSettingsUIOnBackPressed);

	// create uI
	return true;
}

// is this enough to justify adding global UI assets?
void UI_MainMenuSettingsUIOnBackPressed(uint32_t btn, int32_t x, int32_t y)
{
	UI_Pop();
}