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
#include "gui_gb_debugger_vram.h"
#include "gb_core/gameboy.h"
#include "gb_core/debug.h"

extern _GB_CONTEXT_ GameBoy;

//-----------------------------------------------------------------------------------------
//                                   PALETTE VIEWER
//-----------------------------------------------------------------------------------------

static HWND hWndPalViewer;
static int PalViewerCreated = 0;
static HWND hPalViewStaticValue;
static HWND hPalViewRGB, hPalViewIndex;

static u32 palview_sprpal = 0;
static u32 palview_selectedindex = 0;


void GLWindow_GBPalViewerUpdate(void)
{
    if(PalViewerCreated == 0) return;

    if(RUNNING != RUN_GB) return;

    InvalidateRect(hWndPalViewer, NULL, FALSE);

    char text[20];

    u32 r,g,b;
    GB_Debug_Get_Palette(palview_sprpal,palview_selectedindex/4,palview_selectedindex%4,&r,&g,&b);
    u16 value = ((r>>3)&0x1F)|(((g>>3)&0x1F)<<5)|(((b>>3)&0x1F)<<10);

    sprintf(text,"Value: 0x%04X",value);
    SetWindowText(hPalViewStaticValue,(LPCTSTR)text);

    sprintf(text,"RGB: (%d,%d,%d)",r>>3,g>>3,b>>3);
    SetWindowText(hPalViewRGB,(LPCTSTR)text);

    sprintf(text,"Index: %d[%d]",palview_selectedindex/4,palview_selectedindex%4);
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
                    5, 5, 97, 185, hWnd, (HMENU) 0, hInstance, NULL);
            SendMessage(hGroupBg, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

            HWND hGroupSpr = CreateWindow(TEXT("button"), TEXT("Sprite"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    110, 5, 97, 185, hWnd, (HMENU) 0, hInstance, NULL);
            SendMessage(hGroupSpr, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

            hPalViewStaticValue = CreateWindow(TEXT("static"), TEXT("Value: 0x0000"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        5, 195, 100, 16, hWnd, NULL, hInstance, NULL);
            SendMessage(hPalViewStaticValue, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            hPalViewRGB = CreateWindow(TEXT("static"), TEXT("RGB: (0,0,0)"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        5, 215, 110, 16, hWnd, NULL, hInstance, NULL);
            SendMessage(hPalViewRGB, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            hPalViewIndex = CreateWindow(TEXT("static"), TEXT("Index: 0[0]"),
                        WS_CHILD | WS_VISIBLE | SS_SUNKEN | BS_CENTER,
                        110, 195, 85, 16, hWnd, NULL, hInstance, NULL);
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
            for(i = 0; i < 32; i++)
            {
                u32 r,g,b;
                //BG
                GB_Debug_Get_Palette(0,i/4,i%4,&r,&g,&b);
                hBrush = (HBRUSH)CreateSolidBrush( r|(g<<8)|(b<<16));
                SelectObject(hDC, hBrush);
                Rectangle(hDC, 12 + ((i%4)*20), 23 + ((i/4)*20), 34 + ((i%4)*20), 45 + ((i/4)*20));
                DeleteObject(hBrush);
                //SPR
                GB_Debug_Get_Palette(1,i/4,i%4,&r,&g,&b);
                hBrush = (HBRUSH)CreateSolidBrush( r|(g<<8)|(b<<16) );
                SelectObject(hDC, hBrush);
                Rectangle(hDC, 117 + ((i%4)*20), 23 + ((i/4)*20), 139 + ((i%4)*20), 45 + ((i/4)*20));
                DeleteObject(hBrush);
            }
            DeleteObject(hPen);

            hBrush = (HBRUSH)CreateSolidBrush( RGB(255,0,0) );

            RECT sel_rc;
            sel_rc.left = ((palview_selectedindex%4)*20) + (palview_sprpal?117:12);
            sel_rc.top = 23 + ((palview_selectedindex/4)*20);
            sel_rc.right = sel_rc.left + 22;
            sel_rc.bottom = sel_rc.top + 22;
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
                if(xpos >= 12 && xpos < 92) //BG
                {
                    int y = ypos - 23;
                    int x = xpos - 12;

                    palview_sprpal = 0;
                    palview_selectedindex = (x/20) + ((y/20)*4);
                }
                else if(xpos >= 117 && xpos < 197) //SPR
                {
                    int y = ypos - 23;
                    int x = xpos - 117;

                    palview_sprpal = 1;
                    palview_selectedindex = (x/20) + ((y/20)*4);
                }
            }

            GLWindow_GBPalViewerUpdate();
            break;
        }
        case WM_SETFOCUS:
        {
            GLWindow_GBPalViewerUpdate();
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

void GLWindow_GBCreatePalViewer(void)
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
	WndClsEx.lpszClassName = "Class_GBPalView";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_GBPalView",
			  "Palette Viewer",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  218,
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

void GLWindow_GBClosePalViewer(void)
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

static void gb_sprite_debug_draw_to_buffer(void)
{
    static int gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };

    //Temp buffer
    int sprbuffer[16*16];
    int sprbuffer_vis[16*16]; memset(sprbuffer_vis,0,sizeof(sprbuffer_vis));

    _GB_MEMORY_ * mem = &GameBoy.Memory;
    s32 spriteheight =  8 << ((mem->IO_Ports[LCDC_REG-0xFF00] & (1<<2)) != 0);
    _GB_OAM_ENTRY_ * GB_Sprite = &(((_GB_OAM_*)(mem->ObjAttrMem))->Sprite[GetScrollPos(hWndScrollBar, SB_CTL)]);
    u32 tile = GB_Sprite->Tile & ((spriteheight == 16) ? 0xFE : 0xFF);
    u8 * spr_data = &mem->VideoRAM[tile<<4]; //Bank 0

    if(GameBoy.Emulator.CGBEnabled)
    {
        if(GB_Sprite->Info & (1<<3)) spr_data += 0x2000; //If bank 1...

        u32 pal_index = (GB_Sprite->Info&7) * 8;

        u32 palette[4];
        palette[0] = GameBoy.Emulator.spr_pal[pal_index] | (GameBoy.Emulator.spr_pal[pal_index+1]<<8);
        palette[1] = GameBoy.Emulator.spr_pal[pal_index+2] | (GameBoy.Emulator.spr_pal[pal_index+3]<<8);
        palette[2] = GameBoy.Emulator.spr_pal[pal_index+4] | (GameBoy.Emulator.spr_pal[pal_index+5]<<8);
        palette[3] = GameBoy.Emulator.spr_pal[pal_index+6] | (GameBoy.Emulator.spr_pal[pal_index+7]<<8);

        int y,x;
        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            if(color != 0) // don't display transparent color
            {
                sprbuffer[y*16 + x] = expand16to32(palette[color]);
                sprbuffer_vis[y*16 + x] = 1;
            }
        }
    }
    else
    {
        u32 palette[4];
        u32 objX_reg;
        if(GB_Sprite->Info & (1<<4)) objX_reg = mem->IO_Ports[OBP1_REG-0xFF00];
        else objX_reg = mem->IO_Ports[OBP0_REG-0xFF00];

        palette[0] = objX_reg & 0x3;
		palette[1] = (objX_reg>>2) & 0x3;
		palette[2] = (objX_reg>>4) & 0x3;
		palette[3] = (objX_reg>>6) & 0x3;

        int y,x;
        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            if(color != 0) // don't display transparent color
            {
                sprbuffer[y*16 + x] = (gb_pal_colors[palette[color]][0] << 16) |
                        (gb_pal_colors[palette[color]][1] << 8) | gb_pal_colors[palette[color]][2];
                sprbuffer_vis[y*16 + x] = 1;
            }
        }
    }

    //Expand/copy to real buffer
    int factor = min(128/8,128/spriteheight);

    int i,j;
    for(i = 0; i < 128; i++) for(j = 0; j < 128; j++)
        SprBuffer[j*128+i] = ((i&32)^(j&32)) ? 0x00808080 : 0x00B0B0B0;

    for(i = 0; i < 128; i++) for(j = 0; j < 128; j++)
    {
        if(sprbuffer_vis[(j/factor)*16 + (i/factor)])
            SprBuffer[j*128+i] = sprbuffer[(j/factor)*16 + (i/factor)];
        else
            if( (i>=(8*factor)) || (j>=(spriteheight*factor)) )
                if( ((i^j)&7) == 0 ) SprBuffer[j*128+i] = 0x00FF0000;
    }
}

void GLWindow_GBSprViewerUpdate(void)
{
    if(SprViewerCreated == 0) return;

    if(RUNNING != RUN_GB) return;

    InvalidateRect(hWndSprViewer, NULL, FALSE);

    _GB_MEMORY_ * mem = &GameBoy.Memory;
    _GB_OAM_ENTRY_ * GB_Sprite = &(((_GB_OAM_*)(GameBoy.Memory.ObjAttrMem))->Sprite[GetScrollPos(hWndScrollBar, SB_CTL)]);

    int sy = 8 << ((mem->IO_Ports[LCDC_REG-0xFF00] & (1<<2)) != 0);
    int tile = GB_Sprite->Tile & ((sy == 16) ? 0xFE : 0xFF);
    int info = GB_Sprite->Info;
    char text[1000];
    sprintf(text,"Size: 8x%d\r\nPosition: %d,%d\r\nTile: %d (Bank %d)\r\n"
        "Priority: %d\r\nPal. Number: %d\r\n"
        "Other: %s%s",
        sy,GB_Sprite->X,GB_Sprite->Y,tile, GameBoy.Emulator.CGBEnabled ? 0 : ((info&BIT(3)) != 0),
        (info&BIT(7)) != 0, GameBoy.Emulator.CGBEnabled ? (info&3) : ((info&BIT(1)) != 0),
        (info&BIT(5))?"H":" ", (info&BIT(6))?"V":" ");

    SetWindowText(hSprViewText,(LPCTSTR)text);

    gb_sprite_debug_draw_to_buffer();
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

            hWndScrollBar = CreateWindowEx(0,TEXT("scrollbar"),NULL,WS_CHILD | WS_VISIBLE | SBS_HORZ,
                       5,25,  150,15,   hWnd,NULL,hInstance,NULL);
            SCROLLINFO si; ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
            si.nMin = 0; si.nMax = 39; si.nPos = 0; si.nPage = 1;
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
                        if( (val != 0xFFFFFFFFFFFFFFFFULL) && (val < 40) )
                            SetScrollPos(hWndScrollBar, SB_CTL, (int)val, TRUE);
                        GLWindow_GBSprViewerUpdate();
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
                case SB_PAGELEFT: if(CurPos >= 4) { CurPos-=4; update = 1; break; }
                                  if(CurPos > 0) { CurPos = 0; update = 1; } break;
                case SB_THUMBPOSITION: CurPos = HIWORD(wParam); update = 1; break;
                case SB_THUMBTRACK: CurPos = HIWORD(wParam); update = 1; break;
                case SB_PAGERIGHT: if(CurPos < 36) { CurPos+=4; update = 1; break; }
                                   if(CurPos < 39) { CurPos = 39; update = 1; } break;
                case SB_LINERIGHT: if(CurPos < 39) { CurPos++; update = 1; } break;
                case SB_RIGHT: CurPos = 39; update = 1; break;
                case SB_ENDSCROLL:
                default:
                    break;
            }

            if(update)
            {
                SetScrollPos(hWndScrollBar, SB_CTL, CurPos, TRUE);
                char text[5]; sprintf(text,"%d",CurPos);
                SetWindowText(hSprNumText,text);
                GLWindow_GBSprViewerUpdate();
            }
            break;
        }
        case WM_MOUSEWHEEL:
        {
            short zDelta = (short)HIWORD(wParam);
            int increment = (zDelta/WHEEL_DELTA);
            int CurPos = GetScrollPos(hWndScrollBar, SB_CTL) + increment;
            if( (CurPos >= 0) && (CurPos < 40) )
            {
                SetScrollPos(hWndScrollBar, SB_CTL, CurPos, TRUE);
                char text[5]; sprintf(text,"%d",CurPos);
                SetWindowText(hSprNumText,text);
                GLWindow_GBSprViewerUpdate();
            }
            break;
        }
        case WM_LBUTTONDOWN:
            SetFocus(hWndSprViewer);
            //no break;
        case WM_SETFOCUS:
            GLWindow_GBSprViewerUpdate();
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

void GLWindow_GBCreateSprViewer(void)
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
	WndClsEx.lpszClassName = "Class_GBSprView";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_GBSprView",
			  "Sprite Viewer",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  305,
			  175,
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

void GLWindow_GBCloseSprViewer(void)
{
    if(SprViewerCreated) SendMessage(hWndSprViewer, WM_CLOSE, 0, 0);
}

//-----------------------------------------------------------------------------------------
//                                   MAP VIEWER
//-----------------------------------------------------------------------------------------

static HWND hWndMapViewer;
static int MapViewerCreated = 0;
static int MapBuffer[256*256], MapTileBuffer[64*64];
static u32 TileX,TileY;
static HWND hMapText;
static int tile_sel, map_sel;

#define IDC_TILE8000    10
#define IDC_TILE8800    11
#define IDC_MAP9800     20
#define IDC_MAP9C00     21

static void gb_map_viewer_draw_to_buffer(void)
{
    u32 gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };

	_GB_MEMORY_ * mem = &GameBoy.Memory;
	u32 y, x;

	u8 * tiledata = tile_sel ? &mem->VideoRAM[0x0800] : &mem->VideoRAM[0x0000];
    u8 * bgtilemap = map_sel ? &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];

    if(GameBoy.Emulator.CGBEnabled)
    {
        for(y = 0; y < 256; y ++) for(x = 0; x < 256; x ++)
        {
			u32 tile_location = ( ((y>>3) & 31) * 32) + ((x >> 3) & 31);
			u32 tile = bgtilemap[tile_location];
			u32 tileinfo = bgtilemap[tile_location + 0x2000];

			if(tile_sel) //If tile base is 0x8800
            {
                if(tile & (1<<7)) tile &= 0x7F;
                else tile += 128;
            }

            u8 * data = &tiledata[(tile<<4) + ( (tileinfo&(1<<3)) ? 0x2000 : 0 )]; //Bank 1?

            //V FLIP
            if(tileinfo & (1<<6)) data += (((7-y)&7) * 2);
            else data += ((y&7) * 2);

            u32 x_;
            //H FLIP
			if(tileinfo & (1<<5)) x_ = (x&7);
            else x_ = 7-(x&7);

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);
            u32 pal_index = ((tileinfo&7) * 8) + (2*color);

            color = GameBoy.Emulator.bg_pal[pal_index] | (GameBoy.Emulator.bg_pal[pal_index+1]<<8);
            MapBuffer[256*y + x] = expand16to32(color);
        }
    }
    else
    {
        for(y = 0; y < 256; y ++) for(x = 0; x < 256; x ++)
        {
            u32 tile = bgtilemap[( ((y>>3) & 31) * 32) + ((x >> 3) & 31)];

            if(tile_sel) //If tile base is 0x8800
            {
                if(tile & (1<<7)) tile &= 0x7F;
                else tile += 128;
            }

            u8 * data = (&tiledata[tile<<4]) + ((y&7) << 1);

            u32 x_ = 7-(x&7);

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            MapBuffer[256*y + x] = (gb_pal_colors[color][0]<<16) | (gb_pal_colors[color][1]<<8) |
                gb_pal_colors[color][2];
        }
    }
}

