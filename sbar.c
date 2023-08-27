/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others

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
// sbar.c -- status bar code

#include "quakedef.h"

int			sb_updates;		// if >= vid.numpages, no update needed

#define STAT_MINUS		10	// num frame for '-' stats digit
qpic_t		*sb_nums[2][11];
qpic_t		*sb_colon, *sb_slash;
qpic_t		*sb_ibar;
qpic_t		*sb_sbar;
qpic_t		*sb_scorebar;

qpic_t      *sb_weapons[7][8];   // 0 is active, 1 is owned, 2-5 are flashes
qpic_t      *sb_ammo[4];
qpic_t		*sb_sigil[4];
qpic_t		*sb_armor[3];
qpic_t		*sb_items[32];

qpic_t	*sb_faces[7][2];		// 0 is gibbed, 1 is dead, 2-6 are alive
							// 0 is static, 1 is temporary animation
qpic_t	*sb_face_invis;
qpic_t	*sb_face_quad;
qpic_t	*sb_face_invuln;
qpic_t	*sb_face_invis_invuln;

qboolean	sb_showscores;

int			sb_lines;			// scan lines to draw

void Sbar_MiniDeathmatchOverlay (void);
void Sbar_DeathmatchOverlay (void);
void M_DrawPic (int x, int y, qpic_t *pic);

