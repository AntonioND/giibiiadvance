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
#include <stdlib.h>

#include "input_utils.h"
#include "debug_utils.h"

#include "gb_core/gb_main.h"
#include "gb_core/sgb.h"

#include "gba_core/gba.h"

//----------------------------------------------------------------------------------------

typedef struct {
    int is_opened;
    SDL_Joystick * joystick;
    char name[50]; // clamp names to 49 chars... it should be more than enough to identify a joystick
    SDL_Haptic * haptic;
} _joystick_info_;

static _joystick_info_ Joystick[4];//only 4 joysticks at a time
static int joystick_number;

//----------------------------------------------------------------------------------------

typedef struct {
    int index;
    int enabled;
} _controller_player_info_;

static _controller_player_info_ ControllerPlayerInfo[4] = {
    { -1, 1}, // keyboard, enabled
    {  0, 0}, // controller 0, disabled
    {  1, 0}, // controller 1, disabled
    {  2, 0}  // controller 2, disabled
};

//default to keyboard the first player, the rest to unused

static SDL_Scancode _player_key_[4][P_NUM_KEYS] = {
    //This is the default configuration, changed when loading the config file (if it exists).

    //Each value can be a SDLK_* define or a joystick button number

//   P_KEY_A, P_KEY_B, P_KEY_L, P_KEY_R, P_KEY_UP, P_KEY_RIGHT, P_KEY_DOWN, P_KEY_LEFT, P_KEY_START, P_KEY_SELECT,
    { SDLK_x,SDLK_z,SDLK_a,SDLK_s,SDLK_UP,SDLK_RIGHT,SDLK_DOWN,SDLK_LEFT,SDLK_RETURN,SDLK_RSHIFT },
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }
};

static SDL_Scancode player_key_speedup = SDLK_SPACE;

const char * GBKeyNames[P_NUM_KEYS] = {
    "A", "B", "L", "R", "Up", "Right", "Down", "Left", "Start", "Select"
};

//----------------------------------------------------------------------------------------

// -1 = keyboard, others = number of joypad
void Input_PlayerSetController(int player, int index)
{
    if(player < 0) return;
    if(player >= 4) return;

    ControllerPlayerInfo[player].index = index;
}

int Input_PlayerGetController(int player)
{
    if(player < 0) return -1;
    if(player >= 4) return -1;

    return ControllerPlayerInfo[player].index;
}

//----------------------------

void Input_PlayerSetEnabled(int player, int enabled)
{
    if(player < 0) return;
    if(player >= 4) return;

    ControllerPlayerInfo[player].enabled = enabled;
}

int Input_PlayerGetEnabled(int player)
{
    if(player < 0) return 0;
    if(player >= 4) return 0;

    return ControllerPlayerInfo[player].enabled;
}

//----------------------------

void Input_ControlsSetKey(int player, _key_config_enum_ keyindex, SDL_Scancode keyscancode)
{
    if(keyindex == P_KEY_NONE) return;
    if(keyindex == P_NUM_KEYS) return;

    if(keyindex == P_KEY_SPEEDUP) player_key_speedup = keyscancode;
    else _player_key_[player][keyindex] = keyscancode;
}

SDL_Scancode Input_ControlsGetKey(int player, _key_config_enum_ keyindex)
{
    if(keyindex == P_KEY_NONE) return 0;
    if(keyindex == P_NUM_KEYS) return 0;

    if(keyindex == P_KEY_SPEEDUP) return player_key_speedup;
    return _player_key_[player][keyindex];
}

//----------------------------------------------------------------------------------------

