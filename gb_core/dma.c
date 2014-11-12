
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

}

void GB_DMAEnd(void)
{

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
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int increment_clocks = reference_clocks - GB_DMAClockCounterGet();



    GB_DMAClockCounterSet(reference_clocks);
}

int GB_DMAGetClocksToNextEvent(void)
{
    int clocks_to_next_event = 256;



    return clocks_to_next_event;
}

//----------------------------------------------------------------

int GB_DMAExecute(void)
{
    _GB_CPU_ * cpu = &GameBoy.CPU;
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int executed_clocks = 0;

    //GB_CPUClockCounterAdd()

    return executed_clocks;
}

//----------------------------------------------------------------
