// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_VIDEO__
#define GB_VIDEO__

#include "gameboy.h"

void GB_SkipFrame(int skip);
int GB_HasToSkipFrame(void);

void GB_EnableBlur(int enable);
void GB_EnableRealColors(int enable);

void GB_ConfigGetPalette(u8 *red, u8 *green, u8 *blue);
void GB_ConfigSetPalette(u8 red, u8 green, u8 blue);
void GB_ConfigLoadPalette(void);

//----------------------------------------------------

#define GB_RGB(r, g, b) ((r) | ((g) << 5) | ((b) << 10))
void GB_SetPalette(u32 red, u32 green, u32 blue);
u32 GB_GameBoyGetGray(u32 number);

u32 gbc_getbgpalcolor(int pal, int color);
u32 gbc_getsprpalcolor(int pal, int color);

void GB_ScreenDrawScanline(s32 y);
void GBC_ScreenDrawScanline(s32 y);
void GBC_GB_ScreenDrawScanline(s32 y); // GBC when switched to GB mode.

void SGB_ScreenDrawBorder(void);
void SGB_ScreenDrawScanline(s32 y);

//----------------------------------------------------

// Write to buffer in 24 bit format
void GB_Screen_WriteBuffer_24RGB(char *buffer);
void GB_Screenshot(void);

#endif // GB_VIDEO__
