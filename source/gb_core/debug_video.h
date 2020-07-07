// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_DEBUG_VIDEO__
#define GB_DEBUG_VIDEO__

// buffer is 24 bit
void GB_Debug_GetSpriteInfo(int sprnum, u8 *x, u8 *y, u8 *tile, u8 *info);
void GB_Debug_PrintSprites(unsigned char *buf);
void GB_Debug_PrintZoomedSprite(unsigned char *buf, int sprite);

// buffer is 32 bit
void GB_Debug_PrintSpritesAlpha(unsigned char *buf);
void GB_Debug_PrintSpriteAlpha(unsigned char *buf, int sprite);

//------------------------------------------------------------------------------

void GB_Debug_GetPalette(int is_sprite, int num, int color,
                         u32 *red, u32 *green, u32 *blue);

//------------------------------------------------------------------------------

void GB_Debug_TileVRAMDraw(unsigned char *buffer0, int bufw0, int bufh0,
                           unsigned char *buffer1, int bufw1, int bufh1);
void GB_Debug_TileVRAMDrawPaletted(unsigned char *buffer0, int bufw0, int bufh0,
                                   unsigned char *buffer1, int bufw1, int bufh1,
                                   int pal, int pal_is_spr);
void GB_Debug_TileDrawZoomed64x64(unsigned char *buffer, int tile, int bank);
void GB_Debug_TileDrawZoomedPaletted64x64(unsigned char *buffer, int tile,
                                          int bank, int palette,
                                          int is_sprite_palette);

//------------------------------------------------------------------------------

void GB_Debug_MapPrint(unsigned char *buffer, int bufw, int bufh, int map,
                       int tile_base);
void GB_Debug_MapPrintBW(unsigned char *buffer, int bufw, int bufh, int map,
                         int tile_base); // Black and white

//------------------------------------------------------------------------------

// index: 0-29
void GB_Debug_GBCameraMiniPhotoPrint(unsigned char *buffer, int bufw, int bufh,
                                     int posx, int posy, int index);

// index: 0-29. if index == -1, get current camera output
void GB_Debug_GBCameraPhotoPrint(unsigned char *buffer, int bufw, int bufh,
                                 int index);

void GB_Debug_GBCameraMiniPhotoPrintAll(unsigned char *buf);

void GB_Debug_GBCameraWebcamOutputPrint(unsigned char *buffer,
                                        int bufw, int bufh);
void GB_Debug_GBCameraRetinaProcessedPrint(unsigned char *buffer,
                                           int bufw, int bufh);

#endif // GB_DEBUG_VIDEO__
