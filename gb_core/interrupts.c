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

/*******************************************************
** Mode 2 -  80 clks \                                **
** Mode 3 - 172 clks |- 456 clks  -  144 times        **
** Mode 0 - 204 clks /                                **
**                                                    **
** Mode 3: 10 SPR - 296 clks || 0 SPR - 172 clks      **
** Mode 0: 10 SPR -  80 clks || 0 SPR - 204 clks      **
**                                                    **
** VBlank (Mode 1) lasts 4560 clks.                   **
**                                                    **
** A complete screen refresh occurs every 70224 clks. **
** 230230230230...11111111...230230230....1111111...  **
*******************************************************/

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
    GameBoy.Emulator.FrameDrawn = 0;
    GameBoy.Emulator.LCD_clocks = 0;
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

static int gb_ppu_clock_counter = 0;

inline void GB_PPUClockCounterReset(void)
{
    gb_ppu_clock_counter = 0;
}

static inline int GB_PPUClockCounterGet(void)
{
    return gb_ppu_clock_counter;
}

static inline void GB_PPUClockCounterSet(int new_reference_clocks)
{
    gb_ppu_clock_counter = new_reference_clocks;
}

void GB_PPUUpdate(int reference_clocks)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int increment_clocks = reference_clocks - GB_PPUClockCounterGet();

    if(GameBoy.Emulator.VBL_clocks_delay)
    {
        GameBoy.Emulator.VBL_clocks_delay -= increment_clocks;

        if(GameBoy.Emulator.VBL_clocks_delay <= 0)
        {
            GB_SetInterrupt(I_VBLANK);
            GameBoy.Emulator.VBL_clocks_delay = 0;
        }
    }

    GameBoy.Emulator.LCD_clocks += increment_clocks;

    //SCREEN
    switch(GameBoy.Emulator.ScreenMode)
    {
        case 2:
            if(GameBoy.Emulator.LCD_clocks > 79)
            {
                GameBoy.Emulator.LCD_clocks -= 80;

                GameBoy.Emulator.ScreenMode = 3;
            //    mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                mem->IO_Ports[STAT_REG-0xFF00] |= 0x03;

                GB_CheckStatSignal();
            }
            break;
        case 3:
            if(GameBoy.Emulator.LCD_clocks > 171) //(GameBoy.Emulator.mode3len-1))
            {
                GameBoy.Emulator.LCD_clocks -= 172; //GameBoy.Emulator.mode3len;

                GameBoy.Emulator.DrawScanlineFn(GameBoy.Emulator.CurrentScanLine);

                GameBoy.Emulator.ScreenMode = 0;
                mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;

                GB_CheckStatSignal();
            }
            break;
        case 0:
            if(GameBoy.Emulator.LCD_clocks > 203) //(GameBoy.Emulator.mode0len-1))
            {
                GameBoy.Emulator.LCD_clocks -= 204; //GameBoy.Emulator.mode0len;

                GameBoy.Emulator.CurrentScanLine ++;

                mem->IO_Ports[LY_REG-0xFF00] = GameBoy.Emulator.CurrentScanLine;

                GB_CheckLYC();

                if(GameBoy.Emulator.CurrentScanLine == 144)
                {
                    GameBoy.Emulator.ScreenMode = 1;
                    mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                    mem->IO_Ports[STAT_REG-0xFF00] |= 0x01;

                    //Call VBL interrupt...
                    if(GameBoy.Emulator.lcd_on)
                    {
                    //    if(mem->HighRAM[IE_REG-0xFF80] & I_VBLANK)
                        //    GB_SetInterrupt(I_VBLANK);
                        GameBoy.Emulator.VBL_clocks_delay = 24 - GameBoy.Emulator.LCD_clocks;
                            //check in GB_CPUClock()
                    }
                }
                else
                {
                    //GB_Mode03UpdateLenght();
                    GameBoy.Emulator.ScreenMode = 2;
                    mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                    mem->IO_Ports[STAT_REG-0xFF00] |= 0x02;
                }

                GB_CheckStatSignal();
            }
            break;
        case 1:
            if(GameBoy.Emulator.LCD_clocks > 455)
            {
                GameBoy.Emulator.LCD_clocks -= 456;

                if(GameBoy.Emulator.CurrentScanLine == 0)
                {
                    GameBoy.Emulator.ScreenMode = 2;
                    mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                    mem->IO_Ports[STAT_REG-0xFF00] |= 0x02;
                    //GB_Mode03UpdateLenght();
                    GB_CheckStatSignal();
                    break;
                }

                GameBoy.Emulator.CurrentScanLine ++;

                if(GameBoy.Emulator.CurrentScanLine == 153)
                {
                    GameBoy.Emulator.LCD_clocks += 456 - 8; // 8 clocks this scanline
                }
                else if(GameBoy.Emulator.CurrentScanLine == 154)
                {
                    GameBoy.Emulator.CurrentScanLine = 0;
                    GameBoy.Emulator.FrameDrawn = 1;
                    //GameBoy.Emulator.FrameCount ++;

                    GameBoy.Emulator.LCD_clocks += 8; // 456 - 8 cycles left of vblank...
                }

                mem->IO_Ports[LY_REG-0xFF00] = GameBoy.Emulator.CurrentScanLine;

                GB_CheckLYC();

                GB_CheckStatSignal();
            }
            break;
    }

    GB_PPUClockCounterSet(reference_clocks);
}

