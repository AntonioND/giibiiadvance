/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2014 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL2/SDL.h>

#include <string.h>

#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../general_utils.h"
#include "../file_utils.h"

#include "win_gba_tileviewer.h"
#include "win_main.h"
#include "win_utils.h"


#include "../png/png_utils.h"

//------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------

static int WinIDGBATileViewer;

#define WIN_GBA_TILEVIEWER_WIDTH  500
#define WIN_GBA_TILEVIEWER_HEIGHT 450

static int GBATileViewerCreated = 0;

//-----------------------------------------------------------------------------------

static u32 gba_tileview_selected_pal = 0;
static u32 gba_tileview_selected_index = 0;

#define GBA_TILE_BUFFER_WIDTH (16*8)
#define GBA_TILE_BUFFER_HEIGHT (24*8)

static char gba_tile_buffer[GBA_TILE_BUFFER_WIDTH*GBA_TILE_BUFFER_HEIGHT*3];

static char gba_tile_zoomed_tile_buffer[64*64*3];

//-----------------------------------------------------------------------------------

static _gui_console gba_tileview_con;
static _gui_element gba_tileview_textbox;

static _gui_element gba_tileview_dumpbtn;

static _gui_element gba_tileview_pal_scrollbar;

static _gui_element gba_tileview_tiles_bmp;
static _gui_element gba_tileview_bank0_label, gba_tileview_bank1_label;

static _gui_element gba_tileview_zoomed_tile_bmp;

static _gui_element * gba_tileviwer_window_gui_elements[] = {
    &gba_tileview_bank0_label,
    &gba_tileview_bank1_label,
    &gba_tileview_tiles_bmp,
    &gba_tileview_zoomed_tile_bmp,
    &gba_tileview_pal_scrollbar,
    &gba_tileview_textbox,
    &gba_tileview_dumpbtn,
    NULL
};

static _gui gba_tileviewer_window_gui = {
    gba_tileviwer_window_gui_elements,
    NULL,
    NULL
};

//----------------------------------------------------------------

static inline void rgb16to32(u16 color, u8 * r, u8 * g, u8 * b)
{
    *r = (color & 31)<<3;
    *g = ((color >> 5) & 31)<<3;
    *b = ((color >> 10) & 31)<<3;
}

//----------------------------------------------------------------

static int _win_gba_tileviewer_tiles_bmp_callback(int x, int y)
{
    gba_tileview_selected_index = (x/8) + ((y/8)*32);
    return 1;
}

static void _win_gba_tileviewer_pal_scrollbar_callback(int value)
{
    gba_tileview_selected_pal = value;
    return;
}

//----------------------------------------------------------------

void Win_GBATileViewerUpdate(void)
{
    if(GBATileViewerCreated == 0) return;

    if(Win_MainRunningGBA() == 0) return;

    GUI_ConsoleClear(&gba_tileview_con);

    GUI_ConsoleModePrintf(&gba_tileview_con,0,0,"Value: %d",gba_tileview_selected_pal);

/*
    GB_Debug_TileVRAMDraw(gba_tile_bank0_buffer,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,
                          gba_tile_bank1_buffer,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT);

    GUI_Draw_SetDrawingColor(255,0,0);
    char * buf;
    if(gba_tileview_selected_bank == 0)
        buf = gba_tile_bank0_buffer;
    else
        buf = gba_tile_bank1_buffer;
    int l = ((gba_tileview_selected_index%16)*8); //left
    int t = ((gba_tileview_selected_index/16)*8); // top
    int r = l + 7; // right
    int b = t + 7; // bottom
    GUI_Draw_Rect(buf,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,l,r,t,b);

    GB_Debug_TileDrawZoomed64x64(gba_tile_zoomed_tile_buffer, gba_tileview_selected_index, gba_tileview_selected_bank);

    u32 tile = gba_tileview_selected_index;
    u32 tileindex = (tile > 255) ? (tile - 256) : (tile);
    if(GameBoy.Emulator.CGBEnabled)
    {
        GUI_ConsoleModePrintf(&gba_tileview_con,0,0,"Tile: %d(%d)\nAddr: 0x%04X\nBank: %d",tile,tileindex,
            0x8000 + (tile * 16),gba_tileview_selected_bank);
    }
    else
    {
        GUI_ConsoleModePrintf(&gba_tileview_con,0,0,"Tile: %d(%d)\nAddr: 0x%04X\nBank: -",tile,tileindex,
            0x8000 + (tile * 16));
    }*/
}

//----------------------------------------------------------------

static void _win_gba_tile_viewer_render(void)
{
    if(GBATileViewerCreated == 0) return;

    char buffer[WIN_GBA_TILEVIEWER_WIDTH*WIN_GBA_TILEVIEWER_HEIGHT*3];
    GUI_Draw(&gba_tileviewer_window_gui,buffer,WIN_GBA_TILEVIEWER_WIDTH,WIN_GBA_TILEVIEWER_HEIGHT,1);

    WH_Render(WinIDGBATileViewer, buffer);
}

