// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_PPU__
#define GB_PPU__

void GB_PPUInit(void);
void GB_PPUEnd(void);

void GB_PPUClockCounterReset(void);
void GB_PPUUpdateClocksCounterReference(int reference_clocks);
int GB_PPUGetClocksToNextEvent(void);

void GB_PPUCheckStatSignal(void);
void GB_PPUCheckLYC(void);

#endif // GB_PPU__
