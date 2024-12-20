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
	
// q_shared.h -- included first by ALL program modules
#pragma once
#ifdef _WIN32
// unknown pragmas are SUPPOSED to be ignored, but....
#pragma warning(disable : 4136)     // X86

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)		// truncation from const double to float

#endif
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdbool>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#ifndef NULL
#define NULL ((void *)0)
#endif

// Z_Malloc tags
#define TAG_LOCALISATION	6666	// clear when Localisation_Shutdown
#define TAG_TEXT_ENGINE		6667	// freed once work with it done

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		256		// max length of an individual token

#define	MAX_QPATH			256		// max length of a quake game pathname (todo: implement in pak files)
#define	MAX_OSPATH			260		// max length of a filesystem pathname

//
// per-level limits
//
#define	MAX_CLIENTS			256		// absolute limit
#define	MAX_EDICTS			2048	// must change protocol to increase more
#define	MAX_LIGHTSTYLES		256
#define	MAX_MODELS			256		// these are sent over the net as bytes
#define	MAX_SOUNDS			256		// so they cannot be blindly increased
#define	MAX_IMAGES			256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	// general config strings

// game print flags
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages

#define	ERR_FATAL			0		// exit the entire game with a popup window
#define	ERR_DROP			1		// print to console and disconnect from game
#define	ERR_DISCONNECT		2		// don't kill server

#define	PRINT_ALL			0
#define PRINT_DEVELOPER		1		// only print when "developer 1"
#define PRINT_ALERT			2		

#define MAP_NAME_LENGTH		32	
#define PLAYER_NAME_LENGTH	80		// Maximum length of a player's name

// destination class for gi.multicast()
typedef enum
{
MULTICAST_ALL,
MULTICAST_PHS,
MULTICAST_PVS,
MULTICAST_ALL_R,
MULTICAST_PHS_R,
MULTICAST_PVS_R
} multicast_t;

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

// color types
typedef vec3_t color3_t;
typedef vec4_t color4_t;

// MSVC doesn't seem to have this?
#ifndef M_PI
#define M_PI		3.14159265358979323846f	// matches value in gcc v2 math.h
#endif

struct cplane_s;

extern vec3_t vec3_origin;

#define DotProduct3(x,y)		(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract3(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define VectorAdd3(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define VectorCopy3(a,b)		(b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define VectorClear3(a)			(a[0]=a[1]=a[2]=0)
#define VectorNegate3(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define VectorSet3(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

// I searched i
#define DotProduct4(x,y)		(x[0]*y[0]+x[1]*y[1]+x[2]*y[2]+x[3]*y[3])
#define VectorSubtract4(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2],c[3]=a[3]-b[3])
#define VectorAdd4(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2],c[3]=a[3]+b[3])
#define VectorCopy4(a,b)		(b[0]=a[0],b[1]=a[1],b[2]=a[2],b[3]=a[3])
#define VectorClear4(a)			(a[0]=a[1]=a[2]=a[3]=0)
#define VectorNegate4(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2],b[3]=-a[3])
#define VectorSet4(v, x, y, z, w)	(v[0]=(x), v[1]=(y), v[2]=(z), v[3]=(w))

// Functions for colours, that are better named
// Dot product isn't needed

#define ColorSubtract3(a,b,c)	VectorSubtract3(a,b,c)
#define ColorAdd3(a,b,c)	VectorAdd3(a,b,c)
#define ColorCopy3(a,b)	VectorCopy3(a,b)
#define ColorClear3(a)	VectorClear3(a)
#define ColorNegate3(a,b)	VectorNegate3(a,b))
#define ColorSet3(v, x, y, z)	VectorSet3(v, x, y, z)

#define ColorSubtract4(a,b,c)	VectorSubtract4(a,b,c)
#define ColorAdd4(a,b,c)	VectorAdd4(a,b,c)
#define ColorCopy4(a,b)	VectorCopy4(a,b)
#define ColorClear4(a)	VectorClear4(a)
#define ColorNegate4(a,b)	VectorNegate4(a,b))
#define ColorSet4(v, x, y, z, w)	VectorSet4(v, x, y, z, w)

void VectorCrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);

void VectorMA3(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);
void VectorAddPointToBounds3(vec3_t v, vec3_t mins, vec3_t maxs);
bool VectorCompare3(vec3_t v1, vec3_t v2);
vec_t VectorLength3(vec3_t v);
vec_t VectorNormalize3(vec3_t v);		// returns vector length
void VectorInverse3(vec3_t v);
void VectorScale3(vec3_t in, vec_t scale, vec3_t out);

void VectorMA4(vec4_t veca, float scale, vec4_t vecb, vec4_t vecc);
void VectorAddPointToBounds4(vec4_t v, vec4_t mins, vec4_t maxs);
bool VectorCompare4(vec4_t v1, vec4_t v2);
vec_t VectorLength4(vec4_t v);
// Here is an article that precisely formulates and demonstrates the non-existence of a cross product in Euclidean spaces of dimensions other than 3 and 7:
// http://www.jstor.org/stable/2323537. 
vec_t VectorNormalize4(vec4_t v);		// returns vector length
void VectorInverse4(vec4_t v);
void VectorScale4(vec4_t in, vec_t scale, vec4_t out);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int32_t BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s* plane);
float	anglemod(float a);
float LerpAngle(float a1, float a2, float frac);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void PerpendicularVector(vec3_t dst, const vec3_t src);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);

//=============================================

void COM_StripExtension(char* in, char* out);
void COM_FileBase(char* in, char* out);
void COM_FilePath(char* in, char* out);

const char* COM_Parse(char** data_p);
// data is an in/out parm, returns a parsed out token

void Com_PageInMemory(uint8_t* buffer, int32_t size);

//=============================================

// portable case insensitive compare
int32_t Q_stricmp(const char* s1, const char* s2);
int32_t Q_strcasecmp(const char* s1, const char* s2);
int32_t Q_strncasecmp(const char* s1, const char* s2, int32_t n);

//=============================================

// If this isn't included from common.h *AND* q_shared.h the game dies BECAUSE THESE FUNCTIONS ALSO GET CALLED FROM THE GAME DLL
extern bool big_endian;

int16_t	BigShort(int16_t l);
int16_t	LittleShort(int16_t l);
uint16_t BigShortUnsigned(int16_t l);
uint16_t LittleShortUnsigned(int16_t l);
int32_t BigInt(int32_t l);
int32_t LittleInt(int32_t l);
uint32_t BigIntUnsigned(int32_t l);
uint32_t LittleIntUnsigned(int32_t l);
float	BigFloat(float l);
float	LittleFloat(float l);

void	Swap_Init();
char*	va(char* format, ...);


//
// key / value info strings
//
#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

const char* Info_ValueForKey(char* s, const char* key);
void Info_RemoveKey(char* s, char* key);
void Info_SetValueForKey(char* s, char* key, char* value);
bool Info_Validate(char* s);

/*
==============================================================

SYSTEM SPECIFIC

==============================================================
*/

extern int32_t curtime;		// time returned by last Sys_Milliseconds
extern int64_t curtime_ns;		// time returned by last Sys_Nanoseconds

int32_t Sys_Milliseconds();
int64_t Sys_Nanoseconds(); // should be platform independent
void Sys_Mkdir(char* path);

// large block stack allocation routines
void* Memory_HunkBegin(int32_t maxsize);
void* Memory_HunkAlloc(int32_t size);
void Memory_HunkFree(void* buf);
int32_t Memory_HunkEnd();

// directory searching
#define SFF_ARCH    0x01
#define SFF_HIDDEN  0x02
#define SFF_RDONLY  0x04
#define SFF_SUBDIR  0x08
#define SFF_SYSTEM  0x10

/*
** pass in an attribute mask of things you wish to REJECT
*/
char* Sys_FindFirst(char* path, uint32_t musthave, uint32_t canthave);
char* Sys_FindNext(uint32_t musthave, uint32_t canthave);
void Sys_FindClose();


// this is only here so the functions in q_shared.c and q_shwin.c can link

