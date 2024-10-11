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
#pragma once 

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdio>
#include <cstdint>
#include <cstdbool>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <KHR/khrplatform.h>
#include "gl_state.hpp"

#include <client/include/ref.hpp>

#define	REF_VERSION	"Legacy OpenGL 1.x"

#define	PITCH	0 // up / down
#define	YAW		1 // left / right
#define	ROLL	2 // fall over

typedef struct
{
	uint32_t		width;
	uint32_t		height;			// coordinates from main game
} viddef_t;

extern viddef_t	vid;
extern int32_t	modfilelen; // for gl_sprite.c

// called from ref only
#ifdef DEBUG
extern int32_t hunk_total_size; // ref only
extern int32_t hunk_count;
#endif

/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
} imagetype_t;

// Struct that defines an image
typedef struct image_s
{
	char				name[MAX_QPATH];			// game path, including extension
	imagetype_t			type;
	int32_t				width, height;				// source image
	int32_t				upload_width, upload_height;	// after power of two and picmip
	int32_t				registration_sequence;		// 0 = free
	struct msurface_s*  texturechain;	// for sort-by-texture world drawing
	int32_t				texnum;						// gl texture binding
	float				sl, tl, sh, th;				// 0,0 - 1,1
	bool				has_alpha;
} image_t;

#define	TEXNUM_LIGHTMAPS	2048
#define	TEXNUM_IMAGES		2177

#define	MAX_GLTEXTURES		2048

//===================================================================

typedef enum
{
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown,

} rserr_t;

/*
** GL config stuff
*/

// keep this system in case we need to work around buggy drivers.
#define GL_RENDERER_OTHER		0x80000000

typedef struct gl_config_s
{
	int32_t     renderer;
	const char* renderer_string;
	const char* vendor_string;
	const char* version_string;
	const char* extensions_string;

	bool	allow_cds;
} gl_config_t;

extern gl_config_t  gl_config;
extern gl_state_t	gl_state;

#include "gl_model.hpp"

void GL_SetDefaultState(void);

extern float gldepthmin;
extern float gldepthmax;

typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;

#define	MAX_LBM_HEIGHT		480

#define BACKFACE_EPSILON	0.01

//====================================================

extern image_t  gltextures[MAX_GLTEXTURES];
extern int32_t	numgltextures;

//TODO: THESE DON'T WORK BECAUSE THE TEXTURE FORMAT CHANGED
extern image_t* r_notexture;
extern image_t* r_particletexture;

extern entity_t* currententity;
extern model_t* currentmodel;
extern int32_t	r_visframecount;
extern int32_t	r_framecount;
extern cplane_t frustum[4];
extern int32_t	c_brush_polys, c_alias_polys;

extern int32_t	gl_filter_min, gl_filter_max;

//
// view origin
//
extern vec3_t vup;
extern vec3_t vpn;
extern vec3_t vright;
extern vec3_t r_origin;

//
// screen size info
//
extern refdef_t r_newrefdef;
extern int32_t	r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern cvar_t* r_norefresh;
extern cvar_t* r_lefthand;
extern cvar_t* r_drawentities;
extern cvar_t* r_highlightentities;
extern cvar_t* r_drawworld;
extern cvar_t* r_speeds;
extern cvar_t* r_fullbright;
extern cvar_t* r_novis;
extern cvar_t* r_nocull;
extern cvar_t* r_lerpmodels;
extern cvar_t* r_scaled_assets_basewidth;

extern cvar_t* r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern cvar_t* gl_vertex_arrays;

extern cvar_t* gl_vsync;
extern cvar_t* gl_ext_compiled_vertex_array;

extern cvar_t* gl_particle_min_size;
extern cvar_t* gl_particle_max_size;
extern cvar_t* gl_particle_size;
extern cvar_t* gl_particle_att_a;
extern cvar_t* gl_particle_att_b;
extern cvar_t* gl_particle_att_c;

extern cvar_t* r_width;
extern cvar_t* r_height;
extern cvar_t* gl_lightmap;
extern cvar_t* gl_shadows;
extern cvar_t* gl_dynamic;
extern cvar_t* gl_round_down;
extern cvar_t* gl_picmip;
extern cvar_t* gl_skymip;
extern cvar_t* gl_showtris;
extern cvar_t* gl_clear;
extern cvar_t* gl_cull;
extern cvar_t* gl_poly;
extern cvar_t* gl_polyblend;
extern cvar_t* gl_flashblend;
extern cvar_t* gl_modulate;
extern cvar_t* gl_drawbuffer;
extern cvar_t* gl_texturemode;
extern cvar_t* gl_texturealphamode;
extern cvar_t* gl_texturesolidmode;
extern cvar_t* gl_drawdistance;
extern cvar_t* gl_skyboxepsilon;
extern cvar_t* gl_lockpvs;
extern cvar_t* vid_borderless;
extern cvar_t* vid_fullscreen;
extern cvar_t* vid_gamma;

