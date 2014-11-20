
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

//----------------------------------------------------------------

#include "gameboy.h"

#include "cpu.h"
#include "memory.h"

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//----------------------------------------------------------------

void GB_DMAInit(void)
{
    GameBoy.Emulator.OAM_DMA_bytes_left = 0;
    GameBoy.Emulator.OAM_DMA_clocks_elapsed = 0;
    GameBoy.Emulator.OAM_DMA_src = 0;
    GameBoy.Emulator.OAM_DMA_dst = 0;

    GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;

    GameBoy.Emulator.gdma_preparation_clocks_left = 0;
    GameBoy.Emulator.gdma_copy_clocks_left = 0;
    GameBoy.Emulator.gdma_src = 0;
    GameBoy.Emulator.gdma_dst = 0;
    GameBoy.Emulator.gdma_bytes_left = 0;

    GameBoy.Emulator.hdma_last_ly_copied = -1;
}

void GB_DMAEnd(void)
{
    //Nothing here...
}

//----------------------------------------------------------------

void GB_DMAInitOAMCopy(int src_addr_high_byte)
{
    GameBoy.Emulator.OAM_DMA_bytes_left = 40*4;
    GameBoy.Emulator.OAM_DMA_clocks_elapsed = 0;
    GameBoy.Emulator.OAM_DMA_src = src_addr_high_byte<<8;
    GameBoy.Emulator.OAM_DMA_dst = 0xFE00;
}

void GB_DMAInitGBCCopy(int register_value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    GameBoy.Emulator.GBC_DMA_enabled = (register_value & (1<<7)) ? GBC_DMA_HBLANK : GBC_DMA_GENERAL;

    if(GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_HBLANK)
    {
        mem->IO_Ports[HDMA5_REG-0xFF00] &= 0x7F;

        u32 blocks = ( ((u32)register_value) & 0x7F ) + 1;
        GameBoy.Emulator.gdma_src = (mem->IO_Ports[HDMA1_REG-0xFF00]<<8) | mem->IO_Ports[HDMA2_REG-0xFF00];
        GameBoy.Emulator.gdma_dst = ((mem->IO_Ports[HDMA3_REG-0xFF00]<<8) | mem->IO_Ports[HDMA4_REG-0xFF00]) + 0x8000;
        GameBoy.Emulator.gdma_bytes_left = blocks * 16;
        GameBoy.Emulator.gdma_preparation_clocks_left = 0;
        GameBoy.Emulator.hdma_last_ly_copied = -1; // reset

        //Debug_DebugMsgArg("HDMA\nSRC = %04X\nDST = %04X\nBYTES = %d",
        //                  GameBoy.Emulator.gdma_src,GameBoy.Emulator.gdma_dst,GameBoy.Emulator.gdma_bytes_left);
    }
    else
    {
        // Timing information in GB_DMAExecute()

        u32 blocks = ( ((u32)register_value) & 0x7F ) + 1;

        if(GameBoy.Emulator.DoubleSpeed) // Tested on hardware
        {
            GameBoy.Emulator.gdma_preparation_clocks_left = 4;
            GameBoy.Emulator.gdma_copy_clocks_left = blocks * 64;
        }
        else
        {
            GameBoy.Emulator.gdma_preparation_clocks_left = 4;
            GameBoy.Emulator.gdma_copy_clocks_left = blocks * 32;
        }

        GameBoy.Emulator.gdma_src = (mem->IO_Ports[HDMA1_REG-0xFF00]<<8) | mem->IO_Ports[HDMA2_REG-0xFF00];
        GameBoy.Emulator.gdma_dst = ((mem->IO_Ports[HDMA3_REG-0xFF00]<<8) | mem->IO_Ports[HDMA4_REG-0xFF00]) + 0x8000;
        GameBoy.Emulator.gdma_bytes_left = blocks * 16;

        //Debug_DebugMsgArg("GDMA\nSRC = %04X\nDST = %04X\nBYTES = %d",
        //    GameBoy.Emulator.gdma_src,GameBoy.Emulator.gdma_dst,GameBoy.Emulator.gdma_bytes_left);
    }
}

void GB_DMAStopGBCCopy(void)
{
    GameBoy.Emulator.gdma_bytes_left = 0;
    GameBoy.Emulator.hdma_last_ly_copied = -1; // reset

    GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;
    GameBoy.Memory.IO_Ports[HDMA5_REG-0xFF00] |= 0x80; //verified on hardware (GBC and GBA SP)
}

//----------------------------------------------------------------

static int gb_dma_clock_counter = 0;

inline void GB_DMAClockCounterReset(void)
{
    gb_dma_clock_counter = 0;
}

static inline int GB_DMAClockCounterGet(void)
{
    return gb_dma_clock_counter;
}

