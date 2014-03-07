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

#ifndef __INTERRUPTS__
#define __INTERRUPTS__

#include "gameboy.h"

//STAT_REG
#define IENABLE_LY_COMPARE (1<<6)
#define IENABLE_OAM        (1<<5)
#define IENABLE_VBL        (1<<4)
#define IENABLE_HBL        (1<<3)
#define I_LY_EQUALS_LYC    (1<<2)
//0-1 = screen mode

//(IE/IF)_REG
#define I_VBLANK (1<<0)
#define I_STAT   (1<<1)
#define I_TIMER  (1<<2)
#define I_SERIAL (1<<3)
#define I_JOYPAD (1<<4)

void GB_HandleTime(void);
void GB_CPUInterruptsInit(void);
void GB_CPUInterruptsEnd(void);
void GB_ChecStatSignal(void);
void GB_TimersUpdate(int clocks);
void GB_LCDUpdate(int clocks);
inline void GB_SetInterrupt(int flag);
inline void GB_CheckStatSignal(void);
inline void GB_CheckLYC(void);
inline void GB_CPUHandleClockEvents(void);
inline int GB_CPUHandleInterrupts(void);

#endif //__INTERRUPTS__

