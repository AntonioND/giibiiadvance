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
#include "../debug_utils.h"

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

#define SHARED_BITS_TIMA_DIV (5)
#define SHARED_TIMA_DIV_COUNT_MAX (1<<SHARED_BITS_TIMA_DIV)
#define SHARED_TIMA_DIV_COUNT_MASK (SHARED_TIMA_DIV_COUNT_MAX-1)

#define TIMER_COUNT_MAX_MASK (1024-1)

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
    GameBoy.Memory.InterruptMasterEnable = 0;

    GameBoy.Memory.IO_Ports[IF_REG-0xFF00] = 0xE0;

    GameBoy.Memory.IO_Ports[TIMA_REG-0xFF00] = 0x00; // Verified on hardware
    GameBoy.Memory.IO_Ports[TMA_REG-0xFF00] = 0x00; // Verified on hardware

    GB_MemWrite8(TAC_REG,0x00); // Verified on hardware

    if(GameBoy.Emulator.enable_boot_rom) // unknown
    {
        switch(GameBoy.Emulator.HardwareType)
        {
            case HW_GB:
            case HW_GBP:
                GameBoy.Emulator.sys_clocks = 8; // Verified on hardware
                break;

            case HW_SGB:
            case HW_SGB2:
                GameBoy.Emulator.sys_clocks = 0; // Unknown. Can't test.
                break;

            case HW_GBC:
            case HW_GBA:
                GameBoy.Emulator.sys_clocks = 0; // Not verified yet
                break;

            default:
                GameBoy.Emulator.sys_clocks = 0;
                Debug_ErrorMsg("GB_CPUInterruptsInit():\nUnknown hardware\n(boot ROM enabled)");
                break;
        }
    }
    else
    {
        switch(GameBoy.Emulator.HardwareType)
        {
            case HW_GB:
            case HW_GBP:
                GameBoy.Emulator.sys_clocks = 0xAF0C; // Verified on hardware
                break;

            case HW_SGB:
            case HW_SGB2:
                GameBoy.Emulator.sys_clocks = 0; // Unknown. Can't test.
                break;

            case HW_GBC:
            case HW_GBA:
                GameBoy.Emulator.sys_clocks = 0x2200; // Not verified yet
                break;

            default:
                GameBoy.Emulator.sys_clocks = 0;
                Debug_ErrorMsg("GB_CPUInterruptsInit():\nUnknown hardware");
                break;
        }
    }

    GameBoy.Emulator.timer_clocks = GameBoy.Emulator.sys_clocks & TIMER_COUNT_MAX_MASK;
    GameBoy.Memory.IO_Ports[DIV_REG-0xFF00] = GameBoy.Emulator.sys_clocks >> 8;

    GameBoy.Emulator.timer_overflow_count = 0;
    GameBoy.Emulator.timer_enabled = 0;
    GameBoy.Emulator.timer_irq_delay_active = 0;

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

//----------------------------------------------------------------

static void GB_TimerIncreaseTIMA(void)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    if(mem->IO_Ports[TIMA_REG-0xFF00] == 0xFF) //overflow
    {
        GameBoy.Emulator.timer_irq_delay_active = 4;
        mem->IO_Ports[TIMA_REG-0xFF00] = mem->IO_Ports[TMA_REG-0xFF00];
    }
    else mem->IO_Ports[TIMA_REG-0xFF00]++;
}

//----------------------------------------------------------------

void GB_TimersWriteDIV(int reference_clocks, int value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    GB_TimersUpdateClocksClounterReference(reference_clocks);
    GameBoy.Emulator.sys_clocks = 0;
    mem->IO_Ports[DIV_REG-0xFF00] = 0;

    //if(GameBoy.Emulator.timer_enabled)
    {
        //if(GameBoy.Emulator.timer_clocks&SHARED_TIMA_DIV_COUNT_MASK)
        {
            GameBoy.Emulator.timer_clocks &= ~SHARED_TIMA_DIV_COUNT_MASK; // clear shared bits
            GameBoy.Emulator.timer_clocks |= (1<<SHARED_BITS_TIMA_DIV);

        //    //if( ( GameBoy.Emulator.timer_clocks & (5<<(SHARED_BITS_TIMA_DIV+1)) ) == (1<<(SHARED_BITS_TIMA_DIV+1)) )
        //    if( ( GameBoy.Emulator.timer_clocks & (1<<(SHARED_BITS_TIMA_DIV+1)) )
        //        GB_TimerIncreaseTIMA();
        }
    }
}

