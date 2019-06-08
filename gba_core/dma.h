/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __GBA_DMA__
#define __GBA_DMA__

#include "gba.h"

void GBA_DMA0Setup(void);
void GBA_DMA1Setup(void);
void GBA_DMA2Setup(void);
void GBA_DMA3Setup(void);

s32 GBA_DMAUpdate(s32 clocks);

int GBA_DMAisWorking(void);

s32 GBA_DMAGetExtraClocksElapsed(void);

int gba_dma3numchunks(void); //for EEPROM
int gba_dma3enabled(void);

void GBA_DMASoundRequestData(int A, int B);

#endif //__GBA_DMA__


