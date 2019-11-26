// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_DEBUG_VIDEO__
#define GB_DEBUG_VIDEO__

// buffer is 24 bit
void GB_Debug_GetSpriteInfo(int sprnum, u8 *x, u8 *y, u8 *tile, u8 *info);
void GB_Debug_PrintSprites(char *buf);
void GB_Debug_PrintZoomedSprite(char *buf, int sprite);

// buffer is 32 bit
void GB_Debug_PrintSpritesAlpha(char *buf);
void GB_Debug_PrintSpriteAlpha(char *buf, int sprite);

//------------------------------------------------------------------------------

void GB_Debug_GetPalette(int is_sprite, int num, int color,
                         u32 *red, u32 *green, u32 *blue);

//------------------------------------------------------------------------------

void GB_Debug_TileVRAMDraw(char *buffer0, int bufw0, int bufh0,
                           char *buffer1, int bufw1, int bufh1);
void GB_Debug_TileVRAMDrawPaletted(char *buffer0, int bufw0, int bufh0,
                                   char *buffer1, int bufw1, int bufh1,
                                   int pal, int pal_is_spr);
void GB_Debug_TileDrawZoomed64x64(char *buffer, int tile, int bank);
void GB_Debug_TileDrawZoomedPaletted64x64(char *buffer, int tile, int bank,
                                          int palette, int is_sprite_palette);

//------------------------------------------------------------------------------

void GB_Debug_MapPrint(char *buffer, int bufw, int bufh, int map, int tile_base);
void GB_Debug_MapPrintBW(char *buffer, int bufw, int bufh, int map,
                         int tile_base); // Black and white

//------------------------------------------------------------------------------

// index: 0-29
void GB_Debug_GBCameraMiniPhotoPrint(char *buffer, int bufw, int bufh,
                                     int posx, int posy, int index);

// index: 0-29. if index == -1, get current camera output
void GB_Debug_GBCameraPhotoPrint(char *buffer, int bufw, int bufh, int index);

void GB_Debug_GBCameraMiniPhotoPrintAll(char *buf);

void GB_Debug_GBCameraWebcamOutputPrint(char *buffer, int bufw, int bufh);
void GB_Debug_GBCameraRetinaProcessedPrint(char *buffer, int bufw, int bufh);

#endif // GB_DEBUG_VIDEO__
