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

#include <client/client.hpp>

// Functions only used in this translation unit
void UI_MainMenuSettingsUIOnBackPressed(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuSettingsUIOnOptionsGamePressed(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuSettingsUIOnOptionsControlsPressed(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuSettingsUIOnOptionsGraphicsPressed(int32_t btn, int32_t x, int32_t y);
void UI_MainMenuSettingsUIOnOptionsSoundPressed(int32_t btn, int32_t x, int32_t y);


bool UI_MainMenuSettingsUICreate()
{
	UI_SetStackable("MainMenuSettingsUI", true);
	UI_AddImage("MainMenuSettingsUI", "UI_MainMenuSettingsUI_Background", "2d/ui/mainmenu/mainmenusettingsui_background", 0, 0, r_width->value, r_height->value);
	UI_SetImageIsStretched("MainMenuSettingsUI", "UI_MainMenuSettingsUI_Background", true);
	UI_UseScaledAssets("MainMenuSettingsUI", "UI_MainMenuSettingsUI_Background", true);

	UI_AddImage("MainMenuSettingsUI", "MainMenuSettingsUI_Back", "2d/ui/global_btn_back", 0.75f, 0.87f, 256, 64);
	UI_SetImageOnHover("MainMenuSettingsUI", "MainMenuSettingsUI_Back", "2d/ui/global_btn_back_hover");
	UI_SetEventOnClickDown("MainMenuSettingsUI", "MainMenuSettingsUI_Back", UI_MainMenuSettingsUIOnBackPressed);

	float x = 0.01f;
	float y = 0.54f;

	// first level menu options
	// aligned with the back sign
	UI_AddText("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsGame", "[STRING_SETTINGSUI_OPTIONSGAME]", x, y);
	y += 0.06f;
	UI_AddText("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsControls", "[STRING_SETTINGSUI_OPTIONSCONTROLS]", x, y);
	y += 0.06f;
	UI_AddText("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsGraphics", "[STRING_SETTINGSUI_OPTIONSGRAPHICS]", x, y);
	y += 0.06f;
	UI_AddText("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsSound", "[STRING_SETTINGSUI_OPTIONSSOUND]", x, y);
	y += 0.06f;
	UI_AddText("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsReset", "[STRING_SETTINGSUI_OPTIONSRESET]", x, y);

	// set the font to big bahnschrift
	UI_SetFont("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsGame", "bahnschrift_bold_18");
	UI_SetFont("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsControls", "bahnschrift_bold_18");
	UI_SetFont("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsGraphics", "bahnschrift_bold_18");
	UI_SetFont("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsSound", "bahnschrift_bold_18");
	UI_SetFont("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsReset", "bahnschrift_bold_18");

	color4_t hover_colour = MAIN_MENU_HOVER_COLOR;

	UI_SetColorOnHover("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsGame", hover_colour);
	UI_SetColorOnHover("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsControls", hover_colour);
	UI_SetColorOnHover("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsGraphics", hover_colour);
	UI_SetColorOnHover("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsSound", hover_colour);
	UI_SetColorOnHover("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsReset", hover_colour);

	// Setup controls
	UI_SetEventOnClickDown("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsGame", UI_MainMenuSettingsUIOnOptionsGamePressed);
	UI_SetEventOnClickDown("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsControls", UI_MainMenuSettingsUIOnOptionsControlsPressed);
	UI_SetEventOnClickDown("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsSound", UI_MainMenuSettingsUIOnOptionsGraphicsPressed);
	UI_SetEventOnClickDown("MainMenuSettingsUI", "MainMenuSettingsUI_OptionsReset", UI_MainMenuSettingsUIOnOptionsSoundPressed);

	// create ui
	return true;
}

// is this enough to justify adding global UI assets?
void UI_MainMenuSettingsUIOnBackPressed(int32_t btn, int32_t x, int32_t y)
{
	UI_Pop();
}

void UI_MainMenuSettingsUIOnOptionsGamePressed(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("SettingsGameUI", true);
	UI_SetActivated("SettingsGameUI", true);
}

void UI_MainMenuSettingsUIOnOptionsControlsPressed(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("SettingsControlsUI", true);
	UI_SetActivated("SettingsControlsUI", true);
}

void UI_MainMenuSettingsUIOnOptionsGraphicsPressed(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("SettingsGameUI", true);
	UI_SetActivated("SettingsGameUI", true);
}

void UI_MainMenuSettingsUIOnOptionsSoundPressed(int32_t btn, int32_t x, int32_t y)
{
	UI_SetEnabled("SettingsSoundUI", true);
	UI_SetActivated("SettingsSoundUI", true);
}