/*
===============
Sbar_ShowScores

Tab key down
===============
*/
void Sbar_ShowScores (void)
{
	if (sb_showscores)
		return;
	sb_showscores = true;
	sb_updates = 0;
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
void Sbar_DontShowScores (void)
{
	sb_showscores = false;
	sb_updates = 0;
}

/*
===============
Sbar_Changed
===============
*/
void Sbar_Changed (void)
{
	sb_updates = 0;	// update next frame
}

/*
===============
Sbar_LoadPics -- johnfitz -- load all the sbar pics
===============
*/
void Sbar_LoadPics (void)
{
	int		i;

	for (i=0 ; i<10 ; i++)
	{
		sb_nums[0][i] = Draw_CachePic (va("gfx/num_%i.lmp",i));
		sb_nums[1][i] = Draw_CachePic (va("gfx/anum_%i.lmp",i));
	}

	sb_nums[0][10] = Draw_CachePic("gfx/num_minus.lmp");
	sb_nums[1][10] = Draw_CachePic("gfx/anum_minus.lmp");

	sb_colon = Draw_CachePic ("gfx/num_colon.lmp");
	sb_slash = Draw_CachePic ("gfx/num_slash.lmp");

	sb_weapons[0][0] = Draw_CachePic ("gfx/inv_shotgun.lmp");
	sb_weapons[0][1] = Draw_CachePic ("gfx/inv_sshotgun.lmp");
	sb_weapons[0][2] = Draw_CachePic ("gfx/inv_nailgun.lmp");
	sb_weapons[0][3] = Draw_CachePic ("gfx/inv_snailgun.lmp");
	sb_weapons[0][4] = Draw_CachePic ("gfx/inv_rlaunch.lmp");
	sb_weapons[0][5] = Draw_CachePic ("gfx/inv_srlaunch.lmp");
	sb_weapons[0][6] = Draw_CachePic ("gfx/inv_lightng.lmp");

	sb_weapons[1][0] = Draw_CachePic ("gfx/inv2_shotgun.lmp");
	sb_weapons[1][1] = Draw_CachePic ("gfx/inv2_sshotgun.lmp");
	sb_weapons[1][2] = Draw_CachePic ("gfx/inv2_nailgun.lmp");
	sb_weapons[1][3] = Draw_CachePic ("gfx/inv2_snailgun.lmp");
	sb_weapons[1][4] = Draw_CachePic ("gfx/inv2_rlaunch.lmp");
	sb_weapons[1][5] = Draw_CachePic ("gfx/inv2_srlaunch.lmp");
	sb_weapons[1][6] = Draw_CachePic ("gfx/inv2_lightng.lmp");

	for (i=0 ; i<5 ; i++)
	{
		sb_weapons[2+i][0] = Draw_CachePic (va("gfx/inva%i_shotgun.lmp",i+1));
		sb_weapons[2+i][1] = Draw_CachePic (va("gfx/inva%i_sshotgun.lmp",i+1));
		sb_weapons[2+i][2] = Draw_CachePic (va("gfx/inva%i_nailgun.lmp",i+1));
		sb_weapons[2+i][3] = Draw_CachePic (va("gfx/inva%i_snailgun.lmp",i+1));
		sb_weapons[2+i][4] = Draw_CachePic (va("gfx/inva%i_rlaunch.lmp",i+1));
		sb_weapons[2+i][5] = Draw_CachePic (va("gfx/inva%i_srlaunch.lmp",i+1));
		sb_weapons[2+i][6] = Draw_CachePic (va("gfx/inva%i_lightng.lmp",i+1));
	}

	sb_ammo[0] = Draw_CachePic ("gfx/sb_shells.lmp");
	sb_ammo[1] = Draw_CachePic ("gfx/sb_nails.lmp");
	sb_ammo[2] = Draw_CachePic ("gfx/sb_rocket.lmp");
	sb_ammo[3] = Draw_CachePic ("gfx/sb_cells.lmp");

	sb_armor[0] = Draw_CachePic ("gfx/sb_armor1.lmp");
	sb_armor[1] = Draw_CachePic ("gfx/sb_armor2.lmp");
	sb_armor[2] = Draw_CachePic ("gfx/sb_armor3.lmp");

	sb_items[0] = Draw_CachePic ("gfx/sb_key1.lmp");
	sb_items[1] = Draw_CachePic ("gfx/sb_key2.lmp");
	sb_items[2] = Draw_CachePic ("gfx/sb_invis.lmp");
	sb_items[3] = Draw_CachePic ("gfx/sb_invuln.lmp");
	sb_items[4] = Draw_CachePic ("gfx/sb_suit.lmp");
	sb_items[5] = Draw_CachePic ("gfx/sb_quad.lmp");

	sb_sigil[0] = Draw_CachePic ("gfx/sb_sigil1.lmp");
	sb_sigil[1] = Draw_CachePic ("gfx/sb_sigil2.lmp");
	sb_sigil[2] = Draw_CachePic ("gfx/sb_sigil3.lmp");
	sb_sigil[3] = Draw_CachePic ("gfx/sb_sigil4.lmp");

	sb_faces[4][0] = Draw_CachePic ("gfx/face1.lmp");
	sb_faces[4][1] = Draw_CachePic ("gfx/face_p1.lmp");
	sb_faces[3][0] = Draw_CachePic ("gfx/face2.lmp");
	sb_faces[3][1] = Draw_CachePic ("gfx/face_p2.lmp");
	sb_faces[2][0] = Draw_CachePic ("gfx/face3.lmp");
	sb_faces[2][1] = Draw_CachePic ("gfx/face_p3.lmp");
	sb_faces[1][0] = Draw_CachePic ("gfx/face4.lmp");
	sb_faces[1][1] = Draw_CachePic ("gfx/face_p4.lmp");
	sb_faces[0][0] = Draw_CachePic ("gfx/face5.lmp");
	sb_faces[0][1] = Draw_CachePic ("gfx/face_p5.lmp");

	sb_face_invis = Draw_CachePic ("gfx/face_invis.lmp");
	sb_face_invuln = Draw_CachePic ("gfx/face_invul2.lmp");
	sb_face_invis_invuln = Draw_CachePic ("gfx/face_inv2.lmp");
	sb_face_quad = Draw_CachePic ("gfx/face_quad.lmp");

	sb_sbar = Draw_CachePic ("gfx/sbar.lmp");
	sb_ibar = Draw_CachePic ("gfx/ibar.lmp");
	sb_scorebar = Draw_CachePic ("gfx/scorebar.lmp");
}

/*
===============
Sbar_Init -- johnfitz -- rewritten
===============
*/
void Sbar_Init (void)
{
	Cmd_AddCommand ("+showscores", Sbar_ShowScores);
	Cmd_AddCommand ("-showscores", Sbar_DontShowScores);

	Sbar_LoadPics ();
}


//=============================================================================

// drawing routines are relative to the status bar location

/*
=============
Sbar_DrawPic -- johnfitz -- rewritten now that GL_SetCanvas is doing the work
=============
*/
void Sbar_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y + 24, pic);
}

