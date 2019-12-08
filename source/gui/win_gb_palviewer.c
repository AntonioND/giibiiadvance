// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include <SDL.h>

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

//------------------------------------------------------------------------------

static int WinIDGBPalViewer;

#define WIN_GB_PALVIEWER_WIDTH  258
#define WIN_GB_PALVIEWER_HEIGHT 222

static int GBPalViewerCreated = 0;

//------------------------------------------------------------------------------

static u32 gb_palview_sprpal = 0; // 1 if selected a color from sprite palettes
static u32 gb_palview_selectedindex = 0;

#define GB_PAL_BUFFER_WIDTH ((20 * 4) + 1)
#define GB_PAL_BUFFER_HEIGHT ((20 * 8) + 1)

static char gb_pal_bg_buffer[GB_PAL_BUFFER_WIDTH * GB_PAL_BUFFER_HEIGHT * 3];
static char gb_pal_spr_buffer[GB_PAL_BUFFER_WIDTH * GB_PAL_BUFFER_HEIGHT * 3];

//------------------------------------------------------------------------------

static _gui_console gb_palview_con;
static _gui_element gb_palview_textbox;

static _gui_element gb_palview_dumpbtn;

static _gui_element gb_palview_bgpal_bmp, gb_palview_sprpal_bmp;
static _gui_element gb_palview_bgpal_label, gb_palview_sprpal_label;

static _gui_element *gb_palviwer_window_gui_elements[] = {
    &gb_palview_bgpal_label,
    &gb_palview_sprpal_label,
    &gb_palview_bgpal_bmp,
    &gb_palview_sprpal_bmp,
    &gb_palview_textbox,
    &gb_palview_dumpbtn,
    NULL
};

static _gui gb_palviewer_window_gui = {
    gb_palviwer_window_gui_elements,
    NULL,
    NULL
};

//----------------------------------------------------------------

static int _win_gb_palviewer_bg_bmp_callback(int x, int y)
{
    if (x >= GB_PAL_BUFFER_WIDTH - 1)
        x = GB_PAL_BUFFER_WIDTH - 2;
    if (y >= GB_PAL_BUFFER_HEIGHT - 1)
        y = GB_PAL_BUFFER_HEIGHT - 2;
    gb_palview_sprpal = 0; // BG
    gb_palview_selectedindex = (x / 20) + ((y / 20) * 4);
    return 1;
}

static int _win_gb_palviewer_spr_bmp_callback(int x, int y)
{
    if (x >= GB_PAL_BUFFER_WIDTH - 1)
        x = GB_PAL_BUFFER_WIDTH - 2;
    if (y >= GB_PAL_BUFFER_HEIGHT - 1)
        y = GB_PAL_BUFFER_HEIGHT - 2;
    gb_palview_sprpal = 1; // Spr
    gb_palview_selectedindex = (x / 20) + ((y / 20) * 4);
    return 1;
}

//----------------------------------------------------------------

void Win_GBPalViewerUpdate(void)
{
    if (GBPalViewerCreated == 0)
        return;

    if (Win_MainRunningGB() == 0)
        return;

    GUI_ConsoleClear(&gb_palview_con);

    u32 r, g, b;
    GB_Debug_GetPalette(gb_palview_sprpal, gb_palview_selectedindex / 4,
                        gb_palview_selectedindex % 4, &r, &g, &b);
    u16 value = ((r >> 3) & 0x1F)
                | (((g >> 3) & 0x1F) << 5)
                | (((b >> 3) & 0x1F) << 10);

    GUI_ConsoleModePrintf(&gb_palview_con, 0, 0, "Value: 0x%04X", value);

    GUI_ConsoleModePrintf(&gb_palview_con, 0, 1, "RGB: (%d,%d,%d)",
                          r >> 3, g >> 3, b >> 3);

    GUI_ConsoleModePrintf(&gb_palview_con, 16, 0, "Index: %d[%d]",
                          gb_palview_selectedindex / 4,
                          gb_palview_selectedindex % 4);

    memset(gb_pal_bg_buffer, 192, sizeof(gb_pal_bg_buffer));
    memset(gb_pal_spr_buffer, 192, sizeof(gb_pal_spr_buffer));

    for (int i = 0; i < 32; i++)
    {
        // BG
        GB_Debug_GetPalette(0, i / 4, i % 4, &r, &g, &b);
        GUI_Draw_SetDrawingColor(r, g, b);
        GUI_Draw_FillRect(gb_pal_bg_buffer,
                          GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT,
                          1 + ((i % 4) * 20), 19 + ((i % 4) * 20),
                          1 + ((i / 4) * 20), 19 + ((i / 4) * 20));
        // SPR
        GB_Debug_GetPalette(1, i / 4, i % 4, &r, &g, &b);
        GUI_Draw_SetDrawingColor(r, g, b);
        GUI_Draw_FillRect(gb_pal_spr_buffer,
                          GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT,
                          1 + ((i % 4) * 20), 19 + ((i % 4) * 20),
                          1 + ((i / 4) * 20), 19 + ((i / 4) * 20));
    }

    GUI_Draw_SetDrawingColor(255, 0, 0);

    char *buf;
    if (gb_palview_sprpal == 0)
        buf = gb_pal_bg_buffer;
    else
        buf = gb_pal_spr_buffer;

    int ll = (gb_palview_selectedindex % 4) * 20; // Left
    int tt = (gb_palview_selectedindex / 4) * 20; // Top
    int rr = ll + 20;                             // Right
    int bb = tt + 20;                             // Bottom
    GUI_Draw_Rect(buf, GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT,
                  ll, rr, tt, bb);
    ll++;
    tt++;
    rr--;
    bb--;
    GUI_Draw_Rect(buf, GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT,
                  ll, rr, tt, bb);
}

