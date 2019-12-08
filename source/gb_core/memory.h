// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_MEMORY__
#define GB_MEMORY__

void GB_MemInit(void);
void GB_MemEnd(void);

void GB_MemUpdateReadWriteFunctionPointers(void);

void GB_MemWrite16(u32 address, u32 value); // Only used by debugger
void GB_MemWrite8(u32 address, u32 value);
void GB_MemWriteReg8(u32 address, u32 value);

u32 GB_MemRead16(u32 address); // Only used by debugger
u32 GB_MemRead8(u32 address);
u32 GB_MemReadReg8(u32 address);

// This assumes that address is 0xFE00-0xFEA0
void GB_MemWriteDMA8(u32 address, u32 value);
u32 GB_MemReadDMA8(u32 address);

void GB_MemWriteHDMA8(u32 address, u32 value);
u32 GB_MemReadHDMA8(u32 address);

void GB_MemoryWriteSVBK(int value); // reference_clocks not needed
void GB_MemoryWriteVBK(int value);  // reference_clocks not needed

#endif // GB_MEMORY__
