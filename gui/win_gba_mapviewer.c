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
#include <malloc.h>

#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../general_utils.h"
#include "../file_utils.h"

#include "win_gba_mapviewer.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gba_core/gba_debug_video.h"
#include "../gba_core/memory.h"

#include "../png/png_utils.h"

//------------------------------------------------------------------------------------------------

static int WinIDGBAMapViewer;

#define WIN_GBA_MAPVIEWER_WIDTH  449
#define WIN_GBA_MAPVIEWER_HEIGHT 268

static int GBAMapViewerCreated = 0;

//-----------------------------------------------------------------------------------

static u32 gba_mapview_selected_bg = 0;
static u32 gba_mapview_selected_tilex = 0;
static u32 gba_mapview_selected_tiley = 0;
static u32 gba_mapview_selected_bitmap_page = 0;

static u32 gba_mapview_sizex = 0;
static u32 gba_mapview_sizey = 0;
static u32 gba_mapview_bgmode = 0;
static u32 gba_mapview_bgcontrolreg = 0;

#define GBA_MAP_BUFFER_WIDTH  (32*8)
#define GBA_MAP_BUFFER_HEIGHT (32*8)

static char gba_complete_map_buffer[1024*1024*4]; // complete map it is copied to gba_map_buffer[] as needed
static char gba_map_buffer[GBA_MAP_BUFFER_WIDTH*GBA_MAP_BUFFER_HEIGHT*3];

static char gba_map_zoomed_tile_buffer[64*64*3];

//-----------------------------------------------------------------------------------

static _gui_console gba_mapview_con;
static _gui_element gba_mapview_textbox;

static _gui_console gba_mapview_tileinfo_con;
static _gui_element gba_mapview_tileinfo_textbox;

static _gui_element gba_mapview_bg_label;
static _gui_element gba_mapview_bg0_radbtn, gba_mapview_bg1_radbtn,
                    gba_mapview_bg2_radbtn, gba_mapview_bg3_radbtn;

static _gui_element gba_mapview_page_label;
static _gui_element gba_mapview_page0_radbtn, gba_mapview_page1_radbtn;

static _gui_element gba_mapview_zoomed_tile_bmp;

static _gui_element gba_mapview_dumpbtn;

static _gui_element gba_mapview_tiles_bmp;

static _gui_element gba_mapview_pal_scrollbar;

static _gui_element * gba_mapviwer_window_gui_elements[] = {
    &gba_mapview_bg_label,
    &gba_mapview_bg0_radbtn, &gba_mapview_bg1_radbtn,
    &gba_mapview_bg2_radbtn, &gba_mapview_bg3_radbtn,
    &gba_mapview_page_label,
    &gba_mapview_page0_radbtn, &gba_mapview_page1_radbtn,
    &gba_mapview_zoomed_tile_bmp,
    &gba_mapview_dumpbtn,
    &gba_mapview_textbox,
    &gba_mapview_tileinfo_textbox,
    &gba_mapview_tiles_bmp,
    &gba_mapview_pal_scrollbar,
    NULL
};

static _gui gba_mapviewer_window_gui = {
    gba_mapviwer_window_gui_elements,
    NULL,
    NULL
};

//----------------------------------------------------------------

static int _win_gba_mapviewer_tiles_bmp_callback(int x, int y)
{
    //gba_mapview_sizex = 0;
    //static u32 gba_mapview_sizey
    //gba_mapview_selected_tile_index = (x/8) + ((y/8)*32);
    return 1;
}

static void _win_gba_mapviewer_pal_scrollbar_callback(int value)
{
    return;
}

static void _win_gba_mapviewer_bgnum_radbtn_callback(int num)
{
    gba_mapview_selected_bg = num;
}

static void _win_gba_mapviewer_pagenum_radbtn_callback(int num)
{
    gba_mapview_selected_bitmap_page = num;
}

//----------------------------------------------------------------

