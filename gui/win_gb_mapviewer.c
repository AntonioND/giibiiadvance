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

#include "win_gb_mapviewer.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/debug_video.h"

#include "../png/png_utils.h"

//------------------------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------------------------

static int WinIDGBMapViewer;

#define WIN_GB_MAPVIEWER_WIDTH  407
#define WIN_GB_MAPVIEWER_HEIGHT 268

static int GBMapViewerCreated = 0;

//-----------------------------------------------------------------------------------

static u32 gb_mapview_selected_x = 0;
static u32 gb_mapview_selected_y = 0;

static u32 gb_mapview_selected_tilebase = 0;
static u32 gb_mapview_selected_mapbase = 0;

#define GB_MAP_BUFFER_WIDTH  (32*8)
#define GB_MAP_BUFFER_HEIGHT (32*8)

static char gb_map_buffer[GB_MAP_BUFFER_WIDTH*GB_MAP_BUFFER_HEIGHT*3];

static char gb_map_zoomed_tile_buffer[64*64*3];

//-----------------------------------------------------------------------------------

static _gui_console gb_mapview_con;
static _gui_element gb_mapview_textbox;

static _gui_element gb_mapview_dumpbtn;

static _gui_element gb_mapview_map_bmp;

static _gui_element gb_mapview_tilebase_label, gb_mapview_mapbase_label;

static _gui_element gb_mapview_tilebase8000_radbtn, gb_mapview_tilebase8800_radbtn;
static _gui_element gb_mapview_mapbase9800_radbtn, gb_mapview_mapbase9C00_radbtn;

static _gui_element gb_mapview_zoomed_tile_bmp;

static _gui_element * gb_tileviwer_window_gui_elements[] = {
    &gb_mapview_tilebase_label,
    &gb_mapview_mapbase_label,
    &gb_mapview_tilebase8000_radbtn,
    &gb_mapview_tilebase8800_radbtn,
    &gb_mapview_mapbase9800_radbtn,
    &gb_mapview_mapbase9C00_radbtn,
    &gb_mapview_map_bmp,
    &gb_mapview_zoomed_tile_bmp,
    &gb_mapview_textbox,
    &gb_mapview_dumpbtn,
    NULL
};

