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

//
// console
//
#pragma once

#define	NUM_CON_CHAT_LINES	9			// Number of lines that will be disabled in chat
#define	CON_TEXTSIZE		131072		// Size of console text buffer

typedef struct console_s
{
	bool	initialized;

	char	text[CON_TEXTSIZE];
	int32_t current;					// line where next message will be printed
	int32_t x;							// offset in current line for next print
	int32_t display;					// bottom of console displays this line

	int32_t linewidth;					// characters across screen
	int32_t totallines;					// total lines in console scrollback

	float	cursorspeed;

	int32_t vislines;

	float	times[NUM_CON_CHAT_LINES];	// cls.realtime time the line was generated
	// for transparent notify lines
} console_t;

extern console_t con;

void Con_CheckResize();
void Con_Init();
void Con_DrawConsole(float frac);
void Con_Print(char* txt);
void Con_CenteredPrint(char* text);
void Con_Clear_f();
void Con_DrawRecentHistory();
void Con_ClearRecentHistory();
void Con_ToggleConsole_f();
