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
// client.h -- primary header for client
#pragma once

#include <cmath>
#include <cstring>
#include <cstdarg>
#include <cstdbool>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include "include/ref.hpp"

#include "include/vid.hpp"
#include "include/render.hpp"
#include "include/sound.hpp"
#include "include/input.hpp"
#include "include/input_keys.hpp"
#include "include/console.hpp"
#include "include/sound_miniaudio.hpp"
#include "include/ui.hpp"

//=============================================================================

typedef struct frame_s
{
	bool			valid;			// cleared if delta parsing was invalid
	int32_t 		serverframe;
	int32_t 		servertime;		// server time the message is valid for (in msec)
	int32_t 		deltaframe;
	uint8_t			areabits[MAX_MAP_AREAS / 8];		// portalarea visibility bits
	player_state_t	playerstate;
	int32_t 		num_entities;
	int32_t 		parse_entities;	// non-masked index into cl_parse_entities array
} frame_t;

typedef struct centity_s
{
	entity_state_t	baseline;		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int32_t 		serverframe;		// if not current, this ent isn't in the frame

	int32_t 		trailcount;			// for diminishing grenade trails
	vec3_t			lerp_origin;		// for trails (variable hz)

	int32_t 		fly_stoptime;
} centity_t;

#define MAX_CLIENTWEAPONMODELS		20

struct image_s;

typedef struct clientinfo_s
{
	char			name[MAX_QPATH];
	char			cinfo[MAX_QPATH];
	struct image_s* skin;
	struct image_s* icon;
	char			iconname[MAX_QPATH];
	struct model_s* model;
	struct model_s* weaponmodel[MAX_CLIENTWEAPONMODELS];
} clientinfo_t;

extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
extern int32_t num_cl_weaponmodels;

#define	CMD_BACKUP			64	// allow a lot of command backups for very fast systems

//
// the client_state_t structure is wiped completely at every
// server map change
//
typedef struct client_state_s
{
	int32_t 		timeoutcount;

	int32_t 		timedemo_frames;
	int32_t 		timedemo_start;

	bool			refresh_prepped;	// false if on new level or new ref dll
	bool			sound_prepped;		// ambient sounds can start
	bool			force_refdef;		// vid has changed, so we can't use a paused refdef

	int32_t 		parse_entities;		// index (not anded off) into cl_parse_entities[]

	usercmd_t		cmd;
	usercmd_t		cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int32_t 		cmd_time[CMD_BACKUP];	// time sent, for calculating pings
	int16_t			predicted_origins[CMD_BACKUP][3];	// for debug comparing against server

	float			predicted_step;				// for stair up smoothing
	uint32_t		predicted_step_time;

	vec3_t			predicted_origin;	// generated by CL_PredictMovement
	vec3_t			predicted_angles;
	vec3_t			prediction_error;

	frame_t			frame;				// received from server
	frame_t			frames[UPDATE_BACKUP];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t			viewangles;

	int32_t 		time;			// this is the time value that the client
	// is rendering at.  always <= cls.realtime
	float			lerpfrac;		// between oldframe and frame

	refdef_t		refdef;

	vec3_t			v_forward, v_right, v_up;	// set when refdef.angles is set

	//
	// transient data from server
	//
	char			layout[1024];		// general 2D overlay
	leaderboard_t	leaderboard;		// ZombonoUI leaderboard

	// NULL indicates dead entries
	loadout_t		loadout;	// Current loadout

	//
	// server state information
	//
	bool			attractloop;		// running the attract loop, any key will menu
	int32_t 		servercount;		// server identification for prespawns
	char			gamedir[MAX_QPATH];
	int32_t 		playernum;

	char			configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	//
	// locally derived information from server state
	//
	struct model_s*	 model_draw[MAX_MODELS];
	struct cmodel_s* model_clip[MAX_MODELS];

	struct sfx_s*	sound_precache[MAX_SOUNDS];
	struct image_s* image_precache[MAX_IMAGES];

	clientinfo_t	clientinfo[MAX_CLIENTS];
	clientinfo_t	baseclientinfo;
} client_state_t;

