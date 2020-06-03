// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include <SDL2/SDL.h>

#include "../debug_utils.h"
#include "../file_utils.h"
#include "../font_utils.h"
#include "../general_utils.h"
#include "../png_utils.h"
#include "../window_handler.h"

#include "win_gb_debugger.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gb_core/debug_video.h"
#include "../gb_core/gameboy.h"
#include "../gb_core/general.h"
#include "../gb_core/sgb.h"

//------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------

static int WinIDGB_SGBViewer;

#define WIN_GB_SGBVIEWER_WIDTH  617
#define WIN_GB_SGBVIEWER_HEIGHT 503

static int GB_SGBViewerCreated = 0;

//------------------------------------------------------------------------------

static char gb_sgb_border_buffer[256 * 256 * 3];
static int gb_sgb_border_tilex;
static int gb_sgb_border_tiley;
static char gb_sgb_border_zoomed_tile_buffer[64 * 64 * 3];

static char gb_sgb_tiles_buffer[128 * 128 * 3];
static int gb_sgb_tiles_selected_index;
static int gb_sgb_tiles_selected_pal;
static char gb_sgb_tiles_zoomed_tile_buffer[64 * 64 * 3];

static char gb_sgb_atf_buffer[(160 + 1) * (144 + 1) * 3];
static int gb_sgb_atf_selected_index; // 0 ~ 0x2D-1
static int gb_sgb_atf_selected_x;
static int gb_sgb_atf_selected_y;

static char gb_sgb_pal_buffer[160 * 80 * 3];
static int gb_sgb_pal_selected_pal;
static int gb_sgb_pal_selected_color;

//------------------------------------------------------------------------------

static _gui_console gb_sgbview_border_con;
static _gui_element gb_sgbview_border_textbox;
static _gui_element gb_sgbview_border_groupbox;
static _gui_element gb_sgbview_border_bmp, gb_sgbview_border_zoomed_tile_bmp;
static _gui_element gb_sgbview_border_dump_btn;

static _gui_element gb_sgbview_tiles_groupbox;
static _gui_element gb_sgbview_tiles_bmp;
static _gui_element gb_sgbview_tiles_select_pal_scrollbar;
static _gui_console gb_sgbview_tiles_con;
static _gui_element gb_sgbview_tiles_textbox;
static _gui_element gb_sgbview_tiles_zoomed_tile_bmp;
static _gui_element gb_sgbview_tiles_dump_btn;

static _gui_element gb_sgbview_atf_groupbox;
static _gui_element gb_sgbview_atf_bmp;
static _gui_console gb_sgbview_atf_con;
static _gui_element gb_sgbview_atf_textbox;
static _gui_element gb_sgbview_atf_select_scrollbar;
static _gui_element gb_sgbview_atf_dump_btn;

static _gui_element gb_sgbview_pal_groupbox;
static _gui_element gb_sgbview_pal_bmp;
static _gui_console gb_sgbview_pal_con;
static _gui_element gb_sgbview_pal_textbox;
static _gui_element gb_sgbview_pal_dump_btn;

static _gui_element gb_sgbview_packet_label;
static _gui_console gb_sgbview_packet_con;
static _gui_element gb_sgbview_packet_textbox;

static _gui_element gb_sgbview_otherinfo_label;
static _gui_console gb_sgbview_otherinfo_con;
static _gui_element gb_sgbview_otherinfo_textbox;

static _gui_element *gb_sgbviwer_window_gui_elements[] = {
    &gb_sgbview_border_groupbox,
    &gb_sgbview_border_textbox,
    &gb_sgbview_border_bmp,
    &gb_sgbview_border_zoomed_tile_bmp,
    &gb_sgbview_border_dump_btn,

    &gb_sgbview_tiles_groupbox,
    &gb_sgbview_tiles_bmp,
    &gb_sgbview_tiles_select_pal_scrollbar,
    &gb_sgbview_tiles_textbox,
    &gb_sgbview_tiles_zoomed_tile_bmp,
    &gb_sgbview_tiles_dump_btn,

    &gb_sgbview_atf_groupbox,
    &gb_sgbview_atf_bmp,
    &gb_sgbview_atf_textbox,
    &gb_sgbview_atf_select_scrollbar,
    &gb_sgbview_atf_dump_btn,

    &gb_sgbview_pal_groupbox,
    &gb_sgbview_pal_bmp,
    &gb_sgbview_pal_textbox,
    &gb_sgbview_pal_dump_btn,

    &gb_sgbview_packet_label,
    &gb_sgbview_packet_textbox,

    &gb_sgbview_otherinfo_label,
    &gb_sgbview_otherinfo_textbox,

    NULL
};

static _gui gb_sgbviewer_window_gui = {
    gb_sgbviwer_window_gui_elements,
    NULL,
    NULL
};

//----------------------------------------------------------------

