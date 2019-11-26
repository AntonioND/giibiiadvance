// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../debug_utils.h"
#include "../general_utils.h"

#include "debug.h"
#include "gameboy.h"
#include "gb_main.h"
#include "general.h"
#include "sgb.h"
#include "video.h"

extern _GB_CONTEXT_ GameBoy;

_SGB_INFO_ SGBInfo;

static u32 sgb_screenbuffer[4 * 1024];

#if 0
const u32 sgb_defaultpalettes[32][16] = {
    {0x330a, 0x5684, 0x6d32, 0x7ef3, 0x330a, 0x5684, 0x6d32, 0x7ef3,
        0x330a, 0x5684, 0x6d32, 0x7ef3, 0x330a, 0x5684, 0x6d32, 0x7ef3},
    {0x0000, 0x3548, 0x4dae, 0x6f63, 0x0000, 0x3548, 0x4dae, 0x6f63,
        0x0000, 0x3548, 0x4dae, 0x6f63, 0x0000, 0x3548, 0x4dae, 0x6f63},
    {0x7399, 0x6786, 0x5f2a, 0x7c7f, 0x7399, 0x6786, 0x5f2a, 0x7c7f,
        0x7399, 0x6786, 0x5f2a, 0x7c7f, 0x7399, 0x6786, 0x5f2a, 0x7c7f},
    {0x2b00, 0x7c00, 0x0c32, 0x7ff5, 0x2b00, 0x7c00, 0x0c32, 0x7ff5,
        0x2b00, 0x7c00, 0x0c32, 0x7ff5, 0x2b00, 0x7c00, 0x0c32, 0x7ff5},
    {0x6b84, 0x5a22, 0x787e, 0x7f6d, 0x6b84, 0x5a22, 0x787e, 0x7f6d,
        0x6b84, 0x5a22, 0x787e, 0x7f6d, 0x6b84, 0x5a22, 0x787e, 0x7f6d},
    {0x0048, 0x5400, 0x1e2a, 0x6eff, 0x0048, 0x5400, 0x1e2a, 0x6eff,
        0x0048, 0x5400, 0x1e2a, 0x6eff, 0x0048, 0x5400, 0x1e2a, 0x6eff},
    {0x7ffa, 0x7bc0, 0x00b7, 0x000a, 0x7ffa, 0x7bc0, 0x00b7, 0x000a,
        0x7ffa, 0x7bc0, 0x00b7, 0x000a, 0x7ffa, 0x7bc0, 0x00b7, 0x000a},
    {0x3300, 0x0440, 0x7fb1, 0x7ee7, 0x3300, 0x0440, 0x7fb1, 0x7ee7,
        0x3300, 0x0440, 0x7fb1, 0x7ee7, 0x3300, 0x0440, 0x7fb1, 0x7ee7},
    {0x0000, 0x53c0, 0x0e32, 0x3e65, 0x0000, 0x53c0, 0x0e32, 0x3e65,
        0x0000, 0x53c0, 0x0e32, 0x3e65, 0x0000, 0x53c0, 0x0e32, 0x3e65},
    {0x281a, 0x7d80, 0x7eea, 0x7fff, 0x281a, 0x7d80, 0x7eea, 0x7fff,
        0x281a, 0x7d80, 0x7eea, 0x7fff, 0x281a, 0x7d80, 0x7eea, 0x7fff},
    {0x5299, 0x7997, 0x5e31, 0x7c7f, 0x5299, 0x7997, 0x5e31, 0x7c7f,
        0x5299, 0x7997, 0x5e31, 0x7c7f, 0x5299, 0x7997, 0x5e31, 0x7c7f},
    {0x000a, 0x7d80, 0x03e0, 0x7fe5, 0x000a, 0x7d80, 0x03e0, 0x7fe5,
        0x000a, 0x7d80, 0x03e0, 0x7fe5, 0x000a, 0x7d80, 0x03e0, 0x7fe5},
    {0x2208, 0x5106, 0x25a7, 0x7e61, 0x2208, 0x5106, 0x25a7, 0x7e61,
        0x2208, 0x5106, 0x25a7, 0x7e61, 0x2208, 0x5106, 0x25a7, 0x7e61},
    {0x6000, 0x1400, 0x7d2a, 0x2fff, 0x6000, 0x1400, 0x7d2a, 0x2fff,
        0x6000, 0x1400, 0x7d2a, 0x2fff, 0x6000, 0x1400, 0x7d2a, 0x2fff},
    {0x0300, 0x1fa1, 0x1d42, 0x5bbc, 0x0300, 0x1fa1, 0x1d42, 0x5bbc,
        0x0300, 0x1fa1, 0x1d42, 0x5bbc, 0x0300, 0x1fa1, 0x1d42, 0x5bbc},
    {0x0000, 0x39ce, 0x77bd, 0x7fff, 0x0000, 0x39ce, 0x77bd, 0x7fff,
        0x0000, 0x39ce, 0x77bd, 0x7fff, 0x0000, 0x39ce, 0x77bd, 0x7fff},
    {0x3246, 0x7cd4, 0x3863, 0x7d79, 0x3246, 0x7cd4, 0x3863, 0x7d79,
        0x3246, 0x7cd4, 0x3863, 0x7d79, 0x3246, 0x7cd4, 0x3863, 0x7d79},
    {0x0108, 0x0140, 0x1c24, 0x6f63, 0x0108, 0x0140, 0x1c24, 0x6f63,
        0x0108, 0x0140, 0x1c24, 0x6f63, 0x0108, 0x0140, 0x1c24, 0x6f63},
    {0x109a, 0x03bf, 0x7ffe, 0x1eb3, 0x109a, 0x03bf, 0x7ffe, 0x1eb3,
        0x109a, 0x03bf, 0x7ffe, 0x1eb3, 0x109a, 0x03bf, 0x7ffe, 0x1eb3},
    {0x0000, 0x4260, 0x1ebe, 0x3ffd, 0x0000, 0x4260, 0x1ebe, 0x3ffd,
        0x0000, 0x4260, 0x1ebe, 0x3ffd, 0x0000, 0x4260, 0x1ebe, 0x3ffd},
    {0x2a4e, 0x37c4, 0x1db6, 0x7fe3, 0x2a4e, 0x37c4, 0x1db6, 0x7fe3,
        0x2a4e, 0x37c4, 0x1db6, 0x7fe3, 0x2a4e, 0x37c4, 0x1db6, 0x7fe3},
    {0x0842, 0x7d60, 0x7edf, 0x7bd3, 0x0842, 0x7d60, 0x7edf, 0x7bd3,
        0x0842, 0x7d60, 0x7edf, 0x7bd3, 0x0842, 0x7d60, 0x7edf, 0x7bd3},
    {0x7000, 0x4d9c, 0x7fff, 0x1b6a, 0x7000, 0x4d9c, 0x7fff, 0x1b6a,
        0x7000, 0x4d9c, 0x7fff, 0x1b6a, 0x7000, 0x4d9c, 0x7fff, 0x1b6a},
    {0x4300, 0x4a38, 0x7a7c, 0x1fe5, 0x4300, 0x4a38, 0x7a7c, 0x1fe5,
        0x4300, 0x4a38, 0x7a7c, 0x1fe5, 0x4300, 0x4a38, 0x7a7c, 0x1fe5},
    {0x001e, 0x2c0b, 0x7abf, 0x3eb6, 0x001e, 0x2c0b, 0x7abf, 0x3eb6,
        0x001e, 0x2c0b, 0x7abf, 0x3eb6, 0x001e, 0x2c0b, 0x7abf, 0x3eb6},
    {0x6210, 0x0bdc, 0x5ca6, 0x3eef, 0x6210, 0x0bdc, 0x5ca6, 0x3eef,
        0x6210, 0x0bdc, 0x5ca6, 0x3eef, 0x6210, 0x0bdc, 0x5ca6, 0x3eef},
    {0x4000, 0x64a7, 0x6cab, 0x7ce7, 0x4000, 0x64a7, 0x6cab, 0x7ce7,
        0x4000, 0x64a7, 0x6cab, 0x7ce7, 0x4000, 0x64a7, 0x6cab, 0x7ce7},
    {0x4092, 0x4ade, 0x2673, 0x7ffd, 0x4092, 0x4ade, 0x2673, 0x7ffd,
        0x4092, 0x4ade, 0x2673, 0x7ffd, 0x4092, 0x4ade, 0x2673, 0x7ffd},
    {0x008c, 0x7b51, 0x1ebe, 0x7f75, 0x008c, 0x7b51, 0x1ebe, 0x7f75,
        0x008c, 0x7b51, 0x1ebe, 0x7f75, 0x008c, 0x7b51, 0x1ebe, 0x7f75},
    {0x7000, 0x0405, 0x6c3b, 0x756b, 0x7000, 0x0405, 0x6c3b, 0x756b,
        0x7000, 0x0405, 0x6c3b, 0x756b, 0x7000, 0x0405, 0x6c3b, 0x756b},
    {0x0026, 0x5100, 0x749a, 0x34f8, 0x0026, 0x5100, 0x749a, 0x34f8,
        0x0026, 0x5100, 0x749a, 0x34f8, 0x0026, 0x5100, 0x749a, 0x34f8},
    {0x0954, 0x0622, 0x747a, 0x7ff3, 0x0954, 0x0622, 0x747a, 0x7ff3,
        0x0954, 0x0622, 0x747a, 0x7ff3, 0x0954, 0x0622, 0x747a, 0x7ff3}
};
#endif

