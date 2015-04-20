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

#ifndef __GBA_CPU__
#define __GBA_CPU__

#include "gba.h"

//-----------------------------------------------------------------------------------

extern _cpu_t CPU;

void GBA_CPUInit(void);

void GBA_CPUChangeMode(u32 value);

inline s32 GBA_ExecuteARM(s32 clocks); // in arm.c
inline s32 GBA_ExecuteTHUMB(s32 clocks); // in thumb.c

inline s32 GBA_Execute(s32 clocks); // returns total clocks not executed
inline void GBA_ExecutionBreak(void);

inline void GBA_CPUSetHalted(s32 value);
inline s32 GBA_CPUGetHalted(void); // 0 = no, 1 = halt, 2 = stop
inline void GBA_CPUClearHalted(void);

//-----------------------------------------------------------------------------------

#endif //__GBA_CPU__
