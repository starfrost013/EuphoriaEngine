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

#define NUM_COLOR_CODES			16
#define TAB_SIZE_CHARS			8 // make a cvar?

// The list of valid colour codes.
// This is based on Quake 3 (which is also used by COD games as late as Black Ops III), but additional colours are added while still remaining compatible.
// Some of these are from EGA!
// RGBA format
color_code_t color_codes[] =
{
	// Original id Q3 colour codes
	{"^0", { 0, 0, 0, 255 } },		// Black
	{"^1", { 255, 0, 0, 255 } },	// Red
	{"^2", { 0, 255, 0, 255 } },	// Green
	{"^3", { 255, 255, 0, 255 } },	// Yellow
	{"^4", { 0, 0, 255, 255 } },	// Blue
	{"^5", { 0, 255, 255, 255} },	// Light Blue / Cyan
	{"^6", { 255, 0, 255, 255} },	// Pink
	{"^7", { 255, 255, 255, 255 } },// White

	// Extended (some colours from the default IBM EGA palette, others from Paint.NET to get more primary colours)
	{"^8", { 85, 85, 85, 255} },	// Grey
	{"^9", { 170, 170, 170, 255} },	// Light grey
	{"^a", { 255, 106, 0, 255 } },	// Orange
	{"^b", { 170, 85, 0, 255 } },	// Brown
	{"^c", { 87, 0, 127, 255} },	// Purple
	{"^d", { 255, 85, 255, 255} },	// Bright pink
	{"^e", { 127, 106, 0, 255} },	// Gold
	{"^f", { 0, 19, 127, 255} },	// Deep blue
};

// cl_text.c : Modern Font Draw (February 18, 2024)

bool Text_GetSizeChar(const char* font, int32_t* x, int32_t* y, char text)
{
	// munge it for the fucking console which uses a 128kb buffer
	char new_string[2] = { 0 };
	new_string[0] = text;
	new_string[1] = '\0';
	return Text_GetSize(font, x, y, new_string);
}

