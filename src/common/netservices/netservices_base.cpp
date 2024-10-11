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
// netservices_base.c: Netservices base and connection test

#include "netservices.hpp"

// Set to true in Netservices_Init()
bool netservices_connected = false;					// Determines if netservices is initialised and you are connected to the internet

// String downloaded from the below url to 
const char* connect_test_string = "This is a connect test file for Zombono Network Services";
const char* connect_test_url = SERVICE_BASE_URL_UPDATER "/connecttest.txt"; // Just use updater service for thi
char netservices_connect_test_buffer[CURL_MAX_WRITE_SIZE];			// Buffer to use for receiving data from curl
	
char connect_test_error_buffer[CURL_ERROR_SIZE];			// Error string buffer returned by CURL functions

//TODO: EXTREMELY NON-REENTRANT BUT WE DO NOT HAVE MULTITHREADING ANYWAY

cvar_t* ns_disabled;										// If true, don't ever contact zombono.com
cvar_t* ns_nointernetcheck;									// If true, don't perform an internet check
cvar_t* ns_noupdatecheck;									// If true, don't perform an update check
#ifndef RELEASE
cvar_t* ns_usetestserver;									// Use test serve 
#endif

CURL*	curl_obj_connect_test;								// The single blocking transfer curl interface object used for testing connections
CURLM*	curl_obj;											// The multi nonblocking transfer curl interface object

int32_t	netservices_running_transfers;						// The number of curl transfers currently running.

void	(*netservices_on_complete_callback)();	// The callback to use when the current transfer is complete.

// functions only used within this file
size_t Netservices_Init_WriteCallback(char *ptr, size_t size, size_t nmemb, char* received_data);				// Callback function on CURL receive

// This function runs before SV_Init and CL_Init, so take that into account
bool Netservices_Init()
{
	ns_nointernetcheck = Cvar_Get("ns_nointernetcheck", "0", CVAR_ARCHIVE);
	ns_noupdatecheck = Cvar_Get("ns_noupdatecheck", "0", CVAR_ARCHIVE);
	ns_disabled = Cvar_Get("ns_disabled", "0", CVAR_ARCHIVE);
#ifndef RELEASE
	ns_usetestserver = Cvar_Get("ns_usetestserver", "0", CVAR_ARCHIVE);
#endif

	if (ns_nointernetcheck->value
		|| ns_disabled->value)
	{
		// assume we are connected unless ns_disabled is set
		if (ns_disabled->value)
		{
			return false;
		}

		return true;
	}

	// initialise cURL
	curl_global_init(CURL_GLOBAL_ALL);
	
	Com_Printf("Netservices_Init: Determining if you are connected to the Internet...\n");

	// init both curl objects

	curl_obj_connect_test = Netservices_AddCurlObject(connect_test_url, false, http_method_get, Netservices_Init_WriteCallback, NULL);
	curl_obj = curl_multi_init();

	if (!curl_obj_connect_test)
	{
		Sys_Msgbox("Warning", 0, "CURL failed to initialise. Updating, master servers, and accounts won't be available.\nYou can still play the game.");
		return false;
	}

	// get the file - this should take a max of 2 seconds because it blocks
	// I don't want the user to try and use netservices things before the connection test finishes
	CURLcode error_code = curl_easy_perform(curl_obj_connect_test);

	if (error_code != CURLE_OK)
	{
		Com_Printf("Internet connection check failed. Disabling netservices... (%s)\n", connect_test_error_buffer);
		return false; 
	}

	// test if teh string was correct (if it isn't it should be a bug)
	if (strncmp(netservices_connect_test_buffer, connect_test_string, strlen(connect_test_string)))
	{
		Com_Printf("Received invalid connect test string from updates.zombono.com (%s, not %s) (Probably a bug)\n", netservices_connect_test_buffer, connect_test_string);
		return false;
	}
	
	// destroy the connection test object

	Netservices_DestroyCurlObject(curl_obj_connect_test, false);

	Com_Printf("Netservices_Init: Connected to netservices successfully.\n");
	netservices_connected = true;
	return true;
}

#define MAX_URL_QUERY_STRING_LENGTH	0x1000

