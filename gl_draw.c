/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2023      starfrost

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

// draw.c -- 2d drawing

#include "quakedef.h"

cvar_t		scr_conalpha = {"scr_conalpha", "1"}; //johnfitz

qpic_t		*draw_disc;
qpic_t		*draw_backtile;

gltexture_t *char_texture; //johnfitz
qpic_t		*pic_ovr, *pic_ins; //johnfitz -- new cursor handling
qpic_t		*pic_nul; //johnfitz -- for missing gfx, don't crash

// zombono, now LMP32
byte pic_ovr_data[21][13] =
{
{ 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, },
{ 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, },
{ 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, },
{ 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, },
{ 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, 0xFF, },
{ 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, },
{ 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, },
{ 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, },
{ 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, },
{ 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, },
{ 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, },
{ 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x1F, },
{ 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, },
{ 0x1F, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x1F, 0x1F, 0x1F, },
{ 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, }
};

byte pic_ins_data[25][13] =
{
{ 0x09, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, },
{ 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, },
{ 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, },
{ 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, },
{ 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, },
{ 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, 0xFF, 0xEB, 0xEB, 0xEB, },
{ 0xFF, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x1F, },
{ 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0xEB, 0xEB, },
{ 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0xFF, 0x1F, 0x1F, },
{ 0x1F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

byte pic_nul_data[23][13] =
{
{ 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, },
{ 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, },
{ 0xF3, 0x93, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, },
{ 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0x00, 0x00, 0x00, 0xFF, },
{ 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, },
{ 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, },
{ 0x93, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, },
{ 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, },
{ 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, },
{ 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, },
{ 0xFF, 0xF3, 0x93, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, },
{ 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0x00, 0x00, 0x00, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF3, },
{ 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, },
{ 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, },
{ 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, },
{ 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, },
{ 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, 0xFF, 0xFF, 0xF3, 0x93, },
};

typedef struct glpic_s
{
	gltexture_t *gltexture;
	float		sl, tl, sh, th;
} glpic_t;

canvastype currentcanvas = CANVAS_NONE; //johnfitz -- for GL_SetCanvas

//==============================================================================
//
//  PIC CACHING
//
//==============================================================================

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	qpic_t		pic;
	byte		padding[32];	// for appended glpic
} cachepic_t;

#define	MAX_CACHED_PICS		384	// was 128 - but a lot more pics get cached since gfx.wad went out the window
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

byte		menuplyr_pixels[4096];

//  scrap allocation
//  Allocate all the little status bar obejcts into a single texture
//  to crutch up stupid hardware / drivers

#define	MAX_SCRAPS		2
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT]; //johnfitz -- removed *4 after BLOCK_HEIGHT
qboolean	scrap_dirty;
gltexture_t	*scrap_textures[MAX_SCRAPS]; //johnfitz

/*
================
Scrap_AllocBlock

returns an index into scrap_texnums[] and the position inside it
================
*/
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		bestx;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("Scrap_AllocBlock: full"); //johnfitz -- correct function name
	return 0; //johnfitz -- shut up compiler
}

/*
================
Scrap_Upload -- johnfitz -- now uses TexMgr
================
*/
void Scrap_Upload (void)
{
	char name[8];
	int	i;

	for (i=0; i<MAX_SCRAPS; i++)
	{
		sprintf (name, "scrap%i", i);
		scrap_textures[i] = TexMgr_LoadImage (NULL, name, BLOCK_WIDTH, BLOCK_HEIGHT, SRC_INDEXED, scrap_texels[i],
			"", (unsigned)scrap_texels[i], TEXPREF_ALPHA | TEXPREF_OVERWRITE | TEXPREF_NOPICMIP);
	}

	scrap_dirty = false;
}


/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;
	glpic_t		*gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
	menu_numcachepics++;
	strcpy (pic->name, path);

//
// load the pic from disk
//
	dat = (qpic_t *)COM_LoadTempFile (path);
	if (!dat)
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	SwapPic (dat);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu 
	// configuration dialog
	if (!strcmp (path, "gfx/menuplyr.lmp"))
		memcpy (menuplyr_pixels, dat->data, dat->width*dat->height);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	gl = (glpic_t *)pic->pic.data;
	gl->gltexture = TexMgr_LoadImage (NULL, path, dat->width, dat->height, SRC_RGBA, dat->data, path,
									  0, TEXPREF_ALPHA | TEXPREF_NOPICMIP); //johnfitz -- TexMgr

	// don't automatically scale to POT - all gpus since Geforce6000 support NPOT, but we still use POT for higher perf
	gl->sl = 0;
	gl->sh = 1; 
	gl->tl = 0;
	gl->th = 1;

	return &pic->pic;
}

