// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include "../build_options.h"
#include "../debug_utils.h"

#include "cpu.h"
#include "gameboy.h"
#include "gb_main.h"
#include "general.h"
#include "interrupts.h"
#include "memory.h"
#include "ppu.h"
#include "video.h"

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//----------------------------------------------------------------

void GB_PPUWriteLYC_GBC(int reference_clocks, int value)
{
    GB_PPUUpdateClocksCounterReference(reference_clocks);
    GameBoy.Memory.IO_Ports[LYC_REG - 0xFF00] = value;
    if (GameBoy.Emulator.lcd_on)
    {
        GB_PPUCheckLYC();
        GB_PPUCheckStatSignal();
        GB_CPUBreakLoop();
    }
}

void GB_PPUWriteLCDC_GBC(int reference_clocks, int value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    GB_PPUUpdateClocksCounterReference(reference_clocks);

    if ((mem->IO_Ports[LCDC_REG - 0xFF00] ^ value) & (1 << 7))
    {
        mem->IO_Ports[LY_REG - 0xFF00] = 0x00;
        GameBoy.Emulator.CurrentScanLine = 0;
        mem->IO_Ports[STAT_REG - 0xFF00] &= 0xFC;
        GameBoy.Emulator.ScreenMode = 0;

        if (value & (1 << 7))
        {
            GameBoy.Emulator.ly_clocks = 456;
            GameBoy.Emulator.CurrentScanLine = 0;
            GameBoy.Emulator.ScreenMode = 1;
            mem->IO_Ports[STAT_REG - 0xFF00] &= 0xFC;
            mem->IO_Ports[STAT_REG - 0xFF00] |= 1;
        }
        else
        {
            GameBoy.Emulator.ly_clocks = 0;
        }

        GB_PPUCheckStatSignal();
        mem->IO_Ports[IF_REG - 0xFF00] &= ~I_STAT;
    }

    GameBoy.Emulator.lcd_on = value >> 7;

    mem->IO_Ports[LCDC_REG - 0xFF00] = value;

    GB_CPUBreakLoop();
}

void GB_PPUWriteSTAT_GBC(unused__ int reference_clocks, int value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    GB_PPUUpdateClocksCounterReference(GB_CPUClockCounterGet());

    GB_CPUBreakLoop();

    mem->IO_Ports[STAT_REG - 0xFF00] &= 0x07;
    mem->IO_Ports[STAT_REG - 0xFF00] |= value & 0xF8;

    GB_PPUCheckStatSignal();

    // No bug in GBC/GBA/GBA_SP
}

//----------------------------------------------------------------

