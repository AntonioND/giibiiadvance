// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_SOUND__
#define GBA_SOUND__

#include "../general_utils.h"

void GBA_SoundInit(void);
int GBA_SoundHardwareIsOn(void);
void GB_ToggleSound(void);
u32 GBA_SoundUpdate(u32 clocks);

void GBA_SoundRegWrite16(u32 address, u16 value);
u16 GBA_SoundRegRead16(u32 address);

void GBA_SoundSaveToWAV(void);
size_t GBA_SoundGetSamplesFrame(void *buffer, size_t buffer_size);
void GBA_SoundResetBufferPointers(void);
void GBA_SoundEnd(void);
void GBA_SoundTimerCheck(u32 number);

void GBA_SoundGetConfig(int *vol, int *chn_flags);
void GBA_SoundSetConfig(int vol, int chn_flags);

int gba_debug_get_psg_vol(int chan);

#endif // GBA_SOUND__