void SGB_RenderScreenBuffer(void)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (mem->IO_Ports[BGP_REG - 0xFF00] != 0xE4)
        Debug_DebugMsgArg("SGB data transfer, BGP != 0xE4");

    u32 lcd_reg = mem->IO_Ports[LCDC_REG - 0xFF00];

    u8 *tiledata = (lcd_reg & (1 << 4)) ?
                   &mem->VideoRAM[0x0000] : &mem->VideoRAM[0x0800];
    u8 *bgtilemap = (lcd_reg & (1<<3)) ?
                    &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];
    u32 *final_buffer = sgb_screenbuffer;

    u32 tile_count = 0;
    // Draw BG
    for (int y = 0; y < 18; y ++)
    {
        for (int x = 0; x < 20; x ++)
        {
            u32 tile = bgtilemap[(y * 32) + x];

            if (!(lcd_reg & (1 << 4))) // If tile base is 0x8800
            {
                if (tile & (1 << 7))
                    tile &= 0x7F;
                else
                    tile += 128;
            }

            u8 *data = &tiledata[tile << 4];

            for (int i = 0; i < 16; i++)
            {
                *final_buffer = (u32)*data;
                final_buffer++;
                data++;
            }

            tile_count++;

            if (tile_count > 255) // 4 KB reached
                return;
        }
    }
}

