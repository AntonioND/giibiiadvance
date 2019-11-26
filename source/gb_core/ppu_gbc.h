// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_PPU_GBC__
#define GB_PPU_GBC__

#include "gameboy.h"

void GB_PPUWriteLYC_GBC(int reference_clocks, int value);
void GB_PPUWriteLCDC_GBC(int reference_clocks, int value);
void GB_PPUWriteSTAT_GBC(int reference_clocks, int value);

void GB_PPUUpdateClocks_GBC(int increment_clocks);

int GB_PPUGetClocksToNextEvent_GBC(void);

#endif // GB_PPU_GBC__
