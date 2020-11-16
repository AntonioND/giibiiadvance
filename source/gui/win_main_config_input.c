// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include <SDL2/SDL.h>

#include "win_main.h"
#include "win_main_config_input.h"
#include "win_utils.h"

#include "../config.h"
#include "../font_utils.h"
#include "../input_utils.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/serial.h"
#include "../gb_core/video.h"

//------------------------------------------------------------------------------

_gui_element mainwindow_config_input_win;

//------------------------------------------------------------------------------

// Note: mwciw = main window config input window

static _gui_element mwciw_win_player_groupbox;

static _gui_element mwciw_player1_radbtn, mwciw_player2_radbtn,
                    mwciw_player3_radbtn, mwciw_player4_radbtn;

static _gui_element mwciw_sel_keyboard_radbtn;
static _gui_element mwciw_sel_controller0_radbtn, mwciw_sel_controller1_radbtn,
                    mwciw_sel_controller2_radbtn, mwciw_sel_controller3_radbtn;

static _gui_element mwciw_change_a_btn, mwciw_change_b_btn, mwciw_change_l_btn,
                    mwciw_change_r_btn, mwciw_change_up_btn,
                    mwciw_change_right_btn, mwciw_change_down_btn,
                    mwciw_change_left_btn, mwciw_change_start_btn,
                    mwciw_change_select_btn;

static _gui_element mwciw_change_disable_btn, mwciw_change_default_btn;

static _gui_element mwciw_selected_a_label, mwciw_selected_b_label,
                    mwciw_selected_l_label, mwciw_selected_r_label,
                    mwciw_selected_up_label, mwciw_selected_right_label,
                    mwciw_selected_down_label, mwciw_selected_left_label,
                    mwciw_selected_start_label, mwciw_selected_select_label;

static _gui_console mwciw_win_controller_con;
static _gui_element mwciw_win_controller_textbox;

static _gui_element mwciw_win_status_label;
static _gui_console mwciw_win_status_con;
static _gui_element mwciw_win_status_textbox;

//------------------------

static _gui_element mwciw_close_btn;
static _gui_element mwciw_inputget;

//------------------------

static _gui_element *mainwindow_config_input_win_gui_elements[] = {

    &mwciw_player1_radbtn, &mwciw_player2_radbtn, &mwciw_player3_radbtn,
    &mwciw_player4_radbtn,

    &mwciw_win_player_groupbox,

    &mwciw_sel_keyboard_radbtn,
    &mwciw_sel_controller0_radbtn, &mwciw_sel_controller1_radbtn,
    &mwciw_sel_controller2_radbtn, &mwciw_sel_controller3_radbtn,

    &mwciw_win_controller_textbox,

    &mwciw_change_a_btn, &mwciw_change_b_btn, &mwciw_change_l_btn,
    &mwciw_change_r_btn, &mwciw_change_up_btn, &mwciw_change_right_btn,
    &mwciw_change_down_btn, &mwciw_change_left_btn, &mwciw_change_start_btn,
    &mwciw_change_select_btn,

    &mwciw_change_disable_btn,
    &mwciw_change_default_btn,

    &mwciw_selected_a_label, &mwciw_selected_b_label, &mwciw_selected_l_label,
    &mwciw_selected_r_label, &mwciw_selected_up_label,
    &mwciw_selected_right_label, &mwciw_selected_down_label,
    &mwciw_selected_left_label, &mwciw_selected_start_label,
    &mwciw_selected_select_label,

    &mwciw_win_status_label,
    &mwciw_win_status_textbox,

    &mwciw_close_btn,
    &mwciw_inputget,

    NULL
};

//------------------------------------------------------------------------------

static _gui mainwindow_subwindow_config_input_gui = {
    mainwindow_config_input_win_gui_elements, NULL, NULL
};

//------------------------------------------------------------------------------

static int win_main_config_selected_player = 0;

static int win_main_config_is_changing_button = P_KEY_NONE;

//------------------------------------------------------------------------------

