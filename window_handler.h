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

#ifndef __WINDOW_HANDLER__
#define __WINDOW_HANDLER__

typedef int (*WH_CallbackFn)(SDL_Event*);

void WH_Init(void);

// if scale = 0 texture will be scaled to window size. If not, centered and scaled to this factor
int WH_Create(int width, int height, int texw, int texh, int scale); // returns -1 if error

// if scale = 0 texture will be scaled to window size. If not, centered and scaled to this factor
void WH_SetSize(int index, int width, int height, int texw, int texh, int scale);

int WH_SetEventCallback(int index, WH_CallbackFn fn); // callback won't ever receive SDL_QUIT event!
int WH_SetEventMainWindow(int index);

void WH_HandleEvents(void);

void WH_SetCaption(int index, const char * caption);

void WH_Render(int index, const char * buffer);

void WH_Close(int index);
void WH_CloseAllBut(int index);
void WH_CloseAllButMain(void);
void WH_CloseAll(void);

int WH_AreAllWindowsClosed(void);

void WH_Focus(int index);

int WH_GetWidth(int index);
int WH_GetHeight(int index);

int WH_HasMouseFocus(int index);
int WH_HasKeyboardFocus(int index);

int WH_IsShown(int index);

#endif // __WINDOW_HANDLER__
