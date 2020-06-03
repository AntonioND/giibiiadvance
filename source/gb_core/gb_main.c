// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../debug_utils.h"
#include "../file_utils.h"
#include "../general_utils.h"

#include "cpu.h"
#include "gameboy.h"
#include "gb_main.h"
#include "general.h"
#include "interrupts.h"
#include "rom.h"
#include "sgb.h"
#include "sound.h"
#include "video.h"

extern _GB_CONTEXT_ GameBoy;

int GB_Input_Get(int player);
void GB_Input_Update(void);

//---------------------------------

int GB_ROMLoad(const char *rom_path)
{
    void *ptr;
    u32 size;

    FileLoad(rom_path, &ptr, &size);

    if (ptr == NULL)
    {
        Debug_ErrorMsgArg("Couldn't load data from %s.", rom_path);
        return 0;
    }

    if (GB_CartridgeLoad(ptr, size) == 0)
    {
        Debug_ErrorMsgArg("Error while loading cartridge.\n"
                          "Read the console output for details.");
        free(ptr);
        return 0;
    }

    // Init after loading the cartridge to set the hardware type value and allow
    // GB_Screen_Init() choose the correct dimensions for the texture.

    GB_Cardridge_Set_Filename((char *)rom_path);

    GB_SRAM_Load();

    GB_PowerOn();
    GB_SkipFrame(0);

    return 1;
}

void GB_End(int save)
{
    if (save)
        GB_SRAM_Save();

    GB_PowerOff();
    GB_Cartridge_Unload();
}

//---------------------------------------------------------------------------

int GB_IsEnabledSGB(void)
{
    return GameBoy.Emulator.SGBEnabled;
}

//---------------------------------------------------------------------------

void GB_RunForOneFrame(void)
{
    GB_CheckJoypadInterrupt();
    GB_RunFor(70224 << GameBoy.Emulator.DoubleSpeed);
}

//---------------------------------------------------------------------------

static int Keys[4];

void GB_InputSet(int player, int a, int b, int st, int se,
                 int r, int l, int u, int d)
{
    Keys[player] = 0;
    if (l)
        Keys[player] |= KEY_LEFT;
    if (u)
        Keys[player] |= KEY_UP;
    if (r)
        Keys[player] |= KEY_RIGHT;
    if (d)
        Keys[player] |= KEY_DOWN;
    if (a)
        Keys[player] |= KEY_A;
    if (b)
        Keys[player] |= KEY_B;
    if (st)
        Keys[player] |= KEY_START;
    if (se)
        Keys[player] |= KEY_SELECT;
}

void GB_InputSetMBC7(int x, int y)
{
    GameBoy.Emulator.MBC7.sensorY = 2047 + y;
    GameBoy.Emulator.MBC7.sensorX = 2047 + x;
}

void GB_InputSetMBC7Buttons(int up, int down, int right, int left)
{
    if (GameBoy.Emulator.MemoryController != MEM_MBC7)
        return;

    if (up)
    {
        if (GameBoy.Emulator.MBC7.sensorY < 2047 - 50)
            GameBoy.Emulator.MBC7.sensorY += 100;
        else if (GameBoy.Emulator.MBC7.sensorY < 2047 + 100)
            GameBoy.Emulator.MBC7.sensorY += 2;
    }
    else if (down)
    {
        if (GameBoy.Emulator.MBC7.sensorY > 2047 + 50)
            GameBoy.Emulator.MBC7.sensorY -= 100;
        else if (GameBoy.Emulator.MBC7.sensorY > 2047 - 100)
            GameBoy.Emulator.MBC7.sensorY -= 2;
    }
    else
    {
        if (GameBoy.Emulator.MBC7.sensorY > 2047)
            GameBoy.Emulator.MBC7.sensorY -= 1;
        else
            GameBoy.Emulator.MBC7.sensorY += 1;
    }

    if (left)
    {
        if (GameBoy.Emulator.MBC7.sensorX < 2047 + 100)
            GameBoy.Emulator.MBC7.sensorX += 2;
    }
    else if (right)
    {
        if (GameBoy.Emulator.MBC7.sensorX > 2047 - 100)
            GameBoy.Emulator.MBC7.sensorX -= 2;
    }
    else
    {
        if (GameBoy.Emulator.MBC7.sensorX > 2047)
            GameBoy.Emulator.MBC7.sensorX -= 1;
        else
            GameBoy.Emulator.MBC7.sensorX += 1;
    }
}

int GB_Input_Get(int player)
{
    return Keys[player];
}
