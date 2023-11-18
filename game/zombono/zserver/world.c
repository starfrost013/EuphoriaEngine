#include "defs.h"

/*  Copyright (C) 1996-1997  Id Software, Inc.
	Copyright (C) 2023		 starfrost

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.

	| | | | | ZOMBONO | | | | | 
		© 2023 starfrost!!!
	| | | | | | | | | | | | | |

	World.qc : World setup code
*/

void InitBodyQue();

entvars_t lastspawn;

//=======================
/*QUAKED worldspawn (0 0 0) ?
Only used for the world entity.
Set message to the level name.
Set sounds to the cd track to play.

World Types:
0: medieval
1: metal
2: base
*/
//=======================
void worldspawn()
{
	lastspawn = world;
	InitBodyQue();

// custom map attributes
	cvar_set("sv_gravity", "800");

// the area based ambient sounds MUST be the first precache_sounds

// player precaches
	W_Precache(); // get weapon precaches

// sounds used from C physics code
	precache_sound("demon/dland2.wav"); // landing thud
	precache_sound("misc/h2ohit1.wav"); // landing splash

// setup precaches allways needed
	precache_sound("items/itembk2.wav"); // item respawn sound
	precache_sound("player/plyrjmp8.wav"); // player jump
	precache_sound("player/land.wav"); // player landing
	precache_sound("player/land2.wav"); // player hurt landing
	precache_sound("player/drown1.wav"); // drowning pain
	precache_sound("player/drown2.wav"); // drowning pain
	precache_sound("player/gasp1.wav"); // gasping for air
	precache_sound("player/gasp2.wav"); // taking breath
	precache_sound("player/h2odeath.wav"); // drowning death

	precache_sound("misc/talk.wav"); // talk
	precache_sound("player/teledth1.wav"); // telefrag
	precache_sound("misc/r_tele1.wav"); // teleport sounds
	precache_sound("misc/r_tele2.wav");
	precache_sound("misc/r_tele3.wav");
	precache_sound("misc/r_tele4.wav");
	precache_sound("misc/r_tele5.wav");
	precache_sound("weapons/tink1.wav"); // zombie fail
	precache_sound("weapons/lock4.wav"); // ammo pick up
	precache_sound("weapons/pkup.wav"); // weapon up
	precache_sound("items/armor1.wav"); // armor up
	precache_sound("weapons/lhit.wav"); //lightning
	precache_sound("weapons/lstart.wav"); //lightning start
	precache_sound("items/damage3.wav");
	precache_sound("items/double1.wav"); // double jump
	precache_sound("misc/power.wav"); //lightning for boss

// player gib sounds
	precache_sound("player/gib.wav"); // player gib sound
	precache_sound("player/udeath.wav"); // player gib sound
	precache_sound("player/tornoff2.wav"); // gib sound

// player pain sounds

	precache_sound("player/pain1.wav");
	precache_sound("player/pain2.wav");
	precache_sound("player/pain3.wav");
	precache_sound("player/pain4.wav");
	precache_sound("player/pain5.wav");
	precache_sound("player/pain6.wav");

// player death sounds
	precache_sound("player/death1.wav");
	precache_sound("player/death2.wav");
	precache_sound("player/death3.wav");
	precache_sound("player/death4.wav");
	precache_sound("player/death5.wav");

// ax sounds
	precache_sound("weapons/ax1.wav"); // ax swoosh
	precache_sound("player/axhit1.wav"); // ax hit meat
	precache_sound("player/axhit2.wav"); // ax hit world

	precache_sound("player/h2ojump.wav"); // player jumping into water
	precache_sound("player/slimbrn2.wav"); // player enter slime
	precache_sound("player/inh2o.wav"); // player enter water
	precache_sound("player/inlava.wav"); // player enter lava
	precache_sound("misc/outwater.wav"); // leaving water sound

	precache_sound("player/lburn1.wav"); // lava burn
	precache_sound("player/lburn2.wav"); // lava burn

	precache_sound("misc/water1.wav"); // swimming
	precache_sound("misc/water2.wav"); // swimming

	precache_model("progs/player.mdl");
	precache_model("progs/eyes.mdl");
	precache_model("progs/h_player.mdl");
	precache_model("progs/gib1.mdl");
	precache_model("progs/gib2.mdl");
	precache_model("progs/gib3.mdl");

	precache_model("progs/s_bubble.spr"); // drowning bubbles
	precache_model("progs/s_explod.spr"); // sprite explosion

	precache_model("progs/v_axe.mdl");
	precache_model("progs/v_shot.mdl");
	precache_model("progs/v_nail.mdl");
	precache_model("progs/v_rock.mdl");
	precache_model("progs/v_shot2.mdl");
	precache_model("progs/v_nail2.mdl");
	precache_model("progs/v_rock2.mdl");

	precache_model("progs/bolt.mdl"); // for lightning gun
	precache_model("progs/bolt2.mdl"); // for lightning gun
	precache_model("progs/bolt3.mdl"); // for boss shock
	precache_model("progs/lavaball.mdl"); // for testing

	precache_model("progs/missile.mdl");
	precache_model("progs/grenade.mdl");
	precache_model("progs/spike.mdl");
	precache_model("progs/s_spike.mdl");

	precache_model("progs/backpack.mdl");

	// ***THESE ARE REQUIRED FOR ZOMBONO BECAUSE ZOMBIES CAN BE SPAWNED OUTSIDE OF ENTITIES**
	precache_model("progs/zombie.mdl");
	precache_model("progs/h_zombie.mdl");
	precache_model("progs/zom_gib.mdl");

	precache_sound("zombie/z_idle.wav");
	precache_sound("zombie/z_idle1.wav");
	precache_sound("zombie/z_shot1.wav");
	precache_sound("zombie/z_gib.wav");
	precache_sound("zombie/z_pain.wav");
	precache_sound("zombie/z_pain1.wav");
	precache_sound("zombie/z_fall.wav");
	precache_sound("zombie/z_miss.wav");
	precache_sound("zombie/z_hit.wav");
	precache_sound("zombie/idle_w2.wav");

	precache_model("progs/v_light.mdl");

//
// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
//

	// 0 normal
	lightstyle(0, "m");

	// 1 FLICKER(first variety)
	lightstyle(1, "mmnmmommommnonmmonqnmmo");

	// 2 SLOW STRONG PULSE
	lightstyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// 3 CANDLE(first variety)
	lightstyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	// 4 FAST STROBE
	lightstyle(4, "mamamamamama");

	// 5 GENTLE PULSE 1
	lightstyle(5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	// 6 FLICKER(second variety)
	lightstyle(6, "nmonqnmomnmomomno");

	// 7 CANDLE(second variety)
	lightstyle(7, "mmmaaaabcdefgmmmmaaaammmaamm");

	// 8 CANDLE(third variety)
	lightstyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	// 9 SLOW STROBE(fourth variety)
	lightstyle(9, "aaaaaaaazzzzzzzz");

	// 10 FLUORESCENT FLICKER
	lightstyle(10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	lightstyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	lightstyle(63, "a");
}

void StartFrame()
{
	// setup cvars
	friendly_fire = cvar("friendly_fire");
	skill = cvar("skill");
	sv_cheats = cvar("sv_cheats");

	// Get game mode
	gamemode = cvar("gamemode");

	vid_width = cvar("vid_width");
	vid_height = cvar("vid_height");

	framecount = framecount + 1;
}

/*
==============================================================================

BODY QUE

==============================================================================
*/

entvars_t bodyque_head;

void bodyque()
{ // just here so spawn functions don't complain after the world
	// creates bodyques
}

void InitBodyQue()
{
	bodyque_head = spawn();
	bodyque_head.classname = "bodyque";
	bodyque_head.owner = spawn();
	bodyque_head.owner.classname = "bodyque";
	bodyque_head.owner.owner = spawn();
	bodyque_head.owner.owner.classname = "bodyque";
	bodyque_head.owner.owner.owner = spawn();
	bodyque_head.owner.owner.owner.classname = "bodyque";
	bodyque_head.owner.owner.owner.owner = bodyque_head;
}


// make a body que entry for the given ent so the ent can be
// respawned elsewhere
void CopyToBodyQue(entvars_t ent)
{
	bodyque_head.angles = ent.angles;
	bodyque_head.model = ent.model;
	bodyque_head.modelindex = ent.modelindex;
	bodyque_head.frame = ent.frame;
	bodyque_head.colormap = ent.colormap;
	bodyque_head.movetype = ent.movetype;
	bodyque_head.velocity = ent.velocity;
	bodyque_head.flags = 0;
	setorigin(bodyque_head, ent.origin);
	setsize(bodyque_head, ent.mins, ent.maxs);
	bodyque_head = bodyque_head.owner;
}