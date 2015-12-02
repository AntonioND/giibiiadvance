/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL.h>

#include <string.h>

#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../general_utils.h"

#include "win_gba_debugger.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gba_core/memory.h"

//-----------------------------------------------------------------------------------

static int WinIDGBAMemViewer;

#define WIN_GBA_MEMVIEWER_WIDTH  495
#define WIN_GBA_MEMVIEWER_HEIGHT 282

static int GBAMemViewerCreated = 0;

//-----------------------------------------------------------------------------------

#define GBA_MEMVIEWER_MAX_LINES (20)
#define GBA_MEMVIEWER_ADDRESS_JUMP_LINE (16)

#define GBA_MEMVIEWER_8  0
#define GBA_MEMVIEWER_16 1
#define GBA_MEMVIEWER_32 2

static int gba_memviewer_mode = GBA_MEMVIEWER_32;

static u32 gba_memviewer_start_address;

static u32 gba_memviewer_clicked_address;

//-----------------------------------------------------------------------------------

static _gui_console gba_memview_con;
static _gui_element gba_memview_textbox;

static _gui_element gba_memview_goto_btn;

static _gui_element gba_memview_mode_8_radbtn, gba_memview_mode_16_radbtn, gba_memview_mode_32_radbtn;

static _gui_element * gba_memviwer_window_gui_elements[] = {
    &gba_memview_textbox,
    &gba_memview_goto_btn,
    &gba_memview_mode_8_radbtn,
    &gba_memview_mode_16_radbtn,
    &gba_memview_mode_32_radbtn,
    NULL
};

_gui_inputwindow gui_iw_gba_memviewer;

static _gui gba_memviewer_window_gui = {
    gba_memviwer_window_gui_elements,
    &gui_iw_gba_memviewer,
    NULL
};

static char win_gba_memviewer_character_fix(char c)
{
    if( (c >= '-') && (c <= '_') ) return c;
    if( (c >= 'a') && (c <= 127) ) return c;
    return '.';
}

void Win_GBAMemViewerUpdate(void)
{
    if(GBAMemViewerCreated == 0) return;

    if(Win_MainRunningGBA() == 0) return;

    GUI_ConsoleClear(&gba_memview_con);

    u32 address = gba_memviewer_start_address;

    char textbuf[300];

    if(gba_memviewer_mode == GBA_MEMVIEWER_32)
    {
        int i;
        for(i = 0; i < GBA_MEMVIEWER_MAX_LINES; i++)
        {
            snprintf(textbuf,sizeof(textbuf),"%08X : ",address);

            u32 tmpaddr = address;
            int j;
            for(j = 0; j < 4; j ++)
            {
                char tmp[30];
                snprintf(tmp,sizeof(tmp),"%08X ",GBA_MemoryReadFast32(tmpaddr));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr += 4;
            }
            s_strncat(textbuf,": ",sizeof(textbuf));

            tmpaddr = address;
            for(j = 0; j < 16; j ++)
            {
                char tmp[30];
                snprintf(tmp,sizeof(tmp),"%c",win_gba_memviewer_character_fix(GBA_MemoryReadFast8(tmpaddr)));
                if((j&3) == 3) s_strncat(tmp," ",sizeof(tmp));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr ++;
            }

            GUI_ConsoleModePrintf(&gba_memview_con,0,i,textbuf);

            address += GBA_MEMVIEWER_ADDRESS_JUMP_LINE;
        }
    }
    else if(gba_memviewer_mode == GBA_MEMVIEWER_16)
    {
        int i;
        for(i = 0; i < GBA_MEMVIEWER_MAX_LINES; i++)
        {
            snprintf(textbuf,sizeof(textbuf),"%08X : ",address);

            u32 tmpaddr = address;
            int j;
            for(j = 0; j < 8; j ++)
            {
                char tmp[30];
                snprintf(tmp,sizeof(tmp),"%04X ",GBA_MemoryReadFast16(tmpaddr));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr += 2;
            }
            s_strncat(textbuf,": ",sizeof(textbuf));

            tmpaddr = address;
            for(j = 0; j < 16; j ++)
            {
                char tmp[30];
                snprintf(tmp,sizeof(tmp),"%c",win_gba_memviewer_character_fix(GBA_MemoryReadFast8(tmpaddr)));
                if((j&3) == 3) s_strncat(tmp," ",sizeof(tmp));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr ++;
            }

            GUI_ConsoleModePrintf(&gba_memview_con,0,i,textbuf);

            address += GBA_MEMVIEWER_ADDRESS_JUMP_LINE;
        }
    }
    else if(gba_memviewer_mode == GBA_MEMVIEWER_8)
    {
        int i;
        for(i = 0; i < GBA_MEMVIEWER_MAX_LINES; i++)
        {
            snprintf(textbuf,sizeof(textbuf),"%08X : ",address);

            u32 tmpaddr = address;
            int j;
            for(j = 0; j < 16; j ++)
            {
                char tmp[30];
                snprintf(tmp,sizeof(tmp),"%02X ",GBA_MemoryReadFast8(tmpaddr));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr ++;
            }
            s_strncat(textbuf,": ",sizeof(textbuf));

            tmpaddr = address;
            for(j = 0; j < 16; j ++)
            {
                char tmp[30];
                snprintf(tmp,sizeof(tmp),"%c",win_gba_memviewer_character_fix(GBA_MemoryReadFast8(tmpaddr)));
                if((j&3) == 3) s_strncat(tmp," ",sizeof(tmp));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr ++;
            }

            GUI_ConsoleModePrintf(&gba_memview_con,0,i,textbuf);

            address += GBA_MEMVIEWER_ADDRESS_JUMP_LINE;
        }
    }
}