extern cvar_t* intensity;

extern int32_t gl_lightmap_format;
extern int32_t gl_solid_format;
extern int32_t gl_alpha_format;
extern int32_t gl_tex_solid_format;
extern int32_t gl_tex_alpha_format;

extern int32_t c_visible_lightmaps;
extern int32_t c_visible_textures;

extern float   r_world_matrix[16];

void GL_Bind(int32_t texnum);
void GL_MBind(GLenum target, int32_t texnum);
void GL_TexEnv(GLenum value);
void GL_EnableMultitexture(bool enable);
void GL_SelectTexture(GLenum);

void R_LightPoint(vec3_t p, vec3_t color);
void R_PushDlights();

//====================================================================

extern model_t* r_worldmodel;

extern int32_t	registration_sequence;

bool R_Init();
void R_Shutdown(void);

void R_RenderView(refdef_t* fd);
void GL_ScreenShot_f();
void R_DrawAliasModel(entity_t* e);
void R_DrawBrushModel(entity_t* e);
void R_DrawSpriteModel(entity_t* e);
void R_DrawBeam(entity_t* e);
void R_DrawWorld();
void R_RenderDlights();
void R_DrawAlphaSurfaces();
void R_RenderBrushPoly(msurface_t* fa);
void R_InitParticleTexture();
void Draw_InitLocal();
void GL_SubdivideSurface(msurface_t* fa);
bool R_CullBox(vec3_t mins, vec3_t maxs);
void R_RotateForEntity(entity_t* e);
void R_MarkLeaves();

void EmitWaterPolys(msurface_t* fa);
void R_AddSkySurface(msurface_t* fa);
void R_ClearSkyBox();
void R_DrawSkyBox();
void R_MarkLights(dlight_t* light, int32_t bit, mnode_t* node);

void COM_StripExtension(char* in, char* out);

// Mostly exports here.
void Draw_GetPicSize(int32_t* w, int32_t* h, const char* name);
void Load_Pic(const char* name); // load but don't draw a pic.
void Draw_Pic(int32_t x, int32_t y, const char* name, color4_t color, bool use_scaled_assets);
void Draw_PicRegion(int32_t x, int32_t y, int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, const char* pic, color4_t color, bool use_scaled_assets);
void Draw_PicStretch(int32_t x, int32_t y, int32_t w, int32_t h, const char* name, color4_t color, bool use_scaled_assets);
void Draw_FontChar(int32_t x, int32_t y, int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, const char* pic, color4_t color, bool use_scaled_assets);

void Draw_TileClear(int32_t x, int32_t y, int32_t w, int32_t h, const char* name);
void Draw_Fill(int32_t x, int32_t y, int32_t w, int32_t h, color4_t color);
void Draw_FadeScreen();
void R_BeginFrame();

void GL_ResampleTexture(uint32_t* in, int32_t inwidth, int32_t inheight, uint32_t* out, int32_t outwidth, int32_t outheight);

struct image_s* R_RegisterSkin(char* name);

image_t* GL_LoadPic(const char* name, uint8_t* pic, int32_t width, int32_t height, imagetype_t type);
image_t* GL_FindImage(const char* name, imagetype_t type);
void GL_SetTextureMode(const char* value);
void GL_ImageList_f();

void GL_InitImages();
void GL_ShutdownImages();

void GL_FreeUnusedImages();

void GL_SetTextureAlphaMode(const char* value);
void GL_SetTextureSolidMode(const char* value);

// Map extents
extern float map_radius;

//
// gl_glfw.c
// GLFW glue functions
//
bool GL_Init();
void GL_SetMousePressedProc(void proc(void* unused, int32_t button, int32_t action, int32_t mods));
void GL_SetMouseScrollProc(void proc(void* unused, double xoffset, double yoffset));
void GL_SetKeyPressedProc(void proc(void* unused, int32_t key, int32_t scancode, int32_t action, int32_t mods));
void GL_SetMouseMovedProc(void proc(void* unused, double xpos, double ypos));
void GL_SetWindowFocusProc(void proc(void* unused, int32_t focused));
void GL_SetWindowIconifyProc(void proc(void* unused, int32_t iconified));
void GL_GetCursorPosition(double* x, double* y);
void GL_SetCursorPosition(double x, double y);
void GL_SetWindowPosition(double x, double y);
void GL_EnableCursor(bool enabled);
void GL_SetResolution(int32_t x, int32_t y);

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void	GL_BeginFrame();
void	GL_EndFrame(void);
rserr_t GL_SetMode(uint32_t* pwidth, uint32_t* pheight, bool fullscreen);
void	GL_Shutdown(void);
void	GL_ShutdownEverything(GLFWwindow* window);