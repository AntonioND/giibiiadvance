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

#include "../build_options.h"

#include "gameboy.h"
#include "cpu.h"
#include "debug.h"
#include "memory.h"
#include "general.h"
#include "ppu.h"
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

void GB_PPUInit(void)
{

}

void GB_PPUEnd(void)
{

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

void GB_PPUUpdateClocksClounterReference(int reference_clocks)
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

    if(GameBoy.Emulator.lcd_on)
    {
        GameBoy.Emulator.LCD_clocks += increment_clocks;

        //SCREEN
        switch(GameBoy.Emulator.ScreenMode)
        {
            case 2:
                if(GameBoy.Emulator.LCD_clocks >= (80<<GameBoy.Emulator.DoubleSpeed) )
                {
                    GameBoy.Emulator.LCD_clocks -= (80<<GameBoy.Emulator.DoubleSpeed);

                    GameBoy.Emulator.ScreenMode = 3;
                    mem->IO_Ports[STAT_REG-0xFF00] |= 0x03;

                    GB_CheckStatSignal();
                }
                break;
            case 3:
                if(GameBoy.Emulator.LCD_clocks >= (172<<GameBoy.Emulator.DoubleSpeed) )
                {
                    GameBoy.Emulator.LCD_clocks -= (172<<GameBoy.Emulator.DoubleSpeed);

                    GameBoy.Emulator.DrawScanlineFn(GameBoy.Emulator.CurrentScanLine);

                    GameBoy.Emulator.ScreenMode = 0;
                    mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;

                    GB_CheckStatSignal();
                }
                break;
            case 0:
                if(GameBoy.Emulator.LCD_clocks >= (204<<GameBoy.Emulator.DoubleSpeed) )
                {
                    GameBoy.Emulator.LCD_clocks -= (204<<GameBoy.Emulator.DoubleSpeed);

                    GameBoy.Emulator.CurrentScanLine ++;

                    mem->IO_Ports[LY_REG-0xFF00] = GameBoy.Emulator.CurrentScanLine;

                    GB_CheckLYC();

                    if(GameBoy.Emulator.CurrentScanLine == 144)
                    {
                        GameBoy.Emulator.ScreenMode = 1;
                        mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                        mem->IO_Ports[STAT_REG-0xFF00] |= 0x01;

                        //Call VBL interrupt...
                        //if(mem->HighRAM[IE_REG-0xFF80] & I_VBLANK)
                        //    GB_SetInterrupt(I_VBLANK);
                        GameBoy.Emulator.VBL_clocks_delay = 24 - GameBoy.Emulator.LCD_clocks;
                    }
                    else
                    {
                        GameBoy.Emulator.ScreenMode = 2;
                        mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                        mem->IO_Ports[STAT_REG-0xFF00] |= 0x02;
                    }

                    GB_CheckStatSignal();
                }
                break;
            case 1:
                if(GameBoy.Emulator.LCD_clocks >= (456<<GameBoy.Emulator.DoubleSpeed) )
                {
                    GameBoy.Emulator.LCD_clocks -= (456<<GameBoy.Emulator.DoubleSpeed);

                    if(GameBoy.Emulator.CurrentScanLine == 0)
                    {
                        GameBoy.Emulator.ScreenMode = 2;
                        mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                        mem->IO_Ports[STAT_REG-0xFF00] |= 0x02;
                        GB_CheckStatSignal();
                        break;
                    }

                    GameBoy.Emulator.CurrentScanLine ++;

                    if(GameBoy.Emulator.CurrentScanLine == 153)
                    {
                        GameBoy.Emulator.LCD_clocks += ((456-8)<<GameBoy.Emulator.DoubleSpeed); // 8 clocks this scanline
                    }
                    else if(GameBoy.Emulator.CurrentScanLine == 154)
                    {
                        GameBoy.Emulator.CurrentScanLine = 0;
                        GameBoy.Emulator.FrameDrawn = 1;

                        GameBoy.Emulator.LCD_clocks += (8<<GameBoy.Emulator.DoubleSpeed); // 456 - 8 cycles left of vblank...
                    }

                    mem->IO_Ports[LY_REG-0xFF00] = GameBoy.Emulator.CurrentScanLine;

                    GB_CheckLYC();

                    GB_CheckStatSignal();
                }
                break;
        }
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
                clocks_to_next_event = (80<<GameBoy.Emulator.DoubleSpeed) - GameBoy.Emulator.LCD_clocks;
                break;
            case 3:
                clocks_to_next_event = (172<<GameBoy.Emulator.DoubleSpeed) - GameBoy.Emulator.LCD_clocks;
                break;
            case 0:
                clocks_to_next_event = (204<<GameBoy.Emulator.DoubleSpeed) - GameBoy.Emulator.LCD_clocks;
                break;
            case 1:
                clocks_to_next_event = (456<<GameBoy.Emulator.DoubleSpeed) - GameBoy.Emulator.LCD_clocks;
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