//----------------------------------------------------------------

static void _win_gba_mem_viewer_goto(void);

//----------------------------------------------------------------

void Win_GBAMemViewerRender(void)
{
    if(GBAMemViewerCreated == 0) return;

    char buffer[WIN_GBA_MEMVIEWER_WIDTH*WIN_GBA_MEMVIEWER_HEIGHT*3];
    GUI_Draw(&gba_memviewer_window_gui,buffer,WIN_GBA_MEMVIEWER_WIDTH,WIN_GBA_MEMVIEWER_HEIGHT,1);

    WH_Render(WinIDGBAMemViewer, buffer);
}

static int _win_gba_mem_viewer_callback(SDL_Event * e)
{
    if(GBAMemViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gba_memviewer_window_gui,e);

    int close_this = 0;

    if(GUI_InputWindowIsEnabled(&gui_iw_gba_memviewer) == 0)
    {
        if(e->type == SDL_MOUSEWHEEL)
        {
            gba_memviewer_start_address -= e->wheel.y*3 * GBA_MEMVIEWER_ADDRESS_JUMP_LINE;
            redraw = 1;
        }
        else if( e->type == SDL_KEYDOWN)
        {
            switch( e->key.keysym.sym )
            {
                case SDLK_F8:
                    _win_gba_mem_viewer_goto();
                    redraw = 1;
                    break;

                case SDLK_DOWN:
                    gba_memviewer_start_address += GBA_MEMVIEWER_ADDRESS_JUMP_LINE;
                    redraw = 1;
                    break;

                case SDLK_UP:
                    gba_memviewer_start_address -= GBA_MEMVIEWER_ADDRESS_JUMP_LINE;
                    redraw = 1;
                    break;
            }
        }
    }

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
            if(GUI_InputWindowIsEnabled(&gui_iw_gba_memviewer))
                GUI_InputWindowClose(&gui_iw_gba_memviewer);
            else
                close_this = 1;
        }
    }

    if(close_this)
    {
        GBAMemViewerCreated = 0;
        if(GUI_InputWindowIsEnabled(&gui_iw_gba_memviewer))
            GUI_InputWindowClose(&gui_iw_gba_memviewer);
        WH_Close(WinIDGBAMemViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBAMemViewerUpdate();
        Win_GBAMemViewerRender();
        return 1;
    }

    return 0;
}

static int gba_memviewer_inputwindow_is_goto = 0;

static void _win_gba_mem_viewer_inputwindow_callback(char * text, int is_valid)
{
    if(is_valid)
    {
        if(gba_memviewer_inputwindow_is_goto)
        {
            text[8] = '\0';
            u32 newvalue = asciihex_to_int(text);
            newvalue &= ~(GBA_MEMVIEWER_ADDRESS_JUMP_LINE-1);
            gba_memviewer_start_address = newvalue;
        }
        else
        {
            if(gba_memviewer_mode == GBA_MEMVIEWER_32)
            {
                text[8] = '\0';
                u32 newvalue = asciihex_to_int(text);
                GBA_MemoryWrite32(gba_memviewer_clicked_address,newvalue);
            }
            else if(gba_memviewer_mode == GBA_MEMVIEWER_16)
            {
                text[4] = '\0';
                u32 newvalue = asciihex_to_int(text);
                GBA_MemoryWrite16(gba_memviewer_clicked_address,newvalue);
            }
            else if(gba_memviewer_mode == GBA_MEMVIEWER_8)
            {
                text[2] = '\0';
                u32 newvalue = asciihex_to_int(text);
                GBA_MemoryWrite8(gba_memviewer_clicked_address,newvalue);
            }
        }
    }
}

