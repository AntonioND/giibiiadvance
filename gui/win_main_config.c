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

#include "../gb_core/gameboy.h"
#include "../gb_core/serial.h"
#include "../gb_core/video.h"

//-----------------------------------------------------------------------------------

_gui_element mainwindow_configwin;

//-----------------------------------------------------------------------------------

static _gui_element mainwindow_configwin_close_btn;

//------------------------

static _gui_element mainwindow_configwin_general_groupbox;

static _gui_element mainwindow_configwin_general_filter_label;
static _gui_element mainwindow_configwin_general_filternearest_radbtn,
                    mainwindow_configwin_general_filterlinear_radbtn;
static _gui_element mainwindow_configwin_general_frameskip_label;
static _gui_element mainwindow_configwin_gameboy_frameskip_auto_radbtn,
                    mainwindow_configwin_gameboy_frameskip_0_radbtn,
                    mainwindow_configwin_gameboy_frameskip_1_radbtn,
                    mainwindow_configwin_gameboy_frameskip_2_radbtn,
                    mainwindow_configwin_gameboy_frameskip_3_radbtn,
                    mainwindow_configwin_gameboy_frameskip_4_radbtn,
                    mainwindow_configwin_gameboy_frameskip_5_radbtn;
static _gui_element mainwindow_configwin_general_load_bootrom_checkbox;
static _gui_element mainwindow_configwin_general_scrnzoom_label;
static _gui_element mainwindow_configwin_general_scrnzoom2_radbtn,
                    mainwindow_configwin_general_scrnzoom3_radbtn,
                    mainwindow_configwin_general_scrnzoom4_radbtn;
static _gui_element mainwindow_configwin_general_autoclose_debugger_checkbox;
static _gui_element mainwindow_configwin_general_debug_messages_enabled_checkbox;

//------------------------

static _gui_element mainwindow_configwin_sound_groupbox;

static _gui_element mainwindow_configwin_sound_mute_checkbox;
static _gui_element mainwindow_configwin_sound_volume_label;
static _gui_element mainwindow_configwin_sound_volume_scrollbar;
static _gui_element mainwindow_configwin_sound_ch_en_label;
static _gui_element mainwindow_configwin_sound_ch_en1_checkbox, mainwindow_configwin_sound_ch_en2_checkbox,
                    mainwindow_configwin_sound_ch_en3_checkbox, mainwindow_configwin_sound_ch_en4_checkbox,
                    mainwindow_configwin_sound_ch_enA_checkbox, mainwindow_configwin_sound_ch_enB_checkbox;

//------------------------

static _gui_element mainwindow_configwin_gameboy_groupbox;
static _gui_element mainwindow_configwin_gameboy_hard_type_label;
static _gui_element mainwindow_configwin_gameboy_hard_type_auto_radbtn,
                    mainwindow_configwin_gameboy_hard_type_gb_radbtn,
                    mainwindow_configwin_gameboy_hard_type_gbp_radbtn,
                    mainwindow_configwin_gameboy_hard_type_sgb_radbtn,
                    mainwindow_configwin_gameboy_hard_type_sgb2_radbtn,
                    mainwindow_configwin_gameboy_hard_type_gbc_radbtn,
                    mainwindow_configwin_gameboy_hard_type_gba_radbtn;
static _gui_element mainwindow_configwin_gameboy_serial_device_label;
static _gui_element mainwindow_configwin_gameboy_serial_device_none_radbtn,
                    mainwindow_configwin_gameboy_serial_device_gbprinter_radbtn,
                    mainwindow_configwin_gameboy_serial_device_gameboy_radbtn;
static _gui_element mainwindow_configwin_gameboy_enableblur_checkbox;
static _gui_element mainwindow_configwin_gameboy_realcolors_checkbox;
static _gui_element mainwindow_configwin_gameboy_palette_label;
static _gui_element mainwindow_configwin_gameboy_pal_r_scrollbar,
                    mainwindow_configwin_gameboy_pal_g_scrollbar,
                    mainwindow_configwin_gameboy_pal_b_scrollbar;
