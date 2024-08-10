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
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <client/client.h>
#include <client/include/menu_framework.h>

extern cvar_t* vid_hudscale;

static int32_t m_main_cursor;

static bool update_sound_quality = false;
static bool search_local_games = false;

#define NUM_CURSOR_FRAMES 15

static char* menu_input_sound = "misc/menu1.wav";
static char* menu_move_sound = "misc/menu2.wav";
static char* menu_out_sound = "misc/menu3.wav";

void M_Menu_Main_f();
void M_Menu_Game_f();
void M_Menu_LoadGame_f();
void M_Menu_SaveGame_f();
void M_Menu_PlayerConfig_f();
void M_Menu_DownloadOptions_f();
void M_Menu_Credits_f(void);
void M_Menu_Multiplayer_f(void);
void M_Menu_JoinServer_f();
void M_Menu_AddressBook_f(void);
void M_Menu_StartServer_f();
void M_Menu_GameOptions_f();
void M_Menu_Video_f();
void M_Menu_Options_f();
void M_Menu_Keys_f();
void M_Menu_Quit_f();

bool m_entersound;		// play after drawing a frame, so caching
// won't disrupt the sound

void	(*m_drawfunc) ();
const char* (*m_keyfunc) (int32_t key);

//=============================================================================
/* Support Routines */

#define	MAX_MENU_DEPTH	8


typedef struct
{
	void	(*draw) ();
	const char* (*key) (int32_t k);
} menulayer_t;

menulayer_t	m_layers[MAX_MENU_DEPTH];
int32_t 	m_menudepth;

static void M_Banner(char* name)
{
	int32_t w, h;

	re.DrawGetPicSize(&w, &h, name);
	re.DrawPic(r_width->value / 2 - w / 2, r_height->value / 2 - 110 * vid_hudscale->value, name, NULL, false);
}

void M_PushMenu(void (*draw) (), const char* (*key) (int32_t k))
{
	int32_t 	i;

	if (Cvar_VariableValue("sv_maxclients") == 1
		&& Com_ServerState())
		Cvar_Set("paused", "1");

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i = 0; i < m_menudepth; i++)
		if (m_layers[i].draw == draw &&
			m_layers[i].key == key)
		{
			m_menudepth = i;
		}

	if (i == m_menudepth)
	{
		if (m_menudepth >= MAX_MENU_DEPTH)
			Com_Error(ERR_FATAL, "M_PushMenu: MAX_MENU_DEPTH");
		m_layers[m_menudepth].draw = m_drawfunc;
		m_layers[m_menudepth].key = m_keyfunc;
		m_menudepth++;
	}

	m_drawfunc = draw;
	m_keyfunc = key;

	m_entersound = true;

	cls.input_dest = key_menu;
}

void M_ForceMenuOff()
{
	m_drawfunc = 0;
	m_keyfunc = 0;
	cls.input_dest = key_game;
	m_menudepth = 0;
	Key_ClearStates();
	Cvar_Set("paused", "0");
}

void M_PopMenu()
{
	S_StartLocalSound(menu_out_sound);
	if (m_menudepth < 1)
		Com_Error(ERR_FATAL, "M_PopMenu: depth < 1");
	m_menudepth--;

	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;

	if (!m_menudepth)
		M_ForceMenuOff();
}


