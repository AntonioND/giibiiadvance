// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_INTERRUPTS__
#define GBA_INTERRUPTS__

#include "gba.h"

#define SCR_DRAW         (0)
#define SCR_HBL          (1)
#define SCR_VBL          (2)
extern u32 screenmode;

#define HDRAW_CLOCKS (960)
#define HBL_CLOCKS   (272)
//#define HLINE_CLOCKS (1232)
//#define VBL_CLOCKS   (83776) // 68 * HLINE_CLOCKS

void GBA_CallInterrupt(u32 flag);

void GBA_InterruptLCD(u32 flag);

int GBA_ScreenJustChangedMode(void);

s32 GBA_UpdateScreenTimings(s32 clocks);

int GBA_InterruptCheck(void);
void GBA_CheckKeypadInterrupt(void);

void GBA_InterruptInit(void);

#endif // GBA_INTERRUPTS__