void GB_TimersWriteTIMA(int reference_clocks, int value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

//    int old_flag = GameBoy.Emulator.timer_irq_delay_active;

    GB_TimersUpdateClocksClounterReference(reference_clocks);
/*
    if( (GameBoy.Emulator.sys_clocks&GameBoy.Emulator.timer_overflow_mask) == 0)
    {
        //If TIMA is written the same clock as incrementing itself prevent the timer IF flag from being set
        GameBoy.Emulator.timer_irq_delay_active = old_flag;
    }
*/
    GameBoy.Emulator.timer_clocks = GameBoy.Emulator.sys_clocks&SHARED_TIMA_DIV_COUNT_MASK; //(GameBoy.Emulator.timer_overflow_count-1);

    mem->IO_Ports[TIMA_REG-0xFF00] = value;
}

void GB_TimersWriteTMA(int reference_clocks, int value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    GB_TimersUpdateClocksClounterReference(reference_clocks);
/*
    if( (GameBoy.Emulator.sys_clocks&GameBoy.Emulator.timer_overflow_mask) == 0)
    {
        //If TMA is written the same clock as reloading TIMA from TMA, load TIMA from written value
        if(mem->IO_Ports[TIMA_REG-0xFF00] == 0)
            mem->IO_Ports[TIMA_REG-0xFF00] = value;
    }
*/
    mem->IO_Ports[TMA_REG-0xFF00] = value;
}

void GB_TimersWriteTAC(int reference_clocks, int value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    GB_TimersUpdateClocksClounterReference(reference_clocks);

    // Strange things
#if 0
    int old_value = mem->IO_Ports[TAC_REG-0xFF00] & 7;
    int new_value = value & 7;

    int extra_increment = 0;

    if( old_value&BIT(2) ) // enabled
    {/*
        if( (old_value&3) == 2 ) // 64 clocks
        {
            extra_increment = 1;
        }
        else if( (old_value&3) == 1 ) // 16 clocks
        {
            const int inc_table[4][8] = {
                { 2,2,2,2, 2,1,2,2 }, // NO INC, 00
                { 1,1,1,1, 1,0,1,1 }, // NO INC, FF
                { 1,1,1,1, 1,1,1,1 }, // INC, 00
                { 0,0,0,0, 0,0,0,0 }  // INC, FF
            };

            int if_flag_set = (mem->IO_Ports[IF_REG-0xFF00] & BIT(2)) != 0;

            int just_incrementing = (GameBoy.Emulator.sys_clocks & (16-1)) == 0;

            extra_increment = inc_table[just_incrementing*2 + if_flag_set][new_value&7];
        }*/
    }
    else // disabled
    {
        if(GameBoy.Emulator.HardwareType == HW_GBA)
        {
            if( (old_value&3) == 1 ) // 16 clocks
            {
                if( new_value == (BIT(2)|3) ) // 256 clocks, enabled
                {
                    int just_incrementing = (GameBoy.Emulator.sys_clocks & (16-1)) == 0;
                    if(just_incrementing == 0) extra_increment = 1;
                    // maybe this only happens if written the clock before incrementing
                }
            }
        }
    }

    while(extra_increment)
    {
        if(mem->IO_Ports[TIMA_REG-0xFF00] == 0xFF) //overflow
        {
            GameBoy.Emulator.timer_irq_delay_active = 1;
            mem->IO_Ports[TIMA_REG-0xFF00] = mem->IO_Ports[TMA_REG-0xFF00];
        }
        else mem->IO_Ports[TIMA_REG-0xFF00]++;

        extra_increment--;
    }
#endif // 0
    // Normal behavior

    if(value & (1<<2))
    {
        const u32 gb_timer_clock_overflow[4] = {1024, 16, 64, 256};
        GameBoy.Emulator.timer_enabled = 1;
        GameBoy.Emulator.timer_overflow_count = gb_timer_clock_overflow[value&3];
    }
    else
    {
        //GameBoy.Emulator.timer_irq_delay_active = 0;
        GameBoy.Emulator.timer_enabled = 0;
        GameBoy.Emulator.timer_overflow_count = 0;
    }

    GameBoy.Emulator.timer_clocks = GameBoy.Emulator.sys_clocks&SHARED_TIMA_DIV_COUNT_MASK; //(GameBoy.Emulator.timer_overflow_count-1);

    mem->IO_Ports[TAC_REG-0xFF00] = value;
}

