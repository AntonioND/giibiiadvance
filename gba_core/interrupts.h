/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __GBA_INTERRUPTS__
#define __GBA_INTERRUPTS__

#include "gba.h"

#define SCR_DRAW         (0)
#define SCR_HBL          (1)
#define SCR_VBL          (2)
extern u32 screenmode;

#define HDRAW_CLOCKS (960)
#define HBL_CLOCKS   (272)
//#define HLINE_CLOCKS (1232)
//#define VBL_CLOCKS   (83776) //68*HLINE_CLOCKS

inline void GBA_CallInterrupt(u32 flag);

inline void GBA_InterruptLCD(u32 flag);

inline int GBA_ScreenJustChangedMode(void);

s32 GBA_UpdateScreenTimings(s32 clocks);

inline int GBA_InterruptCheck(void);
inline void GBA_CheckKeypadInterrupt(void);

void GBA_InterruptInit(void);

#endif //__GBA_INTERRUPTS__