int Input_IsGameBoyKeyPressed(int player, _key_config_enum_ keyindex)
{
    if(ControllerPlayerInfo[player].enabled == 0)
        return 0;

    if(ControllerPlayerInfo[player].index == -1) // keyboard
    {
        const Uint8 * state = SDL_GetKeyboardState(NULL);
        int key = Input_ControlsGetKey(player,keyindex);
        if(key)
            return state[SDL_GetScancodeFromKey(Input_ControlsGetKey(player,keyindex))];
        else
            return 0;
    }
    else // joystick
    {
        int joystick_index = ControllerPlayerInfo[player].index;
        int btn = Input_ControlsGetKey(player,keyindex);
        if(btn & KEYCODE_IS_AXIS) // axis
        {
            btn &= ~KEYCODE_IS_AXIS; // remove axis flag

            int is_btn_positive = (btn&KEYCODE_POSITIVE_AXIS);
            btn &= ~KEYCODE_POSITIVE_AXIS; //remove positive flag

            int axis_value = SDL_JoystickGetAxis(Joystick[joystick_index].joystick,btn);

            if( is_btn_positive && (axis_value > (16*1024) ) )
                return 1;
            else if( (!is_btn_positive) && (axis_value < (-16*1024) ) )
                return 1;
            else
                return 0;
        }
        else if(btn & KEYCODE_IS_HAT) // axis
        {
            btn &= ~KEYCODE_IS_HAT; // remove hat flag

            int position = (btn >> 4);

            int hat_index = btn & 0xF;

            if(position & SDL_JoystickGetHat(Joystick[joystick_index].joystick,hat_index))
                return 1;
            else
                return 0;
        }
        else // button
        {
            if(btn != -1)
                return SDL_JoystickGetButton(Joystick[joystick_index].joystick,btn);
            else
                return 0;
        }
    }

    return 0;
}

void Input_GetKeyboardElementName(char * name, int namelen, int btncode)
{
    if(btncode <= 0)
    {
        s_strncpy(name,"None",namelen);
    }
    else
    {
        s_strncpy(name,SDL_GetKeyName(btncode),namelen);
    }
}

void Input_GetJoystickElementName(char * name, int namelen, int btncode)
{
    if(btncode == -1)
    {
        s_strncpy(name,"None",namelen);
    }
    else if(btncode & KEYCODE_IS_AXIS) // axis
    {
        btncode &= ~KEYCODE_IS_AXIS; // remove axis flag

        int is_btn_positive = (btncode&KEYCODE_POSITIVE_AXIS);
        btncode &= ~KEYCODE_POSITIVE_AXIS; //remove positive flag

        s_snprintf(name,namelen,"Axis %d%c",btncode,is_btn_positive?'+':'-');
    }
    else if(btncode & KEYCODE_IS_HAT) // axis
    {
        btncode &= ~KEYCODE_IS_HAT; // remove hat flag

        int position = (btncode >> 4);

        int hat_index = btncode & 0xF;

        char * dir = "Centered [!]";
        if(position & SDL_HAT_UP) dir = "Up";
        else if(position & SDL_HAT_RIGHT) dir = "Right";
        else if(position & SDL_HAT_DOWN) dir = "Down";
        else if(position & SDL_HAT_LEFT) dir = "Left";

        s_snprintf(name,namelen,"Hat %d %s",hat_index,dir);
    }
    else // button
    {
        s_snprintf(name,namelen,"Button %d",btncode);
    }
}

//----------------------------------------------------------------------------------------

void Input_Update_GB(void)
{
    int players = 1;
    if(SGB_MultiplayerIsEnabled()) players = 4;
    int i;
    for(i = 0; i < players; i++)
    {
        int a = Input_IsGameBoyKeyPressed(i,P_KEY_A);
        int b = Input_IsGameBoyKeyPressed(i,P_KEY_B);
        int st = Input_IsGameBoyKeyPressed(i,P_KEY_START);
        int se = Input_IsGameBoyKeyPressed(i,P_KEY_SELECT);
        int dr = Input_IsGameBoyKeyPressed(i,P_KEY_RIGHT);
        int dl = Input_IsGameBoyKeyPressed(i,P_KEY_LEFT);
        int du = Input_IsGameBoyKeyPressed(i,P_KEY_UP);
        int dd = Input_IsGameBoyKeyPressed(i,P_KEY_DOWN);

        GB_InputSet(i, a, b, st, se, dr, dl, du, dd);
    }

    const Uint8 * state = SDL_GetKeyboardState(NULL);

    int accr = state[SDL_SCANCODE_KP_6];
    int accl = state[SDL_SCANCODE_KP_4];
    int accu = state[SDL_SCANCODE_KP_8];
    int accd = state[SDL_SCANCODE_KP_2];

    GB_InputSetMBC7Buttons(accu,accd,accr,accl);
//void GB_InputSetMBC7Joystick(int x, int y); // -200 to 200
//void GB_InputSetMBC7Buttons(int up, int down, int right, int left);
}

