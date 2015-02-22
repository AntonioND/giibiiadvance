/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

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

#ifndef __GB_VIDEO__
#define __GB_VIDEO__

#include "gameboy.h"

//----------------------------------------------------

inline void GB_SkipFrame(int skip);
inline int GB_HasToSkipFrame(void);

inline void GB_EnableBlur(int enable);
inline void GB_EnableRealColors(int enable);

void GB_ConfigGetPalette(u8 * red, u8 * green, u8 * blue);
void GB_ConfigSetPalette(u8 red, u8 green, u8 blue);
void GB_ConfigLoadPalette(void);

//----------------------------------------------------

#define GB_RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
void GB_SetPalette(u32 red, u32 green, u32 blue);
inline u32 GB_GameBoyGetGray(u32 number);

u32 gbc_getbgpalcolor(int pal, int color);
u32 gbc_getsprpalcolor(int pal, int color);

void GB_ScreenDrawScanline(s32 y);
void GBC_ScreenDrawScanline(s32 y);
void GBC_GB_ScreenDrawScanline(s32 y); //gbc when switched to gb mode.

void SGB_ScreenDrawBorder(void);
void SGB_ScreenDrawScanline(s32 y);

//----------------------------------------------------

void GB_Screen_WriteBuffer_24RGB(char * buffer); // write to buffer in 24 bit format
void GB_Screenshot(void);

//----------------------------------------------------

#endif //__GB_VIDEO__