void Win_GBAMapViewerUpdate(void)
{
    if(GBAMapViewerCreated == 0) return;

    if(Win_MainRunningGBA() == 0) return;

    GUI_ConsoleClear(&gba_mapview_con);
    GUI_ConsoleClear(&gba_mapview_tileinfo_con);

    u16 control = 0;
    switch(gba_mapview_selected_bg)
    {
        case 0: control = REG_BG0CNT; break;
        case 1: control = REG_BG1CNT; break;
        case 2: control = REG_BG2CNT; break;
        case 3: control = REG_BG3CNT; break;
        default: return;
    }

    u32 mode = REG_DISPCNT & 0x7;
    static const u32 bgtypearray[8][4] = {  //1 = text, 2 = affine, 3,4,5 = bmp mode 3,4,5
        {1,1,1,1}, {1,1,2,0}, {0,0,2,2}, {0,0,3,0}, {0,0,4,0}, {0,0,5,0}, {0,0,0,0}, {0,0,0,0}
    };
    gba_mapview_bgmode = bgtypearray[mode][gba_mapview_selected_bg];

    gba_mapview_bgcontrolreg = control;
    gba_mapview_selected_tilex = 0; gba_mapview_selected_tiley = 0;

    if(gba_mapview_bgmode == 0) //Shouldn't get here...
    {
        gba_mapview_sizex = gba_mapview_sizey = 0;
        GUI_ConsoleModePrintf(&gba_mapview_con,0,0,"Size: ------- (Unused)\nColors: ---\n"
                              "Priority: -\nWrap: -\nChar Base: ----------\nScrn Base: ----------\nMosaic: -");
        GUI_ConsoleModePrintf(&gba_mapview_tileinfo_con,0,0,"Tile: --- (--)\nPos: ---------\nPal: --\n"
                              "Addr: ----------\n");
/*
        SCROLLINFO si; ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        si.nMin = 0; si.nMax = 0; si.nPos = 0; si.nPage = 1;
        SetScrollInfo(hWndMapScrollH, SB_CTL, &si, TRUE);
        SetScrollInfo(hWndMapScrollV, SB_CTL, &si, TRUE);
*/
    }
    else if(gba_mapview_bgmode == 1) //text
    {
        static const u32 text_bg_size[4][2] = { {256,256}, {512,256}, {256,512}, {512,512} };
        u32 CBB = ( ((control>>2)&3) * (16*1024) ) + 0x06000000;
        u32 SBB = ( ((control>>8)&0x1F) * (2*1024) ) + 0x06000000;
        gba_mapview_sizex = text_bg_size[control>>14][0];
        gba_mapview_sizey = text_bg_size[control>>14][1];
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int colors = (control & BIT(7)) ? 256 : 16;
        int prio = control&3;
        GUI_ConsoleModePrintf(&gba_mapview_con,0,0,"Size: %dx%d (Text)\nColors: %d\nPriority: %d\nWrap: 1\n"
                              "Char Base: 0x%08X\nScrn Base: 0x%08X\nMosaic: %d",
                              gba_mapview_sizex,gba_mapview_sizey,colors,prio,CBB,SBB, mosaic);
    }
    else if(gba_mapview_bgmode == 2) //affine
    {
        static const u32 affine_bg_size[4] = { 128, 256, 512, 1024 };
        u32 CBB = ( ((control>>2)&3) * (16*1024) ) + 0x06000000;
        u32 SBB = ( ((control>>8)&0x1F) * (2*1024) ) + 0x06000000;
        gba_mapview_sizex = affine_bg_size[control>>14];
        gba_mapview_sizey = gba_mapview_sizex;
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int wrap = (control & BIT(13)) != 0;
        int prio = control&3;
        GUI_ConsoleModePrintf(&gba_mapview_con,0,0,"Size: %dx%d (Affine)\nColors: 256\nPriority: %d\nWrap: %d\n"
                              "Char Base: 0x%08X\nScrn Base: 0x%08X\nMosaic: %d",
                              gba_mapview_sizex,gba_mapview_sizey,prio,wrap,CBB,SBB, mosaic);
    }
    else if(gba_mapview_bgmode == 3) //bg2 mode 3
    {
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int prio = control&3;
        gba_mapview_sizex = 240; gba_mapview_sizey = 160;
        GUI_ConsoleModePrintf(&gba_mapview_con,0,0,"Size: 240x160 (Bitmap)\nColors: 16bit\nPriority: %d\nWrap: 0\n"
                              "Char Base: ----------\nScrn Base: 0x06000000\nMosaic: %d",prio,mosaic);
        GUI_ConsoleModePrintf(&gba_mapview_tileinfo_con,0,0,"Tile: --- (--)\nPos: ---------\nPal: --\n"
                              "Addr: ----------\n");
    }
    else if(gba_mapview_bgmode == 4) //bg2 mode 4
    {
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int prio = control&3;
        u32 SBB = 0x06000000 + ((REG_DISPCNT&BIT(4))?0xA000:0);
        gba_mapview_sizex = 240; gba_mapview_sizey = 160;
        GUI_ConsoleModePrintf(&gba_mapview_con,0,0,"Size: 240x160 (Bitmap)\nColors: 256\nPriority: %d\nWrap: 0\n"
                              "Char Base: ----------\nScrn Base: 0x%08X\nMosaic: %d",prio,SBB,mosaic);
        GUI_ConsoleModePrintf(&gba_mapview_tileinfo_con,0,0,"Tile: --- (--)\nPos: ---------\nPal: --\n"
                              "Addr: ----------\n");
    }
    else if(gba_mapview_bgmode == 5) //bg2 mode 5
    {
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int prio = control&3;
        u32 SBB = 0x06000000 + ((REG_DISPCNT&BIT(4))?0xA000:0);
        gba_mapview_sizex = 160; gba_mapview_sizey = 128;
        GUI_ConsoleModePrintf(&gba_mapview_con,0,0,"Size: 160x128 (Bitmap)\nColors: 16bit\nPriority: %d\nWrap: 0\n"
                              "Char Base: ----------\nScrn Base: 0x%08X\nMosaic: %d",prio,SBB,mosaic);
        GUI_ConsoleModePrintf(&gba_mapview_tileinfo_con,0,0,"Tile: --- (--)\nPos: ---------\nPal: --\n"
                              "Addr: ----------\n");
    }
/*
    gba_map_viewer_draw_to_buffer(control,bgmode);
    gba_map_viewer_update_scrollbars();
    gba_map_update_window_from_temp_buffer();
    gba_map_viewer_update_tile();
    */

/*
    u32 address = (gba_mapview_selected_colors == 16) ?
            ( gba_mapview_selected_cbb + (gba_mapview_selected_index&0x3FF)*32 ) :
            ( gba_mapview_selected_cbb + (gba_mapview_selected_index&0x3FF)*64 ) ;

    GUI_ConsoleModePrintf(&gba_mapview_con,0,0,"Tile: %d\nAddr: %08X\nPal: %d",
                          gba_mapview_selected_index, address,
                          gba_mapview_selected_pal);

    GBA_Debug_PrintTiles(gba_map_buffer,GBA_MAP_BUFFER_WIDTH,GBA_MAP_BUFFER_HEIGHT,
                         gba_mapview_selected_cbb, gba_mapview_selected_colors,
                         gba_mapview_selected_pal);

    GUI_Draw_SetDrawingColor(255,0,0);
    int l = ((gba_mapview_selected_index%32)*8); //left
    int t = ((gba_mapview_selected_index/32)*8); // top
    int r = l + 7; // right
    int b = t + 7; // bottom
    GUI_Draw_Rect(gba_map_buffer,GBA_MAP_BUFFER_WIDTH,GBA_MAP_BUFFER_HEIGHT,l,r,t,b);

    GBA_Debug_TilePrint64x64(gba_map_zoomed_tile_buffer, 64,64,
                             gba_mapview_selected_cbb, gba_mapview_selected_index,
                             gba_mapview_selected_colors, gba_mapview_selected_pal);*/
}

