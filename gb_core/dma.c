
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

    GameBoy.Emulator.HBlankHDMAdone = 0;
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
        GameBoy.Emulator.HBlankHDMAdone = 0;
        mem->IO_Ports[HDMA5_REG-0xFF00] &= 0x7F;
    }
    else
    {
        // Timing information in GB_DMAExecute()

        u32 blocks = ( ((u32)register_value) & 0x7F ) + 1;

        if(GameBoy.Emulator.DoubleSpeed)
        {
            GameBoy.Emulator.gdma_preparation_clocks_left = 890;
            GameBoy.Emulator.gdma_copy_clocks_left = blocks * 64;
        }
        else
        {
            GameBoy.Emulator.gdma_preparation_clocks_left = 858;
            GameBoy.Emulator.gdma_copy_clocks_left = blocks * 32;
        }

        //Debug_DebugMsgArg("GDMA\nSRC = %04X\nDST = %04X\nBLOCKS = %d",
        //    (mem->IO_Ports[HDMA1_REG-0xFF00]<<8) | mem->IO_Ports[HDMA2_REG-0xFF00],
        //    ((mem->IO_Ports[HDMA3_REG-0xFF00]<<8) | mem->IO_Ports[HDMA4_REG-0xFF00]) + 0x8000, blocks);
    }
}

void GB_DMAStopGBCCopy(void)
{
    GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;
    GameBoy.Memory.IO_Ports[HDMA5_REG-0xFF00] |= (1<<7);
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
    int clocks_to_next_event = 256;

#warning "TODO"


    return clocks_to_next_event;
}

//----------------------------------------------------------------

int GB_DMAExecute(void)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int executed_clocks = 0;

    //This code assumes that this function is called as soon as the CPU has requested a GDMA,
    //or as soon as entering mode 0 if there is a HDMA copy running.

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

#warning "TODO"
/*
GameBoy.Emulator.gdma_preparation_clocks_left = 890;
GameBoy.Emulator.gdma_copy_clocks_left
*/
        int start_clocks = GB_CPUClockCounterGet();

        //Preparation time
        if(GameBoy.Emulator.DoubleSpeed)
            GB_CPUClockCounterAdd(890);
        else
            GB_CPUClockCounterAdd(858);

        u32 source = (mem->IO_Ports[HDMA1_REG-0xFF00]<<8) | mem->IO_Ports[HDMA2_REG-0xFF00];
        u32 dest = ((mem->IO_Ports[HDMA3_REG-0xFF00]<<8) | mem->IO_Ports[HDMA4_REG-0xFF00]) + 0x8000;
        u32 bytes = ( (mem->IO_Ports[HDMA5_REG-0xFF00] & 0x7F) + 1 ) * 16;

        while(bytes)
        {
            GB_MemWrite8(dest++,GB_MemRead8(source++));
            GB_CPUClockCounterAdd(2<<GameBoy.Emulator.DoubleSpeed);
            bytes --;
        }

        mem->IO_Ports[HDMA1_REG-0xFF00] = source >> 8;
        mem->IO_Ports[HDMA2_REG-0xFF00] = source & 0xFF;
        mem->IO_Ports[HDMA3_REG-0xFF00] = (dest >> 8) & 0x1F;
        mem->IO_Ports[HDMA4_REG-0xFF00] = dest & 0xFF;
        mem->IO_Ports[HDMA5_REG-0xFF00] = 0;

        GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;

        int end_clocks = GB_CPUClockCounterGet();

        executed_clocks = end_clocks - start_clocks;
    }
    else if(GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_HBLANK)
    {
        executed_clocks = 0;
    }

    return executed_clocks;
}