static void _win_main_config_input_update_window(void)
{
    GUI_ConsoleClear(&mwciw_win_controller_con);

    int i;

    for (i = 0; i < Input_GetJoystickNumber(); i++)
    {
        if (i < 4)
        {
            const char *name = Input_GetJoystickName(i);
            char *rumble;
            if ((int)Input_JoystickHasRumble(i) == -1)
                rumble = " (Rumble error)";
            else if (Input_JoystickHasRumble(i) == 0)
                rumble = " (No Rumble)";
            else
                rumble = " (Rumble)";

            GUI_ConsoleModePrintf(&mwciw_win_controller_con, 0, i,
                                  "Joystick %d: %s%s\n", i,
                                  name ? name : "Unknown Joystick", rumble);
        }
    }

    if (i == 0)
    {
        GUI_ConsoleModePrintf(&mwciw_win_controller_con, 0, 0,
                              "No joysticks detected!");
    }

    int controller = Input_PlayerGetController(win_main_config_selected_player);

    if (controller == -1)
    {
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,
                                  &mwciw_sel_keyboard_radbtn);
    }
    else if (controller == 0)
    {
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,
                                  &mwciw_sel_controller0_radbtn);
    }
    else if (controller == 1)
    {
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,
                                  &mwciw_sel_controller1_radbtn);
    }
    else if (controller == 2)
    {
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,
                                  &mwciw_sel_controller2_radbtn);
    }
    else if (controller == 3)
    {
        GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,
                                  &mwciw_sel_controller3_radbtn);
    }

    GUI_ConsoleClear(&mwciw_win_status_con);

    if (win_main_config_is_changing_button == P_KEY_NONE)
    {
        GUI_ConsoleModePrintf(&mwciw_win_status_con, 0, 0, "Waiting...");
    }
    else
    {
        char *name = "Unknown (Error)";
        switch (win_main_config_is_changing_button)
        {
            case P_KEY_A:
                name = "A";
                break;
            case P_KEY_B:
                name = "B";
                break;
            case P_KEY_L:
                name = "L";
                break;
            case P_KEY_R:
                name = "R";
                break;
            case P_KEY_UP:
                name = "Up";
                break;
            case P_KEY_RIGHT:
                name = "Right";
                break;
            case P_KEY_DOWN:
                name = "Down";
                break;
            case P_KEY_LEFT:
                name = "Left";
                break;
            case P_KEY_START:
                name = "Start";
                break;
            case P_KEY_SELECT:
                name = "Select";
                break;
            default:
                name = "Unknown (Error)";
                break;
        }

        GUI_ConsoleModePrintf(&mwciw_win_status_con, 0, 0,
                              "Setting key [ %s ] for player %d ...", name,
                              win_main_config_selected_player);
    }

    GUI_SetButtonText(&mwciw_change_disable_btn,
                      Input_PlayerGetEnabled(win_main_config_selected_player)
                              ? "Disable player"
                              : "Enable player");

    if (Input_PlayerGetController(win_main_config_selected_player) == -1)
    {
        char name[20];
        int btn;

        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_A);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_a_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_B);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_b_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_L);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_l_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_R);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_r_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_UP);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_up_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_RIGHT);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_right_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_DOWN);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_down_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_LEFT);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_left_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_START);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_start_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_SELECT);
        Input_GetKeyboardElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_select_label, name);
    }
    else
    {
        char name[20];
        int btn;
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_A);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_a_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_B);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_b_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_L);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_l_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_R);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_r_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_UP);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_up_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_RIGHT);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_right_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_DOWN);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_down_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_LEFT);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_left_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_START);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_start_label, name);
        btn = Input_ControlsGetKey(win_main_config_selected_player, P_KEY_SELECT);
        Input_GetJoystickElementName(name, sizeof(name), btn);
        GUI_SetLabelCaption(&mwciw_selected_select_label, name);
    }
}

//------------------------------------------------------------------------------

static void _win_main_config_input_select_player_radbtn_callback(int num)
{
    win_main_config_is_changing_button = P_KEY_NONE;

    win_main_config_selected_player = num;

    _win_main_config_input_update_window();
}

