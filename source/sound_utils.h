// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef SOUND_UTILS__
#define SOUND_UTILS__

typedef void (Sound_CallbackPointer)(void *, long);

void Sound_Init(void);

void Sound_SetCallback(Sound_CallbackPointer *fn);

void Sound_Enable(void);
void Sound_Disable(void);

void Sound_SetVolume(int vol);

void Sound_SetEnabled(int enable);
int Sound_GetEnabled(void);

void Sound_SetEnabledChannels(int flags);

#endif // SOUND_UTILS__
