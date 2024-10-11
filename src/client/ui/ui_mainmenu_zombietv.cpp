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
// cl_ui_mainmenu_zombietv.c: Zombie Television menu (June 16, 2024)

#include <client/client.hpp>

void UI_MainMenuZombieTVUIOnBackPressed(int32_t btn, int32_t x, int32_t y);

bool UI_MainMenuZombieTVUICreate()
{
	UI_SetStackable("MainMenuZombieTVUI", true);
	UI_AddImage("MainMenuZombieTVUI", "UI_MainMenuZombieTVUI_Background", "2d/ui/mainmenu/mainmenuzombietvui_background", 0, 0, r_width->value, r_height->value);
	UI_SetImageIsStretched("MainMenuZombieTVUI", "UI_MainMenuZombieTVUI_Background", true);
	UI_UseScaledAssets("MainMenuZombieTVUI", "UI_MainMenuZombieTVUI_Background", true);

	UI_AddImage("MainMenuZombieTVUI", "MainMenuZombieTVUI_Back", "2d/ui/global_btn_back", 0.75f, 0.87f, 256, 64);
	UI_SetImageOnHover("MainMenuZombieTVUI", "MainMenuZombieTVUI_Back", "2d/ui/global_btn_back_hover");
	UI_SetEventOnClickDown("MainMenuZombieTVUI", "MainMenuZombieTVUI_Back", UI_MainMenuZombieTVUIOnBackPressed);

	// create uI
	return true;
}

// is this enough to justify adding global UI assets?
void UI_MainMenuZombieTVUIOnBackPressed(int32_t btn, int32_t x, int32_t y)
{
	UI_Pop();
}