/*
=============
Sbar_DrawPicAlpha -- johnfitz
=============
*/
Sbar_DrawPicAlpha (int x, int y, qpic_t *pic, float alpha)
{
	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glColor4f(1,1,1,alpha);
	Draw_Pic (x, y + 24, pic);
	glColor3f(1,1,1);
	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);
}

/*
================
Sbar_DrawCharacter -- johnfitz -- rewritten now that GL_SetCanvas is doing the work
================
*/
void Sbar_DrawCharacter (int x, int y, int num)
{
	Draw_Character (x, y + 24, num);
}

/*
================
Sbar_DrawString -- johnfitz -- rewritten now that GL_SetCanvas is doing the work
================
*/
void Sbar_DrawString (int x, int y, char *str)
{
	Draw_String (x, y + 24, str);
}

/*
===============
Sbar_DrawScrollString -- johnfitz

scroll the string inside a glscissor region
===============
*/
void Sbar_DrawScrollString (int x, int y, int width, char* str)
{
	float scale;
	int len, ofs, left;

	scale = CLAMP (1.0, scr_sbarscale.value, (float)glwidth / 320.0);
	left = x * scale;

	glEnable (GL_SCISSOR_TEST);
	glScissor (left, 0, width * scale, glheight);

	len = strlen(str)*8 + 40;
	ofs = ((int)(realtime*30))%len;
	Sbar_DrawString (x - ofs, y, str);
	Sbar_DrawCharacter (x - ofs + len - 32, y, '/');
	Sbar_DrawCharacter (x - ofs + len - 24, y, '/');
	Sbar_DrawCharacter (x - ofs + len - 16, y, '/');
	Sbar_DrawString (x - ofs + len, y, str);

	glDisable (GL_SCISSOR_TEST);
}

/*
=============
Sbar_itoa
=============
*/
int Sbar_itoa (int num, char *buf)
{
	char	*str;
	int		pow10;
	int		dig;

	str = buf;

	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for (pow10 = 10 ; num >= pow10 ; pow10 *= 10)
	;

	do
	{
		pow10 /= 10;
		dig = num/pow10;
		*str++ = '0'+dig;
		num -= dig*pow10;
	} while (pow10 != 1);

	*str = 0;

	return str-buf;
}


/*
=============
Sbar_DrawNum
=============
*/
void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	num = min(999,num); //johnfitz -- cap high values rather than truncating number

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawPic (x,y,sb_nums[color][frame]); //johnfitz -- DrawTransPic is obsolete
		x += 24;
		ptr++;
	}
}

//=============================================================================

int		fragsort[MAX_CLIENTS];

char	scoreboardtext[MAX_CLIENTS][20];
int		scoreboardtop[MAX_CLIENTS];
int		scoreboardbottom[MAX_CLIENTS];
int		scoreboardcount[MAX_CLIENTS];
int		scoreboardlines;

/*
===============
Sbar_SortScoreboard

Renamed for clarity
===============
*/
void Sbar_SortScoreboard (void)
{
	int		i, j, k;

// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<cl.maxclients ; i++)
	{

		if (cl.scores[i].name[0])
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
		}
	}
}

/*
===============
Sbar_UpdateScoreboard
===============
*/
void Sbar_UpdateScoreboard (void)
{
	int		i, k;
	int		top, bottom;
	scoreboard_t	*s;

	Sbar_SortScoreboard ();

// draw the text
	memset (scoreboardtext, 0, sizeof(scoreboardtext));

	for (i=0 ; i<scoreboardlines; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		sprintf (&scoreboardtext[i][1], "%3i %s", s->frags, s->name);
	}
}

