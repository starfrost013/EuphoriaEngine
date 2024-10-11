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

// localisation.c
// Text localisation and dictionary system (July 1, 2024)

#include <common/common.hpp>

// cvars
cvar_t* language;

// globals
localisation_entry_t localisation_entries[LOCALISATION_ENTRIES_MAX];
cached_string_t cached_strings[LOCALISATION_ENTRIES_MAX];

uint32_t localisation_entries_count;		// Counts the number of localisation string entries for the current language.
uint32_t cached_strings_count;			// Counts the number of cached localisation strings for the current language.

// Determines if the localisation subsystem has been initialised.
bool localisation_initialised;

// functions only used in this translation unit
void Localisation_LoadCurrentLanguage();

// ran when localisation system initialised
// AND when changing language
void Localisation_Init()
{
	language = Cvar_Get("language", "english", CVAR_ARCHIVE);
	Localisation_LoadCurrentLanguage();
	localisation_initialised = true;
}

#define LINE_BUFFER_LENGTH	LOCALISATION_MAX_LENGTH_KEY + LOCALISATION_MAX_LENGTH_VALUE + 1

// Loads the current localisation language.
void Localisation_LoadCurrentLanguage()
{
	FILE* localisation_lang_ptr;
	char loc_filename[MAX_QPATH] = { 0 };
	char line_buf[LINE_BUFFER_LENGTH] = { 0 }; // +1 for "=" sign
	char* token_ptr;

	localisation_entries_count = 0;

	snprintf(loc_filename, MAX_QPATH, "text/%s/%s", language->string, LOCALISATION_DICTIONARY_FILENAME);

	Com_Printf("Loading localisation information for language %s from file %s...\n", language->string, loc_filename);

	int32_t file_size = FS_FOpenFile(loc_filename, &localisation_lang_ptr);

	if (file_size < 0)
		Sys_Error("Failed to load language strings file %s!", loc_filename);

	// temporarily allocate some storage
	// changed from calloc to (now calloc but with slight overhead) Z_TagMalloc for the purposes of tracking

	uint8_t* file_ptr = (uint8_t*)Memory_ZoneMallocTagged(file_size, TAG_LOCALISATION);

	token_ptr = (char*)file_ptr;

	if (!file_ptr)
	{
		Sys_Error("Failed to allocate memory to read language file ***BUG - OUT OF MEMORY???***");
		return; // shut up compiler
	}

	fread(file_ptr, 1, file_size, localisation_lang_ptr);
	fseek(localisation_lang_ptr, 0, SEEK_SET);

	while ((uint8_t*)token_ptr < (file_ptr + file_size))
	{
		int32_t key_length = 0;
		int32_t value_length = 0;

		// If there is a newline, continue. This is an empty line
		if (*token_ptr == '\r'
			|| *token_ptr == '\n')
		{
			// comment
			if (*token_ptr == '/'
				&& *(token_ptr + 1) == '/')
			{
				goto on_fail;
			}

			token_ptr++;

			continue;
		}

		// find the = sign
		while (*token_ptr != '=')
		{
			if (*token_ptr == '/'
				&& *(token_ptr + 1) == '/')
			{
				// comment so skip
				goto on_fail;
			}

			key_length++;
			token_ptr++;
		}
		
		// in the case of a malformed file, abort processing and move on to the next line

		strncpy(localisation_entries[localisation_entries_count].key, (token_ptr - key_length), key_length);

		// don't include the "=" in the description so just advance by a single byte
		token_ptr++;

		// now copy the value
		while (*token_ptr != '\r'
			&& *token_ptr != '\n')
		{
			value_length++;
			token_ptr++;
		}

		strncpy(localisation_entries[localisation_entries_count].value, (token_ptr - value_length), value_length);

		goto on_success;

	on_fail:
		// In the case the line was not finished, continue until the newline and then go to the end.
		while (*token_ptr != '\r'
			&& *token_ptr != '\n')
		{
			token_ptr++;
		}

		// continue until after the end so the condition at the start of this is satisfied
		while (*token_ptr == '\r'
			|| *token_ptr == '\n')
		{
			token_ptr++;
		}

		continue; 

	on_success:

		// continue until after the end so the condition at the start of this is satisfied
		while (*token_ptr == '\r'
			|| *token_ptr == '\n')
		{
			token_ptr++;
		}

		localisation_entries_count++;
		continue;
	}

	Com_Printf("Loaded %d localisation strings\n", localisation_entries_count);
}

