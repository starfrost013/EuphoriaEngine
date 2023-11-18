#include "defs.h"

/*  Copyright (C) 1996-1997  Id Software, Inc.
	Copyright (C) 2023       starfrost

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

/*

A monster is in fight mode if it thinks it can effectively attack its
enemy.

When it decides it can't attack, it goes into hunt mode.

*/

float anglemod(float v);

void knight_atk1();
void knight_runatk1();
void ogre_smash1();
void ogre_swing1();

void sham_smash1();
void sham_swingr1();
void sham_swingl1();

float DemonCheckAttack();
void Demon_Melee(float side);

void ChooseTurn(vec3_t dest);

void ai_face();


float enemy_vis, enemy_infront, enemy_range;
float enemy_yaw;


void knight_attack()
{
	float len;

// decide if now is a good swing time
	len = vlen(self.enemy.origin+self.enemy.view_ofs - (self.origin+self.view_ofs));

	if (len<80)
		knight_atk1();
	else
		knight_runatk1();
}

//=============================================================================

/*
===========
CheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
float CheckAttack()
{
	vec3_t spot1, spot2;
	entvars_t targ;
	float chance;

	targ = self.enemy;

// see if any entities are in the way of the shot
	spot1 = self.origin + self.view_ofs;
	spot2 = targ.origin + targ.view_ofs;

	traceline(spot1, spot2, FALSE, self);

	if (trace_ent != targ)
		return FALSE; // don't have a clear shot

	if (trace_inopen && trace_inwater)
		return FALSE; // sight line crossed contents

	if (enemy_range == RANGE_MELEE)
	{ // melee attack
		if (self.th_melee)
		{
			if (self.classname == "monster_knight")
				knight_attack();
			else
				self.th_melee();
			return TRUE;
		}
	}

// missile attack
	if (!self.th_missile)
		return FALSE;

	if (time < self.attack_finished)
		return FALSE;

	if (enemy_range == RANGE_FAR)
		return FALSE;

	if (enemy_range == RANGE_MELEE)
	{
		chance = 0.9;
		self.attack_finished = 0;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		if (self.th_melee)
			chance = 0.2;
		else
			chance = 0.4;
	}
	else if (enemy_range == RANGE_MID)
	{
		if (self.th_melee)
			chance = 0.05;
		else
			chance = 0.1;
	}
	else
		chance = 0;

	if (random() < chance)
	{
		self.th_missile();
		SUB_AttackFinished(2*random());
		return TRUE;
	}

	return FALSE;
}


/*
=============
ai_face

Stay facing the enemy
=============
*/
void ai_face()
{
	self.ideal_yaw = vectoyaw(self.enemy.origin - self.origin);
	ChangeYaw();
}

/*
=============
ai_charge

The monster is in a melee attack, so get as close as possible to .enemy
=============
*/
float visible(entvars_t targ);
float infront(entvars_t targ);
float range(entvars_t targ);

void ai_charge(float d)
{
	ai_face();
	movetogoal(d); // done in C code...
}

void ai_charge_side()
{
	vec3_t dtemp;
	float heading;

// aim to the left of the enemy for a flyby

	self.ideal_yaw = vectoyaw(self.enemy.origin - self.origin);
	ChangeYaw();

	makevectors(self.angles);
	dtemp = self.enemy.origin - 30*v_right;
	heading = vectoyaw(dtemp - self.origin);

	walkmove(heading, 20);
}

/*
=============
ai_melee

=============
*/
void ai_melee()
{
	vec3_t delta;
	float ldmg;

	if (!self.enemy)
		return; // removed before stroke

	delta = self.enemy.origin - self.origin;

	if (vlen(delta) > 60)
		return;

	ldmg = (random() + random() + random()) * 3;
	T_Damage(self.enemy, self, self, ldmg);
}


