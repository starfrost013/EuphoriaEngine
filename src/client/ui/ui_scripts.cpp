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

// ui_scripts.c: ZombonoUI UI creation scripts
// 12/14/2023: Created
// Note - LeaderboardUI is in cl_ui_leaderboard.c

#include <client/client.hpp>

// 
// TeamUI
// 

void UI_TeamUISetDirectorTeam(int32_t btn, int32_t x, int32_t y)
{
	UI_SetActivated("TeamUI", false);
	UI_SetEnabled("TeamUI", false);
	MSG_WriteByte(&cls.netchan.message, clc_event);
	MSG_WriteByte(&cls.netchan.message, event_type_cl_player_set_team);
	MSG_WriteByte(&cls.netchan.message, 1);
}

void UI_TeamUISetPlayerTeam(int32_t btn, int32_t x, int32_t y)
{
	UI_SetActivated("TeamUI", false);
	UI_SetEnabled("TeamUI", false);
	MSG_WriteByte(&cls.netchan.message, clc_event);
	MSG_WriteByte(&cls.netchan.message, event_type_cl_player_set_team);
	MSG_WriteByte(&cls.netchan.message, 2);
}

bool UI_TeamUICreate()
{
	UI_AddText("TeamUI", "TeamUI_TeamSelectText", "[STRING_TEAMUI_SELECT]", 0.461f, 0.375f);
	UI_AddImage("TeamUI", "TeamUI_DirectorTeam", "2d/ui/teamui_btn_director", 0.25f, 0.4f, 256, 128);
	UI_AddImage("TeamUI", "TeamUI_PlayerTeam", "2d/ui/teamui_btn_player", 0.5f, 0.4f, 256, 128);

	// set on hover images 
	UI_SetImageOnHover("TeamUI", "TeamUI_DirectorTeam", "2d/ui/teamui_btn_director_hover");
	UI_SetImageOnHover("TeamUI", "TeamUI_PlayerTeam", "2d/ui/teamui_btn_player_hover");

	UI_AddText("TeamUI", "TeamUI_DirectorText", "[STRING_TEAMUI_HINT_DIRECTOR]", 0.25f, 0.575f);
	UI_AddText("TeamUI", "TeamUI_PlayerText", "[STRING_TEAMUI_HINT_PLAYER]", 0.5f, 0.575f);

	UI_SetEventOnClickDown("TeamUI", "TeamUI_DirectorTeam", UI_TeamUISetDirectorTeam);
	UI_SetEventOnClickDown("TeamUI", "TeamUI_PlayerTeam", UI_TeamUISetPlayerTeam);
	return true; 
}

//
// TeamWavesUI
// I could just change the text based on the gamemode
// but I intend to have a different UI here
//

void UI_TeamWavesUISetDirectorTeam(int32_t btn, int32_t x, int32_t y)
{
	if (current_ui == NULL) return;
	if (strncmp(current_ui->name, "TeamWavesUI", 11)) return;

	UI_SetActivated("TeamWavesUI", false);
	UI_SetEnabled("TeamWavesUI", false);
	MSG_WriteByte(&cls.netchan.message, clc_event);
	MSG_WriteByte(&cls.netchan.message, event_type_cl_player_set_team);
	MSG_WriteByte(&cls.netchan.message, 1);
}

void UI_TeamWavesUISetPlayerTeam(int32_t btn, int32_t x, int32_t y)
{
	if (current_ui == NULL) return;
	if (strncmp(current_ui->name, "TeamWavesUI", 11)) return;

	UI_SetActivated("TeamWavesUI", false);
	UI_SetEnabled("TeaWavesUI", false);
	MSG_WriteByte(&cls.netchan.message, clc_event);
	MSG_WriteByte(&cls.netchan.message, event_type_cl_player_set_team);
	MSG_WriteByte(&cls.netchan.message, 2);
}

bool UI_TeamWavesUICreate()
{
	UI_AddText("TeamWavesUI", "TeamWavesUI_TeamSelectText", "[STRING_TEAMUI_SELECT]", 0.461f, 0.375f);
	UI_AddImage("TeamWavesUI", "TeamWavesUI_DirectorTeam", "2d/ui/teamui_btn_director", 0.25f, 0.4f, 256, 128);
	UI_AddImage("TeamWavesUI", "TeamWavesUI_PlayerTeam", "2d/ui/teamui_btn_player", 0.5f, 0.4f, 256, 128);

	// set on hover images 
	UI_SetImageOnHover("TeamWavesUI", "TeamWavesUI_DirectorTeam", "2d/ui/teamui_btn_director_hover");
	UI_SetImageOnHover("TeamWavesUI", "TeamWavesUI_PlayerTeam", "2d/ui/teamui_btn_player_hover");

	UI_AddText("TeamWavesUI", "TeamWavesUI_DirectorText", "[STRING_TEAMWAVESUI_HINT_DIRECTOR]", 0.25f, 0.575f);
	UI_AddText("TeamWavesUI", "TeamWavesUI_PlayerText", "[STRING_TEAMWAVESUI_HINT_PLAYER]", 0.5f, 0.575f);

	UI_SetEventOnClickDown("TeamWavesUI", "TeamWavesUI_DirectorTeam", UI_TeamUISetDirectorTeam);
	UI_SetEventOnClickDown("TeamWavesUI", "TeamWavesUI_PlayerTeam", UI_TeamUISetPlayerTeam);
	return true;
}


