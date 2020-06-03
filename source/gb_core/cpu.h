// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_CPU__
#define GB_CPU__

#include "gameboy.h"

void GB_CPUInit(void);
void GB_CPUEnd(void);

//----------------------------------------------------------------

void GB_CPUClockCounterReset(void);
int GB_CPUClockCounterGet(void);
void GB_CPUClockCounterAdd(int value);

void GB_UpdateCounterToClocks(int reference_clocks);

//----------------------------------------------------------------

// This will make the execution to exit the CPU loop and update the other
// systems of the GB. Call when writing to a register that can generate an
// event!!!
void GB_CPUBreakLoop(void);

//----------------------------------------------------------------

// Run GB emulation for the specified number of clocks (1 frame = 70224 clocks).
// It returns 1 if a breakpoint is found.
int GB_RunFor(s32 clocks);

void GB_RunForInstruction(void);

#endif // GB_CPU__