const char* Default_MenuKey(menuframework_t* m, int32_t key)
{
	const char* sound = NULL;
	menucommon_t* item;

	if (m)
	{
		if ((item = Menu_ItemAtCursor(m)) != 0)
		{
			if (item->type == MTYPE_FIELD)
			{
				if (Field_Key((menufield_t*)item, key))
					return NULL;
			}
		}
	}

	switch (key)
	{
	case K_ESCAPE:
		M_PopMenu();
		return menu_out_sound;
	case K_UPARROW:
		if (m)
		{
			m->cursor--;
			Menu_AdjustCursor(m, -1);
			sound = menu_move_sound;
		}
		break;
	case K_TAB:
		if (m)
		{
			m->cursor++;
			Menu_AdjustCursor(m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_DOWNARROW:
		if (m)
		{
			m->cursor++;
			Menu_AdjustCursor(m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_LEFTARROW:
		if (m)
		{
			Menu_SlideItem(m, -1);
			sound = menu_move_sound;
		}
		break;
	case K_RIGHTARROW:
		if (m)
		{
			Menu_SlideItem(m, 1);
			sound = menu_move_sound;
		}
		break;

	case K_MOUSE1:
	case K_MOUSE2:
	case K_MOUSE3:
	case K_MOUSE4:
	case K_MOUSE5:
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
	case K_AUX17:
	case K_AUX18:
	case K_AUX19:
	case K_AUX20:
	case K_AUX21:
	case K_AUX22:
	case K_AUX23:
	case K_AUX24:
	case K_AUX25:
	case K_AUX26:
	case K_AUX27:
	case K_AUX28:
	case K_AUX29:
	case K_AUX30:
	case K_AUX31:
	case K_AUX32:

	case K_KP_ENTER:
	case K_ENTER:
		if (m)
			Menu_SelectItem(m);
		sound = menu_move_sound;
		break;
	}

	return sound;
}

//=============================================================================

/*
================
Menu_DrawCenteredImage

Draws a centered graphics image.
cx and cy are in 320*240 coordinates, and will be centered on
higher res screens.
================
*/
void Menu_DrawCenteredImage(int32_t cx, int32_t cy, char* image)
{
	re.DrawPic(cx + (((int32_t)r_width->value - 320) >> 1) * vid_hudscale->value, cy + (((int32_t)r_height->value - 240) >> 1) * vid_hudscale->value, image, NULL, false);
}

/*
=============
M_DrawCursor

Draws an animating cursor with the point at
x,y.  The pic will extend to the left of x,
and both above and below y.
=============
*/
void M_DrawCursor(int32_t x, int32_t y, int32_t f)
{
	char	cursorname[80];
	static bool cached;

	if (!cached)
	{
		int32_t i;

		for (i = 0; i < NUM_CURSOR_FRAMES; i++)
		{
			Com_sprintf(cursorname, sizeof(cursorname), "2d/m_cursor%d", i);

			re.RegisterPic(cursorname);
		}
		cached = true;
	}

	Com_sprintf(cursorname, sizeof(cursorname), "2d/m_cursor%d", f);
	re.DrawPic(x, y, cursorname, NULL, false);
}

// uses 320*240 coords
void Menu_DrawTextBox(int32_t x, int32_t y, int32_t width, int32_t lines)
{
	int32_t 	cx, cy;
	int32_t 	n;

	// draw left side
	cx = x;
	cy = y;
	Menu_DrawCenteredImage(cx, cy, "2d/textbox_top_01");
	for (n = 0; n < lines; n++)
	{
		cy += 8 * vid_hudscale->value;
		Menu_DrawCenteredImage(cx, cy, "2d/textbox_middle_01");
	}
	Menu_DrawCenteredImage(cx, cy + 8 * vid_hudscale->value, "2d/textbox_bottom_01");

	// draw middle
	cx += 8 * vid_hudscale->value;
	while (width > 0)
	{
		cy = y;
		Menu_DrawCenteredImage(cx, cy, "2d/textbox_top_02");
		for (n = 0; n < lines; n++)
		{
			cy += 8 * vid_hudscale->value;
			Menu_DrawCenteredImage(cx, cy, "2d/textbox_middle_02");
		}
		Menu_DrawCenteredImage(cx, cy + 8 * vid_hudscale->value, "2d/textbox_bottom_02");
		width -= 1;
		cx += 8 * vid_hudscale->value;
	}

	// draw right side
	cy = y;
	Menu_DrawCenteredImage(cx, cy, "2d/textbox_top_03");
	for (n = 0; n < lines; n++)
	{
		cy += 8.0f * vid_hudscale->value;
		Menu_DrawCenteredImage(cx, cy, "2d/textbox_middle_03");
	}
	Menu_DrawCenteredImage(cx, cy + 8 * vid_hudscale->value, "2d/textbox_bottom_03");
}


/*
=======================================================================

MAIN MENU

=======================================================================
*/
#define	Main_ITEMS	5


void M_Main_Draw()
{
	int32_t i;
	int32_t w, h;
	int32_t ystart;
	int32_t xoffset;
	int32_t size_x = 0, size_y = 0;
	int32_t widest = -1;
	int32_t totalheight = 0;
	char litname[80];

	// menu options name
	char* names[] =
	{
		"2d/m_main_game",
		"2d/m_main_multiplayer",
		"2d/m_main_options",
		"2d/m_main_video",
		"2d/m_main_quit",
		0
	};

	for (i = 0; names[i] != 0; i++)
	{
		re.DrawGetPicSize(&w, &h, names[i]);

		if (w > widest)
			widest = w;
		totalheight += (h + 12);
	}

	ystart = (r_height->value / 2 - 100 * vid_hudscale->value);
	xoffset = (r_width->value - widest * vid_hudscale->value) / 2; // moved left by 70 pixels, because we got rid of the "QUAKE2" logo

	for (i = 0; names[i] != 0; i++)
	{
		if (i != m_main_cursor)
			re.DrawPic(xoffset, ystart + (i * 40 + 13) * vid_hudscale->value, names[i], NULL, false);
	}
	strcpy(litname, names[m_main_cursor]);
	strcat(litname, "_sel");
	re.DrawPic(xoffset, ystart + (m_main_cursor * 40 + 13) * vid_hudscale->value, litname, NULL, false);

	M_DrawCursor(xoffset - (25 * vid_hudscale->value), ystart + (m_main_cursor * 40 + 11) * vid_hudscale->value, (int32_t)(cls.realtime / 100) % NUM_CURSOR_FRAMES);

	const char* nav_text = "#[STRING_DEPRECATED_NAVIGATE]";
	Text_GetSize(cl_system_font->string, &size_x, &size_y, nav_text);
	// bad positioning hack
	Text_Draw(cl_system_font->string, xoffset - (size_x / 4) * vid_hudscale->value, ystart + (6 * 40 + 20) * vid_hudscale->value, nav_text);
}


const char* M_Main_Key(int32_t key)
{
	const char* sound = menu_move_sound;

	switch (key)
	{
	case K_ESCAPE:
		M_PopMenu();
		break;

	case K_DOWNARROW:
		if (++m_main_cursor >= Main_ITEMS)
			m_main_cursor = 0;
		return sound;

	case K_UPARROW:
		if (--m_main_cursor < 0)
			m_main_cursor = Main_ITEMS - 1;
		return sound;

	case K_KP_ENTER:
	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_Game_f();
			break;

		case 1:
			M_Menu_Multiplayer_f();
			break;

		case 2:
			M_Menu_Options_f();
			break;

		case 3:
			M_Menu_Video_f();
			break;

		case 4:
			M_Menu_Quit_f();
			break;
		}
	}

	return NULL;
}


void M_Menu_Main_f()
{

	M_PushMenu(M_Main_Draw, M_Main_Key);
}

/*
=======================================================================

MULTIPLAYER MENU

=======================================================================
*/
static menuframework_t	s_multiplayer_menu;
static menuaction_t		s_browse_servers_action;
static menuaction_t		s_direct_connect_action;
static menuaction_t		s_start_server_action;
static menuaction_t		s_player_setup_action;

static void Multiplayer_MenuDraw()
{
	M_Banner("2d/m_banner_multiplayer");

	Menu_AdjustCursor(&s_multiplayer_menu, 1);
	Menu_Draw(&s_multiplayer_menu);
}

static void PlayerSetupFunc(void* unused)
{
	M_Menu_PlayerConfig_f();
}

// This UI will only be implemented in the new UI system
static void BrowseServersFunc(void* unused)
{
}

static void DirectConnectFunc(void* unused)
{
	M_Menu_JoinServer_f();
}

static void StartNetworkServerFunc(void* unused)
{
	M_Menu_StartServer_f();
}

void Multiplayer_MenuInit(void)
{
	s_multiplayer_menu.x = r_width->value * 0.5f - 64 * vid_hudscale->value;
	s_multiplayer_menu.nitems = 0;

	s_browse_servers_action.generic.type = MTYPE_ACTION;
	s_browse_servers_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_browse_servers_action.generic.x = 0;
	s_browse_servers_action.generic.y = 0;
	s_browse_servers_action.generic.name = " [STRING_QUICKSTARTUI_BROWSESERVERS]";
	s_browse_servers_action.generic.callback = BrowseServersFunc;

	s_direct_connect_action.generic.type = MTYPE_ACTION;
	s_direct_connect_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_direct_connect_action.generic.x = 0;
	s_direct_connect_action.generic.y = 10 * vid_hudscale->value;
	s_direct_connect_action.generic.name = " [STRING_QUICKSTARTUI_DIRECTCONNECT]";
	s_direct_connect_action.generic.callback = DirectConnectFunc;

	s_start_server_action.generic.type = MTYPE_ACTION;
	s_start_server_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_start_server_action.generic.x = 0;
	s_start_server_action.generic.y = 20 * vid_hudscale->value;
	s_start_server_action.generic.name = " [STRING_QUICKSTARTUI_STARTSERVER]";
	s_start_server_action.generic.callback = StartNetworkServerFunc;

	s_player_setup_action.generic.type = MTYPE_ACTION;
	s_player_setup_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_player_setup_action.generic.x = 0;
	s_player_setup_action.generic.y = 30 * vid_hudscale->value;
	s_player_setup_action.generic.name = " [STRING_QUICKSTARTUI_PLAYERSETUP]";
	s_player_setup_action.generic.callback = PlayerSetupFunc;


	Menu_AddItem(&s_multiplayer_menu, (void*)&s_browse_servers_action);
	Menu_AddItem(&s_multiplayer_menu, (void*)&s_direct_connect_action);
	Menu_AddItem(&s_multiplayer_menu, (void*)&s_start_server_action);
	Menu_AddItem(&s_multiplayer_menu, (void*)&s_player_setup_action);

	Menu_SetStatusBar(&s_multiplayer_menu, NULL);

	Menu_Center(&s_multiplayer_menu);
}

const char* Multiplayer_MenuKey(int32_t key)
{
	return Default_MenuKey(&s_multiplayer_menu, key);
}

void M_Menu_Multiplayer_f(void)
{
	Multiplayer_MenuInit();
	M_PushMenu(Multiplayer_MenuDraw, Multiplayer_MenuKey);
}

/*
=======================================================================

KEYS MENU

=======================================================================
*/
char* bindnames[][2] =
{
{"+attack1", 		"^5[STRING_BINDING_ATTACK1]"},
{"+attack2",		"^5[STRING_BINDING_ATTACK2]"},
{"weapnext", 		"^5[STRING_BINDING_WEAPNEXT]"},
{"weapprev", 		"^5[STRING_BINDING_WEAPPREV]"},
{"weaplast", 		"^5[STRING_BINDING_WEAPLAST]"},
{"+forward", 		"^5[STRING_BINDING_FORWARD]"},
{"+back", 			"^5[STRING_BINDING_BACK]"},
{"+left", 			"^5[STRING_BINDING_LEFT]"},
{"+right", 			"^5[STRING_BINDING_RIGHT]"},
{"+speed", 			"^5[STRING_BINDING_SPEED]"},
{"+moveleft", 		"^5[STRING_BINDING_MOVELEFT]"},
{"+moveright", 		"^5[STRING_BINDING_MOVERIGHT]"},
{"+lookup", 		"^5[STRING_BINDING_LOOKUP]"},
{"+lookdown", 		"^5[STRING_BINDING_LOOKDOWN]"},
{"centerview", 		"^5[STRING_BINDING_CENTERVIEW]"},
{"+mlook", 			"^5[STRING_BINDING_MLOOK]"},
{"+klook", 			"^5[STRING_BINDING_KLOOK]"},
{"+moveup",			"^5[STRING_BINDING_MOVEUP]"},
{"+movedown",		"^5[STRING_BINDING_MOVEDOWN]"},

{"invuse",			"^5[STRING_BINDING_INVUSE]"},
{"invdrop",			"^5[STRING_BINDING_INVDROP]"},
{"invprev",			"^5[STRING_BINDING_INVPREV]"},
{"invnext",			"^5[STRING_BINDING_INVNEXT]"},
{ 0, 0 }
};

int32_t keys_cursor;
static int32_t bind_grab;

static menuframework_t	s_keys_menu;
static menuaction_t		s_keys_attack_action;
static menuaction_t		s_keys_change_weapon_action;
static menuaction_t		s_keys_change_weapon_prev_action;
static menuaction_t		s_keys_change_weapon_last_action;
static menuaction_t		s_keys_walk_forward_action;
static menuaction_t		s_keys_backpedal_action;
static menuaction_t		s_keys_turn_left_action;
static menuaction_t		s_keys_turn_right_action;
static menuaction_t		s_keys_run_action;
static menuaction_t		s_keys_step_left_action;
static menuaction_t		s_keys_step_right_action;
static menuaction_t		s_keys_sidestep_action;
static menuaction_t		s_keys_look_up_action;
static menuaction_t		s_keys_look_down_action;
static menuaction_t		s_keys_center_view_action;
static menuaction_t		s_keys_mouse_look_action;
static menuaction_t		s_keys_keyboard_look_action;
static menuaction_t		s_keys_move_up_action;
static menuaction_t		s_keys_move_down_action;
static menuaction_t		s_keys_inv_use_action;
static menuaction_t		s_keys_inv_drop_action;
static menuaction_t		s_keys_inv_prev_action;
static menuaction_t		s_keys_inv_next_action;

static void M_UnbindCommand(char* command)
{
	int32_t 	j;
	int32_t 	l;
	char* b;

	l = (int32_t)strlen(command);

	for (j = 0; j < NUM_KEYS; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp(b, command, l))
			Key_SetBinding(j, "");
	}
}

static void M_FindKeysForCommand(char* command, int32_t* twokeys)
{
	int32_t 	count;
	int32_t 	j;
	int32_t 	l;
	char* b;

	twokeys[0] = twokeys[1] = -1;
	l = (int32_t)strlen(command);
	count = 0;

	for (j = 0; j < NUM_KEYS; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp(b, command, l))
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

static void KeyCursorDrawFunc(menuframework_t* menu)
{
	if (bind_grab)
	{
		Text_DrawChar(cl_system_font->string, menu->x, menu->y + menu->cursor * 9 * vid_hudscale->value, '=');
	}
	else
	{
		if ((int32_t)(Sys_Milliseconds() / 250) & 1)
		{
			re.DrawPic(menu->x, menu->y + menu->cursor * 9 * vid_hudscale->value, "2d/menu_cursor_on", NULL, false);
		}
	}
}

#define NAME_ARRAY_SIZE		128

static void DrawKeyBindingFunc(void* self)
{
	int32_t keys[2];
	menuaction_t* a = (menuaction_t*)self;
	int32_t size_x = 0, size_y = 0;
	M_FindKeysForCommand(bindnames[a->generic.localdata[0]][0], keys);

	if (keys[0] == -1)
	{
		Text_Draw(cl_system_font->string, a->generic.x + a->generic.parent->x + 16 * vid_hudscale->value, a->generic.y + a->generic.parent->y, "^1???");
	}
	else
	{
		char name[NAME_ARRAY_SIZE];

		if (keys[1] != -1)
		{
			snprintf(name, NAME_ARRAY_SIZE, "^2%s^7 OR ^2%s", Key_VirtualToPhysical(keys[0], false), Key_VirtualToPhysical(keys[1], false));
		}
		else
		{
			snprintf(name, NAME_ARRAY_SIZE, "^2%s^7", Key_VirtualToPhysical(keys[0], false));
		}

		Text_GetSize(cl_system_font->string, &size_x, &size_y, name);
		Text_Draw(cl_system_font->string, a->generic.x + a->generic.parent->x + 16 * vid_hudscale->value, a->generic.y + a->generic.parent->y, name);
	}
}

static void KeyBindingFunc(void* self)
{
	menuaction_t* a = (menuaction_t*)self;
	int32_t keys[2];

	M_FindKeysForCommand(bindnames[a->generic.localdata[0]][0], keys);

	if (keys[1] != -1)
		M_UnbindCommand(bindnames[a->generic.localdata[0]][0]);

	bind_grab = true;

	Menu_SetStatusBar(&s_keys_menu, "[STRING_CONTROLSUI_HINT_SET]");
}

static void Keys_MenuInit(void)
{
	int32_t y = 0;
	int32_t i = 0;

	s_keys_menu.x = r_width->value * 0.50;
	s_keys_menu.nitems = 0;
	s_keys_menu.cursordraw = KeyCursorDrawFunc;

	s_keys_attack_action.generic.type = MTYPE_ACTION;
	s_keys_attack_action.generic.flags = QMF_GRAYED;
	s_keys_attack_action.generic.x = 0;
	s_keys_attack_action.generic.y = y * vid_hudscale->value;
	s_keys_attack_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_attack_action.generic.localdata[0] = i;
	s_keys_attack_action.generic.name = bindnames[s_keys_attack_action.generic.localdata[0]][1];

	s_keys_change_weapon_action.generic.type = MTYPE_ACTION;
	s_keys_change_weapon_action.generic.flags = QMF_GRAYED;
	s_keys_change_weapon_action.generic.x = 0;
	s_keys_change_weapon_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_change_weapon_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_change_weapon_action.generic.localdata[0] = ++i;
	s_keys_change_weapon_action.generic.name = bindnames[s_keys_change_weapon_action.generic.localdata[0]][1];

	s_keys_change_weapon_prev_action.generic.type = MTYPE_ACTION;
	s_keys_change_weapon_prev_action.generic.flags = QMF_GRAYED;
	s_keys_change_weapon_prev_action.generic.x = 0;
	s_keys_change_weapon_prev_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_change_weapon_prev_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_change_weapon_prev_action.generic.localdata[0] = ++i;
	s_keys_change_weapon_prev_action.generic.name = bindnames[s_keys_change_weapon_prev_action.generic.localdata[0]][1];

	s_keys_change_weapon_last_action.generic.type = MTYPE_ACTION;
	s_keys_change_weapon_last_action.generic.flags = QMF_GRAYED;
	s_keys_change_weapon_last_action.generic.x = 0;
	s_keys_change_weapon_last_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_change_weapon_last_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_change_weapon_last_action.generic.localdata[0] = ++i;
	s_keys_change_weapon_last_action.generic.name = bindnames[s_keys_change_weapon_last_action.generic.localdata[0]][1];

	s_keys_walk_forward_action.generic.type = MTYPE_ACTION;
	s_keys_walk_forward_action.generic.flags = QMF_GRAYED;
	s_keys_walk_forward_action.generic.x = 0;
	s_keys_walk_forward_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_walk_forward_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_walk_forward_action.generic.localdata[0] = ++i;
	s_keys_walk_forward_action.generic.name = bindnames[s_keys_walk_forward_action.generic.localdata[0]][1];

	s_keys_backpedal_action.generic.type = MTYPE_ACTION;
	s_keys_backpedal_action.generic.flags = QMF_GRAYED;
	s_keys_backpedal_action.generic.x = 0;
	s_keys_backpedal_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_backpedal_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_backpedal_action.generic.localdata[0] = ++i;
	s_keys_backpedal_action.generic.name = bindnames[s_keys_backpedal_action.generic.localdata[0]][1];

	s_keys_turn_left_action.generic.type = MTYPE_ACTION;
	s_keys_turn_left_action.generic.flags = QMF_GRAYED;
	s_keys_turn_left_action.generic.x = 0;
	s_keys_turn_left_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_turn_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_left_action.generic.localdata[0] = ++i;
	s_keys_turn_left_action.generic.name = bindnames[s_keys_turn_left_action.generic.localdata[0]][1];

	s_keys_turn_right_action.generic.type = MTYPE_ACTION;
	s_keys_turn_right_action.generic.flags = QMF_GRAYED;
	s_keys_turn_right_action.generic.x = 0;
	s_keys_turn_right_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_turn_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_right_action.generic.localdata[0] = ++i;
	s_keys_turn_right_action.generic.name = bindnames[s_keys_turn_right_action.generic.localdata[0]][1];

	s_keys_run_action.generic.type = MTYPE_ACTION;
	s_keys_run_action.generic.flags = QMF_GRAYED;
	s_keys_run_action.generic.x = 0;
	s_keys_run_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_run_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_run_action.generic.localdata[0] = ++i;
	s_keys_run_action.generic.name = bindnames[s_keys_run_action.generic.localdata[0]][1];

	s_keys_step_left_action.generic.type = MTYPE_ACTION;
	s_keys_step_left_action.generic.flags = QMF_GRAYED;
	s_keys_step_left_action.generic.x = 0;
	s_keys_step_left_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_step_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_left_action.generic.localdata[0] = ++i;
	s_keys_step_left_action.generic.name = bindnames[s_keys_step_left_action.generic.localdata[0]][1];

	s_keys_step_right_action.generic.type = MTYPE_ACTION;
	s_keys_step_right_action.generic.flags = QMF_GRAYED;
	s_keys_step_right_action.generic.x = 0;
	s_keys_step_right_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_step_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_right_action.generic.localdata[0] = ++i;
	s_keys_step_right_action.generic.name = bindnames[s_keys_step_right_action.generic.localdata[0]][1];

	s_keys_sidestep_action.generic.type = MTYPE_ACTION;
	s_keys_sidestep_action.generic.flags = QMF_GRAYED;
	s_keys_sidestep_action.generic.x = 0;
	s_keys_sidestep_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_sidestep_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_sidestep_action.generic.localdata[0] = ++i;
	s_keys_sidestep_action.generic.name = bindnames[s_keys_sidestep_action.generic.localdata[0]][1];

	s_keys_look_up_action.generic.type = MTYPE_ACTION;
	s_keys_look_up_action.generic.flags = QMF_GRAYED;
	s_keys_look_up_action.generic.x = 0;
	s_keys_look_up_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_look_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_up_action.generic.localdata[0] = ++i;
	s_keys_look_up_action.generic.name = bindnames[s_keys_look_up_action.generic.localdata[0]][1];

	s_keys_look_down_action.generic.type = MTYPE_ACTION;
	s_keys_look_down_action.generic.flags = QMF_GRAYED;
	s_keys_look_down_action.generic.x = 0;
	s_keys_look_down_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_look_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_down_action.generic.localdata[0] = ++i;
	s_keys_look_down_action.generic.name = bindnames[s_keys_look_down_action.generic.localdata[0]][1];

	s_keys_center_view_action.generic.type = MTYPE_ACTION;
	s_keys_center_view_action.generic.flags = QMF_GRAYED;
	s_keys_center_view_action.generic.x = 0;
	s_keys_center_view_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_center_view_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_center_view_action.generic.localdata[0] = ++i;
	s_keys_center_view_action.generic.name = bindnames[s_keys_center_view_action.generic.localdata[0]][1];

	s_keys_mouse_look_action.generic.type = MTYPE_ACTION;
	s_keys_mouse_look_action.generic.flags = QMF_GRAYED;
	s_keys_mouse_look_action.generic.x = 0;
	s_keys_mouse_look_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_mouse_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_mouse_look_action.generic.localdata[0] = ++i;
	s_keys_mouse_look_action.generic.name = bindnames[s_keys_mouse_look_action.generic.localdata[0]][1];

	s_keys_keyboard_look_action.generic.type = MTYPE_ACTION;
	s_keys_keyboard_look_action.generic.flags = QMF_GRAYED;
	s_keys_keyboard_look_action.generic.x = 0;
	s_keys_keyboard_look_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_keyboard_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_keyboard_look_action.generic.localdata[0] = ++i;
	s_keys_keyboard_look_action.generic.name = bindnames[s_keys_keyboard_look_action.generic.localdata[0]][1];

	s_keys_move_up_action.generic.type = MTYPE_ACTION;
	s_keys_move_up_action.generic.flags = QMF_GRAYED;
	s_keys_move_up_action.generic.x = 0;
	s_keys_move_up_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_move_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_up_action.generic.localdata[0] = ++i;
	s_keys_move_up_action.generic.name = bindnames[s_keys_move_up_action.generic.localdata[0]][1];

	s_keys_move_down_action.generic.type = MTYPE_ACTION;
	s_keys_move_down_action.generic.flags = QMF_GRAYED;
	s_keys_move_down_action.generic.x = 0;
	s_keys_move_down_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_move_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_down_action.generic.localdata[0] = ++i;
	s_keys_move_down_action.generic.name = bindnames[s_keys_move_down_action.generic.localdata[0]][1];

	s_keys_inv_use_action.generic.type = MTYPE_ACTION;
	s_keys_inv_use_action.generic.flags = QMF_GRAYED;
	s_keys_inv_use_action.generic.x = 0;
	s_keys_inv_use_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_inv_use_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_use_action.generic.localdata[0] = ++i;
	s_keys_inv_use_action.generic.name = bindnames[s_keys_inv_use_action.generic.localdata[0]][1];

	s_keys_inv_drop_action.generic.type = MTYPE_ACTION;
	s_keys_inv_drop_action.generic.flags = QMF_GRAYED;
	s_keys_inv_drop_action.generic.x = 0;
	s_keys_inv_drop_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_inv_drop_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_drop_action.generic.localdata[0] = ++i;
	s_keys_inv_drop_action.generic.name = bindnames[s_keys_inv_drop_action.generic.localdata[0]][1];

	s_keys_inv_prev_action.generic.type = MTYPE_ACTION;
	s_keys_inv_prev_action.generic.flags = QMF_GRAYED;
	s_keys_inv_prev_action.generic.x = 0;
	s_keys_inv_prev_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_inv_prev_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_prev_action.generic.localdata[0] = ++i;
	s_keys_inv_prev_action.generic.name = bindnames[s_keys_inv_prev_action.generic.localdata[0]][1];

	s_keys_inv_next_action.generic.type = MTYPE_ACTION;
	s_keys_inv_next_action.generic.flags = QMF_GRAYED;
	s_keys_inv_next_action.generic.x = 0;
	s_keys_inv_next_action.generic.y = y += 9 * vid_hudscale->value;
	s_keys_inv_next_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_next_action.generic.localdata[0] = ++i;
	s_keys_inv_next_action.generic.name = bindnames[s_keys_inv_next_action.generic.localdata[0]][1];

	Menu_AddItem(&s_keys_menu, (void*)&s_keys_attack_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_change_weapon_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_change_weapon_prev_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_change_weapon_last_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_walk_forward_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_backpedal_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_turn_left_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_turn_right_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_run_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_step_left_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_step_right_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_sidestep_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_look_up_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_look_down_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_center_view_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_mouse_look_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_keyboard_look_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_move_up_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_move_down_action);

	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inv_use_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inv_drop_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inv_prev_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inv_next_action);

	Menu_SetStatusBar(&s_keys_menu, "[STRING_CONTROLSUI_HINT_CLEAR]");
	Menu_Center(&s_keys_menu);
}

static void Keys_MenuDraw()
{
	Menu_AdjustCursor(&s_keys_menu, 1);
	Menu_Draw(&s_keys_menu);
}

static const char* Keys_MenuKey(int32_t key)
{
	menuaction_t* item = (menuaction_t*)Menu_ItemAtCursor(&s_keys_menu);

	if (bind_grab)
	{
		if (key != K_ESCAPE && key != '`')
		{
			char cmd[1024];

			Com_sprintf(cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_VirtualToPhysical(key, false), bindnames[item->generic.localdata[0]][0]);
			Cbuf_InsertText(cmd);
		}

		Menu_SetStatusBar(&s_keys_menu, "[STRING_CONTROLSUI_HINT_CLEAR]");
		bind_grab = false;
		return menu_out_sound;
	}

	switch (key)
	{
	case K_KP_ENTER:
	case K_ENTER:
		KeyBindingFunc(item);
		return menu_input_sound;
	case K_BACKSPACE:		// delete bindings
	case K_DELETE:				// delete bindings
		M_UnbindCommand(bindnames[item->generic.localdata[0]][0]);
		return menu_out_sound;
	default:
		return Default_MenuKey(&s_keys_menu, key);
	}
}

void M_Menu_Keys_f()
{
	Keys_MenuInit();
	M_PushMenu(Keys_MenuDraw, Keys_MenuKey);
}


/*
=======================================================================

CONTROLS MENU

=======================================================================
*/

static menuframework_t	s_options_menu;
static menuaction_t		s_options_defaults_action;
static menuaction_t		s_options_customize_options_action;
static menuslider_t		s_options_sensitivity_slider;
static menulist_t		s_options_freelook_box;
static menulist_t		s_options_alwaysrun_box;
static menulist_t		s_options_invertmouse_box;
static menulist_t		s_options_lookspring_box;
static menulist_t		s_options_lookstrafe_box;
static menulist_t		s_options_crosshair_box;
static menuslider_t		s_options_sfxvolume_slider;
static menuslider_t		s_options_musicvolume_slider;
static menulist_t		s_options_joystick_box;
static menulist_t		s_options_quality_list;
static menulist_t		s_options_compatibility_list;
static menulist_t		s_options_console_action;

static void CrosshairFunc(void* unused)
{
	Cvar_SetValue("crosshair", s_options_crosshair_box.curvalue);
}

static void CustomizeControlsFunc(void* unused)
{
	M_Menu_Keys_f();
}

static void AlwaysRunFunc(void* unused)
{
	Cvar_SetValue("cl_run", s_options_alwaysrun_box.curvalue);
}

static void FreeLookFunc(void* unused)
{
	Cvar_SetValue("freelook", s_options_freelook_box.curvalue);
}

static void MouseSensitivityFunc(void* unused)
{
	Cvar_SetValue("sensitivity", s_options_sensitivity_slider.curvalue / 2.0F);
}

static float ClampCvar(float min, float max, float value)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

static void ControlsSetMenuItemValues(void)
{
	s_options_sfxvolume_slider.curvalue = Cvar_VariableValue("s_volume_sfx") * 10;
	s_options_musicvolume_slider.curvalue = Cvar_VariableValue("s_volume_music") * 10;
	s_options_quality_list.curvalue = !Cvar_VariableValue("s_loadas8bit");
	s_options_sensitivity_slider.curvalue = (sensitivity->value) * 2;

	Cvar_SetValue("cl_run", ClampCvar(0, 1, cl_run->value));
	s_options_alwaysrun_box.curvalue = cl_run->value;

	s_options_invertmouse_box.curvalue = m_pitch->value < 0;

	Cvar_SetValue("lookspring", ClampCvar(0, 1, lookspring->value));
	s_options_lookspring_box.curvalue = lookspring->value;

	Cvar_SetValue("lookstrafe", ClampCvar(0, 1, lookstrafe->value));
	s_options_lookstrafe_box.curvalue = lookstrafe->value;

	Cvar_SetValue("freelook", ClampCvar(0, 1, freelook->value));
	s_options_freelook_box.curvalue = freelook->value;

	Cvar_SetValue("crosshair", ClampCvar(0, 3, crosshair->value));
	s_options_crosshair_box.curvalue = crosshair->value;
}

static void ControlsResetDefaultsFunc(void* unused)
{
	Cbuf_AddText("exec default.cfg\n");
	Cbuf_Execute();

	ControlsSetMenuItemValues();
}

static void InvertMouseFunc(void* unused)
{
	Cvar_SetValue("m_pitch", -m_pitch->value);
}

static void LookspringFunc(void* unused)
{
	Cvar_SetValue("lookspring", !lookspring->value);
}

static void LookstrafeFunc(void* unused)
{
	Cvar_SetValue("lookstrafe", !lookstrafe->value);
}

static void UpdateVolumeFunc(void* unused)
{
	Cvar_SetValue("s_volume_sfx", s_options_sfxvolume_slider.curvalue / 10);
}

static void UpdateCDVolumeFunc(void* unused)
{
	Cvar_SetValue("s_volume_music", s_options_musicvolume_slider.curvalue / 10);
}

static void ConsoleFunc(void* unused)
{
	/*
	** the proper way to do this is probably to have ToggleConsole_f accept a parameter
	*/
	extern void Key_ClearTyping(void);

	if (cl.attractloop)
	{
		Cbuf_AddText("killserver\n");
		return;
	}

	Key_ClearTyping();
	Con_ClearNotify();

	M_ForceMenuOff();
	cls.input_dest = key_console;
}

static void UpdateSoundQualityFunc(void* unused)
{
	if (s_options_quality_list.curvalue)
	{
		Cvar_SetValue("s_khz", 22);
		Cvar_SetValue("s_loadas8bit", false);
	}
	else
	{
		Cvar_SetValue("s_khz", 11);
		Cvar_SetValue("s_loadas8bit", true);
	}

	Cvar_SetValue("s_primary", s_options_compatibility_list.curvalue);

	update_sound_quality = true;
}

void Options_MenuInit(void)
{
	static const char* quality_items[] =
	{
		"[STRING_LOW]", 
		"[STRING_HIGH]",
		0
	};

	static const char* compatibility_items[] =
	{
		"[STRING_SETTINGSUI_SOUNDCOMPATIBILITY_COMPATIBILITY]",
		"[STRING_SETTINGSUI_SOUNDCOMPATIBILITY_PERFORMANCE]",
		0
	};

	static const char* yes_no_names[] =
	{
		"^1[STRING_NO]",
		"^2[STRING_YES]",
		0
	};

	static const char* crosshair_names[] =
	{
		"[STRING_NONE]",
		"[STRING_SETTINGSUI_CROSSHAIR1]",
		"[STRING_SETTINGSUI_CROSSHAIR2]",
		"[STRING_SETTINGSUI_CROSSHAIR3]",
		0
	};

	/*
	** configure controls menu and menu items
	*/
	s_options_menu.x = r_width->value / 2;
	s_options_menu.y = r_height->value / 2 - 58 * vid_hudscale->value;
	s_options_menu.nitems = 0;

	s_options_sfxvolume_slider.generic.type = MTYPE_SLIDER;
	s_options_sfxvolume_slider.generic.x = 0;
	s_options_sfxvolume_slider.generic.y = 0;
	s_options_sfxvolume_slider.generic.name = "^5[STRING_SETTINGSUI_SFXVOLUME]";
	s_options_sfxvolume_slider.generic.callback = UpdateVolumeFunc;
	s_options_sfxvolume_slider.minvalue = 0;
	s_options_sfxvolume_slider.maxvalue = 10;
	s_options_sfxvolume_slider.curvalue = Cvar_VariableValue("s_volume_sfx") * 10;

	s_options_musicvolume_slider.generic.type = MTYPE_SLIDER;
	s_options_musicvolume_slider.generic.x = 0;
	s_options_musicvolume_slider.generic.y = 10 * vid_hudscale->value;
	s_options_musicvolume_slider.generic.name = "^5[STRING_SETTINGSUI_MUSICVOLUME]";
	s_options_musicvolume_slider.generic.callback = UpdateCDVolumeFunc;
	s_options_musicvolume_slider.minvalue = 0;
	s_options_musicvolume_slider.maxvalue = 10;
	s_options_musicvolume_slider.curvalue = Cvar_VariableValue("s_volume_music") * 10;

	s_options_quality_list.generic.type = MTYPE_SPINCONTROL;
	s_options_quality_list.generic.x = 0;
	s_options_quality_list.generic.y = 20 * vid_hudscale->value;
	s_options_quality_list.generic.name = "^5[STRING_SETTINGSUI_SOUNDQUALITY]";
	s_options_quality_list.generic.callback = UpdateSoundQualityFunc;
	s_options_quality_list.itemnames = quality_items;
	s_options_quality_list.curvalue = !Cvar_VariableValue("s_loadas8bit");

	s_options_compatibility_list.generic.type = MTYPE_SPINCONTROL;
	s_options_compatibility_list.generic.x = 0;
	s_options_compatibility_list.generic.y = 30 * vid_hudscale->value;
	s_options_compatibility_list.generic.name = "^5[STRING_SETTINGSUI_SOUNDCOMPATIBILITY]";
	s_options_compatibility_list.generic.callback = UpdateSoundQualityFunc;
	s_options_compatibility_list.itemnames = compatibility_items;
	s_options_compatibility_list.curvalue = Cvar_VariableValue("s_primary");

	s_options_sensitivity_slider.generic.type = MTYPE_SLIDER;
	s_options_sensitivity_slider.generic.x = 0;
	s_options_sensitivity_slider.generic.y = 50 * vid_hudscale->value;
	s_options_sensitivity_slider.generic.name = "^5[STRING_SETTINGSUI_MOUSESENSITIVITY]";
	s_options_sensitivity_slider.generic.callback = MouseSensitivityFunc;
	s_options_sensitivity_slider.minvalue = 2;
	s_options_sensitivity_slider.maxvalue = 22;

	s_options_alwaysrun_box.generic.type = MTYPE_SPINCONTROL;
	s_options_alwaysrun_box.generic.x = 0;
	s_options_alwaysrun_box.generic.y = 60 * vid_hudscale->value;
	s_options_alwaysrun_box.generic.name = "^5[STRING_SETTINGSUI_ALWAYSRUN]";
	s_options_alwaysrun_box.generic.callback = AlwaysRunFunc;
	s_options_alwaysrun_box.itemnames = yes_no_names;

	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x = 0;
	s_options_invertmouse_box.generic.y = 70 * vid_hudscale->value;
	s_options_invertmouse_box.generic.name = "^5[STRING_SETTINGSUI_INVERTMOUSE]";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yes_no_names;

	s_options_lookspring_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookspring_box.generic.x = 0;
	s_options_lookspring_box.generic.y = 80 * vid_hudscale->value;
	s_options_lookspring_box.generic.name = "^5[STRING_SETTINGSUI_MOUSESENSITIVITY]";
	s_options_lookspring_box.generic.callback = LookspringFunc;
	s_options_lookspring_box.itemnames = yes_no_names;

	s_options_lookstrafe_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookstrafe_box.generic.x = 0;
	s_options_lookstrafe_box.generic.y = 90 * vid_hudscale->value;
	s_options_lookstrafe_box.generic.name = "^5[STRING_SETTINGSUI_LOOKSTRAFE]";
	s_options_lookstrafe_box.generic.callback = LookstrafeFunc;
	s_options_lookstrafe_box.itemnames = yes_no_names;

	s_options_freelook_box.generic.type = MTYPE_SPINCONTROL;
	s_options_freelook_box.generic.x = 0;
	s_options_freelook_box.generic.y = 100 * vid_hudscale->value;
	s_options_freelook_box.generic.name = "^5[STRING_SETTINGSUI_FREELOOK]";
	s_options_freelook_box.generic.callback = FreeLookFunc;
	s_options_freelook_box.itemnames = yes_no_names;

	s_options_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_options_crosshair_box.generic.x = 0;
	s_options_crosshair_box.generic.y = 110 * vid_hudscale->value;
	s_options_crosshair_box.generic.name = "^5[STRING_SETTINGSUI_CROSSHAIR]";
	s_options_crosshair_box.generic.callback = CrosshairFunc;
	s_options_crosshair_box.itemnames = crosshair_names;

	s_options_customize_options_action.generic.type = MTYPE_ACTION;
	s_options_customize_options_action.generic.x = 0;
	s_options_customize_options_action.generic.y = 140 * vid_hudscale->value;
	s_options_customize_options_action.generic.name = "^3[STRING_SETTINGSUI_CUSTOMISECONTROLS]";
	s_options_customize_options_action.generic.callback = CustomizeControlsFunc;

	s_options_defaults_action.generic.type = MTYPE_ACTION;
	s_options_defaults_action.generic.x = 0;
	s_options_defaults_action.generic.y = 150 * vid_hudscale->value;
	s_options_defaults_action.generic.name = "^3[STRING_SETTINGSUI_RESET]";
	s_options_defaults_action.generic.callback = ControlsResetDefaultsFunc;

	s_options_console_action.generic.type = MTYPE_ACTION;
	s_options_console_action.generic.x = 0;
	s_options_console_action.generic.y = 160 * vid_hudscale->value;
	s_options_console_action.generic.name = "^3[STRING_SETTINGSUI_GOTOCONSOLE]";
	s_options_console_action.generic.callback = ConsoleFunc;

	ControlsSetMenuItemValues();

	Menu_AddItem(&s_options_menu, (void*)&s_options_sfxvolume_slider);
	Menu_AddItem(&s_options_menu, (void*)&s_options_musicvolume_slider);
	Menu_AddItem(&s_options_menu, (void*)&s_options_quality_list);
	Menu_AddItem(&s_options_menu, (void*)&s_options_compatibility_list);
	Menu_AddItem(&s_options_menu, (void*)&s_options_sensitivity_slider);
	Menu_AddItem(&s_options_menu, (void*)&s_options_alwaysrun_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_invertmouse_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_lookspring_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_lookstrafe_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_freelook_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_crosshair_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_customize_options_action);
	Menu_AddItem(&s_options_menu, (void*)&s_options_defaults_action);
	Menu_AddItem(&s_options_menu, (void*)&s_options_console_action);
}

void Options_MenuDraw()
{
	M_Banner("2d/m_banner_options");
	Menu_AdjustCursor(&s_options_menu, 1);
	Menu_Draw(&s_options_menu);
	font_t* system_font_ptr = Font_GetByName(cl_system_font->string); // get pointer to system font

	if (update_sound_quality)
	{
		int32_t y = 120 - 48 * vid_hudscale->value;

		Menu_DrawTextBox(8 - 152 * (vid_hudscale->value - 1.f), 120 - 48 * vid_hudscale->value, 36, 3);
		y += (system_font_ptr->line_height * vid_hudscale->value);
		Text_Draw(cl_system_font->string, 32 - 136 * (vid_hudscale->value - 1.f), y, "[STRING_SETTINGSUI_RESTARTINGSOUND1]");
		y += (system_font_ptr->line_height * vid_hudscale->value);
		Text_Draw(cl_system_font->string, 32 - 136 * (vid_hudscale->value - 1.f), y, "[STRING_SETTINGSUI_RESTARTINGSOUND2]");
		y += (system_font_ptr->line_height * vid_hudscale->value);
		Text_Draw(cl_system_font->string, 32 - 136 * (vid_hudscale->value - 1.f), y, "[STRING_SETTINGSUI_RESTARTINGSOUND3]");

		CL_Snd_Restart_f();
		update_sound_quality = false;
	}
}

const char* Options_MenuKey(int32_t key)
{
	return Default_MenuKey(&s_options_menu, key);
}

void M_Menu_Options_f()
{
	Options_MenuInit();
	M_PushMenu(Options_MenuDraw, Options_MenuKey);
}

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

void M_Menu_Video_f()
{
	Vid_MenuInit();
	M_PushMenu(Vid_MenuDraw, Vid_MenuKey);
}

/*
=============================================================================

END GAME MENU

=============================================================================
*/
static int32_t credits_start_time;
static const char** credits;
static char* creditsIndex[256];
static char* creditsBuffer;
//todo: localisable
static const char* creditstext[] =
{
	"^dZ O M B O N O ^3  C R E D I T S"
	"",
	"",
	"^5PROGRAMMING^7",
	"",
	"Connor Hyde (starfrost) (Digital Euphoria???)",
	"",
	"^5MUSIC^7",
	"",
	"'Stephanie Peterson, LLC' / zorg_is_also_here / sirZorg",
	"",
	"^5MODELLING^7",
	"",
	"JV_726 (Ventus3D) (https://www.behance.net/Ventus3D)",
	"",
	"^5TOOLS^7",
	"",
	"TrenchBroom ^5Team^7",
	"^5ZBSP^7 based off of Qbism - Qbism ^5Team^7",
	"Krzysztof Kondrak ^5(vkquake2)^7",
	"^5id^7 Software ^5(Original Quake 2 engine)^7",
	"unsubtract ^5(mkpak)^7",
	"",
	"",
	"^5TESTERS^7",
	"NotnHeavy",
	"UNNAMO",
	"unsubtract",
	"Hugo5551",
	"goopy goob",
	"Samw220506",
	"solarlight",
	"CTriggerHurt",
	"how2rot",
	"PokeFan919",
	"ExperiencersInternational",
	"'merican boot",
	"Adrian",
	"didimmick",
	"pivotman319",
	"kino",
	"UnknownPolish",	
	"",
	"",
	"^5Game software (C) 2023-2024 starfrost^7",
	"",
	"Based on ^5vkQuake2^7",
	"Original code (C) 1997-2001 ^5id Software^7",
	"(C) 2018-2023 ^5Krzysztof Kondrak^7",
	"",
	"See licensing file for licensing information",
	"Open-source on GitHub!",
	"",
	"^3Thanks for playing!",
	0
};

void M_Credits_MenuDraw(void)
{
	int32_t i, y;

	/*
	** draw the credits
	*/
	for (i = 0, y = r_height->value - ((cls.realtime - credits_start_time) / 40.0F);
		credits[i] && y < r_height->value * vid_hudscale->value; y += 10 * vid_hudscale->value, i++)
	{
		int32_t bold = false;

		if (y <= -8 * vid_hudscale->value)
			continue;

		int32_t x;
		int32_t size_x = 0, size_y = 0;

		Text_GetSize(cl_system_font->string, &size_x, &size_y, credits[i]);
		x = (r_width->value / 2) - (size_x / 2); 
		Text_Draw(cl_system_font->string, x, y, credits[i]);
	}

	if (y < 0)
		credits_start_time = cls.realtime;
}

const char* M_Credits_Key(int32_t key)
{
	switch (key)
	{
	case K_ESCAPE:
		if (creditsBuffer)
			FS_FreeFile(creditsBuffer);
		M_PopMenu();
		break;
	}

	return menu_out_sound;

}

void M_Menu_Credits_f(void)
{
	int32_t 	n;
	int32_t 	count;
	char* p;

	creditsBuffer = NULL;
	count = FS_LoadFile("credits", (void**)&creditsBuffer);
	if (count != -1)
	{
		p = creditsBuffer;
		for (n = 0; n < 255; n++)
		{
			creditsIndex[n] = p;
			while (*p != '\r' && *p != '\n')
			{
				p++;
				if (--count == 0)
					break;
			}
			if (*p == '\r')
			{
				*p++ = 0;
				if (--count == 0)
					break;
			}
			*p++ = 0;
			if (--count == 0)
				break;
		}
		creditsIndex[++n] = 0;
		credits = (const char**)creditsIndex;
	}
	else
	{
		// load default credits
		credits = creditstext;
	}

	credits_start_time = cls.realtime;
	M_PushMenu(M_Credits_MenuDraw, M_Credits_Key);
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

static int32_t 	m_game_cursor;

static menuframework_t	s_game_menu;
static menuaction_t		s_easy_game_action;
static menuaction_t		s_medium_game_action;
static menuaction_t		s_hard_game_action;
static menuaction_t		s_hard_plus_game_action;
static menuaction_t		s_load_game_action;
static menuaction_t		s_save_game_action;
static menuaction_t		s_credits_action;
static menuseparator_t	s_blankline;

static void StartGame(void)
{
	// disable updates
	cl.servercount = -1;
	M_ForceMenuOff();

	// TODO: REMOVE
	Cvar_SetValue("gamemode", 0);

	Cbuf_AddText("loading ; killserver ; wait ; newgame\n");
	cls.input_dest = key_game;
}

static void EasyGameFunc(void* data)
{
	Cvar_ForceSet("skill", "0");
	StartGame();
}

static void MediumGameFunc(void* data)
{
	Cvar_ForceSet("skill", "1");
	StartGame();
}

static void HardGameFunc(void* data)
{
	Cvar_ForceSet("skill", "2");
	StartGame();
}

static void HardPlusGameFunc(void* data)
{
	Cvar_ForceSet("skill", "3");
	StartGame();
}

static void LoadGameFunc(void* unused)
{
	M_Menu_LoadGame_f();
}

static void SaveGameFunc(void* unused)
{
	M_Menu_SaveGame_f();
}

static void CreditsFunc(void* unused)
{
	M_Menu_Credits_f();
}

void Game_MenuInit(void)
{
	s_game_menu.x = r_width->value * 0.50;
	s_game_menu.nitems = 0;

	s_easy_game_action.generic.type = MTYPE_ACTION;
	s_easy_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_easy_game_action.generic.x = 0;
	s_easy_game_action.generic.y = 0;
	s_easy_game_action.generic.name = "[STRING_SKILL0]";
	s_easy_game_action.generic.callback = EasyGameFunc;

	s_medium_game_action.generic.type = MTYPE_ACTION;
	s_medium_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_medium_game_action.generic.x = 0;
	s_medium_game_action.generic.y = 10 * vid_hudscale->value;
	s_medium_game_action.generic.name = "[STRING_SKILL1]";
	s_medium_game_action.generic.callback = MediumGameFunc;

	s_hard_game_action.generic.type = MTYPE_ACTION;
	s_hard_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_hard_game_action.generic.x = 0;
	s_hard_game_action.generic.y = 20 * vid_hudscale->value;
	s_hard_game_action.generic.name = "[STRING_SKILL2]";
	s_hard_game_action.generic.callback = HardGameFunc;

	s_hard_plus_game_action.generic.type = MTYPE_ACTION;
	s_hard_plus_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_hard_plus_game_action.generic.x = 0;
	s_hard_plus_game_action.generic.y = 30 * vid_hudscale->value;
	s_hard_plus_game_action.generic.name = "[STRING_SKILL3]"; 
	s_hard_plus_game_action.generic.callback = HardPlusGameFunc;

	s_blankline.generic.type = MTYPE_SEPARATOR;

	s_load_game_action.generic.type = MTYPE_ACTION;
	s_load_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_load_game_action.generic.x = 0;
	s_load_game_action.generic.y = 50 * vid_hudscale->value;
	s_load_game_action.generic.name = "[STRING_LOAD_GAME]";
	s_load_game_action.generic.callback = LoadGameFunc;

	s_save_game_action.generic.type = MTYPE_ACTION;
	s_save_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_save_game_action.generic.x = 0;
	s_save_game_action.generic.y = 60 * vid_hudscale->value;
	s_save_game_action.generic.name = "[STRING_SAVE_GAME]";
	s_save_game_action.generic.callback = SaveGameFunc;

	s_credits_action.generic.type = MTYPE_ACTION;
	s_credits_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_credits_action.generic.x = 0;
	s_credits_action.generic.y = 70 * vid_hudscale->value;
	s_credits_action.generic.name = "[STRING_CREDITS]";
	s_credits_action.generic.callback = CreditsFunc;

	Menu_AddItem(&s_game_menu, (void*)&s_easy_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_medium_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_hard_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_hard_plus_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_blankline);
	Menu_AddItem(&s_game_menu, (void*)&s_load_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_save_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_blankline);
	Menu_AddItem(&s_game_menu, (void*)&s_credits_action);

	Menu_Center(&s_game_menu);
}

void Game_MenuDraw(void)
{
#ifdef PLAYTEST
	font_t* system_font_ptr = Font_GetByName(cl_system_font->string);
	int32_t size_x = 0, size_y = 0;
	const char* no_playtest_text = "[STRING_ERROR_NO_SERVER_PLAYTEST]";
	Text_GetSize(cl_system_font->string, &size_x, &size_y, no_playtest_text);
	Text_Draw(cl_system_font->string, r_width->value / 2 - (size_x / 2), r_height->value / 2 - (92 * vid_hudscale->value), no_playtest_text);
#else
	M_Banner("2d/m_banner_game");
	Menu_AdjustCursor(&s_game_menu, 1);
	Menu_Draw(&s_game_menu);
#endif
}

const char* Game_MenuKey(int32_t key)
{
#ifdef PLAYTEST
	M_PopMenu();
#else
	return Default_MenuKey(&s_game_menu, key);
#endif
	return 0x00;
}

void M_Menu_Game_f()
{
	Game_MenuInit();
	M_PushMenu(Game_MenuDraw, Game_MenuKey);
	m_game_cursor = 1;
}

/*
=============================================================================

LOADGAME MENU

=============================================================================
*/

#define	MAX_SAVEGAMES	15

static menuframework_t	s_savegame_menu;

static menuframework_t	s_loadgame_menu;
static menuaction_t		s_loadgame_actions[MAX_SAVEGAMES];

char		m_savestrings[MAX_SAVEGAMES][32];
bool	m_savevalid[MAX_SAVEGAMES];

void Create_Savestrings()
{
	int32_t 	i;
	FILE* f;
	char	name[MAX_OSPATH];

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		Com_sprintf(name, sizeof(name), "%s/save/save%i/server.ssv", FS_Gamedir(), i);
		f = fopen(name, "rb");
		if (!f)
		{
			strcpy(m_savestrings[i], "<EMPTY>");
			m_savevalid[i] = false;
		}
		else
		{
			FS_Read(m_savestrings[i], sizeof(m_savestrings[i]), f);
			fclose(f);
			m_savevalid[i] = true;
		}
	}
}

void LoadGameCallback(void* self)
{
	menuaction_t* a = (menuaction_t*)self;

	if (m_savevalid[a->generic.localdata[0]])
		Cbuf_AddText(va("load save%i\n", a->generic.localdata[0]));
	M_ForceMenuOff();
}

void LoadGame_MenuInit(void)
{
	int32_t i;

	s_loadgame_menu.x = r_width->value / 2 - 120 * vid_hudscale->value;
	s_loadgame_menu.y = r_height->value / 2 - 58 * vid_hudscale->value;
	s_loadgame_menu.nitems = 0;

	Create_Savestrings();

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		s_loadgame_actions[i].generic.name = m_savestrings[i];
		s_loadgame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_loadgame_actions[i].generic.localdata[0] = i;
		s_loadgame_actions[i].generic.callback = LoadGameCallback;

		s_loadgame_actions[i].generic.x = 0;
		s_loadgame_actions[i].generic.y = (i) * 10 * vid_hudscale->value;
		if (i > 0)	// separate from autosave
			s_loadgame_actions[i].generic.y += 10 * vid_hudscale->value;

		s_loadgame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem(&s_loadgame_menu, &s_loadgame_actions[i]);
	}
}

void LoadGame_MenuDraw(void)
{
	M_Banner("2d/m_banner_load_game");
	//	Menu_AdjustCursor( &s_loadgame_menu, 1 );
	Menu_Draw(&s_loadgame_menu);
}

const char* LoadGame_MenuKey(int32_t key)
{
	if (key == K_ESCAPE || key == K_ENTER)
	{
		s_savegame_menu.cursor = s_loadgame_menu.cursor - 1;
		if (s_savegame_menu.cursor < 0)
			s_savegame_menu.cursor = 0;
	}
	return Default_MenuKey(&s_loadgame_menu, key);
}

void M_Menu_LoadGame_f()
{
	LoadGame_MenuInit();
	M_PushMenu(LoadGame_MenuDraw, LoadGame_MenuKey);
}


/*
=============================================================================

SAVEGAME MENU

=============================================================================
*/
static menuframework_t	s_savegame_menu;
static menuaction_t		s_savegame_actions[MAX_SAVEGAMES];

void SaveGameCallback(void* self)
{
	menuaction_t* a = (menuaction_t*)self;

	Cbuf_AddText(va("save save%i\n", a->generic.localdata[0]));
	M_ForceMenuOff();
}

void SaveGame_MenuDraw(void)
{
	M_Banner("2d/m_banner_save_game");
	Menu_AdjustCursor(&s_savegame_menu, 1);
	Menu_Draw(&s_savegame_menu);
}

void SaveGame_MenuInit(void)
{
	int32_t i;

	s_savegame_menu.x = r_width->value / 2 - 120 * vid_hudscale->value;
	s_savegame_menu.y = r_height->value / 2 - 58 * vid_hudscale->value;
	s_savegame_menu.nitems = 0;

	Create_Savestrings();

	// don't include the autosave slot
	for (i = 0; i < MAX_SAVEGAMES - 1; i++)
	{
		s_savegame_actions[i].generic.name = m_savestrings[i + 1];
		s_savegame_actions[i].generic.localdata[0] = i + 1;
		s_savegame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_savegame_actions[i].generic.callback = SaveGameCallback;

		s_savegame_actions[i].generic.x = 0;
		s_savegame_actions[i].generic.y = (i) * 10 * vid_hudscale->value;

		s_savegame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem(&s_savegame_menu, &s_savegame_actions[i]);
	}
}

const char* SaveGame_MenuKey(int32_t key)
{
	if (key == K_ENTER || key == K_ESCAPE)
	{
		s_loadgame_menu.cursor = s_savegame_menu.cursor - 1;
		if (s_loadgame_menu.cursor < 0)
			s_loadgame_menu.cursor = 0;
	}
	return Default_MenuKey(&s_savegame_menu, key);
}

void M_Menu_SaveGame_f()
{
	if (!Com_ServerState())
		return;		// not playing a game

	SaveGame_MenuInit();
	M_PushMenu(SaveGame_MenuDraw, SaveGame_MenuKey);
	Create_Savestrings();
}


/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/
#define MAX_LOCAL_SERVERS 8

static menuframework_t	s_joinserver_menu;
static menuseparator_t	s_joinserver_server_title;
static menuaction_t		s_joinserver_search_action;
static menuaction_t		s_joinserver_address_book_action;
static menuaction_t		s_joinserver_server_actions[MAX_LOCAL_SERVERS];

int32_t 	m_num_servers;
#define	NO_SERVER_STRING	"<no server>"

// user readable information
static char local_server_names[MAX_LOCAL_SERVERS][80];

// network address
static netadr_t local_server_netadr[MAX_LOCAL_SERVERS];

void M_AddToServerList(netadr_t adr, char* info)
{
	int32_t 	i;

	if (m_num_servers == MAX_LOCAL_SERVERS)
		return;
	while (*info == ' ')
		info++;

	// ignore if duplicated
	for (i = 0; i < m_num_servers; i++)
		if (!strcmp(info, local_server_names[i]))
			return;

	local_server_netadr[m_num_servers] = adr;
	strncpy(local_server_names[m_num_servers], info, sizeof(local_server_names[0]) - 1);
	m_num_servers++;
}


void JoinServerFunc(void* self)
{
	char	buffer[128];
	int32_t 	index;

	index = (menuaction_t*)self - s_joinserver_server_actions;

	if (Q_stricmp(local_server_names[index], NO_SERVER_STRING) == 0)
		return;

	if (index >= m_num_servers)
		return;

	Com_sprintf(buffer, sizeof(buffer), "connect %s\n", Net_AdrToString(local_server_netadr[index]));
	Cbuf_AddText(buffer);
	M_ForceMenuOff();
}

void AddressBookFunc(void* self)
{
	M_Menu_AddressBook_f();
}

void SearchLocalGames(void)
{
	int32_t 	i;

	m_num_servers = 0;
	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
		strcpy(local_server_names[i], NO_SERVER_STRING);

	search_local_games = true;
}

void SearchLocalGamesFunc(void* self)
{
	SearchLocalGames();
}

void JoinServer_MenuInit(void)
{
	int32_t i;

	s_joinserver_menu.x = r_width->value * 0.50 - 120 * vid_hudscale->value;
	s_joinserver_menu.nitems = 0;

	s_joinserver_address_book_action.generic.type = MTYPE_ACTION;
	s_joinserver_address_book_action.generic.name = "^3[STRING_JOINSERVERUI_ADDRESSBOOK]";
	s_joinserver_address_book_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_joinserver_address_book_action.generic.x = 0;
	s_joinserver_address_book_action.generic.y = 0;
	s_joinserver_address_book_action.generic.callback = AddressBookFunc;

	s_joinserver_search_action.generic.type = MTYPE_ACTION;
	s_joinserver_search_action.generic.name = "^3[STRING_JOINSERVERUI_REFRESH]";
	s_joinserver_search_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_joinserver_search_action.generic.x = 0;
	s_joinserver_search_action.generic.y = 10 * vid_hudscale->value;
	s_joinserver_search_action.generic.callback = SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar = "[STRING_JOINSERVERUI_SEARCH]";

	s_joinserver_server_title.generic.type = MTYPE_SEPARATOR;
	s_joinserver_server_title.generic.name = "^5[STRING_JOINSERVERUI_CONNECT]";
	s_joinserver_server_title.generic.x = 80 * vid_hudscale->value;
	s_joinserver_server_title.generic.y = 30 * vid_hudscale->value;

	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
	{
		s_joinserver_server_actions[i].generic.type = MTYPE_ACTION;
		strcpy(local_server_names[i], NO_SERVER_STRING);
		s_joinserver_server_actions[i].generic.name = local_server_names[i];
		s_joinserver_server_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_joinserver_server_actions[i].generic.x = 0;
		s_joinserver_server_actions[i].generic.y = (40 + i * 10) * vid_hudscale->value;
		s_joinserver_server_actions[i].generic.callback = JoinServerFunc;
		s_joinserver_server_actions[i].generic.statusbar = "[STRING_JOINSERVERUI_PRESSENTER]";
	}

	Menu_AddItem(&s_joinserver_menu, &s_joinserver_address_book_action);
	Menu_AddItem(&s_joinserver_menu, &s_joinserver_server_title);
	Menu_AddItem(&s_joinserver_menu, &s_joinserver_search_action);

	for (i = 0; i < 8; i++)
		Menu_AddItem(&s_joinserver_menu, &s_joinserver_server_actions[i]);

	Menu_Center(&s_joinserver_menu);

	SearchLocalGames();
}

void JoinServer_MenuDraw()
{
	M_Banner("2d/m_banner_join_server");
	Menu_Draw(&s_joinserver_menu);
	font_t* system_font_ptr = Font_GetByName(cl_system_font->string);

	if (search_local_games)
	{
		int32_t y = 120 - 48 * vid_hudscale->value;
		Menu_DrawTextBox(8 - 152 * (vid_hudscale->value - 1.f), y, 36, 3);
		y += (system_font_ptr->line_height * vid_hudscale->value);
		Text_Draw(cl_system_font->string, 32 - 136 * (vid_hudscale->value - 1.f), y, "[STRING_JOINSERVERUI_LOCALSEARCH1]");
		y += (system_font_ptr->line_height * vid_hudscale->value);
		Text_Draw(cl_system_font->string, 32 - 136 * (vid_hudscale->value - 1.f), y, "[STRING_JOINSERVERUI_LOCALSEARCH2]");
		y += (system_font_ptr->line_height * vid_hudscale->value);
		Text_Draw(cl_system_font->string, 32 - 136 * (vid_hudscale->value - 1.f), y, "[STRING_JOINSERVERUI_LOCALSEARCH3]");

		// send out info packets
		CL_PingServers_f();
		search_local_games = false;
	}
}


const char* JoinServer_MenuKey(int32_t key)
{
	return Default_MenuKey(&s_joinserver_menu, key);
}

void M_Menu_JoinServer_f()
{
	JoinServer_MenuInit();
	M_PushMenu(JoinServer_MenuDraw, JoinServer_MenuKey);
}


/*
=============================================================================

START SERVER MENU

=============================================================================
*/
static menuframework_t s_startserver_menu;
static char** mapnames;
static int32_t   nummaps;

static menuaction_t	s_startserver_start_action;
static menuaction_t	s_startserver_gameoptions_action;
static menufield_t	s_timelimit_field;
static menufield_t	s_fraglimit_field;
static menufield_t	s_maxclients_field;
static menufield_t	s_hostname_field;
static menulist_t	s_startmap_list;
static menulist_t	s_rules_box;

void gameoptionsFunc(void* self)
{
	if (s_rules_box.curvalue == 1)
		return;
	M_Menu_GameOptions_f();
}

void RulesChangeFunc(void* self)
{
	// DM
	if (s_rules_box.curvalue == 0)
	{
		s_maxclients_field.generic.statusbar = NULL;
		s_startserver_gameoptions_action.generic.statusbar = NULL;
	}
}

void StartServerActionFunc(void* self)
{
	char	startmap[1024];
	int32_t 	timelimit;
	int32_t 	fraglimit;
	int32_t 	maxclients;
	char* spot;

	strcpy(startmap, strchr(mapnames[s_startmap_list.curvalue], '\n') + 1);

	maxclients = atoi(s_maxclients_field.buffer);
	timelimit = atoi(s_timelimit_field.buffer);
	fraglimit = atoi(s_fraglimit_field.buffer);

	Cvar_SetValue("sv_maxclients", ClampCvar(0, maxclients, maxclients));
	Cvar_SetValue("timelimit", ClampCvar(0, timelimit, timelimit));
	Cvar_SetValue("fraglimit", ClampCvar(0, fraglimit, fraglimit));
	Cvar_Set("hostname", s_hostname_field.buffer);

	Cvar_SetValue("gamemode", s_rules_box.curvalue);

	spot = NULL;

	if (spot)
	{
		if (Com_ServerState())
			Cbuf_AddText("disconnect\n");
		Cbuf_AddText(va("gamemap \"*%s$%s\"\n", startmap, spot));
	}
	else
	{
		Cbuf_AddText(va("map %s\n", startmap));
	}

	M_ForceMenuOff();

	if (mapnames)
	{
		int32_t i;

		for (i = 0; i < nummaps; i++)
			free(mapnames[i]);
		free(mapnames);
	}
	mapnames = 0;
	nummaps = 0;
}

void StartServer_MenuInit(void)
{
	static const char* gamemode_names[] =
	{
		"^2[STRING_GAMEMODE_TDM]",
		"^2[STRING_GAMEMODE_HOSTAGE]",
		"^2[STRING_GAMEMODE_WAVES]",
		"^2[STRING_GAMEMODE_COOP]",
		"^2[STRING_GAMEMODE_CONTROL_POINT]",
		"^2[STRING_GAMEMODE_TOURNAMENT]",
		0
	};

	char* buffer;
	char  mapsname[1024];
	char* s;
	int32_t length;
	int32_t i;
	FILE* fp;

	/*
	** load the list of map names
	*/
	Com_sprintf(mapsname, sizeof(mapsname), "%s/maps.lst", FS_Gamedir());
	if ((fp = fopen(mapsname, "rb")) == 0)
	{
		if ((length = FS_LoadFile("maps.lst", (void**)&buffer)) == -1)
			Com_Error(ERR_DROP, "[STRING_ERROR_NO_MAP_LIST]\n");
	}
	else
	{
#ifdef _WIN32
		length = filelength(fileno(fp));
#else
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
#endif
		buffer = malloc(length);
		fread(buffer, length, 1, fp);
	}

	s = buffer;

	i = 0;
	while (i < length)
	{
		if (s[i] == '\n')
			nummaps++;
		i++;
	}

	if (nummaps == 0)
		Com_Error(ERR_DROP, "STRING_ERROR_NO_MAPS]\n");

	mapnames = malloc(sizeof(char*) * (nummaps + 1));
	memset(mapnames, 0, sizeof(char*) * (nummaps + 1));

	s = buffer;

	for (i = 0; i < nummaps; i++)
	{
		char  shortname[MAX_TOKEN_CHARS];
		char  longname[MAX_TOKEN_CHARS];
		char  scratch[200];
		int32_t 	j, l;

		strcpy(shortname, COM_Parse(&s));
		l = (int32_t)strlen(shortname);
		for (j = 0; j < l; j++)
			shortname[j] = toupper(shortname[j]);
		strcpy(longname, COM_Parse(&s));
		Com_sprintf(scratch, sizeof(scratch), "%s\n%s", longname, shortname);

		mapnames[i] = malloc(strlen(scratch) + 1);
		strcpy(mapnames[i], scratch);
	}
	mapnames[nummaps] = 0;

	if (fp != 0)
	{
		fp = 0;
		free(buffer);
	}
	else
	{
		FS_FreeFile(buffer);
	}

	/*
	** initialize the menu stuff
	*/
	s_startserver_menu.x = r_width->value * 0.50;
	s_startserver_menu.nitems = 0;

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.x = 0;
	s_startmap_list.generic.y = 0;
	s_startmap_list.generic.name = "^5[STRING_STARTSERVERUI_INITIALMAP]";
	s_startmap_list.itemnames = (const char**)mapnames;

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.x = 0;
	s_rules_box.generic.y = 20 * vid_hudscale->value;
	s_rules_box.generic.name = "^5[STRING_STARTSERVERUI_GAMEMODE]";

	s_rules_box.itemnames = gamemode_names;

	s_rules_box.curvalue = 0;

	s_rules_box.generic.callback = RulesChangeFunc;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "^5[STRING_STARTSERVERUI_TIMELIMIT]";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.x = 0;
	s_timelimit_field.generic.y = 36 * vid_hudscale->value;
	s_timelimit_field.generic.statusbar = "[STRING_STARTSERVERUI_TIMELIMIT_HINT]";
	s_timelimit_field.length = 3;
	s_timelimit_field.visible_length = 3;
	strcpy(s_timelimit_field.buffer, Cvar_VariableString("timelimit"));

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "^5[STRING_STARTSERVERUI_FRAGLIMIT]";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x = 0;
	s_fraglimit_field.generic.y = 54 * vid_hudscale->value;
	s_fraglimit_field.generic.statusbar = "^5[STRING_STARTSERVERUI_FRAGLIMIT_HINT]";
	s_fraglimit_field.length = 3;
	s_fraglimit_field.visible_length = 3;
	strcpy(s_fraglimit_field.buffer, Cvar_VariableString("fraglimit"));

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is.
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "^5[STRING_STARTSERVERUI_MAXPLAYERS]";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.generic.x = 0;
	s_maxclients_field.generic.y = 72 * vid_hudscale->value;
	s_maxclients_field.generic.statusbar = NULL;
	s_maxclients_field.length = 3;
	s_maxclients_field.visible_length = 3;
	if (Cvar_VariableValue("sv_maxclients") == 1)
		strcpy(s_maxclients_field.buffer, "8");
	else
		strcpy(s_maxclients_field.buffer, Cvar_VariableString("sv_maxclients"));

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "^5[STRING_STARTSERVERUI_HOSTNAME]";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.generic.x = 0;
	s_hostname_field.generic.y = 90 * vid_hudscale->value;
	s_hostname_field.generic.statusbar = NULL;
	s_hostname_field.length = 12;
	s_hostname_field.visible_length = 12;
	strcpy(s_hostname_field.buffer, Cvar_VariableString("hostname"));

	s_startserver_gameoptions_action.generic.type = MTYPE_ACTION;
	s_startserver_gameoptions_action.generic.name = " ^3[STRING_STARTSERVERUI_GAMESETTINGS]";
	s_startserver_gameoptions_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_startserver_gameoptions_action.generic.x = 29 * vid_hudscale->value;
	s_startserver_gameoptions_action.generic.y = 107 * vid_hudscale->value;
	s_startserver_gameoptions_action.generic.statusbar = NULL;
	s_startserver_gameoptions_action.generic.callback = gameoptionsFunc;

	s_startserver_start_action.generic.type = MTYPE_ACTION;
	s_startserver_start_action.generic.name = " ^3[STRING_START_ENTHUSIASTIC]";
	s_startserver_start_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_startserver_start_action.generic.x = 29 * vid_hudscale->value;
	s_startserver_start_action.generic.y = 120 * vid_hudscale->value;
	s_startserver_start_action.generic.callback = StartServerActionFunc;

	Menu_AddItem(&s_startserver_menu, &s_startmap_list);
	Menu_AddItem(&s_startserver_menu, &s_rules_box);
	Menu_AddItem(&s_startserver_menu, &s_timelimit_field);
	Menu_AddItem(&s_startserver_menu, &s_fraglimit_field);
	Menu_AddItem(&s_startserver_menu, &s_maxclients_field);
	Menu_AddItem(&s_startserver_menu, &s_hostname_field);
	Menu_AddItem(&s_startserver_menu, &s_startserver_gameoptions_action);
	Menu_AddItem(&s_startserver_menu, &s_startserver_start_action);

	Menu_Center(&s_startserver_menu);

	// call this now to set proper inital state
	RulesChangeFunc(NULL);
}

void StartServer_MenuDraw()
{
#ifdef PLAYTEST
	font_t* system_font_ptr = Font_GetByName(cl_system_font->string);
	int32_t size_x = 0, size_y = 0;
	const char* no_playtest_text = "^1Servers cannot be started in playtest builds!\n\n^7Press any key to return to the main menu.";
	Text_GetSize(cl_system_font->string, &size_x, &size_y, no_playtest_text);
	Text_Draw(cl_system_font->string, r_width->value / 2 - (size_x / 2), r_height->value / 2 - (92 * vid_hudscale->value), no_playtest_text);
#else
	Menu_Draw(&s_startserver_menu);
#endif
}

const char* StartServer_MenuKey(int32_t key)
{
#ifdef PLAYTEST
	M_PopMenu();
#else
	if (key == K_ESCAPE)
	{
		if (mapnames)
		{
			int32_t i;

			for (i = 0; i < nummaps; i++)
				free(mapnames[i]);
			free(mapnames);
		}
		mapnames = 0;
		nummaps = 0;
	}

	return Default_MenuKey(&s_startserver_menu, key);
#endif

	return 0x00;
}

void M_Menu_StartServer_f()
{
	StartServer_MenuInit();
	M_PushMenu(StartServer_MenuDraw, StartServer_MenuKey);
}

/*
=============================================================================

Game Options menu

=============================================================================
*/
static char gameoptions_statusbar[128];

static menuframework_t s_gameoptions_menu;

static menulist_t s_friendlyfire_box;
static menulist_t s_friendlyfire_item_box;
static menulist_t s_falls_box;
static menulist_t s_weapons_stay_box;
static menulist_t s_instant_powerups_box;
static menulist_t s_powerups_box;
static menulist_t s_health_box;
static menulist_t s_spawn_farthest_box;
static menulist_t s_samelevel_box;
static menulist_t s_force_respawn_box;
static menulist_t s_armor_box;
static menulist_t s_allow_exit_box;
static menulist_t s_infinite_ammo_box;
static menulist_t s_quad_drop_box;
static menulist_t s_individual_fraglimit_box;
static menulist_t s_director_can_pickup_player_items_box;
static menulist_t s_dont_drop_director_items_box;

static void GameFlagCallback(void* self)
{
	menulist_t* f = (menulist_t*)self;
	int32_t flags;
	int32_t bit = 0;

	flags = Cvar_VariableValue("gameflags");

	if (f == &s_friendlyfire_box)
	{
		if (f->curvalue)
			flags &= ~GF_NO_FRIENDLY_FIRE;
		else
			flags |= GF_NO_FRIENDLY_FIRE;
		goto setvalue;
	}
	else if (f == &s_falls_box)
	{
		if (f->curvalue)
			flags &= ~GF_NO_FALL_DAMAGE;
		else
			flags |= GF_NO_FALL_DAMAGE;
		goto setvalue;
	}
	else if (f == &s_weapons_stay_box)
	{
		bit = GF_WEAPONS_STAY;
	}
	else if (f == &s_instant_powerups_box)
	{
		bit = GF_INSTANT_ITEMS;
	}
	else if (f == &s_allow_exit_box)
	{
		bit = GF_ALLOW_EXIT;
	}
	else if (f == &s_powerups_box)
	{
		if (f->curvalue)
			flags &= ~GF_NO_ITEMS;
		else
			flags |= GF_NO_ITEMS;
		goto setvalue;
	}
	else if (f == &s_health_box)
	{
		if (f->curvalue)
			flags &= ~GF_NO_HEALTH;
		else
			flags |= GF_NO_HEALTH;
		goto setvalue;
	}
	else if (f == &s_spawn_farthest_box)
	{
		bit = GF_SPAWN_FARTHEST;
	}
	else if (f == &s_samelevel_box)
	{
		bit = GF_SAME_LEVEL;
	}
	else if (f == &s_force_respawn_box)
	{
		bit = GF_FORCE_RESPAWN;
	}
	else if (f == &s_armor_box)
	{
		if (f->curvalue)
			flags &= ~GF_NO_ARMOR;
		else
			flags |= GF_NO_ARMOR;
		goto setvalue;
	}
	else if (f == &s_infinite_ammo_box)
	{
		bit = GF_INFINITE_AMMO;
	}
	else if (f == &s_quad_drop_box)
	{
		bit = GF_QUAD_DROP;
	}
	else if (f == &s_friendlyfire_item_box)
	{
		// intentional, this is an "allow" flag
		if (f->curvalue)
			flags |= GF_ITEM_FRIENDLY_FIRE;

		else
			flags &= ~GF_ITEM_FRIENDLY_FIRE;
		goto setvalue;
	}
	else if (f == &s_individual_fraglimit_box)
	{
		if (f->curvalue)
			flags |= GF_INDIVIDUAL_FRAGLIMIT;
		else
			flags &= ~GF_INDIVIDUAL_FRAGLIMIT;
		goto setvalue;
	}
	else if (f == &s_director_can_pickup_player_items_box)
	{
		if (f->curvalue)
			flags |= GF_DIRECTOR_CAN_PICKUP_PLAYER_ITEMS;
		else
			flags &= ~GF_DIRECTOR_CAN_PICKUP_PLAYER_ITEMS;
		goto setvalue;
	}
	else if (f == &s_dont_drop_director_items_box)
	{
		if (f->curvalue)
			flags |= GF_DONT_DROP_DIRECTOR_ITEMS;
		else
			flags &= ~GF_DONT_DROP_DIRECTOR_ITEMS;
		goto setvalue;
	}


	if (f)
	{
		if (f->curvalue == 0)
			flags &= ~bit;
		else
			flags |= bit;
	}

setvalue:
	Cvar_SetValue("gameflags", flags);

	Com_sprintf(gameoptions_statusbar, sizeof(gameoptions_statusbar), "gameflags = %d", flags);

}

void GameOptions_MenuInit(void)
{
	static const char* yes_no_names[] =
	{
		"^1[STRING_NO]",
		"^2[STRING_YES]", 
		0
	};

	int32_t gameflags = Cvar_VariableValue("gameflags");
	int32_t y = 0;

	s_gameoptions_menu.x = r_width->value * 0.50;
	s_gameoptions_menu.nitems = 0;

	s_falls_box.generic.type = MTYPE_SPINCONTROL;
	s_falls_box.generic.x = 0;
	s_falls_box.generic.y = y * vid_hudscale->value;
	s_falls_box.generic.name = "^5[STRING_GAMEFLAG_FALLDAMAGE]";
	s_falls_box.generic.callback = GameFlagCallback;
	s_falls_box.itemnames = yes_no_names;
	s_falls_box.curvalue = (gameflags & GF_NO_FALL_DAMAGE) == 0;

	s_weapons_stay_box.generic.type = MTYPE_SPINCONTROL;
	s_weapons_stay_box.generic.x = 0;
	s_weapons_stay_box.generic.y = y += 10 * vid_hudscale->value;
	s_weapons_stay_box.generic.name = "^5[STRING_GAMEFLAG_WEAPONSSTAYAFTERDEATH]";
	s_weapons_stay_box.generic.callback = GameFlagCallback;
	s_weapons_stay_box.itemnames = yes_no_names;
	s_weapons_stay_box.curvalue = (gameflags & GF_WEAPONS_STAY) != 0;

	s_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_powerups_box.generic.x = 0;
	s_powerups_box.generic.y = y += 10 * vid_hudscale->value;
	s_powerups_box.generic.name = "^5[STRING_GAMEFLAG_ALLOWPOWERUPS]";
	s_powerups_box.generic.callback = GameFlagCallback;
	s_powerups_box.itemnames = yes_no_names;
	s_powerups_box.curvalue = (gameflags & GF_NO_ITEMS) == 0;

	s_instant_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_instant_powerups_box.generic.x = 0;
	s_instant_powerups_box.generic.y = y += 10 * vid_hudscale->value;
	s_instant_powerups_box.generic.name = "^5[STRING_GAMEFLAG_INSTANTPOWERUPS]";
	s_instant_powerups_box.generic.callback = GameFlagCallback;
	s_instant_powerups_box.itemnames = yes_no_names;
	s_instant_powerups_box.curvalue = (gameflags & GF_INSTANT_ITEMS) != 0;

	s_health_box.generic.type = MTYPE_SPINCONTROL;
	s_health_box.generic.x = 0;
	s_health_box.generic.y = y += 10 * vid_hudscale->value;
	s_health_box.generic.callback = GameFlagCallback;
	s_health_box.generic.name = "^5[STRING_GAMEFLAG_ALLOWHEALTHPICKUPS]";
	s_health_box.itemnames = yes_no_names;
	s_health_box.curvalue = (gameflags & GF_NO_HEALTH) == 0;

	s_armor_box.generic.type = MTYPE_SPINCONTROL;
	s_armor_box.generic.x = 0;
	s_armor_box.generic.y = y += 10 * vid_hudscale->value;
	s_armor_box.generic.name = "^5[STRING_GAMEFLAG_ALLOWARMOR]";
	s_armor_box.generic.callback = GameFlagCallback;
	s_armor_box.itemnames = yes_no_names;
	s_armor_box.curvalue = (gameflags & GF_NO_ARMOR) == 0;

	s_spawn_farthest_box.generic.type = MTYPE_SPINCONTROL;
	s_spawn_farthest_box.generic.x = 0;
	s_spawn_farthest_box.generic.y = y += 10 * vid_hudscale->value;
	s_spawn_farthest_box.generic.name = "^5[STRING_GAMEFLAG_USEFARTHESTSPAWNPOINT]";
	s_spawn_farthest_box.generic.callback = GameFlagCallback;
	s_spawn_farthest_box.itemnames = yes_no_names;
	s_spawn_farthest_box.curvalue = (gameflags & GF_SPAWN_FARTHEST) != 0;

	s_samelevel_box.generic.type = MTYPE_SPINCONTROL;
	s_samelevel_box.generic.x = 0;
	s_samelevel_box.generic.y = y += 10 * vid_hudscale->value;
	s_samelevel_box.generic.name = "^5[STRING_GAMEFLAG_NOCYCLE]";
	s_samelevel_box.generic.callback = GameFlagCallback;
	s_samelevel_box.itemnames = yes_no_names;
	s_samelevel_box.curvalue = (gameflags & GF_SAME_LEVEL) != 0;

	s_force_respawn_box.generic.type = MTYPE_SPINCONTROL;
	s_force_respawn_box.generic.x = 0;
	s_force_respawn_box.generic.y = y += 10 * vid_hudscale->value;
	s_force_respawn_box.generic.name = "^5[STRING_GAMEFLAG_FORCERESPAWN]";
	s_force_respawn_box.generic.callback = GameFlagCallback;
	s_force_respawn_box.itemnames = yes_no_names;
	s_force_respawn_box.curvalue = (gameflags & GF_FORCE_RESPAWN) != 0;

	s_allow_exit_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_exit_box.generic.x = 0;
	s_allow_exit_box.generic.y = y += 10 * vid_hudscale->value;
	s_allow_exit_box.generic.name = "^5[STRING_GAMEFLAG_ALLOWEXIT]";
	s_allow_exit_box.generic.callback = GameFlagCallback;
	s_allow_exit_box.itemnames = yes_no_names;
	s_allow_exit_box.curvalue = (gameflags & GF_ALLOW_EXIT) != 0;

	s_infinite_ammo_box.generic.type = MTYPE_SPINCONTROL;
	s_infinite_ammo_box.generic.x = 0;
	s_infinite_ammo_box.generic.y = y += 10 * vid_hudscale->value;
	s_infinite_ammo_box.generic.name = "^5[STRING_GAMEFLAG_INFINITEAMMO]";
	s_infinite_ammo_box.generic.callback = GameFlagCallback;
	s_infinite_ammo_box.itemnames = yes_no_names;
	s_infinite_ammo_box.curvalue = (gameflags & GF_INFINITE_AMMO) != 0;

	s_quad_drop_box.generic.type = MTYPE_SPINCONTROL;
	s_quad_drop_box.generic.x = 0;
	s_quad_drop_box.generic.y = y += 10 * vid_hudscale->value;
	s_quad_drop_box.generic.name = "^5[STRING_GAMEFLAG_ALLOWQUAD]";
	s_quad_drop_box.generic.callback = GameFlagCallback;
	s_quad_drop_box.itemnames = yes_no_names;
	s_quad_drop_box.curvalue = (gameflags & GF_QUAD_DROP) != 0;

	s_friendlyfire_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_box.generic.x = 0;
	s_friendlyfire_box.generic.y = y += 10 * vid_hudscale->value;
	s_friendlyfire_box.generic.name = "^5[STRING_GAMEFLAG_FRIENDLYFIREPLAYER]";
	s_friendlyfire_box.generic.callback = GameFlagCallback;
	s_friendlyfire_box.itemnames = yes_no_names;
	s_friendlyfire_box.curvalue = (gameflags & GF_NO_FRIENDLY_FIRE) == 0;

	s_friendlyfire_item_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_item_box.generic.x = 0;
	s_friendlyfire_item_box.generic.y = y += 10 * vid_hudscale->value;
	s_friendlyfire_item_box.generic.name = "^5[STRING_GAMEFLAG_FRIENDLYFIREMONSTER]";
	s_friendlyfire_item_box.generic.callback = GameFlagCallback;
	s_friendlyfire_item_box.itemnames = yes_no_names;
	s_friendlyfire_item_box.curvalue = (gameflags & GF_ITEM_FRIENDLY_FIRE) != 0;

	s_individual_fraglimit_box.generic.type = MTYPE_SPINCONTROL;
	s_individual_fraglimit_box.generic.x = 0;
	s_individual_fraglimit_box.generic.y = y += 10 * vid_hudscale->value;
	s_individual_fraglimit_box.generic.name = "^5[STRING_GAMEFLAG_PERPLAYERFRAGLIMIT]";
	s_individual_fraglimit_box.generic.callback = GameFlagCallback;
	s_individual_fraglimit_box.itemnames = yes_no_names;
	s_individual_fraglimit_box.curvalue = (gameflags & GF_INDIVIDUAL_FRAGLIMIT) != 0;

	s_director_can_pickup_player_items_box.generic.type = MTYPE_SPINCONTROL;
	s_director_can_pickup_player_items_box.generic.x = 0;
	s_director_can_pickup_player_items_box.generic.y = y += 10 * vid_hudscale->value;
	s_director_can_pickup_player_items_box.generic.name = "^5[STRING_GAMEFLAG_DIRECTORCANPICKUPPLAYERITEMS]";
	s_director_can_pickup_player_items_box.generic.callback = GameFlagCallback;
	s_director_can_pickup_player_items_box.itemnames = yes_no_names;
	s_director_can_pickup_player_items_box.curvalue = (gameflags & GF_DIRECTOR_CAN_PICKUP_PLAYER_ITEMS) != 0;

	s_dont_drop_director_items_box.generic.type = MTYPE_SPINCONTROL;
	s_dont_drop_director_items_box.generic.x = 0;
	s_dont_drop_director_items_box.generic.y = y += 10 * vid_hudscale->value;
	s_dont_drop_director_items_box.generic.name = "^5[STRING_GAMEFLAG_NODROPDIRECTORITEMS]";
	s_dont_drop_director_items_box.generic.callback = GameFlagCallback;
	s_dont_drop_director_items_box.itemnames = yes_no_names;
	s_dont_drop_director_items_box.curvalue = (gameflags & GF_DONT_DROP_DIRECTOR_ITEMS) != 0;

	Menu_AddItem(&s_gameoptions_menu, &s_falls_box);
	Menu_AddItem(&s_gameoptions_menu, &s_weapons_stay_box);
	Menu_AddItem(&s_gameoptions_menu, &s_instant_powerups_box);
	Menu_AddItem(&s_gameoptions_menu, &s_powerups_box);
	Menu_AddItem(&s_gameoptions_menu, &s_health_box);
	Menu_AddItem(&s_gameoptions_menu, &s_armor_box);
	Menu_AddItem(&s_gameoptions_menu, &s_spawn_farthest_box);
	Menu_AddItem(&s_gameoptions_menu, &s_samelevel_box);
	Menu_AddItem(&s_gameoptions_menu, &s_force_respawn_box);
	Menu_AddItem(&s_gameoptions_menu, &s_allow_exit_box);
	Menu_AddItem(&s_gameoptions_menu, &s_infinite_ammo_box);
	Menu_AddItem(&s_gameoptions_menu, &s_quad_drop_box);
	Menu_AddItem(&s_gameoptions_menu, &s_friendlyfire_box);
	Menu_AddItem(&s_gameoptions_menu, &s_friendlyfire_item_box);
	Menu_AddItem(&s_gameoptions_menu, &s_individual_fraglimit_box);
	Menu_AddItem(&s_gameoptions_menu, &s_director_can_pickup_player_items_box);
	Menu_AddItem(&s_gameoptions_menu, &s_dont_drop_director_items_box);
	Menu_Center(&s_gameoptions_menu);

	// set the original gameflags statusbar
	GameFlagCallback(0);
	Menu_SetStatusBar(&s_gameoptions_menu, gameoptions_statusbar);
}

void GameOptions_MenuDraw()
{
	Menu_Draw(&s_gameoptions_menu);
}

const char* GameOptions_MenuKey(int32_t key)
{
	return Default_MenuKey(&s_gameoptions_menu, key);
}

void M_Menu_GameOptions_f()
{
	GameOptions_MenuInit();
	M_PushMenu(GameOptions_MenuDraw, GameOptions_MenuKey);
}

/*
=============================================================================

DOWNLOADOPTIONS BOOK MENU

=============================================================================
*/
static menuframework_t s_downloadoptions_menu;

static menuseparator_t	s_download_title;
static menulist_t	s_allow_download_box;
static menulist_t	s_allow_download_maps_box;
static menulist_t	s_allow_download_models_box;
static menulist_t	s_allow_download_players_box;
static menulist_t	s_allow_download_sounds_box;

static void DownloadCallback(void* self)
{
	menulist_t* f = (menulist_t*)self;

	if (f == &s_allow_download_box)
	{
		Cvar_SetValue("allow_download", f->curvalue);
	}

	else if (f == &s_allow_download_maps_box)
	{
		Cvar_SetValue("allow_download_maps", f->curvalue);
	}

	else if (f == &s_allow_download_models_box)
	{
		Cvar_SetValue("allow_download_models", f->curvalue);
	}

	else if (f == &s_allow_download_players_box)
	{
		Cvar_SetValue("allow_download_players", f->curvalue);
	}

	else if (f == &s_allow_download_sounds_box)
	{
		Cvar_SetValue("allow_download_sounds", f->curvalue);
	}
}

void DownloadOptions_MenuInit(void)
{
	static const char* yes_no_names[] =
	{
		"^1No", "^2Yes", 0
	};
	int32_t y = 0;

	s_downloadoptions_menu.x = r_width->value * 0.50;
	s_downloadoptions_menu.nitems = 0;

	s_download_title.generic.type = MTYPE_SEPARATOR;
	s_download_title.generic.name = "^3[STRING_DOWNLOADOPTIONSUI]";
	s_download_title.generic.x = 48;
	s_download_title.generic.y = y * vid_hudscale->value;

	s_allow_download_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_box.generic.x = 0;
	s_allow_download_box.generic.y = y += 20 * vid_hudscale->value;
	s_allow_download_box.generic.name = "^5[STRING_DOWNLOADOPTIONSUI_ALLOWDOWNLOADING_HEADER]";
	s_allow_download_box.generic.callback = DownloadCallback;
	s_allow_download_box.itemnames = yes_no_names;
	s_allow_download_box.curvalue = (Cvar_VariableValue("allow_download") != 0);

	s_allow_download_maps_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_maps_box.generic.x = 0;
	s_allow_download_maps_box.generic.y = y += 20 * vid_hudscale->value;
	s_allow_download_maps_box.generic.name = "^5[STRING_DOWNLOADOPTIONSUI_ALLOWDOWNLOADING_MAPS]";
	s_allow_download_maps_box.generic.callback = DownloadCallback;
	s_allow_download_maps_box.itemnames = yes_no_names;
	s_allow_download_maps_box.curvalue = (Cvar_VariableValue("allow_download_maps") != 0);

	s_allow_download_players_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_players_box.generic.x = 0;
	s_allow_download_players_box.generic.y = y += 10 * vid_hudscale->value;
	s_allow_download_players_box.generic.name = "^5[STRING_DOWNLOADOPTIONSUI_ALLOWDOWNLOADING_PLAYERSTUFF]";
	s_allow_download_players_box.generic.callback = DownloadCallback;
	s_allow_download_players_box.itemnames = yes_no_names;
	s_allow_download_players_box.curvalue = (Cvar_VariableValue("allow_download_players") != 0);

	s_allow_download_models_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_models_box.generic.x = 0;
	s_allow_download_models_box.generic.y = y += 10 * vid_hudscale->value;
	s_allow_download_models_box.generic.name = "^5[STRING_DOWNLOADOPTIONSUI_ALLOWDOWNLOADING_MODELS]";
	s_allow_download_models_box.generic.callback = DownloadCallback;
	s_allow_download_models_box.itemnames = yes_no_names;
	s_allow_download_models_box.curvalue = (Cvar_VariableValue("allow_download_models") != 0);

	s_allow_download_sounds_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_sounds_box.generic.x = 0;
	s_allow_download_sounds_box.generic.y = y += 10 * vid_hudscale->value;
	s_allow_download_sounds_box.generic.name = "^5[STRING_DOWNLOADOPTIONSUI_ALLOWDOWNLOADING_SOUNDS]";
	s_allow_download_sounds_box.generic.callback = DownloadCallback;
	s_allow_download_sounds_box.itemnames = yes_no_names;
	s_allow_download_sounds_box.curvalue = (Cvar_VariableValue("allow_download_sounds") != 0);

	Menu_AddItem(&s_downloadoptions_menu, &s_download_title);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_box);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_maps_box);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_players_box);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_models_box);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_sounds_box);

	Menu_Center(&s_downloadoptions_menu);

	// skip over title
	if (s_downloadoptions_menu.cursor == 0)
		s_downloadoptions_menu.cursor = 1;
}