static int _win_gba_tile_viewer_callback(SDL_Event * e)
{
    if(GBATileViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gba_tileviewer_window_gui,e);

    int close_this = 0;

    if(e->type == SDL_WINDOWEVENT)
    {
        if(e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            redraw = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_EXPOSED)
        {
            redraw = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_CLOSE)
        {
            close_this = 1;
        }
    }
    else if( e->type == SDL_KEYDOWN)
    {
        if( e->key.keysym.sym == SDLK_ESCAPE )
        {
            close_this = 1;
        }
    }

    if(close_this)
    {
        GBATileViewerCreated = 0;
        WH_Close(WinIDGBATileViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBATileViewerUpdate();
        _win_gba_tile_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gba_tileviewer_dump_btn_callback(void)
{/*
    if(Win_MainRunningGBA() == 0) return;

    GB_Debug_TileVRAMDraw(gba_tile_bank0_buffer,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,
                          gba_tile_bank1_buffer,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT);

    char buf0[GBA_TILE_BUFFER_WIDTH*GBA_TILE_BUFFER_HEIGHT*4];
    char buf1[GBA_TILE_BUFFER_WIDTH*GBA_TILE_BUFFER_HEIGHT*4];

    int i;
    for(i = 0; i < (GBA_TILE_BUFFER_WIDTH*GBA_TILE_BUFFER_HEIGHT); i++)
    {
        buf0[i*4+0] = gba_tile_bank0_buffer[i*3+0];
        buf0[i*4+1] = gba_tile_bank0_buffer[i*3+1];
        buf0[i*4+2] = gba_tile_bank0_buffer[i*3+2];
        buf0[i*4+3] = 255;
    }

    char * name_b0 = FU_GetNewTimestampFilename("gba_tiles_bank0");
    Save_PNG(name_b0,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,buf0,0);

    if(GameBoy.Emulator.CGBEnabled)
    {
        for(i = 0; i < (GBA_TILE_BUFFER_WIDTH*GBA_TILE_BUFFER_HEIGHT); i++)
        {
            buf1[i*4+0] = gba_tile_bank1_buffer[i*3+0];
            buf1[i*4+1] = gba_tile_bank1_buffer[i*3+1];
            buf1[i*4+2] = gba_tile_bank1_buffer[i*3+2];
            buf1[i*4+3] = 255;
        }

        char * name_b1 = FU_GetNewTimestampFilename("gba_tiles_bank1");
        Save_PNG(name_b1,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,buf1,0);
    }


    Win_GBATileViewerUpdate();*/
}

//----------------------------------------------------------------

int Win_GBATileViewerCreate(void)
{
    if(GBATileViewerCreated == 1)
        return 0;

    if(Win_MainRunningGBA() == 0) return 0;

    GUI_SetLabel(&gba_tileview_bank0_label,117,6,GBA_TILE_BUFFER_WIDTH,FONT_HEIGHT,"Bank 0");
    GUI_SetLabel(&gba_tileview_bank1_label,251,6,GBA_TILE_BUFFER_WIDTH,FONT_HEIGHT,"Bank 1");

    GUI_SetTextBox(&gba_tileview_textbox,&gba_tileview_con,
                   6,6, 15*FONT_WIDTH,3*FONT_HEIGHT, NULL);

    GUI_SetBitmap(&gba_tileview_tiles_bmp,117,24,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,gba_tile_buffer,
                  _win_gba_tileviewer_tiles_bmp_callback);

    GUI_SetBitmap(&gba_tileview_zoomed_tile_bmp,6,48, 64,64, gba_tile_zoomed_tile_buffer,
                  NULL);

    GUI_SetButton(&gba_tileview_dumpbtn,106,192,FONT_WIDTH*6,FONT_HEIGHT*2,"Dump",
                  _win_gba_tileviewer_dump_btn_callback);

    GUI_SetScrollBar(&gba_tileview_pal_scrollbar, 6, 250, 200, 12,
                     0,15, 0, _win_gba_tileviewer_pal_scrollbar_callback);
    //GUI_SetScrollBar(&gba_tileview_pal_scrollbar, 6, 200, 12, 200,
    //                 0,15, 0, _win_gba_tileviewer_pal_scrollbar_callback);

    GBATileViewerCreated = 1;

    gba_tileview_selected_pal = 0;
    gba_tileview_selected_index = 0;

    WinIDGBATileViewer = WH_Create(WIN_GBA_TILEVIEWER_WIDTH,WIN_GBA_TILEVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBATileViewer,"GBA Tile Viewer");

    WH_SetEventCallback(WinIDGBATileViewer,_win_gba_tile_viewer_callback);

    Win_GBATileViewerUpdate();
    _win_gba_tile_viewer_render();

    return 1;
}

void Win_GBATileViewerClose(void)
{
    if(GBATileViewerCreated == 0)
        return;

    GBATileViewerCreated = 0;
    WH_Close(WinIDGBATileViewer);
}

//----------------------------------------------------------------

