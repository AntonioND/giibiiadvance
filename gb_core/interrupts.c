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

#include <malloc.h>
#include <time.h>

#include "../build_options.h"

#include "gameboy.h"
#include "cpu.h"
#include "debug.h"
#include "memory.h"
#include "general.h"
#include "video.h"
#include "interrupts.h"
#include "gb_main.h"

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//----------------------------------------------------------------

void GB_HandleRTC(void)
{
    if(GameBoy.Emulator.HasTimer) //Handle time...
    {
        if(GameBoy.Emulator.Timer.halt == 0) //Increase timer...
        {
            GameBoy.Emulator.Timer.sec ++;
            if(GameBoy.Emulator.Timer.sec > 59)
            {
                GameBoy.Emulator.Timer.sec -= 60;

                GameBoy.Emulator.Timer.min ++;
                if(GameBoy.Emulator.Timer.min > 59)
                {
                    GameBoy.Emulator.Timer.min -= 60;

                    GameBoy.Emulator.Timer.hour ++;
                    if(GameBoy.Emulator.Timer.hour > 23)
                    {
                        GameBoy.Emulator.Timer.hour -= 24;

                        GameBoy.Emulator.Timer.days ++;
                        if(GameBoy.Emulator.Timer.days > 511)
                        {
                            GameBoy.Emulator.Timer.days -= 512;

                            GameBoy.Emulator.Timer.carry = 1;
                        }
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------

void GB_CPUInterruptsInit(void)
{
    GameBoy.Emulator.TimerClocks = 0;
    GameBoy.Emulator.timer_total_clocks = 0;
    GameBoy.Emulator.timer_enabled = 0;
    GameBoy.Emulator.DivClocks = 0;

    if(GameBoy.Emulator.HasTimer)
    {
        GameBoy.Emulator.LatchedTime.sec = 0;
        GameBoy.Emulator.LatchedTime.min = 0;
        GameBoy.Emulator.LatchedTime.hour = 0;
        GameBoy.Emulator.LatchedTime.days = 0;
        GameBoy.Emulator.LatchedTime.carry = 0; //GameBoy.Emulator.Timer is already loaded
        GameBoy.Emulator.LatchedTime.halt = 0; //GameBoy.Emulator.Timer.halt; //?
    }
}

void GB_CPUInterruptsEnd(void)
{
    // Nothing to do
}

//----------------------------------------------------------------

inline void GB_SetInterrupt(int flag)
{
    GameBoy.Memory.IO_Ports[IF_REG-0xFF00] |= flag;
    if(GameBoy.Memory.HighRAM[IE_REG-0xFF80] & flag)
        GameBoy.Emulator.CPUHalt = 0;
}

//----------------------------------------------------------------

static int gb_timer_clock_counter = 0;

inline void GB_TimersClockCounterReset(void)
{
    gb_timer_clock_counter = 0;
}

static inline int GB_TimersClockCounterGet(void)
{
    return gb_timer_clock_counter;
}

static inline void GB_TimersClockCounterSet(int new_reference_clocks)
{
    gb_timer_clock_counter = new_reference_clocks;
}

void GB_TimersUpdateClocksClounterReference(int reference_clocks)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int increment_clocks = reference_clocks - GB_TimersClockCounterGet();

    //DIV -- EVERY 256 CLOCKS
    GameBoy.Emulator.DivClocks += increment_clocks;
    while(GameBoy.Emulator.DivClocks >= 256)
    {
        mem->IO_Ports[DIV_REG-0xFF00] ++;
        GameBoy.Emulator.DivClocks -= 256;
    }

    //TIMA
    if(GameBoy.Emulator.timer_enabled)
    {
        GameBoy.Emulator.TimerClocks += increment_clocks;

        while(GameBoy.Emulator.TimerClocks >= GameBoy.Emulator.timer_total_clocks)
        {
            if(mem->IO_Ports[TIMA_REG-0xFF00] == 0xFF) //overflow
            {
                GB_SetInterrupt(I_TIMER);
                mem->IO_Ports[TIMA_REG-0xFF00] = mem->IO_Ports[TMA_REG-0xFF00];
            }
            else mem->IO_Ports[TIMA_REG-0xFF00]++;

            GameBoy.Emulator.TimerClocks -= GameBoy.Emulator.timer_total_clocks;
        }
    }

    GB_TimersClockCounterSet(reference_clocks);
}

int GB_TimersGetClocksToNextEvent(void)
{
    int clocks_to_next_event = 256 - GameBoy.Emulator.DivClocks; // DIV

    if(GameBoy.Emulator.timer_enabled)
    {
        int clocks_left_for_timer = GameBoy.Emulator.timer_total_clocks - GameBoy.Emulator.TimerClocks;
        if(clocks_left_for_timer < clocks_to_next_event)
            clocks_to_next_event = clocks_left_for_timer;
    }

    return clocks_to_next_event;
}

//----------------------------------------------------------------

void GB_CheckJoypadInterrupt(void)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    u32 i = mem->IO_Ports[P1_REG-0xFF00];
    u32 result = 0;

    int Keys = GB_Input_Get(0);
    if((i & (1<<5)) == 0) //A-B-SEL-STA
        result |= Keys & (KEY_A|KEY_B|KEY_SELECT|KEY_START);
    if((i & (1<<4)) == 0) //PAD
        result |= Keys & (KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT);

    if(result)
    {
        if( ! ( (GameBoy.Emulator.HardwareType == HW_GBC) || (GameBoy.Emulator.HardwareType == HW_GBA) ) )
            GB_SetInterrupt(I_JOYPAD); //No joypad interrupt in GBA/GBC? TODO: TEST

        if(GameBoy.Emulator.CPUHalt == 2) // Exit stop mode in any hardware
            GameBoy.Emulator.CPUHalt = 0;
    }

    return;
}

//----------------------------------------------------------------
//----------------------------------------------------------------
