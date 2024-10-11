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
#include <client/client.hpp>

// cl_font.c: 
// Font Engine (February 8, 2024)

// Globals

font_t	fonts[MAX_FONTS] = { 0 };		// The fonts loaded.
cvar_t*	cl_system_font;					// The font used for in-game text.
cvar_t*	cl_console_font;				// The font used for the console.
int32_t num_fonts;						// The number of loaded fonts.
bool	fonts_initialised = false;		// Determines if the font engine is initialised.

// Functions not exposed in headers
// TODO: HANDLE JSON_ERROR IN THESE FUNCTIONS!!!
bool Font_LoadFont(char file_name[MAX_QPATH]);
bool Font_LoadFontConfig(JSON_stream* json_stream, font_t* font_ptr);
bool Font_LoadFontGlyphs(JSON_stream* json_stream, font_t* font_ptr);

#define FONT_LIST_FILE_NAME "fonts\\fonts.txt"

bool Font_Init()
{
	FILE*	font_list_stream;
	char	file_name_font[MAX_QPATH] = { 0 };
	long	file_location = 0;
	char*	token;
	char*	file;

	// we init every time we change vid so we need to reload and set num_fonts to 0
	memset(&fonts, 0x00, sizeof(font_t) * num_fonts);
	num_fonts = 0;

	Com_Printf("Loading fonts...\n");

	// create the system font cvar
	cl_system_font = Cvar_Get("cl_system_font", "bahnschrift_bold_8", 0);
	cl_console_font = Cvar_Get("cl_console_font", "cascadia_code_regular_8", 0);

	// open up fonts.txt

	int32_t file_size = FS_FOpenFile(FONT_LIST_FILE_NAME, &font_list_stream);

	if (font_list_stream == NULL
		|| file_size < 0)
	{
		Sys_Error("Failed to initialise font engine: Couldn't open %s", FONT_LIST_FILE_NAME);
		return false; // shut up compiler
	}

	// get the file length

	fseek(font_list_stream, 0, SEEK_END);

	// msvc does not support VLAs
	file = (char*)Memory_ZoneMallocTagged(file_size, TAG_TEXT_ENGINE);

	assert(file != NULL);

	// return to start and read the file
	fseek(font_list_stream, 0, SEEK_SET);
	
	fread(file, 1, file_size, font_list_stream);
	
	// start at the first byte
	token = file;

	// parse fonts.txt
	while (file_location < file_size)
	{
		file_location = token - file;

		int32_t n = 0; // initialise length variable

		// if its not a comment por newline, parse the filename
		// otherwise go to next line
		if (token[0] != '/'
			&& token[0] != '\n'
			&& token[0] != '\r'
			&& ((file_size - file_location > 1) && token[1] != '/')) // don't overflow if we're on the last byte
		{
			while (token[0] != '\n'
				&& token[0] != '\r') // prevents /r from being added to filenames
			{
				// copy 1 byte at a time (memory alignment to word size?), ignore windows newlines
				memcpy(file_name_font + n, token, 1);
				n++;
				token++;

				if (n > MAX_QPATH - 2) // -2 because of expression below
				{
					file_name_font[MAX_QPATH - 1] = '\0'; // terminate string

					Com_Printf("Tried to load font with path name > 256 chars (not allowed): truncated name %s", file_name_font);
				}
			}

			file_name_font[n] = '\0'; // terminate string
			
			Font_LoadFont(file_name_font);

			// n re-initialised above
		}
		else // go to next line
		{
			if (file_location < file_size)
			{
				while (token[0] != '\n') token++;
				token++; // go past the newline
				file_location += 2; 
			}

		}
		// token at this point is the character after the newline

	}

	// close it
	fclose(font_list_stream); 
	Memory_ZoneFree(file);

	// check if the system font (font used by in game text) was loaded properly.
	// Sys_Error if it failed

	if (!Font_GetByName(cl_system_font->string))
	{
		Sys_Error("Failed to load system font %s", cl_system_font->string);
		return false; 
	}

	if (!Font_GetByName(cl_console_font->string))
	{
		Sys_Error("Failed to load console font %s", cl_console_font->string);
		return false;
	}

	Com_Printf("Successfully loaded %d fonts!\n", num_fonts);

	fonts_initialised = true;
	return true; 
}