static void rgb16to32(u16 color, int *r, int *g, int *b)
{
    *r = (color & 31) << 3;
    *g = ((color >> 5) & 31) << 3;
    *b = ((color >> 10) & 31) << 3;
}

//----------------------------------------------------------------

static void _win_gb_sgbviewer_draw_border(void)
{
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            u32 info = SGBInfo.tile_map[(j * 32) + i];

            u32 tile = info & 0xFF;
            u32 *tile_ptr = &SGBInfo.tile_data[((8 * 8 * 4) / 8) * tile];

            u32 pal = (info >> 10) & 7; // 4 to 7 (officially 4 to 6)
            if (pal < 4)
                pal += 4;

            //u32 prio = info & (1 << 13); // not used

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

                    int temp = ((y + (j << 3)) * 256) + (x + (i << 3));
                    if (color)
                    {
                        color = SGBInfo.palette[pal][color];
                        int r, g, b;
                        rgb16to32(color, &r, &g, &b);
                        gb_sgb_border_buffer[temp * 3 + 0] = r;
                        gb_sgb_border_buffer[temp * 3 + 1] = g;
                        gb_sgb_border_buffer[temp * 3 + 2] = b;
                    }
                    else
                    {
                        // Check if inside
                        if ((i >= 6) && (i < 26) && (j >= 4) && (j < 23))
                        {
                            int outcolor = (((x + (i << 3)) & 32) ^
                                            ((y + (j << 3)) & 32)) ?
                                                0x80 : 0xB0;
                            gb_sgb_border_buffer[temp * 3 + 0] = outcolor;
                            gb_sgb_border_buffer[temp * 3 + 1] = outcolor;
                            gb_sgb_border_buffer[temp * 3 + 2] = outcolor;
                        }
                        else
                        {
                            color = SGBInfo.palette[pal][color];
                            int r, g, b;
                            rgb16to32(color, &r, &g, &b);
                            gb_sgb_border_buffer[temp * 3 + 0] = r;
                            gb_sgb_border_buffer[temp * 3 + 1] = g;
                            gb_sgb_border_buffer[temp * 3 + 2] = b;
                        }
                    }
                }
            }
        }
    }
}

static void _win_gb_sgbviewer_draw_border_zoomed_tile(void)
{
    u32 info = SGBInfo.tile_map[gb_sgb_border_tiley * 32 + gb_sgb_border_tilex];

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

            data += y << 1;
            data2 += y << 1;

            u32 x_ = 7 - x;

            u32 color = (*data >> x_) & 1;
            color |= ((((*(data + 1)) >> x_) << 1) & (1 << 1));
            color |= ((((*data2) >> x_) << 2) & (1 << 2));
            color |= ((((*(data2 + 1)) >> x_) << 3) & (1 << 3));
            color = SGBInfo.palette[pal][color];

            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    int temp = (((y * 8) + j) * 64) + (x * 8) + i;
                    int r, g, b;
                    rgb16to32(color, &r, &g, &b);
                    gb_sgb_border_zoomed_tile_buffer[temp * 3 + 0] = r;
                    gb_sgb_border_zoomed_tile_buffer[temp * 3 + 1] = g;
                    gb_sgb_border_zoomed_tile_buffer[temp * 3 + 2] = b;
                }
            }
        }
    }

    //--------------------

    GUI_ConsoleClear(&gb_sgbview_border_con);

    GUI_ConsoleModePrintf(&gb_sgbview_border_con, 0, 0,
                          "Pos: %d,%d\n"
                          "Tile: %d\n"
                          "Flip: %s%s\n"
                          "Pal: %d",
                          gb_sgb_border_tilex, gb_sgb_border_tiley, tile,
                          xflip ? "H" : "-", yflip ? "V" : "-", pal);

    // Mark in border buffer the zoomed tile
    GUI_Draw_SetDrawingColor(255, 0, 0);
    int l = gb_sgb_border_tilex * 8; // Left
    int t = gb_sgb_border_tiley * 8; // Top
    int r = l + 7;                   // Right
    int b = t + 7;                   // Bottom
    GUI_Draw_Rect(gb_sgb_border_buffer, 256, 256, l, r, t, b);
}

