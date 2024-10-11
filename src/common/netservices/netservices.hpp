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
// netservices.h: Accounts, updating, master servers
// March 23, 2024

#pragma once
//CURL_STATICLIB MUST BE DEFINED ON COMMAND LINE OR JHERE
#include <common/common.hpp>
#include <common/curl/curl.h>

// Cvars
// 
// All netservices cvars start with ns_* 
extern cvar_t* ns_nointernetcheck;							// If true, no internet check will be performed.
extern cvar_t* ns_noupdatecheck;							// If true, no update check will be performed.
extern cvar_t* ns_disabled;									// If true, netservices will be entirely disabled. Zombono.com will not be contacted at all. Essentially acts as if Netservices_Init failed.

// cvars
#ifndef RELEASE
extern cvar_t* ns_usetestserver;
#endif

// base urls

#define GAME_WEBSITE_URL			"https://zombono.com"			// Game URL
#define GAME_WEBSITE_URL_STAGING	"https://staging.zombono.com"	// Staging URL for test servers

#define SERVICE_BASE_URL_UPDATER	"https://updates.zombono.com"	// Base URL for the updater service
#define SERVICE_BASE_URL_API		"https://servers.zombono.com"	// Base URL for the master sercer services

#define MAX_UPDATE_STR_LENGTH		1024							// Maximum length of an update description string

#define	ENGINE_USER_AGENT			"EuphoriaEngine/" ENGINE_VERSION

extern char netservices_connect_test_buffer[CURL_MAX_WRITE_SIZE];	// The data actually received from the connect test.

//
// netservices_base.c
//

// Globals
extern bool		netservices_connected;							// TRUE if you are connected to the internet and can use netservices, FALSE otherwise.
extern CURLM*	curl_obj;										// The curl multi object (used for multiple nonblocking transfers)

// Enumerates HTTP methods
typedef enum http_method_e
{
	http_method_get = 0,

	http_method_post = 1,
} http_method;

// Function
bool Netservices_Init();								// Initialises Netservices and determines if we are connected to the internet.
// Sets up an easy curl object for use with a particular URL and the write callback write_callback (optional query_string)
CURL* Netservices_AddCurlObject(const char* url, bool multi, http_method http_method, size_t write_callback(char* ptr, size_t size, size_t nmemb, char* userdata), const char* query_string);
void Netservices_DestroyCurlObject(CURL* object, bool multi);	// Destroys the easy curl object represented by object and optionally removes it from the multi object.
void Netservices_SetOnCompleteCallback(void on_complete()); // Sets the current on-complete callback to use when performing a nonblocking Netservices transfer.
void Netservices_StartPendingTransfers();					// Starts the current netservices transfers
void Netservices_Frame();								// Checks to see if a curl_multi_obj transfer is complete
void Netservices_Shutdown();							// Shuts down netservices

//
// netservices_update.c
//

typedef enum game_update_channel_e
{
	update_channel_release = 0,								// Release update channel - regular players.

	update_channel_playtest = 1,							// Playtest update channel - invited testers / pre-0.1.0

	update_channel_debug = 2,								// Debug update channel - private tests / devs / custom builds to fix bugs
} game_update_channel;

// Defines a game version.
typedef struct game_version_s
{
	int32_t	major;											// The major version of the game.
	int32_t	minor;											// The minor version of the game.
	int32_t	revision;										// The revision version of the game.
	int32_t	build;											// Should we have this?
} game_version_t;

// Defines a game software update.
typedef struct game_update_s
{
	bool				update_available;					// When Netservices_UpdaterGetUpdate returns an instance of this struct, if this is TRUE, a newer version is available.
	game_update_channel	channel;							// The channel for which the update is available.
	game_version_t		version;							// The version of the game you are updating to.
	struct tm			release_date;						// Release date of the update.
	char				description[MAX_UPDATE_STR_LENGTH];	// A description of the update's features
	bool				dismissed;							// Has the update notification been dismissed?
} game_update_t;

extern game_update_channel	update_current_channel;			// The currently defined channel - corresponds with the engine's build config
extern game_update_t		update_info;					// The most recently obtained update information.

void Netservices_UpdaterGetUpdate();			// Gets an Update. Returns a game_update_t structure containing update information.
bool Netservices_UpdaterPromptForUpdate();		// Prompts for an update. Returns true if the user wanted to update.
void Netservices_UpdaterStartUpdate();			// Starts the update process.

//
// netservices_account.c
// Account system 
//

//
// netservices_masterserver.c
// The Zombono master server protocol
//

#define SERVICE_API_VERSION "1"

// API endpoints
#define SERVICE_BASE_URL_HEARTBEAT SERVICE_BASE_URL_API "/masterserver/v" SERVICE_API_VERSION "/ServerHeartbeat.php"
#define SERVICE_BASE_URL_SERVER_ADD SERVICE_BASE_URL_API "/masterserver/v" SERVICE_API_VERSION "/ServerAdd.php"
#define SERVICE_BASE_URL_SERVER_DELETE SERVICE_BASE_URL_API "/masterserver/v" SERVICE_API_VERSION "/ServerDelete.php"
#define SERVICE_BASE_URL_SERVER_UPDATE SERVICE_BASE_URL_API "/masterserver/v" SERVICE_API_VERSION "/ServerUpdate.php"
#define SERVICE_BASE_URL_SERVER_GET SERVICE_BASE_URL_API "/masterserver/v" SERVICE_API_VERSION "/ServerGet.php"
#define SERVICE_BASE_URL_SERVER_LIST SERVICE_BASE_URL_API "/masterserver/v" SERVICE_API_VERSION "/ServerList.php"

// other defines
#define MAX_MASTER_SERVER_ENTRIES		2048 //hopium

typedef struct master_server_entry_s
{
	uint32_t	ip;
	uint16_t	port;
	// lets hope the upper two bytes never get used here
	uint32_t	ping; 
	uint64_t	id;													// Server ID
	char		map[NETWORK_MAX_MAP_NAME_LENGTH];					// The name of the map currently running on the server.
	char		gamemode[NETWORK_MAX_GAMEMODE_NAME_LENGTH];			// The name of the gamemode currently running on the server
	// TODO: country, etc
} master_server_entry_t;

extern master_server_entry_t master_server_entries[MAX_MASTER_SERVER_ENTRIES];

void Netservices_MasterHeartbeat();									// Sends a heartbeat to the current master server ui.
void Netservices_MasterAddServer(master_server_entry_t entry);		// Adds a server entry to the master server.
void Netservices_MasterDeleteServer();								// Deletes the current server from the master server
void Netservices_MasterGetServerByCriteria();						// Searches the master server
void Netservices_MasterListServers();								// List servers

void Netservices_MasterIDToString(char* str);						// Converts a master ID to a hex filled string in the buffer str. Does this need to be a function?