static void gb_map_viewer_update_tile(void)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    //Update text information
    {
        u8 * bgtilemap = map_sel ? &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];
        u32 tile_location = ((TileY & 31) * 32) + (TileX & 31);
        int tile = bgtilemap[tile_location];
        int info = GameBoy.Emulator.CGBEnabled ? bgtilemap[tile_location + 0x2000] : 0;
        u32 addr = (map_sel?0x9C00:0x9800)+(TileY*32)+TileX;

        char text[512];
        sprintf(text,"Pos: %d,%d\r\nAddr: 0x%04X\r\nTile: %d (Bank %d)\r\nFlip: %s%s\r\nPal: %d\r\nPriority: %d",
            TileX,TileY,addr,tile,(info&(1<<3)) != 0,(info & (1<<6)) ? "V" : "-",
            (info & (1<<5)) ? "H" : "-",info&7,(info & (1<<7)) != 0);
        SetWindowText(hMapText,(LPCTSTR)text);
    }

    int tiletempbuffer[8*8];

    u32 gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };

	u8 * tiledata = tile_sel ? &mem->VideoRAM[0x0800] : &mem->VideoRAM[0x0000];
    u8 * bgtilemap = map_sel ? &mem->VideoRAM[0x1C00] : &mem->VideoRAM[0x1800];

    if(GameBoy.Emulator.CGBEnabled)
    {
        u32 tile_location = ( (TileY & 31) * 32) + (TileX & 31);
        u32 tile = bgtilemap[tile_location];
        u32 tileinfo = bgtilemap[tile_location + 0x2000];
        if(tile_sel) //If tile base is 0x8800
        {
            if(tile & (1<<7)) tile &= 0x7F;
            else tile += 128;
        }

        u32 y, x;
        for(y = 0; y < 8; y ++) for(x = 0; x < 8; x ++)
        {
            u8 * data = &tiledata[(tile<<4) + ( (tileinfo&(1<<3)) ? 0x2000 : 0 )]; //Bank 1?
            data += (y * 2);

            u32 x_ = 7-x;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);
            u32 pal_index = ((tileinfo&7) * 8) + (2*color);

            color = GameBoy.Emulator.bg_pal[pal_index] | (GameBoy.Emulator.bg_pal[pal_index+1]<<8);
            tiletempbuffer[8*y + x] = expand16to32(color);
        }
    }
    else
    {
        u32 tile_location = ( (TileY & 31) * 32) + (TileX & 31);
        u32 tile = bgtilemap[tile_location];
        if(tile_sel) //If tile base is 0x8800
        {
            if(tile & (1<<7)) tile &= 0x7F;
            else tile += 128;
        }

        u32 y, x;
        for(y = 0; y < 8; y ++) for(x = 0; x < 8; x ++)
        {
            u8 * data = (&tiledata[tile<<4]) + (y * 2);

            u32 x_ = 7-x;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            tiletempbuffer[8*y + x] = (gb_pal_colors[color][0]<<16) | (gb_pal_colors[color][1]<<8) |
                gb_pal_colors[color][2];
        }
    }

    //Expand to 64x64
    int i,j;
    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
    {
        MapTileBuffer[j*64+i] = tiletempbuffer[(j/8)*8 + (i/8)];
    }

    //Update window
    RECT rc; rc.top = 197; rc.left = 5; rc.bottom = 197+256; rc.right = 5+256;
    InvalidateRect(hWndMapViewer, &rc, FALSE);
}

