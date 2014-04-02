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

#ifndef __GBA_DISASSEMBLER__
#define __GBA_DISASSEMBLER__

#include "gba.h"

void GBA_DebugAddBreakpoint(u32 addr);
void GBA_DebugClearBreakpoint(u32 addr);
int GBA_DebugIsBreakpoint(u32 addr); // used in debugger
int GBA_DebugCPUIsBreakpoint(u32 addr); // used in CPU loop
void GBA_DebugClearBreakpointAll(void);

void GBA_DisassembleARM(u32 opcode, u32 address, char * dest, int dest_size);

void GBA_DisassembleTHUMB(u16 opcode, u32 address, char * dest, int dest_size);

#endif //__GBA_DISASSEMBLER__

