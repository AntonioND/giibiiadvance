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

#include "win_gb_sgbviewer.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/debug_video.h"
#include "../gb_core/sgb.h"

#include "../png/png_utils.h"

//------------------------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------------------------

static int WinIDGB_SGBViewer;

#define WIN_GB_SGBVIEWER_WIDTH  700
#define WIN_GB_SGBVIEWER_HEIGHT 500

static int GB_SGBViewerCreated = 0;

//-----------------------------------------------------------------------------------

static char gb_sgb_border_buffer[256*256*3];
static int gb_sgb_border_tilex;
static int gb_sgb_border_tiley;

static char gb_sgb_border_zoomed_tile_buffer[64*64*3];

//-----------------------------------------------------------------------------------

static _gui_console gb_sgbview_border_con;
static _gui_element gb_sgbview_border_textbox;
static _gui_element gb_sgbview_border_label;
static _gui_element gb_sgbview_border_bmp, gb_sgbview_border_zoomed_tile_bmp;
static _gui_element gb_sgbview_border_dump_btn;

static _gui_element * gb_sgbviwer_window_gui_elements[] = {
    &gb_sgbview_border_label,
    &gb_sgbview_border_textbox,
    &gb_sgbview_border_bmp,
    &gb_sgbview_border_zoomed_tile_bmp,
    &gb_sgbview_border_dump_btn,
    NULL
};

static _gui gb_sgbviewer_window_gui = {
    gb_sgbviwer_window_gui_elements,
    NULL,
    NULL
};

//----------------------------------------------------------------

static inline void rgb16to32(u16 color, int * r, int * g, int * b)
{
    *r = (color & 31)<<3;
    *g = ((color >> 5) & 31)<<3;
    *b = ((color >> 10) & 31)<<3;
}

//----------------------------------------------------------------

static void _win_gb_sgbviewer_draw_border(void)
{
	u32 i,j;
	for(i = 0; i < 32; i++) for(j = 0; j < 32; j++)
	{
        u32 info = SGBInfo.tile_map[(j*32) + i];

        u32 tile = info & 0xFF;
        u32 * tile_ptr = &SGBInfo.tile_data[((8*8*4)/8) * tile];

        u32 pal =  (info >> 10) & 7; // 4 to 7 (officially 4 to 6)
        if(pal < 4) pal += 4;

        //u32 prio = info & (1<<13); //not used

        u32 xflip = info & (1<<14);
        u32 yflip = info & (1<<15);

        u32 x,y;
        for(y = 0; y < 8; y++) for(x = 0; x < 8; x++)
        {

            u32 * data = tile_ptr;
            u32 * data2 = tile_ptr + 16;

            if(yflip)
            {
                data += (7-y)<<1;
                data2 += (7-y)<<1;
            }
            else
            {
                data += y<<1;
                data2 += y<<1;
            }

            u32 x_;
            if(xflip) x_ = x;
            else x_ = 7-x;

            u32 color = (*data >> x_) & 1;
            color |= ( ( ( (*(data+1)) >> x_) << 1) & (1<<1));
            color |= ( ( ( (*data2) >> x_) << 2) & (1<<2));
            color |= ( ( ( (*(data2+1)) >> x_) << 3) & (1<<3));

            int temp = ((y+(j<<3))*256) + (x+(i<<3));
            if(color)
            {
                color = SGBInfo.palette[pal][color];
                int r,g,b;
                rgb16to32(color,&r,&g,&b);
                gb_sgb_border_buffer[temp*3+0] = r;
                gb_sgb_border_buffer[temp*3+1] = g;
                gb_sgb_border_buffer[temp*3+2] = b;
            }
            else
            {
                if( (i>=6) && (i<26) && (j>=4) && (j<23) ) //inside
                {
                    int outcolor = (((x+(i<<3))&32)^((y+(j<<3))&32)) ? 0x80 : 0xB0;
                    gb_sgb_border_buffer[temp*3+0] = outcolor;
                    gb_sgb_border_buffer[temp*3+1] = outcolor;
                    gb_sgb_border_buffer[temp*3+2] = outcolor;
                }
                else
                {
                    color = SGBInfo.palette[pal][color];
                    int r,g,b;
                    rgb16to32(color,&r,&g,&b);
                    gb_sgb_border_buffer[temp*3+0] = r;
                    gb_sgb_border_buffer[temp*3+1] = g;
                    gb_sgb_border_buffer[temp*3+2] = b;
                }
            }
        }
	}
}