extern	client_state_t	cl;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

typedef enum {
	ca_uninitialized,
	ca_disconnected, 	// not talking to a server
	ca_connecting,		// sending request packets to the server
	ca_connected,		// netchan_t established, waiting for svc_serverdata
	ca_active			// game views should be displayed
} connstate_t;

typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_single
} dltype_t;		// download type

typedef enum 
{ 
	key_game, 
	key_console, 
	key_message, 
	key_menu 
} keydest_t;

typedef struct
{
	connstate_t	state;
	keydest_t	input_dest;

	int32_t 	framecount;
	int32_t 	realtime;			// always increasing, no clamping, etc
	float		frametime;			// seconds since last frame
	float		fps;				// most recent framerate

	// screen rendering information
	float		disable_screen;		// showing loading plaque between levels
	// or changing rendering dlls
	// if time gets > 30 seconds ahead, break it

	bool		disable_input;		// turns off input

	// Disable individual elements of input
	bool		disable_keyboard;	// turns off keyboard events
	bool		disable_mouse;		// turns off mouse events

	int32_t 	disable_servercount;// when we receive a frame and cl.servercount
	// > cls.disable_servercount, clear disable_screen

// connection information
	char		servername[MAX_OSPATH];	// name of server from original connect
	float		connect_time;		// for connection retransmits

	int32_t 	netchan_port;		// a 16 bit value that allows quake servers
	// to work around address translating routers
	netchan_t	netchan;
	int32_t 	server_protocol;	// in case we are doing some kind of version hack

	int32_t 	challenge;			// from the server to use for connecting

	FILE* download;			// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int32_t 	downloadnumber;
	dltype_t	downloadtype;
	int32_t 	downloadpercent;

	// demo recording info must be here, so it isn't cleared on level change
	bool		demorecording;
	bool		demowaiting;	// don't record until a non-delta message is received
	FILE* demofile;
} client_static_t;

extern client_static_t	cls;

//=============================================================================

//
// cvars
//

extern cvar_t* cl_maxfps;

extern cvar_t* cl_gun;
extern cvar_t* cl_add_blend;
extern cvar_t* cl_add_lights;
extern cvar_t* cl_add_particles;
extern cvar_t* cl_add_entities;
extern cvar_t* cl_predict;
extern cvar_t* cl_footsteps;
extern cvar_t* cl_noskins;
extern cvar_t* cl_autoskins;

extern cvar_t* cl_upspeed;
extern cvar_t* cl_forwardspeed;
extern cvar_t* cl_sidespeed;

extern cvar_t* cl_yawspeed;
extern cvar_t* cl_pitchspeed;

extern cvar_t* cl_run;

extern cvar_t* cl_anglespeedkey;

extern cvar_t* cl_shownet;
extern cvar_t* cl_showmiss;
extern cvar_t* cl_showclamp;
extern cvar_t* cl_showinfo;

extern cvar_t* lookstrafe;
extern cvar_t* sensitivity;

extern cvar_t* m_pitch;
extern cvar_t* m_yaw;
extern cvar_t* m_forward;
extern cvar_t* m_side;

extern cvar_t* freelook;

extern cvar_t* cl_lightlevel;	// FIXME HACK

extern cvar_t* cl_paused;
extern cvar_t* cl_timedemo;

extern cvar_t* cl_console_fraction;
extern cvar_t* cl_console_disabled;

extern cvar_t* cl_vwep;

extern cvar_t* cl_drawhud;

// these ones are temporary until UI scripts implemented
extern cvar_t* cl_showintro;

extern cvar_t* cl_intro1;
extern cvar_t* cl_intro2;
extern cvar_t* cl_intro1_time;
extern cvar_t* cl_intro2_time;

extern cvar_t* cl_timeout;

extern cvar_t* input_mouse_enabled;

extern cvar_t* r_width;
extern cvar_t* r_height;

extern cvar_t* rcon_address;
extern cvar_t* rcon_client_password;

extern cvar_t* gender;
extern cvar_t* gender_auto;

