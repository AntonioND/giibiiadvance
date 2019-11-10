// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __GBA_SAVE__
#define __GBA_SAVE__

#include "gba.h"

int GBA_SaveIsEEPROM(void);

void GBA_DetectSaveType(u8 * romptr, int size);
void GBA_ResetSaveBuffer(void);
void GBA_SaveSetFilename(char * rom_path);

const char * GBA_GetSaveTypeString(void);

u8 GBA_SaveRead8(u32 address);
u16 GBA_SaveRead16(u32 address);
void GBA_SaveWrite8(u32 address, u8 data);
void GBA_SaveWrite16(u32 address, u16 data);

void GBA_SaveWriteFile(void);
void GBA_SaveReadFile(void);

#endif //__GBA_SAVE__


