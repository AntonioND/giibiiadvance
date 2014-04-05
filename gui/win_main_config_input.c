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

#include "win_main_config_input.h"
#include "win_main.h"

#include "../font_utils.h"
#include "../config.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/serial.h"
#include "../gb_core/video.h"

//-----------------------------------------------------------------------------------

_gui_element mainwindow_config_input_win;

//-----------------------------------------------------------------------------------

static _gui_element mainwindow_config_input_win_close_btn;

//------------------------

static _gui_element mainwindow_config_input_win_mbc7_groupbox;

//------------------------

static _gui_element * mainwindow_config_input_win_gui_elements[] = {

    &mainwindow_config_input_win_mbc7_groupbox,

    &mainwindow_config_input_win_close_btn,

    NULL
};

//-----------------------------------------------------------------------------------

static _gui mainwindow_subwindow_config_input_gui = {
    mainwindow_config_input_win_gui_elements, NULL, NULL
};

//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------

void Win_MainCreateConfigInputWindow(void)
{
    //-----------------------------

    GUI_SetGroupBox(&mainwindow_config_input_win_mbc7_groupbox,234,195,222,153,"MBC7");

    //-----------------------------

    GUI_SetButton(&mainwindow_config_input_win_close_btn, 407,354,7*FONT_WIDTH,2*FONT_HEIGHT,"Close",
                  Win_MainCloseConfigInputWindow);

    GUI_SetWindow(&mainwindow_config_input_win,25,25,256*2-50,224*2-50,&mainwindow_subwindow_config_input_gui,
                  "Input Configuration");

    GUI_WindowSetEnabled(&mainwindow_config_input_win,0);
}

void Win_MainOpenConfigInputWindow(void)
{
    GUI_WindowSetEnabled(&mainwindow_config_input_win,1);
}

void Win_MainCloseConfigInputWindow(void)
{
    GUI_WindowSetEnabled(&mainwindow_config_input_win,0);
}

//-----------------------------------------------------------------------------------



