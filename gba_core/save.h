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

#ifndef __GBA_SAVE__
#define __GBA_SAVE__

#include "gba.h"

inline int GBA_SaveIsEEPROM(void);

void GBA_DetectSaveType(u8 * romptr, int size);
void GBA_ResetSaveBuffer(void);
void GBA_SaveSetFilename(char * rom_path);

inline const char * GBA_GetSaveTypeString(void);

u8 GBA_SaveRead8(u32 address);
u16 GBA_SaveRead16(u32 address);
void GBA_SaveWrite8(u32 address, u8 data);
void GBA_SaveWrite16(u32 address, u16 data);

void GBA_SaveWriteFile(void);
void GBA_SaveReadFile(void);

#endif //__GBA_SAVE__


