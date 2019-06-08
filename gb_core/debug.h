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

