// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../debug_utils.h"
#include "../general_utils.h"

#include "cpu.h"
#include "debug.h"
#include "dma.h"
#include "gameboy.h"
#include "gb_main.h"
#include "general.h"
#include "interrupts.h"
#include "memory_dmg.h"
#include "memory_gbc.h"
#include "memory.h"
#include "ppu.h"
#include "serial.h"
#include "sgb.h"
#include "sound.h"
#include "video.h"

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//----------------------------------------------------------------

void GB_MemUpdateReadWriteFunctionPointers(void)
{
    if (GameBoy.Emulator.enable_boot_rom)
    {
        switch (GameBoy.Emulator.HardwareType)
        {
            case HW_SGB:  // Unknown. Can't test.
            case HW_SGB2: // Unknown. Can't test.
            case HW_GB:
            case HW_GBP:
                GameBoy.Memory.MemRead = GB_MemRead8_DMG_BootEnabled;
                GameBoy.Memory.MemWrite = GB_MemWrite8_DMG;
                GameBoy.Memory.MemReadReg = GB_MemReadReg8_DMG;
                GameBoy.Memory.MemWriteReg = GB_MemWriteReg8_DMG;
                break;

            default:
                Debug_ErrorMsg("GB_MemUpdateReadWriteFunctionPointers():\n"
                               "Unknown hardware");
                // fallthrough
            case HW_GBC:
            case HW_GBA:
            case HW_GBA_SP:
                GameBoy.Memory.MemRead = GB_MemRead8_GBC_BootEnabled;
                GameBoy.Memory.MemWrite = GB_MemWrite8_GBC;
                GameBoy.Memory.MemReadReg = GB_MemReadReg8_GBC;
                GameBoy.Memory.MemWriteReg = GB_MemWriteReg8_GBC;
                break;
        }
    }
    else
    {
        switch (GameBoy.Emulator.HardwareType)
        {
            case HW_SGB:  // Unknown. Can't test.
            case HW_SGB2: // Unknown. Can't test.
            case HW_GB:
            case HW_GBP:
                GameBoy.Memory.MemRead = GB_MemRead8_DMG_BootDisabled;
                GameBoy.Memory.MemWrite = GB_MemWrite8_DMG;
                GameBoy.Memory.MemReadReg = GB_MemReadReg8_DMG;
                GameBoy.Memory.MemWriteReg = GB_MemWriteReg8_DMG;
                break;

            default:
                Debug_ErrorMsg("GB_MemUpdateReadWriteFunctionPointers():\n"
                               "Unknown hardware");
                // fallthrough
            case HW_GBC:
            case HW_GBA:
            case HW_GBA_SP:
                GameBoy.Memory.MemRead = GB_MemRead8_GBC_BootDisabled;
                GameBoy.Memory.MemWrite = GB_MemWrite8_GBC;
                GameBoy.Memory.MemReadReg = GB_MemReadReg8_GBC;
                GameBoy.Memory.MemWriteReg = GB_MemWriteReg8_GBC;
                break;
        }
    }
}