/*
===============
Sbar_SoloScoreboard -- johnfitz -- new layout
===============
*/
void Sbar_SoloScoreboard (void)
{
	char	str[80];
	int		minutes, seconds, tens, units;
	int		len;

	sprintf (str,"Kills: %i/%i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	Sbar_DrawString (8, 12, str);

	sprintf (str,"Secrets: %i/%i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
	Sbar_DrawString (312 - strlen(str)*8, 12, str);

	minutes = cl.time / 60;
	seconds = cl.time - 60*minutes;
	tens = seconds / 10;
	units = seconds - 10*tens;
	sprintf (str,"%i:%i%i", minutes, tens, units);
	Sbar_DrawString (160 - strlen(str)*4, 12, str);

	len = strlen (cl.levelname);
	if (len > 40)
		Sbar_DrawScrollString (0, 4, 320, cl.levelname);
	else
		Sbar_DrawString (160 - len*4, 4, cl.levelname);
}

/*
===============
Sbar_DrawScoreboard
===============
*/
void Sbar_DrawScoreboard (void)
{
	Sbar_SoloScoreboard ();
	Sbar_DeathmatchOverlay ();
}

//=============================================================================

/*
===============
Sbar_DrawInventory
===============
*/
void Sbar_DrawInventory (void)
{
	int		i;
	char	num[6];
	float	time;
	int		flashon;

	Sbar_DrawPicAlpha(0, -24, sb_ibar, scr_sbaralpha.value); //johnfitz -- scr_sbaralpha

// weapons
	for (i=0 ; i<7 ; i++)
	{
		if (cl.items & (IT_SHOTGUN<<i) )
		{
			time = cl.item_gettime[i];
			flashon = (int)((cl.time - time)*10);
			if (flashon >= 10)
			{
				if ( cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i)  )
					flashon = 1;
				else
					flashon = 0;
			}
			else
				flashon = (flashon%5) + 2;

         Sbar_DrawPic (i*24, -16, sb_weapons[flashon][i]);

			if (flashon > 1)
				sb_updates = 0;		// force update to remove flash
		}
	}

// ammo counts
	for (i=0 ; i<4 ; i++)
	{
		sprintf (num, "%3i", min(999,cl.stats[STAT_SHELLS+i])); //johnfitz -- cap displayed value to 999
		if (num[0] != ' ')
			Sbar_DrawCharacter ( (6*i+1)*8 + 2, -24, 18 + num[0] - '0');
		if (num[1] != ' ')
			Sbar_DrawCharacter ( (6*i+2)*8 + 2, -24, 18 + num[1] - '0');
		if (num[2] != ' ')
			Sbar_DrawCharacter ( (6*i+3)*8 + 2, -24, 18 + num[2] - '0');
	}

	flashon = 0;
   // items
   for (i=0 ; i<6 ; i++)
      if (cl.items & (1<<(17+i)))
      {
         time = cl.item_gettime[17+i];
         if (time && time > cl.time - 2 && flashon )
         {  // flash frame
            sb_updates = 0;
         }
         else
         {
         //MED 01/04/97 changed keys
			 Sbar_DrawPic(192 + i * 16, -16, sb_items[i]);
         }
         if (time && time > cl.time - 2)
            sb_updates = 0;
      }

   for (i = 0; i < 4; i++)
   {
	   if (cl.items & (1 << (28 + i)))
	   {
		   time = cl.item_gettime[28 + i];
		   if (time && time > cl.time - 2 && flashon)
		   {	// flash frame
			   sb_updates = 0;
		   }
		   else
			   Sbar_DrawPic(320 - 32 + i * 8, -16, sb_sigil[i]);
		   if (time && time > cl.time - 2)
			   sb_updates = 0;
	   }
   }
}

//=============================================================================

/*
===============
Sbar_DrawFrags -- johnfitz -- heavy revision
===============
*/
void Sbar_DrawFrags (void)
{
	int				numscores, i, x, color;
	char			num[12];
	scoreboard_t	*s;

	Sbar_SortScoreboard ();

// draw the text
	numscores = min (scoreboardlines, 4);

	for (i=0, x=184; i<numscores; i++, x+=32)
	{
		s = &cl.scores[fragsort[i]];
		if (!s->name[0] || !svs.clients[i].active)
			continue;

		// top color
		// get team color
		color4_t color = TEAM_GetColor(svs.clients[i].edict->v.team);

		// number
		sprintf(num, "%3i", s->frags);
		Sbar_DrawCharacter(x + 12, -24, num[0]);
		Sbar_DrawCharacter(x + 20, -24, num[1]);
		Sbar_DrawCharacter(x + 28, -24, num[2]);

	// brackets
		if (fragsort[i] == cl.viewentity - 1)
		{
			Sbar_DrawCharacter (x + 6, -24, 16);
			Sbar_DrawCharacter (x + 32, -24, 17);
		}
	}
}

//=============================================================================


/*
===============
Sbar_DrawFace
===============
*/
void Sbar_DrawFace (void)
{
	int		f, anim;

// PGM 01/19/97 - team color drawing
// PGM 03/02/97 - fixed so color swatch only appears in CTF modes

// PGM 01/19/97 - team color drawing

	if ( (cl.items & (IT_INVISIBILITY | IT_INVULNERABILITY) )
	== (IT_INVISIBILITY | IT_INVULNERABILITY) )
	{
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
		return;
	}
	if (cl.items & IT_QUAD)
	{
		Sbar_DrawPic (112, 0, sb_face_quad );
		return;
	}
	if (cl.items & IT_INVISIBILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invis );
		return;
	}
	if (cl.items & IT_INVULNERABILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invuln);
		return;
	}

	if (cl.stats[STAT_HEALTH] >= 100)
		f = 4;
	else
		f = cl.stats[STAT_HEALTH] / 20;

	if (cl.time <= cl.faceanimtime)
	{
		anim = 1;
		sb_updates = 0;		// make sure the anim gets drawn over
	}
	else
		anim = 0;
	Sbar_DrawPic (112, 0, sb_faces[f][anim]);
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw (void)
{
	float w; //johnfitz

	if (scr_con_current == vid.height)
		return;		// console is full screen

	if (cl.intermission)
		return; //johnfitz -- never draw sbar during intermission

	if (sb_updates >= vid.numpages && !gl_clear.value && scr_sbaralpha.value >= 1 && !isIntelVideo) //johnfitz -- gl_clear, scr_sbaralpha, intel workarounds from baker
		return;

	sb_updates++;

	GL_SetCanvas (CANVAS_DEFAULT); //johnfitz

	//johnfitz -- don't waste fillrate by clearing the area behind the sbar
	w = CLAMP (320.0f, scr_sbarscale.value * 320.0f, (float)glwidth);
	if (sb_lines && glwidth > w)
	{
		if (scr_sbaralpha.value < 1)
			Draw_TileClear (0, glheight - sb_lines, glwidth, sb_lines);

		Draw_TileClear (w, glheight - sb_lines, glwidth - w, sb_lines);

	}
	//johnfitz

	GL_SetCanvas (CANVAS_SBAR); //johnfitz

	if (scr_viewsize.value < 110) //johnfitz -- check viewsize instead of sb_lines
	{
		Sbar_DrawInventory ();
		if (cl.maxclients != 1)
			Sbar_DrawFrags ();
	}

	if (sb_showscores || cl.stats[STAT_HEALTH] <= 0)
	{
		Sbar_DrawPicAlpha (0, 0, sb_scorebar, scr_sbaralpha.value); //johnfitz -- scr_sbaralpha
		Sbar_DrawScoreboard ();
		sb_updates = 0;
	}
	else if (scr_viewsize.value < 120) //johnfitz -- check viewsize instead of sb_lines
	{
		Sbar_DrawPicAlpha (0, 0, sb_sbar, scr_sbaralpha.value); //johnfitz -- scr_sbaralpha

		//MED 01/04/97 moved keys here so they would not be overwritten

   // armor
		if (cl.items & IT_INVULNERABILITY)
		{
			Sbar_DrawNum (24, 0, 666, 3, 1);
			Sbar_DrawPic (0, 0, draw_disc);
		}
		else
		{
			Sbar_DrawNum(24, 0, cl.stats[STAT_ARMOR], 3
				, cl.stats[STAT_ARMOR] <= 25);
			if (cl.items & IT_ARMOR3)
				Sbar_DrawPic(0, 0, sb_armor[2]);
			else if (cl.items & IT_ARMOR2)
				Sbar_DrawPic(0, 0, sb_armor[1]);
			else if (cl.items & IT_ARMOR1)
				Sbar_DrawPic(0, 0, sb_armor[0]);
		}

	// face
		Sbar_DrawFace ();

	// health
		Sbar_DrawNum (136, 0, cl.stats[STAT_HEALTH], 3
		, cl.stats[STAT_HEALTH] <= 25);

	// ammo icon
		if (cl.items & IT_SHELLS)
			Sbar_DrawPic(224, 0, sb_ammo[0]);
		else if (cl.items & IT_NAILS)
			Sbar_DrawPic(224, 0, sb_ammo[1]);
		else if (cl.items & IT_ROCKETS)
			Sbar_DrawPic(224, 0, sb_ammo[2]);
		else if (cl.items & IT_CELLS)
			Sbar_DrawPic(224, 0, sb_ammo[3]);

		Sbar_DrawNum (248, 0, cl.stats[STAT_AMMO], 3,
					  cl.stats[STAT_AMMO] <= 10);
	}

	//johnfitz -- removed the vid.width > 320 check here
	Sbar_MiniDeathmatchOverlay ();
}