static inline void GB_DMAClockCounterSet(int new_reference_clocks)
{
    gb_dma_clock_counter = new_reference_clocks;
}

void GB_DMAUpdateClocksClounterReference(int reference_clocks)
{
    int increment_clocks = reference_clocks - GB_DMAClockCounterGet();

    // pandocs says it needs aprox 160 us in normal speed mode
    //  bytes * (clocks per byte (guesswork)) * (us per clock)
    // (40*4) * 4 * 0.2384185791015625 = 152.587890625 us -> not sure if 8 us needed to setup or just
    //                                                       incorrect measurement
    if(GameBoy.Emulator.OAM_DMA_bytes_left > 0)
    {
        int stop_clocks = GameBoy.Emulator.OAM_DMA_clocks_elapsed + increment_clocks;

        while(stop_clocks > GameBoy.Emulator.OAM_DMA_clocks_elapsed)
        {
            GameBoy.Emulator.OAM_DMA_clocks_elapsed ++;
            if((GameBoy.Emulator.OAM_DMA_clocks_elapsed & 3) == 0)
            {
                GB_MemWrite8(GameBoy.Emulator.OAM_DMA_dst++,GB_MemRead8(GameBoy.Emulator.OAM_DMA_src++));
                GameBoy.Emulator.OAM_DMA_bytes_left --;

                if(GameBoy.Emulator.OAM_DMA_bytes_left == 0)
                {
                    GameBoy.Emulator.OAM_DMA_clocks_elapsed = 0;
                    GameBoy.Emulator.OAM_DMA_src = 0;
                    GameBoy.Emulator.OAM_DMA_dst = 0;
                    break;
                }
            }
        }
    }

    GB_DMAClockCounterSet(reference_clocks);
}

int GB_DMAGetClocksToNextEvent(void)
{
    int clocks_to_next_event = 0x7FFFFFFF;

    if(GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_GENERAL)
    {
        if(GameBoy.Emulator.gdma_preparation_clocks_left > 0)
            clocks_to_next_event = GameBoy.Emulator.gdma_preparation_clocks_left;
        else if(GameBoy.Emulator.gdma_copy_clocks_left > 0)
            clocks_to_next_event = GameBoy.Emulator.gdma_copy_clocks_left;
    }
    //else if(GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_HBLANK)
    //{
    //    return clocks to video mode 0. That is done already in GB_PPUGetClocksToNextEvent()
    //}

    return clocks_to_next_event;
}

//----------------------------------------------------------------

