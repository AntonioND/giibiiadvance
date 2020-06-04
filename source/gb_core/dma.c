// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include "gameboy.h"

#include "cpu.h"
#include "memory.h"

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//----------------------------------------------------------------

void GB_DMAInit(void)
{
    // DMA
    GameBoy.Emulator.OAM_DMA_enabled = 0;
    GameBoy.Emulator.OAM_DMA_clocks_elapsed = 0;
    GameBoy.Emulator.OAM_DMA_src = 0;
    GameBoy.Emulator.OAM_DMA_dst = 0;
    GameBoy.Emulator.OAM_DMA_last_read_byte = 0;

    // HDMA/GDMA
    GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;

    GameBoy.Emulator.gdma_preparation_clocks_left = 0;
    GameBoy.Emulator.gdma_copy_clocks_left = 0;
    GameBoy.Emulator.gdma_src = 0;
    GameBoy.Emulator.gdma_dst = 0;
    GameBoy.Emulator.gdma_bytes_left = 0;

    GameBoy.Emulator.hdma_last_ly_copied = -1;

    if (GameBoy.Emulator.CGBEnabled == 1)
        GameBoy.Memory.IO_Ports[HDMA5_REG - 0xFF00] = 0xFF;
}

void GB_DMAEnd(void)
{
    // Nothing here...
}

//----------------------------------------------------------------

static void GB_DMAInitOAMCopy(int src_addr_high_byte)
{
    GameBoy.Emulator.OAM_DMA_enabled = 1;
    GameBoy.Emulator.OAM_DMA_clocks_elapsed = 0;
    GameBoy.Emulator.OAM_DMA_src = src_addr_high_byte << 8;
    GameBoy.Emulator.OAM_DMA_dst = 0xFE00;
    GameBoy.Emulator.OAM_DMA_last_read_byte = 0x00;
    // Change read and write functions
}

static void GB_DMAInitGBCCopy(int register_value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    GameBoy.Emulator.GBC_DMA_enabled = (register_value & (1 << 7)) ?
                                        GBC_DMA_HBLANK : GBC_DMA_GENERAL;

    if (GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_HBLANK)
    {
        mem->IO_Ports[HDMA5_REG - 0xFF00] &= 0x7F;

        u32 blocks = (((u32)register_value) & 0x7F) + 1;
        GameBoy.Emulator.gdma_src = (mem->IO_Ports[HDMA1_REG - 0xFF00] << 8)
                                    | mem->IO_Ports[HDMA2_REG - 0xFF00];
        GameBoy.Emulator.gdma_dst = ((mem->IO_Ports[HDMA3_REG - 0xFF00] << 8)
                                     | mem->IO_Ports[HDMA4_REG - 0xFF00])
                                    + 0x8000;
        GameBoy.Emulator.gdma_bytes_left = blocks * 16;
        GameBoy.Emulator.gdma_preparation_clocks_left = 0;
        GameBoy.Emulator.hdma_last_ly_copied = -1; // Reset

        //Debug_DebugMsgArg("HDMA\n"
        //                  "SRC = %04X\n"
        //                  "DST = %04X\n"
        //                  "BYTES = %d", GameBoy.Emulator.gdma_src,
        //                  GameBoy.Emulator.gdma_dst,
        //                  GameBoy.Emulator.gdma_bytes_left);
    }
    else
    {
        // Timing information in GB_DMAExecute()

        u32 blocks = (((u32)register_value) & 0x7F) + 1;

        if (GameBoy.Emulator.DoubleSpeed) // Tested on hardware
        {
            GameBoy.Emulator.gdma_preparation_clocks_left = 4;
            GameBoy.Emulator.gdma_copy_clocks_left = blocks * 64;
        }
        else
        {
            GameBoy.Emulator.gdma_preparation_clocks_left = 4;
            GameBoy.Emulator.gdma_copy_clocks_left = blocks * 32;
        }

        GameBoy.Emulator.gdma_src = (mem->IO_Ports[HDMA1_REG - 0xFF00] << 8)
                                    | mem->IO_Ports[HDMA2_REG - 0xFF00];
        GameBoy.Emulator.gdma_dst = ((mem->IO_Ports[HDMA3_REG - 0xFF00] << 8)
                                     | mem->IO_Ports[HDMA4_REG - 0xFF00])
                                    + 0x8000;
        GameBoy.Emulator.gdma_bytes_left = blocks * 16;

        //_gb_break_to_debugger();
        //Debug_DebugMsgArg("GDMA\n"
        //                  "SRC = %04X\n"
        //                  "DST = %04X\n""
        //                  BYTES = %d", GameBoy.Emulator.gdma_src,
        //                  GameBoy.Emulator.gdma_dst,
        //                  GameBoy.Emulator.gdma_bytes_left);
    }
}