extern cvar_t* skin;

//userinfo
extern cvar_t* msg;

typedef struct cdlight_s
{
	int32_t key;				// so entities can reuse same entry
	vec3_t	color;
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
} cdlight_t;

extern	centity_t	cl_entities[MAX_EDICTS];
extern	cdlight_t	cl_dlights[MAX_DLIGHTS];

int32_t CL_CountReceivedEntities();		// Count the received entity count

// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define	MAX_PARSE_ENTITIES	1024
extern	entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

// App activation

extern bool app_active;
extern bool app_minimized;
void App_Activate(bool fActive, bool minimize);

//=============================================================================

extern netadr_t		net_from;
extern sizebuf_t	net_message;

bool CL_CheckOrDownloadFile(char* filename);
void CL_ParseDownload();

void CL_TeleporterParticles(entity_state_t* ent);
void CL_ParticleEffect(vec3_t org, vec3_t dir, color4_t color, int32_t count);
void CL_ParticleEffect2(vec3_t org, vec3_t dir, color4_t color, int32_t count);

//=============================================================================

typedef struct particle_s
{
	struct particle_s* next;
	float		time;
	vec3_t		org;
	vec3_t		vel;
	vec3_t		accel;
	color4_t	color;
	float		colorvel;
	float		alphavel;
	int32_t		lifetime; // alternative way of having particles die without fading away
	bool		permanent; // used for func_particle_effect
} cparticle_t;


#define	PARTICLE_GRAVITY		40
#define BLASTER_PARTICLE_COLOR	0xe0
#define INSTANT_PARTICLE		-10000.0

void CL_ClearEffects();
void CL_ClearTEnts();
void CL_BlasterTrail(vec3_t start, vec3_t end);
void CL_QuadTrail(vec3_t start, vec3_t end);
void CL_RailTrail(vec3_t start, vec3_t end);
void CL_BubbleTrail(vec3_t start, vec3_t end);

void CL_Flashlight(int32_t ent, vec3_t pos);
void CL_FlameEffects(centity_t* ent, vec3_t origin);
void CL_GenericParticleEffect(vec3_t org, vec3_t dir, color4_t color, int32_t count, color4_t run, int32_t dirspread, vec3_t velocity, int32_t lifetime, float alphavel);
void CL_ParticleSteamEffect(vec3_t org, vec3_t dir, color4_t color, int32_t count, int32_t magnitude);
void CL_ColorFlash(int32_t ent, vec3_t pos, int32_t intensity, color4_t color);
void CL_ParticleSmokeEffect(vec3_t org, vec3_t dir, color4_t color, int32_t count, int32_t magnitude);

int32_t CL_ParseEntityBits(uint32_t* bits);
void CL_ParseDelta(entity_state_t* from, entity_state_t* to, int32_t number, int32_t bits);
void CL_ParseFrame();

void CL_ParseTEnt();
void CL_ParseConfigString();
void CL_ParseMuzzleFlash();
void CL_ParseMuzzleFlash2();

void CL_SetLightstyle(int32_t i);

void CL_RunDLights();
void CL_RunLightStyles();

void CL_AddEntities();
void CL_AddDLights();
void CL_AddTEnts();
void CL_AddLightStyles();

//=================================================

void CL_RegisterSounds();

void CL_Quit_f();

//
// client_main.c
//
extern refexport_t re;		// interface to refresh .dll

void CL_Init();

void CL_FixUpGender();
void CL_Disconnect();
void CL_Disconnect_f();
void CL_PingServers_f();
void CL_Snd_Restart_f();
void CL_Precache_f();
void CL_RequestNextDownload();

//
// client_cvars.c
//
void CL_InitCvars();

//
// client_commands.c
// 
void CL_InitCommands();

//
// client_commands_demo.c
//
void CL_WriteDemoMessage();
void CL_Stop_f();
void CL_Record_f();

//
// client_parse.c
//
extern const char* svc_strings[256];