void DownloadOptions_MenuDraw()
{
	Menu_Draw(&s_downloadoptions_menu);
}

const char* DownloadOptions_MenuKey(int32_t key)
{
	return Default_MenuKey(&s_downloadoptions_menu, key);
}

void M_Menu_DownloadOptions_f()
{
	DownloadOptions_MenuInit();
	M_PushMenu(DownloadOptions_MenuDraw, DownloadOptions_MenuKey);
}
/*
=============================================================================

ADDRESS BOOK MENU

=============================================================================
*/
#define NUM_ADDRESSBOOK_ENTRIES 9

static menuframework_t	s_addressbook_menu;
static menufield_t		s_addressbook_fields[NUM_ADDRESSBOOK_ENTRIES];

void AddressBook_MenuInit(void)
{
	int32_t i;

	s_addressbook_menu.x = r_width->value / 2 - 142 * vid_hudscale->value;
	s_addressbook_menu.y = r_height->value / 2 - 58 * vid_hudscale->value;
	s_addressbook_menu.nitems = 0;

	for (i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++)
	{
		cvar_t* adr;
		char buffer[20];

		Com_sprintf(buffer, sizeof(buffer), "adr%d", i);

		adr = Cvar_Get(buffer, "", CVAR_ARCHIVE);

		s_addressbook_fields[i].generic.type = MTYPE_FIELD;
		s_addressbook_fields[i].generic.name = 0;
		s_addressbook_fields[i].generic.callback = 0;
		s_addressbook_fields[i].generic.x = 0;
		s_addressbook_fields[i].generic.y = i * 18 * vid_hudscale->value + 0;
		s_addressbook_fields[i].generic.localdata[0] = i;
		s_addressbook_fields[i].cursor = 0;
		s_addressbook_fields[i].length = 60;
		s_addressbook_fields[i].visible_length = 30;

		strcpy(s_addressbook_fields[i].buffer, adr->string);

		Menu_AddItem(&s_addressbook_menu, &s_addressbook_fields[i]);
	}
}

