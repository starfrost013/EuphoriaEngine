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
#include "../client/client.h"
#include "../client/qmenu.h"

#define REF_SOFT	0
#define REF_SOFTX11	1
#define REF_MESA3D  2
#define REF_3DFXGL 3
#define REF_OPENGLX	4
#define REF_MESA3DGLX 6

extern cvar_t *vid_ref;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *vid_hudscale;
extern cvar_t *scr_viewsize;

static cvar_t *gl_mode;
static cvar_t *gl_driver;
static cvar_t *gl_picmip;
static cvar_t *gl_ext_palettedtexture;
static cvar_t *vk_msaa;
static cvar_t *vk_aniso;
static cvar_t *vk_sampleshading;
static cvar_t *vk_texturemode;
static cvar_t *vk_lmaptexturemode;
static cvar_t *vk_vsync;
static cvar_t *vk_postprocess;
static cvar_t *vk_picmip;

static cvar_t *sw_mode;
static cvar_t *sw_stipplealpha;

static cvar_t *_windowed_mouse;
static cvar_t *vk_mode;
static cvar_t *vk_driver;

extern void M_ForceMenuOff( void );

/*
====================================================================

MENU INTERACTION

====================================================================
*/
#define OPENGL_MENU   	0

#define MENU_COUNT	1

static menuframework_t	s_opengl_menu;
static menuframework_t *s_current_menu;
static int				s_current_menu_index;

static menulist_t		s_mode_list[3];
static menulist_t		s_ref_list[3];
static menuslider_t		s_tq_slider;
static menuslider_t		s_screensize_slider[3];
static menuslider_t		s_brightness_slider[3];
static menulist_t  		s_fs_box[3];
static menulist_t  		s_stipple_box;
static menulist_t  		s_paletted_texture_box;
static menulist_t  		s_windowed_mouse;
static menulist_t		s_sampleshading;
static menulist_t		s_vsync;
static menulist_t		s_postprocess;
static menuaction_t		s_apply_action[3];
static menuaction_t		s_cancel_action[3];
static menuaction_t		s_defaults_action[3];

static void DriverCallback( void *unused )
{
	int curr_value = s_ref_list[s_current_menu_index].curvalue;
	s_ref_list[0].curvalue = curr_value;
	s_ref_list[1].curvalue = curr_value;
	s_ref_list[2].curvalue = curr_value;

	s_current_menu = &s_opengl_menu;
	s_current_menu_index = 1;
	
	/*	
	restore when we add the gl3.x renderer
	if ( s_ref_list[s_current_menu_index].curvalue < 2 )
	{
		s_current_menu = &s_software_menu;
		s_current_menu_index = 0;
	}
	else
	{

	}
	*/
}

static void ScreenSizeCallback( void *s )
{
	menuslider_t *slider = ( menuslider_t * ) s;

	Cvar_SetValue( "viewsize", slider->curvalue * 10 );
}

static void BrightnessCallback( void *s )
{
	menuslider_t *slider = ( menuslider_t * ) s;
	int i;

	for ( i = 0; i < MENU_COUNT; i++ )
	{
		s_brightness_slider[i].curvalue = s_brightness_slider[s_current_menu_index].curvalue;
	}

}

static void ResetDefaults( void *unused )
{
	VID_MenuInit();
}

static void ApplyChanges( void *unused )
{
	float gamma;
	int i;

	/*
	** make values consistent
	*/
	for ( i = 0; i < MENU_COUNT; i++ )
	{
		s_fs_box[i].curvalue = s_fs_box[s_current_menu_index].curvalue;
		s_brightness_slider[i].curvalue = s_brightness_slider[s_current_menu_index].curvalue;
		s_ref_list[i].curvalue = s_ref_list[s_current_menu_index].curvalue;
	}

	/*
	** invert sense so greater = brighter, and scale to a range of 0.5 to 1.3
	*/
	gamma = ( 0.8 - ( s_brightness_slider[s_current_menu_index].curvalue/10.0 - 0.5 ) ) + 0.5;

	Cvar_SetValue( "vid_gamma", gamma );
	Cvar_SetValue( "gl_picmip", 3 - s_tq_slider.curvalue );
	Cvar_SetValue( "vid_fullscreen", s_fs_box[s_current_menu_index].curvalue );
	Cvar_SetValue( "gl_ext_palettedtexture", s_paletted_texture_box.curvalue );
	Cvar_SetValue( "gl_mode", s_mode_list[OPENGL_MENU].curvalue == 0 ? -1 : s_mode_list[OPENGL_MENU].curvalue - 1 );
	Cvar_SetValue( "_windowed_mouse", s_windowed_mouse.curvalue);

	switch ( s_ref_list[s_current_menu_index].curvalue )
	{
	case REF_MESA3D :
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "libMesaGL.so.2" );
		if (gl_driver->modified)
			vid_ref->modified = true;
		break;

	case REF_OPENGLX :
		Cvar_Set( "vid_ref", "glx" );
		Cvar_Set( "gl_driver", "libGL.so" );
		if (gl_driver->modified)
			vid_ref->modified = true;
		break;

	case REF_MESA3DGLX :
		Cvar_Set( "vid_ref", "glx" );
		Cvar_Set( "gl_driver", "libMesaGL.so.2" );
		if (gl_driver->modified)
			vid_ref->modified = true;
		break;

	case REF_3DFXGL :
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "lib3dfxgl.so" );
		if (gl_driver->modified)
			vid_ref->modified = true;
		break;
	}

	M_ForceMenuOff();
}