static _gui_element mainwindow_configwin_gameboy_palette_bitmap;

static char gameboy_palette[32*32*3];

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
    &mainwindow_configwin_general_frameskip_label,
    &mainwindow_configwin_gameboy_frameskip_auto_radbtn, &mainwindow_configwin_gameboy_frameskip_0_radbtn,
    &mainwindow_configwin_gameboy_frameskip_1_radbtn, &mainwindow_configwin_gameboy_frameskip_2_radbtn,
    &mainwindow_configwin_gameboy_frameskip_3_radbtn, &mainwindow_configwin_gameboy_frameskip_4_radbtn,
    &mainwindow_configwin_gameboy_frameskip_5_radbtn,

    &mainwindow_configwin_sound_groupbox,
    &mainwindow_configwin_sound_mute_checkbox,
    &mainwindow_configwin_sound_volume_label, &mainwindow_configwin_sound_volume_scrollbar,
    &mainwindow_configwin_sound_ch_en_label,
    &mainwindow_configwin_sound_ch_en1_checkbox, &mainwindow_configwin_sound_ch_en2_checkbox,
    &mainwindow_configwin_sound_ch_en3_checkbox, &mainwindow_configwin_sound_ch_en4_checkbox,
    &mainwindow_configwin_sound_ch_enA_checkbox, &mainwindow_configwin_sound_ch_enB_checkbox,

    &mainwindow_configwin_gameboy_groupbox,
    &mainwindow_configwin_gameboy_hard_type_label,
    &mainwindow_configwin_gameboy_hard_type_auto_radbtn, &mainwindow_configwin_gameboy_hard_type_gb_radbtn,
    &mainwindow_configwin_gameboy_hard_type_gbp_radbtn, &mainwindow_configwin_gameboy_hard_type_sgb_radbtn,
    &mainwindow_configwin_gameboy_hard_type_sgb2_radbtn, &mainwindow_configwin_gameboy_hard_type_gbc_radbtn,
    &mainwindow_configwin_gameboy_hard_type_gba_radbtn,
    &mainwindow_configwin_gameboy_serial_device_label, &mainwindow_configwin_gameboy_serial_device_none_radbtn,
    &mainwindow_configwin_gameboy_serial_device_gbprinter_radbtn, &mainwindow_configwin_gameboy_serial_device_gameboy_radbtn,
    &mainwindow_configwin_gameboy_enableblur_checkbox,
    &mainwindow_configwin_gameboy_realcolors_checkbox,
    &mainwindow_configwin_gameboy_palette_label,
    &mainwindow_configwin_gameboy_pal_r_scrollbar, &mainwindow_configwin_gameboy_pal_g_scrollbar,
    &mainwindow_configwin_gameboy_pal_b_scrollbar, &mainwindow_configwin_gameboy_palette_bitmap,

    &mainwindow_configwin_gameboyadvance_groupbox,

    &mainwindow_configwin_close_btn,

    NULL
};

//-----------------------------------------------------------------------------------

static _gui mainwindow_subwindow_config_gui = {
    mainwindow_configwin_gui_elements, NULL, NULL
};

//-----------------------------------------------------------------------------------