int GB_DMAExecute(int clocks)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int executed_clocks = 0;

    //This code assumes that this function is called as soon as the CPU has requested a GDMA,
    //or as soon as entering mode 0 if there is a HDMA copy running. The only possible delay is
    //the one caused by finishing the CPU instruction or IRQ handling.

    if(GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_GENERAL)
    {
        //It takes (220 + (n * 7.63)) microseconds in single speed and (110 + (n * 7.63)) microseconds
        //in double speed mode. (Where n = 0 is 16 bytes, n = 1 is 32 bytes, n=2 is 48 bytes, etc... )
        //This translates to a transfer rate of 2097152 bytes/sec if you don't consider the initial
        //startup delay of 220uS (110uS in double speed mode.) The value 110uS is accurate within +/-5uS.
        //The value 220uS is accurate within +/- 10uS.

        // Single speed
        // 220 us = 922 (+/- 42) clocks
        // 7.63 us = 32 clocks
        // Preparation clocks = 922 - 32 = 890
        // Clocks per byte = 32/16 = 2

        //Double speed
        // 110 us = 922 (+/- 42) clocks
        // 7.63 us = 64 clocks
        // Preparation clocks = 922 - 64 = 858 (?)
        // Clocks per byte = 64/16 = 4

        //This copy has to be divided. Only execute the specified clocks.
        //The CPU can force to end an instruction because the worst case is about 30 clocks off, but
        //GDMA worst case is about 9050 clocks

        int start_clocks = GB_CPUClockCounterGet();

        while(clocks > 0)
        {
            if(GameBoy.Emulator.gdma_preparation_clocks_left > 0)
            {
                if(clocks >= GameBoy.Emulator.gdma_preparation_clocks_left)
                {
                    GB_CPUClockCounterAdd(GameBoy.Emulator.gdma_preparation_clocks_left);
                    clocks -= GameBoy.Emulator.gdma_preparation_clocks_left;
                    GameBoy.Emulator.gdma_preparation_clocks_left = 0;
                }
                else
                {
                    GB_CPUClockCounterAdd(clocks);
                    GameBoy.Emulator.gdma_preparation_clocks_left -= clocks;
                    clocks = 0;
                }
            }
            else if(GameBoy.Emulator.gdma_copy_clocks_left > 0)
            {
                int execute_now_clocks = (clocks < GameBoy.Emulator.gdma_copy_clocks_left) ?
                                          clocks : GameBoy.Emulator.gdma_copy_clocks_left;

                int clocks_per_byte_mask = (2<<GameBoy.Emulator.DoubleSpeed)-1;

                u32 bytes = GameBoy.Emulator.gdma_bytes_left;

                while(execute_now_clocks--)
                {
                    GameBoy.Emulator.gdma_copy_clocks_left --;
                    GB_CPUClockCounterAdd(1);

                    if( (GameBoy.Emulator.gdma_copy_clocks_left & clocks_per_byte_mask) == 0 )
                    {
                        GB_MemWrite8(GameBoy.Emulator.gdma_dst++,GB_MemRead8(GameBoy.Emulator.gdma_src++));
                        bytes --;
                        if(bytes == 0)
                        {
                            GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;
                            break;
                        }
                    }
                }

                GameBoy.Emulator.gdma_bytes_left = bytes;

                mem->IO_Ports[HDMA1_REG-0xFF00] = GameBoy.Emulator.gdma_src >> 8;
                mem->IO_Ports[HDMA2_REG-0xFF00] = GameBoy.Emulator.gdma_src & 0xFF;
                mem->IO_Ports[HDMA3_REG-0xFF00] = (GameBoy.Emulator.gdma_dst >> 8) & 0x1F;
                mem->IO_Ports[HDMA4_REG-0xFF00] = GameBoy.Emulator.gdma_dst & 0xFF;

                if(GameBoy.Emulator.gdma_bytes_left > 0)
                    mem->IO_Ports[HDMA5_REG-0xFF00] = ( (GameBoy.Emulator.gdma_bytes_left-1) / 16);
                else
                    mem->IO_Ports[HDMA5_REG-0xFF00] = 0xFF; // when finished, block count = -1

                clocks = 0; // exit loop
            }
        }

        int end_clocks = GB_CPUClockCounterGet();

        executed_clocks = end_clocks - start_clocks;
    }
    else if(GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_HBLANK)
    {
        //This doesn't need to be divided. Worst case is 64 clocks.

        if(GameBoy.Emulator.CPUHalt == 0) // TODO: TEST ?
        {/*
            if(GameBoy.Emulator.gdma_preparation_clocks_left > 0)
            {
                GB_CPUClockCounterAdd(GameBoy.Emulator.gdma_preparation_clocks_left);
                executed_clocks = GameBoy.Emulator.gdma_preparation_clocks_left;
                GameBoy.Emulator.gdma_preparation_clocks_left = 0;
            }
            else
            {*/
                int current_ly = -1;

                if(GameBoy.Emulator.lcd_on)
                    current_ly = mem->IO_Ports[LY_REG-0xFF00];
                else
                    current_ly = -2; // if screen is off, copy one block, continue when screen is switched on

                if(GameBoy.Emulator.hdma_last_ly_copied != current_ly)
                {
                    if(GameBoy.Emulator.ScreenMode == 0)
                    {
                        GameBoy.Emulator.hdma_last_ly_copied = current_ly;
                        if(current_ly < 144)
                        {
                            int start_clocks = GB_CPUClockCounterGet();

                            GB_CPUClockCounterAdd(4); // Init copy

                            int i;
                            for(i = 0; i < 16; i++)
                            {
                                //The copy needs the same TIME in single and double speeds mode.
                                GB_CPUClockCounterAdd(2<<GameBoy.Emulator.DoubleSpeed);
                                GB_MemWrite8(GameBoy.Emulator.gdma_dst++,GB_MemRead8(GameBoy.Emulator.gdma_src++));
                            }

                            mem->IO_Ports[HDMA1_REG-0xFF00] = GameBoy.Emulator.gdma_src >> 8;
                            mem->IO_Ports[HDMA2_REG-0xFF00] = GameBoy.Emulator.gdma_src & 0xFF;
                            mem->IO_Ports[HDMA3_REG-0xFF00] = (GameBoy.Emulator.gdma_dst >> 8) & 0x1F;
                            mem->IO_Ports[HDMA4_REG-0xFF00] = GameBoy.Emulator.gdma_dst & 0xFF;

                            GameBoy.Emulator.gdma_bytes_left -= 16;

                            if(GameBoy.Emulator.gdma_bytes_left == 0)
                            {
                                GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;
                                GameBoy.Emulator.hdma_last_ly_copied = -1;
                                mem->IO_Ports[HDMA5_REG-0xFF00] = 0xFF;
                            }
                            else
                            {
                                mem->IO_Ports[HDMA5_REG-0xFF00] = (GameBoy.Emulator.gdma_bytes_left-1)/16;
                            }

                            int end_clocks = GB_CPUClockCounterGet();

                            executed_clocks = end_clocks - start_clocks;
                        }
                    }
                }
            //}
        }
    }

    return executed_clocks;
}

//----------------------------------------------------------------