static void _win_main_config_input_select_controller_radbtn_callback(int num)
{
    win_main_config_is_changing_button = P_KEY_NONE;

    if (Input_PlayerGetController(win_main_config_selected_player) != num)
    {
        Input_PlayerSetController(win_main_config_selected_player, num);

        // Set default configuration to avoid mixing controllers
        if (num == -1)
        {
            if (win_main_config_selected_player == 0)
            {
                Input_PlayerSetEnabled(0, 1);
                Input_PlayerSetController(0, -1);
                Input_ControlsSetKey(0, P_KEY_A, SDLK_x);
                Input_ControlsSetKey(0, P_KEY_B, SDLK_z);
                Input_ControlsSetKey(0, P_KEY_L, SDLK_a);
                Input_ControlsSetKey(0, P_KEY_R, SDLK_s);
                Input_ControlsSetKey(0, P_KEY_UP, SDLK_UP);
                Input_ControlsSetKey(0, P_KEY_RIGHT, SDLK_RIGHT);
                Input_ControlsSetKey(0, P_KEY_DOWN, SDLK_DOWN);
                Input_ControlsSetKey(0, P_KEY_LEFT, SDLK_LEFT);
                Input_ControlsSetKey(0, P_KEY_START, SDLK_RETURN);
                Input_ControlsSetKey(0, P_KEY_SELECT, SDLK_RSHIFT);
            }
            else
            {
                for (int i = 0; i < P_NUM_KEYS; i++)
                    Input_ControlsSetKey(win_main_config_selected_player, i, -1);
            }
        }
        else
        {
            // Default: disable joysticks for players 2-4
            //if (win_main_config_selected_player != 0)
            //    Input_PlayerSetEnabled(win_main_config_selected_player, 0);

            for (int i = 0; i < P_NUM_KEYS; i++)
                Input_ControlsSetKey(win_main_config_selected_player, i, -1);
        }
    }

    _win_main_config_input_update_window();
}

static int _win_main_config_input_inputget_callback(SDL_Event *e)
{
    if (win_main_config_is_changing_button == P_KEY_NONE)
        return 0;

    int redraw_gui = 1;

    int controller = Input_PlayerGetController(win_main_config_selected_player);

    if (controller == -1)
    {
        // Keyboard
        if (e->type == SDL_KEYDOWN)
        {
            SDL_KeyCode scancode; // scancode name

            scancode = SDL_GetKeyFromScancode(e->key.keysym.scancode);

            if (win_main_config_is_changing_button < P_NUM_KEYS)
            {
                Input_ControlsSetKey(win_main_config_selected_player,
                                     win_main_config_is_changing_button,
                                     scancode);
                redraw_gui = 1;
            }
            else
            {
                redraw_gui = 0;
            }
        }
        else
        {
            redraw_gui = 0;
        }
    }
    else // Joypad
    {
        if (e->type == SDL_JOYBUTTONDOWN)
        {
            if (e->jbutton.which == controller)
            {
                int btn = e->jbutton.button;

                if (win_main_config_is_changing_button < P_NUM_KEYS)
                {
                    Input_ControlsSetKey(win_main_config_selected_player,
                                         win_main_config_is_changing_button,
                                         btn);
                    redraw_gui = 1;
                }
                else
                {
                    redraw_gui = 0;
                }
            }
            else
            {
                redraw_gui = 0;
            }
        }
        else if (e->type == SDL_JOYAXISMOTION)
        {
            if (e->jbutton.which == controller)
            {
                int btn = KEYCODE_IS_AXIS
                          + ((e->jaxis.value > 0) ? KEYCODE_POSITIVE_AXIS : 0)
                          + e->jaxis.axis;

                if (win_main_config_is_changing_button < P_NUM_KEYS)
                {
                    Input_ControlsSetKey(win_main_config_selected_player,
                                         win_main_config_is_changing_button,
                                         btn);
                    redraw_gui = 1;
                }
                else
                {
                    redraw_gui = 0;
                }
            }
            else
            {
                redraw_gui = 0;
            }
        }
        else if (e->type == SDL_JOYHATMOTION)
        {
            if (e->jbutton.which == controller)
            {
                int btn = KEYCODE_IS_HAT + (e->jhat.value << 4) + e->jhat.hat;

                if (win_main_config_is_changing_button < P_NUM_KEYS)
                {
                    Input_ControlsSetKey(win_main_config_selected_player,
                                         win_main_config_is_changing_button,
                                         btn);
                    redraw_gui = 1;
                }
                else
                {
                    redraw_gui = 0;
                }
            }
            else
            {
                redraw_gui = 0;
            }
        }
        else
        {
            redraw_gui = 0;
        }
    }

    if (redraw_gui) // If any key has changed
    {
        win_main_config_is_changing_button = P_KEY_NONE;
        Input_PlayerSetEnabled(win_main_config_selected_player, 1);
    }

    _win_main_config_input_update_window();

    return redraw_gui;
}

