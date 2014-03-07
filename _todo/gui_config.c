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

#include "build_options.h"
#include "resource.h"

#include "gui_main.h"
#include "gui_mainloop.h"
#include "gui_config.h"
#include "main.h"
#include "config.h"
#include "gb_core/gb_main.h"
#include "gb_core/gameboy.h"
#include "gb_core/serial.h"
#include "gb_core/video.h"

extern _GB_CONTEXT_ GameBoy;

//---------------------------------------------------

static HWND hWndConfig;
static int ConfigCreated = 0;

//---------------

//General
static HWND hCtrlFrameskip;
static HWND hCtrlZoom;
static HWND hCheckDebug;
static HWND hCheckLoadBoot;
static HWND hCheckTimer;
static HWND hCtrlOglFilter;
static HWND hCheckAutoClose;

//Sound
static HWND hCtrlSndBufLen;
static HWND hCheckChn[6];
static HWND hScrollVolume;
static HWND hCheckSndMute;

//Gameboy
static HWND hCtrlGBHardType;
static HWND hCtrlGBSerialDevice;
static HWND hCheckGBBlur;
static HWND hCheckGBRealColors;

//Gameboy Advance

//---------------

//FOR CHECKBOXES
#define SET_CHECK(hwnd,condition) SendMessage(hwnd, BM_SETCHECK, (WPARAM)((condition)?BST_CHECKED:BST_UNCHECKED), 0)
#define IS_CHECKED(hwnd) (SendMessage(hwnd, BM_GETCHECK, (WPARAM)0, (LPARAM)0)==BST_CHECKED)

//FOR COMBOBOXES
#define GET_SELECTION(hwnd) SendMessage(hwnd, CB_GETCURSEL, 0, 0)

//---------------

void GLWindow_ConfigExit(void)
{
    int showwarning = 0;

    int aux;

    //GENERAL
    EmulatorConfig.frameskip = GET_SELECTION(hCtrlFrameskip)-1;

    aux = GET_SELECTION(hCtrlZoom)+1;
    if(EmulatorConfig.screen_size != aux)
    {
        GLWindow_SetZoom(aux);
        if(RUNNING == RUN_GBA) GLWindow_ChangeScreen(SCR_GBA);
        else if(RUNNING == RUN_GB)
        {
            if(GameBoy.Emulator.SGBEnabled) GLWindow_ChangeScreen(SCR_SGB);
            else GLWindow_ChangeScreen(SCR_GB);
        }
    }
    EmulatorConfig.screen_size = aux;

    EmulatorConfig.debug_msg_enable = IS_CHECKED(hCheckDebug);

    EmulatorConfig.load_from_boot_rom = IS_CHECKED(hCheckLoadBoot);

    aux = IS_CHECKED(hCheckTimer);
    if(EmulatorConfig.highperformancetimer != aux) showwarning = 1;
    EmulatorConfig.highperformancetimer = aux;

    EmulatorConfig.oglfilter = GET_SELECTION(hCtrlOglFilter);

    EmulatorConfig.auto_close_debugger = IS_CHECKED(hCheckAutoClose);

    //SOUND
    aux = (GET_SELECTION(hCtrlSndBufLen)+1)*50;
    if(EmulatorConfig.server_buffer_len != aux) showwarning = 1;
    EmulatorConfig.server_buffer_len = aux;

    aux = 0;
    if(IS_CHECKED(hCheckChn[0])) aux |= BIT(0);
    if(IS_CHECKED(hCheckChn[1])) aux |= BIT(1);
    if(IS_CHECKED(hCheckChn[2])) aux |= BIT(2);
    if(IS_CHECKED(hCheckChn[3])) aux |= BIT(3);
    if(IS_CHECKED(hCheckChn[4])) aux |= BIT(4);
    if(IS_CHECKED(hCheckChn[5])) aux |= BIT(5);
    EmulatorConfig.chn_flags = aux;

    EmulatorConfig.volume = GetScrollPos(hScrollVolume, SB_CTL);

    //EmulatorConfig.snd_mute = IS_CHECKED(hCheckSndMute); //done by GLWindow_SetMute
    GLWindow_SetMute(IS_CHECKED(hCheckSndMute));

    //GAMEBOY
    EmulatorConfig.hardware_type = GET_SELECTION(hCtrlGBHardType) - 1;

    EmulatorConfig.serial_device = GET_SELECTION(hCtrlGBSerialDevice);
    GB_SerialPlug(EmulatorConfig.serial_device);

    EmulatorConfig.enableblur = IS_CHECKED(hCheckGBBlur);
    GB_EnableBlur(EmulatorConfig.enableblur);

    EmulatorConfig.realcolors = IS_CHECKED(hCheckGBRealColors);
    GB_EnableRealColors(EmulatorConfig.realcolors);

    //gb palette is changed in WM_LBUTTONDOWN

    //GAMEBOY ADVANCE

    //END

    if(showwarning)
    {
        MessageBox(hWndMain, "Please, close GiiBiiAdvance for all the parameters "
                "to be set to their new values.", "Configuration", MB_OK|MB_ICONINFORMATION);
    }

    Config_Save(); //Save to GiiBiiAdvance.ini
}

