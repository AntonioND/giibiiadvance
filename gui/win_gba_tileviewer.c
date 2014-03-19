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

#include "../gba_core/gba_debug_video.h"

#include "../png/png_utils.h"

//------------------------------------------------------------------------------------------------

static int WinIDGBATileViewer;

#define WIN_GBA_TILEVIEWER_WIDTH  449
#define WIN_GBA_TILEVIEWER_HEIGHT 268

static int GBATileViewerCreated = 0;

//-----------------------------------------------------------------------------------

static u32 gba_tileview_selected_pal = 0;
static u32 gba_tileview_selected_cbb = 0;
static u32 gba_tileview_selected_colors = 0;
static u32 gba_tileview_selected_index = 0;

#define GBA_TILE_BUFFER_WIDTH  (32*8)
#define GBA_TILE_BUFFER_HEIGHT (32*8)

static char gba_tile_buffer[GBA_TILE_BUFFER_WIDTH*GBA_TILE_BUFFER_HEIGHT*3];

static char gba_tile_zoomed_tile_buffer[64*64*3];

//-----------------------------------------------------------------------------------

static _gui_console gba_tileview_con;
static _gui_element gba_tileview_textbox;

static _gui_element gba_tileview_cbb_label;
static _gui_element gba_tileview_cbb_06000000_radbtn, gba_tileview_cbb_06004000_radbtn,
                    gba_tileview_cbb_06008000_radbtn, gba_tileview_cbb_0600C000_radbtn,
                    gba_tileview_cbb_06010000_radbtn, gba_tileview_cbb_06014000_radbtn;

static _gui_element gba_tileview_colornum_label;
static _gui_element gba_tileview_colornum_16_radbtn, gba_tileview_colornum_256_radbtn;

static _gui_element gba_tileview_zoomed_tile_bmp;

static _gui_element gba_tileview_dumpbtn;

static _gui_element gba_tileview_tiles_bmp;

static _gui_element gba_tileview_pal_scrollbar;

