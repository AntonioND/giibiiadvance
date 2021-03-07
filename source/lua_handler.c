// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <SDL2/SDL.h>

#ifdef ENABLE_LUA
# include <lua.h>
# include <lualib.h>
# include <lauxlib.h>
#endif

#include "gba_core/gba.h"
#include "gba_core/sound.h"

#include "gui/win_main.h"

#include "debug_utils.h"
#include "general_utils.h"
#include "sound_utils.h"
#include "wav_utils.h"
#include "window_handler.h"

#ifdef ENABLE_LUA
static char *script_path = NULL;
static SDL_Thread *script_thread;
static int script_running = 0;

static uint16_t lua_keyinput = 0;

// ----------------------------------------------------------------------------

static int lua_run_frames_and_pause(lua_State *L)
{
    // Number of arguments
    int narg = lua_gettop(L);
    if (narg != 1)
    {
        Debug_LogMsgArg("%s(): Invalid number of arguments: %d", __func__, narg);
        return 0;
    }

    // Get argument of the function and remove it from the stack
    lua_Integer frames = lua_tointeger(L, -1);
    lua_pop(L, 1);

    Debug_LogMsgArg("%s(%lld)", __func__, frames);

    for (int i = 0; i < frames; i++)
    {
        if (Win_MainRunningGBA())
        {
            GBA_SoundResetBufferPointers();
            GBA_RunForOneFrame();
            GBA_SoundSaveToWAV();
        }
    }

    // Number of results
    return 0;
}

static int lua_screenshot(lua_State *L)
{
    // Number of arguments
    int narg = lua_gettop(L);
    if (narg == 0)
    {
        Debug_LogMsgArg("%s()", __func__);
        GBA_Screenshot("screenshot.png");
    }
    else if (narg == 1)
    {
        const char *name = lua_tostring(L, -1);

        Debug_LogMsgArg("%s(%s)", __func__, name);
        GBA_Screenshot(name);

        lua_pop(L, 1);
    }
    else
    {
        Debug_LogMsgArg("%s(): Invalid number of arguments: %d", __func__, narg);
        return 0;
    }

    // Number of results
    return 0;
}

uint16_t get_bit_from_key_name(const char *name)
{
    struct {
        const char *name;
        uint16_t mask;
    } keyinfo[10] = {
        { "A", BIT(0) },
        { "B", BIT(1) },
        { "SELECT", BIT(2) },
        { "START", BIT(3) },
        { "RIGHT", BIT(4) },
        { "LEFT", BIT(5) },
        { "UP", BIT(6) },
        { "DOWN", BIT(7) },
        { "R", BIT(8) },
        { "L", BIT(9) },
    };

    for (int i = 0; i < 10; i++)
    {
        if (strcmp(keyinfo[i].name, name) == 0)
            return keyinfo[i].mask;
    }

    Debug_LogMsgArg("%s: Unknown name: %s", __func__, name);
    return 0;
}

static int lua_keys_hold(lua_State *L)
{
    uint16_t keys = 0;

    Debug_LogMsgArg("%s()", __func__);

    // Number of arguments
    int narg = lua_gettop(L);

    for (int i = 0; i < narg; i++)
    {
        const char *name = lua_tostring(L, -1);

        Debug_LogMsgArg("    %s", name);

        keys |= get_bit_from_key_name(name);

        lua_pop(L, 1);
    }

    lua_keyinput |= keys;

    GBA_HandleInputFlags(lua_keyinput);

    // Number of results
    return 0;
}

static int lua_keys_release(lua_State *L)
{
    uint16_t keys = 0;

    Debug_LogMsgArg("%s()", __func__);

    // Number of arguments
    int narg = lua_gettop(L);

    for (int i = 0; i < narg; i++)
    {
        const char *name = lua_tostring(L, -1);

        Debug_LogMsgArg("    %s", name);

        keys |= get_bit_from_key_name(name);

        lua_pop(L, 1);
    }

    lua_keyinput &= ~keys;

    GBA_HandleInputFlags(lua_keyinput);

    // Number of results
    return 0;
}

