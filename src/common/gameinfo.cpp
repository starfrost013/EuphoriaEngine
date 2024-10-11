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

// Gameinfo

#include "common.hpp"

#define GAMEINFO_PATH "gameinfo.json"

gameinfo_t gameinfo;

// copied from gameinfo struct
cvar_t* game_name;
cvar_t* game_asset_path;

void Gameinfo_Load()
{
	game_name = Cvar_Get("game_name", "Zombono", CVAR_LATCH | CVAR_SERVERINFO);
	game_asset_path = Cvar_Get("game_asset_path", "zombonogame", CVAR_LATCH | CVAR_SERVERINFO);

	FILE* json_file_handle = fopen(GAMEINFO_PATH, "r+");

	if (!json_file_handle)
		Sys_Error("Failed to load gameinfo.json!");

	JSON_stream json_stream = { 0 };

	JSON_open_stream(&json_stream, json_file_handle);

	enum JSON_type json_next = JSON_peek(&json_stream);
	const char* json_string;

	// loop until the end of the file
	while (json_next != JSON_DONE)
	{
		switch (json_next)
		{
		case JSON_ERROR:
			Sys_Error("Malformed gameinfo.json! (1): %s", JSON_get_error(&json_stream));
			return;
			// don't parse any arrays or anything like that
		case JSON_OBJECT:
			json_next = JSON_next(&json_stream);

			while (json_next != JSON_OBJECT_END)
			{
				switch (json_next)
				{
				case JSON_ERROR:
					Sys_Error("Malformed gameinfo.json! (2): %s", JSON_get_error(&json_stream));
					return;
				case JSON_STRING:
					json_string = JSON_get_string(&json_stream, NULL);

					if (json_next == JSON_ERROR)
						Sys_Error("Malformed gameinfo.json! (3): %s", JSON_get_error(&json_stream));

					// figure out what we just loaded
					// we have to copy the data we load from the JSON somewhere, because JSON_get_string returns
					// a global pointer held by PDJSON, so when json_get_string is called it will change
					if (!strcmp(json_string, "name")) // name of the game to load, also used for window title
					{	
						json_next = JSON_next(&json_stream);
						// get the value
						json_string = JSON_get_string(&json_stream, NULL);
						
						strncpy(gameinfo.name, json_string, MAX_QPATH);
						game_name->string = gameinfo.name;
					}
					else if (!strcmp(json_string, "asset_path")) // asset path of the game to load
					{
						json_next = JSON_next(&json_stream);
						// get the value
						json_string = JSON_get_string(&json_stream, NULL);

						strncpy(gameinfo.asset_path, json_string, MAX_QPATH);
						game_asset_path->string = gameinfo.asset_path;
					}
					break;
				}

				json_next = JSON_next(&json_stream);
			}

			json_next = JSON_next(&json_stream);

			break;
		}
	}

	Com_Printf("Loading game %s from asset path %s...", game_name->string, game_asset_path->string);

	return;
}