bool Font_LoadFont(char file_name[MAX_QPATH])
{
	Com_DPrintf("Loading font %s\n", file_name);

	// allocate a JSON stream, FILE handle and the font object
	FILE*			json_handle;
	JSON_stream		json_stream;
	font_t*			font = &fonts[num_fonts];

	//+4 for extension
	char tga_filename[MAX_QPATH+4] = {0};
	char json_filename[MAX_QPATH+4] = {0};

	// open up json, load targa as a texture.
	// .tga is *assumed* by LoadPic!!
	snprintf(tga_filename, MAX_QPATH + 4, "fonts/%s", file_name);
	snprintf(json_filename, MAX_QPATH + 4, "fonts/%s.json", file_name);

	Com_DPrintf("Font_LoadFont: Loading Font TGA %s.tga\n", tga_filename);
	// TODO: MERGE PICS AND IMAGES!!!
	re.LoadPic(tga_filename);

	Com_DPrintf("Font_LoadFont: Loading Font JSON %s\n", json_filename);

	if (FS_FOpenFile(json_filename, &json_handle) == -1)
	{
		Sys_Error("Failed to load Font JSON %s!", json_filename);
		return false;
	}

	JSON_open_stream(&json_stream, json_handle);

	enum JSON_type next_type = JSON_peek(&json_stream);

	// start the real parsing
	// two JSON_DONES means EOF
	int32_t 			done_count = 0;
	bool				running = true; 
	const char*			json_string = { 0 };
	font_json_section	json_section_current = font_json_config;

	while (running)
	{	
		if (next_type != JSON_DONE) done_count = 0;

		// check the type
		switch (next_type)
		{
		case JSON_OBJECT:
		case JSON_ARRAY:
			//JSON_next(json_stream);
			json_string = JSON_get_string(&json_stream, NULL);

			if (!strcmp(json_string, "config"))
			{
				json_section_current = font_json_config;

				if (!Font_LoadFontConfig(&json_stream, font))
				{
					Sys_Error("Failed to load font configuration for font %s: %s (line %d, column %d)",
						&json_filename, JSON_get_error(&json_stream), JSON_get_lineno(&json_stream), JSON_get_position(&json_stream));
				}
			}
			else if (!strcmp(json_string, "kerning"))
			{
				json_section_current = font_json_kerning;
				//not implemented for now
			}
			else if (!strcmp(json_string, "symbols"))
			{
				json_section_current = font_json_symbols;

				if (!Font_LoadFontGlyphs(&json_stream, font))
				{
					Sys_Error("Failed to load font glyph information for font %s: %s (line %d, column %d)",
						&json_filename, JSON_get_error(&json_stream), JSON_get_lineno(&json_stream), JSON_get_position(&json_stream));
				}
			}
			break;
		case JSON_OBJECT_END:
		case JSON_ARRAY_END:
			JSON_next(&json_stream); // just skip????
			break;
		case JSON_ERROR:
			Sys_Error("Invalid Font JSON %s: %s (Line %d, column %d)!", json_filename, 
				JSON_get_error(&json_stream), JSON_get_lineno(&json_stream), JSON_get_position(&json_stream));
			return false; 
		case JSON_DONE:
			done_count++;
			// two JSON_DONEs returned if it's the end of the file
			if (done_count >= 2)
			{
				running = false;
			}
			else
			{
				JSON_reset(&json_stream);
			}
			break;
		}

		next_type = JSON_next(&json_stream);
	}

	// we're done, close up
	JSON_close(&json_stream);

	fclose(json_handle);

	// make sure we didn't hit the font limit

	if (num_fonts >= MAX_FONTS-1) // -1 is for the index
	{
		Com_Printf("[BUG] Tried to load too many fonts (the limit is %d). Not loading any more.", MAX_FONTS);
		return true;
	}

	// set the font name to what fonts.lst indicated
	strncpy(font->name, file_name, MAX_QPATH);

	// increment the number of fonts
	num_fonts++;
	return true; 
}

bool Font_LoadFontConfig(JSON_stream* json_stream, font_t* font_ptr)
{
	enum JSON_type next_type = JSON_peek(json_stream);
	const char* json_string = { 0 };
	double		json_number = 0.0;

	while (next_type != JSON_OBJECT_END)
	{
		switch (next_type)
		{
		case JSON_STRING:
			json_string = JSON_get_string(json_stream, NULL);

			if (next_type == JSON_ERROR) return false;

			// we don't need to load most of these
			// as we make the following assumptions:
			//
			// - texture is 256*256
			// - difference between bold, italic, et cetera will be stored in the glyph information
			// - not a monospace font, and x and y information is stored per-glyph, so we don't need to care about spacing (charHeight is used for newlines)
			if (!strcmp(json_string, "size"))
			{	
				next_type = JSON_next(json_stream);
				font_ptr->size = (int32_t)JSON_get_number(json_stream);
			}
			else if (!strcmp(json_string, "charHeight"))
			{
				next_type = JSON_next(json_stream);
				font_ptr->line_height = (int32_t)JSON_get_number(json_stream);
			}
			else if (!strcmp(json_string, "face"))
			{
				next_type = JSON_next(json_stream);
				strncpy(font_ptr->name, json_string, MAX_QPATH);
			}

			// go to the next one

			break;
		case JSON_ERROR:
			return false;
		// will not be the end of the file as it will be preceded by the end of object marker, so don't bother checking for two here
		case JSON_DONE:
			JSON_reset(json_stream);
			break;
		}

		next_type = JSON_next(json_stream);
	}

	return true; 
}

