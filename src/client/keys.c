/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018-2019 Krzysztof Kondrak

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
#include <ctype.h>

/*

key up events are sent even if in console mode

*/

#define		MAXCMDLINE	256
char	key_lines[128][MAXCMDLINE];
int32_t 	key_linepos;
int32_t 	shift_down=false;
int32_t anykeydown;

int32_t 	edit_line=0;
int32_t 	history_line=0;

int32_t 	key_waiting;
char	*keybindings[NUM_KEYS];
bool	consolekeys[NUM_KEYS];	// if true, can't be rebound while in console
bool	menubound[NUM_KEYS];	// if true, can't be rebound while in menu
int32_t 	keyshift[NUM_KEYS];		// key to map to if shift held down in console
int32_t 	key_repeats[NUM_KEYS];	// if > 1, it is autorepeating
bool	keydown[NUM_KEYS];

typedef struct
{
	char	*name;
	int32_t 	keynum;
} keyname_t;

// Used for all keys for virtual keyboard support
keyname_t keynames[] =
{
	{ "0", K_0 },
	{ "1", K_1 },
	{ "2", K_2 },
	{ "3", K_3 },
	{ "4", K_4 },
	{ "5", K_5 },
	{ "6", K_6 },
	{ "7", K_7 },
	{ "8", K_8 },
	{ "9", K_9 },

	{ "a", K_A },
	{ "b", K_B },
	{ "c", K_C },
	{ "d", K_D },
	{ "e", K_E },
	{ "f", K_F },
	{ "g", K_G },
	{ "h", K_H },
	{ "i", K_I },
	{ "j", K_J },
	{ "k", K_K },
	{ "l", K_L },
	{ "m", K_M },
	{ "n", K_N },
	{ "o", K_O },
	{ "p", K_P },
	{ "q", K_Q },
	{ "r", K_R },
	{ "s", K_S },
	{ "t", K_T },
	{ "u", K_U },
	{ "v", K_V },
	{ "w", K_W },
	{ "x", K_X },
	{ "y", K_Y },
	{ "z", K_Z },
	{ "6", K_6 },
	{ "7", K_7 },
	{ "8", K_8 },
	{ "9", K_9 },

	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{ "ESCAPE", K_ESCAPE },
	{ "SPACE", K_SPACE },
	{ "BACKSPACE", K_BACKSPACE },
	{ "UPARROW", K_UPARROW },
	{ "DOWNARROW", K_DOWNARROW },
	{ "LEFTARROW", K_LEFTARROW },
	{ "RIGHTARROW", K_RIGHTARROW },

	{ "ALT", K_ALT },
	{ "CTRL", K_CTRL },
	{ "SHIFT", K_SHIFT },

	{ "F1", K_F1 },
	{ "F2", K_F2 },
	{ "F3", K_F3 },
	{ "F4", K_F4 },
	{ "F5", K_F5 },
	{ "F6", K_F6 },
	{ "F7", K_F7 },
	{ "F8", K_F8 },
	{ "F9", K_F9 },
	{ "F10", K_F10 },
	{ "F11", K_F11 },
	{ "F12", K_F12 },
	{ "F13", K_F13 },
	{ "F14", K_F14 },
	{ "F15", K_F15 },
	{ "F16", K_F16 },
	{ "F17", K_F17 },
	{ "F18", K_F18 },
	{ "F19", K_F19 },
	{ "F20", K_F20 },
	{ "F21", K_F21 },
	{ "F22", K_F22 },
	{ "F23", K_F23 },
	{ "F24", K_F24 },

	{ "INS", K_INSERT },
	{ "DEL", K_DELETE },
	{ "PGDN", K_PAGE_DOWN },
	{ "PGUP", K_PAGE_UP },
	{ "HOME", K_HOME },
	{ "END", K_END },

	{ " ", K_SPACE },
	{ "'", K_APOSTROPHE },
	{ ",", K_COMMA },
	{ "-", K_MINUS },
	{ ".", K_PERIOD },
	{ "/", K_SLASH },
	{ "[", K_LEFT_BRACKET },
	{ "\\", K_BACKSLASH },
	{ "]", K_RIGHT_BRACKET },

	{ "MOUSE1", K_MOUSE1 },
	{ "MOUSE2", K_MOUSE2 },
	{ "MOUSE3", K_MOUSE3 },
	{ "MOUSE4", K_MOUSE4 },
	{ "MOUSE5", K_MOUSE5 },
	{ "MOUSE6", K_MOUSE6 },
	{ "MOUSE7", K_MOUSE7 },
	{ "MOUSE8", K_MOUSE8 },

	{ "JOY1", K_JOY1 },
	{ "JOY2", K_JOY2 },
	{ "JOY3", K_JOY3 },
	{ "JOY4", K_JOY4 },

	{ "AUX1", K_AUX1 },
	{ "AUX2", K_AUX2 },
	{ "AUX3", K_AUX3 },
	{ "AUX4", K_AUX4 },
	{ "AUX5", K_AUX5 },
	{ "AUX6", K_AUX6 },
	{ "AUX7", K_AUX7 },
	{ "AUX8", K_AUX8 },
	{ "AUX9", K_AUX9 },
	{ "AUX10", K_AUX10 },
	{ "AUX11", K_AUX11 },
	{ "AUX12", K_AUX12 },
	{ "AUX13", K_AUX13 },
	{ "AUX14", K_AUX14 },
	{ "AUX15", K_AUX15 },
	{ "AUX16", K_AUX16 },
	{ "AUX17", K_AUX17 },
	{ "AUX18", K_AUX18 },
	{ "AUX19", K_AUX19 },
	{ "AUX20", K_AUX20 },
	{ "AUX21", K_AUX21 },
	{ "AUX22", K_AUX22 },
	{ "AUX23", K_AUX23 },
	{ "AUX24", K_AUX24 },
	{ "AUX25", K_AUX25 },
	{ "AUX26", K_AUX26 },
	{ "AUX27", K_AUX27 },
	{ "AUX28", K_AUX28 },
	{ "AUX29", K_AUX29 },
	{ "AUX30", K_AUX30 },
	{ "AUX31", K_AUX31 },
	{ "AUX32", K_AUX32 },

	{ "WORLD1", K_WORLD_1 },
	{ "WORLD2", K_WORLD_2 },

	{ "KP_HOME",			K_HOME },
	{ "UPARROW",		K_UPARROW },
	{ "KP_PGUP",			K_PAGE_UP },
	{ "LEFTARROW",	K_LEFTARROW },
	{ "KP_0",			K_KP_0 },
	{ "KP_1",			K_KP_1 },
	{ "KP_2",			K_KP_2 },
	{ "KP_3",			K_KP_3 },
	{ "KP_4",			K_KP_4 },
	{ "KP_5",			K_KP_5 },
	{ "KP_6",			K_KP_6 },
	{ "KP_7",			K_KP_7 },
	{ "KP_8",			K_KP_8 },
	{ "KP_9",			K_KP_9 },
	{ "RIGHTARROW",	K_RIGHTARROW },
	{ "KP_END",			K_END },
	{ "DOWNARROW",	K_DOWNARROW },
	{ "KP_PGDN",			K_PAGE_DOWN },
	{ "KP_ADD",			K_KP_ADD },
	{ "KP_SLASH",		K_KP_DIVIDE },
	{ "KP_MINUS",		K_KP_SUBTRACT },
	{ "KP_DECIMAL",		K_KP_DECIMAL },
	{ "KP_ENTER",		K_KP_ENTER },
	{ "KP_EQUAL",		K_KP_EQUAL },
	{ "KP_PLUS",		VK_OEM_PLUS },

	{ "LEFT SHIFT", K_LEFT_SHIFT },
	{ "LEFT CTRL", K_LEFT_CONTROL },
	{ "LEFT ALT", K_LEFT_ALT },
#ifdef _WIN32
	{ "LEFT SUPER/WINDOWS KEY", K_LEFT_SUPER },
#else
	{ "LEFT SUPER", K_LEFT_SUPER },
#endif

	{ "RIGHT SHIFT", K_RIGHT_SHIFT },
	{ "RIGHT CTRL", K_RIGHT_CONTROL },
	{ "RIGHT ALT", K_RIGHT_ALT },
	{ "RIGHT SUPER", K_RIGHT_SUPER },

	{ "MENU", K_MENU }, 
	{ "CAPS LOCK", K_CAPS_LOCK },
	{ "SCROLL LOCK", K_SCROLL_LOCK },
	{ "NUM LOCK", K_NUM_LOCK },
	{ "PRINT SCREEN", K_PRINT_SCREEN },
	{ "PAUSE", K_PAUSE },
	{ "MWHEELUP", K_MWHEELUP },
	{ "MWHEELDOWN", K_MWHEELDOWN },

	{ "SEMICOLON", ';' },	// because a raw semicolon seperates commands

	{NULL,0}
};

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

