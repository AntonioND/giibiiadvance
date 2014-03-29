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

#include "win_main_config.h"
#include "win_utils.h"

#include "../font_utils.h"
#include "../config.h"

//-----------------------------------------------------------------------------------

_gui_element mainwindow_configwin;

//-----------------------------------------------------------------------------------

static _gui_element mainwindow_configwin_close_btn;

static _gui_element mainwindow_configwin_general_groupbox;
static _gui_element mainwindow_configwin_general_frameskip_label;
static _gui_element mainwindow_configwin_general_frameskip_scrollbar;


static _gui_element mainwindow_configwin_sound_groupbox;


static _gui_element mainwindow_configwin_gameboy_groupbox;


static _gui_element mainwindow_configwin_gameboyadvance_groupbox;


static _gui_element * mainwindow_configwin_gui_elements[] = {
    &mainwindow_configwin_general_groupbox,
    &mainwindow_configwin_general_frameskip_label,
    &mainwindow_configwin_general_frameskip_scrollbar,

    &mainwindow_configwin_sound_groupbox,


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

void _win_main_set_frameskip(int frameskip); // in win_main.c

static void _win_main_config_frameskip_scrollbar_callback(int value)
{
    EmulatorConfig.frameskip = value;
    _win_main_set_frameskip(value);
    return;
}

//-----------------------------------------------------------------------------------

void Win_MainCreateConfigWindow(void)
{
    //-----------------------------

    GUI_SetGroupBox(&mainwindow_configwin_general_groupbox,6,6,222,183,"General");

    GUI_SetLabel(&mainwindow_configwin_general_frameskip_label,12,24,-1,FONT_HEIGHT,"Frameskip: Auto");

    GUI_SetScrollBar(&mainwindow_configwin_general_frameskip_scrollbar,12,42,100,12,
                     0,9,EmulatorConfig.frameskip,_win_main_config_frameskip_scrollbar_callback);

    GUI_SetButton(&mainwindow_configwin_close_btn, 300,300,6*FONT_WIDTH,2*FONT_HEIGHT,"Close",Win_MainCloseConfigWindow);

    //-----------------------------

    GUI_SetGroupBox(&mainwindow_configwin_sound_groupbox,234,6,222,200,"Sound");

    //-----------------------------

    GUI_SetGroupBox(&mainwindow_configwin_gameboy_groupbox,6,195,222,183,"Game Boy");

    //-----------------------------

    //&mainwindow_configwin_gameboyadvance_groupbox,

    //-----------------------------

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
}

//-----------------------------------------------------------------------------------
