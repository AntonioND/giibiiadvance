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
    GameBoy.Emulator.sys_clocks = 0;
    GameBoy.Emulator.timer_overflow_mask = -1;
    GameBoy.Emulator.timer_enabled = 0;

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
    //if(GameBoy.Memory.HighRAM[IE_REG-0xFF80] & flag)
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

void GB_TimersWriteTIMA(int reference_clocks, int value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int old_if = GameBoy.Memory.IO_Ports[IF_REG-0xFF00];

    GB_TimersUpdateClocksClounterReference(reference_clocks);

    if( (GameBoy.Emulator.sys_clocks&GameBoy.Emulator.timer_overflow_mask) == 0)
    {
        //If TIMA is written the same clock as incrementing itself prevent the timer IF flag from being set
        mem->IO_Ports[IF_REG-0xFF00] = old_if;
    }

    mem->IO_Ports[TIMA_REG-0xFF00] = value;
}

void GB_TimersWriteTMA(int reference_clocks, int value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    GB_TimersUpdateClocksClounterReference(reference_clocks);

    if( (GameBoy.Emulator.sys_clocks&GameBoy.Emulator.timer_overflow_mask) == 0)
    {
        //If TMA is written the same clock as reloading TIMA from TMA, load TIMA from written value
        if(mem->IO_Ports[TIMA_REG-0xFF00] == 0)
            mem->IO_Ports[TIMA_REG-0xFF00] = value;
    }

    mem->IO_Ports[TMA_REG-0xFF00] = value;
}

void GB_TimersUpdateClocksClounterReference(int reference_clocks)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int increment_clocks = reference_clocks - GB_TimersClockCounterGet();

    //Don't assume that this function will increase just a few clocks. Timer can be increased up to 4 times
    //just because of HDMA needing 64 clocks in double speed mode (min. timer period is 16 clocks).

    int old_sys_clocks = GameBoy.Emulator.sys_clocks;

    GameBoy.Emulator.sys_clocks = (increment_clocks+GameBoy.Emulator.sys_clocks)&0xFFFF;

    //DIV -- Upper 8 bits of the 16 register sys_clocks
    mem->IO_Ports[DIV_REG-0xFF00] = (GameBoy.Emulator.sys_clocks>>8);

    //TIMA
    if(GameBoy.Emulator.timer_enabled)
    {
        int timer_mask = GameBoy.Emulator.timer_overflow_mask;
        int timer_update_clocks = (old_sys_clocks & timer_mask) + increment_clocks;

        while(timer_update_clocks > timer_mask)
        {
            if(mem->IO_Ports[TIMA_REG-0xFF00] == 0xFF) //overflow
            {
                GB_SetInterrupt(I_TIMER);
                mem->IO_Ports[TIMA_REG-0xFF00] = mem->IO_Ports[TMA_REG-0xFF00];
            }
            else mem->IO_Ports[TIMA_REG-0xFF00]++;

            timer_update_clocks -= (timer_mask+1);
        }
    }

    //TODO : SOUND UPDATE PROBABLY GOES HERE!!

    GB_TimersClockCounterSet(reference_clocks);
}

int GB_TimersGetClocksToNextEvent(void)
{
    int clocks_to_next_event = 256 - (GameBoy.Emulator.sys_clocks&0xFF); // DIV

    if(GameBoy.Emulator.timer_enabled)
    {
        int timer_mask = GameBoy.Emulator.timer_overflow_mask;
        int timer_update_clocks = (GameBoy.Emulator.sys_clocks & timer_mask);
        int clocks_left_for_timer = (timer_mask+1) - timer_update_clocks;

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
