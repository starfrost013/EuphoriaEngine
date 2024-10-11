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
// netservices_update.c: Updater

#include "netservices.hpp"

// Defines
#define UPDATE_JSON_URL SERVICE_BASE_URL_UPDATER		"/updateinfo.json"	// URL for update info json file
#define UPDATE_BINARY_BASE_URL SERVICE_BASE_URL_UPDATER "/updates"			// base URL for update binary
#define UPDATE_PROMPT_STR_LENGTH				2048				// Length of the update prompt string
#define DOWNLOAD_URL_STR_LENGTH					256					// Length of the download URL string
#define DOWNLOAD_CMD_STR_LENGTH					512					// Length of the command to run
#ifdef _WIN32
#define DOWNLOAD_URL_FORMAT						"Zombono-v%d.%d.%d.%d-%s-%s.exe"	// Format for the update package binary.
#else
#define DOWNLOAD_URL_FORMAT						"Zombono-v%d.%d.%d.%d-%s-%s"	// Format for the update package binary.
#endif

// Functions only used in this file
size_t	Netservices_UpdateInfoJsonReceive(char* ptr, size_t size, size_t nmemb, char* userdata);		// Callback function for when updateinfo.json stuff is received.
void	Netservices_UpdateInfoJsonComplete();															// Callback function for when updateinfo.json stuff is completed.
__declspec(noreturn) void Netservices_UpdaterUpdateGame();											// Actually perform the game update

size_t	Netservices_UpdateInfoBinaryReceive(char* ptr, size_t size, size_t nmemb, char* userdata);		// Callback function for when binary stuff is received.
void	Netservices_UpdateInfoBinaryComplete();															// Callback function for when binary stuff is received.

// Globals
// 
// Set the update channel and a string for it based on the build configuration
#if DEBUG
game_update_channel current_update_channel = update_channel_debug;
const char* selected_update_channel_str = "debug";
#elif PLAYTEST
game_update_channel current_update_channel = update_channel_playtest;
const char* selected_update_channel_str = "playtest";
#else
game_update_channel current_update_channel = update_channel_release;
const char* selected_update_channel_str = "release";
#endif

CURL*	update_json_curl_obj = { 0 };
CURL*	update_binary_curl_obj = { 0 };			// see how fast this is and if we need multiple simultaneous connections (curl_multitransfer_t)

// tmpfile() handle
FILE*	update_json_handle;						// updateinfo.json file handle
char	update_json_file_name[L_tmpnam];		// updateinfo.json temp file name

FILE*	update_binary_handle;					// Update binary file handle
char	update_binary_file_name[DOWNLOAD_URL_STR_LENGTH] = { 0 };	// Update binary file name after downloaded

char*	update_json_file_name_ptr = update_json_file_name;
char*	update_binary_file_name_ptr = update_binary_file_name;

// stores available update information
// if update_available is true, it will ask the user to update
game_update_t	update_info = { 0 };

// Does not block
void Netservices_UpdaterGetUpdate()
{
	// don't try and update if updating or netservices is disabled
	if (ns_noupdatecheck->value
		|| ns_disabled->value)
			return;

	// initialise the updateinfo structs
	memset(&update_info, 0x00, sizeof(game_update_t));

	Com_Printf("Checking for updates...\n");

	update_json_curl_obj = Netservices_AddCurlObject(UPDATE_JSON_URL, true, http_method_get, Netservices_UpdateInfoJsonReceive, NULL);

	// create a temporary file
	tmpnam(update_json_file_name_ptr);
	
	// tell the user about this but not stop them from playing the game
	if (update_json_file_name[0] == '\0')
	{
		Sys_Msgbox("Non-Fatal Error", 0, "Failed to create Netservices UpdateInfo.json - couldn't get a temp file name!");
		return;
	}

	// open it
	update_json_handle = fopen(update_json_file_name, "w");

	if (update_json_handle == NULL)
	{
		Sys_Msgbox("Non-Fatal Error", 0, "Failed to create Netservices UpdateInfo.json - couldn't create a temp file!");
		return;
	}

	// tell netservices to call the "is an update available?" function
	Netservices_SetOnCompleteCallback(Netservices_UpdateInfoJsonComplete);
	
	// and start the transfer...
	Netservices_StartPendingTransfers();
}

size_t Netservices_UpdateInfoJsonReceive(char* ptr, size_t size, size_t nmemb, char* userdata)
{
	// write it
	// we close it in the complete callback because this can be called many times (only 16kb is transferred at a time in CURL)
	fwrite(ptr, nmemb, size, update_json_handle);
	return nmemb;
}