void Input_Update_GBA(void)
{
    int a = Input_IsGameBoyKeyPressed(0,P_KEY_A);
    int b = Input_IsGameBoyKeyPressed(0,P_KEY_B);
    int l = Input_IsGameBoyKeyPressed(0,P_KEY_L);
    int r = Input_IsGameBoyKeyPressed(0,P_KEY_R);
    int st = Input_IsGameBoyKeyPressed(0,P_KEY_START);
    int se = Input_IsGameBoyKeyPressed(0,P_KEY_SELECT);
    int dr = Input_IsGameBoyKeyPressed(0,P_KEY_RIGHT);
    int dl = Input_IsGameBoyKeyPressed(0,P_KEY_LEFT);
    int du = Input_IsGameBoyKeyPressed(0,P_KEY_UP);
    int dd = Input_IsGameBoyKeyPressed(0,P_KEY_DOWN);

    GBA_HandleInput(a, b, l, r, st, se, dr, dl, du, dd);
}

int Input_Speedup_Enabled(void)
{
    const Uint8 * state = SDL_GetKeyboardState(NULL);

    return state[SDL_SCANCODE_SPACE];
}

//----------------------------------------------------------------------------------------

SDL_Joystick * Input_GetJoystick(int index)
{
    if(index >= 4) return NULL;
    if(index < 0) return NULL;

    return Joystick[index].joystick;
}

char * Input_GetJoystickName(int index)
{
    if(index >= 4) return NULL;
    if(index < 0) return NULL;

    return Joystick[index].name;
}

static void _Input_CloseSystem(void)
{
    int i;
    for(i = 0; i < 4; i++) if(Joystick[i].is_opened)
    {
        Joystick[i].is_opened = 0;
        SDL_JoystickClose(Joystick[i].joystick);
        Joystick[i].joystick = NULL;

        if(Joystick[i].haptic)
            SDL_HapticClose(Joystick[i].haptic);
    }
}

int Input_GetJoystickNumber(void)
{
    return joystick_number;
}

void Input_InitSystem(void)
{
    atexit(_Input_CloseSystem);

    int i;
    for(i = 0; i < 4; i++) // clear things
    {
        Joystick[i].is_opened = 0;
        Joystick[i].joystick = NULL;
    }

    joystick_number = SDL_NumJoysticks();

    for(i = 0; i < 4; i++) // load joysticks
    {
        if(i < SDL_NumJoysticks())
        {
            Joystick[i].joystick = SDL_JoystickOpen(i);
            Joystick[i].is_opened = (Joystick[i].joystick != NULL);

            const char * name = SDL_JoystickNameForIndex(i);
            if(name)
                s_strncpy(Joystick[i].name,name,sizeof(Joystick[i].name));
            else
                s_strncpy(Joystick[i].name,"Unknown Joystick",sizeof(Joystick[i].name));

            if(Joystick[i].joystick)
            {
                //create rumble functions

                Joystick[i].haptic = SDL_HapticOpenFromJoystick(Joystick[i].joystick);
                if(Joystick[i].haptic)
                {
                    if(SDL_HapticRumbleInit(Joystick[i].haptic) == 0)
                    {
                        SDL_HapticClose(Joystick[i].haptic);
                        Joystick[i].haptic = NULL;
                    }

                       //else SDL_HapticRumblePlay( Joystick[0].haptic, 0.5, 1000 ); // rumble for 1 ms for player 0

                }
                else
                {
                    Debug_LogMsgArg("Couldn't open haptic for joystick %d: %s",i,SDL_GetError());
                }
            }
        }
    }
}

void Input_RumbleEnable(void)
{
    int controller = ControllerPlayerInfo[0].index; // player 0 only
    if(controller >= 0)
        SDL_HapticRumblePlay( Joystick[controller].haptic, 0.5, 16 ); // rumble for 16 ms (one frame)
}

//----------------------------------------------------------------------------------------