const char* AddressBook_MenuKey(int32_t key)
{
	if (key == K_ESCAPE)
	{
		int32_t index;
		char buffer[20];

		for (index = 0; index < NUM_ADDRESSBOOK_ENTRIES; index++)
		{
			Com_sprintf(buffer, sizeof(buffer), "adr%d", index);
			Cvar_Set(buffer, s_addressbook_fields[index].buffer);
		}
	}
	return Default_MenuKey(&s_addressbook_menu, key);
}

void AddressBook_MenuDraw()
{
	M_Banner("2d/m_banner_addressbook");
	Menu_Draw(&s_addressbook_menu);
}

void M_Menu_AddressBook_f()
{
	AddressBook_MenuInit();
	M_PushMenu(AddressBook_MenuDraw, AddressBook_MenuKey);
}

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/
static menuframework_t	s_player_config_menu;
static menufield_t		s_player_name_field;
static menulist_t		s_player_skin_box;
static menuseparator_t	s_player_skin_title;
static menuseparator_t	s_player_model_title;
static menulist_t		s_player_model_box;
static menuseparator_t	s_player_handedness_title;
static menulist_t		s_player_handedness_box;
static menuseparator_t	s_player_gender_title;
static menulist_t		s_player_gender_box;
static menuaction_t		s_player_download_action;