CURL* Netservices_AddCurlObject(const char* url, bool multi, http_method http_method, size_t write_callback(char* ptr, size_t size, size_t nmemb, char* userdata), const char* query_string)
{
	CURL* new_obj = curl_easy_init();

	// Wow, a decent use for Goto! 
	// It's so we don't have to duplicate curl_easy_cleanup...

	// 40 byte text file should take maximum of 2 seconds to download to minimise user wait time
	if (curl_easy_setopt(new_obj, CURLOPT_TIMEOUT_MS, 2000))
		goto on_fail;

	if (curl_easy_setopt(new_obj, CURLOPT_URL, url))
		goto on_fail;

	if (curl_easy_setopt(new_obj, CURLOPT_WRITEFUNCTION, write_callback))
		goto on_fail;

	if (curl_easy_setopt(new_obj, CURLOPT_USERAGENT, ENGINE_USER_AGENT))
		goto on_fail;

	if (curl_easy_setopt(new_obj, CURLOPT_ERRORBUFFER, &connect_test_error_buffer))
		goto on_fail;

	// If the user provided a query string, check the specified HTTP method
	// If it's a get snprintf into the query string (this is a weird libcurl stupidity)
	// If it's a post, use CURLOPT_POSTFIELDS
	if (query_string)
	{
		switch (http_method)
		{
		case http_method_get:
		{
				//only allocate if we need tp
			char full_url_buf[MAX_URL_QUERY_STRING_LENGTH] = { 0 };

			if (strlen(url) + strlen(query_string) >= MAX_URL_QUERY_STRING_LENGTH)
			{
				Com_Printf("Netservices_AddCurlObject: strlen(url) + strlen(query_string) >= MAX_URL_QUERY_STRING_LENGTH!");
				goto on_fail;
			}

			snprintf(full_url_buf, MAX_URL_QUERY_STRING_LENGTH, "%s%s", url, query_string);
			// set the url to the full url
			if (curl_easy_setopt(new_obj, CURLOPT_URL, full_url_buf))
				goto on_fail;

			break;
		}
		// set POST options
		case http_method_post:
			if (curl_easy_setopt(new_obj, CURLOPT_POST, true))
				goto on_fail;

			if (curl_easy_setopt(new_obj, CURLOPT_POSTFIELDS, query_string))
				goto on_fail;

			break;
		}

	}

	if (multi)
	{
		if (curl_multi_add_handle(curl_obj, new_obj))
			goto on_fail;
	}

	goto on_success;

on_fail:
	curl_easy_cleanup(curl_obj);
	return NULL;

on_success:
	return new_obj;
}

void Netservices_DestroyCurlObject(CURL* object, bool multi)
{
	if (multi)
		curl_multi_remove_handle(curl_obj, object);

	// this is a function as we intend to do more stuff here later
	curl_easy_cleanup(object);
}

void Netservices_SetOnCompleteCallback(void on_complete())
{
	netservices_on_complete_callback = on_complete;
}

void Netservices_StartPendingTransfers()
{
	// perform one perform to get things going
	CURLMcode error_code = curl_multi_perform(curl_obj, &netservices_running_transfers);

	if (error_code)
		Com_Printf("Initial curl_multi_perform failed %d", error_code);
}

// Checks for a game update and polls
void Netservices_Frame()
{
	// See if a game update is available
	if (update_info.update_available
		&& !update_info.dismissed)
	{
		if (Netservices_UpdaterPromptForUpdate())
			Netservices_UpdaterStartUpdate();

		update_info.dismissed = true;
	}

	// if there is nothing to return (NULL means it was never set)
	if (netservices_running_transfers == 0
		|| netservices_on_complete_callback == NULL)
	{
		return;
	}
		
	CURLMcode err_code = curl_multi_perform(curl_obj, &netservices_running_transfers);

	if (err_code != CURLM_OK)
	{
		Com_Printf("curl_multi_perform failed %d (%d running transfers). Stopping transfers...", err_code);
		return;
	}

	if (netservices_running_transfers > 0)
	{
		err_code = curl_multi_poll(curl_obj, NULL, 0, 0, NULL); // we cannot wait as this runs while the game is running as a part of the game loop

		if (err_code)
		{
			Com_Printf("curl_multi_poll failed %d (%d running transfers). Stopping transfers...", err_code);
			return;
		}
	}
	// if we have just finished run the oncomplete function
	else
	{
		netservices_on_complete_callback();
	}
}

size_t Netservices_Init_WriteCallback(char* ptr, size_t size, size_t nmemb, char* userdata)
{
	if (nmemb >= CURL_MAX_WRITE_SIZE)
	{
		Com_Printf("Netservices_Init_WriteCallback: nmemb (%d) > CURL_MAX_WRITE_SIZE (%d)", nmemb, CURL_MAX_WRITE_SIZE);
 		return nmemb;
	}

	strncpy(netservices_connect_test_buffer, ptr, nmemb);
	netservices_connect_test_buffer[nmemb] = '\0'; // null terminate string (curl does not do that by default)

	return nmemb;
}

void Netservices_Shutdown()
{
	Netservices_DestroyCurlObject(curl_obj_connect_test, false);
	curl_multi_cleanup(curl_obj);

	// uninitialise curl
	curl_global_cleanup();
}