void CompleteCommand (void)
{
	char	*cmd, *s;

	s = key_lines[edit_line]+1;
	if (*s == '\\' || *s == '/')
		s++;

	cmd = Cmd_CompleteCommand (s);
	if (!cmd)
		cmd = Cvar_CompleteVariable (s);
	if (cmd)
	{
		key_lines[edit_line][1] = '/';
		strcpy (key_lines[edit_line]+2, cmd);
		key_linepos = (int32_t)strlen(cmd)+2;
		key_lines[edit_line][key_linepos] = ' ';
		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
		return;
	}
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console (int32_t key)
{

	switch ( key )
	{
	case K_SLASH:
		key = '/';
		break;
	case K_MINUS:
		key = '-';
		break;
	case VK_OEM_PLUS:
		key = '+';
		break;
	case K_KP_0:
		key = K_0;
		break;
	case K_KP_1:
		key = K_1;
		break;
	case K_KP_2:
		key = K_2;
		break;
	case K_KP_3:
		key = K_3;
		break;
	case K_KP_4:
		key = K_4;
		break;
	case K_KP_5:
		key = K_5;
		break;
	case K_KP_6:
		key = K_6;
		break;
	case K_KP_7:
		key = K_7;
		break;
	case K_KP_8:
		key = K_8;
		break;
	case K_KP_9:
		key = K_8;
		break;
	case K_DELETE:
		key = '.';
		break;
	}

	if ( ( toupper( key ) == K_V && keydown[K_CTRL] ) ||
		 ( ( ( key == K_INSERT ) || ( key == K_INSERT ) ) && keydown[K_SHIFT] ) )
	{
		char *cbd;
		
		if ( ( cbd = Sys_GetClipboardData() ) != 0 )
		{
			int32_t i;

			strtok( cbd, "\n\r\b" );

			i = (int32_t)strlen( cbd );
			if ( i + key_linepos >= MAXCMDLINE)
				i= MAXCMDLINE - key_linepos;

			if ( i > 0 )
			{
				cbd[i]=0;
				strcat( key_lines[edit_line], cbd );
				key_linepos += i;
			}
			free( cbd );
		}

		return;
	}

	if ( key == K_L ) 
	{
		if ( keydown[K_CTRL] )
		{
			Cbuf_AddText ("clear\n");
			return;
		}
	}

	if ( key == K_ENTER || key == K_KP_ENTER )
	{	// backslash text are commands, else chat
		if (key_lines[edit_line][1] == '\\' || key_lines[edit_line][1] == '/')
			Cbuf_AddText (key_lines[edit_line]+2);	// skip the >
		else
			Cbuf_AddText (key_lines[edit_line]+1);	// valid command

		Cbuf_AddText ("\n");
		Com_Printf ("%s\n",key_lines[edit_line]);
		edit_line = (edit_line + 1) & 31;
		history_line = edit_line;
		key_lines[edit_line][0] = ']';
		key_linepos = 1;
		if (cls.state == ca_disconnected)
			SCR_UpdateScreen ();	// force an update, because the command
									// may take some time
		return;
	}

	if (key == K_TAB)
	{	// command completion
		CompleteCommand ();
		return;
	}
	
	if ( ( key == K_BACKSPACE ) || ( key == K_LEFTARROW ) || ( key == K_LEFTARROW ) || ( ( key == 'h' ) && ( keydown[K_CTRL] ) ) )
	{
		if (key_linepos > 1)
			key_linepos--;
		return;
	}

	if ( ( key == K_UPARROW ) || ( key == K_UPARROW ) ||
		 ( ( key == 'p' ) && keydown[K_CTRL] ) )
	{
		do
		{
			history_line = (history_line - 1) & 31;
		} while (history_line != edit_line
				&& !key_lines[history_line][1]);
		if (history_line == edit_line)
			history_line = (edit_line+1)&31;
		strcpy(key_lines[edit_line], key_lines[history_line]);
		key_linepos = (int32_t)strlen(key_lines[edit_line]);
		return;
	}

	if ( ( key == K_DOWNARROW ) || ( key == K_DOWNARROW ) ||
		 ( ( key == K_N ) && keydown[K_CTRL] ) )
	{
		if (history_line == edit_line) return;
		do
		{
			history_line = (history_line + 1) & 31;
		}
		while (history_line != edit_line
			&& !key_lines[history_line][1]);
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = ']';
			key_linepos = 1;
		}
		else
		{
			strcpy(key_lines[edit_line], key_lines[history_line]);
			key_linepos = (int32_t)strlen(key_lines[edit_line]);
		}
		return;
	}

	if (key == K_PAGE_UP || key == K_MWHEELUP || key == K_PAGE_UP )
	{
		con.display -= 2;
		return;
	}

	if (key == K_PAGE_DOWN || key == K_MWHEELDOWN || key == K_PAGE_DOWN )
	{
		con.display += 2;
		if (con.display > con.current)
			con.display = con.current;
		return;
	}

	if (key == K_HOME || key == K_HOME )
	{
		con.display = con.current - con.totallines + 10;
		return;
	}

	if (key == K_END || key == K_END )
	{
		con.display = con.current;
		return;
	}
	
	if (key < 32 || key > 127)
		return;	// non printable
		
	// translate from virtual to physical keys
	char final_key = Key_KeynumToString(key)[0];

	if (key_linepos < MAXCMDLINE-1
		&& final_key != -1)
	{	
		key_lines[edit_line][key_linepos] = final_key;
		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
	}

}