static _gui_element * gba_tileviwer_window_gui_elements[] = {
    &gba_tileview_cbb_label,
    &gba_tileview_cbb_06000000_radbtn, &gba_tileview_cbb_06004000_radbtn,
    &gba_tileview_cbb_06008000_radbtn, &gba_tileview_cbb_0600C000_radbtn,
    &gba_tileview_cbb_06010000_radbtn, &gba_tileview_cbb_06014000_radbtn,
    &gba_tileview_colornum_label,
    &gba_tileview_colornum_16_radbtn, &gba_tileview_colornum_256_radbtn,
    &gba_tileview_zoomed_tile_bmp,
    &gba_tileview_dumpbtn,
    &gba_tileview_textbox,
    &gba_tileview_tiles_bmp,
    &gba_tileview_pal_scrollbar,
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

static void _win_gba_tileviewer_cbb_radbtn_callback(int num)
{
    switch(num)
    {
        case 0: gba_tileview_selected_cbb = 0x06000000; break;
        case 1: gba_tileview_selected_cbb = 0x06004000; break;
        case 2: gba_tileview_selected_cbb = 0x06008000; break;
        case 3: gba_tileview_selected_cbb = 0x0600C000; break;
        case 4: gba_tileview_selected_cbb = 0x06010000; break;
        case 5: gba_tileview_selected_cbb = 0x06014000; break;
        default: gba_tileview_selected_cbb = 0x06000000; break;
    }
}

static void _win_gba_tileviewer_colors_radbtn_callback(int num)
{
    if(num == 1)
        gba_tileview_selected_colors = 256;
    else
        gba_tileview_selected_colors = 16;
}

//----------------------------------------------------------------

void Win_GBATileViewerUpdate(void)
{
    if(GBATileViewerCreated == 0) return;

    if(Win_MainRunningGBA() == 0) return;

    GUI_ConsoleClear(&gba_tileview_con);

    GUI_ConsoleModePrintf(&gba_tileview_con,0,0,"Tile: 789\nAd: 0xFFFFFFFF\nPal: %d",gba_tileview_selected_pal);

    GBA_Debug_PrintTiles(gba_tile_buffer,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,
                         gba_tileview_selected_cbb, gba_tileview_selected_colors,
                         gba_tileview_selected_pal);


    GUI_Draw_SetDrawingColor(255,0,0);
    int l = ((gba_tileview_selected_index%32)*8); //left
    int t = ((gba_tileview_selected_index/32)*8); // top
    int r = l + 7; // right
    int b = t + 7; // bottom
    GUI_Draw_Rect(gba_tile_buffer,GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,l,r,t,b);

/*
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

    GUI_SetLabel(&gba_tileview_cbb_label,6,6,20*FONT_WIDTH,FONT_HEIGHT,"Character Base Block");
    GUI_SetRadioButton(&gba_tileview_cbb_06000000_radbtn,  6,24,25*FONT_WIDTH,18,
                  "0x06000000 - BG         ", 0, 0, 1,_win_gba_tileviewer_cbb_radbtn_callback);
    GUI_SetRadioButton(&gba_tileview_cbb_06004000_radbtn,  6,44,25*FONT_WIDTH,18,
                  "0x06004000 - BG         ", 0, 1, 0,_win_gba_tileviewer_cbb_radbtn_callback);
    GUI_SetRadioButton(&gba_tileview_cbb_06008000_radbtn,  6,64,25*FONT_WIDTH,18,
                  "0x06008000 - BG         ", 0, 2, 0,_win_gba_tileviewer_cbb_radbtn_callback);
    GUI_SetRadioButton(&gba_tileview_cbb_0600C000_radbtn,  6,84,25*FONT_WIDTH,18,
                  "0x0600C000 - BG         ", 0, 3, 0,_win_gba_tileviewer_cbb_radbtn_callback);
    GUI_SetRadioButton(&gba_tileview_cbb_06010000_radbtn,  6,104,25*FONT_WIDTH,18,
                  "0x06010000 - OBJ/BG     ", 0, 4, 0,_win_gba_tileviewer_cbb_radbtn_callback);
    GUI_SetRadioButton(&gba_tileview_cbb_06014000_radbtn,  6,124,25*FONT_WIDTH,18,
                  "0x06014000 - (OBJ)      ", 0, 5, 0,_win_gba_tileviewer_cbb_radbtn_callback);

    GUI_SetLabel(&gba_tileview_colornum_label,6,148,6*FONT_WIDTH,FONT_HEIGHT,"Colors");
    GUI_SetRadioButton(&gba_tileview_colornum_16_radbtn,  6,166,8*FONT_WIDTH,18,
                  "16",  1, 0, 1,_win_gba_tileviewer_colors_radbtn_callback);
    GUI_SetRadioButton(&gba_tileview_colornum_256_radbtn,  12+8*FONT_WIDTH,166,8*FONT_WIDTH,18,
                  "256", 1, 1, 0,_win_gba_tileviewer_colors_radbtn_callback);

    GUI_SetBitmap(&gba_tileview_zoomed_tile_bmp,6,198, 64,64, gba_tile_zoomed_tile_buffer,
                  NULL);

    GUI_SetButton(&gba_tileview_dumpbtn,136,157,FONT_WIDTH*6,FONT_HEIGHT*2,"Dump",
                  _win_gba_tileviewer_dump_btn_callback);

    GUI_SetTextBox(&gba_tileview_textbox,&gba_tileview_con,
                   76,198, 15*FONT_WIDTH,3*FONT_HEIGHT, NULL);

    GUI_SetScrollBar(&gba_tileview_pal_scrollbar, 76,248, 15*FONT_WIDTH, 12,
                     0,15, 0, _win_gba_tileviewer_pal_scrollbar_callback);

    GUI_SetBitmap(&gba_tileview_tiles_bmp,76+15*FONT_WIDTH+6,6,
                  GBA_TILE_BUFFER_WIDTH,GBA_TILE_BUFFER_HEIGHT,gba_tile_buffer,
                  _win_gba_tileviewer_tiles_bmp_callback);

    GBATileViewerCreated = 1;

    gba_tileview_selected_pal = 0;
    gba_tileview_selected_cbb = 0x06000000;
    gba_tileview_selected_colors = 16;
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

#if 0

static void gba_tile_viewer_draw_to_buffer(void)
{
    memset(TileBuffer,0,sizeof(TileBuffer));

    u8 * charbaseblockptr = (u8*)&Mem.vram[SelectedBase-0x06000000];

    int i,j;
    for(i = 0; i < 256; i++) for(j = 0; j < 256; j++)
        TileBuffer[j*256+i] = ((i&16)^(j&16)) ? 0x00808080 : 0x00B0B0B0;

    if(TileViewColors == 256) //256 Colors
    {
        int jmax = (SelectedBase == 0x06014000) ? 64 : 128; //half size

        //SelectedBase >= 0x06010000 --> sprite
        u32 pal = (SelectedBase >= 0x06010000) ? 256 : 0;

        for(i = 0; i < 256; i++) for(j = 0; j < jmax; j++)
        {
            u32 Index = (i/8) + (j/8)*32;
            u8 * dataptr = (u8*)&(charbaseblockptr[(Index&0x3FF)*64]);

            int data = dataptr[(i&7)+((j&7)*8)];

            TileBuffer[j*256+i] = expand16to32(((u16*)Mem.pal_ram)[data+pal]);
        }
        for(i = 0; i < 256; i++) for(j = jmax; j < 256; j++)
        {
            if( ((i^j)&7) == 0 ) TileBuffer[j*256+i] = 0x00FF0000;
        }
    }
    else if(TileViewColors == 16) //16 colors
    {
        int jmax = (SelectedBase == 0x06014000) ? 128 : 256; //half size

        //SelectedBase >= 0x06010000 --> sprite
        u32 pal = (SelectedBase >= 0x06010000) ? (SelectedPalette+16) : SelectedPalette;
        u16 * palptr = (u16*)&Mem.pal_ram[pal*2*16];

        for(i = 0; i < 256; i++) for(j = 0; j < jmax; j++)
        {
            u32 Index = (i/8) + (j/8)*32;
            u8 * dataptr = (u8*)&(charbaseblockptr[(Index&0x3FF)*32]);

            int data = dataptr[ ((i&7)+((j&7)*8))/2 ];

            if(i&1) data = data>>4;
            else data = data & 0xF;

            TileBuffer[j*256+i] = expand16to32(palptr[data]);
        }

        for(i = 0; i < 256; i++) for(j = jmax; j < 256; j++)
        {
            if( ((i^j)&7) == 0 ) TileBuffer[j*256+i] = 0x00FF0000;
        }
    }
}

static void gba_tile_viewer_update_tile(void)
{
    int tiletempbuffer[8*8], tiletempvis[8*8];
    memset(tiletempvis,0,sizeof(tiletempvis));

    u8 * charbaseblockptr = (u8*)&Mem.vram[SelectedBase-0x06000000];

    u32 Index = SelTileX + SelTileY*32;

    if(TileViewColors == 256) //256 Colors
    {
        u8 * data = (u8*)&(charbaseblockptr[(Index&0x3FF)*64]);

        //SelectedBase >= 0x06010000 --> sprite
        u32 pal = (SelectedBase >= 0x06010000) ? 256 : 0;

        int i,j;
        for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
        {
            u8 dat_ = data[j*8 + i];

            tiletempbuffer[j*8 + i] = expand16to32(((u16*)Mem.pal_ram)[dat_+pal]);
            tiletempvis[j*8 + i] = dat_;
        }

        char text[1000];
        sprintf(text,"Tile: %d\r\nAddr: 0x%08X\r\nPal: --",Index&0x3FF, SelectedBase + (Index&0x3FF)*64);
        SetWindowText(hTileText,(LPCTSTR)text);
    }
    else if(TileViewColors == 16)//16 Colors
    {
        u8 * data = (u8*)&(charbaseblockptr[(Index&0x3FF)*32]);

        //SelectedBase >= 0x06010000 --> sprite
        u32 pal = (SelectedBase >= 0x06010000) ? (SelectedPalette+16) : SelectedPalette;
        u16 * palptr = (u16*)&Mem.pal_ram[pal*2*16];

        int i,j;
        for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
        {
            u8 dat_ = data[(j*8 + i)/2];
            if(i&1) dat_ = dat_>>4;
            else dat_ = dat_ & 0xF;

            tiletempbuffer[j*8 + i] = expand16to32(palptr[dat_]);
            tiletempvis[j*8 + i] = dat_;
        }

        char text[1000];
        sprintf(text,"Tile: %d\r\nAddr: 0x%08X\r\nPal: %d",Index&0x3FF,
            SelectedBase + (Index&0x3FF)*32,SelectedPalette);
        SetWindowText(hTileText,(LPCTSTR)text);
    }

    //Expand to 64x64
    int i,j;
    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
        SelectedTileBuffer[j*64+i] = ((i&16)^(j&16)) ? 0x00808080 : 0x00B0B0B0;

    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
    {
        if(tiletempvis[(j/8)*8 + (i/8)])
            SelectedTileBuffer[j*64+i] = tiletempbuffer[(j/8)*8 + (i/8)];
    }

    //Update window
    RECT rc; rc.top = 194; rc.left = 5; rc.bottom = 194+64; rc.right = 5+64;
    InvalidateRect(hWndTileViewer, &rc, FALSE);
}

#endif // 0