static void _win_gb_sgbviewer_draw_tiles(void)
{
    GUI_ConsoleClear(&gb_sgbview_tiles_con);

    GUI_ConsoleModePrintf(&gb_sgbview_tiles_con, 0, 0,
                          "Pal: %d\n"
                          "Ind: %d",
                          gb_sgb_tiles_selected_pal,
                          gb_sgb_tiles_selected_index);

    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            u32 tile = i + j * 16;

            u32 *tile_ptr = &SGBInfo.tile_data[((8 * 8 * 4) / 8) * tile];

            for (int y = 0; y < 8; y++)
            {
                for (int x = 0; x < 8; x++)
                {
                    u32 *data = tile_ptr;
                    u32 *data2 = tile_ptr + 16;

                    data += y << 1;
                    data2 += y << 1;

                    u32 x_ = 7 - x;

                    u32 color = (*data >> x_) & 1;
                    color |= ((((*(data + 1)) >> x_) << 1) & (1 << 1));
                    color |= ((((*data2) >> x_) << 2) & (1 << 2));
                    color |= ((((*(data2 + 1)) >> x_) << 3) & (1 << 3));
                    color = SGBInfo.palette[gb_sgb_tiles_selected_pal][color];

                    int temp = ((y + (j << 3)) * 128) + (x + (i << 3));

                    int r, g, b;
                    rgb16to32(color, &r, &g, &b);
                    gb_sgb_tiles_buffer[temp * 3 + 0] = r;
                    gb_sgb_tiles_buffer[temp * 3 + 1] = g;
                    gb_sgb_tiles_buffer[temp * 3 + 2] = b;
                }
            }
        }
    }
}

static void _win_gb_sgbviewer_draw_tiles_zoomed_tile(void)
{
    u32 *tile_ptr = &SGBInfo.tile_data[((8 * 8 * 4) / 8)
                                       * gb_sgb_tiles_selected_index];

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            u32 *data = tile_ptr;
            u32 *data2 = tile_ptr + 16;

            data += y << 1;
            data2 += y << 1;

            u32 x_ = 7 - x;

            u32 color = (*data >> x_) & 1;
            color |= ((((*(data + 1)) >> x_) << 1) & (1 << 1));
            color |= ((((*data2) >> x_) << 2) & (1 << 2));
            color |= ((((*(data2 + 1)) >> x_) << 3) & (1 << 3));
            color = SGBInfo.palette[gb_sgb_tiles_selected_pal][color];

            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    int temp = (((y * 8) + j) * 64) + (x * 8) + i;
                    int r, g, b;
                    rgb16to32(color, &r, &g, &b);
                    gb_sgb_tiles_zoomed_tile_buffer[temp * 3 + 0] = r;
                    gb_sgb_tiles_zoomed_tile_buffer[temp * 3 + 1] = g;
                    gb_sgb_tiles_zoomed_tile_buffer[temp * 3 + 2] = b;
                }
            }
        }
    }

    // Mark in buffer the zoomed tile
    GUI_Draw_SetDrawingColor(255, 0, 0);
    int l = (gb_sgb_tiles_selected_index % 16) * 8; // Left
    int t = (gb_sgb_tiles_selected_index / 16) * 8; // Top
    int r = l + 7;                                  // Right
    int b = t + 7;                                  // Bottom
    GUI_Draw_Rect(gb_sgb_tiles_buffer, 128, 128, l, r, t, b);
}

static void _win_gb_sgbviewer_draw_atf(void)
{
    memset(gb_sgb_atf_buffer, 0, 161 * 145 * 3);

    for (int i = 0; i < 20; i++)
    {
        for (int j = 0; j < 18; j++)
        {
            u32 pal = SGBInfo.ATF_list[gb_sgb_atf_selected_index][(20 * j) + i];

            for (int y = 0; y < 8; y++)
            {
                for (int x = 0; x < 8; x++)
                {
                    if (!((x == 0) || (y == 0)))
                    {
                        u32 color;

                        if (x < 4)
                        {
                            if (y < 4)
                                color = 0;
                            else
                                color = 2;
                        }
                        else
                        {
                            if (y < 4)
                                color = 1;
                            else
                                color = 3;
                        }

                        int temp = ((y + (j << 3)) * (160 + 1))
                                   + (x + (i << 3));

                        color = SGBInfo.palette[pal][color];

                        int r, g, b;
                        rgb16to32(color, &r, &g, &b);
                        gb_sgb_atf_buffer[temp * 3 + 0] = r;
                        gb_sgb_atf_buffer[temp * 3 + 1] = g;
                        gb_sgb_atf_buffer[temp * 3 + 2] = b;
                    }
                }
            }
        }
    }

    // Mark in buffer the selected tile
    GUI_Draw_SetDrawingColor(255, 0, 0);
    int l = gb_sgb_atf_selected_x * 8; // Left
    int t = gb_sgb_atf_selected_y * 8; // Top
    int r = l + 8;                     // Right
    int b = t + 8;                     // Bottom
    GUI_Draw_Rect(gb_sgb_atf_buffer, 160 + 1, 144 + 1, l, r, t, b);
    l++;
    r--;
    t++;
    b--;
    GUI_Draw_Rect(gb_sgb_atf_buffer, 160 + 1, 144 + 1, l, r, t, b);

    u32 pal = SGBInfo.ATF_list[gb_sgb_atf_selected_index]
                              [(20 * gb_sgb_atf_selected_y) + gb_sgb_atf_selected_x];

    GUI_ConsoleClear(&gb_sgbview_atf_con);

    GUI_ConsoleModePrintf(&gb_sgbview_atf_con, 0, 0,
                          "Sel: %2d\n"
                          "X: %d\n"
                          "Y: %d\n"
                          "Pal: %d\n"
                          "Now: %2d",
                          gb_sgb_atf_selected_index, gb_sgb_atf_selected_x,
                          gb_sgb_atf_selected_y, pal, SGBInfo.curr_ATF);
}

