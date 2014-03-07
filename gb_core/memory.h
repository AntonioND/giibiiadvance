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

#ifndef __MEMORY__
#define __MEMORY__

void GB_MemInit(void);

inline void GB_MemWrite16(u32 address, u32 value);
void GB_MemWrite8(u32 address, u32 value);
void GB_MemWriteReg8(u32 address, u32 value);

inline u32 GB_MemRead16(u32 address);
u32 GB_MemRead8(u32 address);
u32 GB_MemReadReg8(u32 address);

inline u32 GBC_HDMAcopy(void);

void GB_CameraEnd(void);
int GB_CameraInit(int createwindow);

#endif //__MEMORY__

