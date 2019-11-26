// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_MEMORY_GBC__
#define GB_MEMORY_GBC__

#include "gameboy.h"

// Note: The functions in this file are the ones used for GBC in both GBC and
// DMG modes.

u32 GB_MemRead8_GBC_BootEnabled(u32 address);
u32 GB_MemRead8_GBC_BootDisabled(u32 address);

void GB_MemWrite8_GBC(u32 address, u32 value);

u32 GB_MemReadReg8_GBC(u32 address);

void GB_MemWriteReg8_GBC(u32 address, u32 value);

#endif // GB_MEMORY_GBC__