static void _win_gb_sgbviewer_draw_pal(void)
{
    GUI_ConsoleClear(&gb_sgbview_pal_con);

    u32 color = SGBInfo.palette[gb_sgb_pal_selected_pal]
                               [gb_sgb_pal_selected_color];
    GUI_ConsoleModePrintf(&gb_sgbview_pal_con, 0, 0,
                          "Color: P%d[%d]\n"
                          "RGB: %d,%d,%d",
                          gb_sgb_pal_selected_pal, gb_sgb_pal_selected_color,
                          color & 0x1F, (color >> 5) & 0x1F,
                          (color >> 10) & 0x1F);

    memset(gb_sgb_pal_buffer, 192, 160 * 80 * 3);

    for (int i = 0; i < 8 * 16; i++)
    {
        if (!(((i / 16) < 4) && ((i % 16) >= 4)))
        {
            int r, g, b;
            rgb16to32(SGBInfo.palette[i / 16][i % 16], &r, &g, &b);
            GUI_Draw_SetDrawingColor(r, g, b);
            GUI_Draw_FillRect(gb_sgb_pal_buffer, 160, 80,
                              ((i % 16) * 10) + 1, ((i % 16) * 10) + 8,
                              ((i / 16) * 10) + 1, ((i / 16) * 10) + 8);
        }
        else
        {
            for (int y = 1; y < 9; y++)
            {
                for (int x = 1; x < 9; x++)
                {
                    int x_ = (i % 16) * 10 + x;
                    int y_ = (i / 16) * 10 + y;

                    int r = 0;

                    if ((((x - 1) & ~1) == ((y - 1) & ~1)))
                        r = 255;

                    gb_sgb_pal_buffer[(y_ * 160 + x_) * 3 + 0] = r;
                    gb_sgb_pal_buffer[(y_ * 160 + x_) * 3 + 1] = 0;
                    gb_sgb_pal_buffer[(y_ * 160 + x_) * 3 + 2] = 0;
                }
            }
        }
    }

    // Mark in buffer the selected color
    GUI_Draw_SetDrawingColor(255, 0, 0);
    int l = gb_sgb_pal_selected_color * 10; // Left
    int t = gb_sgb_pal_selected_pal * 10;   // Top
    int r = l + 9;                          // Right
    int b = t + 9;                          // Bottom
    GUI_Draw_Rect(gb_sgb_pal_buffer, 160, 80, l, r, t, b);
    l++;
    r--;
    t++;
    b--;
    GUI_Draw_Rect(gb_sgb_pal_buffer, 160, 80, l, r, t, b);
}

static void _win_gb_sgbviewer_packetdata_update(void)
{
    GUI_ConsoleClear(&gb_sgbview_packet_con);

    int numpackets = SGBInfo.data[0][0] & 0x07;

    if (numpackets == 0)
        numpackets = 1;

    u32 i = 0;
    while (numpackets--)
    {
        GUI_ConsoleModePrintf(&gb_sgbview_packet_con, 0, i + 2,
                "%d : %02X%02X %02X%02X %02X%02X %02X%02X "
                "%02X%02X %02X%02X %02X%02X %02X%02X",
                i + 1, SGBInfo.data[i][0], SGBInfo.data[i][1],
                SGBInfo.data[i][2], SGBInfo.data[i][3], SGBInfo.data[i][4],
                SGBInfo.data[i][5], SGBInfo.data[i][6], SGBInfo.data[i][7],
                SGBInfo.data[i][8], SGBInfo.data[i][9], SGBInfo.data[i][10],
                SGBInfo.data[i][11], SGBInfo.data[i][12], SGBInfo.data[i][13],
                SGBInfo.data[i][14], SGBInfo.data[i][15]);

        i++;
    }

    //--------------------------------------

    const char *command_names[0x20] = {
        "PAL01", "PAL23", "PAL03", "PAL12", "ATTR_BLK", "ATTR_LIN", "ATTR_DIV",
        "ATTR_CHR", "SOUND", "SOU_TRN", "PAL_SET", "PAL_TRN", "ATRC_EN",
        "TEST_EN", "ICON_EN", "DATA_SND", "DATA_TRN", "MLT_REG", "JUMP",
        "CHR_TRN", "PCT_TRN", "ATTR_TRN", "ATTR_SET", "MASK_EN", "OBJ_TRN",
        "???", "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", "BIOS_1 (?)",
        "BIOS_2 (?)"
    };

    const char *command_len[0x20] = {
        "1", "1", "1", "1", "1~7", "1~7", "1", "1~6",
        "1", "1", "1", "1", "1", "1", "1", "1",
        "1", "1", "1", "1", "1", "1", "1", "1",
        "1", "1?", "?", "?", "?", "?", "1?", "1?"
    };

    GUI_ConsoleModePrintf(&gb_sgbview_packet_con, 0, 0,
                          "Command: 0x%02X %s | Length = %s",
                          (SGBInfo.data[0][0] >> 3) & 0x1F,
                          command_names[(SGBInfo.data[0][0] >> 3) & 0x1F],
                          command_len[(SGBInfo.data[0][0] >> 3) & 0x1F]);
}

