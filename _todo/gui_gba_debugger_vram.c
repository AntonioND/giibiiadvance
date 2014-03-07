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

#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <ctype.h>

#include "build_options.h"
#include "resource.h"

#include "gui_mainloop.h"
#include "gui_main.h"
#include "gui_gba_debugger_vram.h"
#include "gba_core/gba.h"
#include "gba_core/memory.h"
#include "gba_core/cpu.h"
#include "gba_core/video.h"

//-----------------------------------------------------------------------------------------
//                                   PALETTE VIEWER
//-----------------------------------------------------------------------------------------

static HWND hWndPalViewer;
static int PalViewerCreated = 0;
static HWND hPalViewStaticAddr, hPalViewStaticValue;
static HWND hPalViewRGB, hPalViewIndex;

static u32 palview_sprpal = 0;
static u32 palview_selectedindex = 0;

void GLWindow_GBAPalViewerUpdate(void)
{
    if(PalViewerCreated == 0) return;

    if(RUNNING != RUN_GBA) return;

    InvalidateRect(hWndPalViewer, NULL, FALSE);

    u32 address = 0x05000000 + (palview_sprpal*(256*2)) + (palview_selectedindex*2);

    char text[20];
    sprintf(text,"Address: 0x%08X",address);
    SetWindowText(hPalViewStaticAddr,(LPCTSTR)text);

    u16 value = ((u16*)Mem.pal_ram)[palview_selectedindex+(palview_sprpal*256)];

    sprintf(text,"Value: 0x%04X",value);
    SetWindowText(hPalViewStaticValue,(LPCTSTR)text);

    sprintf(text,"RGB: (%d,%d,%d)",value&31,(value>>5)&31,(value>>10)&31);
    SetWindowText(hPalViewRGB,(LPCTSTR)text);

    sprintf(text,"Index: %d|%d[%d]",palview_selectedindex,palview_selectedindex/16,
            palview_selectedindex%16);
    SetWindowText(hPalViewIndex,(LPCTSTR)text);
}

static inline COLORREF rgb16to32(u16 color)
{
    return RGB((color & 31)<<3,((color >> 5) & 31)<<3,((color >> 10) & 31)<<3);
}

