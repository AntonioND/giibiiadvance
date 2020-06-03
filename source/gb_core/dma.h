// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_DMA__
#define GB_DMA__

#include "gameboy.h"

void GB_DMAInit(void);
void GB_DMAEnd(void);

void GB_DMAWriteDMA(int reference_clocks, int value);

void GB_DMAWriteHDMA1(int value); // reference_clocks not needed
void GB_DMAWriteHDMA2(int value);
void GB_DMAWriteHDMA3(int value);
void GB_DMAWriteHDMA4(int value);
void GB_DMAWriteHDMA5(int value);

void GB_DMAClockCounterReset(void);
void GB_DMAUpdateClocksCounterReference(int reference_clocks);
int GB_DMAGetClocksToNextEvent(void);

int GB_DMAExecute(int clocks);

#endif // GB_DMA__