void SGB_Init(void)
{
    GameBoy.Emulator.wait_cycles = 0;
    SGBInfo.sending = 0;
    SGBInfo.disable_sgb = 0;
    SGBInfo.delay = 0;

    SGBInfo.attracion_mode = 1;
    SGBInfo.test_speed_mode = 0;

    SGBInfo.multiplayer = 0;
    SGBInfo.read_joypad = 0;
    SGBInfo.current_joypad = 0;

    SGBInfo.freeze_screen = 0;

    SGBInfo.sgb_bank0_ram = NULL; // Only allocate memory if used

    const u32 gbpalettes[4] = {
        GB_RGB(31, 31, 31), GB_RGB(21, 21, 21), GB_RGB(10, 10, 10),
        GB_RGB(0, 0, 0)
    };

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
            SGBInfo.palette[i][j] = gbpalettes[j];
    }
    for (int i = 4; i < SGB_NUM_PALETTES; i++)
    {
        for (int j = 0; j < 16; j++)
            SGBInfo.palette[i][j] = gbpalettes[j >> 2];
    }

    for (int i = 0; i < 512; i++)
    {
        for (int j = 0; j < 4; j++)
            SGBInfo.snes_palette[i][j] = gbpalettes[j];
    }

    SGBInfo.curr_ATF = 0;

    for (int i = 0; i < 0x2D; i++)
        memset(&SGBInfo.ATF_list[i], 0, 20 * 18 * 4);

    memset(SGBInfo.tile_data, 0, ((8 * 8 * 4) / 8) * 256 * 4);
    memset(SGBInfo.tile_map, 0, 32 * 32 * 4);

    memset(SGBInfo.data, 0, sizeof(SGBInfo.data));
}

// For delay between frames (not used for now)
//void SGB_Clock(int clocks)
//{
//    if (GameBoy.Emulator.SGBEnabled == 1)
//        if (SGBInfo.delay > 0)
//            SGBInfo.delay -= clocks;
//}

void SGB_End(void)
{
    if (SGBInfo.sgb_bank0_ram)
    {
        free(SGBInfo.sgb_bank0_ram);
        SGBInfo.sgb_bank0_ram = NULL;
    }
}

//------------------------------------------------------------------------------

void SGB_SetBackdrop(u32 rgb)
{
    for (int i = 0; i < 8; i++)
        SGBInfo.palette[i][0] = rgb;

    SGB_ScreenDrawBorder();
}

static void SGB_PALNN(u32 pal1, u32 pal2)
{
    SGB_SetBackdrop(SGBInfo.data[0][1] | (SGBInfo.data[0][2] << 8));

    SGBInfo.palette[pal1][1] = SGBInfo.data[0][3] | (SGBInfo.data[0][4] << 8);
    SGBInfo.palette[pal1][2] = SGBInfo.data[0][5] | (SGBInfo.data[0][6] << 8);
    SGBInfo.palette[pal1][3] = SGBInfo.data[0][7] | (SGBInfo.data[0][8] << 8);

    SGBInfo.palette[pal2][1] = SGBInfo.data[0][9] | (SGBInfo.data[0][10] << 8);
    SGBInfo.palette[pal2][2] = SGBInfo.data[0][11] | (SGBInfo.data[0][12] << 8);
    SGBInfo.palette[pal2][3] = SGBInfo.data[0][13] | (SGBInfo.data[0][14] << 8);
}

