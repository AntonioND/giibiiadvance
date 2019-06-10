// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __GBA_SOUND__
#define __GBA_SOUND__

#include "../general_utils.h"

void GBA_SoundInit(void);

int GBA_SoundHardwareIsOn(void);

void GB_ToggleSound(void);

u32 GBA_SoundUpdate(u32 clocks);

void GBA_SoundRegWrite16(u32 address, u16 value);

void GBA_SoundResetBufferPointers(void);

void GBA_SoundEnd(void);

void GBA_SoundCallback(void * buffer, long len);

void GBA_SoundTimerCheck(int number);

//---------------------------------

void GBA_SoundGetConfig(int * vol, int * chn_flags);

void GBA_SoundSetConfig(int vol, int chn_flags);

//---------------------------------

int gba_debug_get_psg_vol(int chan);

#endif //__GBA_SOUND__


