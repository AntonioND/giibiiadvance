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
#include "../input_utils.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/serial.h"
#include "../gb_core/video.h"

//-----------------------------------------------------------------------------------

_gui_element mainwindow_config_input_win;

//-----------------------------------------------------------------------------------

static _gui_element mainwindow_config_input_win_player_groupbox;

static _gui_element mainwindow_config_input_player1_radbtn;
static _gui_element mainwindow_config_input_player2_radbtn;
static _gui_element mainwindow_config_input_player3_radbtn;
static _gui_element mainwindow_config_input_player4_radbtn;

static _gui_element mainwindow_config_input_sel_keyboard_radbtn;
static _gui_element mainwindow_config_input_sel_controller0_radbtn;
static _gui_element mainwindow_config_input_sel_controller1_radbtn;
static _gui_element mainwindow_config_input_sel_controller2_radbtn;
static _gui_element mainwindow_config_input_sel_controller3_radbtn;

static _gui_console mainwindow_config_input_win_controller_con;
static _gui_element mainwindow_config_input_win_controller_textbox;

//------------------------

static _gui_element mainwindow_config_input_win_close_btn;
static _gui_element mainwindow_config_input_win_inputget;

//------------------------

static _gui_element * mainwindow_config_input_win_gui_elements[] = {

    &mainwindow_config_input_player1_radbtn,
    &mainwindow_config_input_player2_radbtn,
    &mainwindow_config_input_player3_radbtn,
    &mainwindow_config_input_player4_radbtn,

    &mainwindow_config_input_win_player_groupbox,

    &mainwindow_config_input_sel_keyboard_radbtn,
    &mainwindow_config_input_sel_controller0_radbtn,
    &mainwindow_config_input_sel_controller1_radbtn,
    &mainwindow_config_input_sel_controller2_radbtn,
    &mainwindow_config_input_sel_controller3_radbtn,

    &mainwindow_config_input_win_controller_textbox,




    &mainwindow_config_input_win_close_btn,
    &mainwindow_config_input_win_inputget,

    NULL
};

//-----------------------------------------------------------------------------------

static _gui mainwindow_subwindow_config_input_gui = {
    mainwindow_config_input_win_gui_elements, NULL, NULL
};

//-----------------------------------------------------------------------------------

static int win_main_config_selected_player = 0;

//-----------------------------------------------------------------------------------

static void _win_main_config_input_update_window(void)
{
    GUI_ConsoleClear(&mainwindow_config_input_win_controller_con);

    int i;
    for(i = 0; i < 4; i++)
    {
        if(i < SDL_NumJoysticks())
        {
            const char * name = SDL_JoystickNameForIndex(i);
            GUI_ConsoleModePrintf(&mainwindow_config_input_win_controller_con,0,i,
                                  "Joystick %d: %s\n", i, name ? name : "Unknown Joystick");
            SDL_Joystick * joystick = SDL_JoystickOpen(i);
            if(joystick == NULL)
            {
                GUI_ConsoleModePrintf(&mainwindow_config_input_win_controller_con,40-9,i," - NOT OK");
                //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_JoystickOpen(%d) failed: %s\n", i,
                //        SDL_GetError());
            }
            else
            {
                GUI_ConsoleModePrintf(&mainwindow_config_input_win_controller_con,40-5,i," - OK");
                SDL_JoystickClose(joystick);
            }
        }
    }

    int controller = Input_PlayerGetController(win_main_config_selected_player);

    if(controller == -1)
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,&mainwindow_config_input_sel_keyboard_radbtn);
    else if(controller == 0)
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,&mainwindow_config_input_sel_controller0_radbtn);
    else if(controller == 1)
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,&mainwindow_config_input_sel_controller1_radbtn);
    else if(controller == 2)
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,&mainwindow_config_input_sel_controller2_radbtn);
    else if(controller == 3)
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,&mainwindow_config_input_sel_controller3_radbtn);


    /*

    UPDATE THINGS ON THE SCREEN ACCORDING TO THE PLAYER

    */
}

