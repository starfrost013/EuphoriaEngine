/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018-2019 Krzysztof Kondrak
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
// cl_render_2d.c -- 2d parts of rendering: refresh, status bar, console, chat, notify, etc
// This sucks ass get rid of it

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  full screen image for quit and victory

  end of unit intermissions

  */

#include <client/client.hpp>
#include <cinttypes>

float	scr_con_current;	// aproaches scr_conlines at scr_conspeed
float	scr_conlines;		// 0.0 to 1.0 lines of console to display

bool	scr_initialized;		// ready to draw

int32_t scr_draw_loading;

vrect_t	scr_vrect;		// position of render window on screen

cvar_t* scr_viewsize;
cvar_t* scr_conspeed;
cvar_t* scr_centertime;
cvar_t* scr_showpause;

cvar_t* scr_timegraph;
cvar_t* scr_netgraph;
cvar_t* scr_graphheight;
cvar_t* scr_graphscale;
cvar_t* scr_graphshift;

cvar_t* vid_hudscale;

extern cvar_t* vid_ref;

typedef struct dirty_s
{
	int32_t 	x1;
	int32_t		y1;
	int32_t		x2;
	int32_t		y2;
} dirty_t;

dirty_t		scr_dirty, scr_old_dirty[2];

char		crosshair_pic[MAX_QPATH];
int32_t 	crosshair_width, crosshair_height;

void Render2D_TimeRefresh_f();
void Render2D_Loading_f();


/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void Render2D_AddNetgraph()
{
	int32_t in;
	int32_t ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_timegraph->value)
		return;
	// see what the latency was on this packet
	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP - 1);
	ping = cls.realtime - cl.cmd_time[in];
	ping /= 30;
	if (ping > 30)
		ping = 30;
	Render2D_DebugGraph(ping, 18, 15, 22, 255);
}

typedef struct
{
	float	value;
	vec_t   color[4];
} graphsamp_t;

static int32_t 		current;
static graphsamp_t	values[1024];

