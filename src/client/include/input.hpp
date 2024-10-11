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
// input.h -- external input devices
#pragma once
#include "input_keys.hpp"

extern int32_t 	window_center_x, window_center_y;
extern bool		mouse_active;

void Input_Init();					
void Input_StartupMouse();
void Input_MouseActivate();
void Input_MouseDeactivate();
void Input_MouseMove(usercmd_t* cmd);

void Input_Shutdown();

// provides an opportunity for devices to stick commands on the script buffer
void Input_Frame();

void Input_Move(usercmd_t* cmd);
// add additional movement on top of the keyboard move cmd

void Input_Activate(bool activated);

// Keyboard stuff

typedef struct
{
	int32_t 		down[2];		// key nums holding it down
	uint32_t		downtime;		// msec timestamp
	uint32_t		msec;			// msec down this frame
	int32_t 		state;			// state of this key
} kbutton_t;

extern kbutton_t	input_mlook;
extern kbutton_t 	input_sprint;

void CL_InitInput();
void CL_SendCmd();

void CL_ClearState();

void CL_ReadPackets();

void CL_BaseMove(usercmd_t* cmd);

extern double last_mouse_pos_x, last_mouse_pos_y;

float CL_KeyState(kbutton_t* key);
const char* Key_VirtualToPhysical(int32_t keynum, bool shift);

extern uint32_t	sys_frame_time;

extern char*	keybindings[NUM_KEYS];
extern int32_t 	Key_repeats[NUM_KEYS];

extern int32_t	anykeydown;
extern char		chat_buffer[];
extern int32_t	chat_bufferlen;
extern bool		chat_team;

// glfw evetts
void Key_Event(void* unused, int32_t key, int32_t scancode, int32_t action, int32_t mods);
void MouseClick_Event(void* unused, int32_t button, int32_t action, int32_t mods);
void MouseMove_Event(void* unused, double xpos, double ypos);
void MouseScroll_Event(void* unused, double xoffset, double yoffset);
void WindowFocus_Event(void* unused, int32_t focused);
void WindowIconify_Event(void* unused, int32_t iconified);

// zombono events
void Input_Event(int32_t key, int32_t mods, bool down, uint32_t time, int32_t x, int32_t y);
void Key_Init();
void Key_WriteBindings(FILE* f);
void Key_SetBinding(int32_t keynum, const char* binding);
void Key_ClearStates();