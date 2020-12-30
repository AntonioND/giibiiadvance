// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef SOUND_UTILS__
#define SOUND_UTILS__

#define GB_SAMPLERATE       (32 * 1024)
#define GBA_SAMPLERATE      (32 * 1024)
#define SDL_SAMPLERATE      (44100)

typedef void(Sound_CallbackPointer)(void *, long);

void Sound_Init(void);

void Sound_Enable(void);
void Sound_Disable(void);

int Sound_IsBufferOverThreshold(void);
void Sound_SendSamples(int16_t *buffer, int len);

void Sound_SetVolume(int vol);

void Sound_SetEnabled(int enable);
int Sound_GetEnabled(void);

void Sound_SetEnabledChannels(int flags);

#endif // SOUND_UTILS__