void CL_ParseServerMessage();
void CL_LoadClientinfo(clientinfo_t* ci, const char* s);
void CL_ShowNet(const char* s);
void CL_ParseClientinfo(int32_t player);
void CL_Download_f();
void CL_ParseEvent();

//
// cl_tent.c
//
void CL_RegisterTEntSounds();
void CL_RegisterTEntModels();
void CL_SmokeAndFlash(vec3_t origin);

//
// cl_pred.c
//
void CL_PredictMovement();
void CL_CheckPredictionError();

//
// cl_fx_dlight.c
//
cdlight_t* CL_AllocDlight(int32_t key);
void CL_ClearDlights();

//
// cl_fx_lightstyle.c
//

void CL_ClearLightStyles();

//
// cl_fx_explosion.c
//

typedef enum exptype_s
{
	ex_free,
	ex_explosion,
	ex_misc,
	ex_flash,
	ex_mflash,
	ex_poly,
	ex_poly2
} exptype_t;

typedef struct explosion_s
{
	exptype_t	type;
	entity_t	ent;
	int32_t 	frames;
	float		light;
	vec3_t		lightcolor;
	float		start;
	int32_t 	baseframe;
} explosion_t;

#define	MAX_EXPLOSIONS	128
extern explosion_t	cl_explosions[MAX_EXPLOSIONS];

explosion_t* CL_AllocExplosion();
void CL_AddExplosions();

//
// cl_fx_beam.c
//

#define	MAX_BEAMS	32
typedef struct
{
	int32_t 		entity;
	int32_t 		dest_entity;
	struct model_s* model;
	int32_t 		endtime;
	vec3_t			offset;
	vec3_t			start, end;
} beam_t;

extern beam_t		cl_beams[MAX_BEAMS];

//PMM - added this for player-linked beams.  Currently only used by the plasma beam
extern beam_t		cl_playerbeams[MAX_BEAMS];

void CL_AddBeams();
void CL_AddPlayerBeams();

//
// cl_fx_particle.c
//
extern struct model_s* cl_mod_smoke;
extern struct model_s* cl_mod_flash;

void CL_RocketTrail(vec3_t start, vec3_t end, centity_t* old);
void CL_DiminishingTrail(vec3_t start, vec3_t end, centity_t* old, int32_t flags);
void CL_FlyEffect(centity_t* ent, vec3_t origin);
void CL_AddParticles();
void CL_EntityEvent(entity_state_t* ent);
void CL_LightningParticles(vec3_t start, float velocity, vec3_t angles);
void CL_LightningParticlesAttachedToEntity(vec3_t start, vec3_t angles);

//
// cl_pred.c
//
void CL_PredictMovement();


// Killfeed support functions
void Killfeed_Add();
void Killfeed_Update();

// Killfeed UI support struct
// Allows us to put up a nice killfeed in the top right 
// See: svc_event documentation, event_type_sv_player_killed

#define MAX_KILLFEED_ENTRIES 8					// The maximum number of killfeed entries
#define MAX_DEATHSTRING_LENGTH 64				// The maximum length of a method of death string

typedef struct killfeed_entry_s
{
	char death_string[MAX_DEATHSTRING_LENGTH];	// The death string to display.
	char icon_path[MAX_QPATH];					// The icon to draw.
	int32_t time;								// The time the killfeed entry was added.
	bool inuse;									// Determines if the killfeed entry is actually in use.
} killfeed_entry_t;

extern killfeed_entry_t killfeed_entries[MAX_KILLFEED_ENTRIES];
extern int32_t killfeed_entry_count;

extern cvar_t* ui_killfeed_entry_time;

// UI: Required CVars

extern cvar_t* vid_hudscale;
	
#define	MAX_FONTS			64					// Maximum number of fonts that can be loaded at any one time.
#define MAX_GLYPHS			256					// Maximum number of glyphs that can be loaded, per font, at any one time.
#define FONT_LIST_FILENAME	"fonts\\fonts.txt"	// File name of the font list.
#define MAX_STRING_LENGTH	2048				// Maximum length of a log string.

//
// cl_font.c
// Image Font Loader (Modern Fontloader)
//