//------------------------------------------------------------------------------

static void _win_main_config_change_a_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_A;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_b_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_B;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_l_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_L;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_r_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_R;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_start_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_START;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_select_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_SELECT;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_up_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_UP;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_down_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_DOWN;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_right_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_RIGHT;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_left_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_LEFT;
    _win_main_config_input_update_window();
}

static void _win_main_config_change_default_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_NONE;

    if (win_main_config_selected_player == 0)
    {
        Input_PlayerSetEnabled(0, 1);
        Input_PlayerSetController(0, -1);
        Input_ControlsSetKey(0, P_KEY_A, SDLK_x);
        Input_ControlsSetKey(0, P_KEY_B, SDLK_z);
        Input_ControlsSetKey(0, P_KEY_L, SDLK_a);
        Input_ControlsSetKey(0, P_KEY_R, SDLK_s);
        Input_ControlsSetKey(0, P_KEY_UP, SDLK_UP);
        Input_ControlsSetKey(0, P_KEY_RIGHT, SDLK_RIGHT);
        Input_ControlsSetKey(0, P_KEY_DOWN, SDLK_DOWN);
        Input_ControlsSetKey(0, P_KEY_LEFT, SDLK_LEFT);
        Input_ControlsSetKey(0, P_KEY_START, SDLK_RETURN);
        Input_ControlsSetKey(0, P_KEY_SELECT, SDLK_RSHIFT);
    }
    else
    {
        Input_PlayerSetEnabled(win_main_config_selected_player, 0);
        Input_PlayerSetController(win_main_config_selected_player,
                                  win_main_config_selected_player - 1);

        for (int i = 0; i < P_NUM_KEYS; i++)
            Input_ControlsSetKey(win_main_config_selected_player, i, -1);
    }

    _win_main_config_input_update_window();
}

static void _win_main_config_change_disable_btn_callback(void)
{
    win_main_config_is_changing_button = P_KEY_NONE;

    if (win_main_config_selected_player == 0) // Player 1 can't be disabled
    {
        _win_main_config_input_update_window();
        return;
    }

    if (Input_PlayerGetEnabled(win_main_config_selected_player))
    {
        Input_PlayerSetEnabled(win_main_config_selected_player, 0);
        Input_PlayerSetController(win_main_config_selected_player,
                                  win_main_config_selected_player - 1);

        for (int i = 0; i < P_NUM_KEYS; i++)
            Input_ControlsSetKey(win_main_config_selected_player, i, -1);

        _win_main_config_input_update_window();
    }
    else
    {
        Input_PlayerSetEnabled(win_main_config_selected_player, 1);
    }

    _win_main_config_input_update_window();
}

//------------------------------------------------------------------------------

