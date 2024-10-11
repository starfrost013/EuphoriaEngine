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
#pragma once

// input_keys.h: Defines all of the keys in the game
/*
From GLFW:
These key codes are inspired by the USB HID Usage Tables v1.12 (p. 53-60), 
but re-arranged to map to 7-bit ASCII for printable keys (function keys are put in the 256+ range).

They have been renamed from KEY_ to Key_ to fit with the rest of the code
*/

#define K_SPACE   32
#define K_APOSTROPHE   39 /* ' */
#define K_COMMA   44 /* , */
#define K_MINUS   45 /* - */
#define K_PERIOD   46 /* . */
#define K_SLASH   47 /* / */
#define K_0   48
#define K_1   49
#define K_2   50
#define K_3   51
#define K_4   52
#define K_5   53
#define K_6   54
#define K_7   55
#define K_8   56
#define K_9   57
#define K_SEMICOLON   59 /* ; */
#define K_EQUAL   61 /* = */
#define K_A   65
#define K_B   66
#define K_C   67
#define K_D   68
#define K_E   69
#define K_F   70
#define K_G   71
#define K_H   72
#define K_I   73
#define K_J   74
#define K_K   75
#define K_L   76
#define K_M   77
#define K_N   78
#define K_O   79
#define K_P   80
#define K_Q   81
#define K_R   82
#define K_S   83
#define K_T   84
#define K_U   85
#define K_V   86
#define K_W   87
#define K_X   88
#define K_Y   89
#define K_Z   90

#define K_LEFT_BRACKET   91 /* [ */
#define K_BACKSLASH   92 /* \ */
#define K_RIGHT_BRACKET   93 /* ] */
#define K_GRAVE_ACCENT   96 /* ` */

// Kludged virtual keys
#define	K_ALT			132
#define	K_CTRL			133
#define	K_SHIFT			134

#define K_WORLD_1   161 /* non-US #1 */
#define K_WORLD_2   162 /* non-US #2 */

//
// mouse buttons generate virtual keys
//
#define	K_MOUSE1		200
#define	K_MOUSE2		201
#define	K_MOUSE3		202
#define	K_MOUSE4		241
#define	K_MOUSE5		242
// GLFW
#define K_MOUSE6		243
#define K_MOUSE7		244
#define K_MOUSE8		245

//
// joystick buttons
//
#define	K_JOY1			203
#define	K_JOY2			204
#define	K_JOY3			205
#define	K_JOY4			206

//
// aux keys are for multi-buttoned joysticks to generate so they can use
// the normal binding process
//
#define	K_AUX1			207
#define	K_AUX2			208
#define	K_AUX3			209
#define	K_AUX4			210
#define	K_AUX5			211
#define	K_AUX6			212
#define	K_AUX7			213
#define	K_AUX8			214
#define	K_AUX9			215
#define	K_AUX10			216
#define	K_AUX11			217
#define	K_AUX12			218
#define	K_AUX13			219
#define	K_AUX14			220
#define	K_AUX15			221
#define	K_AUX16			222
#define	K_AUX17			223
#define	K_AUX18			224
#define	K_AUX19			225
#define	K_AUX20			226
#define	K_AUX21			227
#define	K_AUX22			228
#define	K_AUX23			229
#define	K_AUX24			230
#define	K_AUX25			231
#define	K_AUX26			232
#define	K_AUX27			233
#define	K_AUX28			234
#define	K_AUX29			235
#define	K_AUX30			236
#define	K_AUX31			237
#define	K_AUX32			238

#define K_MWHEELDOWN	239
#define K_MWHEELUP		240

#define K_ESCAPE   256
#define K_ENTER   257
#define K_TAB   258
#define K_BACKSPACE   259
#define K_INSERT   260
#define K_DELETE   261
#define K_RIGHTARROW  262
#define K_LEFTARROW   263
#define K_DOWNARROW   264
#define K_UPARROW   265
#define K_PAGE_UP   266
#define K_PAGE_DOWN   267
#define K_HOME   268
#define K_END   269
#define K_CAPS_LOCK   280
#define K_SCROLL_LOCK   281
#define K_NUM_LOCK   282
#define K_PRINT_SCREEN   283
#define K_PAUSE   284
#define K_F1   290
#define K_F2   291
#define K_F3   292
#define K_F4   293
#define K_F5   294
#define K_F6   295
#define K_F7   296
#define K_F8   297
#define K_F9   298
#define K_F10   299
#define K_F11   300
#define K_F12   301
#define K_F13   302
#define K_F14   303
#define K_F15   304
#define K_F16   305
#define K_F17   306
#define K_F18   307
#define K_F19   308
#define K_F20   309
#define K_F21   310
#define K_F22   311
#define K_F23   312
#define K_F24   313
#define K_F25   314

#define K_KP_0   320
#define K_KP_1   321
#define K_KP_2   322
#define K_KP_3   323
#define K_KP_4   324
#define K_KP_5   325
#define K_KP_6   326
#define K_KP_7   327
#define K_KP_8   328
#define K_KP_9   329
#define K_KP_DECIMAL   330
#define K_KP_DIVIDE   331
#define K_KP_MULTIPLY   332
#define K_KP_SUBTRACT   333
#define K_KP_ADD   334
#define K_KP_ENTER   335
#define K_KP_EQUAL   336
#define K_LEFT_SHIFT   340
#define K_LEFT_CONTROL   341
#define K_LEFT_ALT   342
#define K_LEFT_SUPER   343
#define K_RIGHT_SHIFT   344
#define K_RIGHT_CONTROL   345
#define K_RIGHT_ALT   346
#define K_RIGHT_SUPER   347
#define K_MENU   348

#define NUM_KEYS   K_MENU