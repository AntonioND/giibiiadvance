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

#include "win_utils.h"

#include "win_main_config.h"
#include "win_main.h"

#include "../font_utils.h"
#include "../config.h"

//-----------------------------------------------------------------------------------

_gui_element mainwindow_configwin;

//-----------------------------------------------------------------------------------

static _gui_element mainwindow_configwin_close_btn;

//------------------------

static _gui_element mainwindow_configwin_general_groupbox;

static _gui_element mainwindow_configwin_general_filter_label;
static _gui_element mainwindow_configwin_general_filternearest_radbtn;
static _gui_element mainwindow_configwin_general_filterlinear_radbtn;
static _gui_element mainwindow_configwin_general_frameskip_label;
static _gui_element mainwindow_configwin_general_frameskip_scrollbar;
static _gui_element mainwindow_configwin_general_load_bootrom_checkbox;
static _gui_element mainwindow_configwin_general_scrnzoom_label;
static _gui_element mainwindow_configwin_general_scrnzoom2_radbtn;
static _gui_element mainwindow_configwin_general_scrnzoom3_radbtn;
static _gui_element mainwindow_configwin_general_scrnzoom4_radbtn;
static _gui_element mainwindow_configwin_general_autoclose_debugger_checkbox;
static _gui_element mainwindow_configwin_general_debug_messages_enabled_checkbox;

//------------------------

static _gui_element mainwindow_configwin_sound_groupbox;

static _gui_element mainwindow_configwin_sound_mute_checkbox;
static _gui_element mainwindow_configwin_sound_volume_label;
static _gui_element mainwindow_configwin_sound_volume_scrollbar;
static _gui_element mainwindow_configwin_sound_ch_en_label;
static _gui_element mainwindow_configwin_sound_ch_en1_checkbox;
static _gui_element mainwindow_configwin_sound_ch_en2_checkbox;
static _gui_element mainwindow_configwin_sound_ch_en3_checkbox;
static _gui_element mainwindow_configwin_sound_ch_en4_checkbox;
static _gui_element mainwindow_configwin_sound_ch_enA_checkbox;
static _gui_element mainwindow_configwin_sound_ch_enB_checkbox;

//------------------------

static _gui_element mainwindow_configwin_gameboy_groupbox;

//------------------------

static _gui_element mainwindow_configwin_gameboyadvance_groupbox;

//------------------------

static _gui_element * mainwindow_configwin_gui_elements[] = {
    &mainwindow_configwin_general_groupbox,
    &mainwindow_configwin_general_filter_label,
    &mainwindow_configwin_general_filternearest_radbtn, &mainwindow_configwin_general_filterlinear_radbtn,
    &mainwindow_configwin_general_load_bootrom_checkbox,
    &mainwindow_configwin_general_scrnzoom_label, &mainwindow_configwin_general_scrnzoom2_radbtn,
    &mainwindow_configwin_general_scrnzoom3_radbtn, &mainwindow_configwin_general_scrnzoom4_radbtn,
    &mainwindow_configwin_general_autoclose_debugger_checkbox,
    &mainwindow_configwin_general_debug_messages_enabled_checkbox,
    &mainwindow_configwin_general_frameskip_label, &mainwindow_configwin_general_frameskip_scrollbar,

    &mainwindow_configwin_sound_groupbox,
    &mainwindow_configwin_sound_mute_checkbox,
    &mainwindow_configwin_sound_volume_label, &mainwindow_configwin_sound_volume_scrollbar,
    &mainwindow_configwin_sound_ch_en_label,
    &mainwindow_configwin_sound_ch_en1_checkbox, &mainwindow_configwin_sound_ch_en2_checkbox,
    &mainwindow_configwin_sound_ch_en3_checkbox, &mainwindow_configwin_sound_ch_en4_checkbox,
    &mainwindow_configwin_sound_ch_enA_checkbox, &mainwindow_configwin_sound_ch_enB_checkbox,

    &mainwindow_configwin_gameboy_groupbox,

    &mainwindow_configwin_gameboyadvance_groupbox,

    &mainwindow_configwin_close_btn,

    NULL
};

//-----------------------------------------------------------------------------------

static _gui mainwindow_subwindow_config_gui = {
    mainwindow_configwin_gui_elements,
    NULL,
    NULL
};

//-----------------------------------------------------------------------------------

static void _win_main_config_frameskip_scrollbar_callback(int value)
{
    EmulatorConfig.frameskip = value;
    Win_MainSetFrameskip(value);
    return;
}

static void _win_main_config_filter_callback(int num)
{
    EmulatorConfig.oglfilter = num;

    if(EmulatorConfig.oglfilter == 1)
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    else
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
}