// Defines a colour code that can be used to change the colour of text.
// TODO: ACTUALLY IMPLEMENT
typedef struct color_code_s
{
	const char* name;			// The string used by the colour code.
	uint8_t		color[4];		// The colour generated by the colour code.
} color_code_t;

// Defines a cached glyph standard.
typedef struct glyph_s
{
	int32_t 		width;							// Width of this glyph.
	int32_t 		height;							// Height of this glyph.
	char			char_code;						// Character code for this character (ansi for now, utf-8 later?)
	int32_t 		x_start;						// Start X position of the glyph within the texture.
	int32_t 		x_advance;						// Amount X has to advance to reach the next character.
	int32_t 		x_offset;						// X offset of where the character starts relative to start_x - x_start + x_offset + x_advance = end of char
	int32_t 		y_start;						// Start Y position of the glyph within the texture.
	int32_t 		y_advance;						// Amount Y has to advance to reach the next character. Only used if you are drawing vertically written languages e.g. Japanese
	int32_t 		y_offset;						// Y offset of where the character starts relative to start_y - y_start + y_offset + y_advance = end of char
} glyph_t;

// font_t defines a font
typedef struct font_s
{
	char			name[MAX_QPATH];	// The name of the current font.
	int32_t 		size;							// The size of the current font.
	int32_t 		num_glyphs;						// The number of loaded glyphs.
	int32_t 		line_height;					// Height of one line in this font.
	glyph_t			glyphs[MAX_GLYPHS];				// The glyphs loaded for the current font.
} font_t;

// util methods
typedef enum font_json_section_e
{
	font_json_config = 0,

	font_json_kerning = 1,

	font_json_symbols = 2,
} font_json_section;

extern font_t	fonts[MAX_FONTS];			// Global object containing all loaded fonts.

extern cvar_t* cl_system_font;				// The font used for the game's text (chat, et cetera).
extern cvar_t* cl_console_font;			// The font used for the console.
extern int32_t 	num_fonts;					// The number of loaded fonts.
extern bool		fonts_initialised;			// Indicates if the font engine has been initialised or not.

//
// text/text_font.c 
// Font system
//

bool Font_Init();									// Initialises the font subsystem.
font_t* Font_GetByName(const char* name);			// Returns a pointer to the font with name name, or NULL.
glyph_t* Glyph_GetByChar(font_t* font, char glyph);	// Returns the pointer to a glyph with char code glyph, or NULL if it does not exist.

//
// text/text.c
// Modern Text Engine
//

void Text_Draw(const char* font, int32_t x, int32_t y, const char* text, ...);						// Draws text using font font.
void Text_DrawColor(const char* font, int32_t x, int32_t y, color4_t color, const char* text, ...);	// Draws text using font font and color color (overridden by color codes).
void Text_DrawChar(const char* font, int32_t x, int32_t y, char text);								// Draws a single character of text text using font font. FOR CONSOLE INTERNAL USE ONLY.
bool Text_GetSize(const char* font, int32_t* size_x, int32_t* size_y, const char* text, ...);		// Gets the size of the text text.#
bool Text_GetSizeChar(const char* font, int32_t* size_x, int32_t* size_y, char text);				// Gets the size of a single character of text text using font font. FOR CONSOLE INTERNAL USE ONLY.
//
// cl_loadout.c
// Client parts of the loadout system
//

//Cvars
extern cvar_t* cl_loadout_fade;
extern cvar_t* cl_loadout_fade_time;

void Loadout_Init();																			// Initialises the loadout system.
void Loadout_Add();																				// Parses a loadout add message
void Loadout_Update();																			// Updates the loadout each frame.
void Loadout_Remove(char* item_name);															// Parses a loadout remove message and removes the element with the name item_name.
void Loadout_SetCurrent(int32_t index);															// Parses a loadout set current message
void Loadout_Clear();																			// Clears the loadout

//
// cl_intro.c
// Game intro stuff
//

extern bool intro_running;																		// Is the intro running/

void Intro_Start();																				// Start the intro
void Intro_Update();																			// Update the intro