static _gui gb_mapviewer_window_gui = {
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

static int _win_gb_mapviewer_map_bmp_callback(int x, int y)
{
    gb_mapview_selected_x = x/8;
    gb_mapview_selected_y = y/8;
    return 1;
}

//----------------------------------------------------------------

void Win_GBMapViewerUpdate(void)
{
    if(GBMapViewerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    GUI_ConsoleClear(&gb_mapview_con);

    _GB_MEMORY_ * mem = &GameBoy.Memory;

    //Update text information
    u8 * bgtilemap = gb_mapview_selected_mapbase ? &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];
    u32 tile_location = (gb_mapview_selected_y * 32) + gb_mapview_selected_x;
    int tile = bgtilemap[tile_location];
    int info = GameBoy.Emulator.CGBEnabled ? bgtilemap[tile_location + 0x2000] : 0;
    u32 addr = (gb_mapview_selected_mapbase?0x9C00:0x9800)+(gb_mapview_selected_y*32)+gb_mapview_selected_x;
    int bank = (info&(1<<3)) != 0;

    GUI_ConsoleModePrintf(&gb_mapview_con,0,0,
            "Pos: %d,%d\nAddr: 0x%04X\nTile: %d (Bank %d)\nFlip: %s%s\nPal: %d\nPriority: %d",
            gb_mapview_selected_x,gb_mapview_selected_y,addr,tile,bank,(info & (1<<6)) ? "V" : "-",
            (info & (1<<5)) ? "H" : "-",info&7,(info & (1<<7)) != 0);

    GB_Debug_MapPrint(gb_map_buffer,GB_MAP_BUFFER_WIDTH,GB_MAP_BUFFER_HEIGHT,gb_mapview_selected_mapbase,
                      gb_mapview_selected_tilebase);
    GUI_Draw_SetDrawingColor(255,0,0);
    int l = gb_mapview_selected_x*8; //left
    int t = gb_mapview_selected_y*8; // top
    int r = l + 7; // right
    int b = t + 7; // bottom
    GUI_Draw_Rect(gb_map_buffer,GB_MAP_BUFFER_WIDTH,GB_MAP_BUFFER_HEIGHT,l,r,t,b);

    if(gb_mapview_selected_tilebase) //If tile base is 0x8800
    {
        if(tile & (1<<7)) tile &= 0x7F;
        else tile += 128;
        tile += 128;
    }

    GB_Debug_TileDrawZoomed64x64(gb_map_zoomed_tile_buffer,tile,bank);
}

//----------------------------------------------------------------

static void _win_gb_map_viewer_render(void)
{
    if(GBMapViewerCreated == 0) return;

    char buffer[WIN_GB_MAPVIEWER_WIDTH*WIN_GB_MAPVIEWER_HEIGHT*3];
    GUI_Draw(&gb_mapviewer_window_gui,buffer,WIN_GB_MAPVIEWER_WIDTH,WIN_GB_MAPVIEWER_HEIGHT,1);

    WH_Render(WinIDGBMapViewer, buffer);
}

static int _win_gb_map_viewer_callback(SDL_Event * e)
{
    if(GBMapViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gb_mapviewer_window_gui,e);

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
        GBMapViewerCreated = 0;
        WH_Close(WinIDGBMapViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBMapViewerUpdate();
        _win_gb_map_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gb_map_viewer_tilebase_radbtn_callback(int btn_id)
{
    gb_mapview_selected_tilebase = btn_id;
    Win_GBMapViewerUpdate();
}

static void _win_gb_map_viewer_mapbase_radbtn_callback(int btn_id)
{
    gb_mapview_selected_mapbase = btn_id;
    Win_GBMapViewerUpdate();
}

//----------------------------------------------------------------

static void _win_gb_mapviewer_dump_btn_callback(void)
{
    if(Win_MainRunningGB() == 0) return;

    GB_Debug_MapPrint(gb_map_buffer,GB_MAP_BUFFER_WIDTH,GB_MAP_BUFFER_HEIGHT,gb_mapview_selected_mapbase,
                      gb_mapview_selected_tilebase);

    char buffer_temp[GB_MAP_BUFFER_WIDTH*GB_MAP_BUFFER_HEIGHT*4];

    char * src = gb_map_buffer;
    char * dst = buffer_temp;
    int i;
    for(i = 0; i < GB_MAP_BUFFER_WIDTH*GB_MAP_BUFFER_HEIGHT; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xFF;
    }

    char * name = FU_GetNewTimestampFilename("gb_map");
    Save_PNG(name,GB_MAP_BUFFER_WIDTH,GB_MAP_BUFFER_HEIGHT,buffer_temp,0);

    Win_GBMapViewerUpdate();
}

//----------------------------------------------------------------

int Win_GBMapViewerCreate(void)
{
    if(GBMapViewerCreated == 1)
        return 0;

    if(Win_MainRunningGB() == 0) return 0;

    GUI_SetLabel(&gb_mapview_tilebase_label,6,6,9*FONT_WIDTH,FONT_HEIGHT,"Tile Base");

    GUI_SetRadioButton(&gb_mapview_tilebase8000_radbtn,  6,24,8*FONT_WIDTH,24,
                  "0x8000", 0, 0, 1,_win_gb_map_viewer_tilebase_radbtn_callback);
    GUI_SetRadioButton(&gb_mapview_tilebase8800_radbtn,  12+8*FONT_WIDTH,24,8*FONT_WIDTH,24,
                  "0x8800", 0, 1, 0,_win_gb_map_viewer_tilebase_radbtn_callback);

    GUI_SetLabel(&gb_mapview_mapbase_label,6,54,8*FONT_WIDTH,FONT_HEIGHT,"Map Base");

    GUI_SetRadioButton(&gb_mapview_mapbase9800_radbtn,  6,72,8*FONT_WIDTH,24,
                  "0x9800", 1, 0, 1,_win_gb_map_viewer_mapbase_radbtn_callback);
    GUI_SetRadioButton(&gb_mapview_mapbase9C00_radbtn,  12+8*FONT_WIDTH,72,8*FONT_WIDTH,24,
                  "0x9C00", 1, 1, 0,_win_gb_map_viewer_mapbase_radbtn_callback);

    GUI_SetTextBox(&gb_mapview_textbox,&gb_mapview_con,
                   6,108, 19*FONT_WIDTH,6*FONT_HEIGHT, NULL);

    GUI_SetBitmap(&gb_mapview_map_bmp,145,6,GB_MAP_BUFFER_WIDTH,GB_MAP_BUFFER_HEIGHT,gb_map_buffer,
                  _win_gb_mapviewer_map_bmp_callback);

    GUI_SetBitmap(&gb_mapview_zoomed_tile_bmp,6,198, 64,64, gb_map_zoomed_tile_buffer,
                  NULL);

    GUI_SetButton(&gb_mapview_dumpbtn,87,238,FONT_WIDTH*6,FONT_HEIGHT*2,"Dump",
                  _win_gb_mapviewer_dump_btn_callback);

    GBMapViewerCreated = 1;

    gb_mapview_selected_x = 0;
    gb_mapview_selected_y = 0;

    WinIDGBMapViewer = WH_Create(WIN_GB_MAPVIEWER_WIDTH,WIN_GB_MAPVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBMapViewer,"GB Map Viewer");

    WH_SetEventCallback(WinIDGBMapViewer,_win_gb_map_viewer_callback);

    Win_GBMapViewerUpdate();
    _win_gb_map_viewer_render();

    return 1;
}

void Win_GBMapViewerClose(void)
{
    if(GBMapViewerCreated == 0)
        return;

    GBMapViewerCreated = 0;
    WH_Close(WinIDGBMapViewer);
}

//----------------------------------------------------------------
