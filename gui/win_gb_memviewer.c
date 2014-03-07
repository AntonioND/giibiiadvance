
#include <SDL2/SDL.h>

#include <string.h>

#include "win_gb_memviewer.h"
#include "win_main.h"
#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../general_utils.h"
#include "win_utils.h"
#include "../gb_core/memory.h"

//-----------------------------------------------------------------------------------

static int WinIDGBMemViewer;

#define WIN_GB_MEMVIEWER_WIDTH  495
#define WIN_GB_MEMVIEWER_HEIGHT 282

static int GBMemViewerCreated = 0;

//-----------------------------------------------------------------------------------

#define GB_MEMVIEWER_MAX_LINES (20)
#define GB_MEMVIEWER_ADDRESS_JUMP_LINE (16)

#define GB_MEMVIEWER_8  0
#define GB_MEMVIEWER_16 1

static int gb_memviewer_mode = GB_MEMVIEWER_8;

static u16 gb_memviewer_start_address;

static u16 gb_memviewer_clicked_address;

//-----------------------------------------------------------------------------------

static _gui_console gb_memview_con;
static _gui_element gb_memview_textbox;

static _gui_element gb_disassembler_goto_btn;

static _gui_element gb_memview_mode_8_radbtn, gb_memview_mode_16_radbtn;

static _gui_element * gb_memviwer_window_gui_elements[] = {
    &gb_memview_textbox,
    &gb_disassembler_goto_btn,
    &gb_memview_mode_8_radbtn,
    &gb_memview_mode_16_radbtn,
    NULL
};

_gui_inputwindow gui_iw_gb_memviewer;

static _gui gb_memviewer_window_gui = {
    gb_memviwer_window_gui_elements,
    &gui_iw_gb_memviewer,
    NULL
};

static char win_gb_memviewer_character_fix(char c)
{
    if( (c >= '-') && (c <= '_') ) return c;
    if( (c >= 'a') && (c <= 127) ) return c;
    return '.';
}

