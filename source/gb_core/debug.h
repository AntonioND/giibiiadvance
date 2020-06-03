// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_DEBUG__
#define GB_DEBUG__

#include "gameboy.h"

void GB_DebugAddBreakpoint(u32 addr);
void GB_DebugClearBreakpoint(u32 addr);
int GB_DebugIsBreakpoint(u32 addr);    // Used in debugger
int GB_DebugCPUIsBreakpoint(u32 addr); // Used in CPU loop
void GB_DebugClearBreakpointAll(void);

int gb_debug_get_address_increment(u32 address);
int gb_debug_get_address_is_code(u32 address);
char *GB_Dissasemble(u16 addr, int *step);

#endif // GB_DEBUG__
