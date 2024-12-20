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
// conproc.c -- support for qhost
#include <cstdio>
#include <cstdint>
#include <process.h>
#include <windows.h>
#include "win32_conproc.hpp"

#define CCOM_WRITE_TEXT		0x2
// Param1 : Text

#define CCOM_GET_TEXT		0x3
// Param1 : Begin line
// Param2 : End line

#define CCOM_GET_SCR_LINES	0x4
// No params

#define CCOM_SET_SCR_LINES	0x5
// Param1 : Number of lines

HANDLE	heventDone;
HANDLE	hfileBuffer;
HANDLE	heventChildSend;
HANDLE	heventParentSend;
HANDLE	hStdout;
HANDLE	hStdin;

uint32_t _stdcall RequestProc (void *arg);
LPVOID GetMappedBuffer (HANDLE hfileBuffer);
void ReleaseMappedBuffer (LPVOID pBuffer);
BOOL GetScreenBufferLines (int32_t *piLines);
BOOL SetScreenBufferLines (int32_t iLines);
BOOL ReadText (LPTSTR pszText, int32_t iBeginLine, int32_t iEndLine);
BOOL WriteText (LPCTSTR szText);
int32_t CharToCode (char c);
BOOL SetConsoleCXCY(HANDLE hStdout, int32_t cx, int32_t cy);

int32_t ccom_argc;
char** ccom_argv;