#define MAX_DISPLAYNAME 16
#define MAX_PLAYERMODELS 1024

typedef struct
{
	int32_t 	nskins;
	char** skindisplaynames;
	char	displayname[MAX_DISPLAYNAME];
	char	directory[MAX_QPATH];
} playermodelinfo_s;

static playermodelinfo_s s_pmi[MAX_PLAYERMODELS];
static char* s_pmnames[MAX_PLAYERMODELS];
static int32_t s_numplayermodels;


void DownloadOptionsFunc(void* self)
{
	M_Menu_DownloadOptions_f();
}

static void HandednessCallback(void* unused)
{
	Cvar_SetValue("hand", s_player_handedness_box.curvalue);
}

static void GenderCallback(void* unused)
{
	Cvar_SetValue("gender", s_player_gender_box.curvalue);
}

static void ModelCallback(void* unused)
{
	s_player_skin_box.itemnames = (const char**)s_pmi[s_player_model_box.curvalue].skindisplaynames;
	s_player_skin_box.curvalue = 0;
}

static void FreeFileList(char** list, int32_t n)
{
	int32_t i;

	for (i = 0; i < n; i++)
	{
		if (list[i])
		{
			free(list[i]);
			list[i] = 0;
		}
	}
	free(list);
}

static bool IconOfSkinExists(char* skin, char** tgafiles, int32_t ntgafiles)
{
	int32_t i;
	char scratch[1024];

	strcpy(scratch, skin);
	*strrchr(scratch, '.') = 0;
	strcat(scratch, "_i.tga");

	for (i = 0; i < ntgafiles; i++)
	{
		if (strcmp(tgafiles[i], scratch) == 0)
			return true;
	}

	return false;
}