static void GB_TimersUpdateDelayTimerIRQ(int increment_clocks)
{
    if(GameBoy.Emulator.timer_irq_delay_active)
    {
        if(GameBoy.Emulator.timer_irq_delay_active > increment_clocks)
        {
            GameBoy.Emulator.timer_irq_delay_active -= increment_clocks;
        }
        else
        {
            GameBoy.Emulator.timer_irq_delay_active = 0;
            GB_SetInterrupt(I_TIMER);
        }
    }
}

void GB_TimersUpdateClocksClounterReference(int reference_clocks)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int increment_clocks = reference_clocks - GB_TimersClockCounterGet();

    //int old_sys_clocks = GameBoy.Emulator.sys_clocks;

    //Don't assume that this function will increase just a few clocks. Timer can be increased up to 4 times
    //just because of HDMA needing 64 clocks in double speed mode (min. timer period is 16 clocks).

    GameBoy.Emulator.sys_clocks = (GameBoy.Emulator.sys_clocks+increment_clocks)&0xFFFF;

    // DIV
    // ---

    //Upper 8 bits of the 16 register sys_clocks
    mem->IO_Ports[DIV_REG-0xFF00] = (GameBoy.Emulator.sys_clocks>>8);


    // TIMA
    // ----

    //if(GameBoy.Emulator.timer_enabled)
        GB_TimersUpdateDelayTimerIRQ(increment_clocks);
    //else
    //    GameBoy.Emulator.timer_irq_delay_active = 0;

    if(GameBoy.Emulator.timer_enabled)
    {
        int timer_update_clocks =
            ( GameBoy.Emulator.timer_clocks&(GameBoy.Emulator.timer_overflow_count-1) ) + increment_clocks;

        while(timer_update_clocks >= GameBoy.Emulator.timer_overflow_count)
        {
            GB_TimerIncreaseTIMA();

            timer_update_clocks -= GameBoy.Emulator.timer_overflow_count;

            GB_TimersUpdateDelayTimerIRQ(timer_update_clocks);
        }
    }

    //if(GameBoy.Emulator.timer_enabled) //?
    GameBoy.Emulator.timer_clocks = (GameBoy.Emulator.timer_clocks+increment_clocks) & TIMER_COUNT_MAX_MASK;


    // SOUND
    // -----


    //TODO : SOUND UPDATE GOES HERE!!


    // Exit

    GB_TimersClockCounterSet(reference_clocks);
}

int GB_TimersGetClocksToNextEvent(void)
{
    int clocks_to_next_event = 256 - (GameBoy.Emulator.sys_clocks&0xFF); // DIV

    //if(GameBoy.Emulator.timer_enabled) ??
        if(GameBoy.Emulator.timer_irq_delay_active)
        {
            return 4; // break right now!
        }

    if(GameBoy.Emulator.timer_enabled)
    {
        return 4;/*
        int timer_mask = GameBoy.Emulator.timer_overflow_mask;
        int timer_update_clocks = (GameBoy.Emulator.sys_clocks & timer_mask);
        int clocks_left_for_timer = (timer_mask+1) - timer_update_clocks;

        if(clocks_left_for_timer < clocks_to_next_event)
            clocks_to_next_event = clocks_left_for_timer;*/
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
