// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_VIDEO__
#define GBA_VIDEO__

#include "gba.h"

void GBA_SkipFrame(int skip);
int GBA_HasToSkipFrame(void);

// Must be called to fill the look up tables used for blending effects.
void GBA_FillFadeTables(void);

// Note: The correct way of emulating is drawing a pixel every 4 clocks. This is
// an optimization that makes pretty much all games show as expected.
void GBA_UpdateDrawScanlineFn(void);

void GBA_VideoUpdateRegister(u32 address);

void GBA_DrawScanline(s32 y);
void GBA_DrawScanlineWhite(s32 y);

// 24-bit RGB
void GBA_ConvertScreenBufferTo24RGB(void *dst);
// 32-bit RGB (with alpha set to 255 in all pixels)
void GBA_ConvertScreenBufferTo32RGB(void *dst);

#endif // GBA_VIDEO__
