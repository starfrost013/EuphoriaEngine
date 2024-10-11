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

// cl_ui_leaderboard.c: The new leaderboard UI! (December 24-25, 2023)

#include <client/client.hpp>

#define TEXT_BUF_LENGTH			16
#define TEXT_BUF_LENGTH_LONG	38

bool UI_LeaderboardUICreate()
{
	UI_SetPassive("LeaderboardUI", true);
	// SIZE SHOULDN'T HAVE TO BE MULTIPLIED BY VID_HUDSCALE

	color4_t color = { 0, 0, 0, 150 };

	UI_AddBox("LeaderboardUI", "LeaderboardUI_Box", 0.17f, 0.2f, 640, 384, color);
	UI_SetEventOnKeyDown("LeaderboardUI", "LeaderboardUI_Box", UI_LeaderboardUIEnable);
	UI_SetEventOnKeyUp("LeaderboardUI", "LeaderboardUI_Box", UI_LeaderboardUIDisable);
	UI_AddImage("LeaderboardUI", "LeaderboardUI_Header", "2d/ui/leaderboardui_header", 0.333f, 0.2f, 320, 64);
	UI_AddText("LeaderboardUI", "LeaderboardUI_Subheader_Name", "Name", 0.183f, 0.308f);
	UI_AddText("LeaderboardUI", "LeaderboardUI_Subheader_Ping", "Ping", 0.35f, 0.308f);
	UI_AddText("LeaderboardUI", "LeaderboardUI_Subheader_Team", "Team", 0.433f, 0.308f);
	UI_AddText("LeaderboardUI", "LeaderboardUI_Subheader_Score", "Score", 0.533f, 0.308f);
	UI_AddText("LeaderboardUI", "LeaderboardUI_Subheader_Time", "Time", 0.616f, 0.308f);
	UI_AddText("LeaderboardUI", "LeaderboardUI_Subheader_Spectating", "Spectating?", 0.70f, 0.308f);
	return true;
}

void UI_LeaderboardUIEnable(int32_t btn)
{
	if (btn == K_TAB)
	{
		if (current_ui != NULL)
		{
			// ugly hack so we can't override teamui
			if (!strncmp(current_ui->name, "TeamUI", 6)) return;
		}

		// fucking hack
		ui_t* leaderboard_ui_ptr = UI_GetUI("LeaderboardUI");

		UI_SetEnabled("LeaderboardUI", true);
		UI_SetActivated("LeaderboardUI", true);
	}
}

void UI_LeaderboardUIDisable(int32_t btn)
{
	if (btn == K_TAB)
	{
		if (current_ui != NULL)
		{
			// ugly hack so we can't override teamui
			if (!strncmp(current_ui->name, "TeamUI", 6)) return;
		}

		// fucking hack
		ui_t* leaderboard_ui_ptr = UI_GetUI("LeaderboardUI");

		UI_SetEnabled("LeaderboardUI", false);
		UI_SetActivated("LeaderboardUI", false);
	}
}