void GB_PPUUpdateClocks_GBC(int increment_clocks)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    GameBoy.Emulator.ly_clocks += increment_clocks;

    int done = 0;
    while (done == 0)
    {
        switch (GameBoy.Emulator.ScreenMode)
        {
            case 2:
                if (GameBoy.Emulator.ly_clocks >=
                                        (82 << GameBoy.Emulator.DoubleSpeed))
                {
                    GameBoy.Emulator.ScreenMode = 3;
                    mem->IO_Ports[STAT_REG - 0xFF00] |= 0x03;

                    GB_PPUCheckStatSignal();
                }
                else
                {
                    done = 1;
                }
                break;
            case 3:
                if (GameBoy.Emulator.ly_clocks >=
                                        (252 << GameBoy.Emulator.DoubleSpeed))
                {
                    GameBoy.Emulator.DrawScanlineFn(GameBoy.Emulator.CurrentScanLine);

                    GameBoy.Emulator.ScreenMode = 0;
                    mem->IO_Ports[STAT_REG - 0xFF00] &= 0xFC;

                    GameBoy.Emulator.ly_drawn = 1;

                    GB_PPUCheckStatSignal();
                }
                else
                {
                    done = 1;
                }
                break;
            case 0:
                if (GameBoy.Emulator.ly_drawn == 0)
                {
                    if (GameBoy.Emulator.ly_clocks >= 4)
                    {
                        GB_PPUCheckLYC();

                        if (GameBoy.Emulator.CurrentScanLine == 144)
                        {
                            GameBoy.Emulator.ScreenMode = 1;
                            mem->IO_Ports[STAT_REG - 0xFF00] &= 0xFC;
                            mem->IO_Ports[STAT_REG - 0xFF00] |= 0x01;

                            GB_InterruptsSetFlag(I_VBLANK);
                        }
                        else
                        {
                            GameBoy.Emulator.ScreenMode = 2;
                            mem->IO_Ports[STAT_REG - 0xFF00] &= 0xFC;
                            mem->IO_Ports[STAT_REG - 0xFF00] |= 0x02;
                        }

                        GB_PPUCheckStatSignal();
                    }
                    else
                    {
                        done = 1;
                    }
                }
                else
                {
                    if (GameBoy.Emulator.ly_clocks >=
                                        (456 << GameBoy.Emulator.DoubleSpeed))
                    {
                        GameBoy.Emulator.ly_clocks -=
                                (456 << GameBoy.Emulator.DoubleSpeed);

                        GameBoy.Emulator.CurrentScanLine++;
                        GameBoy.Emulator.ly_drawn = 0;

                        mem->IO_Ports[LY_REG - 0xFF00] =
                                GameBoy.Emulator.CurrentScanLine;
                    }
                    else
                    {
                        done = 1;
                    }
                }
                break;
            case 1:
                if (GameBoy.Emulator.ly_clocks >=
                                        (456 << GameBoy.Emulator.DoubleSpeed))
                {
                    GameBoy.Emulator.ly_clocks -=
                            (456 << GameBoy.Emulator.DoubleSpeed);

                    if (GameBoy.Emulator.CurrentScanLine == 0)
                    {
                        GameBoy.Emulator.ScreenMode = 2;
                        mem->IO_Ports[STAT_REG - 0xFF00] &= 0xFC;
                        mem->IO_Ports[STAT_REG - 0xFF00] |= 0x02;
                        GB_PPUCheckStatSignal();
                        break;
                    }

                    GameBoy.Emulator.CurrentScanLine++;

                    if (GameBoy.Emulator.CurrentScanLine == 153)
                    {
                        // 8 clocks this scanline
                        GameBoy.Emulator.ly_clocks +=
                                ((456 - 8) << GameBoy.Emulator.DoubleSpeed);
                    }
                    else if (GameBoy.Emulator.CurrentScanLine == 154)
                    {
                        GameBoy.Emulator.CurrentScanLine = 0;
                        GameBoy.Emulator.FrameDrawn = 1;

                        // 456 - 8 cycles left of vblank...
                        GameBoy.Emulator.ly_clocks +=
                                (8 << GameBoy.Emulator.DoubleSpeed);
                    }

                    mem->IO_Ports[LY_REG - 0xFF00] =
                            GameBoy.Emulator.CurrentScanLine;

                    GB_PPUCheckLYC();

                    GB_PPUCheckStatSignal();
                }
                else
                {
                    done = 1;
                }
                break;
            default:
                done = 1;
                break;
        }
    }
}

int GB_PPUGetClocksToNextEvent_GBC(void)
{
    int clocks_to_next_event = 0x7FFFFFFF;

    if (GameBoy.Emulator.lcd_on)
    {
        switch (GameBoy.Emulator.ScreenMode)
        {
            case 2:
                clocks_to_next_event = (82 << GameBoy.Emulator.DoubleSpeed)
                                       - GameBoy.Emulator.ly_clocks;
                break;
            case 3:
                clocks_to_next_event = 4;
                break;
            case 0:
                clocks_to_next_event = 4;
                //clocks_to_next_event = (456 << GameBoy.Emulator.DoubleSpeed)
                //                     - GameBoy.Emulator.ly_clocks;
                break;
            case 1:
                clocks_to_next_event = (456 << GameBoy.Emulator.DoubleSpeed)
                                       - GameBoy.Emulator.ly_clocks;
                break;
            default:
                break;
        }
    }

    return clocks_to_next_event;
}
