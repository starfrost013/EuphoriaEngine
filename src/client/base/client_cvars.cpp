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
// client_cvars.c - client cvars

#include <client/client.hpp>

cvar_t* r_width;
cvar_t* r_height;

cvar_t* freelook;

cvar_t* adr0;
cvar_t* adr1;
cvar_t* adr2;
cvar_t* adr3;
cvar_t* adr4;
cvar_t* adr5;
cvar_t* adr6;
cvar_t* adr7;
cvar_t* adr8;

cvar_t* rcon_client_password;
cvar_t* rcon_address;

cvar_t* cl_noskins;
cvar_t* cl_autoskins;
cvar_t* cl_footsteps;
cvar_t* cl_timeout;
cvar_t* cl_predict;
cvar_t* cl_maxfps;
cvar_t* cl_gun;

cvar_t* cl_add_particles;
cvar_t* cl_add_lights;
cvar_t* cl_add_entities;
cvar_t* cl_add_blend;

cvar_t* cl_drawhud;

cvar_t* cl_shownet;
cvar_t* cl_showmiss;
cvar_t* cl_showclamp;
cvar_t* cl_showinfo;

cvar_t* cl_showintro;

// until UI scripts implemented
cvar_t* cl_intro1;
cvar_t* cl_intro2;
cvar_t* cl_intro1_time;
cvar_t* cl_intro2_time;

cvar_t* cl_paused;
cvar_t* cl_timedemo;

cvar_t* cl_console_fraction;
cvar_t* cl_console_disabled;

cvar_t* lookstrafe;
cvar_t* sensitivity;

cvar_t* m_pitch;
cvar_t* m_yaw;
cvar_t* m_forward;
cvar_t* m_side;

cvar_t* cl_lightlevel;

//
// userinfo
//
cvar_t* info_password;
cvar_t* info_spectator;
cvar_t* name;
cvar_t* skin;
cvar_t* fov;
cvar_t* msg;
cvar_t* hand;
cvar_t* gender;
cvar_t* gender_auto;

cvar_t* cl_vwep;

void CL_InitCvars()
{
	// start with address book (probably this feature will be removed or renamed)

	adr0 = Cvar_Get("adr0", "", CVAR_ARCHIVE);
	adr1 = Cvar_Get("adr1", "", CVAR_ARCHIVE);
	adr2 = Cvar_Get("adr2", "", CVAR_ARCHIVE);
	adr3 = Cvar_Get("adr3", "", CVAR_ARCHIVE);
	adr4 = Cvar_Get("adr4", "", CVAR_ARCHIVE);
	adr5 = Cvar_Get("adr5", "", CVAR_ARCHIVE);
	adr6 = Cvar_Get("adr6", "", CVAR_ARCHIVE);
	adr7 = Cvar_Get("adr7", "", CVAR_ARCHIVE);
	adr8 = Cvar_Get("adr8", "", CVAR_ARCHIVE);

	//
	// register our variables
	//
	cl_add_blend = Cvar_Get("cl_blend", "1", 0);
	cl_add_lights = Cvar_Get("cl_lights", "1", 0);
	cl_add_particles = Cvar_Get("cl_particles", "1", 0);
	cl_add_entities = Cvar_Get("cl_entities", "1", 0);
	cl_drawhud = Cvar_Get("cl_drawhud", "1", 0);
	cl_gun = Cvar_Get("cl_gun", "1", 0);
	cl_footsteps = Cvar_Get("cl_footsteps", "1", 0);
	cl_noskins = Cvar_Get("cl_noskins", "0", 0);
	cl_autoskins = Cvar_Get("cl_autoskins", "0", 0);
	cl_predict = Cvar_Get("cl_predict", "1", 0);
	cl_maxfps = Cvar_Get("cl_maxfps", "90", 0);

	cl_upspeed = Cvar_Get("cl_upspeed", "200", 0);
	cl_forwardspeed = Cvar_Get("cl_forwardspeed", "200", 0);
	cl_sidespeed = Cvar_Get("cl_sidespeed", "200", 0);
	cl_yawspeed = Cvar_Get("cl_yawspeed", "140", 0);
	cl_pitchspeed = Cvar_Get("cl_pitchspeed", "150", 0);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);

	cl_run = Cvar_Get("cl_run", "0", CVAR_ARCHIVE);
	freelook = Cvar_Get("freelook", "0", CVAR_ARCHIVE);
	lookstrafe = Cvar_Get("lookstrafe", "0", CVAR_ARCHIVE);
	sensitivity = Cvar_Get("sensitivity", "3", CVAR_ARCHIVE);

	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get("m_yaw", "0.022", 0);
	m_forward = Cvar_Get("m_forward", "1", 0);
	m_side = Cvar_Get("m_side", "1", 0);

	cl_shownet = Cvar_Get("cl_shownet", "0", 0);
	cl_showmiss = Cvar_Get("cl_showmiss", "0", 0);
	cl_showclamp = Cvar_Get("showclamp", "0", 0);
#ifndef NDEBUG
	cl_showinfo = Cvar_Get("cl_showinfo", "1", 0);
#else
	cl_showinfo = Cvar_Get("cl_showinfo", "0", 0);
#endif
	cl_timeout = Cvar_Get("cl_timeout", "120", 0);
	cl_paused = Cvar_Get("paused", "0", 0);
	cl_timedemo = Cvar_Get("timedemo", "0", 0);

	cl_console_fraction = Cvar_Get("cl_console_fraction", "0.5", 0);
	cl_console_disabled = Cvar_Get("cl_console_disabled", "0", 0);

	cl_showintro = Cvar_Get("cl_showintro", "1", CVAR_ARCHIVE);
	cl_intro1 = Cvar_Get("cl_intro1", "2d/ui/introui_background2", CVAR_ARCHIVE);
	cl_intro2 = Cvar_Get("cl_intro2", "2d/ui/introui_background", CVAR_ARCHIVE);

	cl_intro1_time = Cvar_Get("cl_intro1_time", "7500", CVAR_ARCHIVE);
	cl_intro2_time = Cvar_Get("cl_intro2_time", "7500", CVAR_ARCHIVE);

	rcon_client_password = Cvar_Get("rcon_password", "", 0);
	rcon_address = Cvar_Get("rcon_address", "", 0);

	cl_lightlevel = Cvar_Get("r_lightlevel", "0", 0);

	//
	// userinfo
	//
	info_password = Cvar_Get("password", "", CVAR_USERINFO);
	info_spectator = Cvar_Get("spectator", "0", CVAR_USERINFO);
	name = Cvar_Get("name", "unnamed", CVAR_USERINFO | CVAR_ARCHIVE);
	skin = Cvar_Get("skin", "male/grunt", CVAR_USERINFO | CVAR_ARCHIVE);
	msg = Cvar_Get("msg", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	hand = Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	fov = Cvar_Get("fov", "90", CVAR_USERINFO | CVAR_ARCHIVE);
	gender = Cvar_Get("gender", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	gender_auto = Cvar_Get("gender_auto", "1", CVAR_ARCHIVE);
	gender->modified = false; // clear this so we know when user sets it manually

	cl_vwep = Cvar_Get("cl_vwep", "1", CVAR_ARCHIVE);
}