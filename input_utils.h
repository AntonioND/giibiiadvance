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

#ifndef __INPUT_UTILS__
#define __INPUT_UTILS__

//----------------------------------------------------------------------------------

typedef enum {

    P_KEY_A,
    P_KEY_B,
    P_KEY_L,
    P_KEY_R,
    P_KEY_UP,
    P_KEY_RIGHT,
    P_KEY_DOWN,
    P_KEY_LEFT,
    P_KEY_START,
    P_KEY_SELECT,

    P_NUM_KEYS,

    P_KEY_SPEEDUP,

    P_KEY_NONE

} _key_config_enum_;

extern const char * GBKeyNames[P_NUM_KEYS];

void Input_ControlsSetKey(int player, _key_config_enum_ keyindex, SDL_Scancode keyscancode);
SDL_Scancode Input_ControlsGetKey(int player, _key_config_enum_ keyindex);

void Input_PlayerSetController(int player, int index); // -1 = keyboard, others = number of joypad
int Input_PlayerGetController(int player);

void Input_PlayerSetEnabled(int player, int enabled);
int Input_PlayerGetEnabled(int player);

//----------------------------------------------------------------------------------

void Input_Update_GB(void);
void Input_Update_GBA(void);

int Input_Speedup_Enabled(void);

//----------------------------------------------------------------------------------

SDL_Joystick * Input_GetJoystick(int index);
char * Input_GetJoystickName(int index);
int Input_GetJoystickNumber(void);

void Input_InitSystem(void); // only work with joypads that are connected when opening the emulator

//----------------------------------------------------------------------------------

#endif // __INPUT_UTILS__