//=============================================================================

/*
==================
Sbar_IntermissionNumber

==================
*/
void Sbar_IntermissionNumber (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_Pic (x,y,sb_nums[color][frame]); //johnfitz -- stretched menus
		x += 24;
		ptr++;
	}
}

#define SCOREBOARD_LINE_SIZE 10
/*
==================
Sbar_DeathmatchOverlay

==================
*/
void Sbar_DeathmatchOverlay (void)
{
	qpic_t			*pic;
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	scoreboard_t	*s;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/ranking.lmp");
	M_DrawPic ((320-pic->width)/2, 8, pic);

// scores
	Sbar_SortScoreboard ();

// draw the text

	x = 80; //johnfitz -- simplified becuase some positioning is handled elsewhere
	y = 40;

	//offset of the "player" string.
	int director_position = y;
	int player_position = y + SCOREBOARD_LINE_SIZE * svs.maxclients / 2; // draw player at minimum of player count

	int num_directors = 0;
	int num_players = 0;

	Draw_String(x, director_position, "D I R E C T O R S");
	Draw_String(x, player_position, "P L A Y E R S");

	for (i=0 ; i<svs.maxclients ; i++)// let's hope these are the same indiceslmao
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// draw background
		
		if (!svs.clients[i].active) continue;

		int team = svs.clients[i].edict->v.team;

		if (team == ZOMBONO_TEAM_DIRECTOR)
		{
			num_directors++;
			y = 40 + SCOREBOARD_LINE_SIZE * (num_directors);
		}
		else if (team == ZOMBONO_TEAM_PLAYER)
		{
			num_players++;
			y = player_position + (SCOREBOARD_LINE_SIZE * num_players);
		}

		color4_t team_color = TEAM_GetColor(team);

	// based on team

		Draw_Fill ( x, y, 40, 10, team_color.r, team_color.g, team_color.b, team_color.alpha); //johnfitz -- stretched overlays

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		Draw_Character ( x+8 , y, num[0]); //johnfitz -- stretched overlays
		Draw_Character ( x+16 , y, num[1]); //johnfitz -- stretched overlays
		Draw_Character ( x+24 , y, num[2]); //johnfitz -- stretched overlays

		if (k == cl.viewentity - 1)
			Draw_Character ( x - 8, y, 12); //johnfitz -- stretched overlays

	// draw name
		M_Print (x+64, y, s->name); //johnfitz -- was Draw_String, changed for stretched overlays
	}

	GL_SetCanvas (CANVAS_SBAR); //johnfitz
}