static void SGB_ATTR_BLK(void)
{
    u32 sets = SGBInfo.data[0][1];
    if (sets > 0x12)
        sets = 0x12;

    u32 curbyte = 2;
    u32 curpacket = 0;

    for (int i = 0; i < sets; i++)
    {
        u32 ctrl = SGBInfo.data[curpacket][curbyte];
        curbyte ++;
        if (curbyte == 16)
        {
            curbyte = 0;
            curpacket++;
        }
        u32 palettes = SGBInfo.data[curpacket][curbyte];
        curbyte ++;
        if (curbyte == 16)
        {
            curbyte = 0;
            curpacket++;
        }
        u32 x1 = SGBInfo.data[curpacket][curbyte];
        curbyte ++;
        if (curbyte == 16)
        {
            curbyte = 0;
            curpacket++;
        }
        u32 y1 = SGBInfo.data[curpacket][curbyte];
        curbyte ++;
        if (curbyte == 16)
        {
            curbyte = 0;
            curpacket++;
        }
        u32 x2 = SGBInfo.data[curpacket][curbyte];
        curbyte ++;
        if (curbyte == 16)
        {
            curbyte = 0;
            curpacket++;
        }
        u32 y2 = SGBInfo.data[curpacket][curbyte];
        curbyte ++;
        if (curbyte == 16)
        {
            curbyte = 0;
            curpacket++;
        }

        //Debug_DebugMsgArg("B:%d P:%d 1:%d,%d 2:%d,%d", curbyte, curpacket,
        //                  x1, y1, x2, y2);

        if ((ctrl & (1 << 1)) || ((ctrl & 0x05) == 0x01) ||
            ((ctrl & 0x05) == 0x04)) // Line
        {
            u32 pal;

            // Only change line to its own color if inside and outside enabled
            if ((ctrl & 0x05) == 0x05)
            {
                pal = (palettes >> 2) & 3;
            }
            else
            {
                if (ctrl & (1 << 0))
                    pal = palettes & 3;
                else
                    pal = (palettes >> 4) & 3;
            }

            // Draw lines...
            for (int x = x1; x <= x2; x ++)
            {
                SGBInfo.ATF_list[SGBInfo.curr_ATF][(y1 * 20) + x] = pal;
                SGBInfo.ATF_list[SGBInfo.curr_ATF][(y2 * 20) + x] = pal;
            }
            for (int y = y1; y <= y2; y ++)
            {
                SGBInfo.ATF_list[SGBInfo.curr_ATF][(y * 20) + x1] = pal;
                SGBInfo.ATF_list[SGBInfo.curr_ATF][(y * 20) + x2] = pal;
            }
        }
        if (ctrl & (1 << 0)) // Inside
        {
            u32 pal = palettes & 3;
            for (int x = x1 + 1; x < x2; x ++)
            {
                for (int y = y1 + 1; y < y2; y ++)
                {
                    SGBInfo.ATF_list[SGBInfo.curr_ATF][(y * 20) + x] = pal;
                }
            }
        }
        if (ctrl & (1 << 2)) // Outside
        {
            u32 pal = (palettes >> 4) & 3;
            for (int x = 0; x < 20; x ++)
            {
                for (int y = 0; y < 18; y ++)
                {
                    if ((x < x1 || x > x2) && (y < y1 || y > y2))
                        SGBInfo.ATF_list[SGBInfo.curr_ATF][(y * 20) + x] = pal;
                }
            }
        }
    }
}

static void SGB_ATTR_LIN(void)
{
    u32 sets = SGBInfo.data[0][1];
    if (sets > 0x6E)
        sets = 0x6E;

    u32 curbyte = 2;
    u32 curpacket = 0;

    for (int i = 0; i < sets; i++)
    {
        u32 data = SGBInfo.data[curpacket][curbyte];
        curbyte++;
        if (curbyte == 16)
        {
            curbyte = 0;
            curpacket++;
        }

        u32 coord = data & 0x1F;
        u32 pal = (data >> 5) & 0x03;
        u32 hor = data & (1 << 7);

        if (hor)
        {
            if (coord > 17)
                coord = 17;

            for (int x = 0; x < 20; x ++)
            {
                SGBInfo.ATF_list[SGBInfo.curr_ATF][(coord * 20) + x] = pal;
            }
        }
        else
        {
            if (coord > 19)
                coord = 19;

            for (int y = 0; y < 18; y ++)
            {
                SGBInfo.ATF_list[SGBInfo.curr_ATF][(y * 20) + coord] = pal;
            }
        }
    }
}

static void SGB_ATTR_DIV(void)
{
    u32 pal_line = (SGBInfo.data[0][1] >> 4) & 3;

    if (SGBInfo.data[0][1] & (1 << 6)) // Horizontal division
    {
        u32 pal_down = SGBInfo.data[0][1] & 3;
        u32 pal_up = (SGBInfo.data[0][1] >> 2) & 3;

        u32 y_ = SGBInfo.data[0][2];

        for (int x = 0; x < 20; x++)
        {
            for (int y = 0; y < 18; y++)
            {
                u32 pal = pal_line;
                if (y_ < y)
                    pal = pal_down;
                else if (y_ > y)
                    pal = pal_up;

                SGBInfo.ATF_list[SGBInfo.curr_ATF][(y * 20) + x] = pal;
            }
        }
    }
    else // Vertical division
    {
        u32 pal_right = SGBInfo.data[0][1] & 3;
        u32 pal_left = (SGBInfo.data[0][1] >> 2) & 3;

        u32 x_ = SGBInfo.data[0][2];

        for (int x = 0; x < 20; x ++)
        {
            for (int y = 0; y < 18; y ++)
            {
                u32 pal = pal_line;
                if (x_ < x)
                    pal = pal_right;
                else if (x_ > x)
                    pal = pal_left;

                SGBInfo.ATF_list[SGBInfo.curr_ATF][(y * 20) + x] = pal;
            }
        }
    }
}