/*

inline u32 GBC_HDMAcopy(void)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    if(GameBoy.Emulator.HDMAenabled == HDMA_NONE) return 0;
    else if(GameBoy.Emulator.HDMAenabled == HDMA_HBLANK)
    {
        if(GameBoy.Emulator.lcd_on == 0) return 0; //Screen OFF, nothing happens
        if(GameBoy.Emulator.CPUHalt) return 0; //Halt mode, nothing happens

        if(GameBoy.Emulator.ScreenMode != 0) // Not hblank period
        {
            GameBoy.Emulator.HBlankHDMAdone = 0;
            GameBoy.Emulator.gbc_dma_working_for = 16 / 2;
            return 0;
        }

        if(GameBoy.Emulator.HBlankHDMAdone)
            return 0; //0x10 bytes copied before

        u32 source = (mem->IO_Ports[HDMA1_REG-0xFF00]<<8) | mem->IO_Ports[HDMA2_REG-0xFF00];
        u32 dest = ((mem->IO_Ports[HDMA3_REG-0xFF00]<<8) | mem->IO_Ports[HDMA4_REG-0xFF00]) + 0x8000;

        GB_MemWrite8(dest++,GB_MemRead8(source++));
        GB_MemWrite8(dest++,GB_MemRead8(source++));

        mem->IO_Ports[HDMA1_REG-0xFF00] = source >> 8;
        mem->IO_Ports[HDMA2_REG-0xFF00] = source & 0xFF;
        mem->IO_Ports[HDMA3_REG-0xFF00] = (dest >> 8) & 0x1F;
        mem->IO_Ports[HDMA4_REG-0xFF00] = dest & 0xFF;

        GameBoy.Emulator.gbc_dma_working_for--;
        if(GameBoy.Emulator.gbc_dma_working_for == 0)
        {
            GameBoy.Emulator.HBlankHDMAdone = 1;

            if(mem->IO_Ports[HDMA5_REG-0xFF00] == 0)
            {
                GameBoy.Emulator.HDMAenabled = HDMA_NONE;
                GameBoy.Emulator.HBlankHDMAdone = 0;
                mem->IO_Ports[HDMA5_REG-0xFF00] = 0xFF;
            }
            else
            {
                mem->IO_Ports[HDMA5_REG-0xFF00] = (mem->IO_Ports[HDMA5_REG-0xFF00] - 1) & 0x7F;
            }
        }

        return 1;
    }
    else if(GameBoy.Emulator.HDMAenabled == HDMA_GENERAL)
    {

        if(GameBoy.Emulator.gdma_preparation_time_countdown > 0) // 220 to 0
        {
            GameBoy.Emulator.gdma_preparation_time_countdown -= 1 << GameBoy.Emulator.DoubleSpeed;
            return 1;
        }

        if(GameBoy.Emulator.gbc_dma_working_for)
        {
            u32 source = (mem->IO_Ports[HDMA1_REG-0xFF00]<<8) | mem->IO_Ports[HDMA2_REG-0xFF00];
            u32 dest = ((mem->IO_Ports[HDMA3_REG-0xFF00]<<8) | mem->IO_Ports[HDMA4_REG-0xFF00]) + 0x8000;

            GB_MemWrite8(dest++,GB_MemRead8(source++));
            GB_MemWrite8(dest++,GB_MemRead8(source++));

            mem->IO_Ports[HDMA1_REG-0xFF00] = source >> 8;
            mem->IO_Ports[HDMA2_REG-0xFF00] = source & 0xFF;
            mem->IO_Ports[HDMA3_REG-0xFF00] = (dest >> 8) & 0x1F;
            mem->IO_Ports[HDMA4_REG-0xFF00] = dest & 0xFF;

            GameBoy.Emulator.gbc_dma_working_for--;

            if( (GameBoy.Emulator.gbc_dma_working_for & 7) == 0)
            {
                if(mem->IO_Ports[HDMA5_REG-0xFF00] == 0)
                {
                    GameBoy.Emulator.HDMAenabled = HDMA_NONE;
                    mem->IO_Ports[HDMA5_REG-0xFF00] = 0xFF;
                }
                else
                {
                    mem->IO_Ports[HDMA5_REG-0xFF00] = (mem->IO_Ports[HDMA5_REG-0xFF00] - 1) & 0x7F;
                }
            }

            return 1;
        }

    }

    return 0;
}


*/
//----------------------------------------------------------------
