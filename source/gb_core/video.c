// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../file_utils.h"
#include "../png_utils.h"

#include "debug.h"
#include "gameboy.h"
#include "general.h"
#include "memory.h"
#include "sgb.h"
#include "sound.h"
#include "video.h"

extern _GB_CONTEXT_ GameBoy;
extern _SGB_INFO_ SGBInfo;

// Variables related to the GameBoy framebuffer
static u32 gb_blur;
static u32 gb_realcolors;
static u32 gb_cur_fb;
static u16 gb_framebuffer[2][256 * 224];

//-----------------------------------------------------------

static int gb_frameskip = 0;

void GB_SkipFrame(int skip)
{
    gb_frameskip = skip;
}

int GB_HasToSkipFrame(void)
{
    return gb_frameskip;
}

//-----------------------------------------------------------

void GB_EnableBlur(int enable)
{
    gb_blur = enable;
}

void GB_EnableRealColors(int enable)
{
    gb_realcolors = enable;
}

static u32 pal_red, pal_green, pal_blue;

void GB_ConfigGetPalette(u8 *red, u8 *green, u8 *blue)
{
    *red = pal_red;
    *green = pal_green;
    *blue = pal_blue;
}

void GB_ConfigSetPalette(u8 red, u8 green, u8 blue)
{
    pal_red = red;
    pal_green = green;
    pal_blue = blue;
}

void GB_ConfigLoadPalette(void)
{
    GB_SetPalette(pal_red, pal_green, pal_blue);
}

// -------------------------------------------------------------
// -------------------------------------------------------------
//                      GAMEBOY
// -------------------------------------------------------------
// -------------------------------------------------------------

static u32 gb_framebuffer_bgcolor0[256];
static u32 gb_framebuffer_bgpriority[256]; // For GBC

static int window_current_line;

static u32 gbpalettes[4] = {
    GB_RGB(31, 31, 31), GB_RGB(21, 21, 21), GB_RGB(10, 10, 10), GB_RGB(0, 0, 0)
};

void GB_SetPalette(u32 red, u32 green, u32 blue)
{
    gbpalettes[0] = GB_RGB(red >> 3, green >> 3, blue >> 3);
    gbpalettes[1] = GB_RGB(((red * 2) / 3) >> 3, ((green * 2) / 3) >> 3,
                           ((blue * 2) / 3) >> 3);
    gbpalettes[2] = GB_RGB((red / 3) >> 3, (green / 3) >> 3, (blue / 3) >> 3);
    gbpalettes[3] = GB_RGB(0, 0, 0);
}

u32 GB_GameBoyGetGray(u32 number)
{
    return gbpalettes[number & 3];
}

