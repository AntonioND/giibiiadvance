// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <time.h>

#include "../build_options.h"
#include "../debug_utils.h"

#include "cpu.h"
#include "debug.h"
#include "gameboy.h"
#include "gb_main.h"
#include "general.h"
#include "interrupts.h"
#include "memory.h"
#include "ppu.h"
#include "serial.h"
#include "video.h"

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

static const u32 gb_timer_clock_overflow_mask[4] = {
    1024 - 1, 16 - 1, 64 - 1, 256 - 1
};

//----------------------------------------------------------------

void GB_HandleRTC(void)
{
    if (!GameBoy.Emulator.HasTimer)
        return;

    // Handle RTC

    if (GameBoy.Emulator.Timer.halt != 0)
        return;

    // Increase timer

    GameBoy.Emulator.Timer.sec++;
    if (GameBoy.Emulator.Timer.sec < 60)
        return;

    GameBoy.Emulator.Timer.sec -= 60;
    GameBoy.Emulator.Timer.min++;

    if (GameBoy.Emulator.Timer.min < 60)
        return;

    GameBoy.Emulator.Timer.min -= 60;
    GameBoy.Emulator.Timer.hour++;

    if (GameBoy.Emulator.Timer.hour < 24)
        return;

    GameBoy.Emulator.Timer.hour -= 24;
    GameBoy.Emulator.Timer.days++;

    if (GameBoy.Emulator.Timer.days < 512)
        return;

    GameBoy.Emulator.Timer.days -= 512;
    GameBoy.Emulator.Timer.carry = 1;
    // The carry flag persists until the user clears it
}

//----------------------------------------------------------------