/*
==================
Sbar_MiniDeathmatchOverlay
==================
*/
void Sbar_MiniDeathmatchOverlay (void)
{
	int				i, k, l, top, bottom, x, y, f, numlines;
	char			num[12];
	float			scale; //johnfitz
	qpic_t			*pic;
	scoreboard_t	*s;

	scale = CLAMP (1.0, scr_sbarscale.value, (float)glwidth / 320.0); //johnfitz

	//MAX_SCOREBOARDNAME = 32, so total width for this overlay plus sbar is 632, but we can cut off some i guess
	if (glwidth/scale < 512 || scr_viewsize.value >= 120) //johnfitz -- test should consider scr_sbarscale
		return;

// scores
	Sbar_SortScoreboard ();

// draw the text
	l = scoreboardlines;
	numlines = (scr_viewsize.value >= 110) ? 3 : 6; //johnfitz

	//find us
	for (i = 0; i < scoreboardlines; i++)
		if (fragsort[i] == cl.viewentity - 1)
			break;
    if (i == scoreboardlines) // we're not there
            i = 0;
    else // figure out start
            i = i - numlines/2;
    if (i > scoreboardlines - numlines)
            i = scoreboardlines - numlines;
    if (i < 0)
            i = 0;

	x = 324;
	y = (scr_viewsize.value >= 110) ? 24 : 0; //johnfitz -- start at the right place
	for ( ; i < svs.maxclients && y <= 48; i++, y+=8) //johnfitz -- change y init, test, inc
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0] || !svs.clients[i].active) // check if not connected
			continue;

	// colors
	
		color4_t color = TEAM_GetColor(svs.clients[i].edict->v.team);

		Draw_Fill(x, y + 1, 40, 7, color.r, color.g, color.b, color.alpha);
		Draw_Fill(x, y + 5, 40, 3, color.r, color.g, color.b, color.alpha);


	// number
		f = s->frags;
		sprintf (num, "%3i",f);
		Draw_Character ( x+8 , y, num[0]);
		Draw_Character ( x+16 , y, num[1]);
		Draw_Character ( x+24 , y, num[2]);

	// brackets
		if (k == cl.viewentity - 1)
		{
			Draw_Character ( x, y, 16);
			Draw_Character ( x+32, y, 17);
		}

	// name
		Draw_String (x+48, y, s->name);
	}
}