static void SGB_ATTR_CHR(void)
{
    u32 x = SGBInfo.data[0][1];
    u32 y = SGBInfo.data[0][2];
    u32 sets = SGBInfo.data[0][3] | (SGBInfo.data[0][4] << 8);
    if (sets > 360)
        sets = 360;

    u32 direction = SGBInfo.data[0][5]; // 0 = left to right, 1 = top to bottom

    u32 curbyte = 6;
    u32 curpacket = 0;
    u32 bit_shift = 6;

    for (int i = 0; i < sets; i++)
    {
        if (direction == 0)
        {
            x++;
            if (x > 19)
            {
                x = 0;
                y++;
                if (y > 17)
                    y = 0;
            }
        }
        else
        {
            y++;
            if (y > 17)
            {
                y = 0;
                x++;
                if (x > 19)
                    x = 0;
            }
        }

        u32 data = (SGBInfo.data[curpacket][curbyte] >> bit_shift) & 3;

        SGBInfo.ATF_list[SGBInfo.curr_ATF][(y * 20) + x] = data;

        if (bit_shift == 0)
        {
            bit_shift = 6;

            curbyte++;
            if (curbyte == 16)
            {
                curbyte = 0;
                curpacket++;
            }
        }
        else
        {
            bit_shift -= 2;
        }
    }
}

static void SGB_PAL_SET(void)
{
    u32 pal0 = SGBInfo.data[0][1] | (SGBInfo.data[0][2] << 8);
    u32 pal1 = SGBInfo.data[0][3] | (SGBInfo.data[0][4] << 8);
    u32 pal2 = SGBInfo.data[0][5] | (SGBInfo.data[0][6] << 8);
    u32 pal3 = SGBInfo.data[0][7] | (SGBInfo.data[0][8] << 8);

    for (int i = 0; i < 4; i++)
    {
        SGBInfo.palette[0][i] = SGBInfo.snes_palette[pal0][i];
        SGBInfo.palette[1][i] = SGBInfo.snes_palette[pal1][i];
        SGBInfo.palette[2][i] = SGBInfo.snes_palette[pal2][i];
        SGBInfo.palette[3][i] = SGBInfo.snes_palette[pal3][i];
    }

    SGB_SetBackdrop(SGBInfo.palette[0][0]); // [0][?]

    u32 attr = SGBInfo.data[0][9];

    if (attr & (1 << 6))
        SGBInfo.freeze_screen = 0;

    if (attr & (1 << 7))
    {
        SGBInfo.curr_ATF = attr & 0x3F;
        if (SGBInfo.curr_ATF > 0x2C)
            SGBInfo.curr_ATF = 0x2C;
    }
}

static void SGB_PAL_TRN(void)
{
    SGB_RenderScreenBuffer();

    for (int i = 0; i < 512; i++)
    {
        for (int j = 0; j < 8; j += 2)
        {
           SGBInfo.snes_palette[i][j >> 1] =
                            (u32)sgb_screenbuffer[(i * 8) + j]
                            | ((u32)sgb_screenbuffer[(i * 8) + j + 1] << 8);
        }
    }
}

static void SGB_DATA_SND(void)
{
    u32 numbytes = SGBInfo.data[0][4];
    if (numbytes > 0xB)
        numbytes = 0xB;
    u32 address = SGBInfo.data[0][1] | (SGBInfo.data[0][2] << 8);
    u32 bank = SGBInfo.data[0][3];

    if (bank == 0)
    {
        if (address > 0x1FFF)
        {
            Debug_DebugMsgArg("SGB DATA_SND warning - %04xh bank 0, %d Bytes",
                              SGBInfo.data[0][1] | (SGBInfo.data[0][2] << 8),
                              SGBInfo.data[0][4]);
            return;
        }

        if (SGBInfo.sgb_bank0_ram == NULL)
            SGBInfo.sgb_bank0_ram = malloc(0x2000);

        for (int i = 0; i < numbytes; i++)
        {
            SGBInfo.sgb_bank0_ram[address + i] = SGBInfo.data[0][5 + i];
        }
    }
    else
    {
        Debug_DebugMsgArg("SGB DATA_SND warning - Wrote to bank %02xh",
                          SGBInfo.data[0][3]);
        return;
    }
}

static void SGB_DATA_TRN(void)
{
    Debug_DebugMsgArg("SGB DATA_TRN warning - Wrote to %04x, bank %02xh",
                      SGBInfo.data[0][1] | (SGBInfo.data[0][2] << 8),
                      SGBInfo.data[0][3]);
    return;
}

static void SGB_CHR_TRN(void)
{
    //if (SGBInfo.data[0][1] & (1 << 1)) // OBJ tiles - no difference?
    SGB_RenderScreenBuffer();

    u32 *dest = SGBInfo.tile_data
              + ((SGBInfo.data[0][1] & (1 << 0)) ?  0x1000 : 0);

    for (int i = 0; i < 0x1000; i++)
        dest[i] = sgb_screenbuffer[i];

    SGB_ScreenDrawBorder();
}

