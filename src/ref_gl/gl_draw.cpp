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

// draw.c

#include "gl_local.hpp"

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal()
{
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

/*
=============
Draw_GetScaledAssetPath

Obtains scaled asset path
=============
*/

// not thread safe buffer
char scaled_asset_path[MAX_QPATH] = { 0 };

const char* Draw_GetScaledImagePath(const char* name)
{
	if (r_scaled_assets_basewidth->value == 0
		|| r_width->value > (r_scaled_assets_basewidth->value / 2))
	{
		return name;
	}
	else if (r_width->value > (r_scaled_assets_basewidth->value / 4))
	{
		snprintf(scaled_asset_path, MAX_QPATH, "%s@0.5x", name);
	}
	else
	{
		snprintf(scaled_asset_path, MAX_QPATH, "%s@0.25x", name);
	}

	return scaled_asset_path;
}

/*
=============
Draw_FindPic
=============
*/
image_t* Draw_FindPic(const char* name)
{
	image_t* gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		snprintf(fullname, sizeof(fullname), "%s.tga", name);
		gl = GL_FindImage(fullname, it_pic);
	}
	else
		gl = GL_FindImage(name + 1, it_pic);

	return gl;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize(int32_t* w, int32_t* h, const char* pic)
{
	image_t* gl;

	gl = Draw_FindPic(pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}

	cvar_t* scale = ri.Cvar_Get("hudscale", "1", 0);

	*w = (int32_t)gl->width * scale->value;
	*h = (int32_t)gl->height * scale->value;
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int32_t x, int32_t y, const char* pic, color4_t color, bool use_scaled_assets)
{
	image_t* gl;
	cvar_t* scale = ri.Cvar_Get("hudscale", "1", 0);

	if (use_scaled_assets)
		pic = Draw_GetScaledImagePath(pic);

	gl = Draw_FindPic(pic);


	if (!gl)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL_Bind(gl->texnum);

	if (color != NULL)
	{
		glEnable(GL_BLEND);
		GL_TexEnv(GL_MODULATE); // multiply the glColor4f by the texture -> create a coloured texture.
		glColor4f(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, color[3] / 255.0f);
	}

	glBegin(GL_QUADS);
	glTexCoord2f(gl->sl, gl->tl);
	glVertex2f(x, y);
	glTexCoord2f(gl->sh, gl->tl);
	glVertex2f(x + gl->width * scale->value, y);
	glTexCoord2f(gl->sh, gl->th);
	glVertex2f(x + gl->width * scale->value, y + gl->height * scale->value);
	glTexCoord2f(gl->sl, gl->th);
	glVertex2f(x, y + gl->height * scale->value);
	glEnd();

	if (color != NULL)
	{
		glColor4f(1.0f, 
			1.0f, 
			1.0f, 
			1.0f);
		glDisable(GL_BLEND);
	}
}

/*
================
Draw_PicArea
Draws a part of an image.
================
*/
void Draw_PicArea(int32_t x, int32_t y, int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, const char* pic, color4_t color, float scale, bool use_scaled_assets)
{
	image_t* gl;

	if (use_scaled_assets)
		pic = Draw_GetScaledImagePath(pic);

	gl = Draw_FindPic(pic);

	if (!gl)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	// set up UV coordinates
	float coord_begin_x = (float)start_x * scale / (float)(gl->upload_width * scale);
	float coord_begin_y = (float)start_y * scale / (float)(gl->upload_height * scale);
	float coord_end_x = (float)end_x * scale / (float)(gl->upload_width * scale);
	float coord_end_y = (float)end_y * scale / (float)(gl->upload_height * scale);

	float size_x = end_x - start_x;
	float size_y = end_y - start_y;

	GL_Bind(gl->texnum);

	// draw it
	// set up the colour as well

	if (color != NULL)
	{
		glEnable(GL_BLEND);
		GL_TexEnv(GL_MODULATE); // multiply the glColor4f by the texture -> create a coloured texture.
		glColor4f(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, color[3] / 255.0f);
	}

	glBegin(GL_QUADS);
	glTexCoord2f(coord_begin_x, coord_begin_y);
	glVertex2f(x, y);
	glTexCoord2f(coord_end_x, coord_begin_y);
	glVertex2f(x + size_x * scale, y);
	glTexCoord2f(coord_end_x, coord_end_y);
	glVertex2f(x + size_x * scale, y + size_y * scale);
	glTexCoord2f(coord_begin_x, coord_end_y);
	glVertex2f(x, y + size_y * scale);
	glEnd();

	if (color != NULL)
	{
		glColor4f(1.0f,
			1.0f, 
			1.0f, 
			1.0f);
		glDisable(GL_BLEND);
	}
}

/*
=============
Draw_PicStretch
=============
*/
void Draw_PicStretch(int32_t x, int32_t y, int32_t w, int32_t h, const char* pic, color4_t color, bool use_scaled_assets)
{
	image_t* gl;

	if (use_scaled_assets)
		pic = Draw_GetScaledImagePath(pic);

	gl = Draw_FindPic(pic);

	if (!gl)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL_Bind(gl->texnum);

	if (color != NULL)
	{
		glEnable(GL_BLEND);
		GL_TexEnv(GL_MODULATE); // multiply the glColor4f by the texture -> create a coloured texture.
		glColor4f(color[0] / 255.0f, 
			color[1] / 255.0f, 
			color[2] / 255.0f, 
			color[3] / 255.0f);
	}

	glBegin(GL_QUADS);
	glTexCoord2f(gl->sl, gl->tl);
	glVertex2f(x, y);
	glTexCoord2f(gl->sh, gl->tl);
	glVertex2f(x + w, y);
	glTexCoord2f(gl->sh, gl->th);
	glVertex2f(x + w, y + h);
	glTexCoord2f(gl->sl, gl->th);
	glVertex2f(x, y + h);
	glEnd();

	if (color != NULL)
	{
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_BLEND);
	}
}