void GB_InterruptsInit(void)
{
    GameBoy.Memory.InterruptMasterEnable = 0;

    GameBoy.Memory.IO_Ports[IF_REG - 0xFF00] = 0xE0;

    GameBoy.Memory.IO_Ports[TIMA_REG - 0xFF00] = 0x00; // Verified on hardware
    GameBoy.Memory.IO_Ports[TMA_REG - 0xFF00] = 0x00;  // Verified on hardware

    GB_MemWrite8(TAC_REG, 0x00); // Verified on hardware

    if (GameBoy.Emulator.enable_boot_rom)
    {
        // The values of the registers at boot ROM init are actually unknown,
        // but one can guess...

        GameBoy.Memory.IO_Ports[P1_REG - 0xFF00] = 0xFF;

        switch (GameBoy.Emulator.HardwareType)
        {
            case HW_GB:
            case HW_GBP:
                // TODO: Can't verify until PPU is emulated correctly
                GameBoy.Emulator.sys_clocks = 8;
                break;

            case HW_SGB:
            case HW_SGB2:
                // TODO: Unknown. Can't test.
                GameBoy.Emulator.sys_clocks = 0;
                break;

            case HW_GBC:
                // Same value for GBC in GB or GBC mode. The boot ROM starts
                // the same way!
                // TODO: Can't verify until PPU is emulated correctly
                GameBoy.Emulator.sys_clocks = 0;
                break;

            case HW_GBA:
            case HW_GBA_SP:
                // Same value for GBC in GB or GBC mode. The boot ROM starts
                // the same way!
                // TODO: Can't verify until PPU is emulated correctly
                GameBoy.Emulator.sys_clocks = 0;
                break;

            default:
                GameBoy.Emulator.sys_clocks = 0;
                Debug_ErrorMsg("GB_InterruptsInit():\n"
                               "Unknown hardware\n"
                               "(boot ROM enabled)");
                break;
        }
    }
    else
    {
        switch (GameBoy.Emulator.HardwareType)
        {
            case HW_GB:
            case HW_GBP:
                GameBoy.Emulator.sys_clocks = 0xABCC; // Verified on hardware
                GameBoy.Memory.IO_Ports[P1_REG - 0xFF00] = 0xCF;
                break;

            case HW_SGB:
            case HW_SGB2:
                // TODO: Unknown. I can't test them. Not the same as DMG
                // (different boot ROM).
                GameBoy.Emulator.sys_clocks = 0;
                GameBoy.Memory.IO_Ports[P1_REG - 0xFF00] = 0xFF;
                break;

            case HW_GBC:
                if (GameBoy.Emulator.game_supports_gbc)
                {
                    // Verified on hardware
                    GameBoy.Emulator.sys_clocks = 0x1EA0;
                    GameBoy.Memory.IO_Ports[P1_REG - 0xFF00] = 0xCF;
                }
                else
                {
                    // Verified on hardware - The values are valid for a boot
                    // with no user interaction during boot (no buttons pressed)
                    GameBoy.Emulator.sys_clocks = 0x267C;
                    GameBoy.Memory.IO_Ports[P1_REG - 0xFF00] = 0xFF;
                }
                break;

            case HW_GBA:
            case HW_GBA_SP:
                if (GameBoy.Emulator.game_supports_gbc)
                {
                    // Verified on hardware
                    GameBoy.Emulator.sys_clocks = 0x1EA4;
                    GameBoy.Memory.IO_Ports[P1_REG - 0xFF00] = 0xCF;
                }
                else
                {
                    // Verified on hardware - The values are valid for a boot
                    // with no user interaction during boot (no buttons pressed)
                    GameBoy.Emulator.sys_clocks = 0x2680;
                    GameBoy.Memory.IO_Ports[P1_REG - 0xFF00] = 0xFF;
                }
                break;

            default:
                GameBoy.Emulator.sys_clocks = 0;
                Debug_ErrorMsg("GB_InterruptsInit():\n"
                               "Unknown hardware");
                break;
        }
    }

    GameBoy.Emulator.timer_overflow_mask = gb_timer_clock_overflow_mask[0];
    GameBoy.Emulator.timer_enabled = 0;
    GameBoy.Emulator.timer_irq_delay_active = 0;
    GameBoy.Emulator.timer_reload_delay_active = 0;
    GameBoy.Emulator.tima_just_reloaded = 0;

    GameBoy.Emulator.joypad_signal = 1;

    GameBoy.Memory.IO_Ports[DIV_REG - 0xFF00] = GameBoy.Emulator.sys_clocks >> 8;

    if (GameBoy.Emulator.HasTimer)
    {
        // GameBoy.Emulator.Timer is already loaded

        GameBoy.Emulator.LatchedTime.sec = 0;
        GameBoy.Emulator.LatchedTime.min = 0;
        GameBoy.Emulator.LatchedTime.hour = 0;
        GameBoy.Emulator.LatchedTime.days = 0;
        GameBoy.Emulator.LatchedTime.carry = 0;
        GameBoy.Emulator.LatchedTime.halt = 0; //GameBoy.Emulator.Timer.halt ?
    }
}

void GB_InterruptsEnd(void)
{
    // Nothing to do
}

//----------------------------------------------------------------

