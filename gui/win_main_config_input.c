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

static _gui_console mainwindow_config_input_win_controller_con;
static _gui_element mainwindow_config_input_win_controller_textbox;

//------------------------

static _gui_element * mainwindow_config_input_win_gui_elements[] = {

    &mainwindow_config_input_win_controller_textbox,
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
    GUI_SetTextBox(&mainwindow_config_input_win_controller_textbox,&mainwindow_config_input_win_controller_con,
                   6,6,40*FONT_WIDTH,10*FONT_HEIGHT,
                   NULL);

    GUI_ConsoleClear(&mainwindow_config_input_win_controller_con);

    int i;
    for(i = 0; i < SDL_NumJoysticks(); i++)
    {
        const char * name = SDL_JoystickNameForIndex(i);
        GUI_ConsoleModePrintf(&mainwindow_config_input_win_controller_con,0,i,
                              "Joystick %d: %s\n", i, name ? name : "Unknown Joystick");
        SDL_Joystick * joystick = SDL_JoystickOpen(i);
        if(joystick == NULL)
        {
            //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_JoystickOpen(%d) failed: %s\n", i,
            //        SDL_GetError());
        }
        else
        {/*
            char guid[64];
            SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick),
                                      guid, sizeof (guid));
            SDL_Log("       axes: %d\n", SDL_JoystickNumAxes(joystick));
            SDL_Log("      balls: %d\n", SDL_JoystickNumBalls(joystick));
            SDL_Log("       hats: %d\n", SDL_JoystickNumHats(joystick));
            SDL_Log("    buttons: %d\n", SDL_JoystickNumButtons(joystick));
            SDL_Log("instance id: %d\n", SDL_JoystickInstanceID(joystick));
            SDL_Log("       guid: %s\n", guid);*/
            SDL_JoystickClose(joystick);
        }
    }



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