void ai_melee_side()
{
	vec3_t delta;
	float ldmg;

	if (!self.enemy)
		return; // removed before stroke

	ai_charge_side();

	delta = self.enemy.origin - self.origin;

	if (vlen(delta) > 60)
		return;
	if (!CanDamage(self.enemy, self))
		return;
	ldmg = (random() + random() + random()) * 3;
	T_Damage(self.enemy, self, self, ldmg);
}


//=============================================================================

/*
===========
SoldierCheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
float SoldierCheckAttack()
{
	vec3_t spot1, spot2;
	entvars_t targ;
	float chance;

	targ = self.enemy;

// see if any entities are in the way of the shot
	spot1 = self.origin + self.view_ofs;
	spot2 = targ.origin + targ.view_ofs;

	traceline(spot1, spot2, FALSE, self);

	if (trace_inopen && trace_inwater)
		return FALSE; // sight line crossed contents

	if (trace_ent != targ)
		return FALSE; // don't have a clear shot


// missile attack
	if (time < self.attack_finished)
		return FALSE;

	if (enemy_range == RANGE_FAR)
		return FALSE;

	if (enemy_range == RANGE_MELEE)
		chance = 0.9;
	else if (enemy_range == RANGE_NEAR)
		chance = 0.4;
	else if (enemy_range == RANGE_MID)
		chance = 0.05;
	else
		chance = 0;

	if (random() < chance)
	{
		self.th_missile();
		SUB_AttackFinished(1 + random());
		if (random() < 0.3)
			self.lefty = !self.lefty;

		return TRUE;
	}

	return FALSE;
}
//=============================================================================

/*
===========
ShamCheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
float ShamCheckAttack()
{
	vec3_t spot1, spot2;
	entvars_t targ;

	if (enemy_range == RANGE_MELEE)
	{
		if (CanDamage(self.enemy, self))
		{
			self.attack_state = AS_MELEE;
			return TRUE;
		}
	}

	if (time < self.attack_finished)
		return FALSE;

	if (!enemy_vis)
		return FALSE;

	targ = self.enemy;

// see if any entities are in the way of the shot
	spot1 = self.origin + self.view_ofs;
	spot2 = targ.origin + targ.view_ofs;

	if (vlen(spot1 - spot2) > 600)
		return FALSE;

	traceline(spot1, spot2, FALSE, self);

	if (trace_inopen && trace_inwater)
		return FALSE; // sight line crossed contents

	if (trace_ent != targ)
	{
		return FALSE; // don't have a clear shot
	}

// missile attack
	if (enemy_range == RANGE_FAR)
		return FALSE;

	self.attack_state = AS_MISSILE;
	SUB_AttackFinished(2 + 2*random());
	return TRUE;
}

//============================================================================

/*
===========
OgreCheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
float OgreCheckAttack()
{
	vec3_t spot1, spot2;
	entvars_t targ;
	float chance;

	if (enemy_range == RANGE_MELEE)
	{
		if (CanDamage(self.enemy, self))
		{
			self.attack_state = AS_MELEE;
			return TRUE;
		}
	}

	if (time < self.attack_finished)
		return FALSE;

	if (!enemy_vis)
		return FALSE;

	targ = self.enemy;

// see if any entities are in the way of the shot
	spot1 = self.origin + self.view_ofs;
	spot2 = targ.origin + targ.view_ofs;

	traceline(spot1, spot2, FALSE, self);

	if (trace_inopen && trace_inwater)
		return FALSE; // sight line crossed contents

	if (trace_ent != targ)
	{
		return FALSE; // don't have a clear shot
	}

// missile attack
	if (time < self.attack_finished)
		return FALSE;

	if (enemy_range == RANGE_FAR)
		return FALSE;

	else if (enemy_range == RANGE_NEAR)
		chance = 0.10;
	else if (enemy_range == RANGE_MID)
		chance = 0.05;
	else
		chance = 0;

	self.attack_state = AS_MISSILE;
	SUB_AttackFinished(1 + 2*random());
	return TRUE;
}