bool Text_GetSize(const char* font, int32_t *x, int32_t *y, const char* text, ...)
{
	va_list args;
	// TODO: VARARGS
	va_start(args, text);

	char final_text[MAX_STRING_LENGTH] = { 0 };

	vsnprintf(final_text, MAX_STRING_LENGTH, text, args);
	va_end(args);

	const char* final_text_ptr = final_text;

	// localise the text
	final_text_ptr = Localisation_ProcessString(final_text);

	font_t* font_ptr = Font_GetByName(font);

	if (!font_ptr)
	{
		Com_Printf("Modern Font Engine failed to get text size: %s (tried to use invalid font %s). Trying system font.\n", text, font);

		font_ptr = Font_GetByName(cl_system_font->string);

		// already a check
		if (!font_ptr)
			return false;
	}

	// so that text doesn't appear weird - truncate and not round so that the text does not become larger than the ui elements
	int32_t font_scale = (int32_t)vid_hudscale->value;

	int32_t string_length = strlen(final_text_ptr);

	// don't bother drawing empty strings
	if (string_length == 0)
		return false;

	if (string_length > MAX_STRING_LENGTH)
	{
		Com_Printf("Tried to print text of pre-colour code length %d, max %d", string_length, MAX_STRING_LENGTH);
		return false;
	}

	int32_t longest_line_x = 0;

	int32_t size_x = 0;
	int32_t size_y = font_ptr->line_height; // assume 1 line

	for (int32_t char_num = 0; char_num < string_length; char_num++)
	{
		char next_char = final_text_ptr[char_num];

		// if we found a newline, return x to the start and advance by the font's line height
		// strings are literally encoded as "\n" in localisation strings
		if ((next_char == '\n'
			|| next_char == '\v')
			|| ((string_length - char_num > 1) && next_char == '\\' && next_char + 1 == 'n')) // uses vertical tabs as well? 
		{
			// check for \n so we can skip it
			if (next_char == '\\')
				char_num += 2;

			if (size_x > longest_line_x) longest_line_x = size_x;
			size_x = 0;

			continue; // skip newlines
		}

		// if we found a space, advance (no character drawn)
		if (next_char == ' ')
		{
			// just use the size/2 for now
			size_x += (font_ptr->size / 2.5f) * font_scale;
			continue; // skip spaces
		}

		// if we've found a tab, advance by the amount above
		if (next_char == '\t')
		{
			size_x += (font_ptr->size / 2.5f) * font_scale * TAB_SIZE_CHARS;
			continue; // skip spaces
		}

		// skip colour codes in size determination
		// ^ is used for colour codes
		if (next_char == '^')
		{
			// determine if the colour code the user supplied is valid (the character and the character after match one of the color codes in the table defined above.
			// don't do anything (will draw the invalid color code) if 
			bool done = false;

			for (int32_t color_code_num = 0; color_code_num < NUM_COLOR_CODES; color_code_num++)
			{
				color_code_t current_color_code = color_codes[color_code_num];

				if (!strncmp(final_text_ptr + char_num, current_color_code.name, 2))
				{
					// ignore
					done = true;
					break;
				}
			}

			// stupid hack
			// if we've found a valid color code, skip BOTH the color code indicator (^) and the 
			if (done)
			{
				char_num++;
				continue; // skip next 2 characters (character after is skipped by the for loop)
			}
		}

		// get the glyph to be drawn
		glyph_t* glyph = Glyph_GetByChar(font_ptr, next_char);

		if (glyph == NULL)
		{
			// todo: block character
			Com_Printf("Error: Tried to get size of glyph char code %02xh not defined in font TGA %s (Skipping)!\n", next_char, font);
			continue; // skip
		}

		// ignore offset for size calculation as it's not used in calculating next position
		// we don't need y as char_height automatically accounts for the "longest" characters' y, plus one pixel.
		// move to next char
		size_x += (glyph->x_advance * font_scale);
	}

	// in the case of only 1 line
	if (longest_line_x == 0) longest_line_x = size_x;

	*x = longest_line_x;
	*y = size_y;

	return true;
}

void Text_DrawChar(const char* font, int32_t x, int32_t y, char text)
{
	// munge it for the fucking console which uses a 128kb buffer
	char new_string[2] = { 0 };
	new_string[0] = text;
	new_string[1] = '\0';
	Text_Draw(font, x, y, new_string);
}

