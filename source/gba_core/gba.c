// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdio.h>
#include <stdlib.h>

#include "../build_options.h"
#include "../debug_utils.h"
#include "../file_utils.h"
#include "../png_utils.h"

#include "bios.h"
#include "cpu.h"
#include "dma.h"
#include "gba.h"
#include "interrupts.h"
#include "memory.h"
#include "rom.h"
#include "save.h"
#include "sound.h"
#include "timers.h"
#include "video.h"

static s32 clocks_to_next_event;
static s32 lastresidualclocks = 0;

static int inited = 0;

int GBA_ROM_SIZE;
int GBA_GetRomSize(void)
{
    return GBA_ROM_SIZE;
}

_cpu_t *GBA_CPUGet(void)
{
    return &CPU;
}

int GBA_InitRom(void *bios_ptr, void *rom_ptr, u32 romsize)
{
    if (inited)
        GBA_EndRom(1); // Shouldn't be needed here

    if (romsize > 0x02000000)
    {
        Debug_ErrorMsgArg("Rom too big!\n"
                          "Size = 0x%08X bytes\n"
                          "Max = 0x02000000 bytes",
                          romsize);
        GBA_ROM_SIZE = 0x02000000;
    }
    else
    {
        GBA_ROM_SIZE = romsize;
    }

    GBA_DetectSaveType(rom_ptr, romsize);
    GBA_ResetSaveBuffer();
    GBA_SaveReadFile();

    GBA_HeaderCheck(rom_ptr);

    GBA_CPUInit();
    GBA_InterruptInit();
    GBA_TimerInitAll();
    GBA_MemoryInit(bios_ptr, rom_ptr, GBA_ROM_SIZE);
    GBA_UpdateDrawScanlineFn();
    GBA_DMA0Setup();
    GBA_DMA1Setup();
    GBA_DMA2Setup();
    GBA_DMA3Setup();
    GBA_SoundInit();
    GBA_FillFadeTables();

    GBA_SkipFrame(0);

    clocks_to_next_event = 1;
    lastresidualclocks = 0;

    inited = 1;

    if (GBA_BiosIsLoaded() == 0)
        ConsolePrint("Using emulated BIOS...");
    else
        ConsolePrint("Using real BIOS...");

    // BIOS switch to GBC mode - Never used in real hardware
    //GBA_DebugAddBreakpoint(0x1954);

    return 1;
}

int GBA_EndRom(int save)
{
    if (inited == 0)
        return 0;

    if (save)
        GBA_SaveWriteFile();

    GBA_MemoryEnd();

    inited = 0;

    return 1;
}

void GBA_Reset(void)
{
    GBA_CPUClearHalted();
    GBA_Swi(0x26);
    // Enough for now
}

void GBA_HandleInput(int a, int b, int l, int r, int st, int se,
                     int dr, int dl, int du, int dd)
{
    u16 input = 0;

    if (a == 0)
        input |= BIT(0);
    if (b == 0)
        input |= BIT(1);
    if (se == 0)
        input |= BIT(2);
    if (st == 0)
        input |= BIT(3);
    if (dr == 0)
        input |= BIT(4);
    if (dl == 0)
        input |= BIT(5);
    if (du == 0)
        input |= BIT(6);
    if (dd == 0)
        input |= BIT(7);
    if (r == 0)
        input |= BIT(8);
    if (l == 0)
        input |= BIT(9);

    REG_KEYINPUT = input;
}

void GBA_HandleInputFlags(u16 flags)
{
    REG_KEYINPUT = 0x3FF & ~flags;
}

void GBA_Screenshot(const char *path)
{
    unsigned char *buffer = malloc(240 * 160 * 3);
    if (buffer == NULL)
    {
        Debug_ErrorMsgArg("%s(): Not enough memory.");
        return;
    }

    GBA_ConvertScreenBufferTo24RGB(buffer);

    if (path)
    {
        Save_PNG(path, buffer, 240, 160, 0);
    }
    else
    {
        char *name = FU_GetNewTimestampFilename("gba_screenshot");
        Save_PNG(name, buffer, 240, 160, 0);
    }

    free(buffer);
}