static void GB_DMAStopGBCCopy(void)
{
    GameBoy.Emulator.gdma_bytes_left = 0;
    GameBoy.Emulator.hdma_last_ly_copied = -1; // Reset

    GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;

    // Verified on hardware (GBC and GBA SP)
    GameBoy.Memory.IO_Ports[HDMA5_REG - 0xFF00] |= 0x80;
}

//----------------------------------------------------------------

static int gb_dma_clock_counter = 0;

void GB_DMAClockCounterReset(void)
{
    gb_dma_clock_counter = 0;
}

static int GB_DMAClockCounterGet(void)
{
    return gb_dma_clock_counter;
}

static void GB_DMAClockCounterSet(int new_reference_clocks)
{
    gb_dma_clock_counter = new_reference_clocks;
}

void GB_DMAUpdateClocksCounterReference(int reference_clocks)
{
    if (GameBoy.Emulator.OAM_DMA_enabled)
    {
        // This needs 160 * 4 + 4 clocks to end

        int increment_clocks = reference_clocks - GB_DMAClockCounterGet();

        GameBoy.Emulator.OAM_DMA_clocks_elapsed += increment_clocks;

        u32 last_destination_to_copy =
                0xFE00 + (GameBoy.Emulator.OAM_DMA_clocks_elapsed / 4) - 2;

        while (GameBoy.Emulator.OAM_DMA_dst <= last_destination_to_copy)
        {
            if (GameBoy.Emulator.OAM_DMA_dst >= 0xFEA0)
                break;

            GameBoy.Emulator.OAM_DMA_last_read_byte =
                    GB_MemReadDMA8(GameBoy.Emulator.OAM_DMA_src++);
            GB_MemWriteDMA8(GameBoy.Emulator.OAM_DMA_dst++,
                            GameBoy.Emulator.OAM_DMA_last_read_byte);
        }

        if (last_destination_to_copy >= 0xFEA0)
        {
            GameBoy.Emulator.OAM_DMA_enabled = 0;
        }
    }

    GB_DMAClockCounterSet(reference_clocks);
}

int GB_DMAGetClocksToNextEvent(void)
{
    int clocks_to_next_event = 0x7FFFFFFF;

    if (GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_GENERAL)
    {
        if (GameBoy.Emulator.gdma_preparation_clocks_left > 0)
            clocks_to_next_event = GameBoy.Emulator.gdma_preparation_clocks_left;
        else if (GameBoy.Emulator.gdma_copy_clocks_left > 0)
            clocks_to_next_event = GameBoy.Emulator.gdma_copy_clocks_left;
    }
    //else if (GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_HBLANK)
    //{
    //    return clocks to video mode 0. That is done already in
    //    GB_PPUGetClocksToNextEvent()
    //}

    if (GameBoy.Emulator.OAM_DMA_enabled)
    {
        int clocks = (160 * 4 + 4) + 4 - GameBoy.Emulator.OAM_DMA_clocks_elapsed;
        if (clocks < clocks_to_next_event)
            clocks_to_next_event = clocks;
    }

    return clocks_to_next_event;
}

//----------------------------------------------------------------

void GB_DMAWriteDMA(int reference_clocks, int value)
{
    GB_DMAUpdateClocksCounterReference(reference_clocks);
    // TODO
    // It should disable non-highram memory (if source is VRAM, VRAM is disabled
    // instead of other ram)... etc...
    GameBoy.Memory.IO_Ports[DMA_REG - 0xFF00] = value;
    GB_DMAInitOAMCopy(value);
    GB_CPUBreakLoop();
}