/*
================
CCheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int32_t CCheckParm(const char* parm)
{
	int32_t             i;
	
	for (i=1 ; i<ccom_argc ; i++)
	{
		if (!ccom_argv[i])
			continue;
		if (!strcmp (parm,ccom_argv[i]))
			return i;
	}
		
	return 0;
}


void InitConProc (int32_t argc, char** argv)
{
	uint32_t	threadAddr;
	HANDLE		hFile = NULL;
	HANDLE		heventParent = NULL;
	HANDLE		heventChild = NULL;
	int32_t 	t;

	ccom_argc = argc;
	ccom_argv = argv;

// give QHOST a chance to hook into the console
	if ((t = CCheckParm ("-HFILE")) > 0)
	{
		if (t < argc)
			hFile = (HANDLE)atoi (ccom_argv[t+1]);
	}
		
	if ((t = CCheckParm ("-HPARENT")) > 0)
	{
		if (t < argc)
			heventParent = (HANDLE)atoi (ccom_argv[t+1]);
	}
		
	if ((t = CCheckParm ("-HCHILD")) > 0)
	{
		if (t < argc)
			heventChild = (HANDLE)atoi (ccom_argv[t+1]);
	}


// ignore if we don't have all the events.
	if (!hFile || !heventParent || !heventChild)
	{
		printf ("Qhost not present.\n");
		return;
	}

	printf ("Initializing for qhost.\n");

	hfileBuffer = hFile;
	heventParentSend = heventParent;
	heventChildSend = heventChild;

// so we'll know when to go away.
	heventDone = CreateEvent (NULL, FALSE, FALSE, NULL);

	if (!heventDone)
	{
		printf ("Couldn't create heventDone\n");
		return;
	}

	if (!_beginthreadex (NULL, 0, RequestProc, NULL, 0, &threadAddr))
	{
		CloseHandle (heventDone);
		printf ("Couldn't create QHOST thread\n");
		return;
	}

// save off the input/output handles.
	hStdout = GetStdHandle (STD_OUTPUT_HANDLE);
	hStdin = GetStdHandle (STD_INPUT_HANDLE);

// force 80 character width, at least 25 character height
	SetConsoleCXCY (hStdout, 80, 25);
}


void DeinitConProc ()
{
	//if (heventDone)
		//SetEvent (heventDone);
}


uint32_t _stdcall RequestProc (void *arg)
{
	int32_t 	*pBuffer;
	DWORD	dwRet;
	HANDLE	heventWait[2];
	int32_t 	iBeginLine, iEndLine;
	
	heventWait[0] = heventParentSend;
	heventWait[1] = heventDone;

	while (1)
	{
		dwRet = WaitForMultipleObjects (2, heventWait, FALSE, INFINITE);

	// heventDone fired, so we're exiting.
		if (dwRet == WAIT_OBJECT_0 + 1)	
			break;

		pBuffer = (int32_t *) GetMappedBuffer (hfileBuffer);
		
	// hfileBuffer is invalid.  Just leave.
		if (!pBuffer)
		{
			printf ("Invalid hfileBuffer\n");
			break;
		}

		switch (pBuffer[0])
		{
			case CCOM_WRITE_TEXT:
			// Param1 : Text
				pBuffer[0] = WriteText ((LPCTSTR) (pBuffer + 1));
				break;

			case CCOM_GET_TEXT:
			// Param1 : Begin line
			// Param2 : End line
				iBeginLine = pBuffer[1];
				iEndLine = pBuffer[2];
				pBuffer[0] = ReadText ((LPTSTR) (pBuffer + 1), iBeginLine, 
									   iEndLine);
				break;

			case CCOM_GET_SCR_LINES:
			// No params
				pBuffer[0] = GetScreenBufferLines (&pBuffer[1]);
				break;

			case CCOM_SET_SCR_LINES:
			// Param1 : Number of lines
				pBuffer[0] = SetScreenBufferLines (pBuffer[1]);
				break;
		}

		ReleaseMappedBuffer (pBuffer);
		SetEvent (heventChildSend);
	}

	_endthreadex (0);
	return 0;
}


LPVOID GetMappedBuffer (HANDLE hfileBuffer)
{
	LPVOID pBuffer;

	pBuffer = MapViewOfFile (hfileBuffer,
							FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	return pBuffer;
}


void ReleaseMappedBuffer (LPVOID pBuffer)
{
	UnmapViewOfFile (pBuffer);
}


BOOL GetScreenBufferLines (int32_t *piLines)
{
	CONSOLE_SCREEN_BUFFER_INFO	info;							  
	BOOL						bRet;

	bRet = GetConsoleScreenBufferInfo (hStdout, &info);
		
	if (bRet)
		*piLines = info.dwSize.Y;

	return bRet;
}


BOOL SetScreenBufferLines (int32_t iLines)
{

	return SetConsoleCXCY (hStdout, 80, iLines);
}


BOOL ReadText (LPTSTR pszText, int32_t iBeginLine, int32_t iEndLine)
{
	COORD	coord;
	DWORD	dwRead;
	BOOL	bRet;

	coord.X = 0;
	coord.Y = iBeginLine;

	bRet = ReadConsoleOutputCharacter(
		hStdout,
		pszText,
		80 * (iEndLine - iBeginLine + 1),
		coord,
		&dwRead);

	// Make sure it's null terminated.
	if (bRet)
		pszText[dwRead] = '\0';

	return bRet;
}


BOOL WriteText (LPCTSTR szText)
{
	DWORD			dwWritten;
	INPUT_RECORD	rec;
	char			upper, *sz;

	sz = (LPTSTR) szText;

	while (*sz)
	{
		// Replace \n with \r for older versions of Windows. (do we even need this anymore? restore if it causes problems)
		//if (*sz == '\n')
		//	*sz = '\r';

		upper = toupper(*sz);

		rec.EventType = KEY_EVENT;
		rec.Event.KeyEvent.bKeyDown = TRUE;
		rec.Event.KeyEvent.wRepeatCount = 1;
		rec.Event.KeyEvent.wVirtualKeyCode = upper;
		rec.Event.KeyEvent.wVirtualScanCode = CharToCode (*sz);
		rec.Event.KeyEvent.uChar.AsciiChar = *sz;
		rec.Event.KeyEvent.uChar.UnicodeChar = *sz;
		rec.Event.KeyEvent.dwControlKeyState = isupper(*sz) ? 0x80 : 0x0; 

		WriteConsoleInput(
			hStdin,
			&rec,
			1,
			&dwWritten);

		rec.Event.KeyEvent.bKeyDown = FALSE;

		WriteConsoleInput(
			hStdin,
			&rec,
			1,
			&dwWritten);

		sz++;
	}

	return TRUE;
}


int32_t CharToCode (char c)
{
	char upper;
		
	upper = toupper(c);

	switch (c)
	{
		case 13:
			return 28;

		default:
			break;
	}

	if (isalpha(c))
		return (30 + upper - 65); 

	if (isdigit(c))
		return (1 + upper - 47);

	return c;
}


BOOL SetConsoleCXCY(HANDLE hStdout, int32_t cx, int32_t cy)
{
	CONSOLE_SCREEN_BUFFER_INFO	info;
	COORD						coordMax;
 
	coordMax = GetLargestConsoleWindowSize(hStdout);

	if (cy > coordMax.Y)
		cy = coordMax.Y;

	if (cx > coordMax.X)
		cx = coordMax.X;
 
	if (!GetConsoleScreenBufferInfo(hStdout, &info))
		return FALSE;
 
// height
    info.srWindow.Left = 0;         
    info.srWindow.Right = info.dwSize.X - 1;                
    info.srWindow.Top = 0;
    info.srWindow.Bottom = cy - 1;          
 
	if (cy < info.dwSize.Y)
	{
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
 
		info.dwSize.Y = cy;
 
		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
    }
    else if (cy > info.dwSize.Y)
    {
		info.dwSize.Y = cy;
 
		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
 
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
    }
 
	if (!GetConsoleScreenBufferInfo(hStdout, &info))
		return FALSE;
 
// width
	info.srWindow.Left = 0;         
	info.srWindow.Right = cx - 1;
	info.srWindow.Top = 0;
	info.srWindow.Bottom = info.dwSize.Y - 1;               
 
	if (cx < info.dwSize.X)
	{
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
 
		info.dwSize.X = cx;
    
		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
	}
	else if (cx > info.dwSize.X)
	{
		info.dwSize.X = cx;
 
		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
 
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
	}
 
	return TRUE;
}
     