int GB_PPUGetClocksToNextEvent(void)
{
    int clocks_to_next_event = 0x7FFFFFFF;

    if(GameBoy.Emulator.lcd_on)
    {
        switch(GameBoy.Emulator.ScreenMode)
        {
            case 2:
                clocks_to_next_event = 80 - GameBoy.Emulator.LCD_clocks;
                break;
            case 3:
                clocks_to_next_event = 172 - GameBoy.Emulator.LCD_clocks;
                break;
            case 0:
                clocks_to_next_event = 204 - GameBoy.Emulator.LCD_clocks;
                break;
            case 1:
                clocks_to_next_event = 456 - GameBoy.Emulator.LCD_clocks;
                break;
            default:
                break;
        }
    }

    if(GameBoy.Emulator.VBL_clocks_delay)
    {
        if(clocks_to_next_event > GameBoy.Emulator.VBL_clocks_delay)
            clocks_to_next_event = GameBoy.Emulator.VBL_clocks_delay;
    }

    return clocks_to_next_event;
}

//----------------------------------------------------------------

// START OF TODO :

inline void GB_CheckStatSignal(void)
{
    if(GameBoy.Emulator.lcd_on == 0)
    {
        GameBoy.Emulator.stat_signal = 0;
        return;
    }

    _GB_MEMORY_ * mem = &GameBoy.Memory;
    u32 screenmode = GameBoy.Emulator.ScreenMode;
    int stat = mem->IO_Ports[STAT_REG-0xFF00];

    if(    (mem->IO_Ports[LY_REG-0xFF00] == mem->IO_Ports[LYC_REG-0xFF00] &&
                                                stat & IENABLE_LY_COMPARE)  ||
        (stat & IENABLE_HBL && screenmode == 0) ||
        (stat & IENABLE_OAM && screenmode == 2)  ||
        (stat & (IENABLE_VBL|IENABLE_OAM) && screenmode == 1)
    //    (stat & IENABLE_VBL && screenmode == 1)
    )

    {
        if(GameBoy.Emulator.stat_signal == 0 /*&& mem->HighRAM[IE_REG-0xFF80] & I_STAT*/)
            GB_SetInterrupt(I_STAT);

        GameBoy.Emulator.stat_signal = 1;

        return;
    }

    GameBoy.Emulator.stat_signal = 0;
}

inline void GB_CheckLYC(void)
{
    if(GameBoy.Memory.IO_Ports[LY_REG-0xFF00] == GameBoy.Memory.IO_Ports[LYC_REG-0xFF00])
        GameBoy.Memory.IO_Ports[STAT_REG-0xFF00] |= I_LY_EQUALS_LYC;
    else
        GameBoy.Memory.IO_Ports[STAT_REG-0xFF00] &= ~I_LY_EQUALS_LYC;
}

/*

inline void GB_Mode03UpdateLenght(void)
{
    int clocks = 0; //GB_SpriteCount(GameBoy.Emulator.CurrentScanLine) * 12;

    GameBoy.Emulator.mode3len = 172 + clocks;
    GameBoy.Emulator.mode0len = 204 - clocks;
}
*/

// END OF TODO :

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

void GB_TimersUpdate(int reference_clocks)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int increment_clocks = reference_clocks - GB_TimersClockCounterGet();

    //DIV -- EVERY 256 CLOCKS
    GameBoy.Emulator.DivClocks += increment_clocks;
    while(GameBoy.Emulator.DivClocks > 255)
    {
        mem->IO_Ports[DIV_REG-0xFF00] ++;
        GameBoy.Emulator.DivClocks -= 256;
    }

    //TIMA
    if(GameBoy.Emulator.timer_enabled)
    {
        GameBoy.Emulator.TimerClocks += increment_clocks;

        while(GameBoy.Emulator.TimerClocks > (GameBoy.Emulator.timer_total_clocks-1))
        {
            if(mem->IO_Ports[TIMA_REG-0xFF00] == 0xFF) //overflow
            {
                //if(mem->HighRAM[IE_REG-0xFF80] & I_TIMER)
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

    if( (GameBoy.Emulator.HardwareType == HW_GBC) || (GameBoy.Emulator.HardwareType == HW_GBA) )
        return; //No joypad interrupt in GBA/GBC

    //if((mem->HighRAM[IE_REG-0xFF80] & I_JOYPAD) == 0) return;

    u32 i = mem->IO_Ports[P1_REG-0xFF00];
    u32 result = 0;

    int Keys = GB_Input_Get(0);
    if((i & (1<<5)) == 0) //A-B-SEL-STA
        result |= Keys & (KEY_A|KEY_B|KEY_SELECT|KEY_START);
    if((i & (1<<4)) == 0) //PAD
        result |= Keys & (KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT);

    if(result)
    {
        GB_SetInterrupt(I_JOYPAD);
        if(GameBoy.Emulator.CPUHalt == 2) // stop
            GameBoy.Emulator.CPUHalt = 0;
    }

    return;
}

//----------------------------------------------------------------
//----------------------------------------------------------------
