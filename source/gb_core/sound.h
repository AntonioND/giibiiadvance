// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_SOUND__
#define GB_SOUND__

#include "gameboy.h"

void GB_SoundInit(void);
int GB_SoundHardwareIsOn(void);
void GB_ToggleSound(void);
void GB_SoundUpdate(u32 clocks);
void GB_SoundRegWrite(u32 address, u32 value);
void GB_SoundResetBufferPointers(void);
void GB_SoundEnd(void);
size_t GB_SoundGetSamplesFrame(void *buffer, size_t buffer_size);
void GB_SoundResetBufferPointers(void);

void GB_SoundClockCounterReset(void);
void GB_SoundUpdateClocksCounterReference(int reference_clocks);
int GB_SoundGetClocksToNextEvent(void);

void GB_SoundGetConfig(int *vol, int *chn_flags);
void GB_SoundSetConfig(int vol, int chn_flags);

#endif // GB_SOUND__