/*
================
Draw_MakePic -- johnfitz -- generate pics from internal data
================
*/
qpic_t *Draw_MakePic (char *name, int width, int height, byte *data)
{
	int flags = TEXPREF_NEAREST | TEXPREF_ALPHA | TEXPREF_PERSIST | TEXPREF_NOPICMIP | TEXPREF_PAD;
	qpic_t		*pic;
	glpic_t		*gl;

	pic = Hunk_Alloc (sizeof(qpic_t) - 4 + sizeof (glpic_t));

	pic->width = width;
	pic->height = height;

	gl = (glpic_t *)pic->data;
	gl->gltexture = TexMgr_LoadImage (NULL, name, width, height, SRC_RGBA, data, "", 0, flags);


	// no longer internally scale to POT
	gl->sl = 0;
	gl->sh = 1; 
	gl->tl = 0;
	gl->th = 1;

	return pic;
}

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
===============
Draw_LoadPics -- johnfitz
===============
*/
void Draw_LoadPics (void)
{
	byte		*data;
	unsigned	offset;

	data = COM_LoadTempFile("gfx/conchars.lmp");

	if (!data) Sys_Error("Draw_LoadPics: couldn't load conchars");

	char_texture = TexMgr_LoadImage(NULL, "gfx/conchars.lmp", 128, 128, SRC_RGBA, data,
		"gfx/conchars.lmp", 0, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP);

	draw_disc = Draw_CachePic ("gfx/disc.lmp");
	draw_backtile = Draw_CachePic ("gfx/backtile.lmp");
}

/*
===============
Draw_NewGame -- johnfitz
===============
*/
void Draw_NewGame (void)
{
	gltexture_t	*glt;
	cachepic_t	*pic;
	int			i;

	// empty scrap and reallocate gltextures
	memset(&scrap_allocated, 0, sizeof(scrap_allocated));
	memset(&scrap_texels, 255, sizeof(scrap_texels));
	Scrap_Upload (); //creates 2 empty gltextures

	// reload wad pics
	Draw_LoadPics ();
	SCR_LoadPics ();
	Sbar_LoadPics ();

	// empty lmp cache
	for (pic = menu_cachepics, i = 0; i < menu_numcachepics; pic++, i++)
		pic->name[0] = 0;
	menu_numcachepics = 0;
}

/*
===============
Draw_Init -- johnfitz -- rewritten
===============
*/
void Draw_Init (void)
{
	Cvar_RegisterVariable (&scr_conalpha, NULL);

	// clear scrap and allocate gltextures
	memset(&scrap_allocated, 0, sizeof(scrap_allocated));
	memset(&scrap_texels, 255, sizeof(scrap_texels));
	Scrap_Upload (); //creates 2 empty textures

	// create internal pics
	pic_ins = Draw_MakePic ("ins", 8, 9, &pic_ins_data[0][0]);
	pic_ovr = Draw_MakePic ("ovr", 8, 8, &pic_ovr_data[0][0]);
	pic_nul = Draw_MakePic ("nul", 8, 8, &pic_nul_data[0][0]);

	// load game pics
	Draw_LoadPics ();
}

//==============================================================================
//
//  2D DRAWING
//
//==============================================================================

/*
================
Draw_CharacterQuad -- johnfitz -- seperate function to spit out verts
================
*/
void Draw_CharacterQuad (int x, int y, char num)
{
	int				row, col;
	float			frow, fcol, size;

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + size, frow );
	glVertex2f (x+8, y);
	glTexCoord2f (fcol + size, frow + size);
	glVertex2f (x+8, y+8);
	glTexCoord2f (fcol, frow + size);
	glVertex2f (x, y+8);
}

/*
================
Draw_Character -- johnfitz -- modified to call Draw_CharacterQuad
================
*/
void Draw_Character (int x, int y, int num)
{
	if (y <= -8)
		return;			// totally off screen

	num &= 255;

	if (num == 32)
		return; //don't waste verts on spaces

	GL_Bind (char_texture);
	glBegin (GL_QUADS);

	Draw_CharacterQuad (x, y, (char) num);

	glEnd ();
}

