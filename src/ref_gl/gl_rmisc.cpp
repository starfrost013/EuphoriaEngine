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
// r_misc.c

#include "gl_local.hpp"

/*
==================
R_InitParticleTexture
==================
*/
uint8_t	dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

void R_InitParticleTexture()
{
	int32_t	x, y;
	uint8_t	data[8][8][4] = { 0 };

	//
	// particle texture
	//
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y] * 255;
		}
	}
	r_particletexture = GL_LoadPic("***particle***", (uint8_t*)data, 8, 8, it_sprite);

	//
	// also use this for bad textures, but without alpha
	//
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = dottexture[x & 3][y & 3] * 255;
			data[y][x][1] = 0; // dottexture[x&3][y&3]*255;
			data[y][x][2] = 0; //dottexture[x&3][y&3]*255;
			data[y][x][3] = 255;
		}
	}
	r_notexture = GL_LoadPic("***r_notexture***", (uint8_t*)data, 8, 8, it_wall);
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

typedef struct targa_header_s
{
	uint8_t 	id_length, colormap_type, image_type;
	uint16_t	colormap_index, colormap_length;
	uint8_t		colormap_size;
	uint16_t	x_origin, y_origin, width, height;
	uint8_t		pixel_size, attributes;
} targa_header_t;

#define SCREENSHOT_BPP			32									// Bits per pixel
#define SCREENSHOT_BUFFER_SIZE	(4 * vid.width * vid.height) + 18	// Size of buffer to save screenshot into

/*
==================
GL_ScreenShot_f
==================
*/
void GL_ScreenShot_f()
{
	uint8_t* buffer;
	char	picname[MAX_OSPATH] = { 0 };
	char	checkname[MAX_OSPATH] = { 0 };
	int32_t	i, c, temp;
	FILE*	f;

	// create the scrnshots directory if it doesn't exist
	snprintf(checkname, sizeof(checkname), "%s/screenshots", ri.FS_Gamedir());
	Sys_Mkdir(checkname);

	// 
	// find a file name to save it to 
	// 

	// get the time
	// TODO: sys_time function
	time_t time_unix;
	struct tm* time_now;
	time(&time_unix);
	time_now = localtime(&time_unix);

	// todo: change
	strftime(picname, 80, "zombono-%Y-%m-%d-%H-%M-%S.tga", time_now);

	snprintf(checkname, sizeof(checkname), "%s/screenshots/%s", ri.FS_Gamedir(), picname);

	f = fopen(checkname, "rb");

	// if it exists (theoretically possible to take more than one screenshot in a second)
	if (f)
	{
		// remove the .tga (so the extension stays later
		picname[strlen(picname) - 4] = '\0';

		// this is so we don't endlessly add our concatenated date and time over and over again
		char tempbuf[128];

		int32_t n = 2;
		bool exists = true;

		while (exists)
		{
			memset(tempbuf, 0x00, sizeof(char) * 128);
			snprintf(tempbuf, 80, "%s-%d", &picname, n);

			exists = fopen(tempbuf, "rb") != NULL;

			if (n > 99) // wtf?
			{
				Com_Printf("Somehow someone saved over 100 files in a second");
				return;
			}
		}

		// restore the .tga file
		strcat(tempbuf, ".tga");

		fclose(f);
	}

	buffer = (uint8_t*)malloc(SCREENSHOT_BUFFER_SIZE);

	assert(buffer != NULL);

	memset(buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width & 255;
	buffer[13] = vid.width >> 8;
	buffer[14] = vid.height & 255;
	buffer[15] = vid.height >> 8;
	buffer[16] = 32;	// pixel size

	glReadPixels(0, 0, vid.width, vid.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer + 18);

	// swap rgba to bgra
	// OpenGL2+ supports BGRA, but to be safe on all GL 1.1 drivers that may not implement the extension,
	// we swap
	c = SCREENSHOT_BUFFER_SIZE;
	for (i = 18; i < c; i += 4)
	{
		temp = buffer[i];
		buffer[i] = buffer[i + 2];
		buffer[i + 2] = temp;
	}

	f = fopen(checkname, "wb");
	fwrite(buffer, 1, c, f);
	fclose(f);

	free(buffer);
	ri.Con_Printf(PRINT_ALL, "Saved screenshot to %s\n", checkname);
}

/*
** GL_Strings_f
*/
void GL_Strings_f(void)
{
	ri.Con_Printf(PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string);
	ri.Con_Printf(PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string);
	ri.Con_Printf(PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string);
	ri.Con_Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string);
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	glColor4f(1, 1, 1, 1);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_FLAT);

	GL_SetTextureMode(gl_texturemode->string);
	GL_SetTextureAlphaMode(gl_texturealphamode->string);
	GL_SetTextureSolidMode(gl_texturesolidmode->string);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv(GL_REPLACE);

	float attenuations[3];

	attenuations[0] = gl_particle_att_a->value;
	attenuations[1] = gl_particle_att_b->value;
	attenuations[2] = gl_particle_att_c->value;

	glEnable(GL_POINT_SMOOTH);
	glPointParameterf(GL_POINT_SIZE_MIN, gl_particle_min_size->value);
	glPointParameterf(GL_POINT_SIZE_MAX, gl_particle_max_size->value);
	glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, attenuations);
}