//----------------------------------------------------------------

static void _win_gba_map_viewer_render(void)
{
    if(GBAMapViewerCreated == 0) return;

    char buffer[WIN_GBA_MAPVIEWER_WIDTH*WIN_GBA_MAPVIEWER_HEIGHT*3];
    GUI_Draw(&gba_mapviewer_window_gui,buffer,WIN_GBA_MAPVIEWER_WIDTH,WIN_GBA_MAPVIEWER_HEIGHT,1);

    WH_Render(WinIDGBAMapViewer, buffer);
}

static int _win_gba_map_viewer_callback(SDL_Event * e)
{
    if(GBAMapViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gba_mapviewer_window_gui,e);

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
        GBAMapViewerCreated = 0;
        WH_Close(WinIDGBAMapViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBAMapViewerUpdate();
        _win_gba_map_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gba_mapviewer_dump_btn_callback(void)
{
    if(Win_MainRunningGBA() == 0) return;
/*
    char * buf = malloc(GBA_MAP_BUFFER_WIDTH*GBA_MAP_BUFFER_HEIGHT*4);
    if(buf == NULL)
        return;

    GBA_Debug_PrintTilesAlpha(buf,GBA_MAP_BUFFER_WIDTH,GBA_MAP_BUFFER_HEIGHT,
                         gba_mapview_selected_cbb, gba_mapview_selected_colors,
                         gba_mapview_selected_pal);

    char * name = FU_GetNewTimestampFilename("gba_maps");
    Save_PNG(name,GBA_MAP_BUFFER_WIDTH,GBA_MAP_BUFFER_HEIGHT,buf,1);

    free(buf);
*/
    Win_GBAMapViewerUpdate();
}

//----------------------------------------------------------------

int Win_GBAMapViewerCreate(void)
{
    if(GBAMapViewerCreated == 1)
        return 0;

    if(Win_MainRunningGBA() == 0) return 0;

    GUI_SetLabel(&gba_mapview_bg_label,6,6,10*FONT_WIDTH,FONT_HEIGHT,"Background");
    GUI_SetRadioButton(&gba_mapview_bg0_radbtn,  6,24,15*FONT_WIDTH,18,
                  "Background 0", 0, 0, 1,_win_gba_mapviewer_bgnum_radbtn_callback);
    GUI_SetRadioButton(&gba_mapview_bg1_radbtn,  6,44,15*FONT_WIDTH,18,
                  "Background 1", 0, 1, 0,_win_gba_mapviewer_bgnum_radbtn_callback);
    GUI_SetRadioButton(&gba_mapview_bg2_radbtn,  6,64,15*FONT_WIDTH,18,
                  "Background 2", 0, 2, 0,_win_gba_mapviewer_bgnum_radbtn_callback);
    GUI_SetRadioButton(&gba_mapview_bg3_radbtn,  6,84,15*FONT_WIDTH,18,
                  "Background 3", 0, 3, 0,_win_gba_mapviewer_bgnum_radbtn_callback);

    GUI_SetLabel(&gba_mapview_page_label,12+15*FONT_WIDTH,6,11*FONT_WIDTH,FONT_HEIGHT,"Bitmap Page");
    GUI_SetRadioButton(&gba_mapview_page0_radbtn,  12+15*FONT_WIDTH,24,8*FONT_WIDTH,18,
                  "Page 0",  1, 0, 1,_win_gba_mapviewer_pagenum_radbtn_callback);
    GUI_SetRadioButton(&gba_mapview_page1_radbtn,  12+15*FONT_WIDTH,44,8*FONT_WIDTH,18,
                  "Page 1", 1, 1, 0,_win_gba_mapviewer_pagenum_radbtn_callback);

    //***********************************************
    // ENABLE / DISABLE RADIO BUTTONS!!!!!!!
    //***********************************************

    GUI_SetBitmap(&gba_mapview_zoomed_tile_bmp,6,198, 64,64, gba_map_zoomed_tile_buffer,
                  NULL);

    GUI_SetButton(&gba_mapview_dumpbtn,19+15*FONT_WIDTH,70,FONT_WIDTH*6,FONT_HEIGHT*2,"Dump",
                  _win_gba_mapviewer_dump_btn_callback);

    GUI_SetTextBox(&gba_mapview_textbox,&gba_mapview_con,
                   6,108, 25*FONT_WIDTH,7*FONT_HEIGHT, NULL);

    GUI_SetTextBox(&gba_mapview_tileinfo_textbox,&gba_mapview_tileinfo_con,
                   76,198, 20*FONT_WIDTH,4*FONT_HEIGHT, NULL);

    GUI_SetScrollBar(&gba_mapview_pal_scrollbar, 76,248, 15*FONT_WIDTH, 12,
                     0,15, 0, _win_gba_mapviewer_pal_scrollbar_callback);

    GUI_SetBitmap(&gba_mapview_tiles_bmp,76+15*FONT_WIDTH+6,6,
                  GBA_MAP_BUFFER_WIDTH,GBA_MAP_BUFFER_HEIGHT,gba_map_buffer,
                  _win_gba_mapviewer_tiles_bmp_callback);

    GBAMapViewerCreated = 1;


    WinIDGBAMapViewer = WH_Create(WIN_GBA_MAPVIEWER_WIDTH,WIN_GBA_MAPVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBAMapViewer,"GBA Map Viewer");

    WH_SetEventCallback(WinIDGBAMapViewer,_win_gba_map_viewer_callback);

    Win_GBAMapViewerUpdate();
    _win_gba_map_viewer_render();

    return 1;
}

void Win_GBAMapViewerClose(void)
{
    if(GBAMapViewerCreated == 0)
        return;

    GBAMapViewerCreated = 0;
    WH_Close(WinIDGBAMapViewer);
}

//----------------------------------------------------------------


#if 0


static HWND hWndMapViewer;
static int MapViewerCreated = 0;
static int MapBuffer[256*256], MapTileBuffer[64*64];
static HWND hWndMapScrollH, hWndMapScrollV;
static u32 SizeX, SizeY;
static u32 BgMode; static u16 BgControl;
static u32 TileX,TileY;
static HWND hMapText, hMapTileText;
static int SelectedLayer;
static int tempmapbuffer[1024*1024], tempvismapbuffer[1024*1024];
#define IDC_BG0    1
#define IDC_BG1    2
#define IDC_BG2    3
#define IDC_BG3    4

static inline u32 se_index(u32 tx, u32 ty, u32 pitch) //from tonc
{
    u32 sbb = (ty/32)*(pitch/32) + (tx/32);
    return sbb*1024 + (ty%32)*32 + tx%32;
}
static inline u32 se_index_affine(u32 tx, u32 ty, u32 tpitch)
{
    return (ty * tpitch) + tx;
}

static void gba_map_update_window_from_temp_buffer(void)
{
    //Copy to real buffer
    int i,j;
    for(i = 0; i < 256; i++) for(j = 0; j < 256; j++)
        MapBuffer[j*256+i] = ((i&32)^(j&32)) ? 0x00808080 : 0x00B0B0B0;

    int scrollx = GetScrollPos(hWndMapScrollH, SB_CTL);
    int scrolly = GetScrollPos(hWndMapScrollV, SB_CTL);

    for(i = 0; i < 256; i++) for(j = 0; j < 256; j++)
    {
        if( (i>=SizeX) || (j>=SizeY) )
        {
            if( ((i^j)&7) == 0 ) MapBuffer[j*256+i] = 0x00FF0000;
        }
        else
        {
            if(tempvismapbuffer[(j+scrolly)*1024 + (i+scrollx)])
                MapBuffer[j*256+i] = tempmapbuffer[(j+scrolly)*1024 + (i+scrollx)];
        }
    }

    RECT rc; rc.top = 5; rc.left = 200; rc.bottom = 5+256; rc.right = 200+256;
    InvalidateRect(hWndMapViewer, &rc, FALSE);
}

static void gba_map_viewer_update_scrollbars(void)
{
    SCROLLINFO si; ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_POS;
    si.nMin = 0; si.nMax = max(0,SizeX-256); si.nPos = 0;
    SetScrollInfo(hWndMapScrollH, SB_CTL, &si, TRUE);
    si.nMin = 0; si.nMax = max(0,SizeY-256); si.nPos = 0;
    SetScrollInfo(hWndMapScrollV, SB_CTL, &si, TRUE);
}

static void gba_map_viewer_draw_to_buffer(u16 control, int bgmode) //1 = text, 2 = affine, 3,4,5 = bmp mode 3,4,5
{
    if(bgmode == 0) return; //how the hell did this function get called with bgmode = 0 ???

    memset(tempvismapbuffer,0,sizeof(tempvismapbuffer));

    if(bgmode == 1) //text
    {
        static const u32 text_bg_size[4][2] = { {256,256}, {512,256}, {256,512}, {512,512} };

        u8 * charbaseblockptr = (u8*)&Mem.vram[((control>>2)&3) * (16*1024)];
        u16 * scrbaseblockptr = (u16*)&Mem.vram[((control>>8)&0x1F) * (2*1024)];

        u32 sizex = text_bg_size[control>>14][0];
        u32 sizey = text_bg_size[control>>14][1];

        if(control & BIT(7)) //256 colors
        {
            int i, j;
            for(i = 0; i < sizex; i++) for(j = 0; j < sizey; j++)
            {
                u32 index = se_index(i/8,j/8,sizex/8);
                u16 SE = scrbaseblockptr[index];
                //screen entry data
                //0-9 tile id
                //10-hflip
                //11-vflip
                int _x = i & 7;
                if(SE & BIT(10)) _x = 7-_x; //hflip

                int _y = j & 7;
                if(SE & BIT(11)) _y = 7-_y; //vflip

                int data = charbaseblockptr[((SE&0x3FF)*64)  +  (_x+(_y*8))];

                tempmapbuffer[j*1024+i] = expand16to32(((u16*)Mem.pal_ram)[data]);
                tempvismapbuffer[j*1024+i] = data;
            }
        }
        else //16 colors
        {
            int i, j;
            for(i = 0; i < sizex; i++) for(j = 0; j < sizey; j++)
            {
                u32 index = se_index(i/8,j/8,sizex/8);
                u16 SE = scrbaseblockptr[index];
                //screen entry data
                //0-9 tile id
                //10-hflip
                //11-vflip
                //12-15-pal
                u16 * palptr = (u16*)&Mem.pal_ram[(SE>>12)*(2*16)];

                int _x = i & 7;
                if(SE & BIT(10)) _x = 7-_x; //hflip

                int _y = j & 7;
                if(SE & BIT(11)) _y = 7-_y; //vflip

                u32 data = charbaseblockptr[((SE&0x3FF)*32)  +  ((_x/2)+(_y*4))];

                if(_x&1) data = data>>4;
                else data = data & 0xF;

                tempmapbuffer[j*1024+i] = expand16to32(palptr[data]);
                tempvismapbuffer[j*1024+i] = data;
            }
        }
    }

    if(bgmode == 2) //affine
    {
        static const u32 affine_bg_size[4] = { 128, 256, 512, 1024 };

        u8 * charbaseblockptr = (u8*)&Mem.vram[((control>>2)&3) * (16*1024)];
        u8 * scrbaseblockptr = (u8*)&Mem.vram[((control>>8)&0x1F) * (2*1024)];

        u32 size = affine_bg_size[control>>14];
        u32 tilesize = size/8;

        //always 256 color

        int i, j;
        for(i = 0; i < size; i++) for(j = 0; j < size; j++)
        {
            int _x = i & 7;
            int _y = j & 7;

            u32 index = se_index_affine(i/8,j/8,tilesize);
            u8 SE = scrbaseblockptr[index];
            u16 data = charbaseblockptr[(SE*64) + (_x+(_y*8))];

            tempmapbuffer[j*1024+i] = expand16to32(((u16*)Mem.pal_ram)[data]);
            tempvismapbuffer[j*1024+i] = data;
        }
    }

    if(bgmode == 3) //bg2 mode 3
    {
        u16 * srcptr = (u16*)&Mem.vram;

        int i,j;
        for(i = 0; i < 240; i++) for(j = 0; j < 160; j++)
        {
            u16 data = srcptr[i+240*j];
            tempmapbuffer[j*1024+i] = expand16to32(data);
            tempvismapbuffer[j*1024+i] = 1;
        }
    }

    if(bgmode == 4) //bg2 mode 4
    {
        u8 * srcptr = (u8*)&Mem.vram[(REG_DISPCNT&BIT(4))?0xA000:0];

        int i,j;
        for(i = 0; i < 240; i++) for(j = 0; j < 160; j++)
        {
            u16 data = ((u16*)Mem.pal_ram)[srcptr[i+240*j]];
            tempmapbuffer[j*1024+i] = expand16to32(data);
            tempvismapbuffer[j*1024+i] = 1;
        }
    }

    if(bgmode == 5) //bg2 mode 5
    {
        u16 * srcptr = (u16*)&Mem.vram[(REG_DISPCNT&BIT(4))?0xA000:0];

        int i,j;
        for(i = 0; i < 160; i++) for(j = 0; j < 128; j++)
        {
            u16 data = srcptr[i+160*j];
            tempmapbuffer[j*1024+i] = expand16to32(data);
            tempvismapbuffer[j*1024+i] = 1;
        }
    }
}

static void gba_map_viewer_update_tile(void)
{
    int tiletempbuffer[8*8], tiletempvis[8*8];
    memset(tiletempvis,0,sizeof(tiletempvis));

    if(BgMode == 1)
    {
        u8 * charbaseblockptr = (u8*)&Mem.vram[((BgControl>>2)&3) * (16*1024)];
        u16 * scrbaseblockptr = (u16*)&Mem.vram[((BgControl>>8)&0x1F) * (2*1024)];

        u32 TileSEIndex = se_index(TileX,TileY,(SizeX/8));

        u16 SE = scrbaseblockptr[TileSEIndex];

        if(BgControl & BIT(7)) //256 Colors
        {
            u8 * data = (u8*)&(charbaseblockptr[(SE&0x3FF)*64]);

            int i,j;
            for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
            {
                u8 dat_ = data[j*8 + i];

                tiletempbuffer[j*8 + i] = expand16to32(((u16*)Mem.pal_ram)[dat_]);
                tiletempvis[j*8 + i] = dat_;
            }

            char text[1000];
            sprintf(text,"Tile: %d (%s%s)\nPos: %d,%d\nPal: --\nAddr: 0x%08X\n",
                SE&0x3FF, (SE & BIT(10)) ? "H" : "-", (SE & BIT(11)) ? "V" : "-" ,
                TileX,TileY,((BgControl>>8)&0x1F) * (2*1024) + TileSEIndex + 0x06000000);
            SetWindowText(hMapTileText,(LPCTSTR)text);
        }
        else //16 Colors
        {
            u8 * data = (u8*)&(charbaseblockptr[(SE&0x3FF)*32]);
            u16 * palptr = (u16*)&Mem.pal_ram[(SE>>12)*(2*16)];

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
            sprintf(text,"Tile: %d (%s%s)\nPos: %d,%d\nPal: %d\nAddr: 0x%08X\n",
                SE&0x3FF, (SE & BIT(10)) ? "H" : "-", (SE & BIT(11)) ? "V" : "-" ,
                TileX,TileY,(SE>>12)&0xF,((BgControl>>8)&0x1F) * (2*1024) + TileSEIndex + 0x06000000);
            SetWindowText(hMapTileText,(LPCTSTR)text);
        }
    }
    else if(BgMode == 2)
    {
        u8 * charbaseblockptr = (u8*)&Mem.vram[((BgControl>>2)&3) * (16*1024)];
        u8 * scrbaseblockptr = (u8*)&Mem.vram[((BgControl>>8)&0x1F) * (2*1024)];

        u32 TileSEIndex = se_index_affine(TileX,TileY,(SizeX/8));

        u16 SE = scrbaseblockptr[TileSEIndex];
        u8 * data = (u8*)&(charbaseblockptr[SE*64]);

        //256 colors always
        int i,j;
        for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
        {
            u8 dat_ = data[j*8 + i];

            tiletempbuffer[j*8 + i] = expand16to32(((u16*)Mem.pal_ram)[dat_]);
            tiletempvis[j*8 + i] = dat_;
        }

        char text[1000];

        sprintf(text,"Tile: %d (--)\nPos: %d,%d\nPal: --\nAddr: 0x%08X\n",
                SE&0x3FF,TileX,TileY,((BgControl>>8)&0x1F) * (2*1024) + TileSEIndex + 0x06000000);
        SetWindowText(hMapTileText,(LPCTSTR)text);
    }
    else
    {
        SetWindowText(hMapTileText,(LPCTSTR)"Tile: --- (--)\nPos: ---------\nPal: --\nAddr: ----------\n");
        memset(tiletempvis,0,sizeof(tiletempvis));
    }

    //Expand to 64x64
    int i,j;
    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
        MapTileBuffer[j*64+i] = ((i&16)^(j&16)) ? 0x00808080 : 0x00B0B0B0;

    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
    {
        if(tiletempvis[(j/8)*8 + (i/8)])
            MapTileBuffer[j*64+i] = tiletempbuffer[(j/8)*8 + (i/8)];
    }

    //Update window
    RECT rc; rc.top = 212; rc.left = 5; rc.bottom = 212+64; rc.right = 5+64;
    InvalidateRect(hWndMapViewer, &rc, FALSE);
}


void GLWindow_GBAMapViewerUpdate(void)
{
    if(MapViewerCreated == 0) return;

    if(RUNNING != RUN_GBA) return;

    u32 mode = REG_DISPCNT & 0x7;
    static const u32 bgenabled[8][4] = {
        {1,1,1,1}, {1,1,1,0}, {0,0,1,1}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,0,0}, {0,0,0,0}
    };

    if(bgenabled[mode][SelectedLayer] == 0)
    {
        CheckDlgButton(hWndMapViewer,IDC_BG0+SelectedLayer,BST_UNCHECKED);

        SelectedLayer = 0;
        while(bgenabled[mode][SelectedLayer] == 0)
        {
            SelectedLayer++;
            if(SelectedLayer >= 4) break;
        }
        if(SelectedLayer == 4) //WTF? Disable all
        {
            HWND hCtl = GetDlgItem(hWndMapViewer, IDC_BG0); EnableWindow(hCtl, FALSE);
            hCtl = GetDlgItem(hWndMapViewer, IDC_BG1); EnableWindow(hCtl, FALSE);
            hCtl = GetDlgItem(hWndMapViewer, IDC_BG2); EnableWindow(hCtl, FALSE);
            hCtl = GetDlgItem(hWndMapViewer, IDC_BG3); EnableWindow(hCtl, FALSE);
        }

        CheckDlgButton(hWndMapViewer,IDC_BG0+SelectedLayer,BST_CHECKED);
    }


    HWND hCtl = GetDlgItem(hWndMapViewer, IDC_BG0);
    EnableWindow(hCtl, bgenabled[mode][0] ? TRUE : FALSE);
    hCtl = GetDlgItem(hWndMapViewer, IDC_BG1);
    EnableWindow(hCtl, bgenabled[mode][1] ? TRUE : FALSE);
    hCtl = GetDlgItem(hWndMapViewer, IDC_BG2);
    EnableWindow(hCtl, bgenabled[mode][2] ? TRUE : FALSE);
    hCtl = GetDlgItem(hWndMapViewer, IDC_BG3);
    EnableWindow(hCtl, bgenabled[mode][3] ? TRUE : FALSE);

    gba_map_viewer_update();

    InvalidateRect(hWndMapViewer, NULL, FALSE);
}

static LRESULT CALLBACK MapViewerProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont, hFontFixed;

    switch(Msg)
    {
        case WM_CREATE:
        {
            hFont = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, DEFAULT_PITCH, NULL);

            hFontFixed = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, FIXED_PITCH, NULL);

            hWndMapScrollH = CreateWindowEx(0,"SCROLLBAR",NULL,WS_CHILD | WS_VISIBLE | SBS_HORZ,
                       200,261,  256,15,   hWnd,NULL,hInstance,NULL);
            hWndMapScrollV = CreateWindowEx(0,"SCROLLBAR",NULL,WS_CHILD | WS_VISIBLE | SBS_VERT,
                       456,5,  15,256,   hWnd,NULL,hInstance,NULL);

            SCROLLINFO si; ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
            si.nMin = 0; si.nMax = 0; si.nPos = 0; si.nPage = 8;
            SetScrollInfo(hWndMapScrollH, SB_CTL, &si, TRUE);
            SetScrollInfo(hWndMapScrollV, SB_CTL, &si, TRUE);

            u32 mode = REG_DISPCNT & 0x7;
            static const u32 bgenabled[8][4] = {
                {1,1,1,1}, {1,1,1,0}, {0,0,1,1}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,0,0}, {0,0,0,0}
            };

            HWND hGroup = CreateWindow(TEXT("button"), TEXT("Layer"),
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                3,1, 110,100, hWnd, (HMENU) 0, hInstance, NULL);
            HWND hBtn1 = CreateWindow(TEXT("button"), TEXT("Background 0"),
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | ( bgenabled[mode][0] ? 0 : WS_DISABLED),
                8, 18, 100, 18, hWnd, (HMENU)IDC_BG0 , hInstance, NULL);
            HWND hBtn2 = CreateWindow(TEXT("button"), TEXT("Background 1"),
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | ( bgenabled[mode][1] ? 0 : WS_DISABLED),
                8, 38, 100, 18, hWnd, (HMENU)IDC_BG1 , hInstance, NULL);
            HWND hBtn3 = CreateWindow(TEXT("button"), TEXT("Background 2"),
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | ( bgenabled[mode][2] ? 0 : WS_DISABLED),
                8, 58, 100, 18, hWnd, (HMENU)IDC_BG2, hInstance, NULL);
            HWND hBtn4 = CreateWindow(TEXT("button"), TEXT("Background 3"),
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | ( bgenabled[mode][3] ? 0 : WS_DISABLED),
                8, 78, 100, 18, hWnd, (HMENU)IDC_BG3, hInstance, NULL);


            SelectedLayer = 0;
            while(bgenabled[mode][SelectedLayer] == 0)
			{
				SelectedLayer++;
				if(SelectedLayer == 4) break;
			}

            if(SelectedLayer < 4) CheckDlgButton(hWnd,IDC_BG0+SelectedLayer,BST_CHECKED);

            SizeX = 0; SizeY = 0;
            TileX = 0; TileY = 0;

            SendMessage(hGroup, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn3, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn4, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

            hMapText = CreateWindow(TEXT("edit"), TEXT(" "),
                        WS_CHILD | WS_VISIBLE | BS_CENTER | ES_MULTILINE | ES_READONLY,
                        5, 103, 170, 110, hWnd, NULL, hInstance, NULL);
            SendMessage(hMapText, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            hMapTileText = CreateWindow(TEXT("edit"), TEXT(" "),
                        WS_CHILD | WS_VISIBLE | BS_CENTER | ES_MULTILINE | ES_READONLY,
                        75, 212, 120, 60, hWnd, NULL, hInstance, NULL);
            SendMessage(hMapTileText, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            GLWindow_GBAMapViewerUpdate();
            break;
        }
        case WM_COMMAND:
        {
            if(HIWORD(wParam) == BN_CLICKED)
            {
                switch(LOWORD(wParam))
                {
                    case IDC_BG0: SelectedLayer = 0; break;
                    case IDC_BG1: SelectedLayer = 1; break;
                    case IDC_BG2: SelectedLayer = 2; break;
                    case IDC_BG3: SelectedLayer = 3; break;
                    default: break;
                }
                GLWindow_GBAMapViewerUpdate();
            }
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT Ps;
            HDC hDC = BeginPaint(hWnd, &Ps);
            //Load the bitmap
            HBITMAP bitmap = CreateBitmap(256, 256, 1, 32, MapBuffer);
            // Create a memory device compatible with the above DC variable
            HDC MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 200, 5, 256, 256, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            bitmap = CreateBitmap(64, 64, 1, 32, MapTileBuffer);
            // Create a memory device compatible with the above DC variable
            MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 5, 212, 64, 64, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            EndPaint(hWnd, &Ps);
            break;
        }
        case WM_HSCROLL:
        {
            int CurPos = GetScrollPos(hWndMapScrollH, SB_CTL);
            int max_x = max(0,SizeX-256);
            switch (LOWORD(wParam))
            {
                case SB_LEFT: CurPos = 0; break;
                case SB_LINELEFT: CurPos-=8; if(CurPos < 0) CurPos = 0;  break;
                case SB_PAGELEFT: if(CurPos >= 8) { CurPos-=8; break; }
                                  if(CurPos > 0) { CurPos = 0; } break;
                case SB_THUMBPOSITION: CurPos = HIWORD(wParam)&~7; break;
                case SB_THUMBTRACK: CurPos = HIWORD(wParam)&~7; break;
                case SB_PAGERIGHT: if(CurPos < (max_x-8)) { CurPos+=8; break; }
                                  if(CurPos < max_x) { CurPos = max_x; } break;
                case SB_LINERIGHT: CurPos+= 8; if(CurPos > max_x) CurPos = max_x; break;
                case SB_RIGHT: CurPos = max_x; break;
                case SB_ENDSCROLL:
                default:
                    break;
            }
            SetScrollPos(hWndMapScrollH, SB_CTL, CurPos, TRUE);

            gba_map_update_window_from_temp_buffer();
            break;
        }
        case WM_LBUTTONDOWN:
        {
            if(BgMode != 1 && BgMode != 2) break; //not text or affine

            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if( (x >= 200) && (x < (200+256)) && (y >= 5) && (y < (5+256)) )
            {
                int scrollx = GetScrollPos(hWndMapScrollH, SB_CTL);
                int scrolly = GetScrollPos(hWndMapScrollV, SB_CTL);

                int bgx = (x-200)+scrollx;
                int bgy = (y-5)+scrolly;
                if(bgx < SizeX && bgy < SizeY)
                {
                    TileX = bgx/8; TileY = bgy/8;

                    gba_map_viewer_update_tile();
                }
            }
            break;
        }
    }
    return 0;
}


#endif
