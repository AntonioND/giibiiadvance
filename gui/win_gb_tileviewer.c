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

#include "win_gb_tileviewer.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/debug_video.h"

#include "../png/png_utils.h"

//------------------------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------------------------

static int WinIDGBTileViewer;

#define WIN_GB_TILEVIEWER_WIDTH  385
#define WIN_GB_TILEVIEWER_HEIGHT 222

static int GBTileViewerCreated = 0;

//-----------------------------------------------------------------------------------

static u32 gb_tileview_selected_bank = 0;
static u32 gb_tileview_selected_index = 0;

#define GB_TILE_BUFFER_WIDTH (16*8)
#define GB_TILE_BUFFER_HEIGHT (24*8)

static char gb_tile_bank0_buffer[GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT*3];
static char gb_tile_bank1_buffer[GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT*3];

static char gb_tile_zoomed_tile_buffer[64*64*3];

//-----------------------------------------------------------------------------------

static _gui_console gb_tileview_con;
static _gui_element gb_tileview_textbox;

static _gui_element gb_tileview_dumpbtn;

static _gui_element gb_tileview_bank0_bmp, gb_tileview_bank1_bmp;
static _gui_element gb_tileview_bank0_label, gb_tileview_bank1_label;

static _gui_element gb_tileview_zoomed_tile_bmp;

static _gui_element * gb_tileviwer_window_gui_elements[] = {
    &gb_tileview_bank0_label,
    &gb_tileview_bank1_label,
    &gb_tileview_bank0_bmp,
    &gb_tileview_bank1_bmp,
    &gb_tileview_zoomed_tile_bmp,
    &gb_tileview_textbox,
    &gb_tileview_dumpbtn,
    NULL
};