static void CancelChanges( void *unused )
{
	extern void M_PopMenu( void );

	M_PopMenu();
}

/*
** VID_MenuInit
*/
void VID_MenuInit( void )
{
	static const char *resolutions[] = 
	{
		"[custom   ]",
		"[320 240  ]",
		"[400 300  ]",
		"[512 384  ]",
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 960 ]",
		"[1366 768 ]",
		"[1600 1200]",
		"[1920 1080]",
		"[1920 1200]",
		"[2048 1536]",
		"[2560 1440]",
		"[3840 2160]",
		0
	};
	static const char *refs[] =
	{
		"[Mesa 3-D 3DFX  ]",
		"[3DFXGL Miniport]",
		"[OpenGL glX     ]",
		"[Mesa 3-D glX   ]",
		0
	};
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *msaa_modes[] =
	{
		"no",
		"x2",
		"x4",
		"x8",
		"x16",
		0
	};
	static const char *filter_modes[] =
	{
		"nearest",
		"linear",
		"mipmap nearest",
		"mipmap linear",
		0
	};
	int i;

	if ( !gl_driver )
		gl_driver = Cvar_Get( "gl_driver", "libGL.so", 0 );
	if ( !gl_picmip )
		gl_picmip = Cvar_Get( "gl_picmip", "0", 0 );
	if ( !gl_mode )
		gl_mode = Cvar_Get( "gl_mode", "6", 0 );
	if ( !sw_mode )
		sw_mode = Cvar_Get( "sw_mode", "0", 0 );
	if ( !gl_ext_palettedtexture )
		gl_ext_palettedtexture = Cvar_Get( "gl_ext_palettedtexture", "1", CVAR_ARCHIVE );

	if ( !_windowed_mouse)
		_windowed_mouse = Cvar_Get( "_windowed_mouse", "0", CVAR_ARCHIVE );

	s_mode_list[OPENGL_MENU].curvalue = gl_mode->value < 0 ? 0 : gl_mode->value + 1;

	if ( !scr_viewsize )
		scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);

	s_screensize_slider[OPENGL_MENU].curvalue = scr_viewsize->value/10;

	if ( strcmp( vid_ref->string, "gl" ) == 0 )
	{
		s_current_menu_index = OPENGL_MENU;
		if ( strcmp( gl_driver->string, "lib3dfxgl.so" ) == 0 )
			s_ref_list[s_current_menu_index].curvalue = REF_3DFXGL;
		else
			s_ref_list[s_current_menu_index].curvalue = REF_MESA3D;
	}
	else if ( strcmp( vid_ref->string, "glx" ) == 0 )
	{
		s_current_menu_index = OPENGL_MENU;
		if ( strcmp( gl_driver->string, "libMesaGL.so.2" ) == 0 )
			s_ref_list[s_current_menu_index].curvalue = REF_MESA3DGLX;
		else
			s_ref_list[s_current_menu_index].curvalue = REF_OPENGLX;
	}

	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;
	
	for ( i = 0; i < MENU_COUNT; i++ ) //
	{
		s_ref_list[i].generic.type = MTYPE_SPINCONTROL;
		s_ref_list[i].generic.name = "driver";
		s_ref_list[i].generic.x = 0;
		s_ref_list[i].generic.y = 0;
		s_ref_list[i].generic.callback = DriverCallback;
		s_ref_list[i].itemnames = refs;

		s_mode_list[i].generic.type = MTYPE_SPINCONTROL;
		s_mode_list[i].generic.name = "video mode";
		s_mode_list[i].generic.x = 0;
		s_mode_list[i].generic.y = 10 * vid_hudscale->value;
		s_mode_list[i].itemnames = resolutions;

		s_screensize_slider[i].generic.type	= MTYPE_SLIDER;
		s_screensize_slider[i].generic.x		= 0;
		s_screensize_slider[i].generic.y		= 20 * vid_hudscale->value;
		s_screensize_slider[i].generic.name	= "screen size";
		s_screensize_slider[i].minvalue = 4;
		s_screensize_slider[i].maxvalue = 10;
		s_screensize_slider[i].generic.callback = ScreenSizeCallback;

		s_brightness_slider[i].generic.type	= MTYPE_SLIDER;
		s_brightness_slider[i].generic.x	= 0;
		s_brightness_slider[i].generic.y	= 30 * vid_hudscale->value;
		s_brightness_slider[i].generic.name	= "brightness";
		s_brightness_slider[i].generic.callback = BrightnessCallback;
		s_brightness_slider[i].minvalue = 5;
		s_brightness_slider[i].maxvalue = 13;
		s_brightness_slider[i].curvalue = ( 1.3 - vid_gamma->value + 0.5 ) * 10;

		s_fs_box[i].generic.type = MTYPE_SPINCONTROL;
		s_fs_box[i].generic.x	= 0;
		s_fs_box[i].generic.y	= 40 * vid_hudscale->value;
		s_fs_box[i].generic.name	= "fullscreen";
		s_fs_box[i].itemnames = yesno_names;
		s_fs_box[i].curvalue = vid_fullscreen->value;

		s_apply_action[i].generic.type = MTYPE_ACTION;
		s_apply_action[i].generic.name = "apply changes";
		s_apply_action[i].generic.x = 0;
		s_apply_action[i].generic.y = 150 * vid_hudscale->value;
		s_apply_action[i].generic.callback = ApplyChanges;

		s_defaults_action[i].generic.type = MTYPE_ACTION;
		s_defaults_action[i].generic.name = "reset to defaults";
		s_defaults_action[i].generic.x    = 0;
		s_defaults_action[i].generic.y    = 160 * vid_hudscale->value;
		s_defaults_action[i].generic.callback = ResetDefaults;

		s_cancel_action[i].generic.type = MTYPE_ACTION;
		s_cancel_action[i].generic.name = "cancel";
		s_cancel_action[i].generic.x    = 0;
		s_cancel_action[i].generic.y    = 170 * vid_hudscale->value;
		s_cancel_action[i].generic.callback = CancelChanges;
	}


	s_windowed_mouse.generic.type = MTYPE_SPINCONTROL;
	s_windowed_mouse.generic.x  = 0;
	s_windowed_mouse.generic.y  = 72 * vid_hudscale->value;
	s_windowed_mouse.generic.name   = "windowed mouse";
	s_windowed_mouse.curvalue = _windowed_mouse->value;
	s_windowed_mouse.itemnames = yesno_names;

	s_tq_slider.generic.type	= MTYPE_SLIDER;
	s_tq_slider.generic.x		= 0;
	s_tq_slider.generic.y		= 60 * vid_hudscale->value;
	s_tq_slider.generic.name	= "texture quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3-gl_picmip->value;

	s_paletted_texture_box.generic.type = MTYPE_SPINCONTROL;
	s_paletted_texture_box.generic.x	= 0;
	s_paletted_texture_box.generic.y	= 70 * vid_hudscale->value;
	s_paletted_texture_box.generic.name	= "8-bit textures";
	s_paletted_texture_box.itemnames = yesno_names;
	s_paletted_texture_box.curvalue = gl_ext_palettedtexture->value;

	s_vsync.generic.type = MTYPE_SPINCONTROL;
	s_vsync.generic.name = "vertical sync";
	s_vsync.generic.x = 0;
	s_vsync.generic.y = 120 * vid_hudscale->value;
	s_vsync.itemnames = yesno_names;
	s_vsync.curvalue = vk_vsync->value > 0 ? 1 : 0;

	s_postprocess.generic.type = MTYPE_SPINCONTROL;
	s_postprocess.generic.name = "postprocessing";
	s_postprocess.generic.x = 0;
	s_postprocess.generic.y = 130 * vid_hudscale->value;
	s_postprocess.itemnames = yesno_names;
	s_postprocess.curvalue = vk_postprocess->value > 0 ? 1 : 0;

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_ref_list[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_mode_list[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_screensize_slider[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_brightness_slider[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_fs_box[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_tq_slider );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_paletted_texture_box );

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_apply_action[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_defaults_action[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_cancel_action[OPENGL_MENU] );

	Menu_Center( &s_opengl_menu );
	s_opengl_menu.x -= 8 * vid_hudscale->value;
	s_opengl_menu.y += 18 * vid_hudscale->value;
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	int w, h;

	if ( s_current_menu_index == 0 )
			s_current_menu = &s_opengl_menu;
	// more for gl3.x renderer

	/*
	** draw the banner
	*/
	re.DrawGetPicSize( &w, &h, "m_banner_video" );
	re.DrawPic( viddef.width / 2 - w / 2, viddef.height /2 - 110 * vid_hudscale->value, "m_banner_video" );

	/*
	** move cursor to a reasonable starting position
	*/
	Menu_AdjustCursor( s_current_menu, 1 );

	/*
	** draw the menu
	*/
	Menu_Draw( s_current_menu );
}

/*
================
VID_MenuKey
================
*/
const char *VID_MenuKey( int key )
{
	menuframework_t *m = s_current_menu;
	static const char *sound = "misc/menu1.wav";

	switch ( key )
	{
	case K_ESCAPE:
		CancelChanges( NULL );
		return NULL;
	case K_KP_UPARROW:
	case K_UPARROW:
		m->cursor--;
		Menu_AdjustCursor( m, -1 );
		sound = "misc/menu2.wav";
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor( m, 1 );
		sound = "misc/menu2.wav";
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem( m, -1 );
		sound = "misc/menu2.wav";
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem( m, 1 );
		sound = "misc/menu2.wav";
		break;
	case K_KP_ENTER:
	case K_ENTER:
		if ( !Menu_SelectItem( m ) )
			ApplyChanges( NULL );
		break;
	}

	return sound;
}


