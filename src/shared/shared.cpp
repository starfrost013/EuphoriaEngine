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
#include "shared.hpp"

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

vec3_t vec3_origin = { 0,0,0 };

//============================================================================
//Restore this if it breaks anything

//#ifdef _WIN32
//#pragma optimize( "", off )
//#endif

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees)
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int32_t i;
	vec3_t vr, vup, vf;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	VectorCrossProduct(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0f;

	zrot[0][0] = cosf(DEG2RAD(degrees));
	zrot[0][1] = sinf(DEG2RAD(degrees));
	zrot[1][0] = -sinf(DEG2RAD(degrees));
	zrot[1][1] = cosf(DEG2RAD(degrees));

	R_ConcatRotations(m, zrot, tmpmat);
	R_ConcatRotations(tmpmat, im, rot);

	for (i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

//#ifdef _WIN32
//#pragma optimize( "", on )
//#endif


void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float angle;
	static float sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI * 2 / 360);
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles[PITCH] * (M_PI * 2 / 360);
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles[ROLL] * (M_PI * 2 / 360);
	sr = sinf(angle);
	cr = cosf(angle);

	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
		right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
		right[2] = -1 * sr * cp;
	}
	if (up)
	{
		up[0] = (cr * sp * cy + -sr * -sy);
		up[1] = (cr * sp * sy + -sr * cy);
		up[2] = cr * cp;
	}
}


void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct3(normal, normal);

	d = DotProduct3(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src)
{
	int32_t pos;
	int32_t i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabsf(src[i]) < minelem)
		{
			pos = i;
			minelem = fabsf(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	VectorNormalize3(dst);
}

/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
}

/*
===============
LerpAngle

===============
*/
float LerpAngle(float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}


float anglemod(float a)
{
	a = (360.0f / 65536.0f) * ((int32_t)(a * (65536.0f / 360.0f)) & 65535);
	return a;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/

int32_t BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s* p)
{
	float	dist1, dist2;
	int32_t 	sides;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		assert(0);
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	assert(sides != 0);

	return sides;
}