void GB_ScreenDrawScanline(s32 y)
{
    if (GB_HasToSkipFrame())
        return;

    if (y > 143)
        return;

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (y == 0)
    {
        window_current_line = 0;
        if ((mem->IO_Ports[WX_REG - 0xFF00] - 7) > 159)
        {
            if (mem->IO_Ports[WY_REG - 0xFF00] > 143)
                window_current_line = -1; // Don't draw
        }
        if ((mem->IO_Ports[LCDC_REG - 0xFF00] & (1 << 5)) == 0)
            window_current_line = -1; // Don't draw
    }

    u32 base_index = y << 8;
    u32 lcd_reg = mem->IO_Ports[LCDC_REG - 0xFF00];

    if (GameBoy.Emulator.lcd_on) // Screen enabled
    {
        if (GameBoy.Emulator.CPUHalt == 2)
        {
            for (int x = 0; x < 160; x++)
                gb_framebuffer[gb_cur_fb][base_index + x] = GB_RGB(31, 31, 31);
            if (y == 143)
                gb_cur_fb ^= 1;
            return;
        }

        u32 bg_pal[4];
        u32 spr_pal0[4];
        u32 spr_pal1[4];

        // GameBoy palettes
        u32 bgp_reg = mem->IO_Ports[BGP_REG - 0xFF00];
        bg_pal[0] = GB_GameBoyGetGray(bgp_reg & 0x3);
        bg_pal[1] = GB_GameBoyGetGray((bgp_reg >> 2) & 0x3);
        bg_pal[2] = GB_GameBoyGetGray((bgp_reg >> 4) & 0x3);
        bg_pal[3] = GB_GameBoyGetGray((bgp_reg >> 6) & 0x3);

        u32 obp0_reg = mem->IO_Ports[OBP0_REG - 0xFF00];
        //spr_pal0[0] = GB_GameBoyGetGray(obp0_reg & 0x3);
        spr_pal0[1] = GB_GameBoyGetGray((obp0_reg >> 2) & 0x3);
        spr_pal0[2] = GB_GameBoyGetGray((obp0_reg >> 4) & 0x3);
        spr_pal0[3] = GB_GameBoyGetGray((obp0_reg >> 6) & 0x3);

        u32 obp1_reg = mem->IO_Ports[OBP1_REG - 0xFF00];
        //spr_pal1[0] = GB_GameBoyGetGray(obp1_reg & 0x3);
        spr_pal1[1] = GB_GameBoyGetGray((obp1_reg >> 2) & 0x3);
        spr_pal1[2] = GB_GameBoyGetGray((obp1_reg >> 4) & 0x3);
        spr_pal1[3] = GB_GameBoyGetGray((obp1_reg >> 6) & 0x3);

        // Scroll values
        u32 scx_reg = mem->IO_Ports[SCX_REG - 0xFF00];
        u32 scy_reg = mem->IO_Ports[SCY_REG - 0xFF00];
        u32 wx_reg = mem->IO_Ports[WX_REG - 0xFF00];
        u32 wy_reg = mem->IO_Ports[WY_REG - 0xFF00];

        // Other
        u8 *tiledata = (lcd_reg & (1 << 4)) ?
                            &mem->VideoRAM[0x0000] : &mem->VideoRAM[0x0800];
        u8 *bgtilemap = (lcd_reg & (1 << 3)) ?
                            &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];
        u8 *wintilemap = (lcd_reg & (1 << 6)) ?
                            &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];

        bool increase_win = false;

        // Draw BG + window
        for (int x = 0; x < 160; x++)
        {
            u32 color = bg_pal[0];
            bool window_draw = 0;
            bool bg_color0 = false;

            if (window_current_line >= 0)
            {
                if ((lcd_reg & (1 << 5)) && (lcd_reg & (1 << 0))) // Window
                {
                    if (wy_reg <= y)
                    {
                        if ((wx_reg < 8)
                            || ((wx_reg > 7) && ((wx_reg - 7) <= x)))
                        {
                            increase_win = true;

                            u32 y_ = window_current_line;
                            u32 x_ = x - wx_reg;
                            u32 tile = wintilemap[((y_ >> 3) * 32)
                                                  + ((x_ + 7) >> 3)];

                            if (!(lcd_reg & (1 << 4))) // If tile base is 0x8800
                            {
                                if (tile & (1 << 7))
                                    tile &= 0x7F;
                                else
                                    tile += 128;
                            }

                            u8 *data = &tiledata[tile << 4];

                            data += (y_ & 7) * 2;

                            x_ = 7 - ((x_ + 7) & 7);

                            color = ((*data >> x_) & 1)
                                    | ((((*(data + 1)) >> x_) << 1) & 2);

                            bg_color0 = (color == 0);

                            color = bg_pal[color];
                            window_draw = true;
                        }
                    }
                }
            }

            if (lcd_reg & (1 << 0) && (!window_draw)) // BG
            {
                u32 x_ = x + scx_reg;
                u32 y_ = y + scy_reg;

                u32 tile = bgtilemap[(((y_ >> 3) & 31) * 32) + ((x_ >> 3) & 31)];

                if (!(lcd_reg & (1 << 4))) // If tile base is 0x8800
                {
                    if (tile & (1 << 7))
                        tile &= 0x7F;
                    else
                        tile += 128;
                }

                u8 *data = &tiledata[tile << 4];

                data += (y_ & 7) << 1;

                x_ = 7 - (x_ & 7);

                color =
                        ((*data >> x_) & 1) | ((((*(data + 1)) >> x_) << 1) & 2);

                bg_color0 = (color == 0);

                color = bg_pal[color];
            }

            gb_framebuffer[gb_cur_fb][base_index + x] = color;
            gb_framebuffer_bgcolor0[x] = bg_color0;
        }

        if (increase_win)
            window_current_line++;

        // If sprites are enabled, draw the ones visible this scanline
        // Note: The BG transparent color is bg_pal[0]
        if (lcd_reg & (1 << 1))
        {
            s32 spriteheight = 8 << ((lcd_reg & (1 << 2)) != 0);
            _GB_OAM_ *GB_OAM = (void *)mem->ObjAttrMem;
            _GB_OAM_ENTRY_ *GB_Sprite;

            // For 8x16 sprites, last bit is ignored
            u32 tilemask = ((spriteheight == 16) ? 0xFE : 0xFF);

            u32 spritesdrawn = 0; // For the 10-sprites-per-scanline limit
            u32 drawsprite[40];

            for (int a = 0; a < 40; a++)
            {
                GB_Sprite = &GB_OAM->Sprite[a];

                drawsprite[a] = 0;

                s32 real_y = GB_Sprite->Y - 16;

                if ((real_y <= y) && ((real_y + spriteheight) > y))
                {
                    if (spritesdrawn < 10)
                    {
                        drawsprite[a] = 1;
                        spritesdrawn++;
                    }
                }
            }

            for (int a = 39; a >= 0; a--)
            {
                // TODO: Fix.
                // When sprites with different x coordinate values overlap, the
                // one with the smaller x coordinate (closer to the left) will
                // have priority and appear above any others. This applies in
                // Non CGB Mode only.

                if (drawsprite[a])
                {
                    GB_Sprite = &GB_OAM->Sprite[a];

                    s32 real_y = GB_Sprite->Y - 16;

                    if ((real_y <= y) && ((real_y + spriteheight) > y))
                    {
                        u32 tile = GB_Sprite->Tile & tilemask;

                        u8 *data = &mem->VideoRAM[tile << 4];

                        // Flip Y
                        if (GB_Sprite->Info & (1 << 6))
                            data += (spriteheight - y + real_y - 1) * 2;
                        else
                            data += (y - real_y) * 2;

                        // Lets draw the sprite...
                        s32 real_x = GB_Sprite->X - 8;
                        for (int x__ = 0; x__ < 8; x__++)
                        {
                            u32 color = ((*data >> x__) & 1)
                                        | ((((*(data + 1)) >> x__) << 1) & 2);

                            if (color != 0) // Color 0 is transparent
                            {
                                if (GB_Sprite->Info & (1 << 4))
                                    color = spr_pal1[color];
                                else
                                    color = spr_pal0[color];

                                s32 x_ = real_x;

                                // Flip X
                                if (GB_Sprite->Info & (1 << 5))
                                    x_ = x_ + x__;
                                else
                                    x_ += 7 - x__;

                                if (x_ >= 0 && x_ < 160)
                                {
                                    // If BG has priority and it is enabled...
                                    if ((GB_Sprite->Info & (1 << 7))
                                        && (lcd_reg & (1 << 0)))
                                    {
                                        if (gb_framebuffer_bgcolor0[x_])
                                        {
                                            gb_framebuffer[gb_cur_fb]
                                                          [base_index + x_] = color;
                                        }
                                    }
                                    else
                                    {
                                        gb_framebuffer[gb_cur_fb]
                                                      [base_index + x_] = color;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (int x = 0; x < 160; x++)
        {
            gb_framebuffer[gb_cur_fb][base_index + x] = GB_RGB(31, 31, 31);
        }
    }

    if (y == 143)
        gb_cur_fb ^= 1;
}

//******************************************************************************

u32 gbc_getbgpalcolor(int pal, int color)
{
    u32 pal_ram_index = (pal * 8) + (2 * color);
    return GameBoy.Emulator.bg_pal[pal_ram_index]
           | (GameBoy.Emulator.bg_pal[pal_ram_index + 1] << 8);
}

u32 gbc_getsprpalcolor(int pal, int color)
{
    u32 pal_ram_index = (pal * 8) + (2 * color);
    return GameBoy.Emulator.spr_pal[pal_ram_index]
           | (GameBoy.Emulator.spr_pal[pal_ram_index + 1] << 8);
}

void GBC_ScreenDrawScanline(s32 y)
{
    if (GB_HasToSkipFrame())
        return;

    if (y > 143)
        return;

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (y == 0)
    {
        window_current_line = 0;
        if ((mem->IO_Ports[WX_REG - 0xFF00] - 7) > 159)
        {
            if (mem->IO_Ports[WY_REG - 0xFF00] > 143)
                window_current_line = -1; // Don't draw
        }
        if ((mem->IO_Ports[LCDC_REG - 0xFF00] & (1 << 5)) == 0)
            window_current_line = -1; // Don't draw
    }

    u32 base_index = y << 8;

    u32 lcd_reg = mem->IO_Ports[LCDC_REG - 0xFF00];

    if (GameBoy.Emulator.lcd_on) // Screen enabled
    {
        if (GameBoy.Emulator.CPUHalt == 2)
        {
            // Believe it or not, it behaves this way... or at least my tests
            // say so. However, even though the sound hadrware changes the
            // color, the IR port doesn't seem to matter.
            u32 color_ = GB_SoundHardwareIsOn() ?
                         GB_RGB(31,31,31) : GB_RGB(0,0,0);
            for (int x = 0; x < 160; x++)
                gb_framebuffer[gb_cur_fb][base_index + x] = color_;
            if (y == 143)
                gb_cur_fb ^= 1;
            return;
        }

        // Scroll values
        u32 scx_reg = mem->IO_Ports[SCX_REG - 0xFF00];
        u32 scy_reg = mem->IO_Ports[SCY_REG - 0xFF00];
        u32 wx_reg = mem->IO_Ports[WX_REG - 0xFF00];
        u32 wy_reg = mem->IO_Ports[WY_REG - 0xFF00];

        // Other
        u8 *tiledata = (lcd_reg & (1 << 4)) ?
                                &mem->VideoRAM[0x0000] : &mem->VideoRAM[0x0800];
        u8 *bgtilemap = (lcd_reg & (1 << 3)) ?
                                &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];
        u8 *wintilemap = (lcd_reg & (1 << 6)) ?
                                &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];

        bool increase_win = false;

        // Draw BG + window
        for (int x = 0; x < 160; x++)
        {
            u32 color = GB_RGB(31, 31, 31);
            bool window_draw = 0;
            bool color0 = false;
            u32 tileinfo;

            if (window_current_line >= 0)
            {
                if (lcd_reg & (1 << 5)) // Window
                {
                    if (wy_reg <= y)
                    {
                        if ((wx_reg < 8)
                            || ((wx_reg > 7) && ((wx_reg - 7) <= x)))
                        {
                            increase_win = true;

                            u32 y_ = window_current_line;
                            u32 x_ = x - wx_reg;

                            u32 tile_location = ((y_ >> 3) * 32)
                                                + ((x_ + 7) >> 3);
                            u32 tile = wintilemap[tile_location];
                            tileinfo = wintilemap[tile_location + 0x2000];

                            if (!(lcd_reg & (1 << 4))) // If tile base is 0x8800
                            {
                                if (tile & (1 << 7))
                                    tile &= 0x7F;
                                else
                                    tile += 128;
                            }

                            // Bank 1?
                            u8 *data = &tiledata[(tile << 4)
                                    + ((tileinfo & (1 << 3)) ? 0x2000 : 0 )];

                            // V flip
                            if (tileinfo & (1 << 6))
                                data += ((7 - y_) & 7) * 2;
                            else
                                data += (y_ & 7) * 2;

                            u32 pal_index = tileinfo & 7;

                            // H flip
                            if (tileinfo & (1 << 5))
                                x_ = (x_ + 7) & 7;
                            else
                                x_ = 7 - ((x_ + 7) & 7);

                            color = ((*data >> x_) & 1)
                                    | ((((*(data + 1)) >> x_) << 1) & 2);

                            color0 = (color == 0);

                            u32 pal_ram_index = (pal_index * 8) + (2 * color);
                            color = GameBoy.Emulator.bg_pal[pal_ram_index]
                                    | (GameBoy.Emulator.bg_pal[pal_ram_index + 1] << 8);
                            window_draw = true;
                        }
                    }
                }
            }

            if (!window_draw) // BG
            {
                u32 x_ = x + scx_reg;
                u32 y_ = y + scy_reg;

                u32 tile_location = (((y_ >> 3) & 31) * 32) + ((x_ >> 3) & 31);
                u32 tile = bgtilemap[tile_location];
                tileinfo = bgtilemap[tile_location + 0x2000];

                if (!(lcd_reg & (1 << 4))) // If tile base is 0x8800
                {
                    if (tile & (1 << 7))
                        tile &= 0x7F;
                    else
                        tile += 128;
                }

                // Bank 1?
                u8 *data = &tiledata[(tile << 4)
                                     + ((tileinfo & (1 << 3)) ? 0x2000 : 0)];

                // V flip
                if (tileinfo & (1 << 6))
                    data += ((7 - y_) & 7) * 2;
                else
                    data += (y_ & 7) * 2;

                // H flip
                if (tileinfo & (1 << 5))
                    x_ = x_ & 7;
                else
                    x_ = 7 - (x_ & 7);

                color = ((*data >> x_) & 1)
                        | ((((*(data + 1)) >> x_) << 1) & 2);
                color0 = (color == 0);

                u32 pal_index = tileinfo & 7;

                u32 pal_ram_index = (pal_index * 8) + (2 * color);
                color = GameBoy.Emulator.bg_pal[pal_ram_index]
                        | (GameBoy.Emulator.bg_pal[pal_ram_index + 1] << 8);
            }

            gb_framebuffer[gb_cur_fb][base_index + x] = color;
            gb_framebuffer_bgcolor0[x] = color0;
            gb_framebuffer_bgpriority[x] = ((tileinfo & (1 << 7)) != 0);
        }

        if (increase_win)
            window_current_line++;

        // If sprites are enabled, draw the ones visible this scanline
        // Note: The BG transparent color is bg_pal[0]
        if (lcd_reg & (1 << 1))
        {
            s32 spriteheight = 8 << ((lcd_reg & (1 << 2)) != 0);
            _GB_OAM_ *GB_OAM = (void *)mem->ObjAttrMem;
            _GB_OAM_ENTRY_ *GB_Sprite;

            // For 8x16 sprites, last bit is ignored
            u32 tilemask = ((spriteheight == 16) ? 0xFE : 0xFF);

            u32 spritesdrawn = 0; // For the 10-sprites-per-scanline limit
            u32 drawsprite[40];

            for (int a = 0; a < 40; a++)
            {
                GB_Sprite = &GB_OAM->Sprite[a];

                drawsprite[a] = 0;

                s32 real_y = GB_Sprite->Y - 16;

                if ((real_y <= y) && ((real_y + spriteheight) > y))
                {
                    if (spritesdrawn < 10)
                    {
                        drawsprite[a] = 1;
                        spritesdrawn++;
                    }
                }
            }

            for (int a = 39; a >= 0; a--)
            {
                if (!drawsprite[a])
                    continue;

                GB_Sprite = &GB_OAM->Sprite[a];

                s32 real_y = GB_Sprite->Y - 16;

                if ((real_y <= y) && ((real_y + spriteheight) > y))
                {
                    u32 tile = GB_Sprite->Tile & tilemask;

                    u8 *data = &mem->VideoRAM[tile << 4]; // Bank 0

                    if (GB_Sprite->Info & (1 << 3)) // Bank 1
                        data += 0x2000;

                    // Flip Y
                    if (GB_Sprite->Info & (1 << 6))
                        data += (spriteheight - y + real_y - 1) * 2;
                    else
                        data += (y - real_y) * 2;

                    u32 pal_index = GB_Sprite->Info & 7;

                    pal_index = pal_index * 8;

                    // Let's draw the sprite...
                    s32 real_x = GB_Sprite->X - 8;
                    for (int x__ = 0; x__ < 8; x__++)
                    {
                        u32 color = ((*data >> x__) & 1)
                                    | ((((*(data + 1)) >> x__) << 1) & 2);

                        if (color == 0) // Color 0 is transparent
                            continue;

                        u32 pal_mem_pointer = pal_index + (2 * color);
                        color = GameBoy.Emulator.spr_pal[pal_mem_pointer]
                                | (GameBoy.Emulator.spr_pal[pal_mem_pointer + 1] << 8);

                        s32 x_ = real_x;

                        // Flip X
                        if (GB_Sprite->Info & (1 << 5))
                            x_ = x_ + x__;
                        else
                            x_ += 7 - x__;

                        if ((x_ >= 0) && (x_ < 160))
                        {
                            // Priorities...
                            if ((lcd_reg & (1 << 0)) == 0) // Master priority
                            {
                                gb_framebuffer[gb_cur_fb][base_index + x_] =
                                        color;
                            }
                            else if (gb_framebuffer_bgpriority[x_]) // BG priority
                            {
                                if (gb_framebuffer_bgcolor0[x_])
                                    gb_framebuffer[gb_cur_fb][base_index + x_] =
                                            color;
                            }
                            else // OAM priority
                            {
                                if (GB_Sprite->Info & (1 << 7))
                                {
                                    if (gb_framebuffer_bgcolor0[x_])
                                    {
                                        gb_framebuffer[gb_cur_fb]
                                                      [base_index + x_] = color;
                                    }
                                }
                                else
                                {
                                    gb_framebuffer[gb_cur_fb]
                                                  [base_index + x_] = color;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (int x = 0; x < 160; x++)
        {
            gb_framebuffer[gb_cur_fb][base_index + x] = GB_RGB(31, 31, 31);
        }
    }

    if (y == 143)
        gb_cur_fb ^= 1;
}

void GBC_GB_ScreenDrawScanline(s32 y)
{
    if (GB_HasToSkipFrame())
        return;

    if (y > 143)
        return;

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (y == 0)
    {
        window_current_line = 0;
        if ((mem->IO_Ports[WX_REG - 0xFF00] - 7) > 159)
        {
            if (mem->IO_Ports[WY_REG - 0xFF00] > 143)
                window_current_line = -1; // Don't draw
        }
        if ((mem->IO_Ports[LCDC_REG - 0xFF00] & (1 << 5)) == 0)
            window_current_line = -1; // Don't draw
    }

    u32 base_index = y << 8;
    u32 lcd_reg = mem->IO_Ports[LCDC_REG - 0xFF00];

    if (GameBoy.Emulator.lcd_on) // Screen enabled
    {
        if (GameBoy.Emulator.CPUHalt == 2)
        {
            for (int x = 0; x < 160; x++)
                gb_framebuffer[gb_cur_fb][base_index + x] = GB_RGB(0, 0, 0);
            if (y == 143)
                gb_cur_fb ^= 1;
            return;
        }

        u32 bg_pal[4];
        u32 spr_pal0[4];
        u32 spr_pal1[4];

        // Palettes
        u32 bgp_reg = mem->IO_Ports[BGP_REG - 0xFF00];
        bg_pal[0] = gbc_getbgpalcolor(0, bgp_reg & 0x3);
        bg_pal[1] = gbc_getbgpalcolor(0, (bgp_reg >> 2) & 0x3);
        bg_pal[2] = gbc_getbgpalcolor(0, (bgp_reg >> 4) & 0x3);
        bg_pal[3] = gbc_getbgpalcolor(0, (bgp_reg >> 6) & 0x3);

        u32 obp0_reg = mem->IO_Ports[OBP0_REG - 0xFF00];
        //spr_pal0[0] = gbc_getsprpalcolor(0, obp0_reg & 0x3);
        spr_pal0[1] = gbc_getsprpalcolor(0, (obp0_reg >> 2) & 0x3);
        spr_pal0[2] = gbc_getsprpalcolor(0, (obp0_reg >> 4) & 0x3);
        spr_pal0[3] = gbc_getsprpalcolor(0, (obp0_reg >> 6) & 0x3);

        u32 obp1_reg = mem->IO_Ports[OBP1_REG - 0xFF00];
        //spr_pal1[0] = gbc_getsprpalcolor(1, obp1_reg & 0x3);
        spr_pal1[1] = gbc_getsprpalcolor(1, (obp1_reg >> 2) & 0x3);
        spr_pal1[2] = gbc_getsprpalcolor(1, (obp1_reg >> 4) & 0x3);
        spr_pal1[3] = gbc_getsprpalcolor(1, (obp1_reg >> 6) & 0x3);

        // Scroll values
        u32 scx_reg = mem->IO_Ports[SCX_REG - 0xFF00];
        u32 scy_reg = mem->IO_Ports[SCY_REG - 0xFF00];
        u32 wx_reg = mem->IO_Ports[WX_REG - 0xFF00];
        u32 wy_reg = mem->IO_Ports[WY_REG - 0xFF00];

        // Other
        u8 *tiledata = (lcd_reg & (1 << 4)) ?
                                &mem->VideoRAM[0x0000] : &mem->VideoRAM[0x0800];
        u8 *bgtilemap = (lcd_reg & (1 << 3)) ?
                                &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];
        u8 *wintilemap = (lcd_reg & (1 << 6)) ?
                                &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];

        bool increase_win = false;

        // Draw BG + window
        for (int x = 0; x < 160; x++)
        {
            // This should only disable BG, but GBC in GB mode disables both
            // window and BG
            if (lcd_reg & (1 << 0))
            {
                u32 color = GB_RGB(31, 31, 31);
                bool window_draw = 0;
                bool bg_color0 = false;

                if (window_current_line >= 0)
                {
                    if (lcd_reg & (1 << 5)) // Window
                    {
                        if (wy_reg <= y)
                        {
                            if ((wx_reg < 8)
                                || ((wx_reg > 7) && ((wx_reg - 7) <= x)))
                            {
                                increase_win = true;

                                u32 y_ = window_current_line;
                                u32 x_ = x - wx_reg;
                                u32 tile = wintilemap[((y_ >> 3) * 32)
                                                      + ((x_ + 7) >> 3)];

                                // If tile base is 0x8800
                                if (!(lcd_reg & (1 << 4)))
                                {
                                    if (tile & (1 << 7))
                                        tile &= 0x7F;
                                    else
                                        tile += 128;
                                }

                                u8 *data = &tiledata[tile << 4];

                                data += (y_ & 7) * 2;

                                x_ = 7 - ((x_ + 7) & 7);

                                color = ((*data >> x_) & 1)
                                        | ((((*(data + 1)) >> x_) << 1) & 2);

                                bg_color0 = (color == 0);

                                color = bg_pal[color];
                                window_draw = true;
                            }
                        }
                    }
                }

                if (!window_draw) // BG
                {
                    u32 x_ = x + scx_reg;
                    u32 y_ = y + scy_reg;

                    u32 tile = bgtilemap[(((y_ >> 3) & 31) * 32)
                                         + ((x_ >> 3) & 31)];

                    if (!(lcd_reg & (1 << 4))) // If tile base is 0x8800
                    {
                        if (tile & (1 << 7))
                            tile &= 0x7F;
                        else
                            tile += 128;
                    }

                    u8 *data = &tiledata[tile << 4];

                    data += (y_ & 7) << 1;

                    x_ = 7 - (x_ & 7);

                    color = ((*data >> x_) & 1)
                            | ((((*(data + 1)) >> x_) << 1) & 2);

                    bg_color0 = (color == 0);

                    color = bg_pal[color];
                }

                gb_framebuffer[gb_cur_fb][base_index + x] = color;
                gb_framebuffer_bgcolor0[x] = bg_color0;
            }
            else
            {
                gb_framebuffer[gb_cur_fb][base_index + x] = bg_pal[0];
                gb_framebuffer_bgcolor0[x] = 1;
            }
        }

        if (increase_win)
            window_current_line++;

        // If sprites are enabled, draw the ones visible this scanline
        // Note: The BG transparent color is bg_pal[0]
        if (lcd_reg & (1 << 1))
        {
            s32 spriteheight = 8 << ((lcd_reg & (1 << 2)) != 0);
            _GB_OAM_ *GB_OAM = (void *)mem->ObjAttrMem;
            _GB_OAM_ENTRY_ *GB_Sprite;

            // For 8x16 sprites, last bit is ignored
            u32 tilemask = ((spriteheight == 16) ? 0xFE : 0xFF);

            u32 spritesdrawn = 0; // For the 10-sprites-per-scanline limit
            u32 drawsprite[40];

            for (int a = 0; a < 40; a++)
            {
                GB_Sprite = &GB_OAM->Sprite[a];

                drawsprite[a] = 0;

                s32 real_y = GB_Sprite->Y - 16;

                if ((real_y <= y) && ((real_y + spriteheight) > y))
                {
                    if (spritesdrawn < 10)
                    {
                        drawsprite[a] = 1;
                        spritesdrawn++;
                    }
                }
            }

            for (int a = 39; a >= 0; a--)
            {
                // TODO: Fix.
                // When sprites with different x coordinate values overlap, the
                // one with the smaller x coordinate (closer to the left) will
                // have priority and appear above any others. This applies in
                // Non CGB Mode only.

                if (!drawsprite[a])
                    continue;

                GB_Sprite = &GB_OAM->Sprite[a];

                s32 real_y = GB_Sprite->Y - 16;

                if ((real_y <= y) && ((real_y + spriteheight) > y))
                {
                    u32 tile = GB_Sprite->Tile & tilemask;

                    u8 *data = &mem->VideoRAM[tile << 4];

                    // Flip Y
                    if (GB_Sprite->Info & (1 << 6))
                        data += (spriteheight - y + real_y - 1) * 2;
                    else
                        data += (y - real_y) * 2;

                    // Lets draw the sprite...
                    s32 real_x = GB_Sprite->X - 8;
                    for (int x__ = 0; x__ < 8; x__++)
                    {
                        u32 color = ((*data >> x__) & 1)
                                    | ((((*(data + 1)) >> x__) << 1) & 2);

                        if (color != 0) // Color 0 is transparent
                        {
                            if (GB_Sprite->Info & (1 << 4))
                                color = spr_pal1[color];
                            else
                                color = spr_pal0[color];

                            s32 x_ = real_x;

                            // Flip X
                            if (GB_Sprite->Info & (1 << 5))
                                x_ = x_ + x__;
                            else
                                x_ += 7 - x__;

                            if ((x_ >= 0) && (x_ < 160))
                            {
                                // If BG has priority and it is enabled...
                                if ((GB_Sprite->Info & (1 << 7))
                                    && (lcd_reg & (1 << 0)))
                                {
                                    if (gb_framebuffer_bgcolor0[x_])
                                        gb_framebuffer[gb_cur_fb][base_index + x_] = color;
                                }
                                else
                                {
                                    gb_framebuffer[gb_cur_fb][base_index + x_] = color;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (int x = 0; x < 160; x++)
        {
            gb_framebuffer[gb_cur_fb][base_index + x] = GB_RGB(31, 31, 31);
        }
    }

    if (y == 143)
        gb_cur_fb ^= 1;
}

//******************************************************************************

u32 SGB_GetPixelColor(u32 x, u32 y, u32 palindex)
{
    int pal = SGBInfo.ATF_list[SGBInfo.curr_ATF][(20 * (y >> 3)) + (x >> 3)];
    return SGBInfo.palette[pal][palindex];
}

void SGB_ScreenDrawBorder(void)
{
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 28; j++)
        {
            // The inside will be drawn in other function
            if (!((i > 5 && i < 26) && (j > 4 && j < 23)))
            {
                u32 info = SGBInfo.tile_map[(j * 32) + i];

                u32 tile = info & 0xFF;

                u32 *tile_ptr = &SGBInfo.tile_data[((8 * 8 * 4) / 8) * tile];

                u32 pal = (info >> 10) & 7; // 4 to 7 (officially 4 to 6)
                if (pal < 4)
                    pal += 4;

                //u32 prio = info & (1 << 13); // Not used

                u32 xflip = info & (1 << 14);
                u32 yflip = info & (1 << 15);

                for (int y = 0; y < 8; y++)
                {
                    for (int x = 0; x < 8; x++)
                    {
                        u32 *data = tile_ptr;
                        u32 *data2 = tile_ptr + 16;

                        if (yflip)
                        {
                            data += (7 - y) << 1;
                            data2 += (7 - y) << 1;
                        }
                        else
                        {
                            data += y << 1;
                            data2 += y << 1;
                        }

                        u32 x_;
                        if (xflip)
                            x_ = x;
                        else
                            x_ = 7 - x;

                        u32 color = (*data >> x_) & 1;
                        color |= ((((*(data + 1)) >> x_) << 1) & (1 << 1));
                        color |= ((((*data2) >> x_) << 2) & (1 << 2));
                        color |= ((((*(data2 + 1)) >> x_) << 3) & (1 << 3));
                        color = SGBInfo.palette[pal][color];

                        int temp = ((y + (j << 3)) * 256) + (x + (i << 3));
                        gb_framebuffer[0][temp] = color;
                        gb_framebuffer[1][temp] = color;
                    }
                }
            }
        }
    }
}

void SGB_ScreenDrawBorderInside(void)
{
    for (int i = 6; i < 26; i++)
    {
        for (int j = 4; j < 23; j++)
        {
            u32 info = SGBInfo.tile_map[(j * 32) + i];

            u32 *tile_ptr =
                    &SGBInfo.tile_data[((8 * 8 * 4) / 8) * (info & 0xFF)];

            u32 pal = (info >> 10) & 7; // 4 to 7 (officially 4 to 6)
            if (pal < 4)
                pal += 4;

            //u32 prio = info & (1 << 13); // Not used

            u32 xflip = info & (1 << 14);
            u32 yflip = info & (1 << 15);

            for (int y = 0; y < 8; y++)
            {
                u32 *data = tile_ptr;
                u32 *data2 = tile_ptr + 16;

                if (yflip)
                {
                    data += (7 - y) << 1;
                    data2 += (7 - y) << 1;
                }
                else
                {
                    data += y << 1;
                    data2 += y << 1;
                }

                //if (*data != 0 && *data2 != 0)
                //{
                    for (int x = 0; x < 8; x++)
                    {
                        u32 x_;
                        if (xflip)
                            x_ = x;
                        else
                            x_ = 7 - x;

                        u32 color = (*data >> x_) & 1;
                        color |= ((((*(data + 1)) >> x_) << 1) & (1 << 1));
                        color |= ((((*data2) >> x_) << 2) & (1 << 2));
                        color |= ((((*(data2 + 1)) >> x_) << 3) & (1 << 3));

                        if (color != 0)
                        {
                            color = SGBInfo.palette[pal][color];

                            int temp = ((y + (j << 3)) * 256) + (x + (i << 3));
                            gb_framebuffer[0][temp] = color;
                            gb_framebuffer[1][temp] = color;
                        }
                    }
                //}
            }
        }
    }
}

void SGB_ScreenDrawScanline(s32 y)
{
    if (GB_HasToSkipFrame())
        return;

    if (y > 143)
        return;

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (y == 0)
    {
        window_current_line = 0;
        if ((mem->IO_Ports[WX_REG - 0xFF00] - 7) > 159)
        {
            if (mem->IO_Ports[WY_REG - 0xFF00] > 143)
                window_current_line = -1; // Don't draw
        }
        if ((mem->IO_Ports[LCDC_REG - 0xFF00] & (1 << 5)) == 0)
            window_current_line = -1; // Don't draw
    }

    u32 base_index = (40 + y) << 8;

    if (SGBInfo.freeze_screen == SGB_SCREEN_FREEZE)
    {
        if (y == 143)
        {
            SGB_ScreenDrawBorderInside();
            gb_cur_fb ^= 1;
        }
        return;
    }
    else if (SGBInfo.freeze_screen == SGB_SCREEN_BLACK)
    {
        for (int x = 48; x < 160 + 48; x++)
            gb_framebuffer[gb_cur_fb][base_index + x] = GB_RGB(0, 0, 0);

        if (y == 143)
        {
            SGB_ScreenDrawBorderInside();
            gb_cur_fb ^= 1;
        }
        return;
    }
    else if (SGBInfo.freeze_screen == SGB_SCREEN_BACKDROP)
    {
        u32 color_ = SGBInfo.palette[0][0];
        for (int x = 48; x < 160 + 48; x++)
            gb_framebuffer[gb_cur_fb][base_index + x] = color_;

        if (y == 143)
        {
            SGB_ScreenDrawBorderInside();
            gb_cur_fb ^= 1;
        }
        return;
    }

    u32 lcd_reg = mem->IO_Ports[LCDC_REG - 0xFF00];

    if (GameBoy.Emulator.lcd_on) // Screen enabled
    {
        if (GameBoy.Emulator.CPUHalt == 2)
        {
            for (int x = 0; x < 160; x++)
            {
                gb_framebuffer[gb_cur_fb][base_index + x + 48] =
                        GB_RGB(31, 31, 31);
            }

            if (y == 143)
            {
                SGB_ScreenDrawBorderInside();
                gb_cur_fb ^= 1;
            }
            return;
        }

        u32 bg_pal[4];
        u32 spr_pal0[4];
        u32 spr_pal1[4];

        // GameBoy palettes
        u32 bgp_reg = mem->IO_Ports[BGP_REG - 0xFF00];
        bg_pal[0] = bgp_reg & 0x3;
        bg_pal[1] = (bgp_reg >> 2) & 0x3;
        bg_pal[2] = (bgp_reg >> 4) & 0x3;
        bg_pal[3] = (bgp_reg >> 6) & 0x3;

        u32 obp0_reg = mem->IO_Ports[OBP0_REG - 0xFF00];
        //spr_pal0[0] = obp0_reg & 0x3;
        spr_pal0[1] = (obp0_reg >> 2) & 0x3;
        spr_pal0[2] = (obp0_reg >> 4) & 0x3;
        spr_pal0[3] = (obp0_reg >> 6) & 0x3;

        u32 obp1_reg = mem->IO_Ports[OBP1_REG - 0xFF00];
        //spr_pal1[0] = obp1_reg & 0x3;
        spr_pal1[1] = (obp1_reg >> 2) & 0x3;
        spr_pal1[2] = (obp1_reg >> 4) & 0x3;
        spr_pal1[3] = (obp1_reg >> 6) & 0x3;

        // Scroll values
        u32 scx_reg = mem->IO_Ports[SCX_REG - 0xFF00];
        u32 scy_reg = mem->IO_Ports[SCY_REG - 0xFF00];
        u32 wx_reg = mem->IO_Ports[WX_REG - 0xFF00];
        u32 wy_reg = mem->IO_Ports[WY_REG - 0xFF00];

        // Other
        u8 *tiledata = (lcd_reg & (1 << 4)) ?
                                &mem->VideoRAM[0x0000] : &mem->VideoRAM[0x0800];
        u8 *bgtilemap = (lcd_reg & (1 << 3)) ?
                                &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];
        u8 *wintilemap = (lcd_reg & (1 << 6)) ?
                                &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];

        bool increase_win = false;

        // Draw BG + window
        for (int x = 0; x < 160; x++)
        {
            u32 color = SGB_GetPixelColor(x, y, bg_pal[0]);
            bool window_draw = 0;
            bool bg_color0 = false;

            if (window_current_line >= 0)
            {
                if ((lcd_reg & (1 << 5)) && (lcd_reg & (1 << 0))) // Window
                {
                    if (wy_reg <= y)
                    {
                        if ((wx_reg < 8) || ((wx_reg > 7) && ((wx_reg - 7) <= x)))
                        {
                            increase_win = true;

                            u32 y_ = window_current_line;
                            u32 x_ = x - wx_reg;
                            u32 tile = wintilemap[((y_ >> 3) * 32)
                                                  + ((x_ + 7) >> 3)];

                            if (!(lcd_reg & (1 << 4))) // If tile base is 0x8800
                            {
                                if (tile & (1 << 7))
                                    tile &= 0x7F;
                                else
                                    tile += 128;
                            }

                            u8 *data = &tiledata[tile << 4];

                            data += (y_ & 7) * 2;

                            x_ = 7 - ((x_ + 7) & 7);

                            color = ((*data >> x_) & 1)
                                    | ((((*(data + 1)) >> x_) << 1) & 2);

                            bg_color0 = (color == 0);

                            color = SGB_GetPixelColor(x, y, bg_pal[color]);
                            window_draw = true;
                        }
                    }
                }
            }

            if ((lcd_reg & (1 << 0)) && (!window_draw)) // BG
            {
                u32 x_ = x + scx_reg;
                u32 y_ = y + scy_reg;

                u32 tile = bgtilemap[(((y_ >> 3) & 31) * 32)
                                     + ((x_ >> 3) & 31)];

                if (!(lcd_reg & (1 << 4))) // If tile base is 0x8800
                {
                    if (tile & (1 << 7))
                        tile &= 0x7F;
                    else
                        tile += 128;
                }

                u8 *data = &tiledata[tile << 4];

                data += (y_ & 7) << 1;

                x_ = 7 - (x_ & 7);

                color = ((*data >> x_) & 1)
                        | ((((*(data + 1)) >> x_) << 1) & 2);

                bg_color0 = (color == 0);

                color = SGB_GetPixelColor(x, y, bg_pal[color]);
            }

            gb_framebuffer[gb_cur_fb][base_index + x + 48] = color;
            gb_framebuffer_bgcolor0[x] = bg_color0;
        }

        if (increase_win)
            window_current_line++;

        // If sprites are enabled, draw the ones visible this scanline
        // Note: The BG transparent color is bg_pal[0]
        if (lcd_reg & (1 << 1))
        {
            s32 spriteheight = 8 << ((lcd_reg & (1 << 2)) != 0);
            _GB_OAM_ *GB_OAM = (void *)mem->ObjAttrMem;
            _GB_OAM_ENTRY_ *GB_Sprite;

            // For 8x16 sprites, last bit is ignored
            u32 tilemask = ((spriteheight == 16) ? 0xFE : 0xFF);

            u32 spritesdrawn = 0; // For the 10-sprites-per-scanline limit
            u32 drawsprite[40];

            for (int a = 0; a < 40; a++)
            {
                GB_Sprite = &GB_OAM->Sprite[a];

                drawsprite[a] = 0;

                s32 real_y = GB_Sprite->Y - 16;

                if ((real_y <= y) && ((real_y + spriteheight) > y))
                {
                    if (spritesdrawn < 10)
                    {
                        drawsprite[a] = 1;
                        spritesdrawn++;
                    }
                }
            }

            for (int a = 39; a >= 0; a--)
            {
                // TODO: Fix.
                // When sprites with different x coordinate values overlap, the
                // one with the smaller x coordinate (closer to the left) will
                // have priority and appear above any others. This applies in
                // Non CGB Mode only.

                if (!drawsprite[a])
                    continue;

                GB_Sprite = &GB_OAM->Sprite[a];

                s32 real_y = (GB_Sprite->Y - 16);

                if ((real_y <= y) && ((real_y + spriteheight) > y))
                {
                    u32 tile = GB_Sprite->Tile & tilemask;

                    u8 *data = &mem->VideoRAM[tile << 4];

                    // Flip Y
                    if (GB_Sprite->Info & (1 << 6))
                        data += (spriteheight - y + real_y - 1) * 2;
                    else
                        data += (y - real_y) * 2;

                    // Let's draw the sprite...
                    s32 real_x = GB_Sprite->X - 8;
                    for (int x__ = 0; x__ < 8; x__++)
                    {
                        u32 color = ((*data >> x__) & 1)
                                    | ((((*(data + 1)) >> x__) << 1) & 2);

                        if (color != 0) // Color 0 is transparent
                        {
                            s32 x_ = real_x;

                            // Flip X
                            if (GB_Sprite->Info & (1 << 5))
                                x_ = x_ + x__;
                            else
                                x_ += 7 - x__;

                            if ((x_ >= 0) && (x_ < 160))
                            {
                                if (GB_Sprite->Info & (1 << 4))
                                {
                                    color = SGB_GetPixelColor(x_, y, spr_pal1[color]);
                                }
                                else
                                {
                                    color = SGB_GetPixelColor(x_, y, spr_pal0[color]);
                                }

                                // If BG has priority and it is enabled...
                                if ((GB_Sprite->Info & (1 << 7))
                                    && (lcd_reg & (1 << 0)))
                                {
                                    if (gb_framebuffer_bgcolor0[x_])
                                    {
                                        gb_framebuffer[gb_cur_fb]
                                                      [base_index + x_ + 48] = color;
                                    }
                                }
                                else
                                {
                                    gb_framebuffer[gb_cur_fb]
                                                  [base_index + x_ + 48] = color;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (int x = 0; x < 160; x++)
        {
            gb_framebuffer[gb_cur_fb][base_index + x + 48] = GB_RGB(31, 31, 31);
        }
    }

    if (y == 143)
    {
        SGB_ScreenDrawBorderInside();
        gb_cur_fb ^= 1;
    }
}

// -------------------------------------------------------------
// -------------------------------------------------------------
//                      DRAW TO FRAMEBUFFER
// -------------------------------------------------------------
// -------------------------------------------------------------

int GB_Screen_Init(void)
{
    memset(gb_framebuffer, 0, sizeof(gb_framebuffer));
    return 0;
}

static void GB_Screen_WritePixel(char *buffer, int x, int y,
                                 int r, int g, int b)
{
    u8 *p = (u8 *)buffer + (y * 160 + x) * 3;
    *p++ = r;
    *p++ = g;
    *p = b;
}

static void GB_Screen_WritePixelSGB(char *buffer, int x, int y,
                                    int r, int g, int b)
{
    u8 *p = (u8 *)buffer + (y * (160 + 96) + x) * 3;
    *p++ = r;
    *p++ = g;
    *p = b;
}

static void gb_scr_writebuffer_sgb(char *buffer)
{
    int last_fb = gb_cur_fb ^ 1;
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 224; j++)
        {
            int data = gb_framebuffer[last_fb][j * 256 + i];
            int r = data & 0x1F;
            int g = (data >> 5) & 0x1F;
            int b = (data >> 10) & 0x1F;
            GB_Screen_WritePixelSGB(buffer, i, j, r << 3, g << 3, b << 3);
        }
    }
}

static void gb_scr_writebuffer_dmg_cgb(char *buffer)
{
    int last_fb = gb_cur_fb ^ 1;
    for (int i = 0; i < 160; i++)
    {
        for (int j = 0; j < 144; j++)
        {
            int data = gb_framebuffer[last_fb][j * 256 + i];
            int r = data & 0x1F;
            int g = (data >> 5) & 0x1F;
            int b = (data >> 10) & 0x1F;
            GB_Screen_WritePixel(buffer, i, j, r << 3, g << 3, b << 3);
        }
    }
}

static void gb_scr_writebuffer_dmg_cgb_blur(char *buffer)
{
    for (int i = 0; i < 160; i++)
    {
        for (int j = 0; j < 144; j++)
        {
            int data1 = gb_framebuffer[0][j * 256 + i];
            int r1 = data1 & 0x1F;
            int g1 = (data1 >> 5) & 0x1F;
            int b1 = (data1 >> 10) & 0x1F;

            int data2 = gb_framebuffer[1][j * 256 + i];
            int r2 = data2 & 0x1F;
            int g2 = (data2 >> 5) & 0x1F;
            int b2 = (data2 >> 10) & 0x1F;
            int r = (r1 + r2) << 2;
            int g = (g1 + g2) << 2;
            int b = (b1 + b2) << 2;
            GB_Screen_WritePixel(buffer, i, j, r, g, b);
        }
    }
}

// Real colors:
// R = ((r * 13 + g * 2 + b) >> 1)
// G = ((g * 3 + b) << 1)
// B = ((r * 3 + g * 2 + b * 11) >> 1)

static void gb_scr_writebuffer_dmg_cgb_realcolors(char *buffer)
{
    int last_fb = gb_cur_fb ^ 1;
    for (int i = 0; i < 160; i++)
    {
        for (int j = 0; j < 144; j++)
        {
            int data = gb_framebuffer[last_fb][j * 256 + i];
            int r = data & 0x1F;
            int g = (data >> 5) & 0x1F;
            int b = (data >> 10) & 0x1F;

            int _r = ((r * 13 + g * 2 + b) >> 1);
            int _g = (g * 3 + b) << 1;
            int _b = ((r * 3 + g * 2 + b * 11) >> 1);
            GB_Screen_WritePixel(buffer, i, j, _r, _g, _b);
        }
    }
}

static void gb_scr_writebuffer_dmg_cgb_blur_realcolors(char *buffer)
{
    for (int i = 0; i < 160; i++)
    {
        for (int j = 0; j < 144; j++)
        {
            int data1 = gb_framebuffer[0][j * 256 + i];
            int r1 = data1 & 0x1F;
            int g1 = (data1 >> 5) & 0x1F;
            int b1 = (data1 >> 10) & 0x1F;
            int data2 = gb_framebuffer[1][j * 256 + i];
            int r2 = data2 & 0x1F;
            int g2 = (data2 >> 5) & 0x1F;
            int b2 = (data2 >> 10) & 0x1F;
            int r = (r1 + r2) >> 1;
            int g = (g1 + g2) >> 1;
            int b = (b1 + b2) >> 1;
            int _r = ((r * 13 + g * 2 + b) >> 1);
            int _g = (g * 3 + b) << 1;
            int _b = ((r * 3 + g * 2 + b * 11) >> 1);
            GB_Screen_WritePixel(buffer, i, j, _r, _g, _b);
        }
    }
}

typedef void (*draw_to_buf_fn)(char *);

void GB_Screen_WriteBuffer_24RGB(char *buffer)
{
    draw_to_buf_fn draw_fn = NULL;

    if ((GameBoy.Emulator.HardwareType == HW_SGB)
        || (GameBoy.Emulator.HardwareType == HW_SGB2))
    {
        draw_fn = &gb_scr_writebuffer_sgb;
    }
    else if ((GameBoy.Emulator.HardwareType == HW_GBA)
             || (GameBoy.Emulator.HardwareType == HW_GBA_SP))
    {
        if (gb_blur)
            draw_fn = &gb_scr_writebuffer_dmg_cgb_blur;
        else
            draw_fn = &gb_scr_writebuffer_dmg_cgb;
    }
    else
    {
        if (gb_blur)
        {
            if (gb_realcolors)
                draw_fn = &gb_scr_writebuffer_dmg_cgb_blur_realcolors;
            else
                draw_fn = &gb_scr_writebuffer_dmg_cgb_blur;
        }
        else
        {
            if (gb_realcolors)
                draw_fn = &gb_scr_writebuffer_dmg_cgb_realcolors;
            else
                draw_fn = &gb_scr_writebuffer_dmg_cgb;
        }
    }

    if (GameBoy.Emulator.rumble)
    {
        int rand_ = rand();
        int mov_x = (rand_ % 3) - 1;
        int mov_y = ((rand_ >> 8) % 3) - 1;

        int w, h;

        if ((GameBoy.Emulator.HardwareType == HW_SGB)
            || (GameBoy.Emulator.HardwareType == HW_SGB2))
        {
            w = 256;
            h = 224;
        }
        else
        {
            w = 160;
            h = 144;
        }

        char *buf = malloc(w * h * 3);
        if (buf == NULL)
            return;
        draw_fn(buf);

        for (int j = 0; j < h; j++)
        {
            for (int i = 0; i < w; i++)
            {
                int y_dst = j + mov_y;
                if ((y_dst >= 0) && (y_dst < h))
                {
                    int x_dst = i + mov_x;

                    if ((x_dst >= 0) && (x_dst < w))
                    {
                        int dst = (y_dst * w + x_dst) * 3;
                        buffer[dst + 0] = buf[(j * w + i) * 3 + 0];
                        buffer[dst + 1] = buf[(j * w + i) * 3 + 1];
                        buffer[dst + 2] = buf[(j * w + i) * 3 + 2];
                    }
                }
            }
        }

        free(buf);
    }
    else
    {
        draw_fn(buffer);
    }
}

// -------------------------------------------------------------
// -------------------------------------------------------------
//                      SCREENSHOTS
// -------------------------------------------------------------
// -------------------------------------------------------------

void GB_Screenshot(void)
{
    char *name = FU_GetNewTimestampFilename("gb_screenshot");

    int width, height;

    if (GameBoy.Emulator.SGBEnabled)
    {
        width = 256;
        height = 224;
    }
    else
    {
        width = 160;
        height = 144;
    }

    u32 *buf_temp = calloc(width * height * 4, 1);
    int last_fb = gb_cur_fb ^ 1;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            u32 data = gb_framebuffer[last_fb][y * 256 + x];
            buf_temp[y * width + x] = ((data & 0x1F) << 3)
                                      | ((((data >> 5) & 0x1F) << 3) << 8)
                                      | ((((data >> 10) & 0x1F) << 3) << 16);
        }
    }
    Save_PNG(name, width, height, buf_temp, 0);
    free(buf_temp);
}