static void SGB_PCT_TRN(void)
{
    u32 *dest = SGBInfo.tile_map;

    SGB_RenderScreenBuffer();

    for (int i = 0; i < 0x800; i += 2)
        dest[i >> 1] = (u32)sgb_screenbuffer[i]
                     | ((u32)sgb_screenbuffer[i + 1] << 8);

    for (int i = 0x800; i < 0x880; i += 2)
    {
        u32 pal = ((i & 0xFF) >> 5) + 4;
        u32 color = (i >> 1) & 0xF;
        SGBInfo.palette[pal][color] = (u32)sgb_screenbuffer[i]
                                    | ((u32)sgb_screenbuffer[i + 1] << 8);
    }

    SGB_ScreenDrawBorder();
}

static void SGB_ATTR_TRN(void)
{
    SGB_RenderScreenBuffer();

    for (int i = 0; i < 0x2D; i++)
    {
        for (int j = 0; j < 20 * 18; j += 4)
        {
            SGBInfo.ATF_list[i][j] =
                    (sgb_screenbuffer[(i * 18 * 5) + (j / 4)] >> 6) & 0x3;
            SGBInfo.ATF_list[i][j + 1] =
                    (sgb_screenbuffer[(i * 18 * 5) + (j / 4)] >> 4) & 0x3;
            SGBInfo.ATF_list[i][j + 2] =
                    (sgb_screenbuffer[(i * 18 * 5) + (j / 4)] >> 2) & 0x3;
            SGBInfo.ATF_list[i][j + 3] =
                    sgb_screenbuffer[(i * 18 * 5) + (j / 4)] & 0x3;
        }
    }
}

static void SGB_OBJ_TRN(void)
{
    if (SGBInfo.data[0][1] & (1 << 0)) // Enable OBJ MODE
    {
        Debug_DebugMsgArg("SGB OBJ_TRN warning - OBJ mode not emulated.");
    }

    if (SGBInfo.data[0][1] & (1 << 1)) // Set palettes
    {
        u32 pal4 = SGBInfo.data[0][2] | (SGBInfo.data[0][3]<<8);
        u32 pal5 = SGBInfo.data[0][4] | (SGBInfo.data[0][5]<<8);
        u32 pal6 = SGBInfo.data[0][6] | (SGBInfo.data[0][7]<<8);
        u32 pal7 = SGBInfo.data[0][8] | (SGBInfo.data[0][9]<<8);

        for (int i = 0; i < 16; i++)
        {
            SGBInfo.palette[4][i] = SGBInfo.snes_palette[pal4 + (i >> 2)][i & 3];
            SGBInfo.palette[5][i] = SGBInfo.snes_palette[pal5 + (i >> 2)][i & 3];
            SGBInfo.palette[6][i] = SGBInfo.snes_palette[pal6 + (i >> 2)][i & 3];
            SGBInfo.palette[7][i] = SGBInfo.snes_palette[pal7 + (i >> 2)][i & 3];
        }
    }
}

