// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

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