static void _win_gb_sgbviewer_otherinfo_update(void)
{
    GUI_ConsoleClear(&gb_sgbview_otherinfo_con);

    const char *screen_mode[4] = {
        "Normal", "Freeze", "Black", "Backdrop"
    };

    GUI_ConsoleModePrintf(&gb_sgbview_otherinfo_con, 0, 0,
                          "Players: %d\n"
                          "\n"
                          "Scrn: %s\n"
                          "\n"
                          "Attraction: %d\n"
                          "Test Speed: %d\n"
                          "\n"
                          "SGB Disabled: %d",
                          SGBInfo.multiplayer != 0 ? SGBInfo.multiplayer : 1,
                          screen_mode[SGBInfo.freeze_screen],
                          SGBInfo.attracion_mode, SGBInfo.test_speed_mode,
                          SGBInfo.disable_sgb);
}

//----------------------------------------------------------------

static int _win_gb_sgbviewer_border_bmp_callback(int x, int y)
{
    gb_sgb_border_tilex = x / 8;
    gb_sgb_border_tiley = y / 8;
    return 1;
}

static int _win_gb_sgbviewer_tiles_bmp_callback(int x, int y)
{
    gb_sgb_tiles_selected_index = (y / 8) * 16 + (x / 8);
    return 1;
}

static void _win_gb_sgbviewer_tiles_select_pal_scrollbar_callback(int value)
{
    gb_sgb_tiles_selected_pal = value;
}

static int _win_gb_sgbviewer_atf_bmp_callback(int x, int y)
{
    gb_sgb_atf_selected_x = x / 8;
    gb_sgb_atf_selected_y = y / 8;

    if (gb_sgb_atf_selected_x >= 20)
        gb_sgb_atf_selected_x = 20 - 1;
    if (gb_sgb_atf_selected_y >= 18)
        gb_sgb_atf_selected_y = 18 - 1;

    return 1;
}

static void _win_gb_sgbviewer_atf_select_scrollbar_callback(int value)
{
    gb_sgb_atf_selected_index = value;
}

static int _win_gb_sgbviewer_pal_bmp_callback(int x, int y)
{
    if (!((x >= 40) && (y < 40)))
    {
        gb_sgb_pal_selected_pal = y / 10;
        gb_sgb_pal_selected_color = x / 10;
    }
    return 1;
}

//----------------------------------------------------------------

void Win_GB_SGBViewerUpdate(void)
{
    if (GB_SGBViewerCreated == 0)
        return;

    if (Win_MainRunningGB() == 0)
        return;

    if (GB_EmulatorIsEnabledSGB() == 0)
        return;

    _win_gb_sgbviewer_draw_border();
    _win_gb_sgbviewer_draw_border_zoomed_tile();

    _win_gb_sgbviewer_draw_tiles();
    _win_gb_sgbviewer_draw_tiles_zoomed_tile();

    _win_gb_sgbviewer_draw_atf();

    _win_gb_sgbviewer_draw_pal();

    _win_gb_sgbviewer_packetdata_update();

    _win_gb_sgbviewer_otherinfo_update();
}

//----------------------------------------------------------------

static void _win_gb_sgb_viewer_render(void)
{
    if (GB_SGBViewerCreated == 0)
        return;

    char buffer[WIN_GB_SGBVIEWER_WIDTH * WIN_GB_SGBVIEWER_HEIGHT * 3];
    GUI_Draw(&gb_sgbviewer_window_gui, buffer,
             WIN_GB_SGBVIEWER_WIDTH, WIN_GB_SGBVIEWER_HEIGHT, 1);

    WH_Render(WinIDGB_SGBViewer, buffer);
}