// Returns the value for the key key.
localisation_entry_t* Localisation_GetString(char* key)
{
	// if language has changed, reload localisation files
	if (language->modified)
	{
		Localisation_LoadCurrentLanguage();
		language->modified = false;
	}

	for (int32_t text_string_id = 0; text_string_id < localisation_entries_count; text_string_id++)
	{
		localisation_entry_t* value = &localisation_entries[text_string_id];

		if (!strcmp(value->key, key))
			return value;
	}

	return NULL;
}

// Shamelessly stolen from Lightning
#define LOCALISATION_KEY_START	"["
#define LOCALISATION_KEY_START_CHAR '['
#define LOCALISATION_KEY_END	"]"
#define LOCALISATION_KEY_END_CHAR	']' // needed for some comparisons

// really big buffer for strtok to use
#define STRING_TEMP_BUF_SIZE	0x10000
char string_temp_buf[STRING_TEMP_BUF_SIZE] = { 0 };

// This code sucks
const char* Localisation_ProcessString(const char* value)
{
	// if language has changed, reload localisation files
	if (language->modified)
	{
		Localisation_LoadCurrentLanguage();
		language->modified = false;
	}

	// don't continuously clear the big buffer we use for the localisation string, just clear whatever is left
	uint32_t clear_amount = 0;

	while (clear_amount < STRING_TEMP_BUF_SIZE
		&& string_temp_buf[clear_amount] != 0x00)
	{
		clear_amount++;
	}

	memset(string_temp_buf, 0x00, clear_amount);

	strncpy(string_temp_buf, value, strlen(value));

	size_t localisation_string_length = 0;
	char loc_string_key_buf[LOCALISATION_MAX_LENGTH_KEY] = { 0 };
	size_t string_length = strlen(string_temp_buf);
	size_t string_length_original = string_length;

	// strstr can only find the first use of the character so we can't use that
	// however we still need to start the string temp without breaking further [ charcters,
	// so just check if thw first character is a localisation start character and set the buffer to the start of the string
	// and use first_token below to determine if we need to call strtok or not

	bool first_char_is_key = (string_temp_buf[0] == LOCALISATION_KEY_START_CHAR);

	char* token_ptr = string_temp_buf;

	if (!first_char_is_key)
		token_ptr = strtok(string_temp_buf, LOCALISATION_KEY_START);

	if (!token_ptr)
		return value; // no string here

	if (string_length > STRING_TEMP_BUF_SIZE)
		Sys_Error("Passed string more than 0x10000 bytes in length to Localisation_ProcessString?!");

	// determine the length of the string
	while (token_ptr < (string_temp_buf + string_length))
	{
		char* key_start_ptr = NULL;

		if (first_char_is_key)
		{
			key_start_ptr = strtok(string_temp_buf, LOCALISATION_KEY_START);
		}
		else
		{
			key_start_ptr = strtok(NULL, LOCALISATION_KEY_START);
		}

		// no localisation to do
		if (!key_start_ptr)
			return value;

		// iterate through each part of the string

		int32_t localisation_key_length = 0;

		char* key_end_ptr = strchr(key_start_ptr, LOCALISATION_KEY_END_CHAR);

		localisation_key_length = key_end_ptr - key_start_ptr;

		if (localisation_key_length > LOCALISATION_MAX_LENGTH_KEY)
		{
			Com_Printf("Warning: Localisation string key length of %d, more than %d\n", localisation_key_length, LOCALISATION_MAX_LENGTH_KEY);
			continue;
		}
		else if (localisation_key_length <= 0)
		{
			Com_Printf("Warning: Empty localisation string key, skipping\n");
			continue;
		}

		// get the localisation string
		strncpy(loc_string_key_buf, key_start_ptr, localisation_key_length);

		// was this string already localised?
			// if so, return
		for (uint32_t localisation_string_id = 0; localisation_string_id < localisation_entries_count; localisation_string_id++)
		{
			if (!strcmp(localisation_entries[localisation_string_id].key, loc_string_key_buf))
				return localisation_entries[localisation_string_id].value;
		}

		localisation_entry_t* loc_string = Localisation_GetString(loc_string_key_buf);

		if (!loc_string)
			return value;
		
		localisation_string_length = strlen(loc_string->value);

		// figure out the new length of the string
		string_length = string_length - localisation_key_length + localisation_string_length;

		token_ptr = strtok(NULL, LOCALISATION_KEY_START);

		// done so break
		if (!token_ptr)
			break;
	}

	if (cached_strings_count >= LOCALISATION_ENTRIES_MAX)
	{
		Com_Printf("Too many localisation strings! (%d > %d)", cached_strings_count, LOCALISATION_ENTRIES_MAX);
		return NULL;
	}

	// allocate memory for the string
	// zero it since loading this happens rarely
	
	cached_strings[cached_strings_count].value = (char*)Memory_ZoneMallocTagged(string_length, TAG_LOCALISATION);

	if (!cached_strings[cached_strings_count].value)
	{
		Sys_Error("Failed to allocate memory for localisation string information");
		return NULL;
	}

	// reset pointer
	// this code might suck
	token_ptr = strtok(string_temp_buf, LOCALISATION_KEY_START);

	// if we got to this point we need to un-terminate the string as modified by strtok
	// so do that
	while (token_ptr < (string_temp_buf + string_length_original))
	{
		if (*token_ptr == '\0')
			*token_ptr = LOCALISATION_KEY_START_CHAR;

		token_ptr++;
	}

	token_ptr = string_temp_buf;

	// actually copy it
	while (token_ptr < (string_temp_buf + string_length_original))
	{
		// get the part actually after the LOCALISATION_KEY_START string
		char* key_start_ptr = strtok(NULL, LOCALISATION_KEY_START);
		char* key_end_ptr = key_start_ptr;

		// copy part before the localisation string
		strncpy(cached_strings[cached_strings_count].value, token_ptr, (key_start_ptr - token_ptr) - 1); // -1 to cut off the [

		// iterate through each part of the string

		int32_t localisation_key_length = 0;

		key_end_ptr = strchr(key_start_ptr, LOCALISATION_KEY_END_CHAR);

		localisation_key_length = key_end_ptr - key_start_ptr;

		// get the localisation string
		strncpy(loc_string_key_buf, key_start_ptr, localisation_key_length);

		localisation_entry_t* loc_string = Localisation_GetString(loc_string_key_buf);

		if (!loc_string)
			return value;

		cached_strings[cached_strings_count].key = loc_string->key;

		uint32_t loc_string_current_length = strlen(cached_strings[cached_strings_count].value);

		// copy the localisation string
		strncpy(cached_strings[cached_strings_count].value + loc_string_current_length, loc_string->value, strlen(loc_string->value));

		token_ptr = strtok(NULL, LOCALISATION_KEY_START);

		// we are done
		if (!token_ptr)
			break;
	}

	// we are done
	cached_strings_count++;
	return cached_strings[cached_strings_count - 1].value;
}

// Frees all localisation strins
void Localisation_Shutdown()
{
	for (int32_t localisation_string_id = 0; localisation_string_id < localisation_entries_count; localisation_string_id++)
	{
		if (cached_strings[localisation_string_id].value != NULL)
			Memory_ZoneFree((void*)cached_strings[localisation_string_id].value);
	}
}