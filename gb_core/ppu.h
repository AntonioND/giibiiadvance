// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __GB_PPU__
#define __GB_PPU__

//----------------------------------------------------------------

#include "gameboy.h"

//----------------------------------------------------------------

void GB_PPUInit(void);
void GB_PPUEnd(void);

//----------------------------------------------------------------

void GB_PPUClockCounterReset(void);
void GB_PPUUpdateClocksCounterReference(int reference_clocks);
int GB_PPUGetClocksToNextEvent(void);

//----------------------------------------------------------------

void GB_PPUCheckStatSignal(void);
void GB_PPUCheckLYC(void);

//----------------------------------------------------------------

#endif // __GB_PPU__