void Win_GBMemViewerUpdate(void)
{
    if(GBMemViewerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    GUI_ConsoleClear(&gb_memview_con);

    u16 address = gb_memviewer_start_address;

    char textbuf[300];

    if(gb_memviewer_mode == GB_MEMVIEWER_16)
    {
        int i;
        for(i = 0; i < GB_MEMVIEWER_MAX_LINES; i++)
        {
            s_snprintf(textbuf,sizeof(textbuf),"%04X : ",address);

            u16 tmpaddr = address;
            int j;
            for(j = 0; j < 8; j ++)
            {
                char tmp[30];
                s_snprintf(tmp,sizeof(tmp),"%04X ",GB_MemRead16(tmpaddr));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr += 2;
            }
            s_strncat(textbuf,": ",sizeof(textbuf));

            tmpaddr = address;
            for(j = 0; j < 16; j ++)
            {
                char tmp[30];
                s_snprintf(tmp,sizeof(tmp),"%c",win_gb_memviewer_character_fix(GB_MemRead8(tmpaddr)));
                if((j&3) == 3) s_strncat(tmp," ",sizeof(tmp));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr ++;
            }

            GUI_ConsoleModePrintf(&gb_memview_con,0,i,textbuf);

            address += GB_MEMVIEWER_ADDRESS_JUMP_LINE;
        }
    }
    else if(gb_memviewer_mode == GB_MEMVIEWER_8)
    {
        int i;
        for(i = 0; i < GB_MEMVIEWER_MAX_LINES; i++)
        {
            s_snprintf(textbuf,sizeof(textbuf),"%04X : ",address);

            u16 tmpaddr = address;
            int j;
            for(j = 0; j < 16; j ++)
            {
                char tmp[30];
                s_snprintf(tmp,sizeof(tmp),"%02X ",GB_MemRead8(tmpaddr));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr ++;
            }
            s_strncat(textbuf,": ",sizeof(textbuf));

            tmpaddr = address;
            for(j = 0; j < 16; j ++)
            {
                char tmp[30];
                s_snprintf(tmp,sizeof(tmp),"%c",win_gb_memviewer_character_fix(GB_MemRead8(tmpaddr)));
                if((j&3) == 3) s_strncat(tmp," ",sizeof(tmp));
                s_strncat(textbuf,tmp,sizeof(textbuf));
                tmpaddr ++;
            }

            GUI_ConsoleModePrintf(&gb_memview_con,0,i,textbuf);

            address += GB_MEMVIEWER_ADDRESS_JUMP_LINE;
        }
    }
}

//----------------------------------------------------------------

void Win_GBMemViewerGoto(void);

//----------------------------------------------------------------

void Win_GBMemViewerRender(void)
{
    if(GBMemViewerCreated == 0) return;

    char buffer[WIN_GB_MEMVIEWER_WIDTH*WIN_GB_MEMVIEWER_HEIGHT*3];
    GUI_Draw(&gb_memviewer_window_gui,buffer,WIN_GB_MEMVIEWER_WIDTH,WIN_GB_MEMVIEWER_HEIGHT,1);

    WH_Render(WinIDGBMemViewer, buffer);
}

int Win_GBMemViewerCallback(SDL_Event * e)
{
    if(GBMemViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gb_memviewer_window_gui,e);

    int close_this = 0;

    if(GUI_InputWindowIsEnabled(&gui_iw_gb_memviewer) == 0)
    {
        if(e->type == SDL_MOUSEWHEEL)
        {
            gb_memviewer_start_address -= e->wheel.y*3 * GB_MEMVIEWER_ADDRESS_JUMP_LINE;
            redraw = 1;
        }
        else if( e->type == SDL_KEYDOWN)
        {
            switch( e->key.keysym.sym )
            {
                case SDLK_F8:
                    Win_GBMemViewerGoto();
                    redraw = 1;
                    break;

                case SDLK_DOWN:
                    gb_memviewer_start_address += GB_MEMVIEWER_ADDRESS_JUMP_LINE;
                    redraw = 1;
                    break;

                case SDLK_UP:
                    gb_memviewer_start_address -= GB_MEMVIEWER_ADDRESS_JUMP_LINE;
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
            if(GUI_InputWindowIsEnabled(&gui_iw_gb_memviewer))
                GUI_InputWindowClose(&gui_iw_gb_memviewer);
            else
                close_this = 1;
        }
    }

    if(close_this)
    {
        GBMemViewerCreated = 0;
        if(GUI_InputWindowIsEnabled(&gui_iw_gb_memviewer))
            GUI_InputWindowClose(&gui_iw_gb_memviewer);
        WH_Close(WinIDGBMemViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBMemViewerUpdate();
        Win_GBMemViewerRender();
        return 1;
    }

    return 0;
}

static int gb_memviewer_inputwindow_is_goto = 0;

void Win_GBMemViewerInputWindowCallback(char * text, int is_valid)
{
    if(is_valid)
    {
        if(gb_memviewer_inputwindow_is_goto)
        {
            text[4] = '\0';
            u32 newvalue = asciihex_to_int(text);
            newvalue &= ~(GB_MEMVIEWER_ADDRESS_JUMP_LINE-1);
            gb_memviewer_start_address = newvalue;
        }
        else
        {
            if(gb_memviewer_mode == GB_MEMVIEWER_16)
            {
                text[4] = '\0';
                u32 newvalue = asciihex_to_int(text);
                GB_MemWrite16(gb_memviewer_clicked_address,newvalue);
            }
            else if(gb_memviewer_mode == GB_MEMVIEWER_8)
            {
                text[2] = '\0';
                u32 newvalue = asciihex_to_int(text);
                GB_MemWrite8(gb_memviewer_clicked_address,newvalue);
            }
        }
    }
}

void Win_GBMemViewTextBoxCallback(int x, int y)
{
    int xtile = x/FONT_12_WIDTH;
    int ytile = y/FONT_12_HEIGHT;

    int has_clicked_addr = 0;
    u32 clicked_addr;
    int numbits;

    u32 line_base_addr = gb_memviewer_start_address+(ytile*GB_MEMVIEWER_ADDRESS_JUMP_LINE);

    if(gb_memviewer_mode == GB_MEMVIEWER_16)
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
    else if(gb_memviewer_mode == GB_MEMVIEWER_8)
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
        gb_memviewer_clicked_address = clicked_addr;
        gb_memviewer_inputwindow_is_goto = 0;
        char caption[100];
        s_snprintf(caption,sizeof(caption),"Change [0x%08X] (%d bits)",clicked_addr,numbits);
        GUI_InputWindowOpen(&gui_iw_gb_memviewer,caption,Win_GBMemViewerInputWindowCallback);
    }
}

void Win_GBMemViewerModeRadioButtonsCallback(int btn_id)
{
    gb_memviewer_mode = btn_id;
    Win_GBMemViewerUpdate();
}

void Win_GBMemViewerGoto(void)
{
    if(GBMemViewerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    gb_memviewer_inputwindow_is_goto = 1;
    GUI_InputWindowOpen(&gui_iw_gb_memviewer,"Go to address",Win_GBMemViewerInputWindowCallback);
}

int Win_GBMemViewerCreate(void)
{
    if(GBMemViewerCreated == 1)
        return 0;

    if(Win_MainRunningGB() == 0) return 0;

    GUI_SetRadioButton(&gb_memview_mode_8_radbtn,   6,6,9*FONT_12_WIDTH,24,
                  "8 bits",  0,GB_MEMVIEWER_8,  1,Win_GBMemViewerModeRadioButtonsCallback);
    GUI_SetRadioButton(&gb_memview_mode_16_radbtn,  6+9*FONT_12_WIDTH+12,6,9*FONT_12_WIDTH,24,
                  "16 bits", 0,GB_MEMVIEWER_16, 0,Win_GBMemViewerModeRadioButtonsCallback);

    GUI_SetButton(&gb_disassembler_goto_btn,66+39*FONT_12_WIDTH+36,6,16*FONT_12_WIDTH,24,
                  "Goto (F8)",Win_GBMemViewerGoto);

    GUI_SetTextBox(&gb_memview_textbox,&gb_memview_con,
                   6,36, 69*FONT_12_WIDTH,GB_MEMVIEWER_MAX_LINES*FONT_12_HEIGHT,
                   Win_GBMemViewTextBoxCallback);

    GUI_InputWindowClose(&gui_iw_gb_memviewer);

    GBMemViewerCreated = 1;

    gb_memviewer_mode = GB_MEMVIEWER_8;

    gb_memviewer_start_address = 0;

    WinIDGBMemViewer = WH_Create(WIN_GB_MEMVIEWER_WIDTH,WIN_GB_MEMVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBMemViewer,"GiiBiiAdvance - GB Memory Viewer");

    WH_SetEventCallback(WinIDGBMemViewer,Win_GBMemViewerCallback);

    Win_GBMemViewerUpdate();
    Win_GBMemViewerRender();

    return 1;
}