static void _win_gba_mem_view_textbox_callback(int x, int y)
{
    int xtile = x/FONT_WIDTH;
    int ytile = y/FONT_HEIGHT;

    int has_clicked_addr = 0;
    u32 clicked_addr;
    int numbits;

    u32 line_base_addr = gba_memviewer_start_address+(ytile*GBA_MEMVIEWER_ADDRESS_JUMP_LINE);

    if(gba_memviewer_mode == GBA_MEMVIEWER_32)
    {
        numbits = 32;
        xtile -= 11;
        int j;
        for(j = 0; j < 4; j++)
        {
            if( (xtile >= 0) && (xtile <= 7) )
            {
                has_clicked_addr = 1;
                clicked_addr = line_base_addr;
                break;
            }
            line_base_addr += 4;
            xtile -= 9;
        }
    }
    else if(gba_memviewer_mode == GBA_MEMVIEWER_16)
    {
        numbits = 16;
        xtile -= 11;
        int j;
        for(j = 0; j < 8; j++)
        {
            if( (xtile >= 0) && (xtile <= 3) )
            {
                has_clicked_addr = 1;
                clicked_addr = line_base_addr;
                break;
            }
            line_base_addr += 2;
            xtile -= 5;
        }
    }
    else if(gba_memviewer_mode == GBA_MEMVIEWER_8)
    {
        numbits = 8;
        xtile -= 11;
        int j;
        for(j = 0; j < 16; j++)
        {
            if( (xtile >= 0) && (xtile <= 1) )
            {
                has_clicked_addr = 1;
                clicked_addr = line_base_addr;
                break;
            }
            line_base_addr += 1;
            xtile -= 3;
        }
    }

    if(has_clicked_addr)
    {
        gba_memviewer_clicked_address = clicked_addr;
        gba_memviewer_inputwindow_is_goto = 0;
        char caption[100];
        snprintf(caption,sizeof(caption),"Change [0x%08X] (%d bits)",clicked_addr,numbits);
        GUI_InputWindowOpen(&gui_iw_gba_memviewer,caption,_win_gba_mem_viewer_inputwindow_callback);
    }
}

static void _win_gba_mem_viewer_mode_radiobtn_callback(int btn_id)
{
    gba_memviewer_mode = btn_id;
    Win_GBAMemViewerUpdate();
}

static void _win_gba_mem_viewer_goto(void)
{
    if(GBAMemViewerCreated == 0) return;

    if(Win_MainRunningGBA() == 0) return;

    gba_memviewer_inputwindow_is_goto = 1;
    GUI_InputWindowOpen(&gui_iw_gba_memviewer,"Go to address",_win_gba_mem_viewer_inputwindow_callback);
}

//-----------------------------------------------------------------------------------

int Win_GBAMemViewerCreate(void)
{
    if(Win_MainRunningGBA() == 0) return 0;

    if(GBAMemViewerCreated == 1)
    {
        WH_Focus(WinIDGBAMemViewer);
        return 0;
    }

    GUI_SetRadioButton(&gba_memview_mode_8_radbtn,   6,6,9*FONT_WIDTH,24,
                  "8 bits",  0,GBA_MEMVIEWER_8,  0,_win_gba_mem_viewer_mode_radiobtn_callback);
    GUI_SetRadioButton(&gba_memview_mode_16_radbtn,  6+9*FONT_WIDTH+12,6,9*FONT_WIDTH,24,
                  "16 bits", 0,GBA_MEMVIEWER_16, 0,_win_gba_mem_viewer_mode_radiobtn_callback);
    GUI_SetRadioButton(&gba_memview_mode_32_radbtn,  18+18*FONT_WIDTH+12,6,9*FONT_WIDTH,24,
                  "32 bits", 0,GBA_MEMVIEWER_32, 1,_win_gba_mem_viewer_mode_radiobtn_callback);

    GUI_SetButton(&gba_memview_goto_btn,68+39*FONT_WIDTH+36,6,16*FONT_WIDTH,24,
                  "Goto (F8)",_win_gba_mem_viewer_goto);

    GUI_SetTextBox(&gba_memview_textbox,&gba_memview_con,
                   6,36, 69*FONT_WIDTH,GBA_MEMVIEWER_MAX_LINES*FONT_HEIGHT,
                   _win_gba_mem_view_textbox_callback);

    GUI_InputWindowClose(&gui_iw_gba_memviewer);

    GBAMemViewerCreated = 1;

    gba_memviewer_start_address = 0;

    gba_memviewer_mode = GBA_MEMVIEWER_32;

    WinIDGBAMemViewer = WH_Create(WIN_GBA_MEMVIEWER_WIDTH,WIN_GBA_MEMVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBAMemViewer,"GBA Memory Viewer");

    WH_SetEventCallback(WinIDGBAMemViewer,_win_gba_mem_viewer_callback);

    Win_GBAMemViewerUpdate();
    Win_GBAMemViewerRender();

    return 1;
}

void Win_GBAMemViewerClose(void)
{
    if(GBAMemViewerCreated == 0)
        return;

    GBAMemViewerCreated = 0;
    WH_Close(WinIDGBAMemViewer);
}

//-----------------------------------------------------------------------------------
