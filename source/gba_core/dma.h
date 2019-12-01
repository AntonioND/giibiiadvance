// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_DMA__
#define GBA_DMA__

#include "gba.h"

void GBA_DMA0Setup(void);
void GBA_DMA1Setup(void);
void GBA_DMA2Setup(void);
void GBA_DMA3Setup(void);

s32 GBA_DMAUpdate(s32 clocks);

int GBA_DMAisWorking(void);

s32 GBA_DMAGetExtraClocksElapsed(void);

int gba_dma3numchunks(void); // For EEPROM
int gba_dma3enabled(void);

void GBA_DMASoundRequestData(int A, int B);

#endif // GBA_DMA__