// sets update_json.update_available to true
void Netservices_UpdateInfoJsonComplete()
{
	JSON_stream update_json_stream;

	Com_DPrintf("Downloaded update information to tmpfile %s\n", update_json_file_name);

	// close and reopen the file as r+ for stupid msvc tmpfile() related reasons
	fclose(update_json_handle);
	update_json_handle = fopen(update_json_file_name_ptr, "r+");

	// open the file as a json stream
	JSON_open_stream(&update_json_stream, update_json_handle);

	// set channel to current
	update_info.channel = current_update_channel;

	// empty string - invalid channel (failsafe)
	if (strlen(selected_update_channel_str) == 0)
	{
		Sys_Msgbox("Update Error", 0, "Invalid update channel selected. Can't update...");
		fclose(update_json_handle);
		remove(update_json_file_name_ptr);
		return;
	}

	// are we parsing the selected update channel?
	bool current_update_channel_selected = false;

	// parse the json stream
	// A PARSE FAILURE IS A SYS_ERROR CONDITION

	enum JSON_type next_object = JSON_next(&update_json_stream);

	const char* json_string;

	while (next_object != JSON_DONE)
	{
		switch (next_object)
		{
			// there is only one array in this json file (it's an array of update channels),
			// so we can use regular JSON_OBJECT objects and ignore the array objects to reduce code complexity
			case JSON_OBJECT:
				
				// iterate through array
				while (next_object != JSON_OBJECT_END)
				{	
					switch (next_object)
					{
						case JSON_STRING:
							json_string = JSON_get_string(&update_json_stream, NULL);

							// update channel name - check that the current channel is selected
							if (!strcmp(json_string, "name"))
							{
								next_object = JSON_next(&update_json_stream);
								const char* current_update_channel_string = JSON_get_string(&update_json_stream, NULL);

								bool is_current_update_channel = !strcmp(current_update_channel_string, selected_update_channel_str);

								if (is_current_update_channel)
								{
									current_update_channel_selected = true;
								}
								else if (!is_current_update_channel
									&& current_update_channel_selected)
								{
									current_update_channel_selected = false;
								}
							}
							// Version information - only parsed if current channel is selected
							else if (!strcmp(json_string, "version")
								&& current_update_channel_selected)
							{
								next_object = JSON_next(&update_json_stream);
								const char* version_string = JSON_get_string(&update_json_stream, NULL);

								// parse the version information
								char* token = strtok((char*)version_string, "."); // NEVER DO THIS BUT THIS CODE IS OBSOLETE

								if (token == NULL)
									Sys_Error("Malformed version information in obtained UpdateInfo.json (1) (THIS IS A BUG)"); 
								update_info.version.major = atoi(token);

								token = strtok(NULL, ".");
								if (token == NULL)
									Sys_Error("Malformed version information in obtained UpdateInfo.json (2) (THIS IS A BUG)");
								update_info.version.minor = atoi(token);

								token = strtok(NULL, ".");
								if (token == NULL)
									Sys_Error("Malformed version information in obtained UpdateInfo.json (3) (THIS IS A BUG)");
								update_info.version.revision = atoi(token);

								token = strtok(NULL, ".");
								if (token == NULL)
									Sys_Error("Malformed version information in obtained UpdateInfo.json (4) (THIS IS A BUG)");
								update_info.version.build = atoi(token);


							}
							// Release date: ISO 8601 format date string for version reelase date
							else if (!strcmp(json_string, "release_date"))
							{
								next_object = JSON_next(&update_json_stream);
								const char* release_date_string = JSON_get_string(&update_json_stream, NULL);

								// parse date format
								int32_t year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

								if (sscanf(release_date_string, "%04d-%04d-%04d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second))
								{
									// parse it
									update_info.release_date.tm_year = year;
									update_info.release_date.tm_mon = month;
									update_info.release_date.tm_mday = day;
									update_info.release_date.tm_hour = hour;
									update_info.release_date.tm_min = minute;
									update_info.release_date.tm_sec = second;
								}
								else // kill it
									Sys_Error("Malformed release date information in obtained UpdateInfo.json (THIS IS A BUG)");
							}
							// A description of the update
							else if (!strcmp(json_string, "description"))
							{
								next_object = JSON_next(&update_json_stream);
								const char* description_string = JSON_get_string(&update_json_stream, NULL);

								strncpy(update_info.description, description_string, MAX_UPDATE_STR_LENGTH);
							}
							break;
					}

					next_object = JSON_next(&update_json_stream);
				}

				break;
		}

		next_object = JSON_next(&update_json_stream);
	}

	// If we got here, we assume the update was correctly parsed.
	// Run a check on the version number.

	// Only build has to be explicitly higher than the current version. Major, minor or revision can be larger
	if ((update_info.version.major >= ENGINE_VERSION_MAJOR
		|| (update_info.version.minor >= ENGINE_VERSION_MINOR)
		|| (update_info.version.revision >= ENGINE_VERSION_REVISION))
		&& update_info.version.build > ENGINE_VERSION_BUILD)
	{
		// make sure the update has been released yet,
		// this gives us time for testing jic we fuck it up and allows us to rollout at specific times
		// just normalise to time_t

		time_t new_version_time = mktime(&update_info.release_date);

		time_t current_time = time(NULL);

		// if the update is actually released a new version is available
		if (current_time > new_version_time)
		{
			Com_DPrintf("Update available for current build configuration: new version %d.%d.%d.%d, description: %s!\n", 
				update_info.version.major, update_info.version.minor, update_info.version.revision, update_info.version.build, update_info.description);

			update_info.update_available = true;
		}
		
	}

	// in any case we need to delete the tempfile so close it first
	JSON_close(&update_json_stream);
	fclose(update_json_handle);
	remove(update_json_file_name_ptr);

	// destroy the curl object

	Netservices_DestroyCurlObject(update_json_curl_obj, true);
}

// The update prompt text.
const char* update_prompt_format =
"An update is available for %s:\n"
"\n"
"Version %d.%d.%d.%d is now available.\n"
"Description: %s\n"
"\n"
"Would you like to update?\n"
"\n"
"(This UI is temporary and will be improved in the future.)";

// Prompts for an update. Returns true if the user wanted to update
bool Netservices_UpdaterPromptForUpdate()
{
	// Temporary UI
	char update_prompt[UPDATE_PROMPT_STR_LENGTH] = { 0 };

	snprintf(update_prompt, UPDATE_PROMPT_STR_LENGTH, gameinfo.name, update_prompt_format,
		update_info.version.major, update_info.version.minor, update_info.version.revision, update_info.version.build,
		update_info.description);

	// cannot use constants, they are windows specific and this is platform independent code
	// TODO: convert to enum...
	int32_t buttons = Sys_Msgbox("Update Available", 4, update_prompt); // 4 = MB_YESNO

	return (buttons == 6);
}

char update_binary_path[DOWNLOAD_URL_STR_LENGTH] = { 0 };

// Starts the update process. We can still get out at this point
void Netservices_UpdaterStartUpdate()
{ 
	char* update_binary_path_ptr = update_binary_path;

	// generate a build url
	snprintf(update_binary_path_ptr, DOWNLOAD_URL_STR_LENGTH, UPDATE_BINARY_BASE_URL "/" DOWNLOAD_URL_FORMAT,
		update_info.version.major, update_info.version.minor, update_info.version.revision, update_info.version.build,
		BUILD_PLATFORM, selected_update_channel_str);

	// generate the local filename to be opened when we're done
	snprintf(update_binary_path_ptr, DOWNLOAD_URL_STR_LENGTH, DOWNLOAD_URL_FORMAT,
		update_info.version.major, update_info.version.minor, update_info.version.revision, update_info.version.build,
		BUILD_PLATFORM, selected_update_channel_str);

	Com_Printf("Downloading update package %s...\n", update_binary_path);

	// cannot be called if noupdatecheck is not set so dont bother
	update_binary_curl_obj = Netservices_AddCurlObject(update_binary_path, true, http_method_get, Netservices_UpdateInfoBinaryReceive, NULL);

	// override timeout because it's a large file
	curl_easy_setopt(update_binary_curl_obj, CURLOPT_TIMEOUT, 120000);
	
	// remove it if it already exists (we don't care about the result)
	remove(update_binary_path_ptr);

	update_binary_handle = fopen(update_binary_path_ptr, "wb");

	if (!update_binary_handle)
	{
		Sys_Msgbox("Update failed", 0, "Failed to update Zombono. Could not create update binary file!");
		return;
	}

	Netservices_SetOnCompleteCallback(Netservices_UpdateInfoBinaryComplete);
	Netservices_StartPendingTransfers();
}

size_t Netservices_UpdateInfoBinaryReceive(char* ptr, size_t size, size_t nmemb, char* userdata)
{
	// write to the update bvinary
	fwrite(ptr, nmemb, size, update_binary_handle);
	return nmemb;
}

void Netservices_UpdateInfoBinaryComplete()
{
	Com_Printf("Update package downloaded!\n");
	fclose(update_binary_handle);
	
	Netservices_UpdaterUpdateGame();
}

// Does not return so mark ti as such for the compiler
#ifdef _MSC_VER
__declspec(noreturn) void Netservices_UpdaterUpdateGame()
#else // gcc
__attribute((noreturn)) void Netservices_UpdaterUpdateGame()
#endif
{
	// TODO: this is incredibly retarded and I am an idiot
	// I was too lazy to do anything else
	char update_exec_parameters[RESTART_MAX_CMD_LINE] = { 0 };
	char* update_exec_parameters_ptr = update_exec_parameters;

#if _WIN32
	snprintf(update_exec_parameters_ptr, RESTART_MAX_CMD_LINE, "taskkill -f -im Zombono.exe >nul && %s -o\".\" -y && del %s && Zombono & exit",
		update_binary_file_name, update_binary_file_name);
#else
	snprintf(update_exec_parameters_ptr, RESTART_MAX_CMD_LINE, "kill -9 Zombono && %s -o\".\" -y && rm %s && Zombono",
		update_binary_file_name, update_binary_file_name);
#endif

	// unhook the console (otherwise it won't close)
	if (dedicated && dedicated->value)
		FreeConsole();
	else if (debug_console->value)
		FreeConsole();

	// run the command
	system(update_exec_parameters_ptr);
}