void VectorAddPointToBounds3(vec3_t v, vec3_t mins, vec3_t maxs)
{
	int32_t i;
	vec_t	val;

	for (i = 0; i < 3; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

void VectorAddPointToBounds4(vec4_t v, vec4_t mins, vec4_t maxs)
{
	int32_t i;
	vec_t	val;

	for (i = 0; i < 4; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

bool VectorCompare3(vec3_t v1, vec3_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
		return false;

	return true;
}

bool VectorCompare4(vec4_t v1, vec4_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3])
		return false;

	return true;
}

vec_t VectorNormalize3(vec3_t v)
{
	float length, ilength;

	length = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

	if (length)
	{
		ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;

}

vec_t VectorNormalize4(vec4_t v)
{
	float length, ilength;

	length = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3]);

	if (length)
	{
		ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
		v[3] *= ilength;
	}

	return length;

}

void VectorMA3(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

void VectorMA4(vec4_t veca, float scale, vec4_t vecb, vec4_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
	vecc[3] = veca[3] + scale * vecb[3];
}

void VectorCrossProduct(vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

vec_t VectorLength3(vec3_t v)
{
	int32_t 	i;
	float	length;

	length = 0;
	for (i = 0; i < 3; i++)
		length += v[i] * v[i];
	length = sqrtf(length);		// FIXME

	return length;
}

vec_t VectorLength4(vec4_t v)
{
	int32_t i;
	float	length;

	length = 0;
	for (i = 0; i < 4; i++)
		length += v[i] * v[i];
	length = sqrtf(length);		// FIXME

	return length;
}


void VectorInverse3(vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorInverse4(vec4_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
	v[3] = -v[3];
}

void VectorScale3(vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

void VectorScale4(vec4_t in, vec_t scale, vec4_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3] * scale;
}



//====================================================================================


/*
============
COM_StripExtension
============
*/
void COM_StripExtension(char* in, char* out)
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}

/*
============
COM_FilePath

Returns the path up to, but not including the last /
============
*/
void COM_FilePath(char* in, char* out)
{
	char* s;

	s = in + strlen(in) - 1;

	while (s != in && *s != '/')
		s--;

	strncpy(out, in, s - in);
	out[s - in] = 0;
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

bool big_endian;

// can't just use function pointers, or dll linkage can
// mess up when common is included in multiple places
int16_t		(*_BigShort) (int16_t l);
int16_t		(*_LittleShort) (int16_t l);
uint16_t	(*_BigShortUnsigned) (int16_t l);
uint16_t	(*_LittleShortUnsigned) (int16_t l);
int32_t		(*_BigInt) (int32_t l);
int32_t		(*_LittleInt) (int32_t l);
uint32_t	(*_BigIntUnsigned) (int32_t l);
uint32_t	(*_LittleIntUnsigned) (int32_t l);
float (*_BigFloat) (float l);
float (*_LittleFloat) (float l);

int16_t BigShort(int16_t l) { return _BigShort(l); }
int16_t LittleShort(int16_t l) { return _LittleShort(l); }
uint16_t BigShortUnsigned(int16_t l) { return _BigShortUnsigned(l); }
uint16_t LittleShortUnsigned(int16_t l) { return _LittleShortUnsigned(l); }
int32_t  BigInt(int32_t l) { return _BigInt(l); }
int32_t  LittleInt(int32_t l) { return _LittleInt(l); }
uint32_t BigIntUnsigned(int32_t l) { return _BigIntUnsigned(l); }
uint32_t LittleIntUnsigned(int32_t l) { return _LittleIntUnsigned(l); }
float BigFloat(float l) { return _BigFloat(l); }
float LittleFloat(float l) { return _LittleFloat(l); }

int16_t ShortSwap(int16_t l)
{
	uint8_t    b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return (b1 << 8) + b2;
}

int16_t ShortNoSwap(int16_t l)
{
	return l;
}

uint16_t ShortSwapUnsigned(int16_t l)
{
	uint8_t   b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return ((uint16_t)b1 << 8) + b2;
}

uint16_t ShortNoSwapUnsigned(int16_t l)
{
	return l;
}

int32_t IntSwap(int32_t l)
{
	uint8_t    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int32_t)b1 << 24) + ((int32_t)b2 << 16) + ((int32_t)b3 << 8) + b4;
}

int32_t IntNoSwap(int32_t l)
{
	return l;
}

uint32_t IntSwapUnsigned(int32_t l)
{
	uint8_t    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((uint32_t)b1 << 24) + ((uint32_t)b2 << 16) + ((uint32_t)b3 << 8) + b4;
}

uint32_t IntNoSwapUnsigned(int32_t l)
{
	return l;
}

float FloatSwap(float f)
{
	union
	{
		float	f;
		uint8_t	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap(float f)
{
	return f;
}

/*
================
Swap_Init
================
*/
void Swap_Init()
{
	uint8_t	swaptest[2] = { 1,0 };

	// set the byte swapping variables in a portable manner	
	if (*(int16_t*)swaptest == 1)
	{
		big_endian = false;
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigShortUnsigned = ShortSwapUnsigned;
		_LittleShortUnsigned = ShortNoSwapUnsigned;
		_BigInt = IntSwap;
		_LittleInt = IntNoSwap;
		_BigIntUnsigned = IntSwapUnsigned;
		_LittleIntUnsigned = IntNoSwapUnsigned;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		big_endian = true;
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigShortUnsigned = ShortNoSwapUnsigned;
		_LittleShortUnsigned = ShortSwapUnsigned;
		_BigInt = IntNoSwap;
		_LittleInt = IntSwap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}

}

#define MAX_VARARGS_SIZE 1024

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char* va(const char* format, ...)
{
	va_list		argptr;
	static char	string[MAX_VARARGS_SIZE];

	va_start(argptr, format);
	vsnprintf(string, MAX_VARARGS_SIZE, format, argptr);
	va_end(argptr);

	return string;
}

char com_token[MAX_TOKEN_CHARS];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
const char* COM_Parse(char** data_p)
{
	int32_t c;
	int32_t len;
	char* data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*data_p = NULL;
		return "";
	}

	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return "";
		}
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c > 32);

	if (len == MAX_TOKEN_CHARS)
	{
		//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}


/*
===============
Com_PageInMemory

===============
*/
int32_t paged_total;

void Com_PageInMemory(uint8_t* buffer, int32_t size)
{
	int32_t 	i;

	for (i = size - 1; i > 0; i -= 4096)
		paged_total += buffer[i];
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

// FIXME: replace all Q_stricmp with Q_strcasecmp
int32_t Q_stricmp(const char* s1, const char* s2)
{
#if defined(WIN32)
	return _stricmp(s1, s2);
#else
	return strcasecmp(s1, s2);
#endif
}

int32_t Q_strncasecmp(const char* s1, const char* s2, int32_t n)
{
	int32_t 	c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;		// strings not equal
		}
	} 
	while (c1);

	return 0;		// strings are equal
}

int32_t Q_strcasecmp(const char* s1, const char* s2)
{
	return Q_strncasecmp(s1, s2, 99999);
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
const char* Info_ValueForKey(char* s, const char* key)
{
	char	pkey[512];
	static	char value[2][512];	// use two buffers so compares
	// work without stomping on each other
	static	int32_t valueindex;
	char* o;

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp(key, pkey))
			return value[valueindex];

		if (!*s)
			return "";
		s++;
	}
}

void Info_RemoveKey(char* s, char* key)
{
	char* start;
	char pkey[512];
	char value[512];
	char* o;

	if (strstr(key, "\\"))
	{
		//		Com_Printf ("Can't use a key with a \\\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp(key, pkey))
		{
			strcpy(start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
bool Info_Validate(char* s)
{
	if (strstr(s, "\""))
		return false;
	if (strstr(s, ";"))
		return false;
	return true;
}

void Info_SetValueForKey(char* s, char* key, char* value)
{
	char	newi[MAX_INFO_STRING], * v;
	int32_t c;
	int32_t maxsize = MAX_INFO_STRING;

	if (strstr(key, "\\") || strstr(value, "\\"))
	{
		Com_Printf("Can't use keys or values with a \\\n");
		return;
	}

	if (strstr(key, ";"))
	{
		Com_Printf("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strstr(key, "\"") || strstr(value, "\""))
	{
		Com_Printf("Can't use keys or values with a \"\n");
		return;
	}

	if (strlen(key) > MAX_INFO_KEY - 1 || strlen(value) > MAX_INFO_KEY - 1)
	{
		Com_Printf("Keys and values must be < 64 characters.\n");
		return;
	}

	Info_RemoveKey(s, key);

	if (!value || !strlen(value))
		return;

	snprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > maxsize)
	{
		Com_Printf("Info string length exceeded\n");
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newi;
	while (*v)
	{
		c = *v++;
		c &= 127;		// strip high bits
		if (c >= 32 && c < 127)
			*s++ = c;
	}
	*s = 0;
}

//====================================================================