static void SGB_ExecuteCommand(void)
{
    //Debug_DebugMsgArg("SGB Command: %02x", SGBInfo.data[0][0] >> 3);

    // Get command...
    switch (SGBInfo.data[0][0] >> 3)
    {
        case 0x00: // PAL01
            SGB_PALNN(0, 1);
            return;
        case 0x01: // PAL23
            SGB_PALNN(2, 3);
            return;
        case 0x02: // PAL03
            SGB_PALNN(0, 3);
            return;
        case 0x03: // PAL12
            SGB_PALNN(1, 2);
            return;
        case 0x04: // ATTR_BLK
            SGB_ATTR_BLK();
            return;
        case 0x05: // ATTR_LIN
            SGB_ATTR_LIN();
            return;
        case 0x06: // ATTR_DIV
            SGB_ATTR_DIV();
            return;
        case 0x07: // ATTR_CHR
            SGB_ATTR_CHR();
            return;
        case 0x08: // SOUND
            //Debug_DebugMsgArg("SGB SOUND command - Not emulated");
            return;
        case 0x09: // SOU_TRN
            //Debug_DebugMsgArg("SGB SOU_TRN command - Not emulated");
            return;
        case 0x0A: // PAL_SET
            SGB_PAL_SET();
            return;
        case 0x0B: // PAL_TRN
            SGB_PAL_TRN();
            return;
        case 0x0C: // ATRC_EN
            SGBInfo.attracion_mode = SGBInfo.data[0][1];
            return;
        case 0x0D: // TEST_EN
            SGBInfo.test_speed_mode = SGBInfo.data[0][1];
            return;
        case 0x0E: // ICON_EN
            if ((SGBInfo.data[0][1] & (1 << 0)) == 0)
            {
                Debug_DebugMsgArg("SGB ICON_EN command - Default palettes\n"
                                  "Not emulated");
                for (int i = 0; i < 4; i++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        SGBInfo.palette[i][j] = GB_GameBoyGetGray(j);
                    }
                }
            }
            if ((SGBInfo.data[0][1] & (1 << 1)) == 0)
            {
                //Debug_DebugMsgArg("SGB ICON_EN command - Set up screen?");
            }
            if (SGBInfo.data[0][1] & (1 << 2))
            {
                SGBInfo.disable_sgb = 1;
                Debug_DebugMsgArg("SGB ICON_EN command - SGB disabled!");
            }
            return;
        case 0x0F: // DATA_SND
            SGB_DATA_SND();
            return;
        case 0x10: // DATA_TRN
            SGB_DATA_TRN();
            return;
        case 0x11: // MLT_REG
            SGBInfo.current_joypad = 0;
            if (SGBInfo.data[0][1] == 0)
            {
                SGBInfo.multiplayer = 0;
            }
            else
            {
                if (SGBInfo.data[0][1] == 1)
                    SGBInfo.multiplayer = 2;
                else if (SGBInfo.data[0][1] == 3)
                    SGBInfo.multiplayer = 4;

                SGBInfo.read_joypad = 0;

                SGBInfo.current_joypad = 1;
            }
            return;
        case 0x12: // JUMP
            Debug_DebugMsgArg("SGB JUMP command - Not emulated");
            return;
        case 0x13: // CHR_TRN
            SGB_CHR_TRN();
            return;
        case 0x14: // PCT_TRN
            SGB_PCT_TRN();
            return;
        case 0x15: // ATTR_TRN
            SGB_ATTR_TRN();
            return;
        case 0x16: // ATTR_SET
            if (SGBInfo.data[0][1] & (1 << 6))
                SGBInfo.freeze_screen = 0;
            SGBInfo.curr_ATF = SGBInfo.data[0][1] & 0x3F;
            if (SGBInfo.curr_ATF > 0x2C)
                SGBInfo.curr_ATF = 0x2C;
            return;
        case 0x17: // MASK_EN
            SGBInfo.freeze_screen = SGBInfo.data[0][1];
            return;

        case 0x18: // OBJ_TRN
            SGB_OBJ_TRN();
            break; // NOT TESTED!!!

        case 0x19: // ???
            //if (SGBInfo.data[0][1] & (1 << 0)) // Enable something?
            return;

        case 0x1E: // BIOS_1 (?)
        case 0x1F: // BIOS_2 (?)
            // SGB rom sends first a 1Eh command, then a 1Fh command. Both are 1
            // packet long and they have the same parameters.
            return;

        default:
            break;
    }

    Debug_DebugMsgArg("Unknown command: %02x, Packets: %02x",
                      SGBInfo.data[0][0] >> 3, SGBInfo.data[0][0] & 0x07);

    for (int i = 0; i < (SGBInfo.data[0][0] & 0x07); i++)
    {
        Debug_DebugMsgArg("%02x%02x %02x%02x %02x%02x %02x%02x "
                          "%02x%02x %02x%02x %02x%02x %02x%02x",
                          SGBInfo.data[i][0], SGBInfo.data[i][1],
                          SGBInfo.data[i][2], SGBInfo.data[i][3],
                          SGBInfo.data[i][4], SGBInfo.data[i][5],
                          SGBInfo.data[i][6], SGBInfo.data[i][7],
                          SGBInfo.data[i][8], SGBInfo.data[i][9],
                          SGBInfo.data[i][10], SGBInfo.data[i][11],
                          SGBInfo.data[i][12], SGBInfo.data[i][13],
                          SGBInfo.data[i][14], SGBInfo.data[i][15]);
    }
}

int SGB_MultiplayerIsEnabled(void)
{
    return SGBInfo.multiplayer;
}

