// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
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

//------------------------------------------------------------------------------

static int WinIDGBSprViewer;

#define WIN_GB_SPRVIEWER_WIDTH  365
#define WIN_GB_SPRVIEWER_HEIGHT 268

static int GBSprViewerCreated = 0;

static char *gb_sprviewer_buffer = NULL;

//------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------

static u32 gb_sprview_selected_spr = 0;

#define GB_SPR_ALLSPR_BUFFER_WIDTH  (256)
#define GB_SPR_ALLSPR_BUFFER_HEIGHT (256)

static char gb_spr_allspr_buffer[GB_SPR_ALLSPR_BUFFER_WIDTH
                                 * GB_SPR_ALLSPR_BUFFER_HEIGHT * 3];

#define GB_SPR_ZOOMED_BUFFER_WIDTH  (8 * 8)
#define GB_SPR_ZOOMED_BUFFER_HEIGHT (16 * 8)

static char gb_spr_zoomed_buffer[GB_SPR_ZOOMED_BUFFER_WIDTH
                                 * GB_SPR_ZOOMED_BUFFER_HEIGHT * 3];

//------------------------------------------------------------------------------

static _gui_console gb_sprview_con;
static _gui_element gb_sprview_textbox;

static _gui_element gb_sprview_allspr_dumpbtn;
static _gui_element gb_sprview_zoomed_spr_dumpbtn;

static _gui_element gb_sprview_allspr_bmp, gb_sprview_zoomedspr_bmp;

static _gui_element *gb_sprviwer_window_gui_elements[] = {
    &gb_sprview_allspr_bmp,
    &gb_sprview_zoomedspr_bmp,
    &gb_sprview_textbox,
    &gb_sprview_allspr_dumpbtn,
    &gb_sprview_zoomed_spr_dumpbtn,
    NULL
};

static _gui gb_sprviewer_window_gui = {
    gb_sprviwer_window_gui_elements,
    NULL,
    NULL
};

//----------------------------------------------------------------

static int _win_gb_sprviewer_allspr_bmp_callback(int x, int y)
{
    int x_ = x / 32;
    if (x_ > 7)
        x_ = 7;

    int y_ = y / 52;
    if (y_ > 4)
        y_ = 4;

    gb_sprview_selected_spr = (y_ * 8) + x_;
    return 1;
}

//----------------------------------------------------------------

void Win_GBSprViewerUpdate(void)
{
    if (GBSprViewerCreated == 0)
        return;

    if (Win_MainRunningGB() == 0)
        return;

    GUI_ConsoleClear(&gb_sprview_con);

    u8 spr_x, spr_y, tile, info;
    GB_Debug_GetSpriteInfo(gb_sprview_selected_spr, &spr_x, &spr_y,
                           &tile, &info);

    int pal, bank;
    if (GameBoy.Emulator.CGBEnabled)
    {
        pal = info & 7;
        bank = (info & (1 << 3)) != 0;
    }
    else
    {
        pal = (info & (1 << 4)) != 0;
        bank = 0;
    }

    _GB_MEMORY_ *mem = &GameBoy.Memory;
    int sy = 8 << ((mem->IO_Ports[LCDC_REG - 0xFF00] & (1 << 2)) != 0);

    GUI_ConsoleModePrintf(&gb_sprview_con, 0, 0, "ID: %2d [8x%d]",
                          gb_sprview_selected_spr, sy);
    GUI_ConsoleModePrintf(&gb_sprview_con, 0, 1,
                          "Tile: %d(%d)\n"
                          "Pal: %d\n"
                          "Pos: %d,%d\n"
                          "Attr: %s%s%s",
                          tile, bank, pal, spr_x, spr_y,
                          (info & (1 << 5)) ? "H" : "-",
                          (info & (1 << 6)) ? "V" : "-",
                          (info & (1 << 7)) ? "P" : "-");

    GB_Debug_PrintSprites(gb_spr_allspr_buffer);

    GUI_Draw_SetDrawingColor(255, 0, 0);
    int l = (gb_sprview_selected_spr % 8) * 32 + 8;  // Left
    int t = (gb_sprview_selected_spr / 8) * 52 + 10; // Top
    int r = l + 15;                                  // Right
    int b = t + 2 * sy - 1;                          // Bottom
    l--;
    r++;
    t--;
    b++;
    GUI_Draw_Rect(gb_spr_allspr_buffer,
                  GB_SPR_ALLSPR_BUFFER_WIDTH, GB_SPR_ALLSPR_BUFFER_HEIGHT,
                  l, r, t, b);
    l--;
    r++;
    t--;
    b++;
    GUI_Draw_Rect(gb_spr_allspr_buffer,
                  GB_SPR_ALLSPR_BUFFER_WIDTH, GB_SPR_ALLSPR_BUFFER_HEIGHT,
                  l, r, t, b);

    GB_Debug_PrintZoomedSprite(gb_spr_zoomed_buffer, gb_sprview_selected_spr);
}

//----------------------------------------------------------------