static void _win_gb_sgbviewer_draw_border_zoomed_tile(void)
{
    GUI_ConsoleClear(&gb_sgbview_border_con);

    u32 info = SGBInfo.tile_map[gb_sgb_border_tiley*32+gb_sgb_border_tilex];

    u32 tile = info & 0xFF;

    u32 * tile_ptr = &SGBInfo.tile_data[((8*8*4)/8) * tile];

    u32 pal =  (info >> 10) & 7; // 4 to 7 (officially 4 to 6)
    if(pal < 4) pal += 4;

    //u32 prio = info & (1<<13); //not used

    u32 xflip = info & (1<<14);
    u32 yflip = info & (1<<15);

    u32 x,y;
    for(y = 0; y < 8; y++) for(x = 0; x < 8; x++)
    {
        u32 * data = tile_ptr;
        u32 * data2 = tile_ptr + 16;

        data += y<<1;
        data2 += y<<1;

        u32 x_ = 7-x;

        u32 color = (*data >> x_) & 1;
        color |= ( ( ( (*(data+1)) >> x_) << 1) & (1<<1));
        color |= ( ( ( (*data2) >> x_) << 2) & (1<<2));
        color |= ( ( ( (*(data2+1)) >> x_) << 3) & (1<<3));
        color = SGBInfo.palette[pal][color];

        int i,j;
        for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
        {
            int temp = (((y*8)+j)*64) + (x*8)+i;
            int r,g,b;
            rgb16to32(color,&r,&g,&b);
            gb_sgb_border_zoomed_tile_buffer[temp*3+0] = r;
            gb_sgb_border_zoomed_tile_buffer[temp*3+1] = g;
            gb_sgb_border_zoomed_tile_buffer[temp*3+2] = b;
        }
    }

    //--------------------

    GUI_ConsoleModePrintf(&gb_sgbview_border_con,0,0,"Pos: %d,%d\nTile: %d\nFlip: %s%s\nPal: %d",
                          gb_sgb_border_tilex, gb_sgb_border_tiley, tile,
                          xflip ? "H" : "-",  yflip ? "V" : "-", pal);

    //Mark in border buffer the zoomed tile
    GUI_Draw_SetDrawingColor(255,0,0);
    int l = (gb_sgb_border_tilex*8); //left
    int t = (gb_sgb_border_tiley*8); // top
    int r = l + 7; // right
    int b = t + 7; // bottom
    GUI_Draw_Rect(gb_sgb_border_buffer,256,256,l,r,t,b);
}

//----------------------------------------------------------------

static int _win_gb_sgbviewer_border_bmp_callback(int x, int y)
{
    gb_sgb_border_tilex = x/8;
    gb_sgb_border_tiley = y/8;
    return 1;
}

//----------------------------------------------------------------