void GB_MemInit(void)
{
    GB_MemUpdateReadWriteFunctionPointers();

    // Setup memory areas
    //-------------------

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    mem->ROM_Base = (void *)GameBoy.Emulator.Rom_Pointer;

    for (int i = 0; i < 512; i++)
        mem->ROM_Switch[i] = NULL;

    for (int i = 0; i < GameBoy.Emulator.ROM_Banks; i++)
        mem->ROM_Switch[i] = (u8 *)GameBoy.Emulator.Rom_Pointer
                           + (16 * 1024 * i);

    memset(mem->VideoRAM, 0, 0x4000);

    // Don't clear cartridge RAM here. Do it when loading the sav file if not
    // found!
    //for (int i = 0; i < GameBoy.Emulator.RAM_Banks; i++)
    //{
    //    memset_rand(mem->ExternRAM[i], 8 * 1024);
    //}

    memset_rand(mem->WorkRAM, 0x1000);

    if (GameBoy.Emulator.CGBEnabled == 1)
    {
        for (int i = 0; i < 7; i++)
        {
            memset_rand(mem->WorkRAM_Switch[i], 0x1000);
        }
        memset_rand(mem->StrangeRAM, 0x30);
    }
    else
    {
        memset_rand(mem->WorkRAM_Switch[0], 0x1000);
    }

    if ((GameBoy.Emulator.HardwareType == HW_GBC)
        || (GameBoy.Emulator.HardwareType == HW_GBA)
        || (GameBoy.Emulator.HardwareType == HW_GBA_SP))
    {
        memset(mem->ObjAttrMem, 0, 0xA0);
    }
    else
    {
        memset_rand(mem->ObjAttrMem, 0xA0);
    }

    memset(mem->IO_Ports, 0x00, 0x80);
    memset(mem->HighRAM, 0, 0x80);

    mem->selected_rom = 1;
    mem->selected_ram = 0;
    mem->selected_wram = 0;
    mem->selected_vram = 0;
    mem->mbc_mode = 0;

    mem->VideoRAM_Curr = mem->VideoRAM;
    mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
    mem->RAM_Curr = mem->ExternRAM[0];
    mem->WorkRAM_Curr = mem->WorkRAM_Switch[0];

    // Prepare registers
    // -----------------

    // Interrupt/timer registers are inited in GB_InterruptsInit()
    // Sound registers are inited in GB_SoundInit()
    // PPU registers in GB_PPUInit()

    GameBoy.Memory.RAMEnabled = 0; // MBC

    GB_MemWriteReg8(0xFF72, 0x00);
    GB_MemWriteReg8(0xFF73, 0x00);

    GB_MemWriteReg8(0xFF6C, 0xFE);

    GB_MemWriteReg8(0xFF75, 0x8F);

    GB_MemWriteReg8(0xFF76, 0x00);
    GB_MemWriteReg8(0xFF77, 0x00);

    if (GameBoy.Emulator.CGBEnabled == 1)
    {
        GB_MemWriteReg8(0xFF74, 0x00);

        GB_MemWriteReg8(SVBK_REG, 0xF8);
        GB_MemWriteReg8(VBK_REG, 0xFE);

        // PALETTE RAM
        for (int i = 0; i < 64; i++)
        {
            GameBoy.Emulator.bg_pal[i] = 0xFF;
            GameBoy.Emulator.spr_pal[i] = rand() & 0xFF;
        }
    }
}

void GB_MemEnd(void)
{
    //Nothing to do
}

//----------------------------------------------------------------

void GB_MemWrite16(u32 address, u32 value)
{
    GB_MemWrite8(address++, value & 0xFF);
    GB_MemWrite8(address & 0xFFFF, value >> 8);
}

void GB_MemWrite8(u32 address, u32 value)
{
    GameBoy.Memory.MemWrite(address, value);
}

void GB_MemWriteReg8(u32 address, u32 value)
{
    GameBoy.Memory.MemWriteReg(address, value);
}

//----------------------------------------------------------------

u32 GB_MemRead16(u32 address)
{
    return GB_MemRead8(address) | (GB_MemRead8((address + 1) & 0xFFFF) << 8);
}

u32 GB_MemRead8(u32 address)
{
    return GameBoy.Memory.MemRead(address);
}

u32 GB_MemReadReg8(u32 address)
{
    return GameBoy.Memory.MemReadReg(address);
}

//----------------------------------------------------------------

// This assumes that address is in 0xFE00-0xFEA0
void GB_MemWriteDMA8(u32 address, u32 value)
{
#ifdef VRAM_MEM_CHECKING
    if (GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 2 or 3)
        return;
#endif
    GameBoy.Memory.ObjAttrMem[address - 0xFE00] = value;
}

u32 GB_MemReadDMA8(u32 address) // Not verified yet - different for GBC and DMG
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3: // 16KB ROM Bank 00
            return mem->ROM_Base[address];
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7: // 16KB ROM Bank 01..NN
            return mem->ROM_Curr[address - 0x4000];
        case 0x8:
        case 0x9: // 8KB Video RAM (VRAM)
