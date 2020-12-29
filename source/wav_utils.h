// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2020 Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef WAV_UTILS_H__
#define WAV_UTILS_H__

#include <stddef.h>
#include <stdint.h>

void WAV_FileStart(const char *path, uint32_t sample_rate);
void WAV_FileEnd(void);

int WAV_FileIsOpen(void);

void WAV_FileStream(int16_t *buffer, size_t size);

#endif // WAV_UTILS_H__