static bool PlayerConfig_ScanDirectories(void)
{
	char findname[1024];
	char scratch[1024];
	int32_t ndirs = 0, npms = 0;
	char** dirnames;
	char* path = NULL;
	int32_t i;

	extern char** FS_ListFiles(char*, int32_t*, uint32_t, uint32_t);

	s_numplayermodels = 0;

	/*
	** get a list of directories
	*/
	do
	{
		path = FS_NextPath(path);
		Com_sprintf(findname, sizeof(findname), "%s/players/*.*", path);

		if ((dirnames = FS_ListFiles(findname, &ndirs, SFF_SUBDIR, 0)) != 0)
			break;
	} while (path);

	if (!dirnames)
		return false;

	/*
	** go through the subdirectories
	*/
	npms = ndirs;
	if (npms > MAX_PLAYERMODELS)
		npms = MAX_PLAYERMODELS;

	for (i = 0; i < npms; i++)
	{
		int32_t k, s;
		char* a, * b, * c;
		char** tganames;
		char** skinnames;
		int32_t ntgafiles;
		int32_t nskins = 0;

		if (dirnames[i] == 0)
			continue;

		// verify the existence of tris.md2
		strcpy(scratch, dirnames[i]);
		strcat(scratch, "/tris.md2");
		if (!Sys_FindFirst(scratch, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM))
		{
			free(dirnames[i]);
			dirnames[i] = 0;
			Sys_FindClose();
			continue;
		}
		Sys_FindClose();

		// verify the existence of at least one tga skin
		strcpy(scratch, dirnames[i]);
		strcat(scratch, "/*.tga");
		tganames = FS_ListFiles(scratch, &ntgafiles, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

		if (!tganames)
		{
			free(dirnames[i]);
			dirnames[i] = 0;
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		for (k = 0; k < ntgafiles - 1; k++)
		{
			if (!strstr(tganames[k], "_i.tga"))
			{
				if (IconOfSkinExists(tganames[k], tganames, ntgafiles - 1))
				{
					nskins++;
				}
			}
		}
		if (!nskins)
			continue;

		skinnames = malloc(sizeof(char*) * (nskins + 1));
		memset(skinnames, 0, sizeof(char*) * (nskins + 1));

		// copy the valid skins
		for (s = 0, k = 0; k < ntgafiles - 1; k++)
		{
			char* a, * b, * c;

			if (!strstr(tganames[k], "_i.tga"))
			{
				if (IconOfSkinExists(tganames[k], tganames, ntgafiles - 1))
				{
					a = strrchr(tganames[k], '/');
					b = strrchr(tganames[k], '\\');

					if (a > b)
						c = a;
					else
						c = b;

					strcpy(scratch, c + 1);

					if (strrchr(scratch, '.'))
						*strrchr(scratch, '.') = 0;

					skinnames[s] = strdup(scratch);
					s++;
				}
			}
		}

		// at this point we have a valid player model
		s_pmi[s_numplayermodels].nskins = nskins;
		s_pmi[s_numplayermodels].skindisplaynames = skinnames;

		// make int16_t name for the model
		a = strrchr(dirnames[i], '/');
		b = strrchr(dirnames[i], '\\');

		if (a > b)
			c = a;
		else
			c = b;

		strncpy(s_pmi[s_numplayermodels].displayname, c + 1, MAX_DISPLAYNAME - 1);
		strcpy(s_pmi[s_numplayermodels].directory, c + 1);

		FreeFileList(tganames, ntgafiles);

		s_numplayermodels++;
	}

	if (dirnames)
		FreeFileList(dirnames, ndirs);

	return true;
}

static int32_t pmicmpfnc(const void* _a, const void* _b)
{
	const playermodelinfo_s* a = (const playermodelinfo_s*)_a;
	const playermodelinfo_s* b = (const playermodelinfo_s*)_b;

	/*
	** sort by male, female, then alphabetical
	*/
	if (strcmp(a->directory, "male") == 0)
		return -1;
	else if (strcmp(b->directory, "male") == 0)
		return 1;

	if (strcmp(a->directory, "female") == 0)
		return -1;
	else if (strcmp(b->directory, "female") == 0)
		return 1;

	return strcmp(a->directory, b->directory);
}


bool PlayerConfig_MenuInit(void)
{
	extern cvar_t* name;
	extern cvar_t* team;
	extern cvar_t* skin;
	char currentdirectory[1024];
	char currentskin[1024];
	int32_t i = 0;

	int32_t currentdirectoryindex = 0;
	int32_t currentskinindex = 0;

	cvar_t* hand = Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);

	static const char* handedness[] = { "[STRING_RIGHT]", "[STRING_LEFT]", "[STRING_CENTER]", 0 };
	static const char* genders[] = { "[STRING_GENDER_MALE]", "[STRING_GENDER_MALE]", "[STRING_GENDER_NONE]", "[STRING_GENDER_OTHER]", 0 };

	PlayerConfig_ScanDirectories();

	if (s_numplayermodels == 0)
		return false;

	if (hand->value < 0 || hand->value > 2)
		Cvar_SetValue("hand", 0);

	strcpy(currentdirectory, skin->string);

	if (strchr(currentdirectory, '/'))
	{
		strcpy(currentskin, strchr(currentdirectory, '/') + 1);
		*strchr(currentdirectory, '/') = 0;
	}
	else if (strchr(currentdirectory, '\\'))
	{
		strcpy(currentskin, strchr(currentdirectory, '\\') + 1);
		*strchr(currentdirectory, '\\') = 0;
	}
	else
	{
		strcpy(currentdirectory, "male");
		strcpy(currentskin, "grunt");
	}

	qsort(s_pmi, s_numplayermodels, sizeof(s_pmi[0]), pmicmpfnc);

	memset(s_pmnames, 0, sizeof(s_pmnames));
	for (i = 0; i < s_numplayermodels; i++)
	{
		s_pmnames[i] = s_pmi[i].displayname;
		if (Q_stricmp(s_pmi[i].directory, currentdirectory) == 0)
		{
			int32_t j;

			currentdirectoryindex = i;

			for (j = 0; j < s_pmi[i].nskins; j++)
			{
				if (Q_stricmp(s_pmi[i].skindisplaynames[j], currentskin) == 0)
				{
					currentskinindex = j;
					break;
				}
			}
		}
	}

	s_player_config_menu.x = r_width->value / 2 - 95 * vid_hudscale->value;
	s_player_config_menu.y = r_height->value / 2 - 97 * vid_hudscale->value;
	s_player_config_menu.nitems = 0;

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "^5[STRING_NAME]";
	s_player_name_field.generic.callback = 0;
	s_player_name_field.generic.x = 0;
	s_player_name_field.generic.y = 0;
	s_player_name_field.length = 20;
	s_player_name_field.visible_length = 20;
	strcpy(s_player_name_field.buffer, name->string);
	s_player_name_field.cursor = (int32_t)strlen(name->string);

	s_player_model_title.generic.type = MTYPE_SEPARATOR;
	s_player_model_title.generic.name = "^5[STRING_PLAYERCONFIGUI_MODEL]";
	s_player_model_title.generic.x = -14 * vid_hudscale->value;
	s_player_model_title.generic.y = 60 * vid_hudscale->value;
	s_player_model_title.generic.flags = QMF_LEFT_JUSTIFY;

	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.x = -56 * vid_hudscale->value;
	s_player_model_box.generic.y = 70 * vid_hudscale->value;
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.generic.cursor_offset = -48;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = (const char**)s_pmnames;

	s_player_skin_title.generic.type = MTYPE_SEPARATOR;
	s_player_skin_title.generic.name = "^5[STRING_PLAYERCONFIGUI_SKIN]";
	s_player_skin_title.generic.x = -21 * vid_hudscale->value;
	s_player_skin_title.generic.y = 84 * vid_hudscale->value;

	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.x = -56 * vid_hudscale->value;
	s_player_skin_box.generic.y = 94 * vid_hudscale->value;
	s_player_skin_box.generic.name = 0;
	s_player_skin_box.generic.callback = 0;
	s_player_skin_box.generic.cursor_offset = -48;
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = (const char**)s_pmi[currentdirectoryindex].skindisplaynames;

	s_player_handedness_title.generic.type = MTYPE_SEPARATOR;
	s_player_handedness_title.generic.name = "^5[STRING_PLAYERCONFIGUI_HANDEDNESS]";
	s_player_handedness_title.generic.x = 18 * vid_hudscale->value;
	s_player_handedness_title.generic.y = 108 * vid_hudscale->value;

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.x = -56 * vid_hudscale->value;
	s_player_handedness_box.generic.y = 118 * vid_hudscale->value;
	s_player_handedness_box.generic.name = 0;
	s_player_handedness_box.generic.cursor_offset = -48;
	s_player_handedness_box.generic.callback = HandednessCallback;
	s_player_handedness_box.curvalue = Cvar_VariableValue("hand");
	s_player_handedness_box.itemnames = handedness;

	// ULTRA WOKE!
	s_player_gender_title.generic.type = MTYPE_SEPARATOR;
	s_player_gender_title.generic.name = "^5[STRING_PLAYERCONFIGUI_GENDER]";
	s_player_gender_title.generic.x = -7 * vid_hudscale->value;
	s_player_gender_title.generic.y = 132 * vid_hudscale->value;

	s_player_gender_box.generic.type = MTYPE_SPINCONTROL;
	s_player_gender_box.generic.x = -56 * vid_hudscale->value;
	s_player_gender_box.generic.y = 142 * vid_hudscale->value;
	s_player_gender_box.generic.name = 0;
	s_player_gender_box.generic.cursor_offset = -48;
	s_player_gender_box.generic.callback = GenderCallback;
	s_player_gender_box.curvalue = Cvar_VariableValue("gender");
	s_player_gender_box.itemnames = genders;

	s_player_download_action.generic.type = MTYPE_ACTION;
	s_player_download_action.generic.name = "^2[STRING_DOWNLOADOPTIONSUI]";
	s_player_download_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_player_download_action.generic.x = -24 * vid_hudscale->value;
	s_player_download_action.generic.y = 210 * vid_hudscale->value;
	s_player_download_action.generic.statusbar = NULL;
	s_player_download_action.generic.callback = DownloadOptionsFunc;

	Menu_AddItem(&s_player_config_menu, &s_player_name_field);
	Menu_AddItem(&s_player_config_menu, &s_player_model_title);
	Menu_AddItem(&s_player_config_menu, &s_player_model_box);
	if (s_player_skin_box.itemnames)
	{
		Menu_AddItem(&s_player_config_menu, &s_player_skin_title);
		Menu_AddItem(&s_player_config_menu, &s_player_skin_box);
	}
	Menu_AddItem(&s_player_config_menu, &s_player_handedness_title);
	Menu_AddItem(&s_player_config_menu, &s_player_handedness_box);
	Menu_AddItem(&s_player_config_menu, &s_player_gender_title);
	Menu_AddItem(&s_player_config_menu, &s_player_gender_box);
	Menu_AddItem(&s_player_config_menu, &s_player_download_action);

	return true;
}

void PlayerConfig_MenuDraw(void)
{
	extern float Render3D_CalcFov(float fov_x, float w, float h);
	refdef_t refdef;
	char scratch[MAX_QPATH];

	memset(&refdef, 0, sizeof(refdef));

	refdef.x = r_width->value / 2;
	refdef.y = r_height->value / 2 - 72 * vid_hudscale->value;
	refdef.width = 144 * vid_hudscale->value;
	refdef.height = 168 * vid_hudscale->value;
	refdef.fov_x = 40;
	refdef.fov_y = Render3D_CalcFov(refdef.fov_x, refdef.width, refdef.height);
	refdef.time = cls.realtime * 0.001f;

	if (s_pmi[s_player_model_box.curvalue].skindisplaynames)
	{
		static int32_t yaw;
		entity_t entity;

		memset(&entity, 0, sizeof(entity));

		Com_sprintf(scratch, sizeof(scratch), "players/%s/tris.md2", s_pmi[s_player_model_box.curvalue].directory);
		entity.model = re.RegisterModel(scratch);
		Com_sprintf(scratch, sizeof(scratch), "players/%s/%s.tga", s_pmi[s_player_model_box.curvalue].directory, s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);
		entity.skin = re.RegisterSkin(scratch);
		entity.flags = RF_FULLBRIGHT;
		entity.origin[0] = 80;
		entity.origin[1] = 0;
		entity.origin[2] = 0;
		VectorCopy3(entity.origin, entity.oldorigin);
		entity.frame = 0;
		entity.oldframe = 0;
		entity.backlerp = 0.0;
		entity.angles[1] = yaw++;
		if (++yaw > 360)
			yaw -= 360;

		refdef.areabits = 0;
		refdef.num_entities = 1;
		refdef.entities = &entity;
		refdef.lightstyles = 0;
		refdef.rdflags = RDF_NOWORLDMODEL;

		Menu_Draw(&s_player_config_menu);

		Menu_DrawTextBox((refdef.x) * (320.0F / r_width->value) - 8 * vid_hudscale->value, (r_height->value / 2) * (240.0F / r_height->value) - 77 * vid_hudscale->value, refdef.width / (8 * vid_hudscale->value), refdef.height / (8 * vid_hudscale->value));
		refdef.height += 4 * vid_hudscale->value;

		re.RenderFrame(&refdef);

		Com_sprintf(scratch, sizeof(scratch), "/players/%s/%s_i.tga",
			s_pmi[s_player_model_box.curvalue].directory,
			s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);
		re.DrawPic(s_player_config_menu.x - 40 * vid_hudscale->value, refdef.y, scratch, NULL, false);
	}
}

const char* PlayerConfig_MenuKey(int32_t key)
{
	int32_t i;

	if (key == K_ESCAPE)
	{
		char scratch[1024];

		Cvar_Set("name", s_player_name_field.buffer);

		Com_sprintf(scratch, sizeof(scratch), "%s/%s",
			s_pmi[s_player_model_box.curvalue].directory,
			s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);

		Cvar_Set("skin", scratch);

		for (i = 0; i < s_numplayermodels; i++)
		{
			int32_t j;

			for (j = 0; j < s_pmi[i].nskins; j++)
			{
				if (s_pmi[i].skindisplaynames[j])
					free(s_pmi[i].skindisplaynames[j]);
				s_pmi[i].skindisplaynames[j] = 0;
			}
			free(s_pmi[i].skindisplaynames);
			s_pmi[i].skindisplaynames = 0;
			s_pmi[i].nskins = 0;
		}
	}
	return Default_MenuKey(&s_player_config_menu, key);
}


void M_Menu_PlayerConfig_f()
{
	if (!PlayerConfig_MenuInit())
	{
		Menu_SetStatusBar(&s_multiplayer_menu, "[STRING_ERROR_NO_PLAYER_MODELS]");
		return;
	}
	Menu_SetStatusBar(&s_multiplayer_menu, NULL);
	M_PushMenu(PlayerConfig_MenuDraw, PlayerConfig_MenuKey);
}


/*
=======================================================================

QUIT MENU

=======================================================================
*/

const char* M_Quit_Key(int32_t key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_N:
		M_PopMenu();
		break;

	case K_Y:
		cls.input_dest = key_console;
		CL_Quit_f();
		break;

	default:
		break;
	}

	return NULL;

}


