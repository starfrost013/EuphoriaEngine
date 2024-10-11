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

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	float		alpha = 1.0F;
	vec3_t		point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t	*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (dsprite_t *)currentmodel->extradata;
	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

	// normal sprite
	up = vup;
	right = vright;

	if ( e->flags & RF_TRANSLUCENT )
		alpha = e->alpha;

	if ( alpha != 1.0F )
		glEnable( GL_BLEND );

	glColor4f( 1, 1, 1, alpha );

    GL_Bind(currentmodel->skins[e->frame]->texnum);

	GL_TexEnv( GL_MODULATE );

	if ( alpha == 1.0 )
		glEnable (GL_ALPHA_TEST);
	else
		glDisable( GL_ALPHA_TEST );

	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA3 (e->origin, -frame->origin_y, up, point);
	VectorMA3 (point, -frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA3 (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA3 (point, -frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA3 (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA3 (point, frame->width - frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA3 (e->origin, -frame->origin_y, up, point);
	VectorMA3 (point, frame->width - frame->origin_x, right, point);
	glVertex3fv (point);
	
	glEnd ();

	glDisable (GL_ALPHA_TEST);
	GL_TexEnv( GL_REPLACE );

	if ( alpha != 1.0F )
		glDisable( GL_BLEND );

	glColor4f( 1, 1, 1, 1 );
}


/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel(model_t* mod, void* buffer)
{
	dsprite_t* sprin, * sprout;
	int32_t    i;

	sprin = (dsprite_t*)buffer;
	sprout = (dsprite_t*)Memory_HunkAlloc(modfilelen);

	sprout->ident = LittleInt(sprin->ident);
	sprout->version = LittleInt(sprin->version);
	sprout->numframes = LittleInt(sprin->numframes);

	if (sprout->version != SPRITE_VERSION)
		ri.Sys_Error(ERR_DROP, "%s has wrong version number (%i should be %i)",
			mod->name, sprout->version, SPRITE_VERSION);

	if (sprout->numframes > MAX_MD2SKINS)
		ri.Sys_Error(ERR_DROP, "%s has too many frames (%i > %i)",
			mod->name, sprout->numframes, MAX_MD2SKINS);

	// byte swap everything
	for (i = 0; i < sprout->numframes; i++)
	{
		sprout->frames[i].width = LittleInt(sprin->frames[i].width);
		sprout->frames[i].height = LittleInt(sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleInt(sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleInt(sprin->frames[i].origin_y);
		memcpy(sprout->frames[i].name, sprin->frames[i].name, MAX_SKINNAME);
		mod->skins[i] = GL_FindImage(sprout->frames[i].name,
			it_sprite);
	}

	mod->type = mod_sprite;
}

