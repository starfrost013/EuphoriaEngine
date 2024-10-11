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
// cl_ui_mainmenu.c: New! main menu (April 20, 2024)

#include <client/client.hpp>

void UI_MainMenuGoToQuickstart(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuGoToBrowseServers(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuGoToZombieTV(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuGoToSettings(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuGoToQuit(int32_t btn, int32_t x, int32_t y);

// Create the root of the main menu UI
bool UI_MainMenuUICreate()
{
	UI_SetStackable("MainMenuUI", true);
	UI_AddImage("MainMenuUI", "MainMenuUI_Background", "2d/ui/mainmenu/mainmenuui_background", 0, 0, r_width->value, r_height->value);
	UI_SetImageIsStretched("MainMenuUI", "MainMenuUI_Background", true);
	UI_UseScaledAssets("MainMenuUI", "MainMenuUI_Background", true);

	float x = 0.01f;
	float y = 0.54f;
	//todo: scale these?
	UI_AddText("MainMenuUI", "MainMenuUI_BtnQuickstart", "[STRING_MAINMENUUI_QUICKSTART]", x, y);
	UI_SetEventOnClickDown("MainMenuUI", "MainMenuUI_BtnQuickstart", UI_MainMenuGoToQuickstart);
	y += 0.06f;
	UI_AddText("MainMenuUI", "MainMenuUI_BtnBrowseServers", "[STRING_MAINMENUUI_BROWSESERVERS]", x, y);
	UI_SetEventOnClickDown("MainMenuUI", "MainMenuUI_BtnBrowseServers", UI_MainMenuGoToBrowseServers);
	y += 0.06f;
	UI_AddText("MainMenuUI", "MainMenuUI_BtnZombieTelevision", "[STRING_MAINMENUUI_ZOMBIETELEVISION]", x, y);
	UI_SetEventOnClickDown("MainMenuUI", "MainMenuUI_BtnZombieTV", UI_MainMenuGoToZombieTV);
	y += 0.06f;
	UI_AddText("MainMenuUI", "MainMenuUI_BtnSettings", "[STRING_MAINMENUUI_SETTINGS]", x, y);
	UI_SetEventOnClickDown("MainMenuUI", "MainMenuUI_BtnSettings", UI_MainMenuGoToSettings);
	y += 0.06f;
	UI_AddText("MainMenuUI", "MainMenuUI_BtnQuit", "[STRING_MAINMENUUI_QUIT]", x, y);
	UI_SetEventOnClickDown("MainMenuUI", "MainMenuUI_BtnQuit", UI_MainMenuGoToQuit);
	y += 0.06f;

	UI_SetFont("MainMenuUI", "MainMenuUI_BtnQuickstart", "bahnschrift_bold_18");
	UI_SetFont("MainMenuUI", "MainMenuUI_BtnBrowseServers", "bahnschrift_bold_18");
	UI_SetFont("MainMenuUI", "MainMenuUI_BtnZombieTelevision", "bahnschrift_bold_18");
	UI_SetFont("MainMenuUI", "MainMenuUI_BtnSettings", "bahnschrift_bold_18");
	UI_SetFont("MainMenuUI", "MainMenuUI_BtnQuit", "bahnschrift_bold_18");

	color4_t hover_colour = MAIN_MENU_HOVER_COLOR;

	UI_SetColorOnHover("MainMenuUI", "MainMenuUI_BtnQuickstart", hover_colour);
	UI_SetColorOnHover("MainMenuUI", "MainMenuUI_BtnBrowseServers", hover_colour);
	UI_SetColorOnHover("MainMenuUI", "MainMenuUI_BtnZombieTelevision", hover_colour);
	UI_SetColorOnHover("MainMenuUI", "MainMenuUI_BtnSettings", hover_colour);
	UI_SetColorOnHover("MainMenuUI", "MainMenuUI_BtnQuit", hover_colour);

	return true;
}

// Go to quickstart menu
void UI_MainMenuGoToQuickstart(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("MainMenuQuickstartUI", true);
	UI_SetActivated("MainMenuQuickstartUI", true);
}

// Go to browse servers menu
void UI_MainMenuGoToBrowseServers(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("MainMenuBrowseServersUI", true);
	UI_SetActivated("MainMenuBrowseServersUI", true);
}

// Go to Zombie TV menu
void UI_MainMenuGoToZombieTV(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("MainMenuZombieTVUI", true);
	UI_SetActivated("MainMenuZombieTVUI", true);
}

// Go to Settings menu
void UI_MainMenuGoToSettings(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("MainMenuSettingsUI", true);
	UI_SetActivated("MainMenuSettingsUI", true);
}

// Go to quit menu
void UI_MainMenuGoToQuit(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("MainMenuQuitUI", true);
	UI_SetActivated("MainMenuQuitUI", true);
}