static void _win_main_config_load_boot_rom_callback(int checked)
{
    EmulatorConfig.load_from_boot_rom = checked;
}

static void _win_main_config_screen_zoom_radbtn_callback(int num)
{
    EmulatorConfig.screen_size = num;
    Win_MainChangeZoom(EmulatorConfig.screen_size);
}

static void _win_main_config_autoclose_debugger_callback(int checked)
{
    EmulatorConfig.auto_close_debugger = checked;
}

static void _win_main_config_debug_msg_enabled_callback(int checked)
{
    EmulatorConfig.debug_msg_enable = checked;
}

static void _win_main_config_mute_snd_callback(int checked)
{
    EmulatorConfig.snd_mute = checked;
}

static void _win_main_config_volume_scrollbar_callback(int value)
{
    EmulatorConfig.volume = value;
    return;
}

static void _win_main_config_ch1en_callback(int value)
{
    if(value) EmulatorConfig.chn_flags |= 1;
    else EmulatorConfig.chn_flags &= ~1;
}

static void _win_main_config_ch2en_callback(int value)
{
    if(value) EmulatorConfig.chn_flags |= 2;
    else EmulatorConfig.chn_flags &= ~2;
}

static void _win_main_config_ch3en_callback(int value)
{
    if(value) EmulatorConfig.chn_flags |= 4;
    else EmulatorConfig.chn_flags &= ~4;
}

static void _win_main_config_ch4en_callback(int value)
{
    if(value) EmulatorConfig.chn_flags |= 8;
    else EmulatorConfig.chn_flags &= ~8;
}

static void _win_main_config_chAen_callback(int value)
{
    if(value) EmulatorConfig.chn_flags |= 16;
    else EmulatorConfig.chn_flags &= ~16;
}

static void _win_main_config_chBen_callback(int value)
{
    if(value) EmulatorConfig.chn_flags |= 32;
    else EmulatorConfig.chn_flags &= ~32;
}

//-----------------------------------------------------------------------------------