/*
================
Draw_PicRegion
================
*/
void Draw_PicRegion(int32_t x, int32_t y, int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, const char* pic, color4_t color, bool use_scaled_assets)
{
	cvar_t* scale = ri.Cvar_Get("hudscale", "1", 0);
	Draw_PicArea(x, y, start_x, start_y, end_x, end_y, pic, color, scale->value, use_scaled_assets);
}

/*
================
Draw_FontChar
Same as Draw_PicRegion, but it only uses integer scales
================
*/
void Draw_FontChar(int32_t x, int32_t y, int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, const char* pic, color4_t color, bool use_scaled_assets)
{
	cvar_t* scale = ri.Cvar_Get("hudscale", "1", 0);
	// truncate the scale
	float font_scale = truncf(scale->value);
	Draw_PicArea(x, y, start_x, start_y, end_x, end_y, pic, color, font_scale, false);
}

/*
=============
Load_Pic

Load an image but don't draw the image immediately.
=============
*/
void Load_Pic(const char* pic)
{
	image_t* gl;
	cvar_t* scale = ri.Cvar_Get("hudscale", "1", 0);

	gl = Draw_FindPic(pic);

	if (!gl)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int32_t x, int32_t y, int32_t w, int32_t h, const char* pic)
{
	image_t* image;

	image = Draw_FindPic(pic);

	if (!image)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL_Bind(image->texnum);
	glBegin(GL_QUADS);
	glTexCoord2f(x / 64.0f, y / 64.0f);
	glVertex2f(x, y);
	glTexCoord2f((x + w) / 64.0f, y / 64.0f);
	glVertex2f(x + w, y);
	glTexCoord2f((x + w) / 64.0f, (y + h) / 64.0f);
	glVertex2f(x + w, y + h);
	glTexCoord2f(x / 64.0f, (y + h) / 64.0f);
	glVertex2f(x, y + h);
	glEnd();

}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int32_t x, int32_t y, int32_t w, int32_t h, color4_t color)
{
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	glColor4f(color[0] / 255.0f,
		color[1] / 255.0f,
		color[2] / 255.0f,
		color[3] / 255.0f);

	glBegin(GL_QUADS);

	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);

	glEnd();
	glColor4f(1, 1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

//=============================================================================


/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen()
{
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glColor4f(0, 0, 0, 0.8f);
	glBegin(GL_QUADS);

	glVertex2f(0, 0);
	glVertex2f(vid.width, 0);
	glVertex2f(vid.width, vid.height);
	glVertex2f(0, vid.height);

	glEnd();
	glColor4f(1, 1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}