void GB_DMAWriteHDMA1(int value) // reference_clocks not needed
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;
    mem->IO_Ports[HDMA1_REG - 0xFF00] = value;
    GameBoy.Emulator.gdma_src = (mem->IO_Ports[HDMA1_REG - 0xFF00] << 8)
                                | GameBoy.Memory.IO_Ports[HDMA2_REG - 0xFF00];
}

void GB_DMAWriteHDMA2(int value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;
    mem->IO_Ports[HDMA2_REG - 0xFF00] = value & 0xF0; // 4 lower bits ignored
    GameBoy.Emulator.gdma_src = (mem->IO_Ports[HDMA1_REG - 0xFF00] << 8)
                                | GameBoy.Memory.IO_Ports[HDMA2_REG - 0xFF00];
}

void GB_DMAWriteHDMA3(int value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;
    mem->IO_Ports[HDMA3_REG - 0xFF00] = value & 0x1F; // Dest is VRAM
    GameBoy.Emulator.gdma_dst = ((mem->IO_Ports[HDMA3_REG - 0xFF00] << 8)
                                 | mem->IO_Ports[HDMA4_REG - 0xFF00])
                                + 0x8000;
}

void GB_DMAWriteHDMA4(int value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;
    mem->IO_Ports[HDMA4_REG - 0xFF00] = value & 0xF0; // 4 lower bits ignored
    GameBoy.Emulator.gdma_dst = ((mem->IO_Ports[HDMA3_REG - 0xFF00] << 8)
                                 | mem->IO_Ports[HDMA4_REG - 0xFF00])
                                + 0x8000;
}

void GB_DMAWriteHDMA5(int value)
{
    //Start/Stop GBC DMA copy

    GB_CPUBreakLoop();

    GameBoy.Memory.IO_Ports[HDMA5_REG - 0xFF00] = value;

    if (GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_NONE)
    {
        GB_DMAInitGBCCopy(value);
    }
    else //if (GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_HBLANK)
    {
        // In GBC_DMA_GENERAL mode the CPU is blocked, not needed to check if
        // that is the current mode
        if (value & BIT(7))
        {
            // Update copy with new size - continue HDMA mode, not with GDMA
            GB_DMAInitGBCCopy(value);
        }
        else
        {
            GB_DMAStopGBCCopy();
        }
    }
}

//----------------------------------------------------------------