static int _win_gb_sgb_viewer_callback(SDL_Event *e)
{
    if (GB_SGBViewerCreated == 0)
        return 1;

    int redraw = GUI_SendEvent(&gb_sgbviewer_window_gui, e);

    int close_this = 0;

    if (e->type == SDL_WINDOWEVENT)
    {
        if (e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            redraw = 1;
        }
        else if (e->window.event == SDL_WINDOWEVENT_EXPOSED)
        {
            redraw = 1;
        }
        else if (e->window.event == SDL_WINDOWEVENT_CLOSE)
        {
            close_this = 1;
        }
    }
    else if (e->type == SDL_KEYDOWN)
    {
        if (e->key.keysym.sym == SDLK_ESCAPE)
        {
            close_this = 1;
        }
    }

    if (close_this)
    {
        GB_SGBViewerCreated = 0;
        WH_Close(WinIDGB_SGBViewer);
        return 1;
    }

    if (redraw)
    {
        Win_GB_SGBViewerUpdate();
        _win_gb_sgb_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gb_sgbviewer_border_dump_btn_callback(void)
{
    if (Win_MainRunningGB() == 0)
        return;

    if (GB_EmulatorIsEnabledSGB() == 0)
        return;

    char *border_buff = malloc(256 * 256 * 4);
    if (border_buff == NULL)
        return;

    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
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

                    int temp = ((y + (j << 3)) * 256) + (x + (i << 3));
                    if (color)
                    {
                        color = SGBInfo.palette[pal][color];
                        int r, g, b;
                        rgb16to32(color, &r, &g, &b);
                        border_buff[temp * 4 + 0] = r;
                        border_buff[temp * 4 + 1] = g;
                        border_buff[temp * 4 + 2] = b;
                        border_buff[temp * 4 + 3] = 255;
                    }
                    else
                    {
                        // Check if inside
                        if ((i >= 6) && (i < 26) && (j >= 4) && (j < 23))
                        {
                            border_buff[temp * 4 + 0] = 0;
                            border_buff[temp * 4 + 1] = 0;
                            border_buff[temp * 4 + 2] = 0;
                            border_buff[temp * 4 + 3] = 0;
                        }
                        else
                        {
                            color = SGBInfo.palette[pal][color];
                            int r, g, b;
                            rgb16to32(color, &r, &g, &b);
                            border_buff[temp * 4 + 0] = r;
                            border_buff[temp * 4 + 1] = g;
                            border_buff[temp * 4 + 2] = b;
                            border_buff[temp * 4 + 3] = 255;
                        }
                    }
                }
            }
        }
    }

    char *name = FU_GetNewTimestampFilename("gb_sgb_border");
    Save_PNG(name, 256, 256, border_buff, 1);

    free(border_buff);

    Win_GB_SGBViewerUpdate();
}

static void _win_gb_sgbviewer_tiles_dump_btn_callback(void)
{
    if (Win_MainRunningGB() == 0)
        return;

    if (GB_EmulatorIsEnabledSGB() == 0)
        return;

    char *tiles_buff = malloc(128 * 128 * 4);
    if (tiles_buff == NULL)
        return;

    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            u32 tile = i + j * 16;

            u32 *tile_ptr = &SGBInfo.tile_data[((8 * 8 * 4) / 8) * tile];

            for (int y = 0; y < 8; y++)
            {
                for (int x = 0; x < 8; x++)
                {
                    u32 *data = tile_ptr;
                    u32 *data2 = tile_ptr + 16;

                    data += y << 1;
                    data2 += y << 1;

                    u32 x_ = 7 - x;

                    u32 color = (*data >> x_) & 1;
                    color |= ((((*(data + 1)) >> x_) << 1) & (1 << 1));
                    color |= ((((*data2) >> x_) << 2) & (1 << 2));
                    color |= ((((*(data2 + 1)) >> x_) << 3) & (1 << 3));
                    color = SGBInfo.palette[gb_sgb_tiles_selected_pal][color];

                    int temp = ((y + (j << 3)) * 128) + (x + (i << 3));

                    int r, g, b;
                    rgb16to32(color, &r, &g, &b);
                    tiles_buff[temp * 4 + 0] = r;
                    tiles_buff[temp * 4 + 1] = g;
                    tiles_buff[temp * 4 + 2] = b;
                    tiles_buff[temp * 4 + 3] = 0xFF;
                }
            }
        }
    }

    char *name = FU_GetNewTimestampFilename("gb_sgb_tiles");
    Save_PNG(name, 128, 128, tiles_buff, 0);

    free(tiles_buff);

    Win_GB_SGBViewerUpdate();
}

static void _win_gb_sgbviewer_atf_dump_btn_callback(void)
{
    if (Win_MainRunningGB() == 0)
        return;

    if (GB_EmulatorIsEnabledSGB() == 0)
        return;

    char *buf = calloc(161 * 145 * 4, 1);
    if (buf == NULL)
        return;

    for (int i = 0; i < 20; i++)
    {
        for (int j = 0; j < 18; j++)
        {
            u32 pal = SGBInfo.ATF_list[gb_sgb_atf_selected_index][(20 * j) + i];

            for (int y = 0; y < 8; y++)
            {
                for (int x = 0; x < 8; x++)
                {
                    if (!((x == 0) || (y == 0)))
                    {
                        u32 color;
                        if (x < 4)
                        {
                            if (y < 4)
                                color = 0;
                            else
                                color = 2;
                        }
                        else
                        {
                            if (y < 4)
                                color = 1;
                            else
                                color = 3;
                        }

                        int temp = ((y + (j << 3)) * (160 + 1))
                                   + (x + (i << 3));

                        color = SGBInfo.palette[pal][color];

                        int r, g, b;
                        rgb16to32(color, &r, &g, &b);
                        buf[temp * 4 + 0] = r;
                        buf[temp * 4 + 1] = g;
                        buf[temp * 4 + 2] = b;
                        buf[temp * 4 + 3] = 0xFF;
                    }
                }
            }
        }
    }

    char *name = FU_GetNewTimestampFilename("gb_sgb_atf");
    Save_PNG(name, 161, 145, buf, 0);

    free(buf);

    Win_GB_SGBViewerUpdate();
}