bool Font_LoadFontGlyphs(JSON_stream* json_stream, font_t* font_ptr)
{
	// for some reason the first object in the array is not returned by JSON_next
	// todo: possibly PD-Json bug?
	enum JSON_type next_type = JSON_OBJECT;
	const char* json_string = { 0 };
	double		json_number = 0.0;
	glyph_t*	current_glyph;

	while (next_type != JSON_ARRAY_END)
	{
		switch (next_type)
		{
		case JSON_OBJECT:
			
			current_glyph = &font_ptr->glyphs[font_ptr->num_glyphs];

			// load an individual glyph
			// for this JSON section, all parsing must be within an object
			while (next_type != JSON_OBJECT_END)
			{
				switch (next_type)
				{
				case JSON_NUMBER:
					json_number = (int32_t)JSON_get_number(json_stream);
					break;
				case JSON_STRING:
					json_string = JSON_get_string(json_stream, NULL);

					// we don't bother parsing the width as we use advance (again, not monospaced)
					if (!strcmp(json_string, "height"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->height = (int32_t)JSON_get_number(json_stream);
					}
					else if (!strcmp(json_string, "width"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->width = (int32_t)JSON_get_number(json_stream);
					}
					else if (!strcmp(json_string, "id"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->char_code = (int32_t)JSON_get_number(json_stream);
					}
					else if (!strcmp(json_string, "x"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->x_start = (int32_t)JSON_get_number(json_stream);
					}
					else if (!strcmp(json_string, "xadvance"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->x_advance = (int32_t)JSON_get_number(json_stream);
					}
					else if (!strcmp(json_string, "xoffset"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->x_offset = (int32_t)JSON_get_number(json_stream);
					}
					else if (!strcmp(json_string, "y"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->y_start = (int32_t)JSON_get_number(json_stream);
					}
					else if (!strcmp(json_string, "yadvance"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->y_advance = (int32_t)JSON_get_number(json_stream);
					}
					else if (!strcmp(json_string, "yoffset"))
					{
						next_type = JSON_next(json_stream);
						current_glyph->y_offset = (int32_t)JSON_get_number(json_stream);
					}
					break;
				case JSON_ERROR:
					return false;
				
				}

				next_type = JSON_next(json_stream);
			}

			if (font_ptr->num_glyphs >= MAX_GLYPHS-1) // -1 for index
			{
				Com_Printf("More than %d glyphs in font %s. Not loading any more glyphs", MAX_GLYPHS, font_ptr->name);
				return false;
			}

			font_ptr->num_glyphs++;
			break;
		case JSON_ERROR:
			return false;
			// will not be the end of the file as the two JSON_DONEs indicating EOF will always be preceded by the end of object marker, so don't bother checking for two here
		case JSON_DONE:
			JSON_reset(json_stream);
			break;
		}

		next_type = JSON_next(json_stream);
	}

	Com_DPrintf("Loaded %d glyphs\n", font_ptr->num_glyphs);
	return true;
}

font_t* Font_GetByName(const char* name)
{
	for (int32_t font_num = 0; font_num < num_fonts; font_num++)
	{
		font_t* fnt_ptr = &fonts[font_num];

		if (!strncmp(fnt_ptr->name, name, MAX_QPATH))
		{
			return fnt_ptr;
		}
	}

	// error condition handled by calling function
	return NULL;
}

glyph_t* Glyph_GetByChar(font_t* font, char glyph)
{
	if (font == NULL)
	{
		// considered a crashing issue
		Sys_Error("NULL font pointer passed to Glyph_GetByChar");
		return NULL;
	}

	for (int32_t glyph_num = 0; glyph_num < font->num_glyphs; glyph_num++)
	{
		glyph_t* candidate_glyph = &font->glyphs[glyph_num];

		if (candidate_glyph->char_code == glyph)
		{
			return candidate_glyph;
		}
	}

	// error condition handled by calling function (usually Text_Draw)
	return NULL;
}
