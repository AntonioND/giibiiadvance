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

#include <stdio.h>
#include <stdlib.h>

#include "../build_options.h"
#include "../general_utils.h"
#include "../debug_utils.h"

#include "gameboy.h"
#include "cpu.h"
#include "debug.h"
#include "memory.h"
#include "interrupts.h"
#include "general.h"
#include "sound.h"
#include "ppu.h"
#include "sgb.h"
#include "video.h"
#include "gb_main.h"
#include "serial.h"
#include "dma.h"
#include "memory_dmg.h"
#include "memory_gbc.h"

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//----------------------------------------------------------------

void GB_MemUpdateReadWriteFunctionPointers(void)
{
    if(GameBoy.Emulator.enable_boot_rom)
    {
        switch(GameBoy.Emulator.HardwareType)
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
                Debug_ErrorMsg("GB_MemUpdateReadWriteFunctionPointers():\nUnknown hardware");
                //fall to GBC
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
        switch(GameBoy.Emulator.HardwareType)
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
                Debug_ErrorMsg("GB_MemUpdateReadWriteFunctionPointers():\nUnknown hardware");
                //fall to GBC
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
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    GB_MemUpdateReadWriteFunctionPointers();

    //LOAD INITIAL VALUES TO RAM

    //Interrupt/timer registers are inited in GB_InterruptsInit()

    //Sound registers are inited in GB_SoundInit()

    //PPU registers in GB_PPUInit()

    GameBoy.Memory.RAMEnabled = 0; // MBC

    GB_MemWriteReg8(0xFF72,0x00);
    GB_MemWriteReg8(0xFF73,0x00);

    GB_MemWriteReg8(0xFF6C,0xFE);

    GB_MemWriteReg8(0xFF75,0x8F);

    GB_MemWriteReg8(0xFF76,0x00);
    GB_MemWriteReg8(0xFF77,0x00);

    if(GameBoy.Emulator.CGBEnabled == 1)
    {
        GB_MemWriteReg8(0xFF74,0x00);

        GB_MemWriteReg8(SVBK_REG,0xF8);
        GB_MemWriteReg8(VBK_REG,0xFE);

        //GB_MemWriteReg8(KEY1_REG,0x7E);
        mem->IO_Ports[KEY1_REG-0xFF00] = 0x7E;

        mem->IO_Ports[HDMA5_REG-0xFF00] = 0xFF;

        //PALETTE RAM
        u32 i;
        for(i= 0; i < 64; i++)
        {
            GameBoy.Emulator.bg_pal[i] = 0xFF;
            GameBoy.Emulator.spr_pal[i] = rand() & 0xFF;
        }
    }

    //All those writes will generate a GB_CPUBreakLoop(), but it doesn't matter.
}

void GB_MemEnd(void)
{
    //Nothing to do
}

//----------------------------------------------------------------

inline void GB_MemWrite16(u32 address, u32 value)
{
    GB_MemWrite8(address++,value&0xFF);
    GB_MemWrite8(address&0xFFFF,value>>8);
}

inline void GB_MemWrite8(u32 address, u32 value)
{
    GameBoy.Memory.MemWrite(address,value);
}

inline void GB_MemWriteReg8(u32 address, u32 value)
{
    GameBoy.Memory.MemWriteReg(address,value);
}

//----------------------------------------------------------------

inline u32 GB_MemRead16(u32 address)
{
    return ( GB_MemRead8(address) | (GB_MemRead8((address+1)&0xFFFF) << 8) );
}

inline u32 GB_MemRead8(u32 address)
{
    return GameBoy.Memory.MemRead(address);
}

inline u32 GB_MemReadReg8(u32 address)
{
    return GameBoy.Memory.MemReadReg(address);
}

//----------------------------------------------------------------

void GB_MemWriteDMA8(u32 address, u32 value) // This assumes that address is 0xFE00-0xFEA0
{
#ifdef VRAM_MEM_CHECKING
    if(GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 2 or 3) return;
#endif
    GameBoy.Memory.ObjAttrMem[address-0xFE00] = value;
}

u32 GB_MemReadDMA8(u32 address) // NOT VERIFIED YET
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3: //16KB ROM Bank 00
            return mem->ROM_Base[address];
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7: //16KB ROM Bank 01..NN
            return mem->ROM_Curr[address-0x4000];
        case 0x8:
        case 0x9: //8KB Video RAM (VRAM)
#ifdef VRAM_MEM_CHECKING
            if(GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 3) return 0xFF;
#endif
            return 0xFF; // ??
        case 0xA:
        case 0xB: //8KB External RAM
            return GameBoy.Memory.MapperRead(address);
        case 0xC: //4KB Work RAM Bank 0
            return mem->WorkRAM[address-0xC000];
        case 0xD: // 4KB Work RAM Bank 1
            return mem->WorkRAM_Curr[address-0xD000];
        case 0xE: // Same as C000-DDFF
        case 0xF:
            return GameBoy.Memory.MapperRead(address+0xA000-0xE000);
        default:
            Debug_ErrorMsgArg("GB_MemReadDMA8() Read address %04X\nPC: %04X\nROM: %d",
                        address,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            return GB_MemReadReg8(address&0xFFFF);
    }

    return 0x00;
}

//----------------------------------------------------------------

void GB_MemWriteHDMA8(u32 address, u32 value)
{
    if( (address & 0xE000) == 0x8000 ) //8000h or 9000h - Video RAM (VRAM)
    {
#ifdef VRAM_MEM_CHECKING
        if(GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 3) return;
#endif
        GameBoy.Memory.VideoRAM_Curr[address-0x8000] = value;
        return;
    }
    else
    {
        Debug_ErrorMsgArg("GB_MemWriteHDMA8(): Wrote address %04x\nPC: %04x\nROM: %d",
                    address,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
        return;
    }
}

u32 GB_MemReadHDMA8(u32 address)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3: //16KB ROM Bank 00
            return mem->ROM_Base[address];
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7: //16KB ROM Bank 01..NN
            return mem->ROM_Curr[address-0x4000];
        case 0x8:
        case 0x9: //8KB Video RAM (VRAM)
#ifdef VRAM_MEM_CHECKING
            if(GameBoy.Emulator.lcd_on && GameBoy.Emulator.ScreenMode == 3) return 0xFF;
#endif
            return 0xFF; // TODO: This is not completely correct.
        case 0xA:
        case 0xB: //8KB External RAM
            return GameBoy.Memory.MapperRead(address);
        case 0xC: //4KB Work RAM Bank 0
            return mem->WorkRAM[address-0xC000];
        case 0xD: // 4KB Work RAM Bank 1
            return mem->WorkRAM_Curr[address-0xD000];
        case 0xE: // Same as C000-DDFF
        case 0xF:
            return GameBoy.Memory.MapperRead(address+0xA000-0xE000);
        default:
            Debug_ErrorMsgArg("GB_MemReadHDMA8() Read address %04X\nPC: %04X\nROM: %d",
                        address,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            return GB_MemReadReg8(address&0xFFFF);
    }

    return 0x00;
}

//----------------------------------------------------------------
