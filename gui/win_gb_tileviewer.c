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

//-----------------------------------------------------------------------------------

static int WinIDGBTileViewer;

#define WIN_GB_TILEVIEWER_WIDTH  258
#define WIN_GB_TILEVIEWER_HEIGHT 222

static int GBTileViewerCreated = 0;

//-----------------------------------------------------------------------------------

static u32 gb_tileview_sprpal = 0; // 1 if selected a color from sprite palettes
static u32 gb_tileview_selectedindex = 0;

#define GB_TILE_BUFFER_WIDTH ((16*8)+1)
#define GB_TILE_BUFFER_HEIGHT ((24*8)+1)

static char gb_tile_bank0_buffer[GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT*3];
static char gb_tile_bank1_buffer[GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT*3];

//-----------------------------------------------------------------------------------

static _gui_console gb_tileview_con;
static _gui_element gb_tileview_textbox;

static _gui_element gb_tileview_dumpbtn;

static _gui_element gb_tileview_bgpal_bmp, gb_tileview_sprpal_bmp;
static _gui_element gb_tileview_bgpal_label, gb_tileview_sprpal_label;

static _gui_element * gb_tileviwer_window_gui_elements[] = {
    &gb_tileview_bgpal_label,
    &gb_tileview_sprpal_label,
    &gb_tileview_bgpal_bmp,
    &gb_tileview_sprpal_bmp,
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

static int _win_gb_tileviewer_bg_bmp_callback(int x, int y)
{/*
    gb_tileview_sprpal = 0; // bg
    gb_tileview_selectedindex = (x/20) + ((y/20)*4);*/
    return 1;
}

static int _win_gb_tileviewer_spr_bmp_callback(int x, int y)
{/*
    gb_tileview_sprpal = 1; // spr
    gb_tileview_selectedindex = (x/20) + ((y/20)*4);*/
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
/*
    u32 r,g,b;
    GB_Debug_GetPalette(gb_tileview_sprpal,gb_tileview_selectedindex/4,gb_tileview_selectedindex%4,&r,&g,&b);
    u16 value = ((r>>3)&0x1F)|(((g>>3)&0x1F)<<5)|(((b>>3)&0x1F)<<10);

    GUI_ConsoleModePrintf(&gb_tileview_con,0,0,"Value: 0x%04X",value);

    GUI_ConsoleModePrintf(&gb_tileview_con,0,1,"RGB: (%d,%d,%d)",r>>3,g>>3,b>>3);

    GUI_ConsoleModePrintf(&gb_tileview_con,16,0,"Index: %d[%d]",
                          gb_tileview_selectedindex/4,gb_tileview_selectedindex%4);

    memset(gb_tile_bg_buffer,192,sizeof(gb_tile_bg_buffer));
    memset(gb_tile_spr_buffer,192,sizeof(gb_tile_spr_buffer));

    int i;
    for(i = 0; i < 32; i++)
    {
        //BG
        GB_Debug_GetPalette(0,i/4,i%4,&r,&g,&b);
        GUI_Draw_SetDrawingColor(r,g,b);
        GUI_Draw_FillRect(gb_tile_bg_buffer,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,
                          1 + ((i%4)*20), 19 + ((i%4)*20), 1 + ((i/4)*20), 19 + ((i/4)*20));
        //SPR
        GB_Debug_GetPalette(1,i/4,i%4,&r,&g,&b);
        GUI_Draw_SetDrawingColor(r,g,b);
        GUI_Draw_FillRect(gb_tile_spr_buffer,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,
                        1 + ((i%4)*20), 19 + ((i%4)*20), 1 + ((i/4)*20), 19 + ((i/4)*20));
    }

    GUI_Draw_SetDrawingColor(255,0,0);

    char * buf;
    if(gb_tileview_sprpal == 0)
        buf = gb_tile_bg_buffer;
    else
        buf = gb_tile_spr_buffer;

    int ll = ((gb_tileview_selectedindex%4)*20); //left
    int tt = ((gb_tileview_selectedindex/4)*20); // top
    int rr = ll + 20; // right
    int bb = tt + 20; // bottom
    GUI_Draw_Rect(buf,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,ll,rr,tt,bb);
    ll++; tt++; rr--; bb--;
    GUI_Draw_Rect(buf,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,ll,rr,tt,bb);*/
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
{/*
    memset(gb_tile_bg_buffer,192,sizeof(gb_tile_bg_buffer));
    memset(gb_tile_spr_buffer,192,sizeof(gb_tile_spr_buffer));

    u32 r,g,b;

    int i;
    for(i = 0; i < 32; i++)
    {
        //BG
        GB_Debug_GetPalette(0,i/4,i%4,&r,&g,&b);
        GUI_Draw_SetDrawingColor(r,g,b);
        GUI_Draw_FillRect(gb_tile_bg_buffer,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,
                          1 + ((i%4)*20), 19 + ((i%4)*20), 1 + ((i/4)*20), 19 + ((i/4)*20));
        //SPR
        GB_Debug_GetPalette(1,i/4,i%4,&r,&g,&b);
        GUI_Draw_SetDrawingColor(r,g,b);
        GUI_Draw_FillRect(gb_tile_spr_buffer,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,
                        1 + ((i%4)*20), 19 + ((i%4)*20), 1 + ((i/4)*20), 19 + ((i/4)*20));
    }

    char buffer_temp[GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT * 4];

    char * src = gb_tile_bg_buffer;
    char * dst = buffer_temp;
    for(i = 0; i < GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xFF;
    }

    char * name_bg = FU_GetNewTimestampFilename("gb_tiles_bg");
    Save_PNG(name_bg,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,buffer_temp,0);


    src = gb_tile_spr_buffer;
    dst = buffer_temp;
    for(i = 0; i < GB_TILE_BUFFER_WIDTH*GB_TILE_BUFFER_HEIGHT; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xFF;
    }

    char * name_spr = FU_GetNewTimestampFilename("gb_tileette_spr");
    Save_PNG(name_spr,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,buffer_temp,0);
*/
    Win_GBTileViewerUpdate();
}

//----------------------------------------------------------------

int Win_GBTileViewerCreate(void)
{
    if(GBTileViewerCreated == 1)
        return 0;

    if(Win_MainRunningGB() == 0) return 0;

    GUI_SetLabel(&gb_tileview_bgpal_label,32,6,GB_TILE_BUFFER_WIDTH,FONT_12_HEIGHT,"Background");
    GUI_SetLabel(&gb_tileview_sprpal_label,145,6,GB_TILE_BUFFER_WIDTH,FONT_12_HEIGHT,"Sprites");

    GUI_SetTextBox(&gb_tileview_textbox,&gb_tileview_con,
                   6,192, 28*FONT_12_WIDTH,2*FONT_12_HEIGHT, NULL);

    GUI_SetBitmap(&gb_tileview_bgpal_bmp,32,24,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,gb_tile_bank0_buffer,
                  _win_gb_tileviewer_bg_bmp_callback);
    GUI_SetBitmap(&gb_tileview_sprpal_bmp,145,24,GB_TILE_BUFFER_WIDTH,GB_TILE_BUFFER_HEIGHT,gb_tile_bank1_buffer,
                  _win_gb_tileviewer_spr_bmp_callback);

    GUI_SetButton(&gb_tileview_dumpbtn,210,192,FONT_12_WIDTH*6,FONT_12_HEIGHT*2,"Dump",
                  _win_gb_tileviewer_dump_btn_callback);

    GBTileViewerCreated = 1;

    WinIDGBTileViewer = WH_Create(WIN_GB_TILEVIEWER_WIDTH,WIN_GB_TILEVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBTileViewer,"GB Tile Viewer");

    WH_SetEventCallback(WinIDGBTileViewer,Win_GBTileViewerCallback);

    Win_GBTileViewerUpdate();
    Win_GBTileViewerRender();

    return 1;
}

#if 0



//-----------------------------------------------------------------------------------------
//                                   TILE VIEWER
//-----------------------------------------------------------------------------------------

static HWND hWndTileViewer;
static int TileViewerCreated = 0;
static int TileBuffer[2][128*192], SelectedTileBuffer[64*64];
static u32 SelTileX,SelTileY, SelBank;
static HWND hTileText;

void GLWindow_GBTileViewerUpdate(void)
{
    if(TileViewerCreated == 0) return;

    if(RUNNING != RUN_GB) return;

    SelTileX = 0; SelTileY = 0;

    gb_tile_viewer_draw_to_buffer();
    gb_tile_viewer_update_tile();

    InvalidateRect(hWndTileViewer, NULL, FALSE);
}

static LRESULT CALLBACK TileViewerProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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

            SelTileX = 0; SelTileY = 0; SelBank = 0;

            hTileText = CreateWindow(TEXT("edit"), TEXT(" "),
                        WS_CHILD | WS_VISIBLE | BS_CENTER | ES_MULTILINE | ES_READONLY,
                        5, 5, 120, 45, hWnd, NULL, hInstance, NULL);
            SendMessage(hTileText, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            GLWindow_GBTileViewerUpdate();
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT Ps;
            HDC hDC = BeginPaint(hWnd, &Ps);
            //Load the bitmap
            HBITMAP bitmap = CreateBitmap(128, 192, 1, 32, TileBuffer[0]);
            // Create a memory device compatible with the above DC variable
            HDC MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 130, 5, 128, 192, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            bitmap = CreateBitmap(128, 192, 1, 32, TileBuffer[1]);
            // Create a memory device compatible with the above DC variable
            MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 130+10+128, 5, 128, 192, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            bitmap = CreateBitmap(64, 64, 1, 32, SelectedTileBuffer);
            // Create a memory device compatible with the above DC variable
            MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 5, 133, 64, 64, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            EndPaint(hWnd, &Ps);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if( (x >= 130) && (x < (130+128)) && (y >= 5) && (y < (5+256)) )
            {
                SelBank = 0;

                int bgx = (x-130);
                int bgy = (y-5);

                SelTileX = bgx/8; SelTileY = bgy/8;

                gb_tile_viewer_update_tile();
            }
            if( (x >= (130+10+128)) && (x < ((130+10+128)+128)) && (y >= 5) && (y < (5+256)) )
            {
                SelBank = 1;

                int bgx = (x-(130+10+128));
                int bgy = (y-5);

                SelTileX = bgx/8; SelTileY = bgy/8;

                gb_tile_viewer_update_tile();
            }
            break;
        }
    }
    return 0;
}



#endif // 0