static _gui gb_tileviewer_window_gui = {
    gb_tileviwer_window_gui_elements,
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

static int _win_gb_tileviewer_bank0_bmp_callback(int x, int y)
{
    gb_tileview_selected_bank = 0;
    gb_tileview_selected_index = (x/8) + ((y/8)*16);
    return 1;
}

static int _win_gb_tileviewer_bank1_bmp_callback(int x, int y)
{
    gb_tileview_selected_bank = 1;
    gb_tileview_selected_index = (x/8) + ((y/8)*16);
    return 1;
}

//----------------------------------------------------------------

void Win_GBTileViewerUpdate(void)
{
    if(GBTileViewerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    GUI_ConsoleClear(&gb_tileview_con);

    GB_Debug_TileVRAMDraw(gb_tile_bank0_buffer,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,
                          gb_tile_bank1_buffer,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT);

    GUI_Draw_SetDrawingColor(255,0,0);
    char * buf;
    if(gb_tileview_selected_bank == 0)
        buf = gb_tile_bank0_buffer;
    else
        buf = gb_tile_bank1_buffer;
    int l = ((gb_tileview_selected_index%16)*8); //left
    int t = ((gb_tileview_selected_index/16)*8); // top
    int r = l + 7; // right
    int b = t + 7; // bottom
    GUI_Draw_Rect(buf,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,l,r,t,b);

    GB_Debug_TileDrawZoomed64x64(gb_tile_zoomed_tile_buffer, gb_tileview_selected_index, gb_tileview_selected_bank);

    u32 tile = gb_tileview_selected_index;
    u32 tileindex = (tile > 255) ? (tile - 256) : (tile);
    if(GameBoy.Emulator.CGBEnabled)
    {
        GUI_ConsoleModePrintf(&gb_tileview_con,0,0,"Tile: %d(%d)\nAddr: 0x%04X\nBank: %d",tile,tileindex,
            0x8000 + (tile * 16),gb_tileview_selected_bank);
    }
    else
    {
        GUI_ConsoleModePrintf(&gb_tileview_con,0,0,"Tile: %d(%d)\nAddr: 0x%04X\nBank: -",tile,tileindex,
            0x8000 + (tile * 16));
    }
}

//----------------------------------------------------------------

void Win_GBTileViewerRender(void)
{
    if(GBTileViewerCreated == 0) return;

    char buffer[WIN_GB_TILEVIEWER_WIDTH*WIN_GB_TILEVIEWER_HEIGHT*3];
    GUI_Draw(&gb_tileviewer_window_gui,buffer,WIN_GB_TILEVIEWER_WIDTH,WIN_GB_TILEVIEWER_HEIGHT,1);

    WH_Render(WinIDGBTileViewer, buffer);
}

int Win_GBTileViewerCallback(SDL_Event * e)
{
    if(GBTileViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gb_tileviewer_window_gui,e);

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
        GBTileViewerCreated = 0;
        WH_Close(WinIDGBTileViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBTileViewerUpdate();
        Win_GBTileViewerRender();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gb_tileviewer_dump_btn_callback(void)
{
    if(Win_MainRunningGB() == 0) return;

    GB_Debug_TileVRAMDraw(gb_tile_bank0_buffer,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,
                          gb_tile_bank1_buffer,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT);

    char buf0[GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT*4];
    char buf1[GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT*4];

    int i;
    for(i = 0; i < (GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT); i++)
    {
        buf0[i*4+0] = gb_tile_bank0_buffer[i*3+0];
        buf0[i*4+1] = gb_tile_bank0_buffer[i*3+1];
        buf0[i*4+2] = gb_tile_bank0_buffer[i*3+2];
        buf0[i*4+3] = 255;
    }

    char * name_b0 = FU_GetNewTimestampFilename("gb_tiles_bank0");
    Save_PNG(name_b0,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,buf0,0);

    if(GameBoy.Emulator.CGBEnabled)
    {
        for(i = 0; i < (GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT); i++)
        {
            buf1[i*4+0] = gb_tile_bank1_buffer[i*3+0];
            buf1[i*4+1] = gb_tile_bank1_buffer[i*3+1];
            buf1[i*4+2] = gb_tile_bank1_buffer[i*3+2];
            buf1[i*4+3] = 255;
        }

        char * name_b1 = FU_GetNewTimestampFilename("gb_tiles_bank1");
        Save_PNG(name_b1,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,buf1,0);
    }


    Win_GBTileViewerUpdate();
}

//----------------------------------------------------------------

int Win_GBTileViewerCreate(void)
{
    if(GBTileViewerCreated == 1)
        return 0;

    if(Win_MainRunningGB() == 0) return 0;

    GUI_SetLabel(&gb_tileview_bank0_label,117,6,GB_TILE_BUFFER_WIDTH,FONT_12_HEIGHT,"Bank 0");
    GUI_SetLabel(&gb_tileview_bank1_label,251,6,GB_TILE_BUFFER_WIDTH,FONT_12_HEIGHT,"Bank 1");

    GUI_SetTextBox(&gb_tileview_textbox,&gb_tileview_con,
                   6,6, 15*FONT_12_WIDTH,3*FONT_12_HEIGHT, NULL);

    GUI_SetBitmap(&gb_tileview_bank0_bmp,117,24,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,gb_tile_bank0_buffer,
                  _win_gb_tileviewer_bank0_bmp_callback);
    GUI_SetBitmap(&gb_tileview_bank1_bmp,251,24,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,gb_tile_bank1_buffer,
                  _win_gb_tileviewer_bank1_bmp_callback);

    GUI_SetBitmap(&gb_tileview_zoomed_tile_bmp,6,48, 64,64, gb_tile_zoomed_tile_buffer,
                  NULL);

    GUI_SetButton(&gb_tileview_dumpbtn,6,192,FONT_12_WIDTH*6,FONT_12_HEIGHT*2,"Dump",
                  _win_gb_tileviewer_dump_btn_callback);

    GBTileViewerCreated = 1;

    gb_tileview_selected_bank = 0;
    gb_tileview_selected_index = 0;

    WinIDGBTileViewer = WH_Create(WIN_GB_TILEVIEWER_WIDTH,WIN_GB_TILEVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBTileViewer,"GB Tile Viewer");

    WH_SetEventCallback(WinIDGBTileViewer,Win_GBTileViewerCallback);

    Win_GBTileViewerUpdate();
    Win_GBTileViewerRender();

    return 1;
}

