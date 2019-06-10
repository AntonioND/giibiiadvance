// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <SDL.h>

#include <string.h>

#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../general_utils.h"
#include "../file_utils.h"

#include "win_gba_debugger.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gba_core/memory.h"

#include "../png/png_utils.h"

//-----------------------------------------------------------------------------------

static int WinIDGBAPalViewer;

#define WIN_GBA_PALVIEWER_WIDTH  356
#define WIN_GBA_PALVIEWER_HEIGHT 222

static int GBAPalViewerCreated = 0;

//-----------------------------------------------------------------------------------

static u32 gba_palview_sprpal = 0; // 1 if selected a color from sprite palettes
static u32 gba_palview_selectedindex = 0;

#define GBA_PAL_BUFFER_SIDE ((10*16)+1)

static char gba_pal_bg_buffer[GBA_PAL_BUFFER_SIDE*GBA_PAL_BUFFER_SIDE*3];
static char gba_pal_spr_buffer[GBA_PAL_BUFFER_SIDE*GBA_PAL_BUFFER_SIDE*3];

//-----------------------------------------------------------------------------------

static _gui_console gba_palview_con;
static _gui_element gba_palview_textbox;

static _gui_element gba_palview_dumpbtn;

static _gui_element gba_palview_bgpal_bmp, gba_palview_sprpal_bmp;
static _gui_element gba_palview_bgpal_label, gba_palview_sprpal_label;

static _gui_element * gba_palviwer_window_gui_elements[] = {
    &gba_palview_bgpal_label,
    &gba_palview_sprpal_label,
    &gba_palview_bgpal_bmp,
    &gba_palview_sprpal_bmp,
    &gba_palview_textbox,
    &gba_palview_dumpbtn,
    NULL
};

static _gui gba_palviewer_window_gui = {
    gba_palviwer_window_gui_elements,
    NULL,
    NULL
};

//----------------------------------------------------------------

static void rgb16to32(u16 color, u8 * r, u8 * g, u8 * b)
{
    *r = (color & 31)<<3;
    *g = ((color >> 5) & 31)<<3;
    *b = ((color >> 10) & 31)<<3;
}

//----------------------------------------------------------------

static int _win_gba_palviewer_bg_bmp_callback(int x, int y)
{
    if(x >= GBA_PAL_BUFFER_SIDE-1) x = GBA_PAL_BUFFER_SIDE-2;
    if(y >= GBA_PAL_BUFFER_SIDE-1) y = GBA_PAL_BUFFER_SIDE-2;
    gba_palview_sprpal = 0; // bg
    gba_palview_selectedindex = (x/10) + ((y/10)*16);
    return 1;
}

static int _win_gba_palviewer_spr_bmp_callback(int x, int y)
{
    if(x >= GBA_PAL_BUFFER_SIDE-1) x = GBA_PAL_BUFFER_SIDE-2;
    if(y >= GBA_PAL_BUFFER_SIDE-1) y = GBA_PAL_BUFFER_SIDE-2;
    gba_palview_sprpal = 1; // spr
    gba_palview_selectedindex = (x/10) + ((y/10)*16);
    return 1;
}

//----------------------------------------------------------------

void Win_GBAPalViewerUpdate(void)
{
    if(GBAPalViewerCreated == 0) return;

    if(Win_MainRunningGBA() == 0) return;

    GUI_ConsoleClear(&gba_palview_con);

    u32 address = 0x05000000 + (gba_palview_sprpal*(256*2)) + (gba_palview_selectedindex*2);

    GUI_ConsoleModePrintf(&gba_palview_con,0,0,"Address: 0x%08X",address);

    u16 value = ((u16*)Mem.pal_ram)[gba_palview_selectedindex+(gba_palview_sprpal*256)];

    GUI_ConsoleModePrintf(&gba_palview_con,0,1,"Value: 0x%04X",value);

    GUI_ConsoleModePrintf(&gba_palview_con,24,0,"RGB: (%d,%d,%d)",value&31,(value>>5)&31,(value>>10)&31);

    GUI_ConsoleModePrintf(&gba_palview_con,24,1,"Index: %d|%d[%d]",gba_palview_selectedindex,
                          gba_palview_selectedindex/16,gba_palview_selectedindex%16);

    memset(gba_pal_bg_buffer,192,sizeof(gba_pal_bg_buffer));
    memset(gba_pal_spr_buffer,192,sizeof(gba_pal_spr_buffer));

    int i;
    for(i = 0; i < 256; i++)
    {
        //BG
        u8 r,g,b;
        rgb16to32(((u16*)Mem.pal_ram)[i],&r,&g,&b);
        GUI_Draw_SetDrawingColor(r,g,b);
        GUI_Draw_FillRect(gba_pal_bg_buffer,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,
                          1 + ((i%16)*10), 9 + ((i%16)*10), 1 + ((i/16)*10), 9 + ((i/16)*10));
        //SPR
        rgb16to32(((u16*)Mem.pal_ram)[i+256],&r,&g,&b);
        GUI_Draw_SetDrawingColor(r,g,b);
        GUI_Draw_FillRect(gba_pal_spr_buffer,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,
                        1 + ((i%16)*10), 9 + ((i%16)*10), 1 + ((i/16)*10), 9 + ((i/16)*10));
    }

    GUI_Draw_SetDrawingColor(255,0,0);

    char * buf;
    if(gba_palview_sprpal == 0)
        buf = gba_pal_bg_buffer;
    else
        buf = gba_pal_spr_buffer;

    int l = ((gba_palview_selectedindex%16)*10); //left
    int t = ((gba_palview_selectedindex/16)*10); // top
    int r = l + 10; // right
    int b = t + 10; // bottom
    GUI_Draw_Rect(buf,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,l,r,t,b);
    l++; t++; r--; b--;
    GUI_Draw_Rect(buf,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,l,r,t,b);
}