static s32 min_(s32 a, s32 b)
{
    return ((a < b) ? a : b);
}

int gba_execution_break = 0;

void GBA_RunFor_ExecutionBreak(void)
{
    gba_execution_break = 1;
}

void GBA_RunForOneFrame(void)
{
    GBA_CheckKeypadInterrupt();
    GBA_RunFor(280896); // Clocksperframe = 280896
}

u32 GBA_RunFor(s32 totalclocks)
{
    s32 residualclocks, executedclocks;
    totalclocks += lastresidualclocks;
    u32 has_executed = 0;

    while (totalclocks >= clocks_to_next_event)
    {
        if (GBA_DMAisWorking())
        {
            executedclocks = GBA_DMAGetExtraClocksElapsed()
                             + clocks_to_next_event;
        }
        else
        {
            if (GBA_InterruptCheck())
            {
                // 2S + 1N cycles ?
                executedclocks = GBA_MemoryGetAccessCycles(0, 1, CPU.OldPC)
                                 + GBA_MemoryGetAccessCyclesNoSeq(1, CPU.R[R_PC])
                                 + GBA_MemoryGetAccessCyclesSeq(1, CPU.R[R_PC]);
            }
            else
            {
                residualclocks = GBA_Execute(clocks_to_next_event);
                executedclocks = clocks_to_next_event - residualclocks;
            }

            has_executed = executedclocks && !GBA_CPUGetHalted();
        }

        clocks_to_next_event = GBA_UpdateScreenTimings(executedclocks);
        int tmp = GBA_DMAUpdate(executedclocks);
        clocks_to_next_event = min_(tmp, clocks_to_next_event);
        tmp = GBA_TimersUpdate(executedclocks);
        clocks_to_next_event = min_(tmp, clocks_to_next_event);
        tmp = GBA_SoundUpdate(executedclocks);
        clocks_to_next_event = min_(tmp, clocks_to_next_event);
        // Check if any other event is going to happen before

        totalclocks -= executedclocks;

        if (gba_execution_break)
        {
            lastresidualclocks = totalclocks;
            gba_execution_break = 0;
            return has_executed;
        }
    }

    while (totalclocks > 0)
    {
        if (GBA_DMAisWorking())
        {
            executedclocks = GBA_DMAGetExtraClocksElapsed()
                             + clocks_to_next_event;
        }
        else
        {
            if (GBA_InterruptCheck())
            {
                // 2S + 1N cycles ?
                executedclocks = GBA_MemoryGetAccessCycles(0, 1, CPU.OldPC)
                                 + GBA_MemoryGetAccessCyclesNoSeq(1, CPU.R[R_PC])
                                 + GBA_MemoryGetAccessCyclesSeq(1, CPU.R[R_PC]);
            }
            else
            {
                residualclocks = GBA_Execute(totalclocks);
                executedclocks = totalclocks - residualclocks;
            }

            has_executed = executedclocks && !GBA_CPUGetHalted();
        }

        clocks_to_next_event = GBA_UpdateScreenTimings(executedclocks);
        clocks_to_next_event = min_(GBA_DMAUpdate(executedclocks),
                                   clocks_to_next_event);
        clocks_to_next_event = min_(GBA_TimersUpdate(executedclocks),
                                   clocks_to_next_event);
        clocks_to_next_event = min_(GBA_SoundUpdate(executedclocks),
                                   clocks_to_next_event);
        // Check if other events are going to happen earlier

        totalclocks -= executedclocks;

        if (gba_execution_break)
        {
            lastresidualclocks = totalclocks;
            gba_execution_break = 0;
            return has_executed;
        }
    }

    lastresidualclocks = totalclocks;

    return has_executed;
}

void GBA_DebugStep(void)
{
    // Hack to make it always execute ONLY one instruction
    int saved_lastresidualclocks = lastresidualclocks;
    lastresidualclocks = 0;

    //Debug_DebugMsgArg("lastresidualclocks = %d", lastresidualclocks);
    int count = 280896; // Clocksperframe = 280896

    while (!GBA_RunFor(1))
    {
        count--;
        if (count == 0)
            break;
    }

    lastresidualclocks += saved_lastresidualclocks;
}