void Win_MainCreateConfigInputWindow(void)
{
    win_main_config_selected_player = 0;

    GUI_SetRadioButton(&mwciw_player1_radbtn,
                       12, 6, 15 * FONT_WIDTH, 18,
                       "Player 1", 0, 0, 1,
                       _win_main_config_input_select_player_radbtn_callback);
    GUI_SetRadioButton(&mwciw_player2_radbtn,
                       18 + 15 * FONT_WIDTH, 6, 15 * FONT_WIDTH, 18,
                       "Player 2", 0, 1, 0,
                       _win_main_config_input_select_player_radbtn_callback);
    GUI_SetRadioButton(&mwciw_player3_radbtn,
                       24 + 30 * FONT_WIDTH, 6, 15 * FONT_WIDTH, 18,
                       "Player 3", 0, 2, 0,
                       _win_main_config_input_select_player_radbtn_callback);
    GUI_SetRadioButton(&mwciw_player4_radbtn,
                       30 + 45 * FONT_WIDTH, 6, 15 * FONT_WIDTH, 18,
                       "Player 4", 0, 3, 0,
                       _win_main_config_input_select_player_radbtn_callback);

    GUI_SetGroupBox(&mwciw_win_player_groupbox,
                    6, 30, 256 * 2 - 50 - 12, 224 * 2 - 50 - 168,
                    "Player configuration");

    int controller = Input_PlayerGetController(0);

    GUI_SetRadioButton(&mwciw_sel_keyboard_radbtn,
                       26, 48, 11 * FONT_WIDTH, 18,
                       "Keyboard", 1, -1, controller == -1,
                       _win_main_config_input_select_controller_radbtn_callback);
    GUI_SetRadioButton(&mwciw_sel_controller0_radbtn,
                       32 + 11 * FONT_WIDTH, 48, 11 * FONT_WIDTH, 18,
                       "Joystick 0", 1, 0, controller == 0,
                       _win_main_config_input_select_controller_radbtn_callback);
    GUI_SetRadioButton(&mwciw_sel_controller1_radbtn,
                       38 + 22 * FONT_WIDTH, 48, 11 * FONT_WIDTH, 18,
                       "Joystick 1", 1, 1, controller == 1,
                       _win_main_config_input_select_controller_radbtn_callback);
    GUI_SetRadioButton(&mwciw_sel_controller2_radbtn,
                       44 + 33 * FONT_WIDTH, 48, 11 * FONT_WIDTH, 18,
                       "Joystick 2", 1, 2, controller == 2,
                       _win_main_config_input_select_controller_radbtn_callback);
    GUI_SetRadioButton(&mwciw_sel_controller3_radbtn,
                       50 + 44 * FONT_WIDTH, 48, 11 * FONT_WIDTH, 18,
                       "Joystick 3", 1, 3, controller == 3,
                       _win_main_config_input_select_controller_radbtn_callback);

    GUI_SetButton(&mwciw_change_a_btn,
                  18, 78, 8 * FONT_WIDTH, 2 * FONT_HEIGHT, "A",
                  _win_main_config_change_a_btn_callback);
    GUI_SetButton(&mwciw_change_b_btn,
                  18, 108, 8 * FONT_WIDTH, 2 * FONT_HEIGHT, "B",
                  _win_main_config_change_b_btn_callback);
    GUI_SetButton(&mwciw_change_l_btn,
                  18, 138, 8 * FONT_WIDTH, 2 * FONT_HEIGHT, "L",
                  _win_main_config_change_l_btn_callback);
    GUI_SetButton(&mwciw_change_r_btn,
                  18, 168, 8 * FONT_WIDTH, 2 * FONT_HEIGHT, "R",
                  _win_main_config_change_r_btn_callback);
    GUI_SetButton(&mwciw_change_start_btn,
                  18, 198, 8 * FONT_WIDTH, 2 * FONT_HEIGHT, "Start",
                  _win_main_config_change_start_btn_callback);
    GUI_SetButton(&mwciw_change_select_btn,
                  18, 228, 8 * FONT_WIDTH, 2 * FONT_HEIGHT, "Select",
                  _win_main_config_change_select_btn_callback);

    GUI_SetButton(&mwciw_change_up_btn,
                  (256 * 2 - 50) / 2, 78, 8 * FONT_WIDTH, 2 * FONT_HEIGHT,
                  "Up", _win_main_config_change_up_btn_callback);
    GUI_SetButton(&mwciw_change_right_btn,
                  (256 * 2 - 50) / 2, 108, 8 * FONT_WIDTH, 2 * FONT_HEIGHT,
                  "Right", _win_main_config_change_right_btn_callback);
    GUI_SetButton(&mwciw_change_down_btn,
                  (256 * 2 - 50) / 2, 138, 8 * FONT_WIDTH, 2 * FONT_HEIGHT,
                  "Down", _win_main_config_change_down_btn_callback);
    GUI_SetButton(&mwciw_change_left_btn,
                  (256 * 2 - 50) / 2, 168, 8 * FONT_WIDTH, 2 * FONT_HEIGHT,
                  "Left", _win_main_config_change_left_btn_callback);

    GUI_SetButton(&mwciw_change_disable_btn,
                  (256 * 2 - 50) / 2, 198, 30 * FONT_WIDTH, 2 * FONT_HEIGHT,
                  "Disable player",
                  _win_main_config_change_disable_btn_callback);

    GUI_SetButton(&mwciw_change_default_btn,
                  (256 * 2 - 50) / 2, 228, 30 * FONT_WIDTH, 2 * FONT_HEIGHT,
                  "Default configuration",
                  _win_main_config_change_default_btn_callback);

    GUI_SetLabel(&mwciw_selected_a_label, 18 + 6 + 8 * FONT_WIDTH,
                 78 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");
    GUI_SetLabel(&mwciw_selected_b_label, 18 + 6 + 8 * FONT_WIDTH,
                 108 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");
    GUI_SetLabel(&mwciw_selected_l_label, 18 + 6 + 8 * FONT_WIDTH,
                 138 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");
    GUI_SetLabel(&mwciw_selected_r_label, 18 + 6 + 8 * FONT_WIDTH,
                 168 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");
    GUI_SetLabel(&mwciw_selected_start_label, 18 + 6 + 8 * FONT_WIDTH,
                 198 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");
    GUI_SetLabel(&mwciw_selected_select_label, 18 + 6 + 8 * FONT_WIDTH,
                 228 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");

    GUI_SetLabel(&mwciw_selected_up_label,
                 (256 * 2 - 50) / 2 + 6 + 8 * FONT_WIDTH,
                 78 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");
    GUI_SetLabel(&mwciw_selected_right_label,
                 (256 * 2 - 50) / 2 + 6 + 8 * FONT_WIDTH, 108 + 6,
                 20 * FONT_WIDTH, FONT_HEIGHT, "-");
    GUI_SetLabel(&mwciw_selected_down_label,
                 (256 * 2 - 50) / 2 + 6 + 8 * FONT_WIDTH,
                 138 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");
    GUI_SetLabel(&mwciw_selected_left_label,
                 (256 * 2 - 50) / 2 + 6 + 8 * FONT_WIDTH,
                 168 + 6, 20 * FONT_WIDTH, FONT_HEIGHT, "-");

    GUI_SetTextBox(&mwciw_win_controller_textbox, &mwciw_win_controller_con,
                   7, 270, 64 * FONT_WIDTH, 4 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&mwciw_win_status_label, 6, 324, -1, FONT_HEIGHT, "Status");

    GUI_SetTextBox(&mwciw_win_status_textbox, &mwciw_win_status_con,
                   6, 342, 56 * FONT_WIDTH, 3 * FONT_HEIGHT, NULL);

    GUI_SetInputGet(&mwciw_inputget, _win_main_config_input_inputget_callback);

    GUI_SetButton(&mwciw_close_btn, 407, 354, 7 * FONT_WIDTH, 2 * FONT_HEIGHT,
                  "Close", Win_MainCloseConfigInputWindow);

    GUI_SetWindow(&mainwindow_config_input_win,
                  25, 25, 256 * 2 - 50, 224 * 2 - 50,
                  &mainwindow_subwindow_config_input_gui,
                  "Input Configuration");

    win_main_config_is_changing_button = P_KEY_NONE;

    GUI_WindowSetEnabled(&mainwindow_config_input_win, 0);
}

void Win_MainOpenConfigInputWindow(void)
{
    win_main_config_selected_player = 0;

    GUI_RadioButtonSetPressed(&mainwindow_subwindow_config_input_gui,
                              &mwciw_player1_radbtn);

    win_main_config_is_changing_button = P_KEY_NONE;

    GUI_WindowSetEnabled(&mainwindow_config_input_win, 1);

    _win_main_config_input_update_window();
}

void Win_MainCloseConfigInputWindow(void)
{
    win_main_config_is_changing_button = P_KEY_NONE;

    GUI_WindowSetEnabled(&mainwindow_config_input_win, 0);

    Config_Save();
}