//============================================================================

bool	chat_team;
char		chat_buffer[MAXCMDLINE];
int32_t 		chat_bufferlen = 0;

void Key_Message (int32_t key)
{

	if ( key == K_ENTER || key == K_KP_ENTER )
	{
		if (chat_team)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");
		Cbuf_AddText(chat_buffer);
		Cbuf_AddText("\"\n");

		cls.key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key == K_ESCAPE)
	{
		cls.key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (key == K_BACKSPACE)
	{
		if (chat_bufferlen)
		{
			chat_bufferlen--;
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	if (chat_bufferlen == sizeof(chat_buffer)-1)
		return; // all full

	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
}

//============================================================================


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int32_t Key_StringToKeynum (char *str)
{
	keyname_t	*kn;
	
	if (!str || !str[0])
		return -1;

	for (kn=keynames ; kn->name ; kn++)
	{
		if (!Q_strcasecmp(str,kn->name))
			return kn->keynum;
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
char *Key_KeynumToString (int32_t keynum)
{
	keyname_t	*kn;	
	static	char	tinystr[2];
	
	if (keynum == -1)
		return "<KEY NOT FOUND>";

	for (kn=keynames ; kn->name ; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int32_t keynum, char *binding)
{
	char	*new;
	int32_t 	l;
			
	if (keynum == -1)
		return;

// free old bindings
	if (keybindings[keynum])
	{
		Z_Free (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}
			
// allocate memory for new binding
	l = (int32_t)strlen (binding);
	new = Z_Malloc (l+1);
	strcpy (new, binding);
	new[l] = 0;
	keybindings[keynum] = new;	
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int32_t 	b;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Com_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding (b, "");
}

void Key_Unbindall_f (void)
{
	int32_t 	i;
	
	for (i=0 ; i<NUM_KEYS ; i++)
		if (keybindings[i])
			Key_SetBinding (i, "");
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int32_t 		i, c, b;
	char			cmd[1024];
	
	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Com_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keybindings[b])
			Com_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b] );
		else
			Com_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
		return;
	}
	
// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != (c-1))
			strcat (cmd, " ");
	}

	Key_SetBinding (b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f)
{
	int32_t 	i;

	for (i=0 ; i<NUM_KEYS ; i++)
		if (keybindings[i] && keybindings[i][0])
			fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
}


/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f (void)
{
	int32_t 	i;

	for (i=0 ; i<NUM_KEYS; i++)
		if (keybindings[i] && keybindings[i][0])
			Com_Printf ("%s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
}


/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int32_t 	i;

	for (i=0 ; i<128 ; i++)
	{
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}
	key_linepos = 1;
	
//
// init ascii characters in console mode
//
	for (i=32 ; i<128 ; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_KP_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_PAGE_UP] = true;
	consolekeys[K_PAGE_DOWN] = true;
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_INSERT] = true;
	consolekeys[K_DELETE] = true;
	consolekeys[K_SLASH] = true;
	consolekeys[VK_OEM_PLUS] = true;
	consolekeys[K_MINUS] = true;
	consolekeys[K_KP_5] = true;

	consolekeys['`'] = false;
	consolekeys['~'] = false;

	for (i=0 ; i<NUM_KEYS ; i++)
		keyshift[i] = i;
	for (i=0 ; i < 26 ; i++)
		keyshift[K_A + i] = 'A' + i;

	// TODO: these don't work 
	keyshift[K_1] = '!';
	keyshift[K_2] = '@';
	keyshift[K_3] = '#';
	keyshift[K_4] = '$';
	keyshift[K_5] = '%';
	keyshift[K_6] = '^';
	keyshift[K_7] = '&';
	keyshift[K_8] = '*';
	keyshift[K_9] = '(';
	keyshift[K_0] = ')';
	keyshift[K_MINUS] = '_';
	keyshift[K_EQUAL] = '+';
	keyshift[K_COMMA] = '<';
	keyshift[K_PERIOD] = '>';
	keyshift[K_SLASH] = '?';
	keyshift[K_SEMICOLON] = ':';
	keyshift[K_BACKSLASH] = '"';
	keyshift[K_LEFT_BRACKET] = '{';
	keyshift[K_RIGHT_BRACKET] = '}';
	keyshift[K_GRAVE_ACCENT] = '~';
	keyshift[K_BACKSLASH] = '|';

	menubound[K_ESCAPE] = true;
	for (i=0 ; i<12 ; i++)
		menubound[K_F1+i] = true;

//
// register our functions
//
	Cmd_AddCommand ("bind",Key_Bind_f);
	Cmd_AddCommand ("unbind",Key_Unbind_f);
	Cmd_AddCommand ("unbindall",Key_Unbindall_f);
	Cmd_AddCommand ("bindlist",Key_Bindlist_f);
}

// from GLFW (we don't link to it in zombono.exe)
#define KEY_RELEASED	0
#define KEY_PRESSED		1
#define KEY_REPEAT		2

#define MOUSE_RELEASED	KEY_RELEASED
#define MOUSE_PRESSED	KEY_PRESSED

double last_x_pos = 0, last_y_pos = 0;

void Key_Event(void* unused, int32_t key, int32_t scancode, int32_t action, int32_t mods)
{
	Input_Event(key, (action == KEY_PRESSED), 0, 0, 0);
}

void MouseClick_Event(void* unused, int32_t button, int32_t action, int32_t mods)
{
	// they are not contiguous numbers
	if (button >= 4)
	{
		Input_Event(K_MOUSE4 + button, (action == MOUSE_PRESSED), 0, last_x_pos, last_y_pos);
	}
	else
	{
		Input_Event(K_MOUSE1 + button, (action == MOUSE_PRESSED), 0, last_x_pos, last_y_pos);
	}
}

void MouseMove_Event(void* unused, double xpos, double ypos)
{
	last_x_pos = xpos;
	last_y_pos = ypos;
}

void WindowFocus_Event(void* unused, int32_t focused)
{
	if (!ui_active 
		|| (ui_active && current_ui->passive))
	{
		re.EnableCursor(!focused);
	}
	else
	{
		re.EnableCursor(true);
	}
}

void WindowIconify_Event(void* unused, int32_t iconified)
{
	if (!ui_active
		|| (ui_active && current_ui->passive))
	{
		re.EnableCursor(iconified);
	}
	else
	{
		re.EnableCursor(true);
	}
}

/*
===================
Input_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Input_Event (int32_t key, bool down, uint32_t time, int32_t x, int32_t y)
{
	char	*kb;
	char	cmd[1024];

	// hack for modal presses
	if (key_waiting == -1)
	{
		if (down)
			key_waiting = key;
		return;
	}

	keydown[key] = down;
	// ALT+ENTER fullscreen toggle
	if (down && keydown[K_ALT] && key == K_ENTER)
	{
		extern cvar_t *vid_fullscreen;
		Cvar_Set("vid_fullscreen", vid_fullscreen->value ? "0" : "1");
		vid_fullscreen->modified = true;
		return;
	}

	// update auto-repeat status
	if (down)
	{
 		key_repeats[key]++;
		if (key != K_BACKSPACE 
			&& key != K_PAUSE 
			&& key != K_PAGE_UP 
			&& key != K_PAGE_DOWN
			&& key_repeats[key] > 1)
			return;	// ignore most autorepeats
			
		if (key >= 200 && key != K_MWHEELUP && key != K_MWHEELDOWN && !keybindings[key])
			Com_Printf ("%s is unbound, hit F4 to set.\n", Key_KeynumToString (key) );
	}
	else
	{
		key_repeats[key] = 0;
	}

	// Just use physical keys here.
	// We can't use mods here in case we need to ever plug into an input system that doesn't recognise them.
	if (key == K_LEFT_SHIFT
		|| key == K_RIGHT_SHIFT)
	{
		shift_down = down;
	}


	// console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~')
	{
		if (!down)
			return;
		Con_ToggleConsole_f ();
		return;
	}

	// any key during the attract mode will bring up the menu
	if (cl.attractloop && cls.key_dest != key_menu &&
		!(key >= K_F1 && key <= K_F12))
		key = K_ESCAPE;

	// Send ZombonoUI events

	// no point handling a mousepress as a key event if we already handled i
	bool mouse_event_handled = false; 

	if (down)
	{
		if (key >= K_MOUSE1 && key <= K_MOUSE5)
		{
			// todo: send mouse button
			UI_HandleEventOnClickDown(key, x, y);
			mouse_event_handled = true; 
		}
		else if (key != K_ESCAPE
			&& !mouse_event_handled) // KEY IS FUCKING HARDBOUND
		{
			UI_HandleEventOnKeyDown(key);
		}
	}
	else
	{
		if (key >= K_MOUSE1 && key <= K_MOUSE5)
		{
			// todo: send mouse button
			UI_HandleEventOnClickUp(key, x, y);
		}
		else if (key != K_ESCAPE
			&& !mouse_event_handled) // KEY IS FUCKING HARDBOUND
		{
			UI_HandleEventOnKeyUp(key);
			mouse_event_handled = true;
		}
	}

	// menu key is hardcoded, so the user can never unbind it
	if (key == K_ESCAPE)
	{
		if (!down)
			return;

		if (cl.frame.playerstate.stats[STAT_LAYOUTS] && cls.key_dest == key_game)
		{	// put away help computer / inventory
			Cbuf_AddText ("cmd putaway\n");
			return;
		}
		switch (cls.key_dest)
		{
		case key_message:
			Key_Message (key);
			break;
		case key_menu:
			M_Keydown (key);
			break;
		case key_game:
		case key_console:
			M_Menu_Main_f ();
			break;
		default:
			Com_Error (ERR_FATAL, "Bad cls.key_dest");
		}
		return;
	}

	if (down)
	{
		if (key_repeats[key] == 1)
			anykeydown++;
	}
	else
	{
		anykeydown--;
		if (anykeydown < 0)
			anykeydown = 0;
	}

//
// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the kenum as a parameter, so multiple
// downs can be matched with ups
//
	if (!down)
	{
		kb = keybindings[key];
		if (kb && kb[0] == '+')
		{
			Com_sprintf (cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
			Cbuf_AddText (cmd);
		}
		if (keyshift[key] != key)
		{
			kb = keybindings[keyshift[key]];
			if (kb && kb[0] == '+')
			{
				Com_sprintf (cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
	if ( (cls.key_dest == key_menu && menubound[key])
	|| (cls.key_dest == key_console && !consolekeys[key])
	|| (cls.key_dest == key_game && ( cls.state == ca_active || !consolekeys[key] ) ) )
	{
		kb = keybindings[key];
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum and time as a parm
				Com_sprintf (cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
				Cbuf_AddText (cmd);
			}
			else
			{
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	if (!down)
		return;		// other systems only care about key down events

	if (shift_down)
		key = keyshift[key];

	switch (cls.key_dest)
	{
	case key_message:
		Key_Message (key);
		break;
	case key_menu:
		M_Keydown (key);
		break;

	case key_game:
	case key_console:
		Key_Console (key);
		break;
	default:
		Com_Error (ERR_FATAL, "Bad cls.key_dest");
	}
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int32_t 	i;

	anykeydown = false;

	for (i=0 ; i<256 ; i++)
	{
		if ( keydown[i] || key_repeats[i] )
			Input_Event( i, false, 0, 0, 0);
		keydown[i] = 0;
		key_repeats[i] = 0;
	}
}


/*
===================
Key_GetKey
===================
*/
int32_t Key_GetKey (void)
{
	key_waiting = -1;

	while (key_waiting == -1)
		Sys_SendKeyEvents ();

	return key_waiting;
}