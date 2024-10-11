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
#pragma once
#include "input_keys.hpp"

#define MAXMENUITEMS	64

#define MTYPE_SLIDER		0
#define MTYPE_ACTION		1
#define MTYPE_SPINCONTROL	2
#define MTYPE_SEPARATOR  	3
#define MTYPE_FIELD			4

#define QMF_LEFT_JUSTIFY	0x00000001
#define QMF_GRAYED			0x00000002
#define QMF_NUMBERSONLY		0x00000004

typedef struct _tag_menuframework
{
	int32_t x, y;
	int32_t cursor;

	int32_t nitems;
	int32_t nslots;
	void* items[64];

	const char* statusbar;

	void (*cursordraw)(struct _tag_menuframework* m);

} menuframework_t;

typedef struct menucommon_s
{
	int32_t type;
	const char* name;
	int32_t x, y;
	menuframework_t* parent;
	int32_t cursor_offset;
	int32_t localdata[4];
	uint32_t flags;

	const char* statusbar;

	void (*callback)(void* self);
	void (*statusbarfunc)(void* self);
	void (*ownerdraw)(void* self);
	void (*cursordraw)(void* self);
} menucommon_t;

typedef struct menufield_s
{
	menucommon_t generic;

	char		buffer[80];
	int32_t 		cursor;
	int32_t 		length;
	int32_t 		visible_length;
	int32_t 		visible_offset;
} menufield_t;

typedef struct menuslider_s
{
	menucommon_t generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_t;

typedef struct menulist_s
{
	menucommon_t generic;

	int32_t curvalue;

	const char** itemnames;
} menulist_t;

typedef struct menuaction_s
{
	menucommon_t generic;
} menuaction_t;

typedef struct menuseparator_s
{
	menucommon_t generic;
} menuseparator_t;

bool Field_Key(menufield_t* field, int32_t key);

void Menu_AddItem(menuframework_t* menu, void* item);
void Menu_AdjustCursor(menuframework_t* menu, int32_t dir);
void Menu_Center(menuframework_t* menu);
void Menu_Draw(menuframework_t* menu);
void* Menu_ItemAtCursor(menuframework_t* m);
bool Menu_SelectItem(menuframework_t* s);
void Menu_SetStatusBar(menuframework_t* s, const char* string);
void Menu_SlideItem(menuframework_t* s, int32_t dir);
int32_t 	Menu_TallySlots(menuframework_t* menu);

