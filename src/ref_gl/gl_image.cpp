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

#include "gl_local.hpp"

image_t			gltextures[MAX_GLTEXTURES];
int32_t			numgltextures;

static uint8_t	intensitytable[256];
static uint8_t	gammatable[256];

cvar_t* intensity;

bool GL_Upload32(uint32_t* data, int32_t width, int32_t height, bool mipmap);

int32_t	gl_solid_format = 3;
int32_t	gl_alpha_format = 4;

int32_t	gl_tex_solid_format = 3;
int32_t	gl_tex_alpha_format = 4;

int32_t	gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int32_t	gl_filter_max = GL_LINEAR;

void GL_EnableMultitexture(bool enable)
{
	if (enable)
	{
		GL_SelectTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_SelectTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	}
	GL_SelectTexture(GL_TEXTURE0);
	GL_TexEnv(GL_REPLACE);
}

void GL_SelectTexture(GLenum texture)
{
	int32_t tmu;

	if (texture == GL_TEXTURE0)
	{
		tmu = 0;
	}
	else
	{
		tmu = 1;
	}

	if (tmu == gl_state.currenttmu)
	{
		return;
	}

	gl_state.currenttmu = tmu;

	glActiveTexture(texture);
	glClientActiveTexture(texture);
}

void GL_TexEnv(GLenum mode)
{
	int32_t lastmodes[2] = { -1, -1 };

	if (mode != lastmodes[gl_state.currenttmu])
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		lastmodes[gl_state.currenttmu] = mode;
	}
}

void GL_Bind(int32_t texnum)
{
	if (gl_state.currenttextures[gl_state.currenttmu] == texnum)
		return;

	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	glBindTexture(GL_TEXTURE_2D, texnum);
}

void GL_MBind(GLenum target, int32_t texnum)
{
	GL_SelectTexture(target);

	if (target == GL_TEXTURE0)
	{
		if (gl_state.currenttextures[0] == texnum)
			return;
	}
	else
	{
		if (gl_state.currenttextures[1] == texnum)
			return;
	}

	GL_Bind(texnum);
}