static void _win_main_config_frameskip_radbtn_callback(int num)
{
    EmulatorConfig.frameskip = num;
    Win_MainSetFrameskip(num);
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

static void _win_main_config_hardware_type_radbtn_callback(int num)
{
    EmulatorConfig.hardware_type = num;
}

static void _win_main_config_serial_device_radbtn_callback(int num)
{
    EmulatorConfig.serial_device = num;
    GB_SerialPlug(EmulatorConfig.serial_device);
}

static void _win_main_config_enable_blur_callback(int checked)
{
    EmulatorConfig.enableblur = checked;
    GB_EnableBlur(EmulatorConfig.enableblur);
}

static void _win_main_config_real_colors_callback(int checked)
{
    EmulatorConfig.realcolors = checked;
    GB_EnableRealColors(EmulatorConfig.realcolors);
}

static void _win_main_config_update_gbpal_info(void)
{
    u8 r,g,b;
    GB_ConfigGetPalette(&r,&g,&b);

    char text[40];
    s_snprintf(text,sizeof(text),"GB Palette: (%2d,%2d,%2d)",r/8,g/8,b/8);
    GUI_SetLabelCaption(&mainwindow_configwin_gameboy_palette_label,text);

    int i;
    for(i = 0; i < 32*32; i++)
    {
        gameboy_palette[(i*3)+0] = r;
        gameboy_palette[(i*3)+1] = g;
        gameboy_palette[(i*3)+2] = b;
    }
}

static void _win_main_config_gbpal_r_scrollbar_callback(int value)
{
    u8 r,g,b; GB_ConfigGetPalette(&r,&g,&b);
    r = value*8;
    GB_ConfigSetPalette(r,g,b);
    _win_main_config_update_gbpal_info();
}

static void _win_main_config_gbpal_g_scrollbar_callback(int value)
{
    u8 r,g,b; GB_ConfigGetPalette(&r,&g,&b);
    g = value*8;
    GB_ConfigSetPalette(r,g,b);
    _win_main_config_update_gbpal_info();
}

static void _win_main_config_gbpal_b_scrollbar_callback(int value)
{
    u8 r,g,b; GB_ConfigGetPalette(&r,&g,&b);
    b = value*8;
    GB_ConfigSetPalette(r,g,b);
    _win_main_config_update_gbpal_info();
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

    GUI_SetLabel(&mainwindow_configwin_general_frameskip_label,12,129,-1,FONT_HEIGHT,"Frameskip:");

    GUI_SetRadioButton(&mainwindow_configwin_gameboy_frameskip_auto_radbtn, 12+10*FONT_WIDTH+6, 126, 5*FONT_WIDTH,18,
                  "Auto", 2, -1, EmulatorConfig.frameskip==-1,_win_main_config_frameskip_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_frameskip_0_radbtn,    12+15*FONT_WIDTH+12,126, 2*FONT_WIDTH,18,
                  "0",    2,  0, EmulatorConfig.frameskip==0, _win_main_config_frameskip_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_frameskip_1_radbtn,    12+17*FONT_WIDTH+18,126, 2*FONT_WIDTH,18,
                  "1",    2,  1, EmulatorConfig.frameskip==1, _win_main_config_frameskip_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_frameskip_2_radbtn,    12+19*FONT_WIDTH+24,126, 2*FONT_WIDTH,18,
                  "2",    2,  2, EmulatorConfig.frameskip==2, _win_main_config_frameskip_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_frameskip_3_radbtn,    12+21*FONT_WIDTH+30,126, 2*FONT_WIDTH,18,
                  "3",    2,  3, EmulatorConfig.frameskip==3, _win_main_config_frameskip_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_frameskip_4_radbtn,    12+23*FONT_WIDTH+36,126, 2*FONT_WIDTH,18,
                  "4",    2,  4, EmulatorConfig.frameskip==4, _win_main_config_frameskip_radbtn_callback);

    // Auto frameskip not implemented.
    GUI_RadioButtonSetEnabled(&mainwindow_configwin_gameboy_frameskip_auto_radbtn,0);

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

    GUI_SetLabel(&mainwindow_configwin_gameboy_hard_type_label,12,213,-1,FONT_HEIGHT,"Hardware type:");
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_hard_type_auto_radbtn, 12+15*FONT_WIDTH+18,210, 5*FONT_WIDTH,18,
                  "Auto", 3, -1, EmulatorConfig.hardware_type==-1,_win_main_config_hardware_type_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_hard_type_gb_radbtn,   12+20*FONT_WIDTH+24,210,5*FONT_WIDTH,18,
                  "GB",   3,  0, EmulatorConfig.hardware_type==0,_win_main_config_hardware_type_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_hard_type_gbp_radbtn,  12,231,5*FONT_WIDTH,18,
                  "GBP",  3,  1, EmulatorConfig.hardware_type==1,_win_main_config_hardware_type_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_hard_type_sgb_radbtn,  12+5*FONT_WIDTH+6,231,5*FONT_WIDTH,18,
                  "SGB",  3,  2, EmulatorConfig.hardware_type==2,_win_main_config_hardware_type_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_hard_type_sgb2_radbtn, 12+10*FONT_WIDTH+12,231,5*FONT_WIDTH,18,
                  "SGB2", 3,  3, EmulatorConfig.hardware_type==3,_win_main_config_hardware_type_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_hard_type_gbc_radbtn,  12+15*FONT_WIDTH+18,231, 5*FONT_WIDTH,18,
                  "GBC",  3,  4, EmulatorConfig.hardware_type==4,_win_main_config_hardware_type_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_hard_type_gba_radbtn,  12+20*FONT_WIDTH+24,231,5*FONT_WIDTH,18,
                  "GBA",  3,  5, EmulatorConfig.hardware_type==5,_win_main_config_hardware_type_radbtn_callback);

    GUI_SetLabel(&mainwindow_configwin_gameboy_serial_device_label,12,260,-1,FONT_HEIGHT,"Serial device:");
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_serial_device_none_radbtn,
                       12,278, 6*FONT_WIDTH,18,
                       "None",       4,  SERIAL_NONE, EmulatorConfig.serial_device == SERIAL_NONE,
                       _win_main_config_serial_device_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_serial_device_gbprinter_radbtn,
                       12+6*FONT_WIDTH+6,278,11*FONT_WIDTH,18,
                       "GB Printer", 4,  SERIAL_GBPRINTER, EmulatorConfig.serial_device == SERIAL_GBPRINTER,
                       _win_main_config_serial_device_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_configwin_gameboy_serial_device_gameboy_radbtn,
                       12+17*FONT_WIDTH+12,278,11*FONT_WIDTH,18,
                       "Game Boy",   4,  SERIAL_GAMEBOY, EmulatorConfig.serial_device == SERIAL_GAMEBOY,
                       _win_main_config_serial_device_radbtn_callback);
    // Game Boy serial to another Game Boy not implemented, so disable this.
    GUI_RadioButtonSetEnabled(&mainwindow_configwin_gameboy_serial_device_gameboy_radbtn,0);

    GUI_SetCheckBox(&mainwindow_configwin_gameboy_enableblur_checkbox,12,302,-1,12,"Enable blur",
                        EmulatorConfig.enableblur, _win_main_config_enable_blur_callback);

    GUI_SetCheckBox(&mainwindow_configwin_gameboy_realcolors_checkbox,12+105,302,-1,12,"Real colors",
                        EmulatorConfig.realcolors, _win_main_config_real_colors_callback);

    u8 r,g,b;
    GB_ConfigGetPalette(&r,&g,&b);
    char text[40];
    s_snprintf(text,sizeof(text),"GB Palette: (%2d,%2d,%2d)",r/8,g/8,b/8);
    GUI_SetLabel(&mainwindow_configwin_gameboy_palette_label,12,320,-1,FONT_HEIGHT,text);
    GUI_SetScrollBar(&mainwindow_configwin_gameboy_pal_r_scrollbar,12,338,172,10,
                     0,31,r/8,_win_main_config_gbpal_r_scrollbar_callback);
    GUI_SetScrollBar(&mainwindow_configwin_gameboy_pal_g_scrollbar,12,349,172,10,
                     0,31,g/8,_win_main_config_gbpal_g_scrollbar_callback);
    GUI_SetScrollBar(&mainwindow_configwin_gameboy_pal_b_scrollbar,12,360,172,10,
                     0,31,b/8,_win_main_config_gbpal_b_scrollbar_callback);

    _win_main_config_update_gbpal_info();

    GUI_SetBitmap(&mainwindow_configwin_gameboy_palette_bitmap,12+172+6,338,32,32, gameboy_palette,NULL);

    //-----------------------------

    GUI_SetGroupBox(&mainwindow_configwin_gameboyadvance_groupbox,234,195,222,153,"Game Boy Advance");

    //-----------------------------

    GUI_SetButton(&mainwindow_configwin_close_btn, 407,354,7*FONT_WIDTH,2*FONT_HEIGHT,"Close",
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



