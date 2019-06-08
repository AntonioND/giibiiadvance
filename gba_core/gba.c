/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>

#include "../build_options.h"
#include "../debug_utils.h"
#include "../file_utils.h"

#include "gba.h"
#include "cpu.h"
#include "bios.h"
#include "memory.h"
#include "interrupts.h"
#include "video.h"
#include "dma.h"
#include "timers.h"
#include "save.h"
#include "rom.h"
#include "sound.h"
#include "../png/png_utils.h"

static s32 clocks_to_next_event;
static s32 lastresidualclocks = 0;

static int inited = 0;

int GBA_ROM_SIZE;
int GBA_GetRomSize(void)
{
    return GBA_ROM_SIZE;
}

_cpu_t * GBA_CPUGet(void)
{
    return &CPU;
}

int GBA_InitRom(void * bios_ptr, void * rom_ptr, u32 romsize)
{
    if(inited) GBA_EndRom(1); //shouldn't be needed here

    if(romsize > 0x02000000)
    {
        Debug_ErrorMsgArg("Rom too big!\nSize = 0x%08X bytes\nMax = 0x02000000 bytes",romsize);
        GBA_ROM_SIZE = 0x02000000;
    }
    else
    {
        GBA_ROM_SIZE = romsize;
    }

    GBA_DetectSaveType(rom_ptr,romsize);
    GBA_ResetSaveBuffer();
    GBA_SaveReadFile();

    GBA_HeaderCheck(rom_ptr);

    GBA_CPUInit();
    GBA_InterruptInit();
    GBA_TimerInitAll();
    GBA_MemoryInit(bios_ptr,rom_ptr,GBA_ROM_SIZE);
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

    //GBA_DebugAddBreakpoint(0x1954); // BIOS switch to GBC mode - Never used in real hardware

    return 1;
}

int GBA_EndRom(int save)
{
    if(inited == 0) return 0;

    if(save) GBA_SaveWriteFile();

    GBA_MemoryEnd();

    inited = 0;

    return 1;
}

void GBA_Reset(void)
{
    GBA_CPUClearHalted();
    GBA_Swi(0x26);
    //it will be enough for now
}

void GBA_HandleInput(int a, int b, int l, int r, int st, int se, int dr, int dl, int du, int dd)
{
    u16 input = 0;
    if(a == 0) input |= BIT(0);
    if(b == 0) input |= BIT(1);
    if(se == 0) input |= BIT(2);
    if(st == 0) input |= BIT(3);
    if(dr == 0) input |= BIT(4);
    if(dl == 0) input |= BIT(5);
    if(du == 0) input |= BIT(6);
    if(dd == 0) input |= BIT(7);
    if(r == 0) input |= BIT(8);
    if(l == 0) input |= BIT(9);
    REG_KEYINPUT = input;
}

void GBA_Screenshot(void)
{
    char * name = FU_GetNewTimestampFilename("gba_screenshot");
    u32 * buffer = malloc(240*160*4);
    GBA_ConvertScreenBufferTo32RGB(buffer);
    Save_PNG(name,240,160,buffer,0);
    free(buffer);
}

static s32 min(s32 a, s32 b)
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
    GBA_RunFor(280896); //clocksperframe = 280896
}

u32 GBA_RunFor(s32 totalclocks)
{
    s32 residualclocks, executedclocks;
    totalclocks += lastresidualclocks;
    u32 has_executed = 0;

    while(totalclocks >= clocks_to_next_event)
    {
        if(GBA_DMAisWorking())
        {
            executedclocks = GBA_DMAGetExtraClocksElapsed() + clocks_to_next_event;
        }
        else
        {
            if(GBA_InterruptCheck())
            {
                executedclocks = GBA_MemoryGetAccessCycles(0,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq(1,CPU.R[R_PC]) +
                        GBA_MemoryGetAccessCyclesSeq(1,CPU.R[R_PC]); //2S + 1N cycles ?
            }
            else
            {
                residualclocks = GBA_Execute(clocks_to_next_event);
                executedclocks = clocks_to_next_event-residualclocks;
            }

            has_executed = executedclocks && !GBA_CPUGetHalted();
        }

        clocks_to_next_event = GBA_UpdateScreenTimings(executedclocks);
        int tmp = GBA_DMAUpdate(executedclocks);
        clocks_to_next_event = min(tmp,clocks_to_next_event);
        tmp = GBA_TimersUpdate(executedclocks);
        clocks_to_next_event = min(tmp,clocks_to_next_event);
        tmp = GBA_SoundUpdate(executedclocks);
        clocks_to_next_event = min(tmp,clocks_to_next_event);
        //check if any other event is going to happen before

        totalclocks -= executedclocks;

        if(gba_execution_break)
        {
            lastresidualclocks = totalclocks;
            gba_execution_break = 0;
            return has_executed;
        }
    }

    while(totalclocks > 0)
    {
        if(GBA_DMAisWorking())
        {
            executedclocks = GBA_DMAGetExtraClocksElapsed() + clocks_to_next_event;
        }
        else
        {
            if(GBA_InterruptCheck())
            {
                executedclocks = GBA_MemoryGetAccessCycles(0,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq(1,CPU.R[R_PC]) +
                        GBA_MemoryGetAccessCyclesSeq(1,CPU.R[R_PC]); //2S + 1N cycles ?
            }
            else
            {
                residualclocks = GBA_Execute(totalclocks);
                executedclocks = totalclocks-residualclocks;
            }

            has_executed = executedclocks && !GBA_CPUGetHalted();
        }

        clocks_to_next_event = GBA_UpdateScreenTimings(executedclocks);
        clocks_to_next_event = min(GBA_DMAUpdate(executedclocks),clocks_to_next_event);
        clocks_to_next_event = min(GBA_TimersUpdate(executedclocks),clocks_to_next_event);
        clocks_to_next_event = min(GBA_SoundUpdate(executedclocks),clocks_to_next_event);
        //mirar si otros eventos van a suceder antes y cambiar clocks por eso

        totalclocks -= executedclocks;

        if(gba_execution_break)
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
    int saved_lastresidualclocks = lastresidualclocks; // hack to make it always execute ONLY one instruction
    lastresidualclocks = 0;

    //Debug_DebugMsgArg("lastresidualclocks = %d",lastresidualclocks);
    int count = 280896; //clocksperframe = 280896
    while(!GBA_RunFor(1))
    {
        count--;
        if(count == 0) break;
    }

    lastresidualclocks += saved_lastresidualclocks;
    //GLWindow_MemViewerUpdate();
    //GLWindow_IOViewerUpdate();
    //GLWindow_DisassemblerUpdate();
}

