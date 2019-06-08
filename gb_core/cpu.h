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

#ifndef __GB_CPU__
#define __GB_CPU__

//----------------------------------------------------------------

void GB_CPUInit(void);
void GB_CPUEnd(void);

//----------------------------------------------------------------

void GB_CPUClockCounterReset(void);
int GB_CPUClockCounterGet(void);
void GB_CPUClockCounterAdd(int value);

void GB_UpdateCounterToClocks(int reference_clocks);

//----------------------------------------------------------------

//This will make the execution to exit the CPU loop and update the other systems of the GB
//Call when writing to a register that can generate an event!!!
void GB_CPUBreakLoop(void);

//----------------------------------------------------------------

int GB_RunFor(s32 clocks); // 1 frame = 70224 clocks
//returns 1 if breakpoint executed

void GB_RunForInstruction(void);

//----------------------------------------------------------------

#endif //__GB_CPU__

