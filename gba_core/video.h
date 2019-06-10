// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __GBA_VIDEO__
#define __GBA_VIDEO__

#include "gba.h"

void GBA_SkipFrame(int skip);
int GBA_HasToSkipFrame(void);

//---------------------------------------------------------

void GBA_FillFadeTables(void);

//---------------------------------------------------------

void GBA_UpdateDrawScanlineFn(void); // ! the correct way of emulating is drawing a pixel every 4 clocks

void GBA_VideoUpdateRegister(u32 address);

void GBA_DrawScanline(s32 y);
void GBA_DrawScanlineWhite(s32 y);

void GBA_ConvertScreenBufferTo24RGB(void * dst); // 24 RGB
void GBA_ConvertScreenBufferTo32RGB(void * dst); // 32 RGB (alpha = 255)

#endif //__GBA_VIDEO__