void GLWindow_ConfigurationCreateDialog(HWND hWnd, HFONT hFont)
{
    HWND hWndAux;

    //GENERAL

    hWndAux = CreateWindow(TEXT("button"), TEXT("General"),
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            5, 5, 170, 190, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("Frameskip:"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            10,24, 65,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    hCtrlFrameskip = CreateWindowEx(0,TEXT("combobox"),NULL,
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
        85,22, 60,200,  hWnd, (HMENU)0, hInstance,NULL);
    SendMessage(hCtrlFrameskip, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"Auto");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"0");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"1");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"2");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"3");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"4");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"5");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"6");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"7");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"8");
    SendMessage(hCtrlFrameskip, CB_ADDSTRING, 0, (LPARAM)"9");
    SendMessage(hCtrlFrameskip, CB_SETCURSEL, (WPARAM)EmulatorConfig.frameskip+1, 0);
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("Screen Zoom:"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            10,48, 80,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    hCtrlZoom = CreateWindowEx(0,TEXT("combobox"),NULL,
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
        100,46, 40,85,  hWnd, (HMENU)0, hInstance,NULL);
    SendMessage(hCtrlZoom, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SendMessage(hCtrlZoom, CB_ADDSTRING, 0, (LPARAM)"x1");
    SendMessage(hCtrlZoom, CB_ADDSTRING, 0, (LPARAM)"x2");
    SendMessage(hCtrlZoom, CB_ADDSTRING, 0, (LPARAM)"x3");
    SendMessage(hCtrlZoom, CB_SETCURSEL, (WPARAM)EmulatorConfig.screen_size-1, 0);
    //-------------------
    hCheckDebug = CreateWindow(TEXT("button"), TEXT("Show Debug Messages"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        10,72, 150,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckDebug, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckDebug, EmulatorConfig.debug_msg_enable);
    //-------------------
    hCheckLoadBoot = CreateWindow(TEXT("button"), TEXT("Start from Boot ROM"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        10,96, 140,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckLoadBoot, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckLoadBoot, EmulatorConfig.load_from_boot_rom);
    //-------------------
    hCheckTimer = CreateWindow(TEXT("button"), TEXT("High Performance Timer"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        10,120, 160,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckTimer, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckTimer, EmulatorConfig.highperformancetimer);
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("Opengl Filter:"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            10,144, 75,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    hCtrlOglFilter = CreateWindowEx(0,TEXT("combobox"),NULL,
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
        90,142, 80,85,  hWnd, (HMENU)0, hInstance,NULL);
    SendMessage(hCtrlOglFilter, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SendMessage(hCtrlOglFilter, CB_ADDSTRING, 0, (LPARAM)"Nearest");
    SendMessage(hCtrlOglFilter, CB_ADDSTRING, 0, (LPARAM)"Linear");
    SendMessage(hCtrlOglFilter, CB_SETCURSEL, (WPARAM)EmulatorConfig.oglfilter, 0);
    //-------------------
    hCheckAutoClose = CreateWindow(TEXT("button"), TEXT("Auto Close Debugger"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        10,168, 160,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckAutoClose, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckAutoClose, EmulatorConfig.auto_close_debugger);

    //SOUND

    hWndAux = CreateWindow(TEXT("button"), TEXT("Sound"),
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            180, 5, 170, 190, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("Buffer lenght (ms):"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            185,24, 100,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    hCtrlSndBufLen = CreateWindowEx(0,TEXT("combobox"),NULL,
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
        290,22, 50,120,  hWnd, (HMENU)0, hInstance,NULL);
    SendMessage(hCtrlSndBufLen, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SendMessage(hCtrlSndBufLen, CB_ADDSTRING, 0, (LPARAM)"50");
    SendMessage(hCtrlSndBufLen, CB_ADDSTRING, 0, (LPARAM)"100");
    SendMessage(hCtrlSndBufLen, CB_ADDSTRING, 0, (LPARAM)"150");
    SendMessage(hCtrlSndBufLen, CB_ADDSTRING, 0, (LPARAM)"200");
    SendMessage(hCtrlSndBufLen, CB_ADDSTRING, 0, (LPARAM)"250");
    SendMessage(hCtrlSndBufLen, CB_SETCURSEL, (WPARAM)(EmulatorConfig.server_buffer_len/50)-1, 0);
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("Channel Enable:"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            185,72, 100,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    hCheckChn[0] = CreateWindow(TEXT("button"), TEXT("Ch1"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        185,96, 45,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckChn[0], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckChn[0], EmulatorConfig.chn_flags&BIT(0));
    hCheckChn[1] = CreateWindow(TEXT("button"), TEXT("Ch2"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        185,120, 45,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckChn[1], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckChn[1], EmulatorConfig.chn_flags&BIT(1));
    hCheckChn[2] = CreateWindow(TEXT("button"), TEXT("Ch3"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        235,96, 45,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckChn[2], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckChn[2], EmulatorConfig.chn_flags&BIT(2));
    hCheckChn[3] = CreateWindow(TEXT("button"), TEXT("Ch4"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        235,120, 45,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckChn[3], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckChn[3], EmulatorConfig.chn_flags&BIT(3));
    hCheckChn[4] = CreateWindow(TEXT("button"), TEXT("Fifo A"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        290,96, 50,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckChn[4], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckChn[4], EmulatorConfig.chn_flags&BIT(4));
    hCheckChn[5] = CreateWindow(TEXT("button"), TEXT("Fifo B"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        290,120, 50,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckChn[5], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckChn[5], EmulatorConfig.chn_flags&BIT(5));
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("Volume:"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            185,48, 50,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    hScrollVolume = CreateWindowEx(0,TEXT("scrollbar"),NULL,WS_CHILD | WS_VISIBLE | SBS_HORZ,
                240,48,  100,15,   hWnd,NULL,hInstance,NULL);
    SCROLLINFO si; ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
    si.nMin = 0; si.nMax = 0x80; si.nPos = EmulatorConfig.volume; si.nPage = 1;
    SetScrollInfo(hScrollVolume, SB_CTL, &si, TRUE);
    //-------------------
    hCheckSndMute = CreateWindow(TEXT("button"), TEXT("Mute Sound"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        185,144, 100,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckSndMute, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckSndMute, EmulatorConfig.snd_mute);

    //GAMEBOY

    hWndAux = CreateWindow(TEXT("button"), TEXT("Gameboy"),
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            5, 195, 170, 140, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("Hardware type:"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            10,214, 85,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    hCtrlGBHardType = CreateWindowEx(0,TEXT("combobox"),NULL,
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
        100,212, 70,160,  hWnd, (HMENU)0, hInstance,NULL);
    SendMessage(hCtrlGBHardType, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SendMessage(hCtrlGBHardType, CB_ADDSTRING, 0, (LPARAM)"Auto");
    SendMessage(hCtrlGBHardType, CB_ADDSTRING, 0, (LPARAM)"GB");
    SendMessage(hCtrlGBHardType, CB_ADDSTRING, 0, (LPARAM)"GBP");
    SendMessage(hCtrlGBHardType, CB_ADDSTRING, 0, (LPARAM)"SGB");
    SendMessage(hCtrlGBHardType, CB_ADDSTRING, 0, (LPARAM)"SGB2");
    SendMessage(hCtrlGBHardType, CB_ADDSTRING, 0, (LPARAM)"GBC");
    SendMessage(hCtrlGBHardType, CB_ADDSTRING, 0, (LPARAM)"GBA");
    SendMessage(hCtrlGBHardType, CB_SETCURSEL, (WPARAM)EmulatorConfig.hardware_type+1, 0);
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("Serial device:"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            10,238, 75,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    hCtrlGBSerialDevice = CreateWindowEx(0,TEXT("combobox"),NULL,
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
        90,236, 80,160,  hWnd, (HMENU)0, hInstance,NULL);
    SendMessage(hCtrlGBSerialDevice, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SendMessage(hCtrlGBSerialDevice, CB_ADDSTRING, 0, (LPARAM)"None");
    SendMessage(hCtrlGBSerialDevice, CB_ADDSTRING, 0, (LPARAM)"GBPrinter");
    //SendMessage(hCtrlGBSerialDevice, CB_ADDSTRING, 0, (LPARAM)"Gameboy");
    SendMessage(hCtrlGBSerialDevice, CB_SETCURSEL, (WPARAM)EmulatorConfig.serial_device, 0);
    //-------------------
    hCheckGBBlur = CreateWindow(TEXT("button"), TEXT("Enable Blur"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        10,262, 100,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckGBBlur, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckGBBlur, EmulatorConfig.enableblur);
    //-------------------
    hCheckGBRealColors = CreateWindow(TEXT("button"), TEXT("Real Colors"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        10,286, 100,17, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hCheckGBRealColors, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    SET_CHECK(hCheckGBRealColors, EmulatorConfig.realcolors);
    //-------------------
    hWndAux = CreateWindow(TEXT("static"), TEXT("GB Palette:"),
            WS_CHILD | WS_VISIBLE | BS_CENTER,
            10,308, 60,17, hWnd, NULL, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));

    //GAMEBOY ADVANCE

    hWndAux = CreateWindow(TEXT("button"), TEXT("Gameboy Advance"),
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            180, 195, 170, 140, hWnd, (HMENU)0, hInstance, NULL);
    SendMessage(hWndAux, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
    //-------------------

    //END
}

static LRESULT CALLBACK ConfigurationProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont;

    switch(Msg)
    {
        case WM_CREATE:
        {
            hFont = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, DEFAULT_PITCH, NULL);
            GLWindow_ConfigurationCreateDialog(hWnd,hFont);
            break;
        }
        case WM_HSCROLL:
        {
            int CurPos = GetScrollPos(hScrollVolume, SB_CTL);
            switch (LOWORD(wParam))
            {
                case SB_LEFT: CurPos = 0; break;
                case SB_LINELEFT: if(CurPos > 0) CurPos--; break;
                case SB_PAGELEFT: if(CurPos >= 0x10) { CurPos-=0x10; break; }
                                  if(CurPos > 0) { CurPos = 0; } break;
                case SB_THUMBPOSITION: CurPos = HIWORD(wParam); break;
                case SB_THUMBTRACK: CurPos = HIWORD(wParam); break;
                case SB_PAGERIGHT: if(CurPos < 0x70) { CurPos+=0x10; break; }
                                   if(CurPos < 0x80) { CurPos = 0x80; } break;
                case SB_LINERIGHT: if(CurPos < 0x80) { CurPos++; } break;
                case SB_RIGHT: CurPos = 0x80; break;
                case SB_ENDSCROLL:
                default:
                    break;
            }
            SetScrollPos(hScrollVolume, SB_CTL, CurPos, TRUE);
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT re;
            HDC hDC = BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &re);
            HPEN hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(192,192,192));
            SelectObject(hDC, hPen);
            u8 r,g,b;
            menu_get_gb_palette(&r,&g,&b);
            HBRUSH hBrush = (HBRUSH)CreateSolidBrush( r|(g<<8)|(b<<16) );
            SelectObject(hDC, hBrush);
            Rectangle(hDC, 80, 309, 80+85, 309+18);
            DeleteObject(hBrush);
            DeleteObject(hPen);
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            int xpos = LOWORD(lParam); // mouse xpos
            int ypos = HIWORD(lParam); // mouse ypos
            if( (xpos >= 80) && (xpos <= (80+85)) ) if( (ypos >= 309) && (ypos <= (309+18)) )
            {
                u8 r,g,b; menu_get_gb_palette(&r,&g,&b);
                u32 currcolor = ((u32)r) | (((u32)g)<<8) | (((u32)b)<<16);

                CHOOSECOLOR cc;
                static COLORREF acrCustClr[16];
                acrCustClr[0] = 0x00B0FFB0;
                // Initialize CHOOSECOLOR
                ZeroMemory(&cc, sizeof(cc));
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = hWnd;
                cc.lpCustColors = (LPDWORD)acrCustClr;
                cc.rgbResult = currcolor;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_SOLIDCOLOR;
                if(ChooseColor(&cc)==TRUE)
                {
                    menu_set_gb_palette(cc.rgbResult&0xFF, (cc.rgbResult>>8)&0xFF,
                        (cc.rgbResult>>16)&0xFF);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            break;
        }
        case WM_DESTROY:
            GLWindow_ConfigExit();
            ConfigCreated = 0;
            DeleteObject(hFont);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

void GLWindow_Configuration(void)
{
    if(ConfigCreated) { SetActiveWindow(hWndConfig); return; }
    ConfigCreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = ConfigurationProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_Configuration";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_Configuration",
			  "Configuration",
			  WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  360,
			  370,
			  hWndMain,
			  NULL,
			  hInstance,
			  NULL);

	if(!hWnd)
	{
	    ConfigCreated = 0;
	    return;
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndConfig = hWnd;
}
