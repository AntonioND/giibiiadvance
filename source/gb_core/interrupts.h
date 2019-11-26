// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_INTERRUPTS__
#define GB_INTERRUPTS__

#include "gameboy.h"

// STAT_REG
#define IENABLE_LY_COMPARE (1 << 6)
#define IENABLE_OAM        (1 << 5)
#define IENABLE_VBL        (1 << 4)
#define IENABLE_HBL        (1 << 3)
#define I_LY_EQUALS_LYC    (1 << 2)
// 0-1 = screen mode

// (IE/IF)_REG
#define I_VBLANK (1 << 0)
#define I_STAT   (1 << 1)
#define I_TIMER  (1 << 2)
#define I_SERIAL (1 << 3)
#define I_JOYPAD (1 << 4)

void GB_HandleRTC(void);

void GB_InterruptsInit(void);
void GB_InterruptsEnd(void);

int GB_InterruptsExecute(void);

void GB_InterruptsSetFlag(int flag);

void GB_InterruptsWriteIE(int value); // reference_clocks not needed
void GB_InterruptsWriteIF(int reference_clocks, int value);

void GB_TimersWriteDIV(int reference_clocks, int value);

void GB_TimersWriteTIMA(int reference_clocks, int value);
void GB_TimersWriteTMA(int reference_clocks, int value);
void GB_TimersWriteTAC(int reference_clocks, int value);

void GB_TimersClockCounterReset(void);
void GB_TimersUpdateClocksCounterReference(int reference_clocks);
int GB_TimersGetClocksToNextEvent(void);

// Important: Must be called at least once per frame
void GB_CheckJoypadInterrupt(void);

#endif // GB_INTERRUPTS__
