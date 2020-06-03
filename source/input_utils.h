// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef INPUT_UTILS__
#define INPUT_UTILS__

#include <SDL2/SDL.h>

typedef enum
{
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

#define KEYCODE_POSITIVE_AXIS (1 << 8)
#define KEYCODE_IS_AXIS       (1 << 9)
#define KEYCODE_IS_HAT        (1 << 10)

extern const char *GBKeyNames[P_NUM_KEYS];

void Input_ControlsSetKey(int player, _key_config_enum_ keyindex,
                          SDL_Scancode keyscancode);
SDL_Scancode Input_ControlsGetKey(int player, _key_config_enum_ keyindex);

// The index is the number of the joystick. The keyboard is -1.
void Input_PlayerSetController(int player, int index);
int Input_PlayerGetController(int player);

void Input_PlayerSetEnabled(int player, int enabled);
int Input_PlayerGetEnabled(int player);

//------------------------------------------------------------------------------

void Input_Update_GB(void);
void Input_Update_GBA(void);

int Input_Speedup_Enabled(void);

//-----------------------------------------------------------------------------

// btncode is a SDL_Scancode
void Input_GetKeyboardElementName(char *name, int namelen, int btncode);
void Input_GetJoystickElementName(char *name, int namelen, int btncode);

//------------------------------------------------------------------------------

SDL_Joystick *Input_GetJoystick(int index);
char *Input_GetJoystickName(int index);
int Input_GetJoystickNumber(void);

// Returns: -1 = keyboard, 0-3 = controller index, -2 = error
int Input_GetJoystickFromName(char *name);

// This function only works with joysticks that are connected when opening the
// emulator.
void Input_InitSystem(void);

void Input_RumbleEnable(void);
// Returns -1 if error opening haptic, 0 if there isn't haptic, if not, correct.
int Input_JoystickHasRumble(int index);

#endif // INPUT_UTILS__