void M_Quit_Draw()
{
	int32_t 	w, h;

	re.DrawGetPicSize(&w, &h, "2d/quit");
	re.DrawPic((r_width->value - w) / 2, (r_height->value - h) / 2, "2d/quit", NULL, false);
}


void M_Menu_Quit_f()
{
	M_PushMenu(M_Quit_Draw, M_Quit_Key);
}



//=============================================================================
/* Menu Subsystem */


/*
=================
M_Init
=================
*/
void M_Init()
{
	Cmd_AddCommand("menu_main", M_Menu_Main_f);
	Cmd_AddCommand("menu_game", M_Menu_Game_f);
	Cmd_AddCommand("menu_loadgame", M_Menu_LoadGame_f);
	Cmd_AddCommand("menu_savegame", M_Menu_SaveGame_f);
	Cmd_AddCommand("menu_joinserver", M_Menu_JoinServer_f);
	Cmd_AddCommand("menu_addressbook", M_Menu_AddressBook_f);
	Cmd_AddCommand("menu_startserver", M_Menu_StartServer_f);
	Cmd_AddCommand("menu_gameoptions", M_Menu_GameOptions_f);
	Cmd_AddCommand("menu_playerconfig", M_Menu_PlayerConfig_f);
	Cmd_AddCommand("menu_downloadoptions", M_Menu_DownloadOptions_f);
	Cmd_AddCommand("menu_credits", M_Menu_Credits_f);
	Cmd_AddCommand("menu_multiplayer", M_Menu_Multiplayer_f);
	Cmd_AddCommand("menu_video", M_Menu_Video_f);
	Cmd_AddCommand("menu_options", M_Menu_Options_f);
	Cmd_AddCommand("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand("menu_quit", M_Menu_Quit_f);
}


/*
=================
M_Draw
=================
*/
void M_Draw()
{
	if (cls.input_dest != key_menu)
		return;

	// repaint32_t everything next frame
	Render2D_DirtyScreen();

	re.DrawFadeScreen();

	m_drawfunc();

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		S_StartLocalSound(menu_input_sound);
		m_entersound = false;
	}
}


/*
=================
M_Keydown
=================
*/
void M_Keydown(int32_t key, int32_t mods)
{
	const char* s;

	if (m_keyfunc)
		if ((s = m_keyfunc(key)) != 0)
			S_StartLocalSound((char*)s);
}


