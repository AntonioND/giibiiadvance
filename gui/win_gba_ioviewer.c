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

#include "win_gba_ioviewer.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gba_core/gba.h"
#include "../gba_core/memory.h"

//-----------------------------------------------------------------------------------

static int WinIDGBAIOViewer;

#define WIN_GBA_IOVIEWER_WIDTH  593
#define WIN_GBA_IOVIEWER_HEIGHT 400

static int GBAIOViewerCreated = 0;

static int gba_ioview_selected_tab = 0;

//-----------------------------------------------------------------------------------

static _gui_element gba_ioview_display_tabbtn, gba_ioview_backgrounds_tabbtn, gba_ioview_dma_tabbtn,
                    gba_ioview_timers_tabbtn, gba_ioview_sound_tabbtn, gba_ioview_other_tabbtn;

static _gui_console gba_ioview_display_lcdcontrol_con;
static _gui_element gba_ioview_display_lcdcontrol_textbox, gba_ioview_display_lcdcontrol_label;

static _gui_element * gba_ioviwer_display_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    &gba_ioview_display_lcdcontrol_textbox, &gba_ioview_display_lcdcontrol_label,
    NULL
};

static _gui_element * gba_ioviwer_backgrounds_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    NULL
};

static _gui_element * gba_ioviwer_dma_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    NULL
};

static _gui_element * gba_ioviwer_timers_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    NULL
};

static _gui_element * gba_ioviwer_sound_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    NULL
};

static _gui_element * gba_ioviwer_other_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    NULL
};

static _gui gba_ioviewer_page_gui[] = {
    { gba_ioviwer_display_elements, NULL, NULL },
    { gba_ioviwer_backgrounds_elements, NULL, NULL },
    { gba_ioviwer_dma_elements, NULL, NULL },
    { gba_ioviwer_timers_elements, NULL, NULL },
    { gba_ioviwer_sound_elements, NULL, NULL },
    { gba_ioviwer_other_elements, NULL, NULL }
};