// are these still needed?
__declspec(noreturn) void Sys_Error(const char *error, ...);
void Com_Printf(const char *msg, ...);


/*
==========================================================

CVARS (console variables)

==========================================================
*/

#define	CVAR_ARCHIVE	1	// set to cause it to be saved to vars.rc
#define	CVAR_USERINFO	2	// added to userinfo  when changed
#define	CVAR_SERVERINFO	4	// added to serverinfo when changed
#define	CVAR_NOSET		8	// don't allow change from console at all,
							// but can be set from the command line
#define	CVAR_LATCH		16	// save changes until server restart

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s
{
	char*			name;
	char*			string;
	char*			latched_string;	// for CVAR_LATCH vars
	int32_t 		flags;
	bool			modified;	// set each time the cvar is changed
	float			value;
	struct cvar_s*	next;
} cvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_MIST			64
#define	LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000

#define	SURF_LIGHT		0x1		// value will hold the light strength

#define	SURF_SLICK		0x2		// effects game physics

#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	// scroll towards angle
#define	SURF_NODRAW		0x80	// don't bother referencing the texture

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)


// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2

// TODO: Move this to common/bsp.h

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s
{
	vec3_t	normal;
	float	dist;
	uint8_t	type;			// for fast side tests
	uint8_t	signbits;		// signx + (signy<<1) + (signz<<1)
	uint8_t	pad[2];
} cplane_t;

// structure offset for asm code
#define CPLANE_NORMAL_X			0
#define CPLANE_NORMAL_Y			4
#define CPLANE_NORMAL_Z			8
#define CPLANE_DIST				12
#define CPLANE_TYPE				16
#define CPLANE_SIGNBITS			17
#define CPLANE_PAD0				18
#define CPLANE_PAD1				19

typedef struct cmodel_s
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	int32_t 	headnode;
} cmodel_t;

typedef struct csurface_s
{
	char		name[16];
	int32_t 	flags;
	int32_t 	value;
} csurface_t;

typedef struct mapsurface_s  // used internally due to name len probs //ZOID
{
	csurface_t	c;
	char		rname[80]; // must be set to TEXTURE_NAME_LENGTH in higher-level common/bsp.h
} mapsurface_t;

// a trace is returned when a box is swept through the world
typedef struct
{
	bool			allsolid;	// if true, plane is not valid
	bool			startsolid;	// if true, the initial point was in a solid area
	float			fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t			endpos;		// final position
	cplane_t		plane;		// surface normal at impact
	csurface_t*		surface;	// surface hit
	int32_t 		contents;	// contents on other side of surface hit
	struct edict_s* ent;		// not set by CM_*() functions
} trace_t;

// pmove_state_t is the information necessary for client side movement
// prediction
typedef enum 
{
	// can accelerate and turn
	PM_NORMAL,
	PM_SPECTATOR,
	// no acceleration or turning
	PM_DEAD,
	PM_GIB,		// different bounding box
	PM_FREEZE
} pmtype_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_ON_GROUND		4
#define	PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	PMF_TIME_LAND		16	// pm_time is time before rejump
#define	PMF_TIME_TELEPORT	32	// pm_time is non-moving time
#define PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync (floats are now used)
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct
{
	pmtype_t	pm_type;

	vec3_t		origin;				// float
	vec3_t		velocity;			// float
	uint8_t		pm_flags;			// ducked, jump_held, etc
	uint8_t		pm_time;			// each unit = 8 ms
	int16_t		gravity;
	int16_t		delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters
} pmove_state_t;

//
// button bits
//
#define	BUTTON_ATTACK1		1			// primary attack
#define	BUTTON_USE			2			// use key
#define BUTTON_ATTACK2		4			// secondary attack
#define	BUTTON_ANY			128			// any key whatsoever

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s
{
	uint8_t	msec;
	uint8_t	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	uint8_t	impulse;		// remove?
	uint8_t	lightlevel;		// light level the player is standing on
} usercmd_t;


