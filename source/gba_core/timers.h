// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_TIMERS__
#define GBA_TIMERS__

#include "gba.h"

void GBA_TimerInitAll(void);

void GBA_TimerSetStart0(u16 val);
void GBA_TimerSetStart1(u16 val);
void GBA_TimerSetStart2(u16 val);
void GBA_TimerSetStart3(u16 val);

// For the debugger
u16 gba_timergetstart0(void);
u16 gba_timergetstart1(void);
u16 gba_timergetstart2(void);
u16 gba_timergetstart3(void);

void GBA_TimerSetup0(void);
void GBA_TimerSetup1(void);
void GBA_TimerSetup2(void);
void GBA_TimerSetup3(void);

s32 GBA_TimersUpdate(s32 clocks);

#endif // GBA_TIMERS__