#ifdef VRAM_MEM_CHECKING
            if (GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 3)
                return 0xFF;
#endif
            return 0xFF; // ??
        case 0xA:
        case 0xB: // 8KB External RAM
            return GameBoy.Memory.MapperRead(address);
        case 0xC: // 4KB Work RAM Bank 0
            return mem->WorkRAM[address - 0xC000];
        case 0xD: // 4KB Work RAM Bank 1
            return mem->WorkRAM_Curr[address - 0xD000];
        case 0xE: // Same as C000-DDFF
        case 0xF:
            return GameBoy.Memory.MapperRead(address + 0xA000 - 0xE000);
        default:
            Debug_ErrorMsgArg("GB_MemReadDMA8() Read address %04X\n"
                              "PC: %04X\n"
                              "ROM: %d", address, GameBoy.CPU.R16.PC,
                              GameBoy.Memory.selected_rom);
            return GB_MemReadReg8(address & 0xFFFF);
    }

    return 0x00;
}

//----------------------------------------------------------------

void GB_MemWriteHDMA8(u32 address, u32 value)
{
    if (GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 3)
        return;

    GameBoy.Memory.VideoRAM_Curr[address & 0x1FFF] = value;

#if 0
    if ((address & 0xE000) == 0x8000) // 8000h or 9000h - Video RAM (VRAM)
    {
#ifdef VRAM_MEM_CHECKING
        if (GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 3)
            return;
#endif
        GameBoy.Memory.VideoRAM_Curr[address - 0x8000] = value;
        return;
    }
    else
    {
        Debug_ErrorMsgArg("GB_MemWriteHDMA8(): Wrote address %04x\n"
                          "PC: %04x\n"
                          "ROM: %d", address, GameBoy.CPU.R16.PC,
                          GameBoy.Memory.selected_rom);
        return;
    }
#endif
}

u32 GB_MemReadHDMA8(u32 address)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3: // 16KB ROM Bank 00
            return mem->ROM_Base[address];
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7: // 16KB ROM Bank 01..NN
            return mem->ROM_Curr[address - 0x4000];
        case 0x8:
        case 0x9: // 8KB Video RAM (VRAM)
#ifdef VRAM_MEM_CHECKING
            if (GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 3)
                return 0xFF;
#endif
            return 0xFF; // TODO: This is not correct.
        case 0xA:
        case 0xB: // 8KB External RAM
            return GameBoy.Memory.MapperRead(address);
        case 0xC: // 4KB Work RAM Bank 0
            return mem->WorkRAM[address - 0xC000];
        case 0xD: // 4KB Work RAM Bank 1
            return mem->WorkRAM_Curr[address - 0xD000];
        case 0xE: // Same as C000-DDFF
        case 0xF:
            return GameBoy.Memory.MapperRead(address + 0xA000 - 0xE000);
        default:
            Debug_ErrorMsgArg("GB_MemReadHDMA8() Read address %04X\n"
                              "PC: %04X\n"
                              "ROM: %d", address, GameBoy.CPU.R16.PC,
                              GameBoy.Memory.selected_rom);
            return GB_MemReadReg8(address & 0xFFFF);
    }

    return 0x00;
}

//----------------------------------------------------------------

void GB_MemoryWriteSVBK(int value) // reference_clocks not needed
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    mem->IO_Ports[SVBK_REG - 0xFF00] = value | (0xF8);

    value &= 7;
    if (value == 0)
        value = 1;

    mem->selected_wram = value - 1;
    mem->WorkRAM_Curr = mem->WorkRAM_Switch[mem->selected_wram];
}

void GB_MemoryWriteVBK(int value) // reference_clocks not needed
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    mem->IO_Ports[VBK_REG - 0xFF00] = value | (0xFE);

    mem->selected_vram = value & 1;

    if (mem->selected_vram)
        mem->VideoRAM_Curr = &mem->VideoRAM[0x2000];
    else
        mem->VideoRAM_Curr = &mem->VideoRAM[0x0000];
}