void UI_LeaderboardUIUpdate()
{
	float x, y;
	char text[TEXT_BUF_LENGTH];
	// for team score TODO: we do this twice in different places
	int32_t director_score = 0, player_score = 0;
	font_t* system_font_ptr = Font_GetByName(cl_system_font->string);

	// stupid hack
	UI_Clear("LeaderboardUI");
	UI_LeaderboardUICreate();

	// byte to reduce net usage
	cl.leaderboard.num_clients = MSG_ReadByte(&net_message);

	y = 0.302f;

	// update all the data here so we don't need to clear it
	for (int32_t client_num = 0; client_num < cl.leaderboard.num_clients; client_num++)
	{
		// reset x
		x = 0.183f;

		leaderboard_entry_t leaderboard_entry = cl.leaderboard.entries[client_num];
		strncpy(leaderboard_entry.name, MSG_ReadString(&net_message), PLAYER_NAME_LENGTH);
		leaderboard_entry.ping = MSG_ReadShort(&net_message);
		leaderboard_entry.score = MSG_ReadShort(&net_message);
		leaderboard_entry.team = (player_team)MSG_ReadShort(&net_message);
		leaderboard_entry.time = MSG_ReadShort(&net_message); //should this be an int?
		leaderboard_entry.is_spectator = MSG_ReadByte(&net_message);
		strncpy(leaderboard_entry.map_name, MSG_ReadString(&net_message), MAP_NAME_LENGTH);
		leaderboard_entry.time_remaining = MSG_ReadShort(&net_message);

		// todo: boxes and headers (headers in cl_ui_scripts)
		
		// move downward by one line from the header
		y += ((system_font_ptr->line_height/UI_SCALE_BASE_Y) - 0.002f); // safety

		// draw name
		UI_AddText("LeaderboardUI", "LeaderboardUIText_TempName", leaderboard_entry.name, x, y);

		x += 0.16667f;
		
		// ping
		snprintf(text, TEXT_BUF_LENGTH, "%d", leaderboard_entry.ping);
		UI_AddText("LeaderboardUI", "LeaderboardUIText_TempPing", text, x, y);

		x += 0.08333f;

		//team
		int32_t box_size_x = 8 * 11; // a bit of padding
		int32_t box_size_y = (system_font_ptr->line_height - 1); // -1 because it looks weird with noen

		color4_t color_director = { 87, 0, 127, 255 };
		color4_t color_team = { 219, 87, 0, 255 };
		color4_t color_unassigned = { 127, 127, 127, 255 };
		
		if (leaderboard_entry.team == team_director)
		{
			UI_AddBox("LeaderboardUI", "LeaderboardUIText_TempTeamBox", x, y, box_size_x, box_size_y, color_director);
			UI_AddText("LeaderboardUI", "LeaderboardUIText_TempTeam", "Director", x, y);
			director_score += leaderboard_entry.score;

		}
		else if (leaderboard_entry.team == team_player)
		{
			UI_AddBox("LeaderboardUI", "LeaderboardUIText_TempTeamBox", x, y, box_size_x, box_size_y, color_team);
			UI_AddText("LeaderboardUI", "LeaderboardUIText_TempTeam", "Player", x, y);
			player_score += leaderboard_entry.score;
		}
		else
		{
			UI_AddBox("LeaderboardUI", "LeaderboardUIText_TempTeamBox", x, y, box_size_x, box_size_y, color_unassigned);
			UI_AddText("LeaderboardUI", "LeaderboardUIText_TempTeam", "Unassigned", x, y);
		}

		x += 0.1f;
		
		// score
		snprintf(text, TEXT_BUF_LENGTH, "%d", leaderboard_entry.score);
		UI_AddText("LeaderboardUI", "LeaderboardUIText_TempScore", text, x, y);

		// time
		x += 0.08333f;
		snprintf(text, TEXT_BUF_LENGTH, "%d minutes", leaderboard_entry.time);
		UI_AddText("LeaderboardUI", "LeaderboardUIText_TempTime", text, x, y);

		x += 0.08333f;

		// are they spectating?
		if (leaderboard_entry.is_spectator)
		{
			UI_AddText("LeaderboardUI", "LeaderboardUIText_TempIsSpectating", "Yes", x, y);
		}
		else
		{
			UI_AddText("LeaderboardUI", "LeaderboardUIText_TempIsSpectating", "No", x, y);
		}

		// stupid fucking hack!!!

		if (client_num == 0)
		{
			x = 0.171f;
			y = 0.66f;

			// 38 map name length + 7 for "Time: " and optional 0
			char map_buf[TEXT_BUF_LENGTH_LONG];
			char time_buf[TEXT_BUF_LENGTH_LONG];

			snprintf(map_buf, TEXT_BUF_LENGTH_LONG, "Map: %s", leaderboard_entry.map_name);

			UI_AddText("LeaderboardUI", "LeaderboardUIText_TempMapName", map_buf, x, y);

			y += ((system_font_ptr->line_height)/UI_SCALE_BASE_Y) - 0.002f;

			// get the time remainig
			int32_t minutes = leaderboard_entry.time_remaining / 60;
			int32_t seconds = leaderboard_entry.time_remaining % 60;

			if (minutes < 10)
			{
				if (seconds < 10)
				{
					snprintf(time_buf, TEXT_BUF_LENGTH_LONG, "Time: 0%d:0%d", minutes, seconds);
				}
				else
				{
					snprintf(time_buf, TEXT_BUF_LENGTH_LONG, "Time: 0%d:%d", minutes, seconds);
				}
			}
			else
			{
				if (seconds < 10)
				{
					snprintf(time_buf, TEXT_BUF_LENGTH_LONG, "Time: %d:0%d", minutes, seconds);
				}
				else
				{
					snprintf(time_buf, TEXT_BUF_LENGTH_LONG, "Time: %d:%d", minutes, seconds);
				}
			}

			UI_AddText("LeaderboardUI", "LeaderboardUIText_TempTime", time_buf, x, y);
		}

		y = (0.302f + (((system_font_ptr->line_height/UI_SCALE_BASE_Y) * (client_num + 1))));
	}
	
	x = 0.35f;
	y = 0.287f;

	// "Director: " + 4 numbers + 1 for safety
	char director_text[TEXT_BUF_LENGTH];
	char player_text[TEXT_BUF_LENGTH]; 

	// draw the director and player total scores
	//TODO: ONLY DO THIS ON TDM MODE!!!!!!!!!!!!!!!!!!!!!!!!!!

	snprintf(director_text, TEXT_BUF_LENGTH, "Directors: %d", director_score);
	snprintf(player_text, TEXT_BUF_LENGTH, "Players: %d", player_score);

	color4_t color_director = { 87, 0, 127, 255 };
	color4_t color_player = { 219, 87, 0, 255 };


	int32_t box_size_large = (8 * 14);

	UI_AddBox("LeaderboardUI", "LeaderboardUIText_TempDirectorScoreBox", x, y, box_size_large, (system_font_ptr->line_height - 1), color_director); 	// todo: define team colours somewhere
	UI_AddText("LeaderboardUI", "LeaderboardUIText_TempDirectorScore", director_text, x, y);

	x = 0.50f;

	UI_AddBox("LeaderboardUI", "LeaderboardUIText_TempPlayerScoreBox", x, y, box_size_large, (system_font_ptr->line_height - 1), color_player); 	// todo: define team colours somewhere
	UI_AddText("LeaderboardUI", "LeaderboardUIText_TempPlayerScore", player_text, x, y);
}