int GB_InterruptsExecute(void)
{
    _GB_CPU_ *cpu = &GameBoy.CPU;
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    int executed_clocks = 0;

    int interrupts = mem->IO_Ports[IF_REG - 0xFF00]
                     & mem->HighRAM[IE_REG - 0xFF80] & 0x1F;
    if (interrupts != 0)
    {
        if (mem->InterruptMasterEnable) // Execute interrupt and clear IME
        {
            int start_clocks = GB_CPUClockCounterGet();
#if 0
            mem->InterruptMasterEnable = 0; // Clear IME

            if (GameBoy.Emulator.CPUHalt == 1)
            {
                // Extra cycle needed to exit halt mode
                GB_CPUClockCounterAdd(4);
                GameBoy.Emulator.CPUHalt = 0;
            }

            if (interrupts & I_VBLANK)
            {
                GB_CPUClockCounterAdd(8);
                cpu->R16.SP = (cpu->R16.SP - 1) & 0xFFFF;
                GB_MemWrite8(cpu->R16.SP, cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP = (cpu->R16.SP - 1) & 0xFFFF;
                GB_MemWrite8(cpu->R16.SP, cpu->R8.PCL);
                cpu->R16.PC = 0x0040;
                mem->IO_Ports[IF_REG - 0xFF00] &= ~I_VBLANK;
                GB_CPUClockCounterAdd(8);
            }
            else if (interrupts & I_STAT)
            {
                if (((GameBoy.Emulator.HardwareType == HW_GB)
                    || (GameBoy.Emulator.HardwareType == HW_GBP)
                    || (GameBoy.Emulator.HardwareType == HW_SGB)
                    || (GameBoy.Emulator.HardwareType == HW_SGB2))
                    && (GameBoy.Emulator.CPUHalt == 0))
                        GB_CPUClockCounterAdd(4);

                GB_CPUClockCounterAdd(8);
                cpu->R16.SP = (cpu->R16.SP - 1) & 0xFFFF;
                GB_MemWrite8(cpu->R16.SP, cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP = (cpu->R16.SP - 1) & 0xFFFF;
                GB_MemWrite8(cpu->R16.SP, cpu->R8.PCL);
                cpu->R16.PC = 0x0048;
                mem->IO_Ports[IF_REG - 0xFF00] &= ~I_STAT;
                GB_CPUClockCounterAdd(8);
            }
            else
            {
                GB_CPUClockCounterAdd(8);
                cpu->R16.SP = (cpu->R16.SP - 1) & 0xFFFF;
                GB_MemWrite8(cpu->R16.SP, cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP = (cpu->R16.SP - 1) & 0xFFFF;
                GB_MemWrite8(cpu->R16.SP, cpu->R8.PCL);
                {
                    if (interrupts & I_TIMER)
                    {
                        cpu->R16.PC = 0x0050;
                        mem->IO_Ports[IF_REG - 0xFF00] &= ~I_TIMER;
                    }
                    else if (interrupts & I_SERIAL)
                    {
                        cpu->R16.PC = 0x0058;
                        mem->IO_Ports[IF_REG - 0xFF00] &= ~I_SERIAL;
                    }
                    else //if (interrupts & I_JOYPAD)
                    {
                        cpu->R16.PC = 0x0060;
                        mem->IO_Ports[IF_REG - 0xFF00] &= ~I_JOYPAD;
                    }
                }
                GB_CPUClockCounterAdd(8);
            }
#endif
            mem->InterruptMasterEnable = 0; // Clear IME

            if (GameBoy.Emulator.CPUHalt == 1)
            {
                // Extra cycle needed to exit halt mode
                GB_CPUClockCounterAdd(4);
                GameBoy.Emulator.CPUHalt = 0;
            }

            GB_CPUClockCounterAdd(8);
            cpu->R16.SP = (cpu->R16.SP - 1) & 0xFFFF;
            GB_MemWrite8(cpu->R16.SP, cpu->R8.PCH);
            GB_CPUClockCounterAdd(4);
            cpu->R16.SP = (cpu->R16.SP - 1) & 0xFFFF;
            GB_MemWrite8(cpu->R16.SP, cpu->R8.PCL);
            {
                if (interrupts & I_VBLANK)
                {
                    cpu->R16.PC = 0x0040;
                    mem->IO_Ports[IF_REG - 0xFF00] &= ~I_VBLANK;
                }
                else if (interrupts & I_STAT)
                {
                    cpu->R16.PC = 0x0048;
                    mem->IO_Ports[IF_REG - 0xFF00] &= ~I_STAT;
                }
                else if (interrupts & I_TIMER)
                {
                    cpu->R16.PC = 0x0050;
                    mem->IO_Ports[IF_REG - 0xFF00] &= ~I_TIMER;
                }
                else if (interrupts & I_SERIAL)
                {
                    cpu->R16.PC = 0x0058;
                    mem->IO_Ports[IF_REG - 0xFF00] &= ~I_SERIAL;
                }
                else //if (interrupts & I_JOYPAD)
                {
                    cpu->R16.PC = 0x0060;
                    mem->IO_Ports[IF_REG - 0xFF00] &= ~I_JOYPAD;
                }
            }
            GB_CPUClockCounterAdd(8);

            int end_clocks = GB_CPUClockCounterGet();
            executed_clocks = end_clocks - start_clocks;
        }
        else
        {
            // Exit HALT mode regardless of IME
            if (GameBoy.Emulator.CPUHalt == 1) // If HALT
            {
                // 4 clocks are needed to exit HALT mode
                GameBoy.Emulator.CPUHalt = 0;
                GB_CPUClockCounterAdd(4);
                executed_clocks = 4;
            }
        }
    }

    return executed_clocks;
}

//----------------------------------------------------------------

void GB_InterruptsSetFlag(int flag)
{
    GameBoy.Memory.IO_Ports[IF_REG - 0xFF00] |= flag;
    // This is checked in GB_IRQExecute(), not here!
    //if (GameBoy.Memory.HighRAM[IE_REG - 0xFF80] & flag)
    //{
    //    // Only exit from HALT mode, not STOP
    //    if(GameBoy.Emulator.CPUHalt == 1)
    //    {
    //        // Clear halt regardless of IME
    //        GameBoy.Emulator.CPUHalt = 0;
    //    }
    //}
    GB_CPUBreakLoop();
}

//----------------------------------------------------------------

void GB_InterruptsWriteIE(int value) // reference_clocks not needed
{
    GameBoy.Memory.HighRAM[IE_REG - 0xFF80] = value;
    GB_CPUBreakLoop();
}

void GB_InterruptsWriteIF(int reference_clocks, int value)
{
    //GB_UpdateCounterToClocks(reference_clocks);
    GB_PPUUpdateClocksCounterReference(reference_clocks);
    GB_TimersUpdateClocksCounterReference(reference_clocks);
    GB_SerialUpdateClocksCounterReference(reference_clocks);
    GameBoy.Memory.IO_Ports[IF_REG - 0xFF00] = value | (0xE0);
    GB_CPUBreakLoop();
}

//----------------------------------------------------------------

static int gb_timer_clock_counter = 0;

void GB_TimersClockCounterReset(void)
{
    gb_timer_clock_counter = 0;
}

static int GB_TimersClockCounterGet(void)
{
    return gb_timer_clock_counter;
}

static void GB_TimersClockCounterSet(int new_reference_clocks)
{
    gb_timer_clock_counter = new_reference_clocks;
}

//----------------------------------------------------------------

static void GB_TimerIncreaseTIMA(void)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (mem->IO_Ports[TIMA_REG - 0xFF00] == 0xFF) // If overflow
    {
        // There's a 4 clock delay between overflow and IF flag being set
        //GB_InterruptsSetFlag(I_TIMER);
        GameBoy.Emulator.timer_irq_delay_active = 4;

        mem->IO_Ports[TIMA_REG - 0xFF00] = 0; //mem->IO_Ports[TMA_REG - 0xFF00];
        GameBoy.Emulator.timer_reload_delay_active = 4;
    }
    else
    {
        mem->IO_Ports[TIMA_REG - 0xFF00]++;
    }
}

//----------------------------------------------------------------

void GB_TimersWriteDIV(int reference_clocks, unused__ int value)
{
    GB_CPUBreakLoop();

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    GB_TimersUpdateClocksCounterReference(reference_clocks);

    if (GameBoy.Emulator.timer_enabled)
    {
        // Falling edge in timer bit
        if (GameBoy.Emulator.sys_clocks
            & ((GameBoy.Emulator.timer_overflow_mask + 1) >> 1))
        {
            GB_TimerIncreaseTIMA();
        }
    }

    GameBoy.Emulator.sys_clocks = 0;
    mem->IO_Ports[DIV_REG - 0xFF00] = 0;
}

void GB_TimersWriteTIMA(int reference_clocks, int value)
{
    GB_CPUBreakLoop();

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    GB_TimersUpdateClocksCounterReference(reference_clocks);

    if (GameBoy.Emulator.timer_enabled)
    {
        if ((GameBoy.Emulator.sys_clocks
            & GameBoy.Emulator.timer_overflow_mask) == 0)
        {
            // If TIMA is written the same clock as incrementing itself prevent
            // the timer IF flag from being set
            GameBoy.Emulator.timer_irq_delay_active = 0;
            // Also, prevent TIMA being loaded from TMA
            GameBoy.Emulator.timer_reload_delay_active = 0;
        }
    }

    // The same clock it is reloaded from TMA, it has higher priority than
    // writing to it
    if (GameBoy.Emulator.tima_just_reloaded == 0)
        mem->IO_Ports[TIMA_REG - 0xFF00] = value;
}

void GB_TimersWriteTMA(int reference_clocks, int value)
{
    GB_CPUBreakLoop();

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    // First, update as normal
    GB_TimersUpdateClocksCounterReference(reference_clocks);

    if (GameBoy.Emulator.timer_enabled)
    {
        if (GameBoy.Emulator.tima_just_reloaded)
        {
            // If TMA is written the same clock as reloading TIMA from TMA, load
            // TIMA from written value, but handle IRQ flag before.
            mem->IO_Ports[TIMA_REG - 0xFF00] = value;
        }
    }

    mem->IO_Ports[TMA_REG - 0xFF00] = value;
}

void GB_TimersWriteTAC(int reference_clocks, int value)
{
    GB_CPUBreakLoop();

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    GB_TimersUpdateClocksCounterReference(reference_clocks);

    // Okay, so this register works differently in each hardware... Try to
    // emulate each one of them.

    int old_tac = mem->IO_Ports[TAC_REG - 0xFF00];
    int new_tac = value;

    int old_enable = old_tac & BIT(2);
    int new_enable = new_tac & BIT(2);

    int old_clocks_half = (gb_timer_clock_overflow_mask[old_tac & 3] + 1) >> 1;
    int new_clocks_half = (gb_timer_clock_overflow_mask[new_tac & 3] + 1) >> 1;

    int sys_clocks = GameBoy.Emulator.sys_clocks;

    int glitch = 0;

    // TODO: I can't test SGB or SGB2! I suppose they work the same way as DMG
    if ((GameBoy.Emulator.HardwareType == HW_GB)
        || (GameBoy.Emulator.HardwareType == HW_GBP)
        || (GameBoy.Emulator.HardwareType == HW_SGB)
        || (GameBoy.Emulator.HardwareType == HW_SGB2))
    {
        if (old_enable == 0)
        {
            glitch = 0;
        }
        else
        {
            if (new_enable == 0)
            {
                glitch = sys_clocks & old_clocks_half;
            }
            else
            {
                glitch = (sys_clocks & old_clocks_half)
                         && ((sys_clocks & new_clocks_half) == 0);
            }
        }
    }
    else if ((GameBoy.Emulator.HardwareType == HW_GBC)
             || (GameBoy.Emulator.HardwareType == HW_GBA)
             || (GameBoy.Emulator.HardwareType == HW_GBA_SP))
    {
        // GBC/GBA/GBA SP are different, but it can't be emulated correctly.

        if (old_enable == 0)
        {
            // This part is different from DMG. A lot glitchier, too... And
            // inconsistent between GBCs.
            if (new_enable == 0)
            {
                glitch = 0;
            }
            else
            {
                // Approximate
                glitch = (sys_clocks & old_clocks_half)
                         && ((sys_clocks & new_clocks_half) == 0);

#if 0
                // Hardware simulation?
                int sysb3 = (sys_clocks & BIT(3)) != 0;
                int sysb5 = (sys_clocks & BIT(5)) != 0;
                int sysb7 = (sys_clocks & BIT(7)) != 0;
                int sysb9 = (sys_clocks & BIT(9)) != 0;

                int step0 = (old_tac & 2) ?
                                ((old_tac&1) ? (sysb7) : (sysb5)) :  // 11 10
                                ((old_tac&1) ? (sysb3) : (sysb9));   // 01 00

                int step1 = (new_tac & 2) ?
                                ((old_tac&1) ? (sysb7) : (sysb5)) :  // 11 10
                                ((old_tac&1) ? (sysb3) : (sysb9));   // 01 00

                int step2 = (new_tac & 2) ?
                                ((new_tac&1) ? (sysb7) : (sysb5)) :  // 11 10
                                ((new_tac&1) ? (sysb3) : (sysb9));   // 01 00

                glitch = (step0 && !step1) ^ (step1 && !step2);
#endif
                // Exact emulation of one of my GBCs
#if 0
                if (old_clocks_half == new_clocks_half)
                {
                    glitch = 0;
                }
                else
                {
                    int new_tac_bits = new_tac & 7;

                    switch (old_tac & 3)
                    {
                        case 0:
                            if (new_tac_bits == 5)
                            {
                                glitch = (sys_clocks & old_clocks_half)
                                       && ((sys_clocks & new_clocks_half) == 0);
                            }
                            else if (new_tac_bits == 7)
                            {
                                glitch = (sys_clocks & old_clocks_half)
                                       && ((sys_clocks & new_clocks_half) == 0)
                                       && (sys_clocks & 32); // ????
                            }
                            else
                            {
                                glitch = 0;
                            }
                            break;

                        case 1:
                            glitch = 0;
                            break;

                        case 2:
                            if (new_tac_bits == 5)
                            {
                                glitch = (sys_clocks & old_clocks_half)
                                       && ((sys_clocks & new_clocks_half) == 0)
                                       && (sys_clocks & 512); // ????
                            }
                            else if (new_tac_bits == 7)
                            {
                                glitch = (sys_clocks & old_clocks_half)
                                       && ((sys_clocks & new_clocks_half) == 0);
                            }
                            else
                            {
                                glitch = 0;
                            }
                            break;

                        case 3:
                            if (new_tac_bits == 6)
                            {
                                glitch = (sys_clocks & old_clocks_half)
                                       && ((sys_clocks & new_clocks_half) == 0);
                            else
                                glitch = 0;
                            break;

                        default:
                            glitch = 0;
                            break;
                    }
                }
#endif
            }
        }
        else
        {
            if (new_enable == 0)
            {
                glitch = sys_clocks & old_clocks_half;
            }
            else
            {
                glitch = (sys_clocks & old_clocks_half)
                         && ((sys_clocks & new_clocks_half) == 0);
            }
        }
    }

    if (glitch)
        GB_TimerIncreaseTIMA();

    if (value & BIT(2))
        GameBoy.Emulator.timer_enabled = 1;
    else
        GameBoy.Emulator.timer_enabled = 0;

    GameBoy.Emulator.timer_overflow_mask =
            gb_timer_clock_overflow_mask[value & 3];

    mem->IO_Ports[TAC_REG - 0xFF00] = value;
}

static void GB_TimersUpdateDelays(int increment_clocks)
{
    if (GameBoy.Emulator.timer_irq_delay_active)
    {
        if (GameBoy.Emulator.timer_irq_delay_active > increment_clocks)
        {
            GameBoy.Emulator.timer_irq_delay_active -= increment_clocks;
        }
        else
        {
            GameBoy.Emulator.timer_irq_delay_active = 0;
            GB_InterruptsSetFlag(I_TIMER);
        }
    }

    if (GameBoy.Emulator.timer_reload_delay_active)
    {
        if (GameBoy.Emulator.timer_reload_delay_active > increment_clocks)
        {
            GameBoy.Emulator.timer_reload_delay_active -= increment_clocks;
        }
        else
        {
            if ((increment_clocks
                            - GameBoy.Emulator.timer_reload_delay_active) < 4)
            {
                GameBoy.Emulator.tima_just_reloaded = 1;
            }

            GameBoy.Emulator.timer_reload_delay_active = 0;
            GameBoy.Memory.IO_Ports[TIMA_REG - 0xFF00] =
                    GameBoy.Memory.IO_Ports[TMA_REG - 0xFF00];
        }
    }
}

void GB_TimersUpdateClocksCounterReference(int reference_clocks)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    int increment_clocks = reference_clocks - GB_TimersClockCounterGet();

    // Don't assume that this function will increase just a few clocks. Timer
    // can be increased up to 4 times just because of HDMA needing 64 clocks in
    // double speed mode (min. timer period is 16 clocks).

    // Sound
    // -----

    // TODO : SOUND UPDATE GOES HERE!!

    // Timer
    // -----

    GameBoy.Emulator.tima_just_reloaded = 0;

    GB_TimersUpdateDelays(increment_clocks);

    if (GameBoy.Emulator.timer_enabled)
    {
        int timer_update_clocks =
                (GameBoy.Emulator.sys_clocks & GameBoy.Emulator.timer_overflow_mask)
                + increment_clocks;

        int timer_overflow_count = GameBoy.Emulator.timer_overflow_mask + 1;

        if (timer_update_clocks >= timer_overflow_count)
        {
            while (timer_update_clocks >= timer_overflow_count)
            {
                GB_TimerIncreaseTIMA();

                timer_update_clocks -= timer_overflow_count;

                GB_TimersUpdateDelays(timer_update_clocks);
            }
        }
        else
        {
            GB_TimersUpdateDelays(timer_update_clocks);
        }
    }

    // DIV
    // ---

    GameBoy.Emulator.sys_clocks =
            (GameBoy.Emulator.sys_clocks + increment_clocks) & 0xFFFF;

    // Upper 8 bits of the 16 register sys_clocks
    mem->IO_Ports[DIV_REG - 0xFF00] = (GameBoy.Emulator.sys_clocks >> 8);

    // Done...

    GB_TimersClockCounterSet(reference_clocks);
}

int GB_TimersGetClocksToNextEvent(void)
{
    // DIV
    int clocks_to_next_event = 256 - (GameBoy.Emulator.sys_clocks & 0xFF);

    // It seems that TAC can't disable the IRQ once it is prepared
    if (GameBoy.Emulator.timer_irq_delay_active)
    {
        if (GameBoy.Emulator.timer_irq_delay_active < clocks_to_next_event)
            clocks_to_next_event = GameBoy.Emulator.timer_irq_delay_active;
    }

    if (GameBoy.Emulator.timer_enabled)
    {
        int mask = GameBoy.Emulator.timer_overflow_mask;
        int timer_counter = GameBoy.Emulator.sys_clocks & mask;

        // Clocks left for next increment
        int clocks_left_for_timer = (mask + 1) - timer_counter;
        // Clocks left for next overflow
        clocks_left_for_timer +=
                (255 - GameBoy.Memory.IO_Ports[TIMA_REG - 0xFF00]) * (mask + 1);

        if (clocks_left_for_timer < clocks_to_next_event)
            clocks_to_next_event = clocks_left_for_timer;

        if (GameBoy.Emulator.timer_reload_delay_active)
        {
            if (GameBoy.Emulator.timer_reload_delay_active < clocks_to_next_event)
                clocks_to_next_event = GameBoy.Emulator.timer_reload_delay_active;
        }
    }

    return clocks_to_next_event;
}

//----------------------------------------------------------------

// Important: Must be called at least once per frame!
void GB_CheckJoypadInterrupt(void)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    int old_signal = GameBoy.Emulator.joypad_signal;

    u32 p1 = mem->IO_Ports[P1_REG - 0xFF00];
    u32 result = 0;

    // TODO: This will fail in SGB multiplayer mode, but who cares?
    int Keys = GB_Input_Get(0);
    if ((p1 & (1 << 5)) == 0) // A-B-SEL-STA
        result |= Keys & (KEY_A | KEY_B | KEY_SELECT | KEY_START);
    if ((p1 & (1 << 4)) == 0) // PAD
        result |= Keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT);

    GameBoy.Emulator.joypad_signal = result ? 0 : 1;

    if ((GameBoy.Emulator.joypad_signal == 0) && (old_signal))
    {
        GB_InterruptsSetFlag(I_JOYPAD);

        if (GameBoy.Emulator.CPUHalt == 2) // Exit stop mode (in any hardware)
            GameBoy.Emulator.CPUHalt = 0;

        GB_CPUBreakLoop();
    }

    return;
}