#define	MAXTOUCH	32
typedef struct
{
	// state (in / out)
	pmove_state_t	s;

	// command (in)
	usercmd_t		cmd;
	bool			snapinitial;	// if s has been changed outside pmove

	// results (out)
	int32_t 		numtouch;
	struct edict_s* touchents[MAXTOUCH];
	
	vec3_t			vieworigin;			// camera position
	vec3_t			viewangles;			// clamped
	float			viewheight;

	vec3_t			mins, maxs;			// bounding box size

	struct edict_s* groundentity;
	int32_t 		watertype;
	int32_t 		waterlevel;

	// callbacks to test the world
	trace_t			(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int32_t 		(*pointcontents) (vec3_t point);
} pmove_t;


// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define	EF_ROTATE			0x1		// rotate (bonus items)
#define	EF_GIB				0x2		// leave a trail
#define	EF_BLASTER			0x4		// redlight + trail
#define	EF_ROCKET			0x8		// redlight + trail
#define	EF_GRENADE			0x10
#define	EF_HYPERBLASTER		0x20
#define EF_COLOR_SHELL		0x40
#define	EF_ANIM01			0x100	// automatically cycle between frames 0 and 1 at 2 hz
#define	EF_ANIM23			0x200	// automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL			0x400	// automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST		0x800	// automatically cycle through all frames at 10hz
#define	EF_FLIES			0x1000
#define	EF_QUAD				0x2000
#define	EF_PENT				0x4000
#define	EF_TELEPORTER		0x8000	// particle fountain
#define EF_LIGHTNING		0x10000	// e.g. tangfuslicator

// entity_state_t->renderfx flags
#define	RF_MINLIGHT			0x1		// allways have some light (viewmodel)
#define	RF_VIEWERMODEL		0x2		// don't draw through eyes, only mirrors
#define	RF_WEAPONMODEL		0x4		// only draw through eyes
#define	RF_FULLBRIGHT		0x8		// allways draw full intensity
#define	RF_DEPTHHACK		0x10	// for view weapon Z crunching
#define	RF_TRANSLUCENT		0x20
#define	RF_FRAMELERP		0x40
#define RF_BEAM				0x80
#define	RF_CUSTOMSKIN		0x100	// skin is an index in image_precache
#define	RF_GLOW				0x200	// pulse lighting for bonus items
#define RF_SHELL_RED		0x400
#define	RF_SHELL_GREEN		0x800
#define RF_SHELL_BLUE		0x1000
#define RF_USE_DISGUISE		0x2000

// player_state_t->refdef flags
#define	RDF_UNDERWATER		1		// warp the screen as apropriate
#define RDF_NOWORLDMODEL	2		// used for player configuration screen

//
// muzzle flashes / player effects
//
#define	MZ_BLASTER			0
#define MZ_MACHINEGUN		1
#define	MZ_SHOTGUN			2
#define	MZ_CHAINGUN1		3
#define	MZ_CHAINGUN2		4
#define	MZ_CHAINGUN3		5
#define	MZ_RAILGUN			6
#define	MZ_ROCKET			7
#define	MZ_GRENADE			8
#define	MZ_LOGIN			9
#define	MZ_LOGOUT			10
#define	MZ_RESPAWN			11
#define	MZ_SSHOTGUN			12
#define	MZ_HYPERBLASTER		13
#define	MZ_ITEMRESPAWN		14
#define MZ_SILENCED			128		// bit flag ORed with one of the above numbers

//
// monster muzzle flashes
//

#define MZ2_SOLDIER_BLASTER_1			1
#define MZ2_SOLDIER_SHOTGUN_1			2
#define MZ2_SOLDIER_MACHINEGUN_1		3

extern vec3_t monster_flash_offset [];

// svc_event is a way for the server to tell the client when something happens that it needs to know about
// e.g. a UI changed, a player was killed or something...

typedef enum event_type_sv_e
{
	event_type_sv_game_start,			// A game started.
	event_type_sv_game_end,				// A game ended.
	event_type_sv_leaderboard_update,	// A leaderboard update occurred.
	event_type_sv_leaderboard_draw,		// ****REMOVE THIS EVENT WHEN CODE REFACTORED****
	event_type_sv_loadout_add,			// A loadout item was added (<loadout_entry_t struct>)
	event_type_sv_loadout_remove,		// A loadout item was removed (<index, or -1 for current>)
	event_type_sv_loadout_setcurrent,	// A loadout item was changed (<item id>)
	event_type_sv_loadout_clear,		// The loadout was cleared
	event_type_sv_player_joined,		// A player joined the game
	event_type_sv_player_left,			// A player left the game
	event_type_sv_player_killed,		// A player was killed (<player 1 name> <player 2 name> <method of death>)
	event_type_sv_ui_draw,				// Draw a user interface
	event_type_sv_ui_set_text,			// Update a UI's text
	event_type_sv_ui_set_image,			// Update a UI's image
} event_type_sv;

// client to server events (clc_event)
typedef enum event_type_cl_e
{
	event_type_cl_player_set_team,		// Set a player's team
} event_type_cl;

// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
typedef enum
{
	TE_GENERIC,
	TE_GUNSHOT,
	TE_BLOOD,
	TE_BLASTER,
	TE_RAILTRAIL,
	TE_SHOTGUN,
	TE_EXPLOSION1,
	TE_EXPLOSION2,
	TE_ROCKET_EXPLOSION,
	TE_GRENADE_EXPLOSION,
	TE_ROCKET_EXPLOSION_WATER,
	TE_GRENADE_EXPLOSION_WATER,
	TE_SPARKS,
	TE_SHIELD_SPARKS,
	TE_BULLET_SPARKS,
	TE_LASER_SPARKS,
	TE_SPLASH,
	TE_BUBBLETRAIL,
	TE_TELEPORT,
	TE_LIGHTNING,			// Tangfuslicator
	// For some reason, this cl_newfx.c stuff was not exposed to tempentities/tempevents...see also TE_GENERIC
	TE_FLAME,
	TE_SMOKE,
	TE_STEAM,
	TE_FLASHLIGHT,
	TE_FLASH,
	
} temp_event_t;

#define SPLASH_UNKNOWN		0
#define SPLASH_SPARKS		1
#define SPLASH_BLUE_WATER	2
#define SPLASH_BROWN_WATER	3
#define SPLASH_SLIME		4
#define	SPLASH_LAVA			5
#define SPLASH_BLOOD		6

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
#define	CHAN_AUTO               0
#define	CHAN_WEAPON             1
#define	CHAN_VOICE              2
#define	CHAN_ITEM               3
#define	CHAN_BODY               4

// modifier flags
#define	CHAN_NO_PHS_ADD			8	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
#define	CHAN_RELIABLE			16	// send by reliable message, not datagram

// sound attenuation values
#define	ATTN_NONE               0	// full volume the entire level
#define	ATTN_NORM               1
#define	ATTN_IDLE               2
#define	ATTN_STATIC             3	// diminish very rapidly with distance

// player_state->stats[] indexes
#define STAT_HEALTH_ICON		0
#define	STAT_HEALTH				1
#define	STAT_AMMO_ICON			2
#define	STAT_AMMO				3
#define	STAT_ARMOR				4
#define STAT_ARMOR_ICON			5
#define	STAT_PICKUP_ICON		6
#define	STAT_PICKUP_STRING		7
#define	STAT_TIMER_ICON			8
#define	STAT_TIMER				9
#define	STAT_LAYOUTS			10
#define	STAT_FRAGS				11
#define	STAT_FLASHES			12		// cleared each frame, 1 = health, 2 = armor
#define STAT_CHASE				13
#define STAT_SPECTATOR			14

#define	MAX_STATS				32

// gameflags->value flags
#define	GF_NO_HEALTH						0x00000001	// 1
#define	GF_NO_ITEMS							0x00000002	// 2
#define	GF_WEAPONS_STAY						0x00000004	// 4
#define	GF_NO_FALL_DAMAGE					0x00000008	// 8
#define	GF_INSTANT_ITEMS					0x00000010	// 16
#define	GF_SAME_LEVEL						0x00000020	// 32
#define GF_NO_FRIENDLY_FIRE					0x00000040	// 64
#define	GF_SPAWN_FARTHEST					0x00000080	// 128
#define GF_FORCE_RESPAWN					0x00000100	// 256
#define GF_NO_ARMOR							0x00000200	// 512
#define GF_ALLOW_EXIT						0x00000400	// 1024
#define GF_INFINITE_AMMO					0x00000800	// 2048
#define GF_QUAD_DROP						0x00001000	// 4096
#define GF_ITEM_FRIENDLY_FIRE				0x00002000	// 8192
#define GF_INDIVIDUAL_FRAGLIMIT				0x00004000	// 16384
#define GF_DIRECTOR_CAN_PICKUP_PLAYER_ITEMS	0x00008000  // 32768
#define GF_DONT_DROP_DIRECTOR_ITEMS			0x00010000  // 65536

// Gamemodes

#define NETWORK_MAX_GAMEMODE_NAME_LENGTH 32

#define	GAMEMODE_TDM			0		// Team Deathmach
#define GAMEMODE_HOSTAGE		1		// Hostage rescue???
#define GAMEMODE_WAVES			2		// Zombono waves
#define GAMEMODE_COOP			3		// Coop???
#define GAMEMODE_CONTROL_POINT	4		// Control Point??? Need more innovative game modes...maybe a build-off

// Camera System
typedef enum camera_type_e
{
	// ps.pmove.origin used for view construction
	camera_type_normal = 0,

	// chase camera used for spectator
	camera_type_chase = 1,

	// top-down camera used for director team's view
	camera_type_topdown = 2,

	// debug freecam
	camera_type_free = 3,

	// sentinel value for the end of the camera list
	camera_type_max = camera_type_free, 
} camera_type;

/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#define	ANGLE2SHORT(x)	((int32_t)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0f/65536))

//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//
#define	CS_NAME						0
#define	CS_CDTRACK					1
#define	CS_SKY						2
#define	CS_SKYAXIS					3		// %f %f %f format
#define	CS_SKYROTATE				4
#define	CS_STATUSBAR				5		// display program string (obsolete - replaced with gameui messages)

#define CS_PHYS_STOPSPEED			20		// stopping speed
#define CS_PHYS_MAXSPEED_PLAYER		21		// max speed - player team, non-TDM modes as well
#define CS_PHYS_MAXSPEED_DIRECTOR	22		// max speed - director team
#define CS_PHYS_DUCKSPEED			23		// duck speed
#define CS_PHYS_ACCELERATE_PLAYER	24		// ground acceleration for players
#define CS_PHYS_ACCELERATE_DIRECTOR	25		// ground acceleration for directors
#define CS_PHYS_ACCELERATE_AIR		26		// air acceleration for both teams (split?)
#define CS_PHYS_ACCELERATE_WATER	27		// water acceleration for both teams (split?)
#define CS_PHYS_FRICTION			28		// ground friction
#define CS_PHYS_FRICTION_WATER		29		// water friction

#define	CS_MAXCLIENTS				30
#define	CS_MAPCHECKSUM				31		// for catching cheater maps

#define	CS_MODELS					32
#define	CS_SOUNDS					(CS_MODELS+MAX_MODELS)
#define	CS_IMAGES					(CS_SOUNDS+MAX_SOUNDS)
#define	CS_LIGHTS					(CS_IMAGES+MAX_IMAGES)
#define	CS_ITEMS					(CS_LIGHTS+MAX_LIGHTSTYLES)
#define	CS_PLAYERSKINS				(CS_ITEMS+MAX_ITEMS)
#define CS_GENERAL					(CS_PLAYERSKINS+MAX_CLIENTS)
#define	MAX_CONFIGSTRINGS			(CS_GENERAL+MAX_GENERAL)

//==============================================

// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
typedef enum
{
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT
} entity_event_t;


// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
typedef struct entity_state_s
{
	int32_t 	number;			// edict index

	vec3_t		origin;
	vec3_t		angles;
	vec3_t		old_origin;		// for lerping
	vec3_t		extents;

	int32_t		modelindex;
	int32_t 	modelindex2, modelindex3, modelindex4;	// weapons, CTF flags, etc
	int32_t 	frame;
	int32_t 	skinnum;
	uint32_t	effects;		// PGM - we're filling it, so it needs to be unsigned
	int32_t 	renderfx;
	int32_t 	solid;			// for client side prediction, 8*(bits 0-4) is x/y radius
	// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
	// gi.linkentity sets this properly
	int32_t 	sound;			// for looping sounds, to guarantee shutoff
	int32_t 	event;			// impulse events -- muzzle flashes, footsteps, etc
	// events only go out for a single frame, they
	// are automatically cleared each frame
} entity_state_t;

//==============================================


// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
typedef struct
{
	pmove_state_t	pmove;		// for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t			vieworigin;		// camera origin
	camera_type		camera_type;	// camera type
	vec3_t			viewangles;		// for fixed views
	vec3_t			viewoffset;		// add to pmovestate->origin
	vec3_t			kick_angles;	// add to view direction to get render angles
								// set by weapon kicks, pain effects, etc

	vec3_t			gunangles;
	vec3_t			gunoffset;
	int32_t 		gunindex;
	int32_t 		gunframe;

	float			blend[4];		// rgba full screen effect
	
	float			fov;			// horizontal field of view

	int32_t 		rdflags;		// refdef flags

	int16_t			stats[MAX_STATS];		// fast status bar updates
} player_state_t;


#define VIDREF_GL		0
#define VIDREF_OTHER	1

extern int32_t vidref_val;

// MUST BE KEPT IN SYNC WITH GAME DLL player_team ENUM!
// Client and server version of game team enum (temp).
typedef enum player_team_e
{
	team_director = 1,

	team_player = 2,

	team_unassigned = 4,

	// Sentinel value defining the last valid team flag.
	team_max = 4,

} player_team;

#define NETWORK_MAX_MAP_NAME_LENGTH	48

// Leaderboard system
typedef struct leaderboard_entry_s
{
	char			name[PLAYER_NAME_LENGTH];		// The player's name
	int32_t 		score;							// The player's score (kills - deaths)
	player_team		team;							// The player's team
	int32_t 		ping;							// The user's ping
	int32_t 		time;							// Time the player has spent in the game since they joined. 
	bool			is_spectator;					// Is the player a spectator?
	char			map_name[NETWORK_MAX_MAP_NAME_LENGTH];					// Map name
	int32_t 		time_remaining;					// Seconds of time left in game.
} leaderboard_entry_t;

// Zombono Leaderboard
typedef struct leaderboard_s
{
	int32_t 				num_clients;			// The number of clients in the leaderboard
	leaderboard_entry_t		entries[MAX_CLIENTS];	// The leaderboard entries.
} leaderboard_t;

// Loadout system

// Currently we only allow the 1-0 keys.
// but in case we want to add more items to tbe loadout...
#define LOADOUT_MAX_ITEMS		32
#define LOADOUT_MAX_STRLEN		48

// Enumerates types of entries in the loadout system.
typedef enum loadout_entry_type_e
{
	// Weapons
	loadout_entry_type_weapon = 0,

	// Ammunition
	loadout_entry_type_ammo = 1,

	// Armour
	loadout_entry_type_armor = 2,

	// Powerup
	loadout_entry_type_powerup = 3,

} loadout_entry_type;

// Defines a loadout entry.
typedef struct loadout_entry_s
{
	char				item_name[LOADOUT_MAX_STRLEN];	// The friendly name of the item (ammo, weapon...)
	char				icon[MAX_QPATH];				// The icon to draw (since entities are defined on game side)
	int32_t				amount;							// The amount (usually only used for the ammunition)
	loadout_entry_type	type;						// The type of this loadout entry.
} loadout_entry_t;

typedef struct loadout_s
{
	int32_t				num_items;					// Number of items the user currently has.
	loadout_entry_t*	client_current_item;		// A pointer to the currently equipped item (clientside).
	loadout_entry_t		items[LOADOUT_MAX_ITEMS];	// The items within the loadout.
} loadout_t;