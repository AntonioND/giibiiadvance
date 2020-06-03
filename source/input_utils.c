// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "debug_utils.h"
#include "input_utils.h"

#include "gb_core/gb_main.h"
#include "gb_core/sgb.h"

#include "gba_core/gba.h"

//------------------------------------------------------------------------------

typedef struct
{
    int is_opened;
    SDL_Joystick *joystick;
    char name[50]; // Should be more than enough to identify a joystick
    SDL_Haptic *haptic;
} _joystick_info_;

static _joystick_info_ Joystick[4]; // Only 4 joysticks at a time
static int joystick_number;

//------------------------------------------------------------------------------

typedef struct
{
    int index;
    int enabled;
} _controller_player_info_;

static _controller_player_info_ ControllerPlayerInfo[4] = {
    { -1, 1 }, // keyboard, enabled
    { 0, 0 },  // controller 0, disabled
    { 1, 0 },  // controller 1, disabled
    { 2, 0 }   // controller 2, disabled
};

// Default to keyboard the first player, the rest to unused

static SDL_Scancode _player_key_[4][P_NUM_KEYS] = {
    // This is the default configuration, changed when loading the config file
    // (if it exists). Each value can be a SDLK_* define or a joystick button
    // number

    // A, B, L, R, UP, RIGHT, DOWN, LEFT, START, SELECT,
    { SDLK_x, SDLK_z, SDLK_a, SDLK_s, SDLK_UP, SDLK_RIGHT, SDLK_DOWN, SDLK_LEFT,
      SDLK_RETURN, SDLK_RSHIFT },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
};

static SDL_Scancode player_key_speedup = SDLK_SPACE;

const char *GBKeyNames[P_NUM_KEYS] = {
    "A", "B", "L", "R", "Up", "Right", "Down", "Left", "Start", "Select"
};

//------------------------------------------------------------------------------

// -1 = keyboard, others = number of joystick
void Input_PlayerSetController(int player, int index)
{
    if (player < 0)
        return;
    if (player >= 4)
        return;

    ControllerPlayerInfo[player].index = index;
}

int Input_PlayerGetController(int player)
{
    if (player < 0)
        return -1;
    if (player >= 4)
        return -1;

    return ControllerPlayerInfo[player].index;
}

//----------------------------

void Input_PlayerSetEnabled(int player, int enabled)
{
    if (player < 0)
        return;
    if (player >= 4)
        return;

    ControllerPlayerInfo[player].enabled = enabled;
}

int Input_PlayerGetEnabled(int player)
{
    if (player < 0)
        return 0;
    if (player >= 4)
        return 0;

    return ControllerPlayerInfo[player].enabled;
}

//----------------------------

void Input_ControlsSetKey(int player, _key_config_enum_ keyindex,
                          SDL_Scancode keyscancode)
{
    if (keyindex == P_KEY_NONE)
        return;
    if (keyindex == P_NUM_KEYS)
        return;

    if (keyindex == P_KEY_SPEEDUP)
        player_key_speedup = keyscancode;
    else
        _player_key_[player][keyindex] = keyscancode;
}

SDL_Scancode Input_ControlsGetKey(int player, _key_config_enum_ keyindex)
{
    if (keyindex == P_KEY_NONE)
        return 0;
    if (keyindex == P_NUM_KEYS)
        return 0;

    if (keyindex == P_KEY_SPEEDUP)
        return player_key_speedup;

    return _player_key_[player][keyindex];
}

//------------------------------------------------------------------------------

