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

int32_t intro_start_time = 0;
bool	intro_running;

void Intro_Start()
{
	intro_running = true; 
	cls.disable_input = true;
	Input_Activate(false);

	intro_start_time = Sys_Milliseconds();
}

void Intro_Update()
{
	if (!intro_running)
		return;

	int32_t current_time = Sys_Milliseconds();
	int32_t current_time_intro = (current_time - intro_start_time);

	int32_t end_time = intro_start_time + cl_intro1_time->value + cl_intro2_time->value;
	
	// end the intro if we reached the end time
	if (current_time > end_time)
	{
		cls.disable_input = false;
		intro_running = false;

		// Technically this isn't needed but if we ever change what happens after the intro it might be, so keep it to be safe
		Input_Activate(true);
	}

	// 255,255,255 = multiply by 1
	color4_t fade_colour = { 255, 255, 255, 0 };

	float start_fade = 0.4;
	float end_fade = 0.6;

	// show first image
	if (current_time_intro <= cl_intro1_time->value)
	{
		// make it fade
		fade_colour[3] = 255 * sinf(M_PI * (current_time_intro / cl_intro1_time->value));

		re.DrawPicStretch(0, 0, r_width->value, r_height->value, cl_intro1->string, fade_colour, true);
	}
	else // show second image
	{
		// for sine function
		int32_t current_time_intro2 = (current_time - (intro_start_time + cl_intro1_time->value));

		fade_colour[3] = 255 * sinf(M_PI * (current_time_intro2 / cl_intro2_time->value));

		re.DrawPicStretch(0, 0, r_width->value, r_height->value, cl_intro2->string, fade_colour, true);
	}
}