static int lua_wav_record_start(lua_State *L)
{
    // Number of arguments
    int narg = lua_gettop(L);
    if (narg == 0)
    {
        Debug_LogMsgArg("%s()", __func__);
        WAV_FileStart(NULL, GBA_SAMPLERATE);
    }
    else if (narg == 1)
    {
        const char *name = lua_tostring(L, -1);

        Debug_LogMsgArg("%s(%s)", __func__, name);
        WAV_FileStart(name, GBA_SAMPLERATE);

        lua_pop(L, 1);
    }
    else
    {
        Debug_LogMsgArg("%s(): Invalid number of arguments: %d", __func__,
                        narg);
        return 0;
    }

    // Number of results
    return 0;
}

static int lua_wav_record_end(lua_State *L)
{
    // Number of arguments
    int narg = lua_gettop(L);
    if (narg != 0)
    {
        Debug_LogMsgArg("%s(): Invalid number of arguments: %d", __func__,
                        narg);
        return 0;
    }

    Debug_LogMsgArg("%s()", __func__);

    WAV_FileEnd();

    // Number of results
    return 0;
}

static int lua_exit(lua_State *L)
{
    // Number of arguments
    int narg = lua_gettop(L);
    if (narg != 0)
    {
        Debug_LogMsgArg("%s(): Invalid number of arguments: %d", __func__, narg);
        return 0;
    }

    Debug_LogMsgArg("%s()", __func__);

    WH_CloseAll();

    // Number of results
    return 0;
}

// ----------------------------------------------------------------------------

static int Script_Runner(unused__ void *ptr)
{
    int ret = 1;

    // Create Lua state
    lua_State *L = luaL_newstate();

    // Load Lua libraries
    luaL_openlibs(L);

    // Load script from file
    int status = luaL_loadfile(L, script_path);
    if (status) {
        // On error, the error message is at the top of the stack
        fprintf(stderr, "Couldn't load file: %s", lua_tostring(L, -1));
        goto exit;
    }

    // Register C functions
    lua_register(L, "run_frames_and_pause", lua_run_frames_and_pause);
    lua_register(L, "screenshot", lua_screenshot);
    lua_register(L, "keys_hold", lua_keys_hold);
    lua_register(L, "keys_release", lua_keys_release);
    lua_register(L, "wav_record_start", lua_wav_record_start);
    lua_register(L, "wav_record_end", lua_wav_record_end);
    lua_register(L, "exit", lua_exit);

    while (!Win_MainRunningGBA() && !Win_MainRunningGB())
        SDL_Delay(0);

    // GB not supported for now
    if (Win_MainRunningGB())
        return 0;

    // Run script with 0 arguments and expect one return value
    int result = lua_pcall(L, 0, 1, 0);
    if (result) {
        fprintf(stderr, "Failed to run script: %s", lua_tostring(L, -1));
        goto exit;
    }

    // Get returned value and remove it from the stack (it's at the top)
    int retval  = lua_tointeger(L, -1);
    lua_pop(L, 1);

    Debug_LogMsgArg("Script returned: %d", retval);

    // Close Lua
    lua_close(L);

    script_running = 0;

    ret = 0;
exit:
    return ret;
}

int Script_IsRunning(void)
{
    return script_running;
}

int Script_RunLua(const char *path)
{
    if (path == NULL)
        return 1;

    size_t len = strlen(path);

    script_path = malloc(len + 1);
    if (script_path == NULL)
        return 1;

    snprintf(script_path, len + 1, "%s", path);

    //is_waiting = 1;

    script_thread = SDL_CreateThread(Script_Runner, "Script Runner", NULL);
    if (script_thread == NULL)
    {
        Debug_LogMsgArg("SDL_CreateThread failed: %s", SDL_GetError());
        return 1;
    }

    script_running = 1;

    return 0;
}

void Script_WaitEnd(void)
{
    int return_status;

    SDL_WaitThread(script_thread, &return_status);

    if (return_status != 0)
        Debug_LogMsgArg("%s: Thread returned with: %d", __func__, return_status);

    free(script_path);

    script_running = 0;
}

#else // ENABLE_LUA

int Script_IsRunning(void)
{
    return 0;
}

int Script_RunLua(unused__ const char *path)
{
    Debug_LogMsgArg("Lua support not present!");

    return 0;
}

void Script_WaitEnd(void)
{
    return;
}

#endif // ENABLE_LUA
