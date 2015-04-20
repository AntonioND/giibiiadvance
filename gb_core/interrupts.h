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

#ifndef __GB_INTERRUPTS__
#define __GB_INTERRUPTS__

//----------------------------------------------------------------

#include "gameboy.h"

//----------------------------------------------------------------

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

//----------------------------------------------------------------

void GB_HandleRTC(void);

//----------------------------------------------------------------

void GB_InterruptsInit(void);
void GB_InterruptsEnd(void);

//----------------------------------------------------------------

int GB_InterruptsExecute(void);

//----------------------------------------------------------------

inline void GB_InterruptsSetFlag(int flag);

//----------------------------------------------------------------

void GB_InterruptsWriteIE(int value); // reference_clocks not needed
void GB_InterruptsWriteIF(int reference_clocks, int value);

//----------------------------------------------------------------

void GB_TimersWriteDIV(int reference_clocks, int value);

void GB_TimersWriteTIMA(int reference_clocks, int value);
void GB_TimersWriteTMA(int reference_clocks, int value);
void GB_TimersWriteTAC(int reference_clocks, int value);

//----------------------------------------------------------------

inline void GB_TimersClockCounterReset(void);
void GB_TimersUpdateClocksCounterReference(int reference_clocks);
int GB_TimersGetClocksToNextEvent(void);

//----------------------------------------------------------------

void GB_CheckJoypadInterrupt(void); // Important: Must be called at least once per frame!

//----------------------------------------------------------------

#endif //__GB_INTERRUPTS__

