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
// cl_ui_mainmenu_quit.c: Quit menu (June 16, 2024)

#include <client/client.hpp>

void UI_MainMenuQuitUIOnYesPressed(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuQuitUIOnNoPressed(int32_t btn, int32_t x, int32_t y);

bool UI_MainMenuQuitUICreate()
{
	UI_SetStackable("MainMenuQuitUI", true);
	UI_AddImage("MainMenuQuitUI", "UI_MainMenuQuitUI_Background", "2d/ui/mainmenu/mainmenuui_background", 0, 0, r_width->value, r_height->value);
	UI_SetImageIsStretched("MainMenuQuitUI", "UI_MainMenuQuitUI_Background", true);
	UI_UseScaledAssets("MainMenuQuitUI", "UI_MainMenuQuitUI_Background", true);

	//todo: randomised quit messgaes 

	color4_t box_colour = { 0, 0, 0, 180 };
	color4_t yes_colour = { 0, 200, 0, 180 };
	color4_t no_colour = { 200, 0, 0, 180 };

	UI_AddBox("MainMenuQuitUI", "MainMenuQuitUI_Box", 0.3f, 0.3f, 384, 192, box_colour);
	UI_AddText("MainMenuQuitUI", "MainMenuQuitUI_AreYouSure", "[STRING_AREYOUSURE]", 0.441f, 0.31f);
	UI_AddBox("MainMenuQuitUI", "MainMenuQuitUI_YesBox", 0.39f, 0.55f, 64, 24, yes_colour);
	UI_AddText("MainMenuQuitUI", "MainMenuQuitUI_Yes", "[STRING_YES]", 0.408f, 0.555f);
	UI_AddBox("MainMenuQuitUI", "MainMenuQuitUI_NoBox", 0.543f, 0.55f, 64, 24, no_colour);
	UI_AddText("MainMenuQuitUI", "MainMenuQuitUI_No", "[STRING_NO]", 0.564f, 0.555f);
	
	// set the font
	UI_SetFont("MainMenuQuitUI", "MainMenuQuitUI_AreYouSure", "bahnschrift_bold_18");
	UI_SetFont("MainMenuQuitUI", "MainMenuQuitUI_Yes", "bahnschrift_bold_18");
	UI_SetFont("MainMenuQuitUI", "MainMenuQuitUI_No", "bahnschrift_bold_18");

	UI_SetEventOnClickDown("MainMenuQuitUI", "MainMenuQuitUI_YesBox", UI_MainMenuQuitUIOnYesPressed);
	UI_SetEventOnClickDown("MainMenuQuitUI", "MainMenuQuitUI_NoBox", UI_MainMenuQuitUIOnNoPressed);

	color4_t yes_color_hover = { 0, 160, 0, 180 };
	color4_t no_color_hover = { 160, 0, 0, 180 };

	UI_SetColorOnHover("MainMenuQuitUI", "MainMenuQuitUI_YesBox", yes_color_hover);
	UI_SetColorOnHover("MainMenuQuitUI", "MainMenuQuitUI_NoBox", no_color_hover);

	// create ui
	return true;
}

void UI_MainMenuQuitUIOnYesPressed(int32_t btn, int32_t x, int32_t y)
{
	// perform same action as quitting using "quit" command
	CL_Quit_f();
}

// is this enough to justify adding global UI assets?
void UI_MainMenuQuitUIOnNoPressed(int32_t btn, int32_t x, int32_t y)
{
	UI_Pop();
}