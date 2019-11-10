// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __GB_SOUND__
#define __GB_SOUND__

#include "gameboy.h"

void GB_SoundInit(void);

int GB_SoundHardwareIsOn(void);

void GB_ToggleSound(void);

void GB_SoundUpdate(u32 clocks);

void GB_SoundRegWrite(u32 address, u32 value);

void GB_SoundResetBufferPointers(void);

void GB_SoundEnd(void);

void GB_SoundCallback(void * buffer, long len);

//----------------------------------------------------------------

void GB_SoundClockCounterReset(void);

void GB_SoundUpdateClocksCounterReference(int reference_clocks);

int GB_SoundGetClocksToNextEvent(void);

//----------------------------------------------------------------

void GB_SoundGetConfig(int * vol, int * chn_flags);

void GB_SoundSetConfig(int vol, int chn_flags);

//----------------------------------------------------------------

#endif //__GB_SOUND__