//----------------------------------------------------------------

static void _win_gba_pal_viewer_render(void)
{
    if(GBAPalViewerCreated == 0) return;

    char buffer[WIN_GBA_PALVIEWER_WIDTH*WIN_GBA_PALVIEWER_HEIGHT*3];
    GUI_Draw(&gba_palviewer_window_gui,buffer,WIN_GBA_PALVIEWER_WIDTH,WIN_GBA_PALVIEWER_HEIGHT,1);

    WH_Render(WinIDGBAPalViewer, buffer);
}

static int _win_gba_pal_viewer_callback(SDL_Event * e)
{
    if(GBAPalViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gba_palviewer_window_gui,e);

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
        GBAPalViewerCreated = 0;
        WH_Close(WinIDGBAPalViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBAPalViewerUpdate();
        _win_gba_pal_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gba_palviewer_dump_btn_callback(void)
{
    if(Win_MainRunningGBA() == 0) return;

    memset(gba_pal_bg_buffer,192,sizeof(gba_pal_bg_buffer));
    memset(gba_pal_spr_buffer,192,sizeof(gba_pal_spr_buffer));

    int i;
    for(i = 0; i < 256; i++)
    {
        //BG
        u8 r,g,b;
        rgb16to32(((u16*)Mem.pal_ram)[i],&r,&g,&b);
        GUI_Draw_SetDrawingColor(r,g,b);
        GUI_Draw_FillRect(gba_pal_bg_buffer,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,
                          1 + ((i%16)*10), 9 + ((i%16)*10), 1 + ((i/16)*10), 9 + ((i/16)*10));
        //SPR
        rgb16to32(((u16*)Mem.pal_ram)[i+256],&r,&g,&b);
        GUI_Draw_SetDrawingColor(r,g,b);
        GUI_Draw_FillRect(gba_pal_spr_buffer,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,
                        1 + ((i%16)*10), 9 + ((i%16)*10), 1 + ((i/16)*10), 9 + ((i/16)*10));
    }

    char buffer_temp[GBA_PAL_BUFFER_SIDE * GBA_PAL_BUFFER_SIDE * 4];

    char * src = gba_pal_bg_buffer;
    char * dst = buffer_temp;
    for(i = 0; i < GBA_PAL_BUFFER_SIDE * GBA_PAL_BUFFER_SIDE; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xFF;
    }

    char * name_bg = FU_GetNewTimestampFilename("gba_palette_bg");
    Save_PNG(name_bg,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,buffer_temp,0);


    src = gba_pal_spr_buffer;
    dst = buffer_temp;
    for(i = 0; i < GBA_PAL_BUFFER_SIDE * GBA_PAL_BUFFER_SIDE; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xFF;
    }

    char * name_spr = FU_GetNewTimestampFilename("gba_palette_spr");
    Save_PNG(name_spr,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,buffer_temp,0);

    Win_GBAPalViewerUpdate();
}

//----------------------------------------------------------------

int Win_GBAPalViewerCreate(void)
{
    if(Win_MainRunningGBA() == 0) return 0;

    if(GBAPalViewerCreated == 1)
    {
        WH_Focus(WinIDGBAPalViewer);
        return 0;
    }

    GUI_SetLabel(&gba_palview_bgpal_label,6,6,GBA_PAL_BUFFER_SIDE,FONT_HEIGHT,"Background");
    GUI_SetLabel(&gba_palview_sprpal_label,188,6,GBA_PAL_BUFFER_SIDE,FONT_HEIGHT,"Sprites");

    GUI_SetTextBox(&gba_palview_textbox,&gba_palview_con,
                   6,192, 42*FONT_WIDTH,2*FONT_HEIGHT, NULL);

    GUI_SetBitmap(&gba_palview_bgpal_bmp,6,24,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,gba_pal_bg_buffer,
                  _win_gba_palviewer_bg_bmp_callback);
    GUI_SetBitmap(&gba_palview_sprpal_bmp,188,24,GBA_PAL_BUFFER_SIDE,GBA_PAL_BUFFER_SIDE,gba_pal_spr_buffer,
                  _win_gba_palviewer_spr_bmp_callback);

    GUI_SetButton(&gba_palview_dumpbtn,308,192,FONT_WIDTH*6,FONT_HEIGHT*2,"Dump",
                  _win_gba_palviewer_dump_btn_callback);

    gba_palview_sprpal = 0;
    gba_palview_selectedindex = 0;

    GBAPalViewerCreated = 1;

    WinIDGBAPalViewer = WH_Create(WIN_GBA_PALVIEWER_WIDTH,WIN_GBA_PALVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBAPalViewer,"GBA Palette Viewer");

    WH_SetEventCallback(WinIDGBAPalViewer,_win_gba_pal_viewer_callback);

    Win_GBAPalViewerUpdate();
    _win_gba_pal_viewer_render();

    return 1;
}

void Win_GBAPalViewerClose(void)
{
    if(GBAPalViewerCreated == 0)
        return;

    GBAPalViewerCreated = 0;
    WH_Close(WinIDGBAPalViewer);
}

//----------------------------------------------------------------
