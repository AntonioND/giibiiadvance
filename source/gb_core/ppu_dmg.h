// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_PPU_DMG__
#define GB_PPU_DMG__

void GB_PPUWriteLYC_DMG(int reference_clocks, int value);
void GB_PPUWriteLCDC_DMG(int reference_clocks, int value);
void GB_PPUWriteSTAT_DMG(int reference_clocks, int value);

void GB_PPUUpdateClocks_DMG(int increment_clocks);

int GB_PPUGetClocksToNextEvent_DMG(void);

#endif // GB_PPU__