/*
==============
scr_netgraph
==============
*/
void Render2D_DebugGraph(float value, int32_t r, int32_t g, int32_t b, int32_t a)
{
	values[current & 1023].value = value;
	values[current & 1023].color[0] = r;
	values[current & 1023].color[1] = g;
	values[current & 1023].color[2] = b;
	values[current & 1023].color[3] = a;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void Render2D_DrawDebugGraph()
{
	int32_t		a, x, y, w, i, h;
	color4_t	debug_graph_color = { 123, 123, 123, 255 };
	float		v;

	//
	// draw the graph
	//
	w = scr_vrect.width;

	x = scr_vrect.x;
	y = scr_vrect.y + scr_vrect.height;
	re.DrawFill(x, y - scr_graphheight->value,
		w, scr_graphheight->value, debug_graph_color);

	for (a = 0; a < w; a++)
	{
		i = (current - 1 - a + 1024) & 1023;
		v = values[i].value;
		v = v * scr_graphscale->value + scr_graphshift->value;

		if (v < 0)
			v += scr_graphheight->value * (1 + (int32_t)(-v / scr_graphheight->value));
		h = (int32_t)v % (int32_t)scr_graphheight->value;
		re.DrawFill(x + w - 1 - a, y - h, 1, h, values[i].color);
	}
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char	scr_centerstring[1024];
float	scr_centertime_start;	// for slow victory printing
float	scr_centertime_off;
int32_t scr_center_lines;
int32_t scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void Render2D_CenterPrint(char* str)
{
	char* s;

	strncpy(scr_centerstring, str, sizeof(scr_centerstring));
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.time;

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;

		s++;
	}

	// echo it to the console
	Com_Printf("\n\n");

	Com_Printf("%s", str);

	Com_Printf("\n\n");
	Con_ClearRecentHistory();
}


void Render2D_DrawCenterString()
{
	char* start;
	int32_t x, y;
	int32_t remaining;
	int32_t size_x = 0, size_y = 0;
	font_t* system_font_ptr = Font_GetByName(cl_system_font->string);

	// the finale prints the characters one at a time
	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = r_height->value * 0.35f;
	else
		y = 48;

	Text_GetSize(cl_system_font->string, &size_x, &size_y, start);
	x = (r_width->value - size_x * vid_hudscale->value) / 2;
	Render2D_AddDirtyPoint(x, y);
	Text_Draw(cl_system_font->string, x, y, start);
	Render2D_AddDirtyPoint(x, y + system_font_ptr->line_height * vid_hudscale->value);
}

void Render2D_CheckDrawCenterString()
{
	scr_centertime_off -= cls.frametime;

	if (scr_centertime_off <= 0)
		return;

	Render2D_DrawCenterString();
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
void Render2D_CalcVrect()
{
	int32_t 	size;

	// bound viewsize
	if (scr_viewsize->value < 40)
		Cvar_Set("viewsize", "40");
	if (scr_viewsize->value > 100)
		Cvar_Set("viewsize", "100");

	size = scr_viewsize->value;

	scr_vrect.width = r_width->value * size / 100;
	scr_vrect.height = r_height->value * size / 100;

	scr_vrect.x = (r_width->value - scr_vrect.width) / 2;
	scr_vrect.y = (r_height->value - scr_vrect.height) / 2;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void Render2D_SizeUp_f()
{
	Cvar_SetValue("viewsize", scr_viewsize->value + 10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void Render2D_SizeDown_f()
{
	Cvar_SetValue("viewsize", scr_viewsize->value - 10);
}

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void Render2D_Sky_f()
{
	float	rotate;
	vec3_t	axis;

	if (Cmd_Argc() < 2)
	{
		Com_Printf("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}

	if (Cmd_Argc() > 2)
		rotate = (float)atof(Cmd_Argv(2));
	else
		rotate = 0;

	if (Cmd_Argc() == 6)
	{
		axis[0] = (float)atof(Cmd_Argv(3));
		axis[1] = (float)atof(Cmd_Argv(4));
		axis[2] = (float)atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	re.SetSky(Cmd_Argv(1), rotate, axis);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void Render2D_Init()
{
	scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
	scr_conspeed = Cvar_Get("scr_conspeed", "3", 0);
	scr_showpause = Cvar_Get("scr_showpause", "1", 0);
	scr_centertime = Cvar_Get("scr_centertime", "2.5", 0);
	scr_timegraph = Cvar_Get("timegraph", "0", 0);
	scr_netgraph = Cvar_Get("netgraph", "0", 0);
	scr_graphheight = Cvar_Get("graphheight", "32", 0);
	scr_graphscale = Cvar_Get("graphscale", "1", 0);
	scr_graphshift = Cvar_Get("graphshift", "0", 0);

	//
	// register our commands
	//
	Cmd_AddCommand("timerefresh", Render2D_TimeRefresh_f);
	Cmd_AddCommand("loading", Render2D_Loading_f);
	Cmd_AddCommand("sizeup", Render2D_SizeUp_f);
	Cmd_AddCommand("sizedown", Render2D_SizeDown_f);
	Cmd_AddCommand("sky", Render2D_Sky_f);

	scr_initialized = true;
}


/*
==============
SCR_DrawNet
==============
*/
void Render2D_DrawNet()
{
	if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged
		< CMD_BACKUP - 1)
		return;

	re.DrawPic(scr_vrect.x + 64, scr_vrect.y, "2d/net", NULL, false);
}

/*
==============
SCR_DrawPause
==============
*/
void Render2D_DrawPause()
{
	int32_t w, h;

	if (!scr_showpause->value)		// turn off for screenshots
		return;

	if (!cl_paused->value)
		return;

	re.DrawGetPicSize(&w, &h, "2d/pause");
	re.DrawPic((r_width->value - w) / 2, r_height->value / 2 + 8 * vid_hudscale->value, "2d/pause", NULL, false);
}

/*
==============
SCR_DrawLoading
==============
*/
void Render2D_DrawLoading()
{
	int32_t w, h;

	if (!scr_draw_loading)
		return;

	scr_draw_loading = false;
	re.DrawGetPicSize(&w, &h, "2d/loading");
	re.DrawPic((r_width->value - w) / 2, (r_height->value - h) / 2, "2d/loading", NULL, false);
}

//=============================================================================

/*
==================
SCR_RunConsole

Scroll it up or down
==================
*/
void Render2D_RunConsole()
{
	// decide on the height of the console
	if (cls.input_dest == key_console)
		scr_conlines = cl_console_fraction->value;		// half screen
	else
		scr_conlines = 0;				// none visible

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value * cls.frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value * cls.frametime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

}

/*
==================
SCR_DrawConsole
==================
*/
void Render2D_DrawConsole()
{
	Con_CheckResize();

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
	{	// forced full screen console
		Con_DrawConsole(scr_con_current);
		return;
	}

	if (cls.state != ca_active || !cl.refresh_prepped)
	{	// connected, but can't render
		Con_DrawConsole(scr_con_current);
		color4_t colour = { 0, 0, 0, 255 };
		re.DrawFill(0, r_height->value / 2, r_width->value, r_height->value / 2, colour);
		return;
	}

	if (scr_con_current)
	{
		Con_DrawConsole(scr_con_current);
	}
	else
	{
		if (cls.input_dest == key_game || cls.input_dest == key_message)
			Con_DrawRecentHistory();	// only draw notify in game
	}
}

//=============================================================================

/*
================
SCR_BeginLoadingPlaque
================
*/
void Render2D_BeginLoadingPlaque()
{
	S_StopAllSounds();
	cl.sound_prepped = false;		// don't play ambients
	Miniaudio_Stop();
	if (cls.disable_screen)
		return;
	if (developer->value)
		return;
	if (cls.state == ca_disconnected)
		return;	// if at console, don't bring up the plaque
	if (cls.input_dest == key_console)
		return;

	scr_draw_loading = 1;

	Render_UpdateScreen();
	cls.disable_screen = Sys_Milliseconds();
	cls.disable_servercount = cl.servercount;
}

/*
================
SCR_EndLoadingPlaque
================
*/
void Render2D_EndLoadingPlaque()
{
	cls.disable_screen = 0;
	Con_ClearRecentHistory();
}

/*
================
SCR_Loading_f
================
*/
void Render2D_Loading_f()
{
	Render2D_BeginLoadingPlaque();
}

/*
================
SCR_TimeRefresh_f
================
*/
int32_t CompareEntities(const entity_t* a, const entity_t* b)
{
	/*
	** all other models are sorted by model then skin
	*/
	if (a->model == b->model)
	{
		return ((intptr_t)a->skin - (intptr_t)b->skin);
	}
	else
	{
		return ((intptr_t)a->model - (intptr_t)b->model);
	}
}

void Render2D_TimeRefresh_f()
{
	int32_t i;
	int32_t start, stop;
	float	time;

	if (cls.state != ca_active)
		return;

	start = Sys_Milliseconds();

	if (Cmd_Argc() == 2)
	{	// run without page flipping
		re.BeginFrame();
		for (i = 0; i < 128; i++)
		{
			cl.refdef.viewangles[1] = i / 128.0f * 360.0f;
			re.RenderFrame(&cl.refdef);
		}
		re.EndFrame();
	}
	else
	{
		for (i = 0; i < 128; i++)
		{
			cl.refdef.viewangles[1] = i / 128.0f * 360.0f;

			re.BeginFrame();
			re.RenderFrame(&cl.refdef);
			re.EndFrame();
		}
	}

	stop = Sys_Milliseconds();
	time = (stop - start) / 1000.0f;
	Com_Printf("%f seconds (%f fps)\n", time, 128 / time);
}

/*
=================
SCR_AddDirtyPoint
=================
*/
void Render2D_AddDirtyPoint(int32_t x, int32_t y)
{
	if (x < scr_dirty.x1)
		scr_dirty.x1 = x;
	if (x > scr_dirty.x2)
		scr_dirty.x2 = x;
	if (y < scr_dirty.y1)
		scr_dirty.y1 = y;
	if (y > scr_dirty.y2)
		scr_dirty.y2 = y;
}

void Render2D_DirtyScreen()
{
	Render2D_AddDirtyPoint(0, 0);
	Render2D_AddDirtyPoint(r_width->value - 1, r_height->value - 1);
}

/*
==============
SCR_TileClear

Clear any parts of the tiled background that were drawn on last frame
==============
*/
void Render2D_TileClear()
{
	if (scr_con_current == 1.0)
		return;		// full screen console
	if (scr_viewsize->value == 100)
		return;		// full screen rendering

	if (scr_vrect.y > 0)
	{	// clear above view screen
		re.DrawTileClear(scr_vrect.x, 0,
			scr_vrect.width, scr_vrect.y, "2d/backtile");

		// clear below view screen
		re.DrawTileClear(scr_vrect.x, scr_vrect.y + scr_vrect.height,
			scr_vrect.width, r_height->value - scr_vrect.height - scr_vrect.y, "2d/backtile");
	}

	if (scr_vrect.x > 0)
	{	// clear left of view screen
		re.DrawTileClear(0, 0, scr_vrect.x, r_height->value, "2d/backtile");

		// clear right of view screen
		re.DrawTileClear(scr_vrect.x + scr_vrect.width, 0, scr_vrect.width, r_height->value, "2d/backtile");
	}
}


//===============================================================


#define STAT_MINUS		10	// num frame for '-' stats digit
const char* sb_nums[2][11] =
{
	// 2 sets of big numbers
	{"2d/num_0", "2d/num_1", "2d/num_2", "2d/num_3", "2d/num_4", "2d/num_5", "2d/num_6", "2d/num_7", "2d/num_8", "2d/num_9", "2d/num_minus"},
	{"2d/anum_0", "2d/anum_1", "2d/anum_2", "2d/anum_3", "2d/anum_4", "2d/anum_5", "2d/anum_6", "2d/anum_7", "2d/anum_8", "2d/anum_9", "2d/anum_minus"}
};

#define	ICON_WIDTH	24
#define	ICON_HEIGHT	24
#define	CHAR_WIDTH	16 * vid_hudscale->value
#define	ICON_SPACE	8

void Render2D_DrawCenteredString(const char* string, int32_t x, int32_t y, int32_t centerwidth)
{
	int32_t margin;
	char	line[1024];
	int32_t width;
	font_t* system_font_ptr = Font_GetByName(cl_system_font->string);

	margin = x;

	while (*string)
	{
		// scan out one line of text from the string
		width = 0;
		while (*string && *string != '\n')
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth)
			x = margin + ((centerwidth - width * 8) * vid_hudscale->value) / 2;
		else
			x = margin;

		// set string terminator for text engine
		char orig_end_of_line = line[width];
		line[width] = '\0';

		Text_Draw(cl_system_font->string, x, y, line);

		line[width] = orig_end_of_line;
		if (*string)
		{
			string++;	// skip the \n
			x = margin;
			y += system_font_ptr->line_height * vid_hudscale->value;
		}
	}
}


/*
==============
SCR_DrawField
==============
*/
void Render2D_DrawField(int32_t x, int32_t y, int32_t color, int32_t width, int32_t value)
{
	char	num[16], * ptr;
	int32_t 	l;
	int32_t 	frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	Render2D_AddDirtyPoint(x, y);
	Render2D_AddDirtyPoint(x + width * CHAR_WIDTH + 2, y + 23);

	snprintf(num, sizeof(num), "%i", value);
	l = (int32_t)strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH * (width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		re.DrawPic(x, y, sb_nums[color][frame], NULL, false);
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}


/*
===============
SCR_TouchPics

Allows rendering code to cache all needed sbar graphics
===============
*/
void Render2D_TouchPics()
{
	int32_t i, j;

	for (i = 0; i < 2; i++)
		for (j = 0; j < 11; j++)
			re.RegisterPic(sb_nums[i][j]);

	if (crosshair->value)
	{
		cvar_t* scale = Cvar_Get("hudscale", "1", 0);

		if (crosshair->value > 3 || crosshair->value < 0)
			crosshair->value = 3;

		snprintf(crosshair_pic, sizeof(crosshair_pic), "2d/ch%i", (int32_t)(crosshair->value));
		re.DrawGetPicSize(&crosshair_width, &crosshair_height, crosshair_pic);

		// remove scaling - it will be applied during draw with proper screen centering
		if (scale->value > 1)
		{
			crosshair_width /= (int32_t)scale->value;
			crosshair_height /= (int32_t)scale->value;
		}

		if (!crosshair_width)
			crosshair_pic[0] = 0;
	}
}

/*
================
SCR_ExecuteLayoutString

================
*/
void Render2D_ExecuteLayoutString(char* s)
{
	int32_t 	x, y;
	int32_t 	value;
	const char*	token;
	int32_t 	width;
	int32_t 	index;

	if (cls.state != ca_active || !cl.refresh_prepped)
		return;

	if (!s[0])
		return;

	x = 0;
	y = 0;
	width = 3;

	while (s)
	{
		token = COM_Parse(&s);

		if (!strcmp(token, "xl"))
		{
			token = COM_Parse(&s);
			x = atoi(token) * vid_hudscale->value;
			continue;
		}

		if (!strcmp(token, "xr"))
		{
			token = COM_Parse(&s);
			x = r_width->value + atoi(token) * vid_hudscale->value;
			continue;
		}

		if (!strcmp(token, "xv"))
		{
			token = COM_Parse(&s);
			x = r_width->value / 2 - 160 * vid_hudscale->value + atoi(token) * vid_hudscale->value;
			continue;
		}

		if (!strcmp(token, "yt"))
		{
			token = COM_Parse(&s);
			y = atoi(token) * vid_hudscale->value;
			continue;
		}

		if (!strcmp(token, "yb"))
		{
			token = COM_Parse(&s);
			y = r_height->value + atoi(token) * vid_hudscale->value;
			continue;
		}

		if (!strcmp(token, "yv"))
		{
			token = COM_Parse(&s);
			y = r_height->value / 2 - 120 * vid_hudscale->value + atoi(token) * vid_hudscale->value;
			continue;
		}

		if (!strcmp(token, "pic"))
		{	// draw a pic from a stat number
			token = COM_Parse(&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (value >= MAX_IMAGES)
				Com_Error(ERR_DROP, "Pic >= MAX_IMAGES");
			if (cl.configstrings[CS_IMAGES + value])
			{
				Render2D_AddDirtyPoint(x, y);
				Render2D_AddDirtyPoint(x + 23, y + 23);
				re.DrawPic(x, y, cl.configstrings[CS_IMAGES + value], NULL, false);
			}
			continue;
		}

		if (!strcmp(token, "picn"))
		{	// draw a pic from a name
			token = COM_Parse(&s);
			Render2D_AddDirtyPoint(x, y);
			Render2D_AddDirtyPoint(x + 23, y + 23);
			re.DrawPic(x, y, token, NULL, false);
			continue;
		}

		if (!strcmp(token, "num"))
		{	
			// draw a number
			token = COM_Parse(&s);
			width = atoi(token);
			token = COM_Parse(&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			Render2D_DrawField(x, y, 0, width, value);
			continue;
		}

		if (!strcmp(token, "hnum"))
		{	
			// health number
			int32_t 	color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_HEALTH];
			if (value > 25)
				color = 0;	// green
			else if (value > 0)
				color = (cl.frame.serverframe >> 2) & 1;		// flash
			else
				color = 1;

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 1)
				re.DrawPic(x, y, "2d/field_3", NULL, false);

			Render2D_DrawField(x, y, color, width, value);
			continue;
		}

		if (!strcmp(token, "anum"))
		{	// ammo number
			int32_t 	color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_AMMO];
			if (value > 5)
				color = 0;	// green
			else if (value >= 0)
				color = (cl.frame.serverframe >> 2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 4)
				re.DrawPic(x, y, "2d/field_3", NULL, false);

			Render2D_DrawField(x, y, color, width, value);
			continue;
		}

		if (!strcmp(token, "rnum"))
		{	// armor number
			int32_t 	color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_ARMOR];
			if (value < 1)
				continue;

			color = 0;	// green

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 2)
				re.DrawPic(x, y, "2d/field_3", NULL, false);

			Render2D_DrawField(x, y, color, width, value);
			continue;
		}


		if (!strcmp(token, "stat_string"))
		{
			token = COM_Parse(&s);
			index = atoi(token);
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error(ERR_DROP, "Bad stat_string index");
			index = cl.frame.playerstate.stats[index];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error(ERR_DROP, "Bad stat_string index");
			Text_Draw(cl_system_font->string, x, y, cl.configstrings[index]);
			continue;
		}

		if (!strcmp(token, "cstring"))
		{
			token = COM_Parse(&s);
			Render2D_DrawCenteredString(token, x, y, 320);
			continue;
		}

		if (!strcmp(token, "string"))
		{
			token = COM_Parse(&s);
			//todo: is this terminated properly?
			Text_Draw(cl_system_font->string, x, y, token);
			continue;
		}

		if (!strcmp(token, "if"))
		{	// draw a number
			token = COM_Parse(&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (!value)
			{	// skip to endif
				while (s && strcmp(token, "endif"))
				{
					token = COM_Parse(&s);
				}
			}

			continue;
		}
	}
}

/*
================
SCR_DrawStats

The status bar is a small layout program that
is based on the stats array
================
*/
void Render2D_DrawStats()
{
	Render2D_ExecuteLayoutString(cl.configstrings[CS_STATUSBAR]);
}

/*
================
SCR_DrawLayout

================
*/
#define	STAT_LAYOUTS		13

void Render2D_DrawLayout()
{
	if (!cl.frame.playerstate.stats[STAT_LAYOUTS])
		return;
	Render2D_ExecuteLayoutString(cl.layout);
}

// Draws some debug info
void Render2D_DrawInfo()
{
	if (cls.state == ca_connecting
		|| cls.state == ca_disconnected)
	{
		return; 
	}

	font_t* console_font_ptr = Font_GetByName(cl_console_font->string);

	if (console_font_ptr == NULL)
	{
		Sys_Error("cl_console_font NULL in SCR_DrawPos!");
		return;
	}

	// I really should make it be an average.

	float target_fps = 60;

	if (cl_maxfps->value < target_fps) target_fps = cl_maxfps->value;

	float warning_fps = target_fps * (55.0f / 60.0f);
	float alarm_fps = (target_fps / 2);

	int32_t x = (10 * vid_hudscale->value);
#ifdef DEBUG
	int32_t y = (r_height->value - (202 * vid_hudscale->value));
#else
	int32_t y = (r_height->value - (155 * vid_hudscale->value));
#endif

	// used for Text_GetSize calls
	int32_t size_x = 0, size_y = 0;

	// cls.frametime limited to 0.2s for sim purposes
	float real_frametime = (1000.0f / cls.fps);

	// draw dynamically allocated memory information on debug builds
	// this is slow so turn it off on relase

#ifdef DEBUG

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y, "Zone memory: %d bytes (count: %d)", 
		z_bytes, z_count);

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y, "Hunk memory: %.0f bytes (count: %.0f)", 
		hunk_total->value, hunk_areas->value);

	float total_bytes = hunk_total->value + (float)z_bytes;
	float total_areas = hunk_total->value + hunk_areas->value;
	float total_megabytes = (total_bytes) / 1000000.0f;

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y, "Total dynamically allocated: %.0f bytes, %.2f MB (count: %.0f)", 
		total_bytes, total_megabytes, hunk_areas->value);

	y += console_font_ptr->line_height * vid_hudscale->value * 2;
#endif

	if (cls.fps < alarm_fps)
	{
		Text_Draw(cl_console_font->string, x, y,
			"FPS: ^1%.2f (%.2fms) (The game is lagging - <50\045 target FPS %.1f)!", cls.fps, real_frametime, target_fps);
	}
	else if (cls.fps < warning_fps
		&& cls.fps > alarm_fps)
	{
		Text_Draw(cl_console_font->string, x, y,
			"FPS: ^3%.2f (%.2fms) (Significantly below target FPS %.1f)!", cls.fps, real_frametime, target_fps);
	}
	else
	{
		Text_Draw(cl_console_font->string, x, y,
			"FPS: %.2f (%.2fms)", cls.fps, real_frametime);
	}

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y,
		"Position: X: %.2f  Y: %.2f  Z: %.2f", cl.frame.playerstate.pmove.origin[0], cl.frame.playerstate.pmove.origin[1], cl.frame.playerstate.pmove.origin[2]);

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y,
		"Velocity: X: %.2f  Y: %.2f  Z: %.2f", cl.frame.playerstate.pmove.velocity[0], cl.frame.playerstate.pmove.velocity[1], cl.frame.playerstate.pmove.velocity[2]);

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y,
		"Camera origin: X: %.2f  Y: %.2f  Z: %.2f (Type: %s)",
		cl.frame.playerstate.vieworigin[0], cl.frame.playerstate.vieworigin[1], cl.frame.playerstate.vieworigin[2], camera_type_names[cl.frame.playerstate.camera_type]);

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y,
		"Camera angle: X: %.2f  Y: %.2f  Z: %.2f (Refdef: X: %.2f  Y: %.2f  Z: %.2f)", cl.viewangles[0], cl.viewangles[1], cl.viewangles[2],
		cl.refdef.viewangles[0], cl.refdef.viewangles[1], cl.refdef.viewangles[2]);

	y += console_font_ptr->line_height * vid_hudscale->value * 2;
	Text_Draw(cl_console_font->string, x, y,
		"Client Entities: %d/%d", r_numentities, MAX_ENTITIES);

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y,
		"Server Entities: %d/%d", CL_CountReceivedEntities(), MAX_EDICTS);

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y,
		"Dynamic Lights: %d/%d", r_numdlights, MAX_DLIGHTS);

	y += console_font_ptr->line_height * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y,
		"Particles: %d/%d", r_numparticles, MAX_PARTICLES);

	y += console_font_ptr->line_height * 2 * vid_hudscale->value;
	Text_Draw(cl_console_font->string, x, y, "Map: %s", map_name);

	// draw the detailed version information on the other side of the screen

	// we don't have justify so just draw two lines
	const char* version_string_line1 = "Engine version %s";
	const char* version_string_line2 = "Gamedll version %s";

	cvar_t* game_version = Cvar_Get("gameversion", "0.0.0", CVAR_SERVERINFO | CVAR_LATCH);

	// draw detailed version information
	Text_GetSize(cl_console_font->string, &size_x, &size_y, version_string_line1, engine_version->string);

	// add a bit of padding
	// engine version
	x = r_width->value - size_x - 3;
	y = r_height->value - (console_font_ptr->line_height * vid_hudscale->value * 2) - size_y;

	Text_Draw(cl_console_font->string, x, y, version_string_line1, engine_version->string);

	// gamedll version
	Text_GetSize(cl_console_font->string, &size_x, &size_y, version_string_line2, game_version->string);

	// reset x 
	x = r_width->value - size_x - 3;
	y += console_font_ptr->line_height * vid_hudscale->value;

	Text_Draw(cl_console_font->string, x, y, version_string_line2, game_version->string);
}

