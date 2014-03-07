/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2014 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __GUI_MAIN__
#define __GUI_MAIN__

#include <windows.h>

#include "build_options.h"

extern char biospath[MAXPATHLEN];

extern HINSTANCE hInstance;
extern HWND hWndMain; // Main window
extern int Keys_JustPressed[256];
extern int Keys_Down[256];
extern int GLWindow_Active;

inline void GLWindow_SwapBuffers(void);

int GLWindow_HandleEvents(void);

void GLWindow_SetMute(int value);

char * GLWindow_OpenRom(HWND hwnd);
int GLWindow_LoadRom(char * path);
void GLWindow_UnloadRom(int save);

void GLWindow_SetCaption(const char * text);
void GLWindow_Kill(void);

//+----------------+
//| GBA -> 240x160 |
//| GBC -> 160x144 |
//| SGB -> 256x224 |
//+----------------+
#define SCR_GBA 0
#define SCR_GB  1
#define SCR_SGB 2
void GLWindow_ChangeScreen(int type);
void GLWindow_SetZoom(int zoom);

int GLWindow_Create(void);

void GLWindow_Resize(int x, int y, int usezoom);


#endif //__GUI_MAIN__

