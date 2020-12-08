// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdlib.h>

#include <SDL2/SDL.h>

#include "config.h"
#include "debug_utils.h"
#include "file_utils.h"
#include "font_utils.h"
#include "input_utils.h"
#include "lua_handler.h"
#include "sound_utils.h"
#include "window_handler.h"

#include "gui/win_main.h"

static int Init(void)
{
    WH_Init();

    // Initialize SDL
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO |
                 SDL_INIT_EVENTS) != 0)
    {
        Debug_LogMsgArg("SDL could not initialize! SDL Error: %s\n",
                        SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    // Try to init joystick and haptic, but don't abort if it fails
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0)
    {
        Debug_LogMsgArg("SDL could not initialize joystick! SDL Error: %s\n",
                        SDL_GetError());
    }
    else
    {
        // If joystick inited, try haptic
        if (SDL_InitSubSystem(SDL_INIT_HAPTIC) != 0)
        {
            Debug_LogMsgArg("SDL could not initialize haptic! SDL Error: %s\n",
                            SDL_GetError());
        }
    }

    // Init this before loading the configuration
    Input_InitSystem();

    Config_Load();
    atexit(Config_Save);

    // Enable VSync
#if 0
    if (!SDL_SetHint( SDL_HINT_RENDER_VSYNC, "1"))
    {
        Debug_LogMsgArg("Warning: VSync not enabled!");
    }
#endif

    Sound_Init();

    if (DirCheckExistence(DirGetScreenshotFolderPath()) == 0)
        DirCreate(DirGetScreenshotFolderPath());

    if (DirCheckExistence(DirGetBiosFolderPath()) == 0)
        DirCreate(DirGetBiosFolderPath());

    return 0;
}

#define FLOAT_MS_PER_FRAME ((double)1000.0 / (double)60.0)

int main(int argc, char *argv[])
{
    // Try to get the path where the binary is running from. Try with SDL's
    // helper, but fallback to argv[0] if it doesn't work or it's not available.
#if SDL_VERSION_ATLEAST(2, 0, 1)
    char *base_path = SDL_GetBasePath();
    if (base_path)
    {
        DirSetRunningPath(base_path);
        SDL_free(base_path);
    }
    else
    {
        DirSetRunningPath(argv[0]);
    }
#else
    if (argc > 0)
        DirSetRunningPath(argv[0]);
#endif // SDL_VERSION_ATLEAST

    Debug_Init();
    atexit(Debug_End);

    if (Init() != 0)
        return 1;

    // Check if the user provided a script

    if (argc > 2)
    {
        if (strcmp(argv[1], "--lua") == 0)
        {
            Script_RunLua(argv[2]);

            // Remove argv[1] and argv[2]

            for (int i = 1; i < argc - 2; i++)
                argv[i] = argv[i + 2];

            argc = argc - 2;
        }
    }

    // Load main window with the ROM provided as argument

    Win_MainCreate((argc > 1) ? argv[1] : NULL);

    double waitforticks = 0;

    while (!WH_AreAllWindowsClosed())
    {
        // Handle events for all windows
        WH_HandleEvents();

        Win_MainLoopHandle();

        // Render main window every frame
        Win_MainRender();

        // Synchronise video
        if (Input_Speedup_Enabled())
        {
            SDL_Delay(0);
        }
        else
        {
            while (waitforticks >= SDL_GetTicks())
                SDL_Delay(1);

            int ticksnow = SDL_GetTicks();

            // If the emulator missed a frame or more, adjust next frame
            if (waitforticks < (ticksnow - FLOAT_MS_PER_FRAME))
                waitforticks = ticksnow + FLOAT_MS_PER_FRAME;
            else
                waitforticks += FLOAT_MS_PER_FRAME;
        }
    }

    return 0;
}
