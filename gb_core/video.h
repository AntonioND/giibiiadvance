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

#ifndef __VIDEO__
#define __VIDEO__

#include "gameboy.h"

inline void GB_FrameskipUpdate(void);
inline void GB_Frameskip(int _frames_to_skip);
inline void GB_EnableBlur(bool enable);
inline void GB_EnableRealColors(bool enable);
inline int GB_HaveToFrameskip(void);

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

#endif //__VIDEO__