static void gb_map_viewer_update(void)
{
    TileX = 0; TileY = 0;

    gb_map_viewer_draw_to_buffer();
    gb_map_viewer_update_tile();
}

void GLWindow_GBMapViewerUpdate(void)
{
    if(MapViewerCreated == 0) return;

    if(RUNNING != RUN_GB) return;

    gb_map_viewer_update();

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

            HWND hGroup1 = CreateWindow(TEXT("button"), TEXT("Tile Base"),
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                3,1, 150,40, hWnd, (HMENU) 0, hInstance, NULL);
            HWND hBtn1 = CreateWindow(TEXT("button"), TEXT("0x8000"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 18, 70, 18, hWnd, (HMENU)IDC_TILE8000 , hInstance, NULL);
            HWND hBtn2 = CreateWindow(TEXT("button"), TEXT("0x8800"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                80, 18, 70, 18, hWnd, (HMENU)IDC_TILE8800 , hInstance, NULL);

            HWND hGroup2 = CreateWindow(TEXT("button"), TEXT("Map Base"),
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                3,42, 150,40, hWnd, (HMENU) 0, hInstance, NULL);
            HWND hBtn3 = CreateWindow(TEXT("button"), TEXT("0x9800"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                8, 58, 70, 18, hWnd, (HMENU)IDC_MAP9800, hInstance, NULL);
            HWND hBtn4 = CreateWindow(TEXT("button"), TEXT("0x9C00"),
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                80, 58, 70, 18, hWnd, (HMENU)IDC_MAP9C00, hInstance, NULL);

            CheckRadioButton(hWnd,IDC_TILE8000,IDC_TILE8800,IDC_TILE8000);
            CheckRadioButton(hWnd,IDC_MAP9800,IDC_MAP9C00,IDC_MAP9800);

            TileX = 0; TileY = 0;
            tile_sel = 0; map_sel = 0;

            SendMessage(hGroup1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hGroup2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn3, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            SendMessage(hBtn4, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

            hMapText = CreateWindow(TEXT("edit"), TEXT(" "),
                        WS_CHILD | WS_VISIBLE | BS_CENTER | ES_MULTILINE | ES_READONLY,
                        5, 85, 150, 110, hWnd, NULL, hInstance, NULL);
            SendMessage(hMapText, WM_SETFONT, (WPARAM)hFontFixed, MAKELPARAM(1, 0));

            GLWindow_GBMapViewerUpdate();
            break;
        }
        case WM_COMMAND:
        {
            if(HIWORD(wParam) == BN_CLICKED)
            {
                switch(LOWORD(wParam))
                {
                    case IDC_TILE8000:
                    case IDC_TILE8800:
                        tile_sel = (LOWORD(wParam) == IDC_TILE8800);
                        CheckRadioButton(hWnd,IDC_TILE8000,IDC_TILE8800,LOWORD(wParam));
                        break;
                    case IDC_MAP9800:
                    case IDC_MAP9C00:
                        map_sel = (LOWORD(wParam) == IDC_MAP9C00);
                        CheckRadioButton(hWnd,IDC_MAP9800,IDC_MAP9C00,LOWORD(wParam));
                        break;
                    default: break;
                }
                GLWindow_GBMapViewerUpdate();
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
            BitBlt(hDC, 160, 5, 256, 256, MemDC, 0, 0, SRCCOPY);
            // Restore the old bitmap
            DeleteDC(MemDC);
            DeleteObject(bitmap);

            bitmap = CreateBitmap(64, 64, 1, 32, MapTileBuffer);
            // Create a memory device compatible with the above DC variable
            MemDC = CreateCompatibleDC(hDC);
            // Select the new bitmap
            SelectObject(MemDC, bitmap);
            // Copy the bits from the memory DC into the current dc
            BitBlt(hDC, 5, 197, 64, 64, MemDC, 0, 0, SRCCOPY);
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

            if( (x >= 160) && (x < (160+256)) && (y >= 5) && (y < (5+256)) )
            {
                TileX = (x-160)/8; TileY = (y-5)/8;
                gb_map_viewer_update_tile();
            }
            break;
        }
        case WM_SETFOCUS:
            GLWindow_GBMapViewerUpdate();
            break;
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

void GLWindow_GBCreateMapViewer(void)
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
	WndClsEx.lpszClassName = "Class_GBMapView";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_GBMapView",
			  "Map Viewer",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  427,
			  298,
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

void GLWindow_GBCloseMapViewer(void)
{
    if(MapViewerCreated) SendMessage(hWndMapViewer, WM_CLOSE, 0, 0);
}

//-----------------------------------------------------------------------------------------
//                                   TILE VIEWER
//-----------------------------------------------------------------------------------------

static HWND hWndTileViewer;
static int TileViewerCreated = 0;
static int TileBuffer[2][128*192], SelectedTileBuffer[64*64];
static u32 SelTileX,SelTileY, SelBank;
static HWND hTileText;

static void gb_tile_viewer_draw_to_buffer(void)
{
    memset(TileBuffer,0,sizeof(TileBuffer));

    _GB_MEMORY_ * mem = &GameBoy.Memory;
	u32 y, x;

	static const u32 gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };

	for(y = 0; y < 192 ; y++)
	{
		for(x = 0; x < 128; x++)
		{
			u32 tile = (x>>3) + ((y>>3)*16);

			u8 * data = &mem->VideoRAM[tile<<4]; //Bank 0

			data += ( (y&7)*2 );

			u32 x_ = 7-(x&7);

			u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            TileBuffer[0][x + y*128] = (gb_pal_colors[color][0]<<16)|(gb_pal_colors[color][1]<<8)|
                gb_pal_colors[color][2];
		}
	}

	for(y = 0; y < 192 ; y++)
	{
		for(x = 0; x < 128; x++)
		{
			u32 tile = (x>>3) + ((y>>3)*16);

			u8 * data = &mem->VideoRAM[tile<<4];
			data += 0x2000; //Bank 1;

			data += ( (y&7)*2 );

			u32 x_ = 7-(x&7);

			u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            TileBuffer[1][x + y*128] = (gb_pal_colors[color][0]<<16)|(gb_pal_colors[color][1]<<8)|
                gb_pal_colors[color][2];
		}
	}
}

static void gb_tile_viewer_update_tile(void)
{
    int tiletempbuffer[8*8];

    static const u32 gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };

    u8 * tile_data = &GameBoy.Memory.VideoRAM[(SelTileX + (SelTileY*16))<<4]; //Bank 0
    if(SelBank) tile_data += 0x2000; //Bank 1;

	u32 y, x;
	for(y = 0; y < 8 ; y++) for(x = 0; x < 8; x++)
    {
        u8 * data = tile_data + ( (y&7)*2 );
        u32 x_ = 7-(x&7);
        u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

        tiletempbuffer[x + y*8] = (gb_pal_colors[color][0]<<16)|(gb_pal_colors[color][1]<<8)|
                gb_pal_colors[color][2];
    }

    u32 tile = (SelTileX + (SelTileY*16));
    u32 tileindex = (tile > 255) ? (tile - 256) : (tile);
    if(GameBoy.Emulator.CGBEnabled)
    {
        char text[1000];
        sprintf(text,"Tile: %d(%d)\r\nAddr: 0x%04X\r\nBank: %d",tile,tileindex,
            0x8000 + (tile * 16),SelBank);
        SetWindowText(hTileText,(LPCTSTR)text);
    }
    else
    {
        char text[1000];
        sprintf(text,"Tile: %d(%d)\r\nAddr: 0x%04X\r\nBank: -",tile,tileindex,
            0x8000 + (tile * 16));
        SetWindowText(hTileText,(LPCTSTR)text);
    }

    //Expand to 64x64
    int i,j;
    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
    {
        SelectedTileBuffer[j*64+i] = tiletempbuffer[(j/8)*8 + (i/8)];
    }

    //Update window
    RECT rc; rc.top = 133; rc.left = 5; rc.bottom = 133+64; rc.right = 5+64;
    InvalidateRect(hWndTileViewer, &rc, FALSE);
}

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
        case WM_SETFOCUS:
            GLWindow_GBTileViewerUpdate();
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

void GLWindow_GBCreateTileViewer(void)
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
	WndClsEx.lpszClassName = "Class_GBTileView";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_GBTileView",
			  "Tile Viewer",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  407,
			  234,
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

void GLWindow_GBCloseTileViewer(void)
{
    if(TileViewerCreated) SendMessage(hWndTileViewer, WM_CLOSE, 0, 0);
}