void Win_GB_SGBViewerUpdate(void)
{
    if(GB_SGBViewerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    if(GameBoy.Emulator.SGBEnabled == 0) return;

    _win_gb_sgbviewer_draw_border();
    _win_gb_sgbviewer_draw_border_zoomed_tile();


}

//----------------------------------------------------------------

static void _win_gb_sgb_viewer_render(void)
{
    if(GB_SGBViewerCreated == 0) return;

    char buffer[WIN_GB_SGBVIEWER_WIDTH*WIN_GB_SGBVIEWER_HEIGHT*3];
    GUI_Draw(&gb_sgbviewer_window_gui,buffer,WIN_GB_SGBVIEWER_WIDTH,WIN_GB_SGBVIEWER_HEIGHT,1);

    WH_Render(WinIDGB_SGBViewer, buffer);
}

static int _win_gb_sgb_viewer_callback(SDL_Event * e)
{
    if(GB_SGBViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gb_sgbviewer_window_gui,e);

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
        GB_SGBViewerCreated = 0;
        WH_Close(WinIDGB_SGBViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GB_SGBViewerUpdate();
        _win_gb_sgb_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gb_sgbviewer_border_dump_btn_callback(void)
{
    if(Win_MainRunningGB() == 0) return;

    if(GameBoy.Emulator.SGBEnabled == 0) return;

    char * border_buff = malloc(256*256*4);
    if(border_buff == NULL)
        return;

    u32 i,j;
	for(i = 0; i < 32; i++) for(j = 0; j < 32; j++)
	{
        u32 info = SGBInfo.tile_map[(j*32) + i];

        u32 tile = info & 0xFF;
        u32 * tile_ptr = &SGBInfo.tile_data[((8*8*4)/8) * tile];

        u32 pal =  (info >> 10) & 7; // 4 to 7 (officially 4 to 6)
        if(pal < 4) pal += 4;

        //u32 prio = info & (1<<13); //not used

        u32 xflip = info & (1<<14);
        u32 yflip = info & (1<<15);

        u32 x,y;
        for(y = 0; y < 8; y++) for(x = 0; x < 8; x++)
        {

            u32 * data = tile_ptr;
            u32 * data2 = tile_ptr + 16;

            if(yflip)
            {
                data += (7-y)<<1;
                data2 += (7-y)<<1;
            }
            else
            {
                data += y<<1;
                data2 += y<<1;
            }

            u32 x_;
            if(xflip) x_ = x;
            else x_ = 7-x;

            u32 color = (*data >> x_) & 1;
            color |= ( ( ( (*(data+1)) >> x_) << 1) & (1<<1));
            color |= ( ( ( (*data2) >> x_) << 2) & (1<<2));
            color |= ( ( ( (*(data2+1)) >> x_) << 3) & (1<<3));

            int temp = ((y+(j<<3))*256) + (x+(i<<3));
            if(color)
            {
                color = SGBInfo.palette[pal][color];
                int r,g,b;
                rgb16to32(color,&r,&g,&b);
                border_buff[temp*4+0] = r;
                border_buff[temp*4+1] = g;
                border_buff[temp*4+2] = b;
                border_buff[temp*4+3] = 255;
            }
            else
            {
                if( (i>=6) && (i<26) && (j>=4) && (j<23) ) //inside
                {
                    border_buff[temp*4+0] = 0;
                    border_buff[temp*4+1] = 0;
                    border_buff[temp*4+2] = 0;
                    border_buff[temp*4+3] = 0;
                }
                else
                {
                    color = SGBInfo.palette[pal][color];
                    int r,g,b;
                    rgb16to32(color,&r,&g,&b);
                    border_buff[temp*4+0] = r;
                    border_buff[temp*4+1] = g;
                    border_buff[temp*4+2] = b;
                    border_buff[temp*4+3] = 255;
                }
            }
        }
	}

	char * name = FU_GetNewTimestampFilename("gb_sgb_border");
    Save_PNG(name,256,256,border_buff,1);

	free(border_buff);

    Win_GB_SGBViewerUpdate();
}

//----------------------------------------------------------------

int Win_GB_SGBViewerCreate(void)
{
    if(GB_SGBViewerCreated == 1)
        return 0;

    if(Win_MainRunningGB() == 0) return 0;

    if(GameBoy.Emulator.SGBEnabled == 0) return 0;

    //Border

    GUI_SetLabel(&gb_sgbview_border_label,6,6,6*FONT_WIDTH,FONT_HEIGHT,"Border");

    GUI_SetTextBox(&gb_sgbview_border_textbox,&gb_sgbview_border_con,
                   76,286, 11*FONT_WIDTH,4*FONT_HEIGHT, NULL);

    GUI_SetBitmap(&gb_sgbview_border_bmp, 6,24, 256,256 ,gb_sgb_border_buffer, _win_gb_sgbviewer_border_bmp_callback);

    GUI_SetBitmap(&gb_sgbview_border_zoomed_tile_bmp, 6,286, 64,64,gb_sgb_border_zoomed_tile_buffer, NULL);

    GUI_SetButton(&gb_sgbview_border_dump_btn,159,286,FONT_WIDTH*6,FONT_HEIGHT*2,"Dump",
                  _win_gb_sgbviewer_border_dump_btn_callback);

    gb_sgb_border_tilex = 0;
    gb_sgb_border_tiley = 0;

    //...



    GB_SGBViewerCreated = 1;

    WinIDGB_SGBViewer = WH_Create(WIN_GB_SGBVIEWER_WIDTH,WIN_GB_SGBVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGB_SGBViewer,"SGB Viewer");

    WH_SetEventCallback(WinIDGB_SGBViewer,_win_gb_sgb_viewer_callback);

    Win_GB_SGBViewerUpdate();
    _win_gb_sgb_viewer_render();

    return 1;
}

void Win_GB_SGBViewerClose(void)
{
    if(GB_SGBViewerCreated == 0)
        return;

    GB_SGBViewerCreated = 0;
    WH_Close(WinIDGB_SGBViewer);
}

//----------------------------------------------------------------

#if 0

static HWND hComboTilesPal, hStaticTilesetInfo;
static u32 sgbtiles[128*128], sgbtiletileset[64*64];;
static int SelectedTileTileset;

static HWND hWndScrollATF, hEditATFInfo;
static u32 atfscreen[(160+1)*(144+1)];
static int SelectedATF, SelectedATFTile;

static HWND hEditPaletteInfo;
static int SelectedPal;

static HWND hEditPacketData;

static HWND hStaticCommandInfo;

static HWND hEditOtherInfo;

//-----------------

static inline u32 rgb16to32(u32 color)
{
    return ( ((color&0x1F)<<(3+16)) | (((color>>5)&0x1F)<<(3+8)) | (((color>>10)&0x1F)<<3) );
}

static inline COLORREF rgb16toCOLORREF(u16 color)
{
    return RGB((color & 31)<<3,((color >> 5) & 31)<<3,((color >> 10) & 31)<<3);
}

//-----------------

static void SGB_TilesDraw(void) // 4 ~ 7
{
    u32 pal = SendMessage(hComboTilesPal, CB_GETCURSEL, 0, 0)+4;

    u32 i,j;
	for(i = 0; i < 16; i++) for(j = 0; j < 16; j++)
	{
	    u32 tile = i + j*16;

        u32 * tile_ptr = &SGBInfo.tile_data[((8*8*4)/8) * tile];

        u32 x,y;
        for(y = 0; y < 8; y++) for(x = 0; x < 8; x++)
        {
            u32 * data = tile_ptr;
            u32 * data2 = tile_ptr + 16;

            data += y<<1;
            data2 += y<<1;

            u32 x_ = 7-x;

            u32 color = (*data >> x_) & 1;
            color |= ( ( ( (*(data+1)) >> x_) << 1) & (1<<1));
            color |= ( ( ( (*data2) >> x_) << 2) & (1<<2));
            color |= ( ( ( (*(data2+1)) >> x_) << 3) & (1<<3));
            color = SGBInfo.palette[pal][color];

            int temp = ((y+(j<<3))*128) + (x+(i<<3));
            sgbtiles[temp] = rgb16to32(color);
        }
	}
}

static void sgb_updatetilesetinfo(void)
{
    char text[20];
    sprintf(text,"Tile: %d",  SelectedTileTileset);
    SetWindowText(hStaticTilesetInfo,(LPCTSTR)text);

    //--------------------

    u32 pal = SendMessage(hComboTilesPal, CB_GETCURSEL, 0, 0)+4;
    u32 * tile_ptr = &SGBInfo.tile_data[((8*8*4)/8) * SelectedTileTileset];

    u32 x,y;
    for(y = 0; y < 8; y++) for(x = 0; x < 8; x++)
    {
        u32 * data = tile_ptr;
        u32 * data2 = tile_ptr + 16;

        data += y<<1;
        data2 += y<<1;

        u32 x_ = 7-x;

        u32 color = (*data >> x_) & 1;
        color |= ( ( ( (*(data+1)) >> x_) << 1) & (1<<1));
        color |= ( ( ( (*data2) >> x_) << 2) & (1<<2));
        color |= ( ( ( (*(data2+1)) >> x_) << 3) & (1<<3));
        color = SGBInfo.palette[pal][color];

        int i,j;
        for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
        {
            int temp = (((y*8)+j)*64) + (x*8)+i;
            sgbtiletileset[temp] = rgb16to32(color);
        }
    }
}

//-----------------

static inline u32 SGB_GetATFPal(u32 atf, u32 tilex, u32 tiley)
{
	return SGBInfo.ATF_list[atf][ (20*tiley) + tilex];
}

static void SGB_ATFDraw(int atf) // 0 ~ 0x2D-1
{
	u32 i,j;
	for(i = 0; i < 20; i++) for(j = 0; j < 18; j++)
	{
        u32 pal = SGB_GetATFPal(atf,i,j);

        u32 x,y;
        for(y = 0; y < 8; y++) for(x = 0; x < 8; x++)
        {
            int temp = ((y+(j<<3))*(160+1)) + (x+(i<<3));

            if( (x == 0) || (y == 0) )
            {
                atfscreen[temp] = 0;
            }
            else
            {
                u32 color;
                if(x < 4)
                {
                    if(y < 4) color = 0;
                    else color = 2;
                }
                else
                {
                    if(y < 4) color = 1;
                    else color = 3;
                }

                color = SGBInfo.palette[pal][color];
                atfscreen[temp] = rgb16to32(color);
            }
        }
	}

	for(i = 0; i < 161; i++) atfscreen[144*(160+1)+i] = 0;
	for(j = 0; j < 145; j++) atfscreen[j*(160+1)+161] = 0;
}

static void sgb_updateatfinfo(void)
{
    char text[30];
    sprintf(text,"ATF: %2d\r\nX: %2d\r\nY: %2d\r\nPal: %d\r\n\r\nNow: %2d",  SelectedATF,
        SelectedATFTile%20, SelectedATFTile/20,
        SGB_GetATFPal(SelectedATF,SelectedATFTile%20,SelectedATFTile/20), SGBInfo.curr_ATF);
    SetWindowText(hEditATFInfo,(LPCTSTR)text);
}

//-----------------

static void sgb_updatepalinfo(void)
{
    u32 color = SGBInfo.palette[SelectedPal/16][SelectedPal%16];
    char text[200];
    sprintf(text,"Pal: %d|Color: %2d\r\nRGB: %2d,%2d,%2d",  SelectedPal/16, SelectedPal%16,
        color&0x1F, (color>>5)&0x1F, (color>>10)&0x1F);
    SetWindowText(hEditPaletteInfo,(LPCTSTR)text);
}

//-----------------

static void sgb_updatepacketinfo(void)
{
    char endtext[1000];
    memset(endtext,0,sizeof(endtext));

    int numpackets = SGBInfo.data[0][0]&0x07;
    if(numpackets == 0) numpackets = 1;
    u32 i = 0;
	while(numpackets--)
	{
	    char text[100];
        sprintf(text,"%d : %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X",
            i+1,SGBInfo.data[i][0],SGBInfo.data[i][1],SGBInfo.data[i][2],SGBInfo.data[i][3],
            SGBInfo.data[i][4],SGBInfo.data[i][5],SGBInfo.data[i][6],SGBInfo.data[i][7],
            SGBInfo.data[i][8],SGBInfo.data[i][9],SGBInfo.data[i][10],SGBInfo.data[i][11],
            SGBInfo.data[i][12],SGBInfo.data[i][13],SGBInfo.data[i][14],SGBInfo.data[i][15]);
        strcat(endtext,text);
        if(numpackets) strcat(endtext,"\r\n");
        i++;
	}
    SetWindowText(hEditPacketData,(LPCTSTR)endtext);

    //--------------------------------------

    char * command_names[0x20] = {
            "PAL01","PAL23","PAL03","PAL12","ATTR_BLK","ATTR_LIN","ATTR_DIV","ATTR_CHR",
            "SOUND","SOU_TRN","PAL_SET","PAL_TRN","ATRC_EN","TEST_EN","ICON_EN","DATA_SND",
            "DATA_TRN","MLT_REG","JUMP","CHR_TRN","PCT_TRN","ATTR_TRN","ATTR_SET","MASK_EN",
            "OBJ_TRN","???","UNKNOWN","UNKNOWN","UNKNOWN","UNKNOWN","BIOS_1 (?)","BIOS_2 (?)"
    };
    char * command_len[0x20] = {
        "1","1","1","1","1~7","1~7","1","1~6",
        "1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1",
        "1","1?","?","?","?","?","1?","1?"
    };
    sprintf(endtext,"CMD:0x%02X %s(%s)",(SGBInfo.data[0][0]>>3)&0x1F,
        command_names[(SGBInfo.data[0][0]>>3)&0x1F],command_len[(SGBInfo.data[0][0]>>3)&0x1F]);
    SetWindowText(hStaticCommandInfo,(LPCTSTR)endtext);
}

//-----------------

static void sgb_updateotherinfo(void)
{
    char * srcmode[4] = {"Normal","Freeze","Black","Backdrop"};

    char text[200];
    sprintf(text,"Players: %d\r\n\r\nScreen: %s\r\n\r\nAttraction: %d\r\n"
        "Test Speed: %d\r\n\r\nSGB Disabled: %d",
        SGBInfo.multiplayer != 0 ? SGBInfo.multiplayer : 1,
        srcmode[SGBInfo.freeze_screen],SGBInfo.attracion_mode,SGBInfo.test_speed_mode,
        SGBInfo.disable_sgb);
    SetWindowText(hEditOtherInfo,(LPCTSTR)text);
}

//-----------------

void GLWindow_SGBViewerUpdate(void)
{
    if(ViewerCreated == 0) return;

    if(RUNNING != RUN_GB) return;
    if(GameBoy.Emulator.SGBEnabled == 0) return;

    SGB_ScreenDraw();
    sgb_updatescreeninfo();

    SGB_TilesDraw();
    sgb_updatetilesetinfo();

    SelectedATF = SGBInfo.curr_ATF;
    SetScrollPos(hWndScrollATF, SB_CTL, SelectedATF, TRUE);
    SGB_ATFDraw(SelectedATF);
    sgb_updateatfinfo();

    sgb_updatepalinfo();

    sgb_updatepacketinfo();

    sgb_updateotherinfo();

    InvalidateRect(hWndSGBViewer, NULL, FALSE);
}

static LRESULT CALLBACK SGBViewerProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont, hFontFixed;

    switch(Msg)
    {
        case WM_PAINT:
        {
            //START
            PAINTSTRUCT Ps;
            HBRUSH hBrush;
            RECT sel_rc;
            HDC hDC = BeginPaint(hWnd, &Ps);

            //SGB SCREEN
            HBITMAP bitmap = CreateBitmap(256, 256, 1, 32, sgbscreen);
            HDC MemDC = CreateCompatibleDC(hDC);
            SelectObject(MemDC, bitmap);
            BitBlt(hDC, 15, 25, 256, 256, MemDC, 0, 0, SRCCOPY);
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            hBrush = (HBRUSH)CreateSolidBrush( RGB(255,0,0) );
            sel_rc.left = 15 + ((SelectedTileScreen%32)*8);
            sel_rc.top = 25 + ((SelectedTileScreen/32)*8);
            sel_rc.right = sel_rc.left + 8;
            sel_rc.bottom = sel_rc.top + 8;
            FrameRect(hDC,&sel_rc,hBrush);
            DeleteObject(hBrush);

            bitmap = CreateBitmap(64, 64, 1, 32, sgbtilescreen);
            MemDC = CreateCompatibleDC(hDC);
            SelectObject(MemDC, bitmap);
            BitBlt(hDC, 15, 285, 64, 64, MemDC, 0, 0, SRCCOPY);
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            //SGB TILES
            bitmap = CreateBitmap(128, 128, 1, 32, sgbtiles);
            MemDC = CreateCompatibleDC(hDC);
            SelectObject(MemDC, bitmap);
            BitBlt(hDC, 300, 25, 128, 128, MemDC, 0, 0, SRCCOPY);
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            hBrush = (HBRUSH)CreateSolidBrush( RGB(255,0,0) );
            sel_rc.left = 300 + ((SelectedTileTileset%16)*8);
            sel_rc.top = 25 + ((SelectedTileTileset/16)*8);
            sel_rc.right = sel_rc.left + 8;
            sel_rc.bottom = sel_rc.top + 8;
            FrameRect(hDC,&sel_rc,hBrush);
            DeleteObject(hBrush);

            bitmap = CreateBitmap(64, 64, 1, 32, sgbtiletileset);
            MemDC = CreateCompatibleDC(hDC);
            SelectObject(MemDC, bitmap);
            BitBlt(hDC, 435, 88, 64, 64, MemDC, 0, 0, SRCCOPY);
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            //ATF
            bitmap = CreateBitmap(160+1, 144+1, 1, 32, atfscreen);
            MemDC = CreateCompatibleDC(hDC);
            SelectObject(MemDC, bitmap);
            BitBlt(hDC, 300, 185, 160+1, 144+1, MemDC, 0, 0, SRCCOPY);
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            hBrush = (HBRUSH)CreateSolidBrush( RGB(255,0,0) );
            sel_rc.left = 300 + ((SelectedATFTile%20)*8);
            sel_rc.top = 185 + ((SelectedATFTile/20)*8);
            sel_rc.right = sel_rc.left + 9;
            sel_rc.bottom = sel_rc.top + 9;
            FrameRect(hDC,&sel_rc,hBrush);
            DeleteObject(hBrush);

            //PALETTES
            HPEN hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(192,192,192));
            SelectObject(hDC, hPen);
            int i;
            for(i = 0; i < 8*16; i++)
            {
                if(!( ((i/16)<4) && ((i%16)>=4) ))
                {
                    hBrush = (HBRUSH)CreateSolidBrush( rgb16toCOLORREF(SGBInfo.palette[i/16][i%16]) );
                    SelectObject(hDC, hBrush);
                    Rectangle(hDC, 15 + ((i%16)*10), 380 + ((i/16)*10), 15 + 10 + ((i%16)*10), 380 + 10 + ((i/16)*10));
                    DeleteObject(hBrush);
                }
            }
            DeleteObject(hPen);

            hBrush = (HBRUSH)CreateSolidBrush( RGB(255,0,0) );
            sel_rc.left = 15 + ((SelectedPal%16)*10);
            sel_rc.top = 380 + ((SelectedPal/16)*10);
            sel_rc.right = sel_rc.left + 10;
            sel_rc.bottom = sel_rc.top + 10;
            FrameRect(hDC,&sel_rc,hBrush);
            sel_rc.left++; sel_rc.top++; sel_rc.right--; sel_rc.bottom--;
            FrameRect(hDC,&sel_rc,hBrush);
            DeleteObject(hBrush);

            //END
            EndPaint(hWnd, &Ps);
            break;
        }
        case WM_HSCROLL:
        {
            int CurPos = GetScrollPos(hWndScrollATF, SB_CTL);
            int update = 0;
            switch (LOWORD(wParam))
            {
                case SB_LEFT: CurPos = 0; update = 1; break;
                case SB_LINELEFT: if(CurPos > 0) { CurPos--; update = 1; } break;
                case SB_PAGELEFT: if(CurPos >= 4) { CurPos-=4; update = 1; break; }
                                  if(CurPos > 0) { CurPos = 0; update = 1; } break;
                case SB_THUMBPOSITION: CurPos = HIWORD(wParam); update = 1; break;
                case SB_THUMBTRACK: CurPos = HIWORD(wParam); update = 1; break;
                case SB_PAGERIGHT: if(CurPos < 40) { CurPos+=4; update = 1; break; }
                                   if(CurPos < 44) { CurPos = 44; update = 1; } break;
                case SB_LINERIGHT: if(CurPos < 44) { CurPos++; update = 1; } break;
                case SB_RIGHT: CurPos = 44; update = 1; break;
                case SB_ENDSCROLL:
                default:
                    break;
            }

            if(update)
            {
                SetScrollPos(hWndScrollATF, SB_CTL, CurPos, TRUE);
                SelectedATF = CurPos;
                sgb_updateatfinfo();
                SGB_ATFDraw(SelectedATF);
                InvalidateRect(hWndSGBViewer, NULL, FALSE);
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            //SGB SCREEN
            if( (x>=15) && (x<15+256) && (y>=25) && (y<25+256) )
            {
                SelectedTileScreen = ((x-15)/8) + ((y-25)/8)*32;
                sgb_updatescreeninfo();
            }
            //SGB TILES
            if( (x>=300) && (x<300+128) && (y>=25) && (y<25+128) )
            {
                SelectedTileTileset = ((x-300)/8) + ((y-25)/8)*16;
                sgb_updatetilesetinfo();
            }
            //ATF
            if( (x>=300) && (x<300+160) && (y>=185) && (y<185+144) )
            {
                SelectedATFTile = ((x-300)/8) + ((y-185)/8)*20;
                sgb_updateatfinfo();
            }
            //PALETTES
            if( (x>=15) && (x<15+40) && (y>=380) && (y<420) )
            {
                SelectedPal = ((x-15)/10) + ((y-380)/10)*16;
                sgb_updatepalinfo();
            }
            if( (x>=15) && (x<15+160) && (y>=420) && (y<460) )
            {
                SelectedPal = ((x-15)/10) + ((y-380)/10)*16;
                sgb_updatepalinfo();
            }
            InvalidateRect(hWndSGBViewer, NULL, FALSE);
            break;
        }
    }
    return 0;
}

#endif // 0