static void _win_gb_spr_viewer_render(void)
{
    if (GBSprViewerCreated == 0)
        return;

    GUI_Draw(&gb_sprviewer_window_gui, gb_sprviewer_buffer,
             WIN_GB_SPRVIEWER_WIDTH, WIN_GB_SPRVIEWER_HEIGHT, 1);

    WH_Render(WinIDGBSprViewer, gb_sprviewer_buffer);
}

static int _win_gb_spr_viewer_callback(SDL_Event *e)
{
    if (GBSprViewerCreated == 0)
        return 1;

    int redraw = GUI_SendEvent(&gb_sprviewer_window_gui, e);

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
        GBSprViewerCreated = 0;
        WH_Close(WinIDGBSprViewer);
        return 1;
    }

    if (redraw)
    {
        Win_GBSprViewerUpdate();
        _win_gb_spr_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gb_sprviewer_allspr_dump_btn_callback(void)
{
    if (Win_MainRunningGB() == 0)
        return;

    char *allbuf = calloc(GB_SPR_ALLSPR_BUFFER_WIDTH *
                          GB_SPR_ALLSPR_BUFFER_HEIGHT * 4, sizeof(char));
    if (allbuf == NULL)
    {
        Debug_ErrorMsgArg("%s(): Not enough memory.");
        return;
    }

    GB_Debug_PrintSpritesAlpha(allbuf);

    char *name = FU_GetNewTimestampFilename("gb_sprite_all");
    Save_PNG(name, GB_SPR_ALLSPR_BUFFER_WIDTH, GB_SPR_ALLSPR_BUFFER_HEIGHT,
             allbuf, 1);

    Win_GBSprViewerUpdate();

    free(allbuf);
}

static void _win_gb_sprviewer_zoomed_dump_btn_callback(void)
{
    if (Win_MainRunningGB() == 0)
        return;

    char buf[8 * 16 * 4];
    GB_Debug_PrintSpriteAlpha(buf, gb_sprview_selected_spr);

    _GB_MEMORY_ *mem = &GameBoy.Memory;
    int sy = 8 << ((mem->IO_Ports[LCDC_REG - 0xFF00] & (1 << 2)) != 0);

    char *name = FU_GetNewTimestampFilename("gb_sprite");
    Save_PNG(name, 8, sy, buf, 1);

    Win_GBSprViewerUpdate();
}

//----------------------------------------------------------------

int Win_GBSprViewerCreate(void)
{
    if (Win_MainRunningGB() == 0)
        return 0;

    if (GBSprViewerCreated == 1)
    {
        WH_Focus(WinIDGBSprViewer);
        return 0;
    }

    gb_sprviewer_buffer = calloc(1, WIN_GB_SPRVIEWER_WIDTH *
                                 WIN_GB_SPRVIEWER_HEIGHT * 3);
    if (gb_sprviewer_buffer == NULL)
    {
        Debug_ErrorMsgArg("%s(): Not enough memory.");
        return 0;
    }

    GUI_SetTextBox(&gb_sprview_textbox, &gb_sprview_con,
                   268, 140, 13 * FONT_WIDTH, 5 * FONT_HEIGHT, NULL);

    GUI_SetBitmap(&gb_sprview_allspr_bmp, 6, 6,
                  GB_SPR_ALLSPR_BUFFER_WIDTH, GB_SPR_ALLSPR_BUFFER_HEIGHT,
                  gb_spr_allspr_buffer,
                  _win_gb_sprviewer_allspr_bmp_callback);
    GUI_SetBitmap(&gb_sprview_zoomedspr_bmp, 280, 6,
                  GB_SPR_ZOOMED_BUFFER_WIDTH, GB_SPR_ZOOMED_BUFFER_HEIGHT,
                  gb_spr_zoomed_buffer, NULL);

    GUI_SetButton(&gb_sprview_zoomed_spr_dumpbtn,
                  268, 207, FONT_WIDTH * 13, FONT_HEIGHT * 2, "Dump zoomed",
                  _win_gb_sprviewer_zoomed_dump_btn_callback);

    GUI_SetButton(&gb_sprview_allspr_dumpbtn,
                  268, 238, FONT_WIDTH * 13, FONT_HEIGHT * 2, "Dump all",
                  _win_gb_sprviewer_allspr_dump_btn_callback);

    gb_sprview_selected_spr = 0;

    GBSprViewerCreated = 1;

    WinIDGBSprViewer = WH_Create(WIN_GB_SPRVIEWER_WIDTH,
                                 WIN_GB_SPRVIEWER_HEIGHT, 0, 0, 0);
    WH_SetCaption(WinIDGBSprViewer, "GB Sprite Viewer");

    WH_SetEventCallback(WinIDGBSprViewer, _win_gb_spr_viewer_callback);

    Win_GBSprViewerUpdate();
    _win_gb_spr_viewer_render();

    return 1;
}

void Win_GBSprViewerClose(void)
{
    if (GBSprViewerCreated == 0)
        return;

    free(gb_sprviewer_buffer);
    gb_sprviewer_buffer = NULL;

    GBSprViewerCreated = 0;
    WH_Close(WinIDGBSprViewer);
}