//-----------------------------------------------------------------------------------

static void _win_main_config_input_select_player_radbtn_callback(int num)
{
    win_main_config_selected_player = num;

    _win_main_config_input_update_window();
}

static void _win_main_config_input_select_controller_radbtn_callback(int num)
{
    Input_PlayerSetController(win_main_config_selected_player,num);
}

static void _win_main_config_input_inputget_callback(SDL_Event * e)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GiiBiiAdvance - Error", "Event", NULL);
}

//-----------------------------------------------------------------------------------

void Win_MainCreateConfigInputWindow(void)
{
    win_main_config_selected_player = 0;

    GUI_SetRadioButton(&mainwindow_config_input_player1_radbtn,  12,6,15*FONT_WIDTH,18,
                  "Player 1", 0, 0, 1, _win_main_config_input_select_player_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_config_input_player2_radbtn,  18+15*FONT_WIDTH,6,15*FONT_WIDTH,18,
                  "Player 2", 0, 1, 0, _win_main_config_input_select_player_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_config_input_player3_radbtn,  24+30*FONT_WIDTH,6,15*FONT_WIDTH,18,
                  "Player 3", 0, 2, 0, _win_main_config_input_select_player_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_config_input_player4_radbtn,  30+45*FONT_WIDTH,6,15*FONT_WIDTH,18,
                  "Player 4", 0, 3, 0, _win_main_config_input_select_player_radbtn_callback);

    GUI_SetGroupBox(&mainwindow_config_input_win_player_groupbox,6,30,256*2-50-12,224*2-50-48,"Player configuration");

    int controller = Input_PlayerGetController(0);

    GUI_SetRadioButton(&mainwindow_config_input_sel_keyboard_radbtn,  18,48,9*FONT_WIDTH,18,
                  "Keyboard", 1, -1, controller==-1, _win_main_config_input_select_controller_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_config_input_sel_controller0_radbtn,  24+9*FONT_WIDTH,48,11*FONT_WIDTH,18,
                  "Joystick 0",  1, 0, controller==0, _win_main_config_input_select_controller_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_config_input_sel_controller1_radbtn,  30+20*FONT_WIDTH,48,11*FONT_WIDTH,18,
                  "Joystick 1",  1, 1, controller==1, _win_main_config_input_select_controller_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_config_input_sel_controller2_radbtn,  36+31*FONT_WIDTH,48,11*FONT_WIDTH,18,
                  "Joystick 2",  1, 2, controller==2, _win_main_config_input_select_controller_radbtn_callback);
    GUI_SetRadioButton(&mainwindow_config_input_sel_controller3_radbtn,  42+42*FONT_WIDTH,48,11*FONT_WIDTH,18,
                  "Joystick 3",  1, 3, controller==3, _win_main_config_input_select_controller_radbtn_callback);

    GUI_SetTextBox(&mainwindow_config_input_win_controller_textbox,&mainwindow_config_input_win_controller_con,
                   6,80,40*FONT_WIDTH,4*FONT_HEIGHT,
                   NULL);

    GUI_SetInputGet(&mainwindow_config_input_win_inputget,_win_main_config_input_inputget_callback);

    GUI_SetButton(&mainwindow_config_input_win_close_btn, 407,354,7*FONT_WIDTH,2*FONT_HEIGHT,"Close",
                  Win_MainCloseConfigInputWindow);

    GUI_SetWindow(&mainwindow_config_input_win,25,25,256*2-50,224*2-50,&mainwindow_subwindow_config_input_gui,
                  "Input Configuration");

    GUI_WindowSetEnabled(&mainwindow_config_input_win,0);
}

void Win_MainOpenConfigInputWindow(void)
{
    GUI_WindowSetEnabled(&mainwindow_config_input_win,1);

    _win_main_config_input_update_window();
}

void Win_MainCloseConfigInputWindow(void)
{
    GUI_WindowSetEnabled(&mainwindow_config_input_win,0);
}

//-----------------------------------------------------------------------------------



