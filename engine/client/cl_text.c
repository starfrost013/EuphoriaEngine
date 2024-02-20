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
#include "client.h"

// The list of valid colour codes.
// This is based on Quake 3 (which is also used by COD games as late as Black Ops III), but additional colours are added while still remaining compatible.
// RGBA format
color_code_t color_codes[16] =
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

void Text_Draw(const char* font, int x, int y, char* text, ...)
{
	// TODO: COLOUR
	// TODO: VARARGS

	font_t* font_ptr = Font_GetByName(font);

	if (font_ptr == NULL)
	{
		Com_Printf("Modern Font Engine failed to draw text: %s (tried to use invalid font %s\n)", text, font);
		return;
	}

	int string_length = strlen(text);

	if (string_length == 0)
	{
		// don't bother drawing empty strings
		return; 
	}
	
	int initial_x = x;

	int current_x = x;
	int current_y = y;

	for (int char_num = 0; char_num < string_length; char_num++)
	{
		char next_char = text[char_num];

		// if we found a newline, return x to the start and advance by the font's line height
		if (next_char == '\n')
		{
			y += font_ptr->line_height;
			current_x = initial_x;
			continue; // skip newlinse
		}

		// if we found a space, advance (no character drawn)
		if (next_char == ' ')
		{
			// just use the size/2 for now
			current_x += font_ptr->size/2;
			continue; // skip spaces
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
		int draw_x = current_x + glyph->x_offset;
		int draw_y = current_y + glyph->y_offset;

		// convert to a file path that drawpicregion in the fonts folder
		char final_name[MAX_FONT_FILENAME_LEN] = { 0 };

		snprintf(&final_name, MAX_FONT_FILENAME_LEN, "fonts/%s", font_ptr->name);

		// draw it
		re.DrawPicRegion(draw_x, draw_y, glyph->x_start, glyph->y_start, glyph->x_start + glyph->x_advance, glyph->y_start + glyph->height, final_name);

		// move to next char
		current_x += (glyph->x_advance);
	}
}