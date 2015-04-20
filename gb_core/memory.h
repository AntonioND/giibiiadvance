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

#ifndef __GB_MEMORY__
#define __GB_MEMORY__

//----------------------------------------------------------------

void GB_MemInit(void);
void GB_MemEnd(void);

//----------------------------------------------------------------

void GB_MemUpdateReadWriteFunctionPointers(void);

//----------------------------------------------------------------

inline void GB_MemWrite16(u32 address, u32 value); // only used by debugger
inline void GB_MemWrite8(u32 address, u32 value);
inline void GB_MemWriteReg8(u32 address, u32 value);

//----------------------------------------------------------------

inline u32 GB_MemRead16(u32 address); // only used by debugger
inline u32 GB_MemRead8(u32 address);
inline u32 GB_MemReadReg8(u32 address);

//----------------------------------------------------------------

void GB_MemWriteDMA8(u32 address, u32 value); // This assumes that address is 0xFE00-0xFEA0
u32 GB_MemReadDMA8(u32 address);

//----------------------------------------------------------------

void GB_MemWriteHDMA8(u32 address, u32 value);
u32 GB_MemReadHDMA8(u32 address);

//----------------------------------------------------------------

void GB_MemoryWriteSVBK(int value); // reference_clocks not needed
void GB_MemoryWriteVBK(int value); // reference_clocks not needed

//----------------------------------------------------------------

#endif //__GB_MEMORY__

