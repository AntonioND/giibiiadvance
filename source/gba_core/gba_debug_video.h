// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_DEBUG_VIDEO__
#define GBA_DEBUG_VIDEO__

#include "gba.h"

void GBA_Debug_PrintZoomedSpriteAt(int spritenum, int buf_has_alpha_channel,
                                   char *buffer, int bufw, int bufh,
                                   int posx, int posy, int sizex, int sizey);

// starting sprite number = page * 64
void GBA_Debug_PrintSpritesPage(int page, int buf_has_alpha_channel,
                                char *buffer, int bufw, int bufh);

void GBA_Debug_PrintTiles(char *buffer, int bufw, int bufh, int cbb,
                          int colors, int palette);
void GBA_Debug_PrintTilesAlpha(char *buffer, int bufw, int bufh, int cbb,
                               int colors, int palette);

void GBA_Debug_TilePrint64x64(char *buffer, int bufw, int bufh, int cbb,
                              int tile, int palcolors, int selected_pal);

void GBA_Debug_PrintBackgroundAlpha(char *buffer, int bufw, int bufh,
                                    u16 control, int bgmode, int page);

#endif // GBA_DEBUG_VIDEO__
