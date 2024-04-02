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

// ref.h: header for exported refresh (rendering) dll

#pragma once

#include "../qcommon/qcommon.h"

#define	MAX_DLIGHTS		32
#define	MAX_ENTITIES	128
#define	MAX_PARTICLES	4096
#define	MAX_LIGHTSTYLES	256

#define POWERSUIT_SCALE		4.0F

#define SHELL_RED_COLOR		0xF2
#define SHELL_GREEN_COLOR	0xD0
#define SHELL_BLUE_COLOR	0xF3

#define SHELL_RG_COLOR		0xDC
#define SHELL_RB_COLOR		0x68
#define SHELL_BG_COLOR		0x78

//ROGUE
#define SHELL_DOUBLE_COLOR	0xDF // 223
#define	SHELL_HALF_DAM_COLOR	0x90
#define SHELL_CYAN_COLOR	0x72
//ROGUE

#define SHELL_WHITE_COLOR	0xD7

typedef struct entity_s
{
	struct model_s		*model;			// opaque type outside refresh
	float				angles[3];

	/*
	** most recent data
	*/
	float				origin[3];		// also used as RF_BEAM's "from"
	int32_t 				frame;			// also used as RF_BEAM's diameter

	/*
	** previous data for lerping
	*/
	float				oldorigin[3];	// also used as RF_BEAM's "to"
	int32_t 				oldframe;

	/*
	** misc
	*/
	float	backlerp;				// 0.0 = current, 1.0 = old
	int32_t 	skinnum;				// also used as RF_BEAM's palette index

	int32_t 	lightstyle;				// for flashing entities
	float	alpha;					// ignore if RF_TRANSLUCENT isn't set

	struct image_s	*skin;			// NULL for inline skin
	int32_t 	flags;

} entity_t;

#define ENTITY_FLAGS  68

typedef struct dlight_s
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
} dlight_t;

typedef struct
{
	vec3_t	origin;
	int32_t 	color;
	float	alpha;
} particle_t;

typedef struct lightstyle_s
{
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightstyle_t;

typedef struct refdef_s
{
	int32_t 		x, y, width, height;// in virtual screen coordinates
	float		fov_x, fov_y;
	float		vieworg[3];
	float		viewangles[3];
	float		blend[4];			// rgba 0-1 full screen blend
	float		time;				// time is uesed to auto animate
	int32_t 		rdflags;			// RDF_UNDERWATER, etc

	uint8_t		*areabits;			// if not NULL, only areas with set bits will be drawn

	lightstyle_t	*lightstyles;	// [MAX_LIGHTSTYLES]

	int32_t 		num_entities;
	entity_t	*entities;

	int32_t 		num_dlights;
	dlight_t	*dlights;

	int32_t 		num_particles;
	particle_t	*particles;
} refdef_t;

#define	API_VERSION		7

//
// these are the functions exported by the refresh module
//
typedef struct refexport_s
{
	// if api_version is different, the dll cannot be used
	int32_t 	api_version;

	// called when the library is loaded
	bool	(*Init) ();

	// called before the library is unloaded
	void	(*Shutdown) (void);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// EndRegistration will free any remaining data that wasn't registered.
	// Any model_s or skin_s pointers from before the BeginRegistration
	// are no longer valid after EndRegistration.
	//
	// Skins and images need to be differentiated, because skins
	// are flood filled to eliminate mip map edge errors.
	// Pics can be loaded from any directory.
	void	(*BeginRegistration) (char *map);
	struct model_s *(*RegisterModel) (char *name);
	struct image_s *(*RegisterSkin) (char *name);
	struct image_s *(*RegisterPic) (char *name);
	void	(*SetSky) (char *name, float rotate, vec3_t axis);
	void	(*EndRegistration) (void);

	void	(*RenderFrame) (refdef_t *fd);

	void	(*DrawGetPicSize) (int32_t *w, int32_t *h, char *name);	// will return 0 0 if not found
	void	(*DrawPic) (int32_t x, int32_t y, char *name);
	void	(*LoadPic) (char* name);
	void	(*DrawStretchPic) (int32_t x, int32_t y, int32_t w, int32_t h, char *name);
	void	(*DrawTileClear) (int32_t x, int32_t y, int32_t w, int32_t h, char *name);
	void	(*DrawFill) (int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t g, int32_t b, int32_t a);
	void	(*DrawPicRegion)(int32_t x, int32_t y, int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, char* pic, float color[4]);
	void	(*DrawFadeScreen) (void);
	/*
	** video mode and refresh state management entry points
	*/
	void	(*BeginFrame)();
	void	(*EndFrame) (void);
	void	(*EndWorldRenderpass) (void); // finish world rendering, apply postprocess and switch to UI render pass

	// void* here is a kludge for GLFW

	void	(*SetMousePressedProc) (void proc(void* unused, int32_t button, int32_t action, int32_t mods));
	void	(*SetMouseMovedProc) (void proc(void* unused, double xpos, double ypos));
	void	(*SetKeyPressedProc) (void proc(void* unused, int32_t key, int32_t scancode, int32_t action, int32_t mods));
	void	(*SetWindowFocusProc) (void proc(void* unused, int32_t focused));
	void	(*SetWindowIconifyProc) (void proc(void* unused, int32_t iconified));
	void	(*EnableCursor) (bool);
} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct
{
	void	(*Sys_Error) (int32_t err_level, char *str, ...);

	void	(*Cmd_AddCommand) (char *name, void(*cmd)(void));
	void	(*Cmd_RemoveCommand) (char *name);
	int32_t (*Cmd_Argc) (void);
	char	*(*Cmd_Argv) (int32_t i);
	void	(*Cmd_ExecuteText) (int32_t exec_when, char *text);

	void	(*Con_Printf) (int32_t print32_t_level, char *str, ...);

	// files will be memory mapped read only
	// the returned buffer may be part of a larger pak file,
	// or a discrete file from anywhere in the quake search path
	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int32_t 	(*FS_LoadFile) (char *name, void **buf);
	void	(*FS_FreeFile) (void *buf);

	// gamedir will be the current directory that generated
	// files should be stored to, ie: "f:\quake\id1"
	char	*(*FS_Gamedir) (void);

	cvar_t	*(*Cvar_Get) (char *name, char *value, int32_t flags);
	cvar_t	*(*Cvar_Set)( char *name, char *value );
	void	 (*Cvar_SetValue)( char *name, float value );

	bool	(*Vid_GetModeInfo)( int32_t *width, int32_t *height, int32_t mode );
	void		(*Vid_MenuInit)( void );
	void		(*Vid_NewWindow)( int32_t width, int32_t height );
} refimport_t;


// this is the only function actually exported at the linker level
typedef	refexport_t	(*GetRefAPI_t) (refimport_t);