/*
================
Draw_String -- johnfitz -- modified to call Draw_CharacterQuad
================
*/
void Draw_String (int x, int y, char *str)
{
	if (y <= -8)
		return;			// totally off screen

	GL_Bind (char_texture);
	glBegin (GL_QUADS);

	while (*str)
	{
		if (*str != 32) //don't waste verts on spaces
			Draw_CharacterQuad (x, y, *str);
		str++;
		x += 8;
	}

	glEnd ();
}

/*
=============
Draw_Pic -- johnfitz -- modified
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	glpic_t			*gl;

	if (scrap_dirty)
		Scrap_Upload ();
	gl = (glpic_t *)pic->data;
	GL_Bind (gl->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, gl->tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, gl->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (gl->sh, gl->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (gl->sl, gl->th);
	glVertex2f (x, y+pic->height);
	glEnd ();
}

/*
=============
Draw_TransPicTranslate -- johnfitz -- rewritten to use texmgr to do translation

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom)
{
	static int oldtop = -2;
	static int oldbottom = -2;
	gltexture_t *glt;

	if (top != oldtop || bottom != oldbottom)
	{
		oldtop = top;
		oldbottom = bottom;
		glt = ((glpic_t *)pic->data)->gltexture;
		TexMgr_ReloadImage (glt, top, bottom);
	}
	Draw_Pic (x, y, pic);
}

/*
================
Draw_ConsoleBackground -- johnfitz -- rewritten
================
*/
void Draw_ConsoleBackground (void)
{
	qpic_t *pic;
	float alpha;

	pic = Draw_CachePic ("gfx/conback.lmp");
	pic->width = vid.conwidth;
	pic->height = vid.conheight;

	alpha = (con_forcedup) ? 1.0 : scr_conalpha.value;

//	GL_SetCanvas (CANVAS_CONSOLE); //in case this is called from weird places

	if (alpha > 0.0)
	{
		if (alpha < 1.0)
		{
			glEnable (GL_BLEND);
			glColor4f (1,1,1,alpha);
			glDisable (GL_ALPHA_TEST);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}

		Draw_Pic (0, 0, pic);

		if (alpha < 1.0)
		{
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glEnable (GL_ALPHA_TEST);
			glDisable (GL_BLEND);
			glColor4f (1,1,1,1);
		}
	}
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	glpic_t	*gl;

	gl = (glpic_t *)draw_backtile->data;

	glColor3f (1,1,1);
	GL_Bind (gl->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (x/64.0, y/64.0);
	glVertex2f (x, y);
	glTexCoord2f ( (x+w)/64.0, y/64.0);
	glVertex2f (x+w, y);
	glTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	glVertex2f (x+w, y+h);
	glTexCoord2f ( x/64.0, (y+h)/64.0 );
	glVertex2f (x, y+h);
	glEnd ();
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, float r, float g, float b, float alpha) //ZOMBONO: Don't use palette.lmp 
{
	byte *pal = (byte *)d_8to24table; //johnfitz -- use d_8to24table instead of host_basepal

	glDisable (GL_TEXTURE_2D);
	glEnable (GL_BLEND); //johnfitz -- for alpha
	glDisable (GL_ALPHA_TEST); //johnfitz -- for alpha
	glColor4f (r/255, g/255, b/255, alpha/255); //johnfitz -- added alpha

	glBegin (GL_QUADS);
	glVertex2f (x,y);
	glVertex2f (x+w, y);
	glVertex2f (x+w, y+h);
	glVertex2f (x, y+h);
	glEnd ();

	glColor3f (1,1,1);
	glDisable (GL_BLEND); //johnfitz -- for alpha
	glEnable (GL_ALPHA_TEST); //johnfitz -- for alpha
	glEnable (GL_TEXTURE_2D);
}

/*
================
Draw_FadeScreen -- johnfitz -- revised
================
*/
void Draw_FadeScreen (void)
{
	GL_SetCanvas (CANVAS_DEFAULT);

	glEnable (GL_BLEND);
	glDisable (GL_ALPHA_TEST);
	glDisable (GL_TEXTURE_2D);
	glColor4f (0, 0, 0, 0.5);
	glBegin (GL_QUADS);

	glVertex2f (0,0);
	glVertex2f (glwidth, 0);
	glVertex2f (glwidth, glheight);
	glVertex2f (0, glheight);

	glEnd ();
	glColor4f (1,1,1,1);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_ALPHA_TEST);
	glDisable (GL_BLEND);

	Sbar_Changed();
}

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	int viewport[4]; //johnfitz
	canvastype oldcanvas; //johnfitz

	if (!draw_disc)
		return;

	//johnfitz -- intel video workarounds from Baker
	if (isIntelVideo)
		return;
	//johnfitz

	//johnfitz -- canvas and matrix stuff
	glGetIntegerv (GL_VIEWPORT, viewport);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix ();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix ();
	oldcanvas = currentcanvas;
	GL_SetCanvas (CANVAS_TOPRIGHT);
	currentcanvas = oldcanvas; // a bit of a hack, since GL_SetCanvas doesn't know we are going to pop the stack
	//johnfitz

	glDrawBuffer  (GL_FRONT);
	Draw_Pic (320 - 24, 0, draw_disc);
	glDrawBuffer  (GL_BACK);

	//johnfitz -- restore everything so that 3d rendering isn't fucked up
	glMatrixMode(GL_PROJECTION);
	glPopMatrix ();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix ();
	glViewport (viewport[0], viewport[1], viewport[2], viewport[3]);
	//johnfitz
}