/*
==================
Sbar_IntermissionOverlay
==================
*/
void Sbar_IntermissionOverlay (void)
{
	qpic_t	*pic;
	int		dig;
	int		num;

	Sbar_DeathmatchOverlay();
	return; 

	/* might put back later
	GL_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/complete.lmp");
	Draw_Pic (64, 24, pic);

	pic = Draw_CachePic ("gfx/inter.lmp");
	Draw_Pic (0, 56, pic);

	dig = cl.completed_time/60;
	Sbar_IntermissionNumber (152, 64, dig, 3, 0); //johnfitz -- was 160
	num = cl.completed_time - dig*60;
	Draw_Pic (224,64,sb_colon); //johnfitz -- was 234
	Draw_Pic (240,64,sb_nums[0][num/10]); //johnfitz -- was 246
	Draw_Pic (264,64,sb_nums[0][num%10]); //johnfitz -- was 266

	Sbar_IntermissionNumber (152, 104, cl.stats[STAT_SECRETS], 3, 0); //johnfitz -- was 160
	Draw_Pic (224,104,sb_slash); //johnfitz -- was 232
	Sbar_IntermissionNumber (240, 104, cl.stats[STAT_TOTALSECRETS], 3, 0); //johnfitz -- was 248

	Sbar_IntermissionNumber (152, 144, cl.stats[STAT_MONSTERS], 3, 0); //johnfitz -- was 160
	Draw_Pic (224,144,sb_slash); //johnfitz -- was 232
	Sbar_IntermissionNumber (240, 144, cl.stats[STAT_TOTALMONSTERS], 3, 0); //johnfitz -- was 248
	*/
}