typedef struct glmode_s
{
	const char* name;
	int32_t	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct gltmode_s
{
	const char* name;
	int32_t mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = 
{
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = 
{
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

/*
===============
GL_TextureMode
===============
*/
void GL_SetTextureMode(const char* value)
{
	int32_t		i;
	image_t* glt;

	for (i = 0; i < NUM_GL_MODES; i++)
	{
		if (!Q_stricmp(modes[i].name, value))
			break;
	}

	if (i == NUM_GL_MODES)
	{
		ri.Con_Printf(PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky)
		{
			GL_Bind(glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_SetTextureAlphaMode(const char* string)
{
	int32_t	i;

	for (i = 0; i < NUM_GL_ALPHA_MODES; i++)
	{
		if (!Q_stricmp(gl_alpha_modes[i].name, string))
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		ri.Con_Printf(PRINT_ALL, "bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_SetTextureSolidMode(const char* string)
{
	int32_t	i;

	for (i = 0; i < NUM_GL_SOLID_MODES; i++)
	{
		if (!Q_stricmp(gl_solid_modes[i].name, string))
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		ri.Con_Printf(PRINT_ALL, "bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
GL_ImageList_f
===============
*/
void GL_ImageList_f()
{
	int32_t		i;
	image_t* image;
	int32_t		texels;

	ri.Con_Printf(PRINT_ALL, "------------------\n");
	texels = 0;

	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (image->texnum <= 0)
			continue;

		texels += image->upload_width * image->upload_height;

		switch (image->type)
		{
		case it_skin:
			ri.Con_Printf(PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Printf(PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Printf(PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Printf(PRINT_ALL, "P");
			break;
		default:
			ri.Con_Printf(PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf(PRINT_ALL, " %3i %3i: %s\n",
			image->upload_width, image->upload_height, image->name);
	}
	ri.Con_Printf(PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct targa_header_s
{
	uint8_t 	id_length, colormap_type, image_type;
	uint16_t	colormap_index, colormap_length;
	uint8_t		colormap_size;
	uint16_t	x_origin, y_origin, width, height;
	uint8_t		pixel_size, attributes;
} targa_header_t;


/*
=============
LoadTGA
=============
*/
void LoadTGA(const char* name, uint8_t** pic, int32_t* width, int32_t* height)
{
	int32_t			columns, rows, numPixels;
	uint8_t* pixbuf;
	int32_t			row, column;
	uint8_t* buf_p;
	uint8_t* buffer;
	int32_t			length;
	targa_header_t	targa_header;
	uint8_t* targa_rgba;
	uint8_t			tmp[2] = { 0 };

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_LoadFile(name, (void**)&buffer);

	if (!buffer)
	{
		// try the missing texture texture
		length = ri.FS_LoadFile("textures/missing_texture.tga", (void**)&buffer);

		// nope
		if (!buffer)
		{
			ri.Con_Printf(PRINT_DEVELOPER, "Bad tga file %s\n (and also couldn't find missing_texture.tga)\n", name);
			return;
		}

		ri.Con_Printf(PRINT_DEVELOPER, "TGA file %s not found (using placeholder texture)\n", name);
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort(*((int16_t*)tmp));
	buf_p += 2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort(*((int16_t*)tmp));
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort(*((int16_t*)buf_p));
	buf_p += 2;
	targa_header.y_origin = LittleShort(*((int16_t*)buf_p));
	buf_p += 2;
	targa_header.width = LittleShort(*((int16_t*)buf_p));
	buf_p += 2;
	targa_header.height = LittleShort(*((int16_t*)buf_p));
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type != 2
		&& targa_header.image_type != 10)
		ri.Sys_Error(ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");

	if (targa_header.colormap_type != 0
		|| (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
		ri.Sys_Error(ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = (uint8_t*)malloc(numPixels * 4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment

	if (targa_header.image_type == 2)
	{  // Uncompressed, RGB images
		for (row = rows - 1; row >= 0; row--)
		{
			pixbuf = targa_rgba + row * columns * 4;

			for (column = 0; column < columns; column++)
			{
				uint8_t red, green, blue, alphabyte;
				switch (targa_header.pixel_size)
				{
				case 24:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;
				}
			}
		}
	}
	else if (targa_header.image_type == 10)
	{
		// Runlength encoded RGB images
		uint8_t red, green, blue, alphabyte, packetHeader, packetSize, j;

		for (row = rows - 1; row >= 0; row--)
		{
			pixbuf = targa_rgba + row * columns * 4;

			for (column = 0; column < columns; )
			{
				packetHeader = *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);

				if (packetHeader & 0x80)
				{        // run-length packet
					switch (targa_header.pixel_size)
					{
					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						break;
					}

					for (j = 0; j < packetSize; j++) {
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if (column == columns)
						{ // run spans across rows
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else {                            // non run-length packet
					for (j = 0; j < packetSize; j++)
					{
						switch (targa_header.pixel_size)
						{
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
						}

						column++;

						if (column == columns)
						{ // pixel packet run spans across rows
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;

							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
		breakOut:;
		}
	}

	ri.FS_FreeFile(buffer);
}

#define RESAMPLE_SIZE		4096

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture(uint32_t* in, int32_t inwidth, int32_t inheight, uint32_t* out, int32_t outwidth, int32_t outheight)
{
	int32_t		i, j;
	uint32_t* inrow, * inrow2;
	uint32_t	frac, fracstep;
	uint32_t	p1[RESAMPLE_SIZE] = { 0 }, p2[RESAMPLE_SIZE] = { 0 };
	uint8_t* pix1, * pix2, * pix3, * pix4;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for (i = 0; i < outwidth; i++)
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}
	frac = 3 * (fracstep >> 2);
	for (i = 0; i < outwidth; i++)
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for (i = 0; i < outheight; i++, out += outwidth)
	{
		inrow = in + inwidth * (int32_t)((i + 0.25) * inheight / outheight);
		inrow2 = in + inwidth * (int32_t)((i + 0.75) * inheight / outheight);
		frac = fracstep >> 1;
		for (j = 0; j < outwidth; j++)
		{
			pix1 = (uint8_t*)inrow + p1[j];
			pix2 = (uint8_t*)inrow + p2[j];
			pix3 = (uint8_t*)inrow2 + p1[j];
			pix4 = (uint8_t*)inrow2 + p2[j];
			((uint8_t*)(out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			((uint8_t*)(out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			((uint8_t*)(out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			((uint8_t*)(out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}
}

/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void GL_LightScaleTexture(uint32_t* in, int32_t inwidth, int32_t inheight, bool only_gamma)
{
	if (only_gamma)
	{
		int32_t	i, c;
		uint8_t* p;

		p = (uint8_t*)in;

		c = inwidth * inheight;

		for (i = 0; i < c; i++, p += 4)
		{
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	}
	else
	{
		int32_t	i, c;
		uint8_t* p;

		p = (uint8_t*)in;

		c = inwidth * inheight;

		for (i = 0; i < c; i++, p += 4)
		{
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
	}
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap(uint8_t* in, int32_t width, int32_t height)
{
	int32_t		i, j;
	uint8_t* out;

	width <<= 2;
	height >>= 1;
	out = in;
	for (i = 0; i < height; i++, in += width)
	{
		for (j = 0; j < width; j += 8, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4] + in[width + 0] + in[width + 4]) >> 2;
			out[1] = (in[1] + in[5] + in[width + 1] + in[width + 5]) >> 2;
			out[2] = (in[2] + in[6] + in[width + 2] + in[width + 6]) >> 2;
			out[3] = (in[3] + in[7] + in[width + 3] + in[width + 7]) >> 2;
		}
	}
}

/*
===============
GL_Upload32

Returns has_alpha
===============
*/
int32_t		upload_width, upload_height;

uint32_t	scaled[MAX_TEXTURE_SIZE * MAX_TEXTURE_SIZE] = { 0 };

bool GL_Upload32(uint32_t* data, int32_t width, int32_t height, bool mipmap)
{
	int32_t		samples;
	int32_t		scaled_width, scaled_height;
	int32_t		i, c;
	uint8_t* scan;
	int32_t		comp;

	for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
		;
	if (gl_round_down->value && scaled_width > width && mipmap)
		scaled_width >>= 1;
	for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
		;
	if (gl_round_down->value && scaled_height > height && mipmap)
		scaled_height >>= 1;

	// let people sample down the world textures for speed
	if (mipmap)
	{
		scaled_width >>= (int32_t)gl_picmip->value;
		scaled_height >>= (int32_t)gl_picmip->value;
	}

	// don't ever bother with >256 textures
	if (scaled_width > MAX_TEXTURE_SIZE)
		scaled_width = MAX_TEXTURE_SIZE;
	if (scaled_height > MAX_TEXTURE_SIZE)
		scaled_height = MAX_TEXTURE_SIZE;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled) / 4)
		ri.Sys_Error(ERR_DROP, "GL_Upload32: too big");

	// scan the texture for any non-255 alpha
	c = width * height;
	scan = ((uint8_t*)data) + 3;
	samples = gl_solid_format;
	for (i = 0; i < c; i++, scan += 4)
	{
		if (*scan != 255)
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if (samples == gl_solid_format)
		comp = gl_tex_solid_format;
	else if (samples == gl_alpha_format)
		comp = gl_tex_alpha_format;
	else {
		ri.Con_Printf(PRINT_ALL,
			"Unknown number of texture components %i\n",
			samples);
		comp = samples;
	}

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

			goto done;
		}
		memcpy(scaled, data, width * height * 4);
	}
	else
		GL_ResampleTexture(data, width, height, scaled, scaled_width, scaled_height);

	GL_LightScaleTexture(scaled, scaled_width, scaled_height, !mipmap);

	glTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap((uint8_t*)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;

			glTexImage2D(GL_TEXTURE_2D, miplevel, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		}
	}
done:;

	if (mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	return (samples == gl_alpha_format);
}

/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t* GL_LoadPic(const char* name, uint8_t* pic, int32_t width, int32_t height, imagetype_t type)
{
	image_t* image;
	int32_t		i;

	// find a free image_t
	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!image->texnum)
			break;
	}

	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			ri.Sys_Error(ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}

	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		ri.Sys_Error(ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	strcpy(image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	image->texnum = TEXNUM_IMAGES + (image - gltextures);
	GL_Bind(image->texnum);
	image->has_alpha = GL_Upload32((uint32_t*)pic, width, height, (image->type != it_pic && image->type != it_sky));
	image->upload_width = upload_width;		// after power of 2 and scales
	image->upload_height = upload_height;
	image->sl = 0;
	image->sh = 1;
	image->tl = 0;
	image->th = 1;
	return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t* GL_FindImage(const char* name, imagetype_t type)
{
	image_t* image;
	int32_t		i, len;
	uint8_t* pic, * palette;
	int32_t		width, height;

	if (!name)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");
	len = (int32_t)strlen(name);
	if (len < 5)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

	// look for it
	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;

	if (!strcmp(name + len - 4, ".tga"))
	{
		LoadTGA(name, &pic, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
		image = GL_LoadPic(name, pic, width, height, type);
	}
	else
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name);

	if (pic)
		free(pic);

	if (palette)
		free(palette);

	return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s* R_RegisterSkin(const char* name)
{
	return GL_FindImage(name, it_skin);
}


/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages()
{
	int32_t		i;
	image_t* image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic)
			continue;		// don't free pics
		// free it
		glDeleteTextures(1, (const GLuint*)&image->texnum);
		memset(image, 0, sizeof(image_t));
	}
}


/*
===============
GL_InitImages
===============
*/
void GL_InitImages()
{
	int32_t	i, j;
	float	g = vid_gamma->value;

	registration_sequence = 1;

	// init intensity conversions
	intensity = ri.Cvar_Get("intensity", "1.2", 0);

	if (intensity->value <= 1)
		ri.Cvar_Set("intensity", "1");

	gl_state.inverse_intensity = 1 / intensity->value;

	for (i = 0; i < 256; i++)
	{
		if (g == 1)
		{
			gammatable[i] = i;
		}
		else
		{
			float inf;

			inf = 255.0f * pow((i + 0.5) / 255.5f, g) + 0.5f;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i = 0; i < 256; i++)
	{
		j = i * intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}
}

/*
===============
GL_ShutdownImages
===============
*/
void GL_ShutdownImages()
{
	int32_t		i;
	image_t* image;

	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot, don't free

		// it's used, so free it
		glDeleteTextures(1, (const GLuint*) & image->texnum);
		memset(image, 0, sizeof(image_t));
	}
}