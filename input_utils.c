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

#include "input_utils.h"

#include "gb_core/gb_main.h"

#include "gba_core/gba.h"

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
    //This is the default configuration, changed when loading the config file.
//   P_KEY_A, P_KEY_B, P_KEY_L, P_KEY_R, P_KEY_UP, P_KEY_RIGHT, P_KEY_DOWN, P_KEY_LEFT, P_KEY_START, P_KEY_SELECT,
    { SDLK_x,SDLK_z,SDLK_a,SDLK_s,SDLK_UP,SDLK_RIGHT,SDLK_DOWN,SDLK_LEFT,SDLK_RETURN,SDLK_RSHIFT },
    { 0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0 }
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

void Input_Update_GB(void)
{
    const Uint8 * state = SDL_GetKeyboardState(NULL);

    int a = state[SDL_SCANCODE_X];
    int b = state[SDL_SCANCODE_Z];
    int st = state[SDL_SCANCODE_RETURN];
    int se = state[SDL_SCANCODE_RSHIFT];
    int dr = state[SDL_SCANCODE_RIGHT];
    int dl = state[SDL_SCANCODE_LEFT];
    int du = state[SDL_SCANCODE_UP];
    int dd = state[SDL_SCANCODE_DOWN];

    GB_InputSet(0, a, b, st, se, dr, dl, du, dd);

    int accr = state[SDL_SCANCODE_KP_6];
    int accl = state[SDL_SCANCODE_KP_4];
    int accu = state[SDL_SCANCODE_KP_8];
    int accd = state[SDL_SCANCODE_KP_2];

    GB_InputSetMBC7Buttons(accu,accd,accr,accl);

    /*
    Keys[0] = 0; Keys[1] = 0; Keys[2] = 0; Keys[3] = 0;

    if(Keys_Down[VK_LEFT]) Keys[0] |= KEY_LEFT;
    if(Keys_Down[VK_UP]) Keys[0] |= KEY_UP;
    if(Keys_Down[VK_RIGHT]) Keys[0] |= KEY_RIGHT;
    if(Keys_Down[VK_DOWN]) Keys[0] |= KEY_DOWN;
    if(Keys_Down['X']) Keys[0] |= KEY_A;
    if(Keys_Down['Z']) Keys[0] |= KEY_B;
    if(Keys_Down[VK_RETURN]) Keys[0] |= KEY_START;
    if(Keys_Down[VK_SHIFT]) Keys[0] |= KEY_SELECT;

    if(SGB_MultiplayerIsEnabled())
    {
        int i;
        for(i = 1; i < 4; i++)
        {
            if(keystate[Config_Controls_Get_Key(i,P_KEY_LEFT)]) Keys[i] |= KEY_LEFT;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_UP)]) Keys[i] |= KEY_UP;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_RIGHT)]) Keys[i] |= KEY_RIGHT;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_DOWN)]) Keys[i] |= KEY_DOWN;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_A)]) Keys[i] |= KEY_A;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_B)]) Keys[i] |= KEY_B;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_START)]) Keys[i] |= KEY_START;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_SELECT)]) Keys[i] |= KEY_SELECT;
        }
    }

    if(GameBoy.Emulator.MemoryController == MEM_MBC7)
    {
        GB_Input_Update_MBC7(Keys_Down[VK_NUMPAD8],Keys_Down[VK_NUMPAD2],
                            Keys_Down[VK_NUMPAD6],Keys_Down[VK_NUMPAD4]);
    }
*/
}

void Input_Update_GBA(void)
{
    const Uint8 * state = SDL_GetKeyboardState(NULL);

    int a = state[SDL_SCANCODE_X];
    int b = state[SDL_SCANCODE_Z];
    int l = state[SDL_SCANCODE_A];
    int r = state[SDL_SCANCODE_S];
    int st = state[SDL_SCANCODE_RETURN];
    int se = state[SDL_SCANCODE_RSHIFT];
    int dr = state[SDL_SCANCODE_RIGHT];
    int dl = state[SDL_SCANCODE_LEFT];
    int du = state[SDL_SCANCODE_UP];
    int dd = state[SDL_SCANCODE_DOWN];

    GBA_HandleInput(a, b, l, r, st, se, dr, dl, du, dd);
}

int Input_Speedup_Enabled(void)
{
    const Uint8 * state = SDL_GetKeyboardState(NULL);

    return state[SDL_SCANCODE_SPACE];
}

//void GB_InputSet(int player, int a, int b, int st, int se, int r, int l, int u, int d);
//void GB_InputSetMBC7Joystick(int x, int y); // -200 to 200
//void GB_InputSetMBC7Buttons(int up, int down, int right, int left);
