// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __GB_DEBUG__
#define __GB_DEBUG__

//---------------------------------------------------------------------------------

void GB_DebugAddBreakpoint(u32 addr);
void GB_DebugClearBreakpoint(u32 addr);
int GB_DebugIsBreakpoint(u32 addr); // used in debugger
int GB_DebugCPUIsBreakpoint(u32 addr); // used in CPU loop
void GB_DebugClearBreakpointAll(void);

//---------------------------------------------------------------------------------

int gb_debug_get_address_increment(u32 address);
int gb_debug_get_address_is_code(u32 address);
char * GB_Dissasemble(u16 addr, int * step);

//---------------------------------------------------------------------------------

#endif //__GB_DEBUG__