static LRESULT CALLBACK PalViewerProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont, hFontFixed;

    switch(Msg)
    {
        case WM_CREATE:
        {
            palview_sprpal = 0;
            palview_selectedindex = 0;

            hFont = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, DEFAULT_PITCH, NULL);

            hFontFixed = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, FIXED_PITCH, NULL);

            HWND hGroupBg = CreateWindow(TEXT("button"), TEXT("Background"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    5, 5, 175, 185, hWnd, (HMENU) 0, hInstance, NULL);
            SendMessage(hGroupBg, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

            HWND hGroupSpr = CreateWindow(TEXT("button"), TEXT("Sprite"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    190, 5, 175, 185, hWnd, (HMENU) 0, hInstance, NULL);
            SendMessage(hGroupSpr, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

            hPalViewStaticAddr = CreateWindow(TEXT("static"), TEXT("Address: 0x05000000"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        5, 195, 140, 16, hWnd, NULL, hInstance, NULL);
            SendMessage(hPalViewStaticAddr, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            hPalViewStaticValue = CreateWindow(TEXT("static"), TEXT("Value: 0x0000"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        190, 195, 100, 16, hWnd, NULL, hInstance, NULL);
            SendMessage(hPalViewStaticValue, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            hPalViewRGB = CreateWindow(TEXT("static"), TEXT("RGB: (0,0,0)"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        190, 215, 110, 16, hWnd, NULL, hInstance, NULL);
            SendMessage(hPalViewRGB, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            hPalViewIndex = CreateWindow(TEXT("static"), TEXT("Index: 0|0[0]"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        5, 215, 125, 16, hWnd, NULL, hInstance, NULL);
            SendMessage(hPalViewIndex, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HBRUSH hBrush;
            HDC hDC = BeginPaint(hWnd, &ps);

            HPEN hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(192,192,192));
            SelectObject(hDC, hPen);
            int i;
            for(i = 0; i < 256; i++)
            {
                //BG
                hBrush = (HBRUSH)CreateSolidBrush( rgb16to32(((u16*)Mem.pal_ram)[i]) );
                SelectObject(hDC, hBrush);
                Rectangle(hDC, 12 + ((i%16)*10), 23 + ((i/16)*10), 23 + ((i%16)*10), 34 + ((i/16)*10));
                DeleteObject(hBrush);
                //SPR
                hBrush = (HBRUSH)CreateSolidBrush( rgb16to32(((u16*)Mem.pal_ram)[i+256]) );
                SelectObject(hDC, hBrush);
                Rectangle(hDC, 197 + ((i%16)*10), 23 + ((i/16)*10), 208 + ((i%16)*10), 34 + ((i/16)*10));
                DeleteObject(hBrush);
            }
            DeleteObject(hPen);

            hBrush = (HBRUSH)CreateSolidBrush( RGB(255,0,0) );

            RECT sel_rc;
            sel_rc.left = ((palview_selectedindex%16)*10) + (palview_sprpal?197:12);
            sel_rc.top = 23 + ((palview_selectedindex/16)*10);
            sel_rc.right = sel_rc.left + 11;
            sel_rc.bottom = sel_rc.top + 11;
            FrameRect(hDC,&sel_rc,hBrush);
            sel_rc.left++; sel_rc.top++; sel_rc.right--; sel_rc.bottom--;
            FrameRect(hDC,&sel_rc,hBrush);
            DeleteObject(hBrush);

            EndPaint(hWnd, &ps);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            int xpos = LOWORD(lParam); // mouse xpos
            int ypos = HIWORD(lParam); // mouse ypos

            if(ypos >= 23 && ypos < 183)
            {
                if(xpos >= 12 && xpos < 172) //BG
                {
                    int y = ypos - 23;
                    int x = xpos - 12;

                    palview_sprpal = 0;
                    palview_selectedindex = (x/10) + ((y/10)*16);
                }
                else if(xpos >= 197 && xpos < 357) //SPR
                {
                    int y = ypos - 23;
                    int x = xpos - 197;

                    palview_sprpal = 1;
                    palview_selectedindex = (x/10) + ((y/10)*16);
                }
            }

            GLWindow_GBAPalViewerUpdate();
            break;
        }
        case WM_SETFOCUS:
        {
            GLWindow_GBAPalViewerUpdate();
            break;
        }
        case WM_DESTROY:
            PalViewerCreated = 0;
            DeleteObject(hFont);
            DeleteObject(hFontFixed);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

void GLWindow_GBACreatePalViewer(void)
{
    if(PalViewerCreated) { SetActiveWindow(hWndPalViewer); return; }
    PalViewerCreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = PalViewerProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_GBAPalView";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_GBAPalView",
			  "Palette Viewer",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  375,
			  270,
			  hWndMain,
			  NULL,
			  hInstance,
			  NULL);

	if(!hWnd)
	{
	    PalViewerCreated = 0;
	    return;
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndPalViewer = hWnd;
}

void GLWindow_GBAClosePalViewer(void)
{
    if(PalViewerCreated) SendMessage(hWndPalViewer, WM_CLOSE, 0, 0);
}

//-----------------------------------------------------------------------------------------
//                                   SPRITE VIEWER
//-----------------------------------------------------------------------------------------

static HWND hWndSprViewer;
static int SprViewerCreated = 0;
static HWND hSprViewText, hSprNumText;
static int SprBuffer[128*128];
static HWND hWndScrollBar;

#define IDC_SPRITE_NUMBER    1

static inline int expand16to32(u16 color)
{
    return ( (((color & 31)<<3)<<16) | ((((color >> 5) & 31)<<3)<<8) | (((color >> 10) & 31)<<3) );
}

static void gba_sprite_debug_draw_to_buffer(void)
{
    //Temp buffer
    int sprbuffer[64*64];
    int sprbuffer_vis[64*64]; memset(sprbuffer_vis,0,sizeof(sprbuffer_vis));

    static const int spr_size[4][4][2] = { //shape, size, (x,y)
        {{8,8},{16,16},{32,32},{64,64}}, //Square
        {{16,8},{32,8},{32,16},{64,32}}, //Horizontal
        {{8,16},{8,32},{16,32},{32,64}}, //Vertical
        {{0,0},{0,0},{0,0},{0,0}} //Prohibited
    };

    _oam_spr_entry_t * spr = &(((_oam_spr_entry_t*)Mem.oam)[GetScrollPos(hWndScrollBar, SB_CTL)]);

    u16 attr0 = spr->attr0;
    u16 attr1 = spr->attr1;
    u16 attr2 = spr->attr2;
    u16 shape = attr0>>14;
    u16 size = attr1>>14;
    int sx = spr_size[shape][size][0];
    int sy = spr_size[shape][size][1];
    u16 tilebaseno = attr2&0x3FF;

    if(attr0&BIT(13)) //256 colors
    {
        tilebaseno >>= 1; //in 256 mode, they need double space

        u16 * palptr = (u16*)&(Mem.pal_ram[256*2]);

        //Start drawing
        int xdiff, ydiff;
        for(ydiff = 0; ydiff < sy; ydiff++) for(xdiff = 0; xdiff < sx; xdiff++)
        {
            u32 tileadd = 0;
            if(REG_DISPCNT & BIT(6)) //1D
            {
                int tilex = xdiff>>3;
                int tiley = ydiff>>3;
                tileadd = tilex + (tiley*sx/8);
            }
            else //2D
            {
                int tilex = xdiff>>3;
                int tiley = ydiff>>3;
                tileadd = tilex + (tiley*16);
            }

            u32 tileindex = tilebaseno+tileadd;
            u8 * tile_ptr = (u8*)&(Mem.vram[0x10000+(tileindex*64)]);

            int _x = xdiff & 7;
            int _y = ydiff & 7;

            u8 data = tile_ptr[_x+(_y*8)];

            if(data)
            {
                sprbuffer[ydiff*64 + xdiff] = expand16to32(palptr[data]);
                sprbuffer_vis[ydiff*64 + xdiff] = 1;
            }
        }
    }
    else //16 colors
    {
        u16 palno = attr2>>12;
        u16 * palptr = (u16*)&Mem.pal_ram[512+(palno*32)];

        //Start drawing
        int xdiff, ydiff;
        for(ydiff = 0; ydiff < sy; ydiff++) for(xdiff = 0; xdiff < sx; xdiff++)
        {
            u32 tileadd = 0;
            if(REG_DISPCNT & BIT(6)) //1D
            {
                int tilex = xdiff>>3;
                int tiley = ydiff>>3;
                tileadd = tilex + (tiley*sx/8);
            }
            else //2D
            {
                int tilex = xdiff>>3;
                int tiley = ydiff>>3;
                tileadd = tilex + (tiley*32);
            }

            u32 tileindex = tilebaseno+tileadd;
            u8 * tile_ptr = (u8*)&(Mem.vram[0x10000+(tileindex*32)]);

            int _x = xdiff & 7;
            int _y = ydiff & 7;

            u8 data = tile_ptr[(_x/2)+(_y*4)];

            if(_x&1) data = data>>4;
            else data = data & 0xF;

            if(data)
            {
                sprbuffer[ydiff*64 + xdiff] = expand16to32(palptr[data]);
                sprbuffer_vis[ydiff*64 + xdiff] = 1;
            }
        }
    }

    //Expand/copy to real buffer
    int factor = min(128/sx,128/sy);

    int i,j;
    for(i = 0; i < 128; i++) for(j = 0; j < 128; j++)
        SprBuffer[j*128+i] = ((i&32)^(j&32)) ? 0x00808080 : 0x00B0B0B0;

    for(i = 0; i < 128; i++) for(j = 0; j < 128; j++)
    {
        if(sprbuffer_vis[(j/factor)*64 + (i/factor)])
            SprBuffer[j*128+i] = sprbuffer[(j/factor)*64 + (i/factor)];
        else
            if( (i>=(sx*factor)) || (j>=(sy*factor)) )
                if( ((i^j)&7) == 0 ) SprBuffer[j*128+i] = 0x00FF0000;
    }
}

void GLWindow_GBASprViewerUpdate(void)
{
    if(SprViewerCreated == 0) return;

    if(RUNNING != RUN_GBA) return;

    InvalidateRect(hWndSprViewer, NULL, FALSE);

    static const int spr_size[4][4][2] = { //shape, size, (x,y)
        {{8,8},{16,16},{32,32},{64,64}}, //Square
        {{16,8},{32,8},{32,16},{64,32}}, //Horizontal
        {{8,16},{8,32},{16,32},{32,64}}, //Vertical
        {{0,0},{0,0},{0,0},{0,0}} //Prohibited
    };

    _oam_spr_entry_t * spr = &(((_oam_spr_entry_t*)Mem.oam)[GetScrollPos(hWndScrollBar, SB_CTL)]);
    u16 attr0 = spr->attr0;
    u16 attr1 = spr->attr1;
    u16 attr2 = spr->attr2;
    int isaffine = attr0 & BIT(8);
    u16 shape = attr0>>14;
    u16 size = attr1>>14;
    int sx = spr_size[shape][size][0];
    int sy = spr_size[shape][size][1];
    int y = (attr0&0xFF); y |= (y < 160) ? 0 : 0xFFFFFF00;
    int x = (int)(attr1&0x1FF) | ((attr1&BIT(8))?0xFFFFFE00:0);
    int mosaic = attr0 & BIT(12);
    int matrix_entry = (attr1>>9) & 0x1F;
    int mode = (attr0>>10)&3;
    int colors = (attr0&BIT(13)) ? 256:16;
    u16 tilebaseno = attr2&0x3FF;
    if(attr0&BIT(13)) tilebaseno>>= 1; //tiles need double space in 256 colors mode
    int vflip = (attr1 & BIT(13));
    int hflip = (attr1 & BIT(12));
    u16 prio = (attr2 >> 10) & 3;
    u16 palno = (attr0&BIT(13)) ?  0 : (attr2>>12);
    int doublesize = (attr0 & BIT(9));
    static const char * spr_mode[4] = {"Normal","Transp.","Window","Prohibited"};
    char text[1000];
    sprintf(text,"Type: %s\r\nSize: %dx%d\r\nPosition: %d,%d\r\n"
        "Mode: %d - %s\r\nTile base: %d\r\nColors: %d\r\nPriority: %d\r\nPal. Number: %d\r\n"
        "Attr: %04X|%04X|%04X\r\nMatrix entry: %d\r\n"
        "Other: %s%s%s%s",
        isaffine?"Affine" :"Regular", sx,sy, x,y,
        mode, spr_mode[mode],tilebaseno,colors,prio,palno,
        attr0,attr1,attr2,matrix_entry,
        mosaic?"M":" ", hflip?"H":" ", vflip?"V":" ", doublesize?"D":" ");
    SetWindowText(hSprViewText,(LPCTSTR)text);

    gba_sprite_debug_draw_to_buffer();
}

static LRESULT CALLBACK SprViewerProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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

            hWndScrollBar = CreateWindowEx(0,"SCROLLBAR",NULL,WS_CHILD | WS_VISIBLE | SBS_HORZ,
                       5,25,  150,15,   hWnd,NULL,hInstance,NULL);
            SCROLLINFO si; ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
            si.nMin = 0; si.nMax = 127; si.nPos = 0; si.nPage = 1;
            SetScrollInfo(hWndScrollBar, SB_CTL, &si, TRUE);

            hSprViewText = CreateWindow(TEXT("edit"), NULL,
                        WS_CHILD | WS_VISIBLE | BS_CENTER | ES_MULTILINE | ES_READONLY,
                        5, 50, 150, 170, hWnd, NULL, hInstance, NULL);
            SendMessage(hSprViewText, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            hSprNumText = CreateWindow(TEXT("edit"), TEXT("0"),
                        WS_CHILD | WS_VISIBLE | BS_CENTER | ES_MULTILINE | WS_BORDER,
                        5, 5, 150, 20, hWnd, (HMENU)IDC_SPRITE_NUMBER, hInstance, NULL);
            SendMessage(hSprNumText, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));
            break;
        }
        case WM_COMMAND:
        {
            switch(HIWORD(wParam))
            {
                case EN_CHANGE:
                    if(LOWORD(wParam) == IDC_SPRITE_NUMBER)
                    {
                        TCHAR text[32];
                        GetWindowText(hSprNumText, text, 32);
                        u64 val = asciidectoint(text);
                        if( (val != 0xFFFFFFFFFFFFFFFFULL) && (val < 128) )
                            SetScrollPos(hWndScrollBar, SB_CTL, (int)val, TRUE);
                        GLWindow_GBASprViewerUpdate();
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT Ps;
            HDC hDC = BeginPaint(hWnd, &Ps);
            //Load the bitmap
            HBITMAP bitmap = CreateBitmap(128, 128, 1, 32, SprBuffer);
            // Create a memory device compatible with the above DC variable
            HDC MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 165, 5, 128, 128, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);
            EndPaint(hWnd, &Ps);
            break;
        }
        case WM_HSCROLL:
        {
            int CurPos = GetScrollPos(hWndScrollBar, SB_CTL);
            int update = 0;
            switch (LOWORD(wParam))
            {
                case SB_LEFT: CurPos = 0; update = 1; break;
                case SB_LINELEFT: if(CurPos > 0) { CurPos--; update = 1; } break;
                case SB_PAGELEFT: if(CurPos >= 8) { CurPos-=8; update = 1; break; }
                                  if(CurPos > 0) { CurPos = 0; update = 1; } break;
                case SB_THUMBPOSITION: CurPos = HIWORD(wParam); update = 1; break;
                case SB_THUMBTRACK: CurPos = HIWORD(wParam); update = 1; break;
                case SB_PAGERIGHT: if(CurPos < 120) { CurPos+=8; update = 1; break; }
                                   if(CurPos < 127) { CurPos = 127; update = 1; } break;
                case SB_LINERIGHT: if(CurPos < 127) { CurPos++; update = 1; } break;
                case SB_RIGHT: CurPos = 127; update = 1; break;
                case SB_ENDSCROLL:
                default:
                    break;
            }

            if(update)
            {
                SetScrollPos(hWndScrollBar, SB_CTL, CurPos, TRUE);
                char text[5]; sprintf(text,"%d",CurPos);
                SetWindowText(hSprNumText,text);
                GLWindow_GBASprViewerUpdate();
            }
            break;
        }
        case WM_MOUSEWHEEL:
        {
            short zDelta = (short)HIWORD(wParam);
            int increment = (zDelta/WHEEL_DELTA);
            int CurPos = GetScrollPos(hWndScrollBar, SB_CTL) + increment;
            if( (CurPos >= 0) && (CurPos < 128) )
            {
                SetScrollPos(hWndScrollBar, SB_CTL, CurPos, TRUE);
                char text[5]; sprintf(text,"%d",CurPos);
                SetWindowText(hSprNumText,text);
                GLWindow_GBASprViewerUpdate();
            }
            break;
        }
        case WM_LBUTTONDOWN:
            SetFocus(hWndSprViewer);
            //no break;
        case WM_SETFOCUS:
            GLWindow_GBASprViewerUpdate();
            break;
        case WM_DESTROY:
            SprViewerCreated = 0;
            DeleteObject(hFont);
            DeleteObject(hFontFixed);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return 0;
}

void GLWindow_GBACreateSprViewer(void)
{
    if(SprViewerCreated) { SetActiveWindow(hWndSprViewer); return; }
    SprViewerCreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = SprViewerProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_GBASprView";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_GBASprView",
			  "Sprite Viewer",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  305,
			  255,
			  hWndMain,
			  NULL,
			  hInstance,
			  NULL);

	if(!hWnd)
	{
	    SprViewerCreated = 0;
	    return;
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndSprViewer = hWnd;
}

void GLWindow_GBACloseSprViewer(void)
{
    if(SprViewerCreated) SendMessage(hWndSprViewer, WM_CLOSE, 0, 0);
}

//-----------------------------------------------------------------------------------------
//                                   MAP VIEWER
//-----------------------------------------------------------------------------------------

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
            sprintf(text,"Tile: %d (%s%s)\r\nPos: %d,%d\r\nPal: --\r\nAddr: 0x%08X\r\n",
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
            sprintf(text,"Tile: %d (%s%s)\r\nPos: %d,%d\r\nPal: %d\r\nAddr: 0x%08X\r\n",
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

        sprintf(text,"Tile: %d (--)\r\nPos: %d,%d\r\nPal: --\r\nAddr: 0x%08X\r\n",
                SE&0x3FF,TileX,TileY,((BgControl>>8)&0x1F) * (2*1024) + TileSEIndex + 0x06000000);
        SetWindowText(hMapTileText,(LPCTSTR)text);
    }
    else
    {
        SetWindowText(hMapTileText,(LPCTSTR)"Tile: --- (--)\r\nPos: ---------\r\nPal: --\r\nAddr: ----------\r\n");
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

static void gba_map_viewer_update(void)
{
    u16 control = 0;
    switch(SelectedLayer)
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
    u32 bgmode = bgtypearray[mode][SelectedLayer];

    BgMode = bgmode; BgControl = control;
    TileX = 0; TileY = 0;

    if(bgmode == 0) //Shouldn't get here...
    {
        SizeX = SizeY = 0;
        SetWindowText(hMapText,(LPCTSTR)"Size: ------- (Unused)\r\nColors: ---\r\nPriority: -\r\nWrap: -\r\n"
        "Char Base: ----------\r\nScrn Base: ----------\r\nMosaic: -");
        SetWindowText(hMapTileText,(LPCTSTR)"Tile: --- (--)\r\nPos: ---------\r\nPal: --\r\nAddr: ----------\r\n");

        SCROLLINFO si; ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        si.nMin = 0; si.nMax = 0; si.nPos = 0; si.nPage = 1;
        SetScrollInfo(hWndMapScrollH, SB_CTL, &si, TRUE);
        SetScrollInfo(hWndMapScrollV, SB_CTL, &si, TRUE);
        return;
    }
    if(bgmode == 1) //text
    {
        static const u32 text_bg_size[4][2] = { {256,256}, {512,256}, {256,512}, {512,512} };
        u32 CBB = ( ((control>>2)&3) * (16*1024) ) + 0x06000000;
        u32 SBB = ( ((control>>8)&0x1F) * (2*1024) ) + 0x06000000;
        u32 sizex = text_bg_size[control>>14][0];
        u32 sizey = text_bg_size[control>>14][1];
        SizeX = sizex; SizeY = sizey;
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int colors = (control & BIT(7)) ? 256 : 16;
        int prio = control&3;
        char text[1000];
        sprintf(text,"Size: %dx%d (Text)\r\nColors: %d\r\nPriority: %d\r\nWrap: 1\r\nChar Base: 0x%08X\r\n"
        "Scrn Base: 0x%08X\r\nMosaic: %d",
        sizex,sizey,colors,prio,CBB,SBB, mosaic);
        SetWindowText(hMapText,(LPCTSTR)text);
    }
    if(bgmode == 2) //affine
    {
        static const u32 affine_bg_size[4] = { 128, 256, 512, 1024 };
        u32 CBB = ( ((control>>2)&3) * (16*1024) ) + 0x06000000;
        u32 SBB = ( ((control>>8)&0x1F) * (2*1024) ) + 0x06000000;
        u32 sizex = affine_bg_size[control>>14];
        u32 sizey = sizex;
        SizeX = sizex; SizeY = sizey;
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int wrap = (control & BIT(13)) != 0;
        int prio = control&3;
        char text[1000];
        sprintf(text,"Size: %dx%d (Affine)\r\nColors: 256\r\nPriority: %d\r\nWrap: %d\r\nChar Base: 0x%08X\r\n"
        "Scrn Base: 0x%08X\r\nMosaic: %d",
        sizex,sizey,prio,wrap,CBB,SBB, mosaic);
        SetWindowText(hMapText,(LPCTSTR)text);
    }
    if(bgmode == 3) //bg2 mode 3
    {
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int prio = control&3;
        SizeX = 240; SizeY = 160;
        char text[1000];
        sprintf(text,"Size: 240x160 (Bitmap)\r\nColors: 16bit\r\nPriority: %d\r\nWrap: 0\r\nChar Base: ----------\r\n"
        "Scrn Base: 0x06000000\r\nMosaic: %d",prio,mosaic);
        SetWindowText(hMapText,(LPCTSTR)text);
        SetWindowText(hMapTileText,(LPCTSTR)"Tile: --- (--)\r\nPos: ---------\r\nPal: --\r\nAddr: ----------\r\n");
    }
    if(bgmode == 4) //bg2 mode 4
    {
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int prio = control&3;
        u32 SBB = 0x06000000 + ((REG_DISPCNT&BIT(4))?0xA000:0);
        SizeX = 240; SizeY = 160;
        char text[1000];
        sprintf(text,"Size: 240x160 (Bitmap)\r\nColors: 256\r\nPriority: %d\r\nWrap: 0\r\nChar Base: ----------\r\n"
        "Scrn Base: 0x%08X\r\nMosaic: %d",prio,SBB,mosaic);
        SetWindowText(hMapText,(LPCTSTR)text);
        SetWindowText(hMapTileText,(LPCTSTR)"Tile: --- (--)\r\nPos: ---------\r\nPal: --\r\nAddr: ----------\r\n");
    }
    if(bgmode == 5) //bg2 mode 5
    {
        int mosaic = (control & BIT(6)) != 0; //mosaic
        int prio = control&3;
        u32 SBB = 0x06000000 + ((REG_DISPCNT&BIT(4))?0xA000:0);
        SizeX = 160; SizeY = 128;
        char text[1000];
        sprintf(text,"Size: 160x128 (Bitmap)\r\nColors: 16bit\r\nPriority: %d\r\nWrap: 0\r\nChar Base: ----------\r\n"
        "Scrn Base: 0x%08X\r\nMosaic: %d",prio,SBB,mosaic);
        SetWindowText(hMapText,(LPCTSTR)text);
        SetWindowText(hMapTileText,(LPCTSTR)"Tile: --- (--)\r\nPos: ---------\r\nPal: --\r\nAddr: ----------\r\n");
    }

    gba_map_viewer_draw_to_buffer(control,bgmode);
    gba_map_viewer_update_scrollbars();
    gba_map_update_window_from_temp_buffer();
    gba_map_viewer_update_tile();
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
        case WM_VSCROLL:
        {
            int CurPos = GetScrollPos(hWndMapScrollV, SB_CTL);
            int max_y = max(0,SizeY-256);
            switch (LOWORD(wParam))
            {
                case SB_TOP: CurPos = 0; break;
                case SB_LINEUP: CurPos-=8; if(CurPos < 0) CurPos = 0;  break;
                case SB_PAGEUP: if(CurPos >= 8) { CurPos-=8; break; }
                                  if(CurPos > 0) { CurPos = 0; } break;
                case SB_THUMBPOSITION: CurPos = HIWORD(wParam)&~7;  break;
                case SB_THUMBTRACK: CurPos = HIWORD(wParam)&~7;  break;
                case SB_PAGEDOWN: if(CurPos < (max_y-8)) { CurPos+=8; break; }
                                  if(CurPos < max_y) { CurPos = max_y; } break;
                case SB_LINEDOWN: CurPos+= 8; if(CurPos > max_y) CurPos = max_y; break;
                case SB_BOTTOM: CurPos = max_y; break;
                case SB_ENDSCROLL:
                default:
                    break;
            }
            SetScrollPos(hWndMapScrollV, SB_CTL, CurPos, TRUE);

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
        case WM_SETFOCUS:
            GLWindow_GBAMapViewerUpdate();
            break;
        /*//case WM_CONTEXTMENU:
        case WM_RBUTTONDOWN:
        {
            //HWND hWnd = (HWND)wParam;
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            if(xPos > 200)
            {
                MessageBox(NULL, "Hi! I'm Alice, nice to meet you!.", "Hi!", MB_OK);
            }
            break;
        }*/
        case WM_DESTROY:
            MapViewerCreated = 0;
            DeleteObject(hFont);
            DeleteObject(hFontFixed);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return 0;
}

void GLWindow_GBACreateMapViewer(void)
{
    if(MapViewerCreated) { SetActiveWindow(hWndMapViewer); return; }
    MapViewerCreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = MapViewerProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_GBAMapView";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_GBAMapView",
			  "Map Viewer",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  482,
			  313,
			  hWndMain,
			  NULL,
			  hInstance,
			  NULL);

	if(!hWnd)
	{
	    MapViewerCreated = 0;
	    return;
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndMapViewer = hWnd;
}

void GLWindow_GBACloseMapViewer(void)
{
    if(MapViewerCreated) SendMessage(hWndMapViewer, WM_CLOSE, 0, 0);
}

//-----------------------------------------------------------------------------------------
//                                   TILE VIEWER
//-----------------------------------------------------------------------------------------

static HWND hWndTileViewer;
static int TileViewerCreated = 0;
u32 SelectedBase, TileViewColors, SelectedPalette;
static int TileBuffer[256*256], SelectedTileBuffer[64*64];
static u32 SelTileX,SelTileY;
static HWND hTileText;
static HWND hWndTileScroll;

#define IDC_CBB00    1
#define IDC_CBB04    2
#define IDC_CBB08    3
#define IDC_CBB0C    4
#define IDC_CBB10    5
#define IDC_CBB14    6

#define IDC_COL16   11
#define IDC_COL256  12

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

void GLWindow_GBATileViewerUpdate(void)
{
    if(TileViewerCreated == 0) return;

    if(RUNNING != RUN_GBA) return;

    SelTileX = 0; SelTileY = 0;

    gba_tile_viewer_draw_to_buffer();
    gba_tile_viewer_update_tile();

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

            HWND hGroup1 = CreateWindow(TEXT("button"), TEXT("Character Base Block"),
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                3,1, 165,140, hWnd, (HMENU) 0, hInstance, NULL);
            HWND hBtn1 = CreateWindow(TEXT("button"), TEXT("0x06000000 - BG"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 18, 130, 18, hWnd, (HMENU)IDC_CBB00 , hInstance, NULL);
            HWND hBtn2 = CreateWindow(TEXT("button"), TEXT("0x06004000 - BG"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 38, 130, 18, hWnd, (HMENU)IDC_CBB04 , hInstance, NULL);
            HWND hBtn3 = CreateWindow(TEXT("button"), TEXT("0x06008000 - BG"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 58, 130, 18, hWnd, (HMENU)IDC_CBB08, hInstance, NULL);
            HWND hBtn4 = CreateWindow(TEXT("button"), TEXT("0x0600C000 - BG"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 78, 130, 18, hWnd, (HMENU)IDC_CBB0C, hInstance, NULL);
            HWND hBtn5 = CreateWindow(TEXT("button"), TEXT("0x06010000 - OBJ/BG"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 98, 150, 18, hWnd, (HMENU)IDC_CBB10, hInstance, NULL);
            HWND hBtn6 = CreateWindow(TEXT("button"), TEXT("0x06014000 - (OBJ)"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 118, 150, 18, hWnd, (HMENU)IDC_CBB14, hInstance, NULL);

            HWND hGroup2 = CreateWindow(TEXT("button"), TEXT("Colors"),
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                3,142, 165,40, hWnd, (HMENU) 0, hInstance, NULL);
            HWND hBtn7 = CreateWindow(TEXT("button"), TEXT("16"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 159, 50, 18, hWnd, (HMENU)IDC_COL16 , hInstance, NULL);
            HWND hBtn8 = CreateWindow(TEXT("button"), TEXT("256"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                85, 159, 50, 18, hWnd, (HMENU)IDC_COL256 , hInstance, NULL);

            SendMessage(hGroup1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn1, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));
            SendMessage(hBtn2, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));
            SendMessage(hBtn3, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));
            SendMessage(hBtn4, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));
            SendMessage(hBtn5, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));
            SendMessage(hBtn6, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));
            SendMessage(hGroup2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn7, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));
            SendMessage(hBtn8, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            CheckRadioButton(hWnd,IDC_CBB00,IDC_CBB14,IDC_CBB00);
            CheckRadioButton(hWnd,IDC_COL16,IDC_COL256,IDC_COL16);

            SelectedBase = 0x06000000;
            TileViewColors = 16;

            SelTileX = 0; SelTileY = 0;

            hTileText = CreateWindow(TEXT("edit"), TEXT(" "),
                        WS_CHILD | WS_VISIBLE | BS_CENTER | ES_MULTILINE | ES_READONLY,
                        72, 194, 120, 45, hWnd, NULL, hInstance, NULL);
            SendMessage(hTileText, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));


            hWndTileScroll = CreateWindowEx(0,"SCROLLBAR",NULL,WS_CHILD | WS_VISIBLE | SBS_HORZ,
                       72,242,  120,15,   hWnd,NULL,hInstance,NULL);

            SCROLLINFO si; ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
            si.nMin = 0; si.nMax = 15; si.nPos = 0; si.nPage = 1;
            SetScrollInfo(hWndTileScroll, SB_CTL, &si, TRUE);

            SelectedPalette = 0;

            GLWindow_GBATileViewerUpdate();
            break;
        }
        case WM_COMMAND:
        {
            if(HIWORD(wParam) == BN_CLICKED)
            {
                switch(LOWORD(wParam))
                {
                    case IDC_CBB00: SelectedBase = 0x06000000;
                        CheckRadioButton(hWnd,IDC_CBB00,IDC_CBB14,IDC_CBB00); break;
                    case IDC_CBB04: SelectedBase = 0x06004000;
                        CheckRadioButton(hWnd,IDC_CBB00,IDC_CBB14,IDC_CBB04); break;
                    case IDC_CBB08: SelectedBase = 0x06008000;
                        CheckRadioButton(hWnd,IDC_CBB00,IDC_CBB14,IDC_CBB08); break;
                    case IDC_CBB0C: SelectedBase = 0x0600C000;
                        CheckRadioButton(hWnd,IDC_CBB00,IDC_CBB14,IDC_CBB0C); break;
                    case IDC_CBB10: SelectedBase = 0x06010000;
                        CheckRadioButton(hWnd,IDC_CBB00,IDC_CBB14,IDC_CBB10); break;
                    case IDC_CBB14: SelectedBase = 0x06014000;
                        CheckRadioButton(hWnd,IDC_CBB00,IDC_CBB14,IDC_CBB14); break;

                    case IDC_COL16: TileViewColors = 16;
                        CheckRadioButton(hWnd,IDC_COL16,IDC_COL256,IDC_COL16); break;
                    case IDC_COL256: TileViewColors = 256;
                        CheckRadioButton(hWnd,IDC_COL16,IDC_COL256,IDC_COL256); break;

                    default: break;
                }
                GLWindow_GBATileViewerUpdate();
            }
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT Ps;
            HDC hDC = BeginPaint(hWnd, &Ps);
            //Load the bitmap
            HBITMAP bitmap = CreateBitmap(256, 256, 1, 32, TileBuffer);
            // Create a memory device compatible with the above DC variable
            HDC MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 200, 5, 256, 256, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            bitmap = CreateBitmap(64, 64, 1, 32, SelectedTileBuffer);
            // Create a memory device compatible with the above DC variable
            MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 5, 194, 64, 64, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            EndPaint(hWnd, &Ps);
            break;
        }
        case WM_HSCROLL:
        {
            SelectedPalette = GetScrollPos(hWndTileScroll, SB_CTL);
            switch(LOWORD(wParam))
            {
                case SB_LEFT: SelectedPalette = 0; break;
                case SB_LINELEFT: if (SelectedPalette > 0) SelectedPalette--;  break;
                case SB_PAGELEFT: if(SelectedPalette >= 4) { SelectedPalette-=4; break; }
                                  if(SelectedPalette > 0) SelectedPalette = 0; break;
                case SB_THUMBPOSITION: SelectedPalette = HIWORD(wParam);  break;
                case SB_THUMBTRACK: SelectedPalette = HIWORD(wParam);  break;
                case SB_PAGERIGHT: if(SelectedPalette < 12) { SelectedPalette+=4; break; }
                                   if(SelectedPalette < 15) SelectedPalette = 15; break;
                case SB_LINERIGHT: if (SelectedPalette < 15) SelectedPalette++; break;
                case SB_RIGHT: SelectedPalette = 15; break;
                case SB_ENDSCROLL:
                default:
                    break;
            }
            SetScrollPos(hWndTileScroll, SB_CTL, SelectedPalette, TRUE);

            GLWindow_GBATileViewerUpdate();
            break;
        }
        case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if( (x >= 200) && (x < (200+256)) && (y >= 5) && (y < (5+256)) )
            {
                int bgx = (x-200);
                int bgy = (y-5);

                if( (TileViewColors == 256) && (bgy > 127) )  break;

                if(SelectedBase == 0x06014000) //half size
                {
                    if( (TileViewColors == 16) && (bgy > 127) )  break;
                    if( (TileViewColors == 256) && (bgy > 63) )  break;
                }
                SelTileX = bgx/8; SelTileY = bgy/8;

                gba_tile_viewer_update_tile();
            }
            break;
        }
        case WM_SETFOCUS:
            GLWindow_GBATileViewerUpdate();
            break;
        case WM_DESTROY:
            TileViewerCreated = 0;
            DeleteObject(hFont);
            DeleteObject(hFontFixed);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return 0;
}

void GLWindow_GBACreateTileViewer(void)
{
    if(TileViewerCreated) { SetActiveWindow(hWndTileViewer); return; }
    TileViewerCreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = TileViewerProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_GBATileView";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_GBATileView",
			  "Tile Viewer",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  467,
			  298,
			  hWndMain,
			  NULL,
			  hInstance,
			  NULL);

	if(!hWnd)
	{
	    TileViewerCreated = 0;
	    return;
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndTileViewer = hWnd;
}

void GLWindow_GBACloseTileViewer(void)
{
    if(TileViewerCreated) SendMessage(hWndTileViewer, WM_CLOSE, 0, 0);
}

