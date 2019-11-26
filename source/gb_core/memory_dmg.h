// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_MEMORY_DMG__
#define GB_MEMORY_DMG__

#include "gameboy.h"

u32 GB_MemRead8_DMG_BootEnabled(u32 address);
u32 GB_MemRead8_DMG_BootDisabled(u32 address);

void GB_MemWrite8_DMG(u32 address, u32 value);

u32 GB_MemReadReg8_DMG(u32 address);

void GB_MemWriteReg8_DMG(u32 address, u32 value);

#endif // GB_MEMORY_DMG__
