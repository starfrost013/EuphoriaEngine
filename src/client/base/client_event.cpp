/*
Copyright (C) 1997-2001 Id Software, Inc.
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
#include <client/client.hpp>

// client_event.c
// Parses client events.
// A client event is a way for the server to let the client know when something happens.

void CL_ParseEvent()
{
	event_type_sv event_id = (event_type_sv)MSG_ReadByte(&net_message);

	char* s;
	// These are for multi-string messages
	char str_tempbuf[MAX_UI_STRLEN] = { 0 };
	char str_tempbuf2[MAX_UI_STRLEN] = { 0 };

	//c2360 MSVC
	bool enabled = false;
	bool activated = false; 

	switch (event_id)
	{
		// these are intended for future uis
	default:
		Com_Error(ERR_DROP, "Invalid server-to-client event type %d", event_id);
		break;
	case event_type_sv_game_start:
	case event_type_sv_game_end:
	case event_type_sv_player_joined:
	case event_type_sv_player_left:
		Com_Printf("Server-to-client event type %d not yet implemented\n", event_id);
		break;
	case event_type_sv_player_killed: //killfeed
		Killfeed_Add();
		break;
	// Loadout events
	case event_type_sv_loadout_add:
		Loadout_Add();
		break;
	case event_type_sv_loadout_clear:
		Loadout_Clear();
		break;
	case event_type_sv_loadout_remove:
		Loadout_Remove(MSG_ReadString(&net_message));
		break;
	case event_type_sv_loadout_setcurrent:
		Loadout_SetCurrent(MSG_ReadByte(&net_message));
		break;
	// Leaderboard events
	case event_type_sv_leaderboard_update:
		UI_LeaderboardUIUpdate();
		break;
	case event_type_sv_leaderboard_draw:
		UI_LeaderboardUIEnable(K_TAB);
		break;
	// UI events
	case event_type_sv_ui_draw:
		s = MSG_ReadString(&net_message);
		// Active can only be changed from UI script

		enabled = (bool)MSG_ReadByte(&net_message);
		activated = (bool)MSG_ReadByte(&net_message);

		UI_SetEnabled(s, enabled);
		UI_SetActivated(s, activated);
		break;

	case event_type_sv_ui_set_text:
		s = MSG_ReadString(&net_message); // UI name
		strncpy(str_tempbuf, s, MAX_UI_STRLEN);
		s = MSG_ReadString(&net_message); // Name
		strncpy(str_tempbuf2, s, MAX_UI_STRLEN);
		s = MSG_ReadString(&net_message); // New Text

		UI_SetText(str_tempbuf, str_tempbuf2, s);

		break;

	case event_type_sv_ui_set_image:
		s = MSG_ReadString(&net_message); // UI name
		strncpy(str_tempbuf, s, MAX_UI_STRLEN);
		s = MSG_ReadString(&net_message); // Name
		strncpy(str_tempbuf2, s, MAX_UI_STRLEN);
		s = MSG_ReadString(&net_message); // New image path

		UI_SetImage(str_tempbuf, str_tempbuf2, s);
		break;

	}
}