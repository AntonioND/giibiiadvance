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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../build_options.h"
#include "../general_utils.h"
#include "../file_utils.h"
#include "../debug_utils.h"

#include "gameboy.h"
#include "rom.h"
#include "general.h"
#include "video.h"
#include "cpu.h"
#include "sound.h"
#include "sgb.h"
#include "gb_main.h"
#include "interrupts.h"

extern _GB_CONTEXT_ GameBoy;

int GB_Input_Get(int player);
void GB_Input_Update(void);

//---------------------------------

int GB_ROMLoad(const char * rom_path)
{
    void * ptr; u32 size;
    FileLoad(rom_path,&ptr,&size);

    if(ptr)
    {
        if(GB_CartridgeLoad(ptr,size))
        {
            //Init after loading the cartridge to set the hardware type value and allow
            //GB_Screen_Init choose the correct dimensions for the texture.

            GB_Cardridge_Set_Filename((char*)rom_path);

            GB_SRAM_Load();
            GB_RTC_Load();
            GB_PowerOn();
            GB_SkipFrame(0);
            return 1;
        }

        Debug_ErrorMsgArg("Error while loading cartridge.\n"
                    "Read the console output for details.");

        free(ptr);
        return 0;
    }

    Debug_ErrorMsgArg("Couldn't load data from %s.",rom_path);

    return 0;
}

void GB_End(int save)
{
    if(save)
    {
        GB_SRAM_Save();
        GB_RTC_Save();
    }

    GB_PowerOff();
    GB_Cartridge_Unload();
}

//---------------------------------------------------------------------------

int GB_IsEnabledSGB(void)
{
    return GameBoy.Emulator.SGBEnabled;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void GB_RunForOneFrame(void)
{
    GB_CheckJoypadInterrupt();
    GB_RunFor(70224 << GameBoy.Emulator.DoubleSpeed);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

int Keys[4];

void GB_InputSet(int player, int a, int b, int st, int se, int r, int l, int u, int d)
{
    Keys[player] = 0;
    if(l) Keys[player] |= KEY_LEFT;
    if(u) Keys[player] |= KEY_UP;
    if(r) Keys[player] |= KEY_RIGHT;
    if(d) Keys[player] |= KEY_DOWN;
    if(a) Keys[player] |= KEY_A;
    if(b) Keys[player] |= KEY_B;
    if(st) Keys[player] |= KEY_START;
    if(se) Keys[player] |= KEY_SELECT;
}

void GB_InputSetMBC7(int x, int y)
{
    GameBoy.Emulator.MBC7.sensorY = 2047 + y;
    GameBoy.Emulator.MBC7.sensorX = 2047 + x;
}

void GB_InputSetMBC7Buttons(int up, int down, int right, int left)
{
    if(GameBoy.Emulator.MemoryController != MEM_MBC7)
        return;

    if(up)
    {
        if(GameBoy.Emulator.MBC7.sensorY < 2047 - 50)
            GameBoy.Emulator.MBC7.sensorY += 100;
        else if(GameBoy.Emulator.MBC7.sensorY < 2047 + 100)
            GameBoy.Emulator.MBC7.sensorY += 2;
    }
    else if(down)
    {
        if(GameBoy.Emulator.MBC7.sensorY > 2047 + 50)
            GameBoy.Emulator.MBC7.sensorY -= 100;
        else if(GameBoy.Emulator.MBC7.sensorY > 2047 - 100)
            GameBoy.Emulator.MBC7.sensorY -= 2;
    }
    else
    {
        if(GameBoy.Emulator.MBC7.sensorY > 2047) GameBoy.Emulator.MBC7.sensorY -= 1;
        else GameBoy.Emulator.MBC7.sensorY += 1;
    }

    if(left)
    {
        if(GameBoy.Emulator.MBC7.sensorX < 2047 + 100)
            GameBoy.Emulator.MBC7.sensorX += 2;
    }
    else if(right)
    {
        if(GameBoy.Emulator.MBC7.sensorX > 2047 - 100)
            GameBoy.Emulator.MBC7.sensorX -= 2;
    }
    else
    {
        if(GameBoy.Emulator.MBC7.sensorX > 2047) GameBoy.Emulator.MBC7.sensorX -= 1;
        else GameBoy.Emulator.MBC7.sensorX += 1;
    }
}

int GB_Input_Get(int player)
{
    return Keys[player];
}