static void _win_gb_sgbviewer_pal_dump_btn_callback(void)
{
    if (Win_MainRunningGB() == 0)
        return;

    if (GB_EmulatorIsEnabledSGB() == 0)
        return;

    char *buf = malloc(160 * 80 * 4);
    if (buf == NULL)
        return;

    // Draw as normal
    memset(gb_sgb_pal_buffer, 192, 160 * 80 * 3);

    for (int i = 0; i < 8 * 16; i++)
    {
        if (!(((i / 16) < 4) && ((i % 16) >= 4)))
        {
            int r, g, b;
            rgb16to32(SGBInfo.palette[i / 16][i % 16], &r, &g, &b);
            GUI_Draw_SetDrawingColor(r, g, b);
            GUI_Draw_FillRect(gb_sgb_pal_buffer, 160, 80,
                              ((i % 16) * 10) + 1, ((i % 16) * 10) + 8,
                              ((i / 16) * 10) + 1, ((i / 16) * 10) + 8);
        }
        else
        {
            for (int y = 1; y < 9; y++)
            {
                for (int x = 1; x < 9; x++)
                {
                    int x_ = (i % 16) * 10 + x;
                    int y_ = (i / 16) * 10 + y;

                    int r = 0;
                    if ((((x - 1) & ~1) == ((y - 1) & ~1)))
                        r = 255;

                    gb_sgb_pal_buffer[(y_ * 160 + x_) * 3 + 0] = r;
                    gb_sgb_pal_buffer[(y_ * 160 + x_) * 3 + 1] = 0;
                    gb_sgb_pal_buffer[(y_ * 160 + x_) * 3 + 2] = 0;
                }
            }
        }
    }

    for (int i = 0; i < 160 * 80; i++)
    {
        buf[i * 4 + 0] = gb_sgb_pal_buffer[i * 3 + 0];
        buf[i * 4 + 1] = gb_sgb_pal_buffer[i * 3 + 1];
        buf[i * 4 + 2] = gb_sgb_pal_buffer[i * 3 + 2];
        buf[i * 4 + 3] = 0xFF;
    }

    char *name = FU_GetNewTimestampFilename("gb_sgb_pal");
    Save_PNG(name, 160, 80, buf, 0);

    free(buf);

    Win_GB_SGBViewerUpdate();
}

//----------------------------------------------------------------