//----------------------------------------------------------------

static void _win_gb_pal_viewer_render(void)
{
    if (GBPalViewerCreated == 0)
        return;

    char buffer[WIN_GB_PALVIEWER_WIDTH * WIN_GB_PALVIEWER_HEIGHT * 3];
    GUI_Draw(&gb_palviewer_window_gui, buffer,
             WIN_GB_PALVIEWER_WIDTH, WIN_GB_PALVIEWER_HEIGHT, 1);

    WH_Render(WinIDGBPalViewer, buffer);
}

static int _win_gb_pal_viewer_callback(SDL_Event *e)
{
    if (GBPalViewerCreated == 0)
        return 1;

    int redraw = GUI_SendEvent(&gb_palviewer_window_gui, e);

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
        GBPalViewerCreated = 0;
        WH_Close(WinIDGBPalViewer);
        return 1;
    }

    if (redraw)
    {
        Win_GBPalViewerUpdate();
        _win_gb_pal_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gb_palviewer_dump_btn_callback(void)
{
    if (Win_MainRunningGB() == 0)
        return;

    memset(gb_pal_bg_buffer, 192, sizeof(gb_pal_bg_buffer));
    memset(gb_pal_spr_buffer, 192, sizeof(gb_pal_spr_buffer));

    u32 r, g, b;

    for (int i = 0; i < 32; i++)
    {
        // BG
        GB_Debug_GetPalette(0, i / 4, i % 4, &r, &g, &b);
        GUI_Draw_SetDrawingColor(r, g, b);
        GUI_Draw_FillRect(gb_pal_bg_buffer, GB_PAL_BUFFER_WIDTH,
                          GB_PAL_BUFFER_HEIGHT,
                          1 + ((i % 4) * 20), 19 + ((i % 4) * 20),
                          1 + ((i / 4) * 20), 19 + ((i / 4) * 20));
        // SPR
        GB_Debug_GetPalette(1, i / 4, i % 4, &r, &g, &b);
        GUI_Draw_SetDrawingColor(r, g, b);
        GUI_Draw_FillRect(gb_pal_spr_buffer,
                          GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT,
                          1 + ((i % 4) * 20), 19 + ((i % 4) * 20),
                          1 + ((i / 4) * 20), 19 + ((i / 4) * 20));
    }

    char buffer_temp[GB_PAL_BUFFER_WIDTH * GB_PAL_BUFFER_HEIGHT * 4];

    char *src = gb_pal_bg_buffer;
    char *dst = buffer_temp;
    for (int i = 0; i < GB_PAL_BUFFER_WIDTH * GB_PAL_BUFFER_HEIGHT; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xFF;
    }

    char *name_bg = FU_GetNewTimestampFilename("gb_palette_bg");
    Save_PNG(name_bg, GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT,
             buffer_temp, 0);

    src = gb_pal_spr_buffer;
    dst = buffer_temp;
    for (int i = 0; i < GB_PAL_BUFFER_WIDTH * GB_PAL_BUFFER_HEIGHT; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xFF;
    }

    char *name_spr = FU_GetNewTimestampFilename("gb_palette_spr");
    Save_PNG(name_spr, GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT,
             buffer_temp, 0);

    Win_GBPalViewerUpdate();
}

//----------------------------------------------------------------

int Win_GBPalViewerCreate(void)
{
    if (Win_MainRunningGB() == 0)
        return 0;

    if (GBPalViewerCreated == 1)
    {
        WH_Focus(WinIDGBPalViewer);
        return 0;
    }

    GUI_SetLabel(&gb_palview_bgpal_label, 32, 6,
                 GB_PAL_BUFFER_WIDTH, FONT_HEIGHT, "Background");
    GUI_SetLabel(&gb_palview_sprpal_label, 145, 6,
                 GB_PAL_BUFFER_WIDTH, FONT_HEIGHT, "Sprites");

    GUI_SetTextBox(&gb_palview_textbox, &gb_palview_con,
                   6, 192, 28 * FONT_WIDTH, 2 * FONT_HEIGHT, NULL);

    GUI_SetBitmap(&gb_palview_bgpal_bmp, 32, 24,
                  GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT, gb_pal_bg_buffer,
                  _win_gb_palviewer_bg_bmp_callback);
    GUI_SetBitmap(&gb_palview_sprpal_bmp, 145, 24,
                  GB_PAL_BUFFER_WIDTH, GB_PAL_BUFFER_HEIGHT, gb_pal_spr_buffer,
                  _win_gb_palviewer_spr_bmp_callback);

    GUI_SetButton(&gb_palview_dumpbtn, 210, 192,
                  FONT_WIDTH * 6, FONT_HEIGHT * 2, "Dump",
                  _win_gb_palviewer_dump_btn_callback);

    gb_palview_sprpal = 0;
    gb_palview_selectedindex = 0;

    GBPalViewerCreated = 1;

    WinIDGBPalViewer = WH_Create(WIN_GB_PALVIEWER_WIDTH,
                                 WIN_GB_PALVIEWER_HEIGHT, 0, 0, 0);
    WH_SetCaption(WinIDGBPalViewer, "GB Palette Viewer");

    WH_SetEventCallback(WinIDGBPalViewer, _win_gb_pal_viewer_callback);

    Win_GBPalViewerUpdate();
    _win_gb_pal_viewer_render();

    return 1;
}

void Win_GBPalViewerClose(void)
{
    if (GBPalViewerCreated == 0)
        return;

    GBPalViewerCreated = 0;
    WH_Close(WinIDGBPalViewer);
}