void Win_MainCreateConfigWindow(void)
{
    //-----------------------------

    GUI_SetGroupBox(&mainwindow_configwin_general_groupbox,6,6,222,183,"General");

    GUI_SetLabel(&mainwindow_configwin_general_filter_label,12,24,-1,18,"Filter:");
    GUI_SetRadioButton(&mainwindow_configwin_general_filternearest_radbtn,  12+9*FONT_WIDTH,24,9*FONT_WIDTH,18,
                  "Nearest", 0, 0, 1,_win_main_config_filter_callback);
    GUI_SetRadioButton(&mainwindow_configwin_general_filterlinear_radbtn,  12+19*FONT_WIDTH,24,9*FONT_WIDTH,18,
                  "Linear",  0, 1, 0,_win_main_config_filter_callback);

    GUI_SetLabel(&mainwindow_configwin_general_scrnzoom_label,12,48,-1,18,"Screen zoom:");
    GUI_SetRadioButton(&mainwindow_configwin_general_scrnzoom2_radbtn,  12+12*FONT_WIDTH+6,48, 4*FONT_WIDTH,18,
                  "x2", 1, 2, EmulatorConfig.screen_size==2,_win_main_config_screen_zoom_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_general_scrnzoom3_radbtn,  12+16*FONT_WIDTH+12,48,4*FONT_WIDTH,18,
                  "x3", 1, 3, EmulatorConfig.screen_size==3,_win_main_config_screen_zoom_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_general_scrnzoom4_radbtn,  12+20*FONT_WIDTH+18,48,4*FONT_WIDTH,18,
                  "x4", 1, 4, EmulatorConfig.screen_size==4,_win_main_config_screen_zoom_radbtn_callback);

    GUI_SetCheckBox(&mainwindow_configwin_general_load_bootrom_checkbox,12,72,-1,12,"Load from boot ROM",
                        EmulatorConfig.load_from_boot_rom, _win_main_config_load_boot_rom_callback);

    GUI_SetCheckBox(&mainwindow_configwin_general_autoclose_debugger_checkbox,12,90,-1,12,"Auto close debugger",
                        EmulatorConfig.auto_close_debugger, _win_main_config_autoclose_debugger_callback);

    GUI_SetCheckBox(&mainwindow_configwin_general_debug_messages_enabled_checkbox,12,108,-1,12,"Debug messages enabled",
                        EmulatorConfig.debug_msg_enable, _win_main_config_debug_msg_enabled_callback);






    GUI_SetLabel(&mainwindow_configwin_general_frameskip_label,12,150,-1,FONT_HEIGHT,"Frameskip: Auto");

    GUI_SetScrollBar(&mainwindow_configwin_general_frameskip_scrollbar,12,162,100,12,
                     0,9,EmulatorConfig.frameskip,_win_main_config_frameskip_scrollbar_callback);
    //-----------------------------

    GUI_SetGroupBox(&mainwindow_configwin_sound_groupbox,234,6,222,183,"Sound");

    GUI_SetCheckBox(&mainwindow_configwin_sound_mute_checkbox,240,24,-1,12,"Mute sound",
                        EmulatorConfig.snd_mute, _win_main_config_mute_snd_callback);

    GUI_SetLabel(&mainwindow_configwin_sound_volume_label,240,42,-1,FONT_HEIGHT,"Volume:");
    GUI_SetScrollBar(&mainwindow_configwin_sound_volume_scrollbar,240,60,210,12,
                     0,128,EmulatorConfig.volume,_win_main_config_volume_scrollbar_callback);

    GUI_SetLabel(&mainwindow_configwin_sound_ch_en_label,240,78,-1,FONT_HEIGHT,"Channels enabled:");
    GUI_SetCheckBox(&mainwindow_configwin_sound_ch_en1_checkbox,240,96,-1,12,"PSG 1",
                        EmulatorConfig.chn_flags&1, _win_main_config_ch1en_callback);
    GUI_SetCheckBox(&mainwindow_configwin_sound_ch_en2_checkbox,240,114,-1,12,"PSG 2",
                        EmulatorConfig.chn_flags&2, _win_main_config_ch2en_callback);
    GUI_SetCheckBox(&mainwindow_configwin_sound_ch_en3_checkbox,240+70,96,-1,12,"PSG 3",
                        EmulatorConfig.chn_flags&4, _win_main_config_ch3en_callback);
    GUI_SetCheckBox(&mainwindow_configwin_sound_ch_en4_checkbox,240+70,114,-1,12,"PSG 4",
                        EmulatorConfig.chn_flags&8, _win_main_config_ch4en_callback);
    GUI_SetCheckBox(&mainwindow_configwin_sound_ch_enA_checkbox,240+140,96,-1,12,"Fifo A",
                        EmulatorConfig.chn_flags&16, _win_main_config_chAen_callback);
    GUI_SetCheckBox(&mainwindow_configwin_sound_ch_enB_checkbox,240+140,114,-1,12,"Fifo B",
                        EmulatorConfig.chn_flags&32, _win_main_config_chBen_callback);

    //-----------------------------

    GUI_SetGroupBox(&mainwindow_configwin_gameboy_groupbox,6,195,222,183,"Game Boy");

    //-----------------------------

    GUI_SetGroupBox(&mainwindow_configwin_gameboyadvance_groupbox,234,195,222,183,"Game Boy Advance");

    //-----------------------------

    GUI_SetButton(&mainwindow_configwin_close_btn, 300,300,7*FONT_WIDTH,2*FONT_HEIGHT,"Close",
                  Win_MainCloseConfigWindow);

    GUI_SetWindow(&mainwindow_configwin,25,25,256*2-50,224*2-50,&mainwindow_subwindow_config_gui,
                  "Configuration");

    GUI_WindowSetEnabled(&mainwindow_configwin,0);
}

void Win_MainOpenConfigWindow(void)
{
    GUI_WindowSetEnabled(&mainwindow_configwin,1);
}

void Win_MainCloseConfigWindow(void)
{
    GUI_WindowSetEnabled(&mainwindow_configwin,0);
    Config_Save(); //Save to GiiBiiAdvance.ini
}

//-----------------------------------------------------------------------------------

#if 0

void GLWindow_ConfigExit(void)
{
    int aux;

    //GENERAL
    EmulatorConfig.frameskip = GET_SELECTION(hCtrlFrameskip)-1;

    //GAMEBOY
    EmulatorConfig.hardware_type = GET_SELECTION(hCtrlGBHardType) - 1;

    EmulatorConfig.serial_device = GET_SELECTION(hCtrlGBSerialDevice);
    GB_SerialPlug(EmulatorConfig.serial_device);

    EmulatorConfig.enableblur = IS_CHECKED(hCheckGBBlur);
    GB_EnableBlur(EmulatorConfig.enableblur);

    EmulatorConfig.realcolors = IS_CHECKED(hCheckGBRealColors);
    GB_EnableRealColors(EmulatorConfig.realcolors);

    //gb palette is changed in WM_LBUTTONDOWN

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

}

static LRESULT CALLBACK ConfigurationProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
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
}

#endif // 0