int Input_IsGameBoyKeyPressed(int player, _key_config_enum_ keyindex)
{
    if (ControllerPlayerInfo[player].enabled == 0)
        return 0;

    if (ControllerPlayerInfo[player].index == -1) // Keyboard
    {
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        int key = Input_ControlsGetKey(player, keyindex);
        if (key == 0)
            return 0;

        SDL_Scancode code = Input_ControlsGetKey(player, keyindex);
        return state[SDL_GetScancodeFromKey(code)];
    }
    else // Joystick
    {
        int joystick_index = ControllerPlayerInfo[player].index;
        int btn = Input_ControlsGetKey(player, keyindex);
        SDL_Joystick *joystick = Joystick[joystick_index].joystick;

        if (btn & KEYCODE_IS_AXIS) // Axis
        {
            btn &= ~KEYCODE_IS_AXIS; // Remove axis flag

            int is_btn_positive = (btn & KEYCODE_POSITIVE_AXIS);
            btn &= ~KEYCODE_POSITIVE_AXIS; // Remove positive flag

            int axis_value = SDL_JoystickGetAxis(joystick, btn);

            if (is_btn_positive && (axis_value > (16 * 1024)))
                return 1;
            else if ((!is_btn_positive) && (axis_value < (-16 * 1024)))
                return 1;
            else
                return 0;
        }
        else if (btn & KEYCODE_IS_HAT) // Axis
        {
            btn &= ~KEYCODE_IS_HAT; // Remove hat flag

            int position = (btn >> 4);
            int hat_index = btn & 0xF;

            if (position & SDL_JoystickGetHat(joystick, hat_index))
                return 1;
            else
                return 0;
        }
        else // Button
        {
            if (btn != -1)
                return SDL_JoystickGetButton(joystick, btn);
            else
                return 0;
        }
    }

    return 0;
}

void Input_GetKeyboardElementName(char *name, int namelen, int btncode)
{
    if (btncode <= 0)
    {
        s_strncpy(name, "None", namelen);
    }
    else
    {
        s_strncpy(name, SDL_GetKeyName(btncode), namelen);
    }
}

void Input_GetJoystickElementName(char *name, int namelen, int btncode)
{
    if (btncode == -1)
    {
        s_strncpy(name, "None", namelen);
    }
    else if (btncode & KEYCODE_IS_AXIS) // Axis
    {
        btncode &= ~KEYCODE_IS_AXIS; // Remove axis flag

        int is_btn_positive = (btncode & KEYCODE_POSITIVE_AXIS);
        btncode &= ~KEYCODE_POSITIVE_AXIS; // Remove positive flag

        snprintf(name, namelen, "Axis %d%c", btncode,
                 is_btn_positive ? '+' : '-');
    }
    else if (btncode & KEYCODE_IS_HAT) // Axis
    {
        btncode &= ~KEYCODE_IS_HAT; // Remove hat flag

        int position = (btncode >> 4);
        int hat_index = btncode & 0xF;

        char *dir = "Centered [!]";
        if (position & SDL_HAT_UP)
            dir = "Up";
        else if (position & SDL_HAT_RIGHT)
            dir = "Right";
        else if (position & SDL_HAT_DOWN)
            dir = "Down";
        else if (position & SDL_HAT_LEFT)
            dir = "Left";

        snprintf(name, namelen, "Hat %d %s", hat_index, dir);
    }
    else // Button
    {
        snprintf(name, namelen, "Button %d", btncode);
    }
}

//------------------------------------------------------------------------------

void Input_Update_GB(void)
{
    int players = 1;

    if (SGB_MultiplayerIsEnabled())
        players = 4;

    for (int i = 0; i < players; i++)
    {
        int a = Input_IsGameBoyKeyPressed(i, P_KEY_A);
        int b = Input_IsGameBoyKeyPressed(i, P_KEY_B);
        int st = Input_IsGameBoyKeyPressed(i, P_KEY_START);
        int se = Input_IsGameBoyKeyPressed(i, P_KEY_SELECT);
        int dr = Input_IsGameBoyKeyPressed(i, P_KEY_RIGHT);
        int dl = Input_IsGameBoyKeyPressed(i, P_KEY_LEFT);
        int du = Input_IsGameBoyKeyPressed(i, P_KEY_UP);
        int dd = Input_IsGameBoyKeyPressed(i, P_KEY_DOWN);

        GB_InputSet(i, a, b, st, se, dr, dl, du, dd);
    }

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    int accr = state[SDL_SCANCODE_KP_6];
    int accl = state[SDL_SCANCODE_KP_4];
    int accu = state[SDL_SCANCODE_KP_8];
    int accd = state[SDL_SCANCODE_KP_2];

    GB_InputSetMBC7Buttons(accu, accd, accr, accl);
    //void GB_InputSetMBC7Joystick(int x, int y); // -200 to 200
    //void GB_InputSetMBC7Buttons(int up, int down, int right, int left);
}