void Win_GBAIOViewerUpdate(void)
{
    if(GBAIOViewerCreated == 0) return;

    if(Win_MainRunningGBA() == 0) return;



    switch(gba_ioview_selected_tab)
    {
        case 0: // Display
        {
            #define CHECK(a) ((a)?CHR_SQUAREBLACK_MID:' ')

            GUI_ConsoleClear(&gba_ioview_display_lcdcontrol_con);
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,0, "000h DISPCNT - %04X",REG_DISPCNT);

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,2, "[%d] Video Mode",REG_DISPCNT&7);
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,3, "[%c] CGB Mode (?)",CHECK(REG_DISPCNT&BIT(3)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,4, "[%c] Display Frame Select",CHECK(REG_DISPCNT&BIT(4)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,5, "[%c] H-Blank Interval Free",CHECK(REG_DISPCNT&BIT(5)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,6, "[%c] 1D Character Mapping",CHECK(REG_DISPCNT&BIT(6)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,7, "[%c] Forced Blank",CHECK(REG_DISPCNT&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,8, "[%c] Display BG0",CHECK(REG_DISPCNT&BIT(8)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,9, "[%c] Display BG1",CHECK(REG_DISPCNT&BIT(9)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,10, "[%c] Display BG2",CHECK(REG_DISPCNT&BIT(10)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,11, "[%c] Display BG3",CHECK(REG_DISPCNT&BIT(11)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,12, "[%c] Display OBJ",CHECK(REG_DISPCNT&BIT(12)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,13, "[%c] Display Window 0",CHECK(REG_DISPCNT&BIT(13)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,14, "[%c] Display Window 1",CHECK(REG_DISPCNT&BIT(14)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,15, "[%c] Display OBJ Window",CHECK(REG_DISPCNT&BIT(15)));

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,18, "002h GREENSWP - %04X",REG_GREENSWAP);

        }
        case 1: // Backgrounds
        {

        }
        case 2: // DMA
        {

        }
        case 3: // Timers
        {

        }
        case 4: // Sound
        {

        }
        case 5: // Other
        {

        }
        default:
        {
            break;
        }
    } // end switch
}
//----------------------------------------------------------------

static void _win_gba_ioviewer_tabnum_radbtn_callback(int num)
{
    gba_ioview_selected_tab = num;
}

//----------------------------------------------------------------

static void _win_gba_io_viewer_render(void)
{
    if(GBAIOViewerCreated == 0) return;

    char buffer[WIN_GBA_IOVIEWER_WIDTH*WIN_GBA_IOVIEWER_HEIGHT*3];
    GUI_Draw(&(gba_ioviewer_page_gui[gba_ioview_selected_tab]),buffer,
             WIN_GBA_IOVIEWER_WIDTH,WIN_GBA_IOVIEWER_HEIGHT,1);

    WH_Render(WinIDGBAIOViewer, buffer);
}

static int _win_gba_io_viewer_callback(SDL_Event * e)
{
    if(GBAIOViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&(gba_ioviewer_page_gui[gba_ioview_selected_tab]),e);

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
            close_this = 1;
    }

    if(close_this)
    {
        GBAIOViewerCreated = 0;
        WH_Close(WinIDGBAIOViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBAIOViewerUpdate();
        _win_gba_io_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

int Win_GBAIOViewerCreate(void)
{
    if(GBAIOViewerCreated == 1)
        return 0;

    if(Win_MainRunningGBA() == 0) return 0;

    gba_ioview_selected_tab = 0;

    GUI_SetRadioButton(&gba_ioview_display_tabbtn,                    0,0, 14*FONT_WIDTH,18,
                  "Display", 0, 0, 1,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_backgrounds_tabbtn,  1+14*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "Backgrounds", 0, 1, 0,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_dma_tabbtn,          2+28*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "DMA", 0, 2, 0,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_timers_tabbtn,       3+42*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "Timers", 0, 3, 0,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_sound_tabbtn,        4+56*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "Sound", 0, 4, 0,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_other_tabbtn,        5+70*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "Other", 0, 5, 0,_win_gba_ioviewer_tabnum_radbtn_callback);

    //--------------------------------------------------------------------

    // Display

    GUI_SetLabel(&gba_ioview_display_lcdcontrol_label,
                   6,24, -1,FONT_HEIGHT, "LCD Control");
    GUI_SetTextBox(&gba_ioview_display_lcdcontrol_textbox,&gba_ioview_display_lcdcontrol_con,
                   6,42, 26*FONT_WIDTH+2,19*FONT_HEIGHT, NULL);

    // Backgrounds

    // DMA

    // Timers

    // Sound

    // Other

    //--------------------------------------------------------------------

    GBAIOViewerCreated = 1;

    WinIDGBAIOViewer = WH_Create(WIN_GBA_IOVIEWER_WIDTH,WIN_GBA_IOVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBAIOViewer,"GBA I/O Viewer");

    WH_SetEventCallback(WinIDGBAIOViewer,_win_gba_io_viewer_callback);

    Win_GBAIOViewerUpdate();
    _win_gba_io_viewer_render();

    return 1;
}

void Win_GBAIOViewerClose(void)
{
    if(GBAIOViewerCreated == 0)
        return;

    GBAIOViewerCreated = 0;
    WH_Close(WinIDGBAIOViewer);
}

//----------------------------------------------------------------

#if 0

#include "gui_mainloop.h"
#include "gui_main.h"
#include "gui_gba_debugger.h"
#include "gba_core/gba.h"
#include "gba_core/memory.h"
#include "gba_core/timers.h"
#include "gba_core/cpu.h"
#include "gba_core/disassembler.h"
#include "gba_core/sound.h"

void GLWindow_GBAIOViewerUpdate(void)
{
    if(IOViewerCreated == 0) return;

    if(RUNNING != RUN_GBA) return;

    #define SET_CHECK(page,item,condition) \
    SendMessage(hTabPageItem[page][item], BM_SETCHECK, ((condition)?BST_CHECKED:BST_UNCHECKED), 0)

    switch(ioviewer_curpage)
    {
        case 0: //Display
        {
            char text[5];

            //DISPSTAT
            sprintf(text,"%04X",REG_DISPSTAT); SetWindowText(hTabPageItem[0][33],(LPCTSTR)text);
            SET_CHECK(0,35,REG_DISPSTAT&BIT(0));
            SET_CHECK(0,37,REG_DISPSTAT&BIT(1));
            SET_CHECK(0,39,REG_DISPSTAT&BIT(2));
            SET_CHECK(0,41,REG_DISPSTAT&BIT(3));
            SET_CHECK(0,43,REG_DISPSTAT&BIT(4));
            SET_CHECK(0,45,REG_DISPSTAT&BIT(5));
            sprintf(text,"%d",REG_DISPSTAT>>8); SetWindowText(hTabPageItem[0][47],(LPCTSTR)text);

            //VCOUNT
            sprintf(text,"%04X",REG_VCOUNT); SetWindowText(hTabPageItem[0][49],(LPCTSTR)text);
            sprintf(text,"%d",REG_VCOUNT); SetWindowText(hTabPageItem[0][51],(LPCTSTR)text);

            //MOSAIC
            u32 mos = REG_MOSAIC;
            sprintf(text,"%04X",mos); SetWindowText(hTabPageItem[0][54],(LPCTSTR)text);
            sprintf(text,"Sp %2d,%2d|Bg %2d,%2d",((mos>>8)&0xF)+1,((mos>>12)&0xF)+1,(mos&0xF)+1,
                ((mos>>4)&0xF)+1); SetWindowText(hTabPageItem[0][55],(LPCTSTR)text);

            //WINDOWS
            sprintf(text,"%04X|%04X",REG_WIN0H,REG_WIN0V); SetWindowText(hTabPageItem[0][63],(LPCTSTR)text);
            sprintf(text,"%04X|%04X",REG_WIN1H,REG_WIN1V); SetWindowText(hTabPageItem[0][65],(LPCTSTR)text);

            u32 x1,x2,y1,y2;
            x1 = (REG_WIN0H>>8)&0xFF; x2 = REG_WIN0H&0xFF;
            if(x2 > 240) x2 = 240; if(x1 > x2) x2 = 240; //real bounds
            y1 = (REG_WIN0V>>8)&0xFF; y2 = REG_WIN0V&0xFF;
            if(y2 > 160) y2 = 160; if(y1 > y2) y2 = 160;
            sprintf(text,"%3d,%3d|%3d,%3d",x1,y1,x2,y2); SetWindowText(hTabPageItem[0][67],(LPCTSTR)text);

            x1 = (REG_WIN1H>>8)&0xFF; x2 = REG_WIN1H&0xFF;
            if(x2 > 240) x2 = 240; if(x1 > x2) x2 = 240;
            y1 = (REG_WIN1V>>8)&0xFF; y2 = REG_WIN1V&0xFF;
            if(y2 > 160) y2 = 160; if(y1 > y2) y2 = 160;
            sprintf(text,"%3d,%3d|%3d,%3d",x1,y1,x2,y2); SetWindowText(hTabPageItem[0][69],(LPCTSTR)text);

            sprintf(text,"%04X|%04X",REG_WININ,REG_WINOUT); SetWindowText(hTabPageItem[0][71],(LPCTSTR)text);

            u32 in0 = REG_WININ&0xFF; u32 in1 = (REG_WININ>>8)&0xFF;
            u32 out = REG_WINOUT&0xFF; u32 obj = (REG_WINOUT>>8)&0xFF;

            SET_CHECK(0,82,in0&BIT(0)); SET_CHECK(0,83,in0&BIT(1)); SET_CHECK(0,84,in0&BIT(2));
            SET_CHECK(0,85,in0&BIT(3)); SET_CHECK(0,86,in0&BIT(4)); SET_CHECK(0,87,in0&BIT(5));

            SET_CHECK(0,88,in1&BIT(0)); SET_CHECK(0,89,in1&BIT(1)); SET_CHECK(0,90,in1&BIT(2));
            SET_CHECK(0,91,in1&BIT(3)); SET_CHECK(0,92,in1&BIT(4)); SET_CHECK(0,93,in1&BIT(5));

            SET_CHECK(0,94,out&BIT(0)); SET_CHECK(0,95,out&BIT(1)); SET_CHECK(0,96,out&BIT(2));
            SET_CHECK(0,97,out&BIT(3)); SET_CHECK(0,98,out&BIT(4)); SET_CHECK(0,99,out&BIT(5));

            SET_CHECK(0,100,obj&BIT(0)); SET_CHECK(0,101,obj&BIT(1)); SET_CHECK(0,102,obj&BIT(2));
            SET_CHECK(0,103,obj&BIT(3)); SET_CHECK(0,104,obj&BIT(4)); SET_CHECK(0,105,obj&BIT(5));

            //BLENDING
            sprintf(text,"%04X",REG_BLDALPHA); SetWindowText(hTabPageItem[0][57],(LPCTSTR)text);
            sprintf(text,"%04X",REG_BLDY); SetWindowText(hTabPageItem[0][107],(LPCTSTR)text);
            sprintf(text,"%04X",REG_BLDCNT); SetWindowText(hTabPageItem[0][109],(LPCTSTR)text);

            int evy = REG_BLDY&0x1F; if(evy > 16) evy = 16;
            sprintf(text,"EVY %2d",evy); SetWindowText(hTabPageItem[0][58],(LPCTSTR)text);

            int eva = REG_BLDALPHA&0x1F; if(eva > 16) eva = 16;
            int evb = (REG_BLDALPHA>>8)&0x1F; if(evb > 16) evb = 16;
            sprintf(text,"EVA %2d|EVB %2d",eva,evb); SetWindowText(hTabPageItem[0][110],(LPCTSTR)text);

            const char * bld_modes[4] = { "Mode: None", "Mode: Alpha", "Mode: White", "Mode: Black" };
            SetWindowText(hTabPageItem[0][111],(LPCTSTR)bld_modes[(REG_BLDCNT>>6)&3]);

            u32 first = REG_BLDCNT&0x3F;
            u32 second = (REG_BLDCNT>>8)&0x3F;

            SET_CHECK(0,120,first&BIT(0)); SET_CHECK(0,121,first&BIT(1));
            SET_CHECK(0,122,first&BIT(2)); SET_CHECK(0,123,first&BIT(3));
            SET_CHECK(0,124,first&BIT(4)); SET_CHECK(0,125,first&BIT(5));

            SET_CHECK(0,126,second&BIT(0)); SET_CHECK(0,127,second&BIT(1));
            SET_CHECK(0,128,second&BIT(2)); SET_CHECK(0,129,second&BIT(3));
            SET_CHECK(0,130,second&BIT(4)); SET_CHECK(0,131,second&BIT(5));

            break;
        }
        case 1: //Background
        {
            char text[20];
            int scrmode = REG_DISPCNT&7;

            //BG0CNT
            sprintf(text,"%04X",REG_BG0CNT); SetWindowText(hTabPageItem[1][2],(LPCTSTR)text);
            sprintf(text,"%d",REG_BG0CNT&3); SetWindowText(hTabPageItem[1][4],(LPCTSTR)text);
            sprintf(text,"%08X",0x06000000+(((REG_BG0CNT>>2)&3)*0x4000));
            SetWindowText(hTabPageItem[1][6],(LPCTSTR)text);
            sprintf(text,"%08X",0x06000000+(((REG_BG0CNT>>8)&31)*0x800));
            SetWindowText(hTabPageItem[1][8],(LPCTSTR)text);

            const char * size_bg01[8][4] = { //mode(isaffine), size
                {"256x256", "512x256", "256x512", "512x512"},
                {"256x256", "512x256", "256x512", "512x512"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"}
            };

            SetWindowText(hTabPageItem[1][10],(LPCTSTR)size_bg01[scrmode][(REG_BG0CNT>>14)&3]);
            SET_CHECK(1,12,REG_BG0CNT&BIT(6));
            SET_CHECK(1,14,REG_BG0CNT&BIT(7));
            SET_CHECK(1,16,REG_BG0CNT&BIT(13));

            //BG1CNT
            sprintf(text,"%04X",REG_BG1CNT); SetWindowText(hTabPageItem[1][18],(LPCTSTR)text);
            sprintf(text,"%d",REG_BG1CNT&3); SetWindowText(hTabPageItem[1][20],(LPCTSTR)text);
            sprintf(text,"%08X",0x06000000+(((REG_BG1CNT>>2)&3)*0x4000));
            SetWindowText(hTabPageItem[1][22],(LPCTSTR)text);
            sprintf(text,"%08X",0x06000000+(((REG_BG1CNT>>8)&31)*0x800));
            SetWindowText(hTabPageItem[1][24],(LPCTSTR)text);
            SetWindowText(hTabPageItem[1][26],(LPCTSTR)size_bg01[scrmode][(REG_BG1CNT>>14)&3]);
            SET_CHECK(1,28,REG_BG1CNT&BIT(6));
            SET_CHECK(1,30,REG_BG1CNT&BIT(7));
            SET_CHECK(1,32,REG_BG1CNT&BIT(13));

            //BG2CNT
            sprintf(text,"%04X",REG_BG2CNT); SetWindowText(hTabPageItem[1][34],(LPCTSTR)text);
            sprintf(text,"%d",REG_BG2CNT&3); SetWindowText(hTabPageItem[1][36],(LPCTSTR)text);
            sprintf(text,"%08X",0x06000000+(((REG_BG2CNT>>2)&3)*0x4000));
            SetWindowText(hTabPageItem[1][38],(LPCTSTR)text);
            sprintf(text,"%08X",0x06000000+(((REG_BG2CNT>>8)&31)*0x800));
            SetWindowText(hTabPageItem[1][40],(LPCTSTR)text);

            const char * size_bg2[8][4] = { //mode(isaffine), size
                {"256x256", "512x256", "256x512", "512x512"},
                {"128x128", "256x256", "512x512", "1024x1024"},
                {"128x128", "256x256", "512x512", "1024x1024"},
                {"240x160", "240x160", "240x160", "240x160"},
                {"240x160", "240x160", "240x160", "240x160"},
                {"160x128", "160x128", "160x128", "160x128"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"}
            };
            SetWindowText(hTabPageItem[1][42],(LPCTSTR)size_bg2[scrmode][(REG_BG2CNT>>14)&3]);
            SET_CHECK(1,44,REG_BG2CNT&BIT(6));
            SET_CHECK(1,46,REG_BG2CNT&BIT(7));
            SET_CHECK(1,48,REG_BG2CNT&BIT(13));

            //BG3CNT
            sprintf(text,"%04X",REG_BG3CNT); SetWindowText(hTabPageItem[1][50],(LPCTSTR)text);
            sprintf(text,"%d",REG_BG3CNT&3); SetWindowText(hTabPageItem[1][52],(LPCTSTR)text);
            sprintf(text,"%08X",0x06000000+(((REG_BG3CNT>>2)&3)*0x4000));
            SetWindowText(hTabPageItem[1][54],(LPCTSTR)text);
            sprintf(text,"%08X",0x06000000+(((REG_BG3CNT>>8)&31)*0x800));
            SetWindowText(hTabPageItem[1][56],(LPCTSTR)text);

            const char * size_bg3[8][4] = { //mode(isaffine), size
                {"256x256", "512x256", "256x512", "512x512"},
                {"---------", "---------", "---------", "---------"},
                {"128x128", "256x256", "512x512", "1024x1024"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"}
            };
            SetWindowText(hTabPageItem[1][58],(LPCTSTR)size_bg3[scrmode][(REG_BG3CNT>>14)&3]);
            SET_CHECK(1,60,REG_BG3CNT&BIT(6));
            SET_CHECK(1,62,REG_BG3CNT&BIT(7));
            SET_CHECK(1,64,REG_BG3CNT&BIT(13));

            //BGxHOFS,BGxVOFS
            static const int bgistext[8][4] = { //mode, bgnumber
                {1,1,1,1},{1,1,0,0},{0,0,0,0},{0,0,0,0},
                {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}
            };
            if(bgistext[scrmode][0]) sprintf(text,"%d,%d",REG_BG0HOFS,REG_BG0VOFS);
            else strcpy(text,"---,---");
            SetWindowText(hTabPageItem[1][67],(LPCTSTR)text);
            if(bgistext[scrmode][1]) sprintf(text,"%d,%d",REG_BG1HOFS,REG_BG1VOFS);
            else strcpy(text,"---,---");
            SetWindowText(hTabPageItem[1][69],(LPCTSTR)text);
            if(bgistext[scrmode][2]) sprintf(text,"%d,%d",REG_BG2HOFS,REG_BG2VOFS);
            else strcpy(text,"---,---");
            SetWindowText(hTabPageItem[1][71],(LPCTSTR)text);
            if(bgistext[scrmode][3]) sprintf(text,"%d,%d",REG_BG3HOFS,REG_BG3VOFS);
            else strcpy(text,"---,---");
            SetWindowText(hTabPageItem[1][73],(LPCTSTR)text);

            //BGxPA-BGxPD,BGxX,BGxY
            static const int bgisaffine[8][2] = { //mode, bgnumber (2,3)
                {0,0},{1,0},{1,1},{1,0},{1,0},{1,0},{0,0},{0,0}
            };

            if(bgisaffine[scrmode][0])
            {
                sprintf(text,"%07X,%07X",REG_BG2X,REG_BG2Y);
                SetWindowText(hTabPageItem[1][76],(LPCTSTR)text);
                sprintf(text,"%04X,%04X",REG_BG2PA,REG_BG2PB);
                SetWindowText(hTabPageItem[1][80],(LPCTSTR)text);
                sprintf(text,"%04X,%04X",REG_BG2PC,REG_BG2PD);
                SetWindowText(hTabPageItem[1][81],(LPCTSTR)text);
            }
            else
            {
                strcpy(text,"-------,-------");
                SetWindowText(hTabPageItem[1][76],(LPCTSTR)text);
                strcpy(text,"----,----");
                SetWindowText(hTabPageItem[1][80],(LPCTSTR)text);
                SetWindowText(hTabPageItem[1][81],(LPCTSTR)text);
            }

            if(bgisaffine[scrmode][1])
            {
                sprintf(text,"%07X,%07X",REG_BG3X,REG_BG3Y);
                SetWindowText(hTabPageItem[1][78],(LPCTSTR)text);
                sprintf(text,"%04X,%04X",REG_BG3PA,REG_BG3PB);
                SetWindowText(hTabPageItem[1][83],(LPCTSTR)text);
                sprintf(text,"%04X,%04X",REG_BG3PC,REG_BG3PD);
                SetWindowText(hTabPageItem[1][84],(LPCTSTR)text);
            }
            else
            {
                strcpy(text,"-------,-------");
                SetWindowText(hTabPageItem[1][78],(LPCTSTR)text);
                strcpy(text,"----,----");
                SetWindowText(hTabPageItem[1][83],(LPCTSTR)text);
                SetWindowText(hTabPageItem[1][84],(LPCTSTR)text);
            }

            static const char * videomodetext[8] = {
                "Video Mode: 0 (4 txt)",
                "Video Mode: 1 (2 txt, 1 aff)",
                "Video Mode: 2 (2 aff)",
                "Video Mode: 3 (bitmap 16bit)",
                "Video Mode: 4 (bitmap 8bit)",
                "Video Mode: 5 (bitmap 16bit)",
                "Video Mode: 6 (INVALID)",
                "Video Mode: 7 (INVALID)"
            };

            SetWindowText(hTabPageItem[1][85],(LPCTSTR)videomodetext[scrmode]);

            break;
        }
        case 2: //DMA
        {
            char text[20];

            const char * srcincmode[4] = { "Increment","Decrement","Fixed","Prohibited" };
            const char * dstincmode[4] = { "Increment","Decrement","Fixed","Inc/Reload" };
            const char * startmode[4][4] = {
                {"Start immediately","Start immediately","Start immediately","Start immediately"},
                {"Start at VBlank","Start at VBlank","Start at VBlank","Start at VBlank"},
                {"Start at HBlank","Start at HBlank","Start at HBlank","Start at HBlank"},
                {"Prohibited","Sound FIFO","Sound FIFO","Video Capture"}
            };

            //DMA 0
            sprintf(text,"%08X",REG_DMA0SAD); SetWindowText(hTabPageItem[2][2],(LPCTSTR)text);
            sprintf(text,"%08X",REG_DMA0DAD); SetWindowText(hTabPageItem[2][4],(LPCTSTR)text);
            sprintf(text,"%08X",REG_DMA0CNT); SetWindowText(hTabPageItem[2][6],(LPCTSTR)text);
            SetWindowText(hTabPageItem[2][7],(LPCTSTR)srcincmode[(REG_DMA0CNT_H>>7)&3]);
            SetWindowText(hTabPageItem[2][8],(LPCTSTR)dstincmode[(REG_DMA0CNT_H>>5)&3]);
            SET_CHECK(2,10,REG_DMA0CNT_H&BIT(10));
            SetWindowText(hTabPageItem[2][11],(LPCTSTR)startmode[(REG_DMA0CNT_H>>12)&3][0]);
            sprintf(text,"%d bytes",REG_DMA0CNT_L*((REG_DMA0CNT_H&BIT(10))?4:2));
            SetWindowText(hTabPageItem[2][12],(LPCTSTR)text);
            SET_CHECK(2,14,REG_DMA0CNT_H&BIT(9));
            SET_CHECK(2,16,REG_DMA0CNT_H&BIT(14));
            SET_CHECK(2,18,REG_DMA0CNT_H&BIT(15));

            //DMA 1
            sprintf(text,"%08X",REG_DMA1SAD); SetWindowText(hTabPageItem[2][21],(LPCTSTR)text);
            sprintf(text,"%08X",REG_DMA1DAD); SetWindowText(hTabPageItem[2][23],(LPCTSTR)text);
            sprintf(text,"%08X",REG_DMA1CNT); SetWindowText(hTabPageItem[2][25],(LPCTSTR)text);
            SetWindowText(hTabPageItem[2][26],(LPCTSTR)srcincmode[(REG_DMA1CNT_H>>7)&3]);
            SetWindowText(hTabPageItem[2][27],(LPCTSTR)dstincmode[(REG_DMA1CNT_H>>5)&3]);
            SET_CHECK(2,29,REG_DMA1CNT_H&BIT(10));
            SetWindowText(hTabPageItem[2][30],(LPCTSTR)startmode[(REG_DMA1CNT_H>>12)&3][1]);
            sprintf(text,"%d bytes",REG_DMA1CNT_L*((REG_DMA1CNT_H&BIT(10))?4:2));
            SetWindowText(hTabPageItem[2][31],(LPCTSTR)text);
            SET_CHECK(2,33,REG_DMA1CNT_H&BIT(9));
            SET_CHECK(2,35,REG_DMA1CNT_H&BIT(14));
            SET_CHECK(2,37,REG_DMA1CNT_H&BIT(15));

            //DMA 2
            sprintf(text,"%08X",REG_DMA2SAD); SetWindowText(hTabPageItem[2][40],(LPCTSTR)text);
            sprintf(text,"%08X",REG_DMA2DAD); SetWindowText(hTabPageItem[2][42],(LPCTSTR)text);
            sprintf(text,"%08X",REG_DMA2CNT); SetWindowText(hTabPageItem[2][44],(LPCTSTR)text);
            SetWindowText(hTabPageItem[2][45],(LPCTSTR)srcincmode[(REG_DMA2CNT_H>>7)&3]);
            SetWindowText(hTabPageItem[2][46],(LPCTSTR)dstincmode[(REG_DMA2CNT_H>>5)&3]);
            SET_CHECK(2,48,REG_DMA2CNT_H&BIT(10));
            SetWindowText(hTabPageItem[2][49],(LPCTSTR)startmode[(REG_DMA2CNT_H>>12)&3][2]);
            sprintf(text,"%d bytes",REG_DMA2CNT_L*((REG_DMA2CNT_H&BIT(10))?4:2));
            SetWindowText(hTabPageItem[2][50],(LPCTSTR)text);
            SET_CHECK(2,52,REG_DMA2CNT_H&BIT(9));
            SET_CHECK(2,54,REG_DMA2CNT_H&BIT(14));
            SET_CHECK(2,56,REG_DMA2CNT_H&BIT(15));

            //DMA 3
            sprintf(text,"%08X",REG_DMA3SAD); SetWindowText(hTabPageItem[2][59],(LPCTSTR)text);
            sprintf(text,"%08X",REG_DMA3DAD); SetWindowText(hTabPageItem[2][61],(LPCTSTR)text);
            sprintf(text,"%08X",REG_DMA3CNT); SetWindowText(hTabPageItem[2][63],(LPCTSTR)text);
            SetWindowText(hTabPageItem[2][64],(LPCTSTR)srcincmode[(REG_DMA3CNT_H>>7)&3]);
            SetWindowText(hTabPageItem[2][65],(LPCTSTR)dstincmode[(REG_DMA3CNT_H>>5)&3]);
            SET_CHECK(2,67,REG_DMA3CNT_H&BIT(10));
            SetWindowText(hTabPageItem[2][68],(LPCTSTR)startmode[(REG_DMA3CNT_H>>12)&3][3]);
            sprintf(text,"%d bytes",REG_DMA3CNT_L*((REG_DMA3CNT_H&BIT(10))?4:2));
            SetWindowText(hTabPageItem[2][69],(LPCTSTR)text);
            SET_CHECK(2,71,REG_DMA3CNT_H&BIT(9));
            SET_CHECK(2,73,REG_DMA3CNT_H&BIT(14));
            SET_CHECK(2,75,REG_DMA3CNT_H&BIT(15));

            SET_CHECK(2,76,REG_DMA3CNT_H&BIT(11));

            break;
        }
        case 3: //Timers
        {
            char text[10];

            const char * tmrfreq[4] = { "16.78MHz","262.2KHz","65.54KHz","16.38KHz" };
            const char * clkspertick[4] = { "1", "64", "256", "1024" };

            //Timer 0
            sprintf(text,"%04X",REG_TM0CNT_L); SetWindowText(hTabPageItem[3][2],(LPCTSTR)text);
            sprintf(text,"%04X",gba_timergetstart0()); SetWindowText(hTabPageItem[3][4],(LPCTSTR)text);
            sprintf(text,"%04X",REG_TM0CNT_H); SetWindowText(hTabPageItem[3][6],(LPCTSTR)text);
            if(REG_TM0CNT_H&BIT(2))
            {
                SetWindowText(hTabPageItem[3][8],(LPCTSTR)"--------");
                SetWindowText(hTabPageItem[3][10],(LPCTSTR)"----");
            }
            else
            {
                SetWindowText(hTabPageItem[3][8],(LPCTSTR)tmrfreq[REG_TM0CNT_H&3]);
                SetWindowText(hTabPageItem[3][10],(LPCTSTR)clkspertick[REG_TM0CNT_H&3]);
            }
            SET_CHECK(3,12,REG_TM0CNT_H&BIT(2));
            SET_CHECK(3,14,REG_TM0CNT_H&BIT(6));
            SET_CHECK(3,16,REG_TM0CNT_H&BIT(7));

            //Timer 1
            sprintf(text,"%04X",REG_TM1CNT_L); SetWindowText(hTabPageItem[3][19],(LPCTSTR)text);
            sprintf(text,"%04X",gba_timergetstart1()); SetWindowText(hTabPageItem[3][21],(LPCTSTR)text);
            sprintf(text,"%04X",REG_TM1CNT_H); SetWindowText(hTabPageItem[3][23],(LPCTSTR)text);
            if(REG_TM1CNT_H&BIT(2))
            {
                SetWindowText(hTabPageItem[3][25],(LPCTSTR)"--------");
                SetWindowText(hTabPageItem[3][27],(LPCTSTR)"----");
            }
            else
            {
                SetWindowText(hTabPageItem[3][25],(LPCTSTR)tmrfreq[REG_TM1CNT_H&3]);
                SetWindowText(hTabPageItem[3][27],(LPCTSTR)clkspertick[REG_TM1CNT_H&3]);
            }
            SET_CHECK(3,29,REG_TM1CNT_H&BIT(2));
            SET_CHECK(3,31,REG_TM1CNT_H&BIT(6));
            SET_CHECK(3,33,REG_TM1CNT_H&BIT(7));

            //Timer 2
            sprintf(text,"%04X",REG_TM2CNT_L); SetWindowText(hTabPageItem[3][36],(LPCTSTR)text);
            sprintf(text,"%04X",gba_timergetstart2()); SetWindowText(hTabPageItem[3][38],(LPCTSTR)text);
            sprintf(text,"%04X",REG_TM2CNT_H); SetWindowText(hTabPageItem[3][40],(LPCTSTR)text);
            if(REG_TM2CNT_H&BIT(2))
            {
                SetWindowText(hTabPageItem[3][42],(LPCTSTR)"--------");
                SetWindowText(hTabPageItem[3][44],(LPCTSTR)"----");
            }
            else
            {
                SetWindowText(hTabPageItem[3][42],(LPCTSTR)tmrfreq[REG_TM2CNT_H&3]);
                SetWindowText(hTabPageItem[3][44],(LPCTSTR)clkspertick[REG_TM2CNT_H&3]);
            }
            SET_CHECK(3,46,REG_TM2CNT_H&BIT(2));
            SET_CHECK(3,48,REG_TM2CNT_H&BIT(6));
            SET_CHECK(3,50,REG_TM2CNT_H&BIT(7));

            //Timer 3
            sprintf(text,"%04X",REG_TM3CNT_L); SetWindowText(hTabPageItem[3][53],(LPCTSTR)text);
            sprintf(text,"%04X",gba_timergetstart3()); SetWindowText(hTabPageItem[3][55],(LPCTSTR)text);
            sprintf(text,"%04X",REG_TM3CNT_H); SetWindowText(hTabPageItem[3][57],(LPCTSTR)text);
            if(REG_TM3CNT_H&BIT(2))
            {
                SetWindowText(hTabPageItem[3][59],(LPCTSTR)"--------");
                SetWindowText(hTabPageItem[3][61],(LPCTSTR)"----");
            }
            else
            {
                SetWindowText(hTabPageItem[3][59],(LPCTSTR)tmrfreq[REG_TM3CNT_H&3]);
                SetWindowText(hTabPageItem[3][61],(LPCTSTR)clkspertick[REG_TM3CNT_H&3]);
            }
            SET_CHECK(3,63,REG_TM3CNT_H&BIT(2));
            SET_CHECK(3,65,REG_TM3CNT_H&BIT(6));
            SET_CHECK(3,67,REG_TM3CNT_H&BIT(7));

            break;
        }
        case 4: //Sound
        {
            char text[50];

            //Channel 1
            sprintf(text,"%04X",REG_SOUND1CNT_L); SetWindowText(hTabPageItem[4][2],(LPCTSTR)text);
            sprintf(text,"%04X",REG_SOUND1CNT_H); SetWindowText(hTabPageItem[4][4],(LPCTSTR)text);
            sprintf(text,"%04X",REG_SOUND1CNT_X); SetWindowText(hTabPageItem[4][6],(LPCTSTR)text);

            sprintf(text,"Volume: %2d",gba_debug_get_psg_vol(1));
            SetWindowText(hTabPageItem[4][7],(LPCTSTR)text);
            sprintf(text,"Freq: %.2f Hz",131072.0/(float)(2048-(REG_SOUND1CNT_X&0x7FF)));
            SetWindowText(hTabPageItem[4][8],(LPCTSTR)text);

            //Channel 2
            sprintf(text,"%04X",REG_SOUND2CNT_L); SetWindowText(hTabPageItem[4][11],(LPCTSTR)text);
            sprintf(text,"%04X",REG_SOUND2CNT_H); SetWindowText(hTabPageItem[4][13],(LPCTSTR)text);

            sprintf(text,"Volume: %2d",gba_debug_get_psg_vol(2));
            SetWindowText(hTabPageItem[4][14],(LPCTSTR)text);
            sprintf(text,"Freq: %.2f Hz",131072.0/(float)(2048-(REG_SOUND2CNT_H&0x7FF)));
            SetWindowText(hTabPageItem[4][15],(LPCTSTR)text);

            //Channel 3
            sprintf(text,"%04X",REG_SOUND3CNT_L); SetWindowText(hTabPageItem[4][18],(LPCTSTR)text);
            sprintf(text,"%04X",REG_SOUND3CNT_H); SetWindowText(hTabPageItem[4][20],(LPCTSTR)text);
            sprintf(text,"%04X",REG_SOUND3CNT_X); SetWindowText(hTabPageItem[4][22],(LPCTSTR)text);

            sprintf(text,"Volume: %2d",gba_debug_get_psg_vol(3));
            SetWindowText(hTabPageItem[4][23],(LPCTSTR)text);
            sprintf(text,"Freq: %.2f Hz",2097152.0/(float)(2048-(REG_SOUND3CNT_X&0x7FF)));
            SetWindowText(hTabPageItem[4][24],(LPCTSTR)text);

            //Channel 4
            sprintf(text,"%04X",REG_SOUND4CNT_L); SetWindowText(hTabPageItem[4][27],(LPCTSTR)text);
            sprintf(text,"%04X",REG_SOUND4CNT_H); SetWindowText(hTabPageItem[4][29],(LPCTSTR)text);

            sprintf(text,"Volume: %2d",gba_debug_get_psg_vol(4));
            SetWindowText(hTabPageItem[4][30],(LPCTSTR)text);
            const s32 NoiseFreqRatio[8] = {1048576,524288,262144,174763,131072,104858,87381,74898 };
            sprintf(text,"Freq: %d Hz",NoiseFreqRatio[REG_SOUND4CNT_H&3] >> (((REG_SOUND4CNT_H>>4)&3) + 1));
            SetWindowText(hTabPageItem[4][31],(LPCTSTR)text);

            //DMA Sound/Control
            sprintf(text,"%04X",REG_SOUNDCNT_L); SetWindowText(hTabPageItem[4][34],(LPCTSTR)text);
            sprintf(text,"%04X",REG_SOUNDCNT_H); SetWindowText(hTabPageItem[4][36],(LPCTSTR)text);
            sprintf(text,"%04X",REG_SOUNDCNT_X); SetWindowText(hTabPageItem[4][38],(LPCTSTR)text);

            u32 sndl = REG_SOUNDCNT_L;
            u32 sndh = REG_SOUNDCNT_H;
            SET_CHECK(4,47,sndl&BIT(8)); SET_CHECK(4,48,sndl&BIT(9));
            SET_CHECK(4,49,sndl&BIT(10)); SET_CHECK(4,50,sndl&BIT(11));
            SET_CHECK(4,51,sndh&BIT(8)); SET_CHECK(4,52,sndh&BIT(12));

            SET_CHECK(4,53,sndl&BIT(12)); SET_CHECK(4,54,sndl&BIT(13));
            SET_CHECK(4,55,sndl&BIT(14)); SET_CHECK(4,56,sndl&BIT(15));
            SET_CHECK(4,57,sndh&BIT(9)); SET_CHECK(4,58,sndh&BIT(13));

            SET_CHECK(4,60,REG_SOUNDCNT_X&BIT(7));

            sprintf(text,"R: %d||L: %d",sndl&7,(sndl>>4)&7);
            SetWindowText(hTabPageItem[4][62],(LPCTSTR)text);

            sprintf(text,"Tmr A: %d",(sndh&BIT(10)) != 0);
            SetWindowText(hTabPageItem[4][63],(LPCTSTR)text);
            sprintf(text,"Tmr B: %d",(sndh&BIT(14)) != 0);
            SetWindowText(hTabPageItem[4][64],(LPCTSTR)text);

            const char * psgvol[4] = {" 25"," 50","100","---"};
            sprintf(text,"PSG:%s||A:%3d|B:%3d", psgvol[sndh&3],sndh&BIT(2)?100:50,sndh&BIT(3)?100:50);
            SetWindowText(hTabPageItem[4][66],(LPCTSTR)text);

            sprintf(text,"%08X",REG_FIFO_A); SetWindowText(hTabPageItem[4][68],(LPCTSTR)text);
            sprintf(text,"%08X",REG_FIFO_B); SetWindowText(hTabPageItem[4][70],(LPCTSTR)text);

            //Wave RAM
            u16 * wav = (u16*)REG_WAVE_RAM;
            sprintf(text,"%04X|%04X|%04X|%04X",wav[0],wav[1],wav[2],wav[3]);
            SetWindowText(hTabPageItem[4][72],(LPCTSTR)text);
            sprintf(text,"%04X|%04X|%04X|%04X",wav[4],wav[5],wav[6],wav[7]);
            SetWindowText(hTabPageItem[4][73],(LPCTSTR)text);

            if(REG_SOUND3CNT_L&BIT(5)) { SET_CHECK(4,74,1); SET_CHECK(4,75,1); }
            else
            {
                if(REG_SOUND3CNT_L&BIT(5)) { SET_CHECK(4,74,0); SET_CHECK(4,75,1); }
                else { SET_CHECK(4,74,1); SET_CHECK(4,75,0); }
            }

            //PWM Control
            sprintf(text,"%04X",REG_SOUNDBIAS); SetWindowText(hTabPageItem[4][79],(LPCTSTR)text);

            const char * biasinfo[4] = { "9bit | 32.768kHz", "8bit | 65.536kHz",
                "7bit | 131.072kHz", "6bit | 262.144kHz" };

            SetWindowText(hTabPageItem[4][80],(LPCTSTR)biasinfo[(REG_SOUNDBIAS>>14)&3]);
            break;
        }
        case 5: //Other
        {
            char text[5];
            //IE,IF,IME
            sprintf(text,"%04X",REG_IE); SetWindowText(hTabPageItem[5][2],(LPCTSTR)text);
            sprintf(text,"%04X",REG_IF); SetWindowText(hTabPageItem[5][4],(LPCTSTR)text);
            sprintf(text,"%04X",REG_IME); SetWindowText(hTabPageItem[5][6],(LPCTSTR)text);

            //"IE IF BIOS" FLAGS
            u16 biosflags = GBA_MemoryReadFast16(0x03007FF8);
            SET_CHECK(5,8,REG_IE&BIT(0)); SET_CHECK(5,9,REG_IF&BIT(0)); SET_CHECK(5,10,biosflags&BIT(0));
            SET_CHECK(5,12,REG_IE&BIT(1)); SET_CHECK(5,13,REG_IF&BIT(1)); SET_CHECK(5,14,biosflags&BIT(1));
            SET_CHECK(5,16,REG_IE&BIT(2)); SET_CHECK(5,17,REG_IF&BIT(2)); SET_CHECK(5,18,biosflags&BIT(2));
            SET_CHECK(5,20,REG_IE&BIT(3)); SET_CHECK(5,21,REG_IF&BIT(3)); SET_CHECK(5,22,biosflags&BIT(3));
            SET_CHECK(5,24,REG_IE&BIT(4)); SET_CHECK(5,25,REG_IF&BIT(4)); SET_CHECK(5,26,biosflags&BIT(4));
            SET_CHECK(5,28,REG_IE&BIT(5)); SET_CHECK(5,29,REG_IF&BIT(5)); SET_CHECK(5,30,biosflags&BIT(5));
            SET_CHECK(5,32,REG_IE&BIT(6)); SET_CHECK(5,33,REG_IF&BIT(6)); SET_CHECK(5,34,biosflags&BIT(6));
            SET_CHECK(5,36,REG_IE&BIT(7)); SET_CHECK(5,37,REG_IF&BIT(7)); SET_CHECK(5,38,biosflags&BIT(7));
            SET_CHECK(5,40,REG_IE&BIT(8)); SET_CHECK(5,41,REG_IF&BIT(8)); SET_CHECK(5,42,biosflags&BIT(8));
            SET_CHECK(5,44,REG_IE&BIT(9)); SET_CHECK(5,45,REG_IF&BIT(9)); SET_CHECK(5,46,biosflags&BIT(9));
            SET_CHECK(5,48,REG_IE&BIT(10)); SET_CHECK(5,49,REG_IF&BIT(10)); SET_CHECK(5,50,biosflags&BIT(10));
            SET_CHECK(5,52,REG_IE&BIT(11)); SET_CHECK(5,53,REG_IF&BIT(11)); SET_CHECK(5,54,biosflags&BIT(11));
            SET_CHECK(5,56,REG_IE&BIT(12)); SET_CHECK(5,57,REG_IF&BIT(12)); SET_CHECK(5,58,biosflags&BIT(12));
            SET_CHECK(5,60,REG_IE&BIT(13)); SET_CHECK(5,61,REG_IF&BIT(13)); SET_CHECK(5,62,biosflags&BIT(13));
            break;
        }
        default:
            break;
    }
}

static void GLWindow_IOViewerMakePages(HWND hWnd)
{
    //-----------------------------------------------------------------------------------
    //                        FIRST PAGE - DISPLAY
    //-----------------------------------------------------------------------------------
    {
        //DISPCNT
        hTabPageItem[0][0] = CreateWindow(TEXT("button"), TEXT("LCD Control"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5, 25, 150, 300, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[0][0], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        #define CREATE_REG_STATIC_TEXT(page,baseid,basex,basey,text) \
        { \
            hTabPageItem[page][baseid] = CreateWindow(TEXT("static"), TEXT(text), WS_CHILD | WS_VISIBLE, \
                            basex+39, basey+1, 7*(strlen(text)-1), 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[page][baseid], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[page][baseid+1] = CreateWindow(TEXT("static"), TEXT("0000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            basex, basey, 35, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[page][baseid+1], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
        }


        #define CREATE_CHECK_STATIC(page,baseid,basex,basey,text) \
        { \
            hTabPageItem[page][baseid] = CreateWindow(TEXT("static"), TEXT(text), \
                            WS_CHILD | WS_VISIBLE, \
                            basex+19, basey+1, 12 + 5*strlen(text), 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[page][baseid], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[page][baseid+1] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
        }


        //DISPSTAT, VCOUNT
        hTabPageItem[0][31] = CreateWindow(TEXT("button"), TEXT("LCD Status"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    160, 25, 135, 202, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[0][31], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        CREATE_REG_STATIC_TEXT(0,32, 165, 45, "004h DISPSTAT");

        CREATE_CHECK_STATIC(0,34, 165, 63, "V-Blank flag");
        CREATE_CHECK_STATIC(0,36, 165, 79, "H-Blank flag");
        CREATE_CHECK_STATIC(0,38, 165, 95, "V-Count flag");
        CREATE_CHECK_STATIC(0,40, 165, 111, "V-Blank IRQ Enable");
        CREATE_CHECK_STATIC(0,42, 165, 127, "H-Blank IRQ Enable");
        CREATE_CHECK_STATIC(0,44, 165, 143, "V-Count IRQ Enable");

        hTabPageItem[0][46] = CreateWindow(TEXT("static"), TEXT("LYC"),
                        WS_CHILD | WS_VISIBLE,
                        195, 162, 80, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][46], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        hTabPageItem[0][47] = CreateWindow(TEXT("static"), TEXT("0"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        165, 163, 26, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][47], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        CREATE_REG_STATIC_TEXT(0,48, 165, 183, "006h VCOUNT");

        hTabPageItem[0][50] = CreateWindow(TEXT("static"), TEXT("LY"),
                        WS_CHILD | WS_VISIBLE,
                        195, 204, 80, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][50], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        hTabPageItem[0][51] = CreateWindow(TEXT("static"), TEXT("0"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        165, 203, 26, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][51], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));


        //MOSAIC
        hTabPageItem[0][52] = CreateWindow(TEXT("button"), TEXT("Effects"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    160, 227, 328, 98, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[0][52], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        CREATE_REG_STATIC_TEXT(0,53, 165,243, "04Ch MOSAIC");

        hTabPageItem[0][55] = CreateWindow(TEXT("static"), TEXT("Sp  1, 1|Bg  1, 1"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        165, 265, 123, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][55], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));



        //WINDOWS
        hTabPageItem[0][61] = CreateWindow(TEXT("button"), TEXT("Windows"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    300, 25, 188, 202, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[0][61], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        #define CREATE_DOUBLE_REG_STATIC_TEXT(page,baseid,basex,basey,text) \
        { \
            hTabPageItem[page][baseid] = CreateWindow(TEXT("static"), TEXT(text), WS_CHILD | WS_VISIBLE, \
                            basex+68, basey+1, 110, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[page][baseid], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[page][baseid+1] = CreateWindow(TEXT("static"), TEXT("0000|0000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            basex, basey, 66, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[page][baseid+1], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
        }

        CREATE_DOUBLE_REG_STATIC_TEXT(0,62, 305,45, "040/044h WIN0[H/V]");
        CREATE_DOUBLE_REG_STATIC_TEXT(0,64, 305,65, "042/046h WIN1[H/V]");

        hTabPageItem[0][66] = CreateWindow(TEXT("static"), TEXT(" Window 0"), WS_CHILD | WS_VISIBLE,
                        305+118, 85+1, 55, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][66], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][67] = CreateWindow(TEXT("static"), TEXT("000,000|000,000"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        305, 85, 110, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][67], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        hTabPageItem[0][68] = CreateWindow(TEXT("static"), TEXT(" Window 1"), WS_CHILD | WS_VISIBLE,
                        305+118, 105+1, 55, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][68], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][69] = CreateWindow(TEXT("static"), TEXT("000,000|000,000"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        305, 105, 110, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][69], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        CREATE_DOUBLE_REG_STATIC_TEXT(0,70, 305,125, "048/04Ah WIN[IN/OUT]");

        hTabPageItem[0][72] = CreateWindow(TEXT("static"), TEXT("Bg0"), WS_CHILD | WS_VISIBLE,
                        305+30, 145, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][72], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][73] = CreateWindow(TEXT("static"), TEXT("Bg1"), WS_CHILD | WS_VISIBLE,
                        305+55, 145, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][73], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][74] = CreateWindow(TEXT("static"), TEXT("Bg2"), WS_CHILD | WS_VISIBLE,
                        305+80, 145, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][74], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][75] = CreateWindow(TEXT("static"), TEXT("Bg3"), WS_CHILD | WS_VISIBLE,
                        305+105, 145, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][75], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][76] = CreateWindow(TEXT("static"), TEXT("Obj"), WS_CHILD | WS_VISIBLE,
                        305+130, 145, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][76], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][77] = CreateWindow(TEXT("static"), TEXT("Spe"), WS_CHILD | WS_VISIBLE,
                        305+155, 145, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][77], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        hTabPageItem[0][78] = CreateWindow(TEXT("static"), TEXT("IN0"), WS_CHILD | WS_VISIBLE,
                        305, 145+16, 55, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][78], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][79] = CreateWindow(TEXT("static"), TEXT("IN1"), WS_CHILD | WS_VISIBLE,
                        305, 145+32, 55, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][79], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][80] = CreateWindow(TEXT("static"), TEXT("OUT"), WS_CHILD | WS_VISIBLE,
                        305, 145+48, 55, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][80], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][81] = CreateWindow(TEXT("static"), TEXT("OBJ"), WS_CHILD | WS_VISIBLE,
                        305, 145+64, 55, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][81], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        #define CREATE_6_CHECK(page,baseid,basex,basey) \
        { \
            hTabPageItem[page][baseid] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+1] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+25, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+2] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+50, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+3] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+75, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+4] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+100, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+5] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+125, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
        }

        CREATE_6_CHECK(0,82,308+30,144+16);
        CREATE_6_CHECK(0,88,308+30,144+32);
        CREATE_6_CHECK(0,94,308+30,144+48);
        CREATE_6_CHECK(0,100,308+30,144+64);

        //BLENDING
        CREATE_REG_STATIC_TEXT(0,56, 165,283, "052h BLDALPHA");
        CREATE_REG_STATIC_TEXT(0,106, 165,303, "054h BLDY");

        CREATE_REG_STATIC_TEXT(0,108, 295,238, "050h BLDCNT");

        hTabPageItem[0][58] = CreateWindow(TEXT("static"), TEXT("EVY  0"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        431, 238, 50, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][58], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        hTabPageItem[0][110] = CreateWindow(TEXT("static"), TEXT("EVA  0|EVB  0"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        295, 258, 98, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][110], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        hTabPageItem[0][111] = CreateWindow(TEXT("static"), TEXT("Mode: Blend"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        398, 258, 83, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][111], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));


        hTabPageItem[0][112] = CreateWindow(TEXT("static"), TEXT("Bg0"), WS_CHILD | WS_VISIBLE,
                        295+30, 276, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][112], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][113] = CreateWindow(TEXT("static"), TEXT("Bg1"), WS_CHILD | WS_VISIBLE,
                        295+55, 276, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][113], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][114] = CreateWindow(TEXT("static"), TEXT("Bg2"), WS_CHILD | WS_VISIBLE,
                        295+80, 276, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][114], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][115] = CreateWindow(TEXT("static"), TEXT("Bg3"), WS_CHILD | WS_VISIBLE,
                        295+105, 276, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][115], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][116] = CreateWindow(TEXT("static"), TEXT("Obj"), WS_CHILD | WS_VISIBLE,
                        295+130, 276, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][116], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][117] = CreateWindow(TEXT("static"), TEXT("BD"), WS_CHILD | WS_VISIBLE,
                        295+155, 276, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][117], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        hTabPageItem[0][118] = CreateWindow(TEXT("static"), TEXT("1st"), WS_CHILD | WS_VISIBLE,
                        295, 276+15, 55, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][118], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[0][119] = CreateWindow(TEXT("static"), TEXT("2nd"), WS_CHILD | WS_VISIBLE,
                        295, 276+30, 55, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[0][119], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        CREATE_6_CHECK(0,120,295+30,276+15);
        CREATE_6_CHECK(0,126,295+30,276+30);
    }
    //-----------------------------------------------------------------------------------
    //                        SECOND PAGE - BACKGROUNDS
    //-----------------------------------------------------------------------------------
    {
        hTabPageItem[1][0] = CreateWindow(TEXT("button"), TEXT("BG control"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5, 25, 485, 170, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[1][0], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        #define BGnCONTROL(idbase,x,y,regname) \
        { \
            CREATE_REG_STATIC_TEXT(1,idbase, x+6, y+20, regname); \
            hTabPageItem[1][idbase+2] = CreateWindow(TEXT("static"), TEXT("Priority"), WS_CHILD | WS_VISIBLE, \
                            x+25, y+42, 60, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][idbase+2], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][idbase+3] = CreateWindow(TEXT("static"), TEXT("0"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+6, y+40, 12, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][idbase+3], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][idbase+4] = CreateWindow(TEXT("static"), TEXT("CBB"), WS_CHILD | WS_VISIBLE, \
                            x+72, y+60, 40, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][idbase+4], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][idbase+5] = CreateWindow(TEXT("static"), TEXT("06000000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+6, y+60, 60, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][idbase+5], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][idbase+6] = CreateWindow(TEXT("static"), TEXT("SBB"), WS_CHILD | WS_VISIBLE, \
                            x+72, y+80, 40, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][idbase+6], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][idbase+7] = CreateWindow(TEXT("static"), TEXT("06000000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+6, y+80, 60, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][idbase+7], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][idbase+8] = CreateWindow(TEXT("static"), TEXT("Size"), WS_CHILD | WS_VISIBLE, \
                            x+82, y+100, 30, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][idbase+8], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][idbase+9] = CreateWindow(TEXT("static"), TEXT("256x256"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+6, y+100, 70, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][idbase+9], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            CREATE_CHECK_STATIC(1,idbase+10, x+6, y+118, "Mosaic"); \
            CREATE_CHECK_STATIC(1,idbase+12, x+6, y+134, "256 colors"); \
            CREATE_CHECK_STATIC(1,idbase+14, x+6, y+150, "Overflow Wrap"); \
        }

        BGnCONTROL(1, 5,25, "008h BG0CNT");
        BGnCONTROL(17, 127,25, "00Ah BG1CNT");
        BGnCONTROL(33, 249,25, "00Ch BG2CNT");
        BGnCONTROL(49, 366,25, "00Eh BG3CNT");

        hTabPageItem[1][65] = CreateWindow(TEXT("button"), TEXT("BG scroll"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5, 195, 200, 100, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[1][65], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        #define CREATE_BGnXOFFSET(base_id, x, y, text) \
        { \
            hTabPageItem[1][base_id] = CreateWindow(TEXT("static"), TEXT(text), WS_CHILD | WS_VISIBLE, \
                            x+66, y, 120, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][base_id], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][base_id+1] = CreateWindow(TEXT("static"), TEXT("000,000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x, y, 60, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][base_id+1], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
        }
        CREATE_BGnXOFFSET(66, 11,210, "010/012h BG0[H/V]OFS");
        CREATE_BGnXOFFSET(68, 11,230, "014/016h BG1[H/V]OFS");
        CREATE_BGnXOFFSET(70, 11,250, "018/01Ah BG2[H/V]OFS");
        CREATE_BGnXOFFSET(72, 11,270, "01C/01Eh BG3[H/V]OFS");

        hTabPageItem[1][74] = CreateWindow(TEXT("button"), TEXT("BG affine transformation"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    210, 195, 280, 130, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[1][74], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        #define CREATE_BGXY(base_id, x, y, text) \
        { \
            hTabPageItem[1][base_id] = CreateWindow(TEXT("static"), TEXT(text), WS_CHILD | WS_VISIBLE, \
                            x+10, y, 100, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][base_id], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][base_id+1] = CreateWindow(TEXT("static"), TEXT("0000000,0000000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x, y+20, 115, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][base_id+1], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
        }
        CREATE_BGXY(75, 225,210, "028/02Ch BG2[X/Y]");
        CREATE_BGXY(77, 360,210, "038/03Ch BG3[X/Y]");

        #define CREATE_AFFINE_MATRIX(base_id, x, y, text) \
        { \
            hTabPageItem[1][base_id] = CreateWindow(TEXT("static"), TEXT(text), WS_CHILD | WS_VISIBLE, \
                            x, y, 110, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][base_id], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][base_id+1] = CreateWindow(TEXT("static"), TEXT("0000,0000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+20, y+20, 70, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][base_id+1], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            hTabPageItem[1][base_id+2] = CreateWindow(TEXT("static"), TEXT("0000,0000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+20, y+40, 70, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[1][base_id+2], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
        }
        CREATE_AFFINE_MATRIX(79, 230,255, "020-026h BG2[PA-PD]");
        CREATE_AFFINE_MATRIX(82, 365,255, "030-036h BG3[PA-PD]");

        hTabPageItem[1][85] = CreateWindow(TEXT("static"), TEXT("Video Mode: 0 (4x text)"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        5, 302, 200, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[1][85], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));
    }
    //-----------------------------------------------------------------------------------
    //                        THIRD PAGE - DMA
    //-----------------------------------------------------------------------------------
    {
        #define CREATE_REG32(page,base_id, x, y, text) \
        { \
            hTabPageItem[page][base_id] = CreateWindow(TEXT("static"), TEXT(text), WS_CHILD | WS_VISIBLE, \
                            x+65, y, 80, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[page][base_id], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[page][base_id+1] = CreateWindow(TEXT("static"), TEXT("00000000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x, y, 60, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[page][base_id+1], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
        }

        #define CREATE_DMA_CHANNEL(idbase,x,y,text,srctext,dsttext,cnttext) \
        { \
            hTabPageItem[2][idbase] = CreateWindow(TEXT("button"), TEXT(text), \
                        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, \
                        x, y, 240, 150, hWnd, (HMENU) 0, hInstance, NULL); \
            SendMessage(hTabPageItem[2][idbase], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0)); \
            CREATE_REG32(2,idbase+1, x+6, y+21, srctext); \
            CREATE_REG32(2,idbase+3, x+6, y+42, dsttext); \
            CREATE_REG32(2,idbase+5, x+6, y+63, cnttext); \
            hTabPageItem[2][idbase+7] = CreateWindow(TEXT("static"), TEXT("Increment"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+158, y+21, 75, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[2][idbase+7], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            hTabPageItem[2][idbase+8] = CreateWindow(TEXT("static"), TEXT("Increment"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+158, y+42, 75, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[2][idbase+8], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            CREATE_CHECK_STATIC(2,idbase+9, x+158, y+63, "32 bit"); \
            hTabPageItem[2][idbase+11] = CreateWindow(TEXT("static"), TEXT("Start immediately"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+6, y+84, 145, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[2][idbase+11], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            hTabPageItem[2][idbase+12] = CreateWindow(TEXT("static"), TEXT("000000 bytes"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+6, y+105, 100, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[2][idbase+12], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            CREATE_CHECK_STATIC(2,idbase+13, x+158, y+84, "Repeat"); \
            CREATE_CHECK_STATIC(2,idbase+15, x+118, y+105, "IRQ Enable"); \
            CREATE_CHECK_STATIC(2,idbase+17, x+6, y+126, "DMA Enable"); \
        }

        CREATE_DMA_CHANNEL(0, 5,25, "DMA 0", "0B0h DMA0SAD","0B4h DMA0DAD","0B8h DMA0CNT");
        CREATE_DMA_CHANNEL(19, 249,25, "DMA 1", "0BCh DMA1SAD","0C0h DMA1DAD","0C4h DMA1CNT");
        CREATE_DMA_CHANNEL(38, 5,175, "DMA 2", "0C8h DMA2SAD","0CCh DMA2DAD","0D0h DMA2CNT");
        CREATE_DMA_CHANNEL(57, 249,175, "DMA 3", "0D4h DMA3SAD","0D8h DMA3DAD","0DCh DMA3CNT");
        CREATE_CHECK_STATIC(2,76, 249+118, 175+126, "Game Pak DRQ");
    }
    //-----------------------------------------------------------------------------------
    //                        FOURTH PAGE - TIMERS
    //-----------------------------------------------------------------------------------
    {
        #define CREATE_TIMER(idbase,x,y,text,l_text,h_text) \
        { \
            hTabPageItem[3][idbase] = CreateWindow(TEXT("button"), TEXT(text), \
                        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, \
                        x, y, 240, 150, hWnd, (HMENU) 0, hInstance, NULL); \
            SendMessage(hTabPageItem[3][idbase], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0)); \
            CREATE_REG_STATIC_TEXT(3,idbase+1, x+6, y+21, l_text); \
            hTabPageItem[3][idbase+3] = CreateWindow(TEXT("static"), TEXT("On reload"), WS_CHILD | WS_VISIBLE, \
                            x+177, y+21, 60, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[3][idbase+3], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[3][idbase+4] = CreateWindow(TEXT("static"), TEXT("0000"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+136, y+21, 35, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[3][idbase+4], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            CREATE_REG_STATIC_TEXT(3,idbase+5, x+6, y+42, h_text); \
            hTabPageItem[3][idbase+7] = CreateWindow(TEXT("static"), TEXT("Frequency"), WS_CHILD | WS_VISIBLE, \
                            x+72, y+63, 55, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[3][idbase+7], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[3][idbase+8] = CreateWindow(TEXT("static"), TEXT("16.78MHz"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+6, y+63, 60, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[3][idbase+8], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            hTabPageItem[3][idbase+9] = CreateWindow(TEXT("static"), TEXT("CPU clocks per tick"), WS_CHILD | WS_VISIBLE, \
                            x+47, y+84, 120, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[3][idbase+9], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
            hTabPageItem[3][idbase+10] = CreateWindow(TEXT("static"), TEXT("1024"), \
                            WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER, \
                            x+6, y+84, 35, 17, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[3][idbase+10], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0)); \
            CREATE_CHECK_STATIC(3,idbase+11, x+140, y+63, "Cascade"); \
            CREATE_CHECK_STATIC(3,idbase+13, x+6, y+105, "IRQ Enable"); \
            CREATE_CHECK_STATIC(3,idbase+15, x+6, y+126, "Timer Enabled"); \
        }

        CREATE_TIMER(0, 5,25, "Timer 0", "100h TM0CNT_L","102h TM0CNT_H");
        CREATE_TIMER(17, 249,25, "Timer 1", "104h TM1CNT_L","106h TM1CNT_H");
        CREATE_TIMER(34, 5,175, "Timer 2", "108h TM2CNT_L","10Ah TM2CNT_H");
        CREATE_TIMER(51, 249,175, "Timer 3", "10Ch TM3CNT_L","10Eh TM3CNT_H");
    }
    //-----------------------------------------------------------------------------------
    //                        FIFTH PAGE - SOUND
    //-----------------------------------------------------------------------------------
    {
        //Channel 1
        hTabPageItem[4][0] = CreateWindow(TEXT("button"), TEXT("Channel 1"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5,25, 155,125, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[4][0], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));
        CREATE_REG_STATIC_TEXT(4,1, 11,46, "060h SOUND1CNT_L");
        CREATE_REG_STATIC_TEXT(4,3, 11,66, "062h SOUND1CNT_H");
        CREATE_REG_STATIC_TEXT(4,5, 11,86, "064h SOUND1CNT_X");
        hTabPageItem[4][7] = CreateWindow(TEXT("static"), TEXT("Volume: 15"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        11, 106, 80, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][7], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));
        hTabPageItem[4][8] = CreateWindow(TEXT("static"), TEXT("Freq: XXXXXX.XX Hz"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        11, 126, 130, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][8], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        //Channel 2
        hTabPageItem[4][9] = CreateWindow(TEXT("button"), TEXT("Channel 2"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5,150, 155,105, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[4][9], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));
        CREATE_REG_STATIC_TEXT(4,10, 11,171, "068h SOUND2CNT_L");
        CREATE_REG_STATIC_TEXT(4,12, 11,191, "06Ch SOUND2CNT_H");
        hTabPageItem[4][14] = CreateWindow(TEXT("static"), TEXT("Volume: 15"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        11, 211, 80, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][14], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));
        hTabPageItem[4][15] = CreateWindow(TEXT("static"), TEXT("Freq: XXXXXX.XX Hz"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        11, 231, 130, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][15], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        //Channel 3
        hTabPageItem[4][16] = CreateWindow(TEXT("button"), TEXT("Channel 3"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    165,25, 155,125, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[4][16], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));
        CREATE_REG_STATIC_TEXT(4,17, 171,46, "070h SOUND3CNT_L");
        CREATE_REG_STATIC_TEXT(4,19, 171,66, "072h SOUND3CNT_H");
        CREATE_REG_STATIC_TEXT(4,21, 171,86, "074h SOUND3CNT_X");
        hTabPageItem[4][23] = CreateWindow(TEXT("static"), TEXT("Volume: 15"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        171, 106, 80, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][23], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));
        hTabPageItem[4][24] = CreateWindow(TEXT("static"), TEXT("Freq: XXXXXX.XX Hz"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        171, 126, 130, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][24], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        //Channel 4
        hTabPageItem[4][25] = CreateWindow(TEXT("button"), TEXT("Channel 4"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    165,150, 155,105, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[4][25], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));
        CREATE_REG_STATIC_TEXT(4,26, 171,171, "078h SOUND4CNT_L");
        CREATE_REG_STATIC_TEXT(4,28, 171,191, "07Ch SOUND4CNT_H");
        hTabPageItem[4][30] = CreateWindow(TEXT("static"), TEXT("Volume: 15"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        171, 211, 80, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][30], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));
        hTabPageItem[4][31] = CreateWindow(TEXT("static"), TEXT("Freq: XXXXXX.XX Hz"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        171, 231, 130, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][31], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        //DMA Sound/Control
        hTabPageItem[4][32] = CreateWindow(TEXT("button"), TEXT("DMA Channels/Control"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    325,25, 163,300, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[4][32], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));
        CREATE_REG_STATIC_TEXT(4,33, 331,46, "080h SOUNDCNT_L");
        CREATE_REG_STATIC_TEXT(4,35, 331,66, "082h SOUNDCNT_H");
        CREATE_REG_STATIC_TEXT(4,37, 331,86, "084h SOUNDCNT_X");

        hTabPageItem[4][39] = CreateWindow(TEXT("static"), TEXT("C1"), WS_CHILD | WS_VISIBLE,
                        340+20, 106, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][39], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[4][40] = CreateWindow(TEXT("static"), TEXT("C2"), WS_CHILD | WS_VISIBLE,
                        340+40, 106, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][40], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[4][41] = CreateWindow(TEXT("static"), TEXT("C3"), WS_CHILD | WS_VISIBLE,
                        340+60, 106, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][41], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[4][42] = CreateWindow(TEXT("static"), TEXT("C4"), WS_CHILD | WS_VISIBLE,
                        340+80, 106, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][42], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[4][43] = CreateWindow(TEXT("static"), TEXT("A"), WS_CHILD | WS_VISIBLE,
                        340+100, 106, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][43], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[4][44] = CreateWindow(TEXT("static"), TEXT("B"), WS_CHILD | WS_VISIBLE,
                        340+120, 106, 20, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][44], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        hTabPageItem[4][45] = CreateWindow(TEXT("static"), TEXT("R"), WS_CHILD | WS_VISIBLE,
                        340, 106+16, 15, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][45], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[4][46] = CreateWindow(TEXT("static"), TEXT("L"), WS_CHILD | WS_VISIBLE,
                        340, 106+32, 15, 13, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][46], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        #define CREATE_6_CHECK_COMPACT(page,baseid,basex,basey) \
        { \
            hTabPageItem[page][baseid] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+1] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+20, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+2] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+40, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+3] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+60, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+4] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+80, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[page][baseid+5] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            basex+100, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
        }

        CREATE_6_CHECK_COMPACT(4,47,340+20,106+16);
        CREATE_6_CHECK_COMPACT(4,53,340+20,106+32);

        CREATE_CHECK_STATIC(4,59, 331, 158, "Sound Master Enable");

        hTabPageItem[4][61] = CreateWindow(TEXT("static"), TEXT("PSG Volume:"), WS_CHILD | WS_VISIBLE,
                            331, 178, 120, 15, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][61], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));

        hTabPageItem[4][62] = CreateWindow(TEXT("static"), TEXT("R: 7||L: 7"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        331, 196, 100, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][62], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        hTabPageItem[4][63] = CreateWindow(TEXT("static"), TEXT("Tmr A: 1"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        331, 218, 65, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][63], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));
        hTabPageItem[4][64] = CreateWindow(TEXT("static"), TEXT("Tmr B: 1"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        400, 218, 65, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][64], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        hTabPageItem[4][65] = CreateWindow(TEXT("static"), TEXT("Mixer Volume %:"), WS_CHILD | WS_VISIBLE,
                            331, 240, 120, 15, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][65], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0));
        hTabPageItem[4][66] = CreateWindow(TEXT("static"), TEXT("PSG:100||A:100|B:100"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        331, 258, 150, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][66], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        CREATE_REG32(4,67, 331, 282, "0A0h FIFO_A");
        CREATE_REG32(4,69, 331, 302, "0A4h FIFO_B");

        //Wave RAM
        hTabPageItem[4][71] = CreateWindow(TEXT("button"), TEXT("090h Wave RAM"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5,255, 170,70, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[4][71], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        hTabPageItem[4][72] = CreateWindow(TEXT("static"), TEXT("0000|0000|0000|0000"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        11, 275, 140, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][72], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));
        hTabPageItem[4][73] = CreateWindow(TEXT("static"), TEXT("0000|0000|0000|0000"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        11, 300, 140, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][73], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));

        hTabPageItem[4][74] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                        155, 275, 12, 17, hWnd, NULL, hInstance, NULL);
        hTabPageItem[4][75] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                        155, 300, 12, 17, hWnd, NULL, hInstance, NULL);

        //PWM Control
        hTabPageItem[4][76] = CreateWindow(TEXT("button"), TEXT("PWM Control"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    180,255, 140,70, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[4][76], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        CREATE_REG_STATIC_TEXT(4,78, 185,275, "088h SOUNDBIAS");

        hTabPageItem[4][80] = CreateWindow(TEXT("static"), TEXT("6bit | 262.144kHz"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        185, 300, 130, 17, hWnd, NULL, hInstance, NULL);
        SendMessage(hTabPageItem[4][80], WM_SETFONT, (WPARAM)hFontFixedIO, MAKELPARAM(1, 0));
    }
    //-----------------------------------------------------------------------------------
    //                        SIXTH PAGE - OTHER
    //-----------------------------------------------------------------------------------
    {
        hTabPageItem[5][0] = CreateWindow(TEXT("button"), TEXT("Interrupts"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5, 25, 300, 65, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[5][0], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        CREATE_REG_STATIC_TEXT(5,1, 11, 45, "200h IE");
        CREATE_REG_STATIC_TEXT(5,3, 11, 65, "202h IF");
        CREATE_REG_STATIC_TEXT(5,5, 100, 45, "208h IME");

        hTabPageItem[5][7] = CreateWindow(TEXT("button"), TEXT("IE IF BIOS"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5, 90, 125, 235, hWnd, (HMENU) 0, hInstance, NULL);
        SendMessage(hTabPageItem[5][7], WM_SETFONT, (WPARAM)hFontNormalIO, MAKELPARAM(1, 0));

        #define CREATE_3_CHECK_STATIC(basey, baseid, text) \
        { \
            hTabPageItem[5][baseid] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            11, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[5][baseid+1] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            30, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[5][baseid+2] = CreateWindow(TEXT("button"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX, \
                            49, basey, 12, 17, hWnd, NULL, hInstance, NULL); \
            hTabPageItem[5][baseid+3] = CreateWindow(TEXT("static"), TEXT(text), WS_CHILD | WS_VISIBLE, \
                            70, basey+1, 50, 13, hWnd, NULL, hInstance, NULL); \
            SendMessage(hTabPageItem[5][baseid+3], WM_SETFONT, (WPARAM)hFontIO, MAKELPARAM(1, 0)); \
        }
        CREATE_3_CHECK_STATIC(105,8,"V-Blank");
        CREATE_3_CHECK_STATIC(120,12,"H-Blank");
        CREATE_3_CHECK_STATIC(135,16,"V-Count");
        CREATE_3_CHECK_STATIC(150,20,"Timer 0");
        CREATE_3_CHECK_STATIC(165,24,"Timer 1");
        CREATE_3_CHECK_STATIC(180,28,"Timer 2");
        CREATE_3_CHECK_STATIC(195,32,"Timer 3");
        CREATE_3_CHECK_STATIC(210,36,"Serial");
        CREATE_3_CHECK_STATIC(225,40,"DMA 0");
        CREATE_3_CHECK_STATIC(240,44,"DMA 1");
        CREATE_3_CHECK_STATIC(255,48,"DMA 2");
        CREATE_3_CHECK_STATIC(270,52,"DMA 3");
        CREATE_3_CHECK_STATIC(285,56,"Keypad");
        CREATE_3_CHECK_STATIC(300,60,"Game Pak");
    }
}

#endif // 0
