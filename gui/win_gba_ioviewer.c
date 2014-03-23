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

//-----------------------------------------------------------------------------------

static int WinIDGBAIOViewer;

#define WIN_GBA_IOVIEWER_WIDTH  593
#define WIN_GBA_IOVIEWER_HEIGHT 262

static int GBAIOViewerCreated = 0;

static int gba_ioview_selected_tab = 0;

//-----------------------------------------------------------------------------------

static _gui_element gba_ioview_display_tabbtn, gba_ioview_backgrounds_tabbtn, gba_ioview_dma_tabbtn,
                    gba_ioview_timers_tabbtn, gba_ioview_sound_tabbtn, gba_ioview_other_tabbtn;

static _gui_console gba_ioview_screen_con;
static _gui_element gba_ioview_screen_textbox, gba_ioview_screen_label;

static _gui_element * gba_ioviwer_display_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    &gba_ioview_screen_textbox, &gba_ioview_screen_label,
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

    GUI_ConsoleClear(&gba_ioview_screen_con);

    //Screen
    GUI_ConsoleModePrintf(&gba_ioview_screen_con,0,0,"40h LCDC - %02X",40);

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

    GUI_SetLabel(&gba_ioview_screen_label,
                   6,36, 13*FONT_WIDTH+2,FONT_HEIGHT, "Screen");
    GUI_SetTextBox(&gba_ioview_screen_textbox,&gba_ioview_screen_con,
                   6,50, 13*FONT_WIDTH+2,9*FONT_HEIGHT, NULL);

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