void Input_Update_GBA(void)
{
    int a = Input_IsGameBoyKeyPressed(0, P_KEY_A);
    int b = Input_IsGameBoyKeyPressed(0, P_KEY_B);
    int l = Input_IsGameBoyKeyPressed(0, P_KEY_L);
    int r = Input_IsGameBoyKeyPressed(0, P_KEY_R);
    int st = Input_IsGameBoyKeyPressed(0, P_KEY_START);
    int se = Input_IsGameBoyKeyPressed(0, P_KEY_SELECT);
    int dr = Input_IsGameBoyKeyPressed(0, P_KEY_RIGHT);
    int dl = Input_IsGameBoyKeyPressed(0, P_KEY_LEFT);
    int du = Input_IsGameBoyKeyPressed(0, P_KEY_UP);
    int dd = Input_IsGameBoyKeyPressed(0, P_KEY_DOWN);

    GBA_HandleInput(a, b, l, r, st, se, dr, dl, du, dd);
}

int Input_Speedup_Enabled(void)
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    return state[SDL_SCANCODE_SPACE];
}

//------------------------------------------------------------------------------

SDL_Joystick *Input_GetJoystick(int index)
{
    if (index >= 4)
        return NULL;
    if (index < 0)
        return NULL;

    return Joystick[index].joystick;
}

char *Input_GetJoystickName(int index)
{
    if (index >= 4)
        return NULL;
    if (index < 0)
        return NULL;

    return Joystick[index].name;
}

int Input_GetJoystickFromName(char *name)
{
    for (int i = 0; i < 4; i++)
    {
        if (strncmp(name, Joystick[i].name, sizeof(Joystick[i].name)) == 0)
            return i;
    }

    if (strncmp(name, "Keyboard", strlen("Keyboard")) == 0)
        return -1;

    return -2;
}

static void _Input_CloseSystem(void)
{
    for (int i = 0; i < 4; i++)
    {
        if (!Joystick[i].is_opened)
            continue;

        Joystick[i].is_opened = 0;
        SDL_JoystickClose(Joystick[i].joystick);
        Joystick[i].joystick = NULL;

        if (Joystick[i].haptic)
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

    for (int i = 0; i < 4; i++) // Clear things
    {
        Joystick[i].is_opened = 0;
        Joystick[i].joystick = NULL;
    }

    joystick_number = SDL_NumJoysticks();

    for (int i = 0; i < 4; i++) // Load joysticks
    {
        // If there are no more joysticks, exit
        if (i >= SDL_NumJoysticks())
            break;

        Joystick[i].joystick = SDL_JoystickOpen(i);
        Joystick[i].is_opened = (Joystick[i].joystick != NULL);

        const char *name = SDL_JoystickNameForIndex(i);

        if (name)
            s_strncpy(Joystick[i].name, name, sizeof(Joystick[i].name));
        else
            s_strncpy(Joystick[i].name, "Unknown Joystick",
                      sizeof(Joystick[i].name));

        // Not available
        if (Joystick[i].joystick == NULL)
            continue;

        // Create rumble functions

        //Joystick[i].haptic = SDL_HapticOpen(0);
        Joystick[i].haptic = SDL_HapticOpenFromJoystick(Joystick[i].joystick);
        if (Joystick[i].haptic == NULL)
        {
            Debug_LogMsgArg("Couldn't open haptic for joystick %d: %s", i,
                            SDL_GetError());
            continue;
        }

        if (!SDL_HapticRumbleSupported(Joystick[i].haptic))
        {
            Debug_LogMsgArg("Rumble not supported by joystick %d", i);
            SDL_HapticClose(Joystick[i].haptic);
            Joystick[i].haptic = (void *)-1;
            continue;
        }

        if (SDL_HapticRumbleInit(Joystick[i].haptic) != 0)
        {
            Debug_LogMsgArg("SDL_HapticRumbleInit() error joystick %d: %s", i,
                            SDL_GetError());
            SDL_HapticClose(Joystick[i].haptic);
            Joystick[i].haptic = (void *)-1;
            continue;
        }

        // Rumble for a bit to tell the user that everything is OK.
        SDL_HapticRumblePlay(Joystick[i].haptic, 0.5, 500);
    }
}

void Input_RumbleEnable(void)
{
    int controller = ControllerPlayerInfo[0].index; // Player 0 only

    if (controller >= 0) // Rumble for 16 ms (one frame)
        SDL_HapticRumblePlay(Joystick[controller].haptic, 1.0, 16);
}

int Input_JoystickHasRumble(int index)
{
    return Joystick[index].haptic != NULL;
}