int Win_GB_SGBViewerCreate(void)
{
    if (Win_MainRunningGB() == 0)
        return 0;

    if (GB_EmulatorIsEnabledSGB() == 0)
        return 0;

    if (GB_SGBViewerCreated == 1)
    {
        WH_Focus(WinIDGB_SGBViewer);
        return 0;
    }

    // Border

    GUI_SetGroupBox(&gb_sgbview_border_groupbox, 6, 6, 268, 350, "Border");

    GUI_SetTextBox(&gb_sgbview_border_textbox, &gb_sgbview_border_con,
                   82, 286, 11 * FONT_WIDTH, 4 * FONT_HEIGHT, NULL);

    GUI_SetBitmap(&gb_sgbview_border_bmp, 12, 24, 256, 256,
                  gb_sgb_border_buffer, _win_gb_sgbviewer_border_bmp_callback);

    GUI_SetBitmap(&gb_sgbview_border_zoomed_tile_bmp, 12, 286, 64, 64,
                  gb_sgb_border_zoomed_tile_buffer, NULL);

    GUI_SetButton(&gb_sgbview_border_dump_btn,
                  165, 286, FONT_WIDTH * 8, FONT_HEIGHT * 2, "Dump",
                  _win_gb_sgbviewer_border_dump_btn_callback);

    gb_sgb_border_tilex = 0;
    gb_sgb_border_tiley = 0;

    // Tiles

    GUI_SetGroupBox(&gb_sgbview_tiles_groupbox, 280, 6, 271, 152, "Tiles");

    GUI_SetBitmap(&gb_sgbview_tiles_bmp, 286, 24, 128, 128, gb_sgb_tiles_buffer,
                  _win_gb_sgbviewer_tiles_bmp_callback);

    GUI_SetBitmap(&gb_sgbview_tiles_bmp, 286, 24, 128, 128, gb_sgb_tiles_buffer,
                  _win_gb_sgbviewer_tiles_bmp_callback);

    GUI_SetTextBox(&gb_sgbview_tiles_textbox, &gb_sgbview_tiles_con,
                   420, 24, 9 * FONT_WIDTH, 2 * FONT_HEIGHT, NULL);

    GUI_SetScrollBar(&gb_sgbview_tiles_select_pal_scrollbar, 420, 54, 64, 12,
                     4, 7, 4,
                     _win_gb_sgbviewer_tiles_select_pal_scrollbar_callback);

    GUI_SetBitmap(&gb_sgbview_tiles_zoomed_tile_bmp, 420, 88, 64, 64,
                  gb_sgb_tiles_zoomed_tile_buffer, NULL);

    GUI_SetButton(&gb_sgbview_tiles_dump_btn,
                  489, 24, FONT_WIDTH * 8, FONT_HEIGHT * 2, "Dump",
                  _win_gb_sgbviewer_tiles_dump_btn_callback);

    gb_sgb_tiles_selected_index = 0;
    gb_sgb_tiles_selected_pal = 4;

    // Attribute Files (ATFs)

    GUI_SetGroupBox(&gb_sgbview_atf_groupbox, 280, 164, 239, 181,
                    "Attribute Files (ATFs)");

    GUI_SetBitmap(&gb_sgbview_atf_bmp, 286, 182, 160 + 1, 144 + 1,
                  gb_sgb_atf_buffer, _win_gb_sgbviewer_atf_bmp_callback);

    GUI_SetScrollBar(&gb_sgbview_atf_select_scrollbar, 286, 327, 160 + 1, 12,
                     0, 0x2D - 1, 0,
                     _win_gb_sgbviewer_atf_select_scrollbar_callback);

    GUI_SetTextBox(&gb_sgbview_atf_textbox, &gb_sgbview_atf_con,
                   457, 182, 8 * FONT_WIDTH, 5 * FONT_HEIGHT, NULL);

    GUI_SetButton(&gb_sgbview_atf_dump_btn,
                  457, 248, FONT_WIDTH * 8, FONT_HEIGHT * 2, "Dump",
                  _win_gb_sgbviewer_atf_dump_btn_callback);

    gb_sgb_atf_selected_index = 0;
    gb_sgb_atf_selected_x = 0;
    gb_sgb_atf_selected_y = 0;

    // Palettes

    GUI_SetGroupBox(&gb_sgbview_pal_groupbox, 6, 362, 172, 135, "Palettes");

    GUI_SetBitmap(&gb_sgbview_pal_bmp, 12, 380, 160, 80, gb_sgb_pal_buffer,
                  _win_gb_sgbviewer_pal_bmp_callback);

    GUI_SetTextBox(&gb_sgbview_pal_textbox, &gb_sgbview_pal_con,
                   12, 466, 14 * FONT_WIDTH, 2 * FONT_HEIGHT, NULL);

    GUI_SetButton(&gb_sgbview_pal_dump_btn,
                  116, 466, FONT_WIDTH * 8, FONT_HEIGHT * 2, "Dump",
                  _win_gb_sgbviewer_pal_dump_btn_callback);

    gb_sgb_pal_selected_pal = 0;
    gb_sgb_pal_selected_color = 0;

    // Packet data

    GUI_SetLabel(&gb_sgbview_packet_label,
                 184, 362, 11 * FONT_WIDTH, FONT_HEIGHT, "Packet data");

    GUI_SetTextBox(&gb_sgbview_packet_textbox, &gb_sgbview_packet_con,
                   184, 380, 44 * FONT_WIDTH, 9 * FONT_HEIGHT, NULL);

    // Other info

    GUI_SetLabel(&gb_sgbview_otherinfo_label,
                 498, 362, 11 * FONT_WIDTH, FONT_HEIGHT, "Other info.");

    GUI_SetTextBox(&gb_sgbview_otherinfo_textbox, &gb_sgbview_otherinfo_con,
                   498, 380, 16 * FONT_WIDTH, 8 * FONT_HEIGHT, NULL);

    //---------------

    GB_SGBViewerCreated = 1;

    WinIDGB_SGBViewer = WH_Create(WIN_GB_SGBVIEWER_WIDTH,
                                  WIN_GB_SGBVIEWER_HEIGHT, 0, 0, 0);
    WH_SetCaption(WinIDGB_SGBViewer, "SGB Viewer");

    WH_SetEventCallback(WinIDGB_SGBViewer, _win_gb_sgb_viewer_callback);

    Win_GB_SGBViewerUpdate();
    _win_gb_sgb_viewer_render();

    return 1;
}

void Win_GB_SGBViewerClose(void)
{
    if (GB_SGBViewerCreated == 0)
        return;

    GB_SGBViewerCreated = 0;
    WH_Close(WinIDGB_SGBViewer);
}