//
// BamfuslicatorUI
//

bool UI_BamfuslicatorUICreate()
{
	int32_t size_x = 0, size_y = 0;
	UI_SetPassive("BamfuslicatorUI", true);
	// we only care about precise Y size here
	// as all the text actually being drawn is smaller than the temp text, this positions it in the middle
	const char* temp_text = "Zombie Type: **** UNKNOWN ZOMBIE TYPE ****"; 
	Text_GetSize(cl_system_font->string, &size_x, &size_y, temp_text);
	float x = 0.375f;
	float y = 0.87f;

	color4_t color = { 232, 96, 0, 127 };

	UI_AddBox("BamfuslicatorUI", "BamfuslicatorUI_TextBackground", x, 
		y, 240, size_y, color);
	// terrible manual positioning because we don't know what the text is (changed at runtime)
	UI_AddText("BamfuslicatorUI", "BamfuslicatorUI_Text", temp_text, (x + 0.0675f), y + 0.001f);
	UI_AddText("BamfuslicatorUI", "BamfuslicatorUI_TextHelp", "Right click to change type of monster to spawn",
		x + 0.00625f, y + 0.0225f); // lazy way of centering it lol
	return true; 
}

//
// TimeUI
//

bool UI_TimeUICreate()
{
	int32_t size_x = 0, size_y = 0;

	UI_SetPassive("TimeUI", true);

	const char* temp_text = "00:00";
	Text_GetSize("bahnschrift_bold_18", &size_x, &size_y, temp_text);

	color4_t colour = { 255, 0, 0, 150 };

	UI_AddBox("TimeUI", "TimeUI_TextBox", 0.5f - ((size_x/UI_SCALE_BASE_X) / 2) - 0.03f, 0.051f,
		size_x + 72, size_y, colour); // add a buffer of 10 pixels for larger numbers

	// text is set by gamecode
	UI_AddText("TimeUI", "TimeUI_Text", "N/A", 0.5f - ((size_x / UI_SCALE_BASE_X) / 2), 0.056f); //padding/advance reasons
	UI_SetFont("TimeUI", "TimeUI_Text", "bahnschrift_bold_18");

	return true; 
}

//
// ScoreUI
//

bool UI_ScoreUICreate()
{
	int32_t size_x = 0, size_y = 0;

	UI_SetPassive("ScoreUI", true);

	color4_t director_colour = { 87, 0, 127, 255 };
	color4_t player_colour = { 255, 106, 0, 255 };

	const char* temp_text = "Directors 0 : Players 0";
	Text_GetSize(cl_system_font->string, &size_x, &size_y, temp_text);
	//87, 0, 127, 255
	UI_AddBox("ScoreUI", "ScoreUI_TextBoxDirector", 0.5f - ((size_x/UI_SCALE_BASE_X) / 2) - 0.05f, 0.02f,
		(size_x + 48 / 2), size_y, director_colour); // add a buffer of 10 pixels for larger numbers 
	UI_AddBox("ScoreUI", "ScoreUI_TextBoxPlayer", 0.5f - ((size_x / UI_SCALE_BASE_X) / 2) + (size_x/UI_SCALE_BASE_X)/2, 0.02f,
		(size_x + 48 / 2), size_y, player_colour); // add a buffer of 10 pixels for larger numbers 

	// text is set by gamecode
	UI_AddText("ScoreUI", "ScoreUI_Text", "N/A", 0.5f - ((size_x)/UI_SCALE_BASE_X / 2) + 0.01f, 
		0.02f);

	return true; 
}

//
// LoadoutUI
//

bool UI_LoadoutUICreate()
{
	UI_SetPassive("LoadoutUI", true);

	// set up size
	int32_t size_x = 288;
	int32_t size_y = 68;

	color4_t background_color = { 0, 0, 0, 127 };

	float x = ((0.5f) - ((float)size_x/2)/UI_SCALE_BASE_X); 
	float y = 0.725f;

	UI_AddBox("LoadoutUI", "LoadoutUI_Background", x, y, size_x, size_y,
		background_color);

	x += 0.02f;

	// create the text
	y += 0.01f;
	UI_AddText("LoadoutUI", "LoadoutUI_Text", "THIS IS A BUG", x, y);

	// create 10 placeholder icons
	y += 0.02f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option0", "2d/i_fixme", x, y, 24, 24);

	// some of these are manually tweaked because there is no point caling Text_GetSize for the numbers 0 to 9
	UI_AddText("LoadoutUI", "LoadoutUI_Text1", "1", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option1", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text2", "2", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option2", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text3", "3", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option3", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text4", "4", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option4", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text5", "5", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option5", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text6", "6", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option6", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text7", "7", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option7", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text8", "8", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option8", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text9", "9", x + 0.01f, y + 0.038f);
	x += 0.025f;
	UI_AddImage("LoadoutUI", "LoadoutUI_Option9", "2d/i_fixme", x, y, 24, 24);
	UI_AddText("LoadoutUI", "LoadoutUI_Text9", "0", x + 0.01f, y + 0.038f);
	x += 0.025f;

	// set everything except the box invisible to start with because you have no items
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Text", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option0", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option1", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option2", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option3", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option4", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option5", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option6", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option7", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option8", true);
	UI_SetInvisible("LoadoutUI", "LoadoutUI_Option9", true);

	return true; 
}