void Text_DrawPerform(const char* font, int32_t x, int32_t y, color4_t draw_color, const char* text, va_list args)
{
	// color4_t* is for functions above

	// setup the text
	char final_text[MAX_STRING_LENGTH] = { 0 };
	vsnprintf(final_text, MAX_STRING_LENGTH, text, args);

	// localise the text
	const char* final_text_ptr = final_text;
	final_text_ptr = Localisation_ProcessString(final_text);

	// get the font to be used for drawing the text
	font_t* font_ptr = Font_GetByName(font);

	// so that text doesn't appear weird - truncate and not round so that the text does not become larger than the ui elements
	int32_t font_scale = (int32_t)vid_hudscale->value;

	if (!font_ptr)
	{
		Com_Printf("Failed to get text size: %s (tried to use invalid font %s\n). Trying system font.", text, font);

		font_ptr = Font_GetByName(cl_system_font->string);

		// we should already refuse to run without a system font but return anyway
		if (!font_ptr)
			return;
	}

	int32_t string_length = strlen(final_text_ptr);

	if (string_length == 0)
		return;

	if (string_length > MAX_STRING_LENGTH)
	{
		Com_Printf("Tried to print text of pre-colour code length %d, max %d", string_length, MAX_STRING_LENGTH);
		return;
	}

	int32_t initial_x = x;

	int32_t current_x = x;
	int32_t current_y = y;

	// default is white

	color4_t color = { 255, 255, 255, 255 };

	//...unless it was overridden
	if (draw_color)
	{
		VectorCopy3(draw_color, color);
	}

	for (int32_t char_num = 0; char_num < string_length; char_num++)
	{
		char next_char = final_text_ptr[char_num];

		// if we found a newline, return x to the start and advance by the font's line height
		// strings are literally encoded as "\n" in localisation strings

		// value by reference because i can't be bothered to change the above to a pointer and add millions of deref operations
		if ((next_char == '\n'
			|| next_char == '\v')
			|| ((string_length - char_num > 1)
				&& final_text_ptr[char_num] == '\\'
				&& final_text_ptr[char_num + 1] == 'n')) // uses vertical tabs as well? 
		{
			current_y += font_ptr->line_height * font_scale;
			current_x = initial_x;

			// skip next character
			if (next_char == '\\')
				char_num++;

			continue; // skip newlines
		}

		// if we found a space, advance (no character drawn)
		if (next_char == ' ')
		{
			// just use the size of the font divided by 2.5 (TODO: DEFINE THIS) for now
			current_x += (int32_t)(font_ptr->size / 2.5f) * font_scale;
			continue; // skip spaces
		}

		// if we've found a tab, advance by the amount above
		if (next_char == '\t')
		{
			current_x += (int32_t)(font_ptr->size / 2.5f) * font_scale * TAB_SIZE_CHARS;
			continue; // skip spaces
		}

		// ^ is used for colour codes
		if (next_char == '^')
		{
			// determine if the colour code the user supplied is valid (the character and the character after match one of the color codes in the table defined above.
			// don't do anything (will draw the invalid color code) if 
			bool done = false;

			for (int32_t color_code_num = 0; color_code_num < NUM_COLOR_CODES; color_code_num++)
			{
				color_code_t current_color_code = color_codes[color_code_num];
				if (!strncmp(final_text_ptr + char_num, color_codes[color_code_num].name, 2))
				{
					// todo: vector* macros for 4
					color[0] = current_color_code.color[0];
					color[1] = current_color_code.color[1];
					color[2] = current_color_code.color[2];
					color[3] = current_color_code.color[3];

					// ignore
					done = true;
					break;
				}
			}

			// stupid hack
			// if we've found a valid color code, skip BOTH the color code indicator (^) and the 
			if (done)
			{
				char_num++;
				continue; // skip next 2 characters (character after is skipped by the for loop)
			}
		}

		// get the glyph to be drawn
		glyph_t* glyph = Glyph_GetByChar(font_ptr, next_char);

		if (glyph == NULL)
		{
			// todo: block character
			Com_Printf("Error: Tried to draw glyph char code %02xh not defined in font TGA %s (Skipping)!\n", next_char, font);
			continue; // skip
		}

		// handle offset
		int32_t draw_x = current_x + glyph->x_offset * font_scale;
		int32_t draw_y = current_y + glyph->y_offset * font_scale;

		// convert to a file path that drawpicregion in the fonts folder
		char final_name[MAX_QPATH] = { 0 };

		snprintf(final_name, MAX_QPATH, "fonts/%s", font_ptr->name);

		// draw it
		re.DrawFontChar(draw_x, draw_y, glyph->x_start, glyph->y_start, glyph->x_start + glyph->width, glyph->y_start + glyph->height, final_name, color, false);

		// move to next char
		current_x += (glyph->x_advance * font_scale);
	}
}

void Text_Draw(const char* font, int32_t x, int32_t y, const char* text, ...)
{
	va_list args;

	va_start(args, text);
	Text_DrawPerform(font, x, y, NULL, text, args);
	va_end(args);
}

void Text_DrawColor(const char* font, int32_t x, int32_t y, color4_t color, const char* text, ...)
{
	va_list args;
	va_start(args, text);
	Text_DrawPerform(font, x, y, color, text, args);
	va_end(args);
}