/*
=============================================================================

automatic byte swapping

=============================================================================
*/

void SwapPic(qpic_t* pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);
}


/*
================
GL_SetCanvas -- johnfitz -- support various canvas types
================
*/
void GL_SetCanvas (canvastype newcanvas)
{
	extern vrect_t scr_vrect;
	float s, w;
	int lines;

	if (newcanvas == currentcanvas)
		return;

	currentcanvas = newcanvas;

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();

	switch(newcanvas)
	{
	case CANVAS_DEFAULT:
		glOrtho (0, glwidth, glheight, 0, -99999, 99999);
		glViewport (glx, gly, glwidth, glheight);
		break;
	case CANVAS_CONSOLE:
		lines = vid.conheight - (scr_con_current * vid.conheight / glheight);
		glOrtho (0, vid.conwidth, vid.conheight + lines, lines, -99999, 99999);
		glViewport (glx, gly, glwidth, glheight);
		break;
	case CANVAS_MENU:
		s = min ((float)glwidth / 320.0, (float)glheight / 200.0);
		s = CLAMP (1.0, scr_menuscale.value, s);
		glOrtho (0, 320, 200, 0, -99999, 99999);
		glViewport (glx + (glwidth - 320*s) / 2, gly + (glheight - 200*s) / 2, 320*s, 200*s);
		break;
	case CANVAS_SBAR:
		s = CLAMP (1.0, scr_sbarscale.value, (float)glwidth / 320.0);
		
		glOrtho(0, glwidth / s, 48, 0, -99999, 99999);
		glViewport(glx, gly, glwidth, 48 * s);
		break;
	case CANVAS_WARPIMAGE:
		glOrtho (0, 128, 0, 128, -99999, 99999);
		glViewport (glx, gly+glheight-gl_warpimagesize, gl_warpimagesize, gl_warpimagesize);
		break;
	case CANVAS_CROSSHAIR: //0,0 is center of viewport
		s = CLAMP (1.0, scr_crosshairscale.value, 10.0);
		glOrtho (scr_vrect.width/-2/s, scr_vrect.width/2/s, scr_vrect.height/2/s, scr_vrect.height/-2/s, -99999, 99999);
		glViewport (scr_vrect.x, glheight - scr_vrect.y - scr_vrect.height, scr_vrect.width & ~1, scr_vrect.height & ~1);
		break;
	case CANVAS_BOTTOMLEFT: //used by devstats
		s = (float)glwidth/vid.conwidth; //use console scale
		glOrtho (0, 320, 200, 0, -99999, 99999);
		glViewport (glx, gly, 320*s, 200*s);
		break;
	case CANVAS_BOTTOMRIGHT: //used by fps/clock
		s = (float)glwidth/vid.conwidth; //use console scale
		glOrtho (0, 320, 200, 0, -99999, 99999);
		glViewport (glx+glwidth-320*s, gly, 320*s, 200*s);
		break;
	case CANVAS_TOPRIGHT: //used by disc
		s = 1;
		glOrtho (0, 320, 200, 0, -99999, 99999);
		glViewport (glx+glwidth-320*s, gly+glheight-200*s, 320*s, 200*s);
		break;
	default:
		Sys_Error ("GL_SetCanvas: bad canvas type");
	}

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
}

/*
================
GL_Set2D -- johnfitz -- rewritten
================
*/
void GL_Set2D (void)
{
	currentcanvas = -1;
	GL_SetCanvas (CANVAS_DEFAULT);

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);
	glColor4f (1,1,1,1);
}