void Render2D_AddFrametimeGraph()
{
	// game caps cls.frametime to 0.2, for simulation purposes
	float real_frametime = (1000.0f / cls.fps);

	// draw some nice text
	int32_t size_x = 0, size_y = 0;
	font_t* cl_console_font_ptr = Font_GetByName(cl_console_font->string);
	const char* text_line_1 = "Frametime: %.2fms";
	Text_GetSize(cl_console_font->string, &size_x, &size_y, text_line_1, real_frametime);
	int32_t y = r_height->value - scr_graphheight->value - 15;
	int32_t x = r_width->value - size_x - 3;
	Text_Draw(cl_console_font->string, x, y, text_line_1, real_frametime);

	y += cl_console_font_ptr->line_height * vid_hudscale->value;

	Render2D_DebugGraph(cls.frametime * 300, 255, 255, 255, 255);

}

/*
=================
SCR_DrawCrosshair
=================
*/
void Render2D_DrawCrosshair()
{
	if (!crosshair->value)
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		Render2D_TouchPics();
	}

	if (!crosshair_pic[0])
		return;

	cvar_t* scale = Cvar_Get("hudscale", "1", 0);

	re.DrawPic(scr_vrect.x + ((scr_vrect.width - (int32_t)scale->value * crosshair_width) >> 1)
		, scr_vrect.y + ((scr_vrect.height - (int32_t)scale->value * crosshair_height) >> 1), crosshair_pic, NULL, false);
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void Render_UpdateScreen()
{
	int32_t numframes;
	int32_t i;

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (Sys_Milliseconds() - cls.disable_screen > 120000)
		{
			cls.disable_screen = 0;
			Com_Printf("Loading plaque timed out.\n");
		}
		return;
	}

	if (!scr_initialized || !con.initialized)
		return;				// not initialized yet

	// no point doing this if we have no resolution e.g. in minimised mode

	if (r_width->value == 0
		|| r_height->value == 0)
	{
		re.EndFrame();
		return;
	}

	numframes = 1;

	for (i = 0; i < numframes; i++)
	{
		re.BeginFrame();

		// end frame and force video restart if swapchain is out of date
		if (vid_ref->modified)
		{
			re.EndWorldRenderpass();
			re.EndFrame();
			return;
		}

		// play the game intro if we are commanded to and don't do anything else
		if (intro_running)
		{
			Intro_Update();
			re.EndWorldRenderpass();
			re.EndFrame();
			return;
		}

		if (scr_draw_loading == 2)
		{	//  loading plaque over black screen
			int32_t 	w, h;

			re.EndWorldRenderpass();
			scr_draw_loading = false;
			re.DrawGetPicSize(&w, &h, "2d/loading");
			re.DrawPic((r_width->value - w) / 2, (r_height->value - h) / 2, "2d/loading", NULL, false);
		}
		else
		{
			// do 3D refresh drawing, and then update the screen
			Render2D_CalcVrect();

			Render3D_RenderView();

			// clear any dirty part of the background
			Render2D_TileClear();

			// still draw console if cl_drawhud is 0
			if (cl_drawhud->value)
			{
				Render2D_DrawStats();
				if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
					Render2D_DrawLayout();

				Render2D_DrawNet();
				Render2D_CheckDrawCenterString();

				if (scr_timegraph->value)
					Render2D_AddFrametimeGraph();

				if (scr_netgraph->value || scr_timegraph->value)
					Render2D_DrawDebugGraph();

				Render2D_DrawPause();

				UI_Draw();
			}

			if (cl_showinfo->value)
				Render2D_DrawInfo();

			if (!cl_console_disabled->value)
				Render2D_DrawConsole();

			Render2D_DrawLoading();
		}
	}
	re.EndFrame();
}