int GB_DMAExecute(int clocks)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    int executed_clocks = 0;

    // This code assumes that this function is called as soon as the CPU has
    // requested a GDMA, or as soon as entering mode 0 if there is a HDMA copy
    // running. The only possible delay is the one caused by finishing the CPU
    // instruction or IRQ handling.

    if (GameBoy.Emulator.CPUHalt != 0) // Only copy if CPU is not halted
        return 0;

    if (GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_GENERAL)
    {
        //This copy has to be divided. Only execute the specified clocks.

        int start_clocks = GB_CPUClockCounterGet();

        while (clocks > 0)
        {
            if (GameBoy.Emulator.gdma_preparation_clocks_left > 0)
            {
                if (clocks >= GameBoy.Emulator.gdma_preparation_clocks_left)
                {
                    GB_CPUClockCounterAdd(
                            GameBoy.Emulator.gdma_preparation_clocks_left);
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
            else if (GameBoy.Emulator.gdma_copy_clocks_left > 0)
            {
                int execute_now_clocks =
                        (clocks < GameBoy.Emulator.gdma_copy_clocks_left) ?
                        clocks : GameBoy.Emulator.gdma_copy_clocks_left;

                int clocks_per_byte_mask =
                        (2 << GameBoy.Emulator.DoubleSpeed) - 1;

                u32 bytes = GameBoy.Emulator.gdma_bytes_left;

                while (execute_now_clocks--)
                {
                    GameBoy.Emulator.gdma_copy_clocks_left--;
                    GB_CPUClockCounterAdd(1);

                    if ((GameBoy.Emulator.gdma_copy_clocks_left
                                                & clocks_per_byte_mask) == 0)
                    {
                        u8 val = GB_MemReadHDMA8(GameBoy.Emulator.gdma_src++);
                        GB_MemWriteHDMA8(GameBoy.Emulator.gdma_dst++, val);

                        bytes--;
                        if (bytes == 0)
                        {
                            GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;
                            break;
                        }
                    }
                }

                GameBoy.Emulator.gdma_bytes_left = bytes;

                mem->IO_Ports[HDMA1_REG - 0xFF00] =
                        GameBoy.Emulator.gdma_src >> 8;
                mem->IO_Ports[HDMA2_REG - 0xFF00] =
                        GameBoy.Emulator.gdma_src & 0xFF;
                mem->IO_Ports[HDMA3_REG - 0xFF00] =
                        (GameBoy.Emulator.gdma_dst >> 8) & 0x1F;
                mem->IO_Ports[HDMA4_REG - 0xFF00] =
                        GameBoy.Emulator.gdma_dst & 0xFF;

                if (GameBoy.Emulator.gdma_bytes_left > 0)
                {
                    mem->IO_Ports[HDMA5_REG - 0xFF00] =
                            (GameBoy.Emulator.gdma_bytes_left - 1) / 16;
                }
                else
                {
                    // When finished, block count = -1
                    mem->IO_Ports[HDMA5_REG - 0xFF00] = 0xFF;
                }

                clocks = 0; // exit loop
            }
        }

        int end_clocks = GB_CPUClockCounterGet();

        executed_clocks = end_clocks - start_clocks;
    }
    else if (GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_HBLANK)
    {
        // This doesn't need to be divided. Worst case is 64 clocks.

        if (GameBoy.Emulator.CPUHalt == 0)
        {
            int current_ly = -1;

            if (GameBoy.Emulator.lcd_on)
            {
                current_ly = mem->IO_Ports[LY_REG - 0xFF00];
            }
            else
            {
                // If screen is off, copy one block, continue when screen is
                // switched on.
                current_ly = -2;
            }

            if (GameBoy.Emulator.hdma_last_ly_copied != current_ly)
            {
                if ((!GameBoy.Emulator.lcd_on)
                    || ((GameBoy.Emulator.ScreenMode == 0)
                        && GameBoy.Emulator.ly_drawn))
                {
                    GameBoy.Emulator.hdma_last_ly_copied = current_ly;
                    if (current_ly < 144)
                    {
                        int start_clocks = GB_CPUClockCounterGet();

                        GB_CPUClockCounterAdd(4); // Init copy

                        for (int i = 0; i < 16; i++)
                        {
                            // The copy needs the same time in single and double
                            // speeds mode.
                            GB_CPUClockCounterAdd(2 << GameBoy.Emulator.DoubleSpeed);
                            u8 v = GB_MemReadHDMA8(GameBoy.Emulator.gdma_src++);
                            GB_MemWriteHDMA8(GameBoy.Emulator.gdma_dst++, v);
                        }

                        mem->IO_Ports[HDMA1_REG - 0xFF00] =
                                GameBoy.Emulator.gdma_src >> 8;
                        mem->IO_Ports[HDMA2_REG - 0xFF00] =
                                GameBoy.Emulator.gdma_src & 0xFF;
                        mem->IO_Ports[HDMA3_REG - 0xFF00] =
                                (GameBoy.Emulator.gdma_dst >> 8) & 0x1F;
                        mem->IO_Ports[HDMA4_REG - 0xFF00] =
                                GameBoy.Emulator.gdma_dst & 0xFF;

                        GameBoy.Emulator.gdma_bytes_left -= 16;

                        if (GameBoy.Emulator.gdma_bytes_left == 0)
                        {
                            GameBoy.Emulator.GBC_DMA_enabled = GBC_DMA_NONE;
                            GameBoy.Emulator.hdma_last_ly_copied = -1;
                            mem->IO_Ports[HDMA5_REG - 0xFF00] = 0xFF;
                        }
                        else
                        {
                            mem->IO_Ports[HDMA5_REG - 0xFF00] =
                                    (GameBoy.Emulator.gdma_bytes_left - 1) / 16;
                        }

                        int end_clocks = GB_CPUClockCounterGet();

                        executed_clocks = end_clocks - start_clocks;
                    }
                }
            }
        }
    }

    return executed_clocks;
}
