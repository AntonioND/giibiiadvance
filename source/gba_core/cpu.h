// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_CPU__
#define GBA_CPU__

#include "gba.h"

extern _cpu_t CPU;

void GBA_CPUInit(void);

void GBA_CPUChangeMode(u32 value);

s32 GBA_ExecuteARM(s32 clocks);   // In arm.c
s32 GBA_ExecuteTHUMB(s32 clocks); // In thumb.c

// Returns total clocks not executed
s32 GBA_Execute(s32 clocks);
void GBA_ExecutionBreak(void);

void GBA_CPUSetHalted(s32 value);
s32 GBA_CPUGetHalted(void); // 0 = no, 1 = halt, 2 = stop
void GBA_CPUClearHalted(void);

#endif // GBA_CPU__