void SGB_WriteP1(u32 value)
{
    if (SGBInfo.disable_sgb)
        return;

    u32 p14 = (value >> 4) & 1;
    u32 p15 = (value >> 5) & 1;

    if (SGBInfo.sending == 0)
    {
        if (SGBInfo.multiplayer != 0)
        {
            if (p14 && p15)
            {
                if ((SGBInfo.read_joypad & 0x30) == 0x30)
                {
                    SGBInfo.current_joypad++;
                    if ((SGBInfo.multiplayer == 2) && (SGBInfo.current_joypad > 1))
                    {
                        SGBInfo.current_joypad = 0;
                    }
                    if( (SGBInfo.multiplayer == 4) && (SGBInfo.current_joypad > 3) )
                    {
                        SGBInfo.current_joypad = 0;
                    }
                    SGBInfo.read_joypad = 0;
                }
                return;
            }
            else if(p14)
            {
                SGBInfo.read_joypad |= 1 << 4;
            }
            else if(p15)
            {
                SGBInfo.read_joypad |= 1 << 5;
            }
        }

        if ((p14 | p15) == 0)
        {
            if (SGBInfo.delay > 0)
                return;

            // Clear everything
            SGBInfo.sending = 1;
            SGBInfo.currbit = 0;
            SGBInfo.currbyte = 0;
            SGBInfo.currpacket = 0;
            SGBInfo.numpackets = SGB_MAX_PACKETS;

            for (int i = 0; i < SGB_MAX_PACKETS; i++)
                for (int j = 0; j < SGB_BYTES_PER_PACKET; j++)
                    SGBInfo.data[i][j] = 0;

            SGBInfo.continue_ = 0;
            return;
        }
    }
    else if(SGBInfo.sending == 1)
    {
        //if (SGBInfo.delay > 0)
        //    return;

        if (SGBInfo.continue_ == 0)
        {
            if ((p14 & p15) == 1)
            {
                SGBInfo.continue_ = 1;
            }
            return;
        }

        SGBInfo.continue_ = 0;

        if (SGBInfo.currbyte == SGB_BYTES_PER_PACKET)
        {
            if (p14 == 0)
            {
                SGBInfo.sending = 0;
                //Debug_DebugMsgArg("Received SGB packet.");
                if (SGBInfo.currpacket == 0)
                {
                    SGBInfo.numpackets = SGBInfo.data[0][0] & 7;
                }
                SGBInfo.currpacket++;

                if (SGBInfo.numpackets == SGBInfo.currpacket)
                {
                    //OK, EXECUTE DATA...
                    //Debug_DebugMsgArg("SGB packet completed!");
                    SGB_ExecuteCommand();
                    //SGBInfo.delay = SGB_PACKET_DELAY;
                }
                //else
                //{
                //    SGBInfo.delay = SGB_PACKET_DELAY;
                //}

                return;
            }
            else
            {
                SGBInfo.sending = 0;
                // Corrupted data...
                Debug_DebugMsgArg("Corrupted SGB packet.");
                return;
            }
        }

        if ((p14 ^ p15) == 0) // If both are 1 or 0...
        {
            SGBInfo.sending = 0;
            // Corrupted data...?
            Debug_DebugMsgArg("SGB warning - Unexpected operation. "
                              "Corrupted SGB packet?");
            return;
        }
        //if (p14 == 0)
        //{
        //    SGBInfo.data[SGBInfo.currpacket][SGBInfo.currbyte] &=
        //                                              ~(1 << SGBInfo.currbit);
        //}
        if (p15 == 0)
        {
            SGBInfo.data[SGBInfo.currpacket][SGBInfo.currbyte] |=
                                                        1 << SGBInfo.currbit;
        }

        SGBInfo.currbit++;
        if (SGBInfo.currbit == 8)
        {
            SGBInfo.currbit = 0;
            SGBInfo.currbyte++;
        }
    }
}

u32 SGB_ReadP1(void)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    u32 p1_reg = mem->IO_Ports[P1_REG - 0xFF00];

    if (SGBInfo.multiplayer == 0)
    {
        // Normal joypad
        u32 result = 0;

        int Keys = GB_Input_Get(0);
        if ((p1_reg & (1 << 5)) == 0) // A-B-SEL-STA
        {
            result |= (Keys & KEY_A) ? JOY_A : 0;
            result |= (Keys & KEY_B) ? JOY_B : 0;
            result |= (Keys & KEY_SELECT) ? JOY_SELECT : 0;
            result |= (Keys & KEY_START) ? JOY_START : 0;
        }
        if ((p1_reg & (1 << 4)) == 0) // PAD
        {
            result |= (Keys & KEY_UP) ? JOY_UP : 0;
            result |= (Keys & KEY_DOWN) ? JOY_DOWN : 0;
            result |= (Keys & KEY_LEFT) ? JOY_LEFT : 0;
            result |= (Keys & KEY_RIGHT) ? JOY_RIGHT : 0;
        }

        result = (~result) & 0x0F;
        result |= p1_reg & 0xF0;
        result |= 0xC0;

        mem->IO_Ports[P1_REG - 0xFF00] = result;

        return result;
    }

    if ((p1_reg & 0x30) == 0x30)
    {
        return 0xFF - SGBInfo.current_joypad;
    }
    else
    {
        u32 result = 0;

        int Keys = GB_Input_Get(SGBInfo.current_joypad);
        if ((p1_reg & (1 << 5)) == 0) // A-B-SEL-STA
        {
            result |= (Keys & KEY_A) ? JOY_A : 0;
            result |= (Keys & KEY_B) ? JOY_B : 0;
            result |= (Keys & KEY_SELECT) ? JOY_SELECT : 0;
            result |= (Keys & KEY_START) ? JOY_START : 0;
        }
        if ((p1_reg & (1 << 4)) == 0) // PAD
        {
            result |= (Keys & KEY_UP) ? JOY_UP : 0;
            result |= (Keys & KEY_DOWN) ? JOY_DOWN : 0;
            result |= (Keys & KEY_LEFT) ? JOY_LEFT : 0;
            result |= (Keys & KEY_RIGHT) ? JOY_RIGHT : 0;
        }

        result = (~result) & 0x0F;
        result |= p1_reg & 0xF0;
        result |= 0xC0;

        mem->IO_Ports[P1_REG - 0xFF00] = result;

        return result;
    }
}
