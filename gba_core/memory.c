// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../debug_utils.h"

#include "gba.h"
#include "memory.h"
#include "video.h"
#include "dma.h"
#include "cpu.h"
#include "shifts.h"
#include "interrupts.h"
#include "timers.h"
#include "save.h"
#include "sound.h"
#include "bios.h"

_mem_t Mem;

//----------------------------------------------------------------------------------------

static u32 * memarray[16];
static u32 memsizemask[16] = {
    0x3FFF, 0, 0x3FFFF, 0x7FFF,
    0x3FF, 0x3FF, 0x1FFFF, 0x3FF, //vram is 18000h... :S
    0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
    0x00FFFFFF, 0x00FFFFFF, 0, 0
};

u32 GBA_MemoryReadFast32(u32 address)
{
    if(address & 0xF0000000) return 0;
    u32 index = (address>>24)&0xF;
    u32 * ptr = memarray[index];
    if(ptr == NULL) return 0;
    else return ptr[(address&memsizemask[index])>>2];
}

void GBA_MemoryWriteFast32(u32 address,u32 data)
{
    if(address & 0xF0000000) return;
    u32 index = (address>>24)&0xF;
    u32 * ptr = memarray[index];
    if(ptr == NULL) return;
    else ptr[(address&memsizemask[index])>>2] = data;
}

u16 GBA_MemoryReadFast16(u32 address)
{
    if(address & 0xF0000000) return 0;
    u32 index = (address>>24)&0xF;
    u16 * ptr = (u16*)memarray[index];
    if(ptr == NULL) return 0;
    else return ptr[(address&memsizemask[index])>>1];
}

void GBA_MemoryWriteFast16(u32 address,u16 data)
{
    if(address & 0xF0000000) return;
    u32 index = (address>>24)&0xF;
    u16 * ptr = (u16*)memarray[index];
    if(ptr == NULL) return;
    else ptr[(address&memsizemask[index])>>1] = data;
}

u8 GBA_MemoryReadFast8(u32 address)
{
    if(address & 0xF0000000) return 0;
    u32 index = (address>>24)&0xF;
    u8 * ptr = (u8*)memarray[index];
    if(ptr == NULL) return 0;
    else return ptr[address&memsizemask[index]];
}

void GBA_MemoryWriteFast8(u32 address,u8 data)
{
    if(address & 0xF0000000) return;
    u32 index = (address>>24)&0xF;
    u8 * ptr = (u8*)memarray[index];
    if(ptr == NULL) return;
    else ptr[address&memsizemask[index]] = data;
}

//----------------------------------------------------------------------------------------

static void GBA_MemoryReadFastFillArray(void)
{
    memarray[0] = (u32*)Mem.rom_bios;
    memarray[1] = NULL;
    memarray[2] = (u32*)Mem.ewram;
    memarray[3] = (u32*)Mem.iwram;
    memarray[4] = (u32*)Mem.io_regs;
    memarray[5] = (u32*)Mem.pal_ram;
    memarray[6] = (u32*)Mem.vram;
    memarray[7] = (u32*)Mem.oam;
    memarray[8] = (u32*)Mem.rom_wait0;
    memarray[9] = (u32*)((u32)Mem.rom_wait0 + 0x01000000);
    memarray[10] = (u32*)Mem.rom_wait1;
    memarray[11] = (u32*)((u32)Mem.rom_wait1 + 0x01000000);
    memarray[12] = (u32*)Mem.rom_wait2;
    memarray[13] = (u32*)((u32)Mem.rom_wait2 + 0x01000000);
    memarray[14] = NULL;
    memarray[15] = NULL;
}

//----------------------------------------------------------------------------------------

void GBA_MemoryInit(u32 * bios_ptr, u32 * rom_ptr, u32 romsize)
{
    Mem.rom_bios = (u8*)calloc(1,16*1024);
    if(bios_ptr) memcpy(Mem.rom_bios,bios_ptr,16*1024);
    else GBA_BiosEmulatedLoad();

    memset(Mem.ewram,0,sizeof(Mem.ewram));
    memset(Mem.iwram,0,sizeof(Mem.iwram));
    memset(Mem.io_regs,0,sizeof(Mem.io_regs));

    memset(Mem.pal_ram,0,sizeof(Mem.pal_ram));
    memset(Mem.vram,0,sizeof(Mem.vram));
    memset(Mem.oam,0,sizeof(Mem.oam));

    u8 * rom_buffer = calloc(1,0x02000000);// 32*1024*1024);
    memcpy(rom_buffer,rom_ptr,romsize);
    Mem.rom_wait0 = rom_buffer;
    Mem.rom_wait1 = rom_buffer;
    Mem.rom_wait2 = rom_buffer;

//    memset(Mem.sram,0,sizeof(Mem.sram));

    GBA_MemoryReadFastFillArray();

    //Init registers:
    //---------------
    GBA_RegisterWrite16(DISPCNT,0x0080);
    GBA_RegisterWrite16(GREENSWAP,0);
    GBA_RegisterWrite16(DISPSTAT,0);
    GBA_RegisterWrite16(VCOUNT,0);
    GBA_RegisterWrite16(BG0CNT,0);
    GBA_RegisterWrite16(BG1CNT,0);
    GBA_RegisterWrite16(BG2CNT,0);
    GBA_RegisterWrite16(BG3CNT,0);
    GBA_RegisterWrite16(BG0HOFS,0);
    GBA_RegisterWrite16(BG0VOFS,0);
    GBA_RegisterWrite16(BG1HOFS,0);
    GBA_RegisterWrite16(BG1VOFS,0);
    GBA_RegisterWrite16(BG2HOFS,0);
    GBA_RegisterWrite16(BG2VOFS,0);
    GBA_RegisterWrite16(BG3HOFS,0);
    GBA_RegisterWrite16(BG3VOFS,0);
    GBA_RegisterWrite16(BG2PA,1<<8);
    GBA_RegisterWrite16(BG2PB,0);
    GBA_RegisterWrite16(BG2PC,0);
    GBA_RegisterWrite16(BG2PD,1<<8);
    GBA_RegisterWrite16(BG2X_L,0);
    GBA_RegisterWrite16(BG2X_H,0);
    GBA_RegisterWrite16(BG2Y_L,0);
    GBA_RegisterWrite16(BG2Y_H,0);
    GBA_RegisterWrite16(BG3PA,1<<8);
    GBA_RegisterWrite16(BG3PB,0);
    GBA_RegisterWrite16(BG3PC,0);
    GBA_RegisterWrite16(BG3PD,1<<8);
    GBA_RegisterWrite16(BG3X_L,0);
    GBA_RegisterWrite16(BG3X_H,0);
    GBA_RegisterWrite16(BG3Y_L,0);
    GBA_RegisterWrite16(BG3Y_H,0);
    GBA_RegisterWrite16(WIN0H,0);
    GBA_RegisterWrite16(WIN1H,0);
    GBA_RegisterWrite16(WIN0V,0);
    GBA_RegisterWrite16(WIN1V,0);
    GBA_RegisterWrite16(WININ,0);
    GBA_RegisterWrite16(WINOUT,0);
    GBA_RegisterWrite16(MOSAIC,0);
    GBA_RegisterWrite16(BLDCNT,0);
    GBA_RegisterWrite16(BLDALPHA,0);
    GBA_RegisterWrite16(BLDY,0);

    GBA_RegisterWrite16(SOUND1CNT_L,0);
    GBA_RegisterWrite16(SOUND1CNT_H,0);
    GBA_RegisterWrite16(SOUND1CNT_X,0);
    GBA_RegisterWrite16(SOUND2CNT_L,0);
    GBA_RegisterWrite16(SOUND2CNT_H,0);
    GBA_RegisterWrite16(SOUND3CNT_L,0);
    GBA_RegisterWrite16(SOUND3CNT_H,0);
    GBA_RegisterWrite16(SOUND3CNT_X,0);
    GBA_RegisterWrite16(SOUND4CNT_L,0);
    GBA_RegisterWrite16(SOUND4CNT_H,0);
    GBA_RegisterWrite16(SOUNDCNT_L,0);
    GBA_RegisterWrite16(SOUNDCNT_H,0x880E);
    GBA_RegisterWrite16(SOUNDCNT_X,0);
    GBA_RegisterWrite16(SOUNDBIAS,0);
    GBA_RegisterWrite32(FIFO_A,0);
    GBA_RegisterWrite32(FIFO_B,0);

    GBA_RegisterWrite32(DMA0SAD,0);
    GBA_RegisterWrite32(DMA0DAD,0);
    GBA_RegisterWrite16(DMA0CNT_L,0);
    GBA_RegisterWrite16(DMA0CNT_H,0);
    GBA_RegisterWrite32(DMA1SAD,0);
    GBA_RegisterWrite32(DMA1DAD,0);
    GBA_RegisterWrite16(DMA1CNT_L,0);
    GBA_RegisterWrite16(DMA1CNT_H,0);
    GBA_RegisterWrite32(DMA2SAD,0);
    GBA_RegisterWrite32(DMA2DAD,0);
    GBA_RegisterWrite16(DMA2CNT_L,0);
    GBA_RegisterWrite16(DMA2CNT_H,0);
    GBA_RegisterWrite32(DMA3SAD,0);
    GBA_RegisterWrite32(DMA3DAD,0);
    GBA_RegisterWrite16(DMA3CNT_L,0);
    GBA_RegisterWrite16(DMA3CNT_H,0);

    GBA_RegisterWrite16(TM0CNT_L,0xFF6F);
    GBA_RegisterWrite16(TM0CNT_H,0);
    GBA_RegisterWrite16(TM1CNT_L,0);
    GBA_RegisterWrite16(TM1CNT_H,0);
    GBA_RegisterWrite16(TM2CNT_L,0);
    GBA_RegisterWrite16(TM2CNT_H,0);
    GBA_RegisterWrite16(TM3CNT_L,0);
    GBA_RegisterWrite16(TM3CNT_H,0);

    GBA_RegisterWrite16(KEYINPUT,0);
    GBA_RegisterWrite16(KEYCNT,0);

    GBA_RegisterWrite16(IE,0);
    GBA_RegisterWrite16(IF,0);
    GBA_RegisterWrite16(WAITCNT,0);
    GBA_RegisterWrite16(IME,0);

    //REG_WAITCNT = 0x4317;
    GBA_MemoryAccessCyclesUpdate();
    GBA_DMA0Setup();
    GBA_DMA1Setup();
    GBA_DMA2Setup();
    GBA_DMA3Setup();
}

void GBA_MemoryEnd(void)
{
    free(Mem.rom_bios);
    free(Mem.rom_wait0); //only free one
}

//----------------------------------------------------------------------------------------

u32 GBA_MemoryRead32(u32 address)
{
    register u32 data;

    switch(address>>24)
    {
        case 0:
            if(address < 0x00004000) if(CPU.R[R_PC] < 0x00004000)
                { data = *((u32*)&(Mem.rom_bios[address])); break; }
            return 0;
        case 1: return 0;
        case 2: data = *((u32*)&(Mem.ewram[address&0x3FFFC])); break;
        case 3: data = *((u32*)&(Mem.iwram[address&0x7FFC])); break;
        case 4: data = GBA_RegisterRead32(address&~3); break;
        case 5: data = *((u32*)&(Mem.pal_ram[address&0x3FC])); break;
        case 6:
            if(address < 0x06018000) { data = *((u32*)&(Mem.vram[address-0x06000000])); break; }
            return 0;
        case 7: data = *((u32*)&(Mem.oam[address&0x3FC])); break;
        case 8:
        case 9: //data = *((u32*)&(Mem.rom_wait0[address&0x01FFFFFC])); break;
        case 0xA:
        case 0xB: //data = *((u32*)&(Mem.rom_wait1[address&0x01FFFFFC])); break;
        case 0xC:
        case 0xD: data = *((u32*)&(Mem.rom_wait2[address&0x01FFFFFC])); break;
        case 0xE:
        case 0xF:
        default:
            return 0;
    }

#ifdef ENABLE_ASM_X86
    asm("and $3,%%eax    \n\t" // eax = address & 3
        "mov $3,%%cl     \n\t" // cl = 3
        "shl %%cl,%%eax  \n\t" // eax = (address & 3) << 3
        "mov %%eax,%%ecx \n\t" // ecx = shift = (address & 3) << 3
        "ror %%cl,%%ebx  \n\t" // ebx = [ data ]  ror [ shift ]
        : "=b" (data)
        : "a" (address), "b" (data)
        : "ecx" );
    return data;
#else
    u32 shift = (address&3)<<3;
    return ror_immed_no_carry(data,shift);
#endif
}

void GBA_MemoryWrite32(u32 address,u32 data)
{
    if(address < 0x02000000) return;
    if(address < 0x03000000) { *((u32*)&(Mem.ewram[address&0x3FFFC])) = data; return; }
    if(address < 0x04000000) { *((u32*)&(Mem.iwram[address&0x7FFC])) = data; return; }
    if(address < 0x05000000) { GBA_RegisterWrite32(address&~3,data); return; }
    if(address < 0x06000000) { *((u32*)&(Mem.pal_ram[address&0x3FC])) = data; return; }
    if(address < 0x06018000) { *((u32*)&(Mem.vram[(address&~3)-0x06000000])) = data; return; }
    if(address < 0x07000000) return;
    if(address < 0x08000000) { *((u32*)&(Mem.oam[address&0x3FC])) = data; return; }

    //if(address < 0x0E000000) return;
    //if(address < 0x0E010000) { *((u32*)&(Mem.sram[address-0x0E000000])) = data; return; } //only 8 bit

    return;
}


u16 GBA_MemoryRead16(u32 address)
{
    if(address < 0x00004000) if(CPU.R[R_PC] < 0x00004000) return *((u16*)&(Mem.rom_bios[address]));
    if(address < 0x02000000) return 0;
    if(address < 0x03000000) return *((u16*)&(Mem.ewram[address&0x3FFFE]));
    if(address < 0x04000000) return *((u16*)&(Mem.iwram[address&0x7FFE]));
    if(address < 0x05000000) return GBA_RegisterRead16(address&~1);

    if(address < 0x06000000) return *((u16*)&(Mem.pal_ram[address&0x3FE]));
    if(address < 0x06018000) return *((u16*)&(Mem.vram[address-0x06000000]));
    if(address < 0x07000000) return 0;
    if(address < 0x08000000) return *((u16*)&(Mem.oam[address&0x3FE]));

    if(GBA_SaveIsEEPROM())
    {
        if(GBA_GetRomSize() > (16*1024*1024))
        {
            if(address < 0x0DFFFF00) return *((u16*)&(Mem.rom_wait2[address&0x01FFFFFF]));
            return GBA_SaveRead16(address);
        }
        else
        {
            if(address < 0x0D000000) return *((u16*)&(Mem.rom_wait2[address&0x01FFFFFF]));
            return GBA_SaveRead16(address);
        }
    }
    else //No EEPROM
    {
        if(address < 0x0E000000) return *((u16*)&(Mem.rom_wait2[address&0x01FFFFFE]));
        //if(address < 0x0E010000) return *((u16*)&(Mem.sram[address-0x0E000000])); //only 8 bit
    }

    return 0;
}

void GBA_MemoryWrite16(u32 address,u16 data)
{
    if(address < 0x02000000) return;
    if(address < 0x03000000) { *((u16*)&(Mem.ewram[address&0x3FFFE])) = data; return; }
    if(address < 0x04000000) { *((u16*)&(Mem.iwram[address&0x7FFE])) = data; return; }
    if(address < 0x05000000) { GBA_RegisterWrite16(address&~1,data); return; }
    if(address < 0x06000000) { *((u16*)&(Mem.pal_ram[address&0x3FE])) = data; return; }
    if(address < 0x06018000) { *((u16*)&(Mem.vram[(address&~1)-0x06000000])) = data; return; }
    if(address < 0x07000000) return;
    if(address < 0x08000000) { *((u16*)&(Mem.oam[address&0x3FE])) = data; return; }

    if(GBA_SaveIsEEPROM())
    {
        if(address < 0x0D000000) return; //for EEPROM
        GBA_SaveWrite16(address,data);
    }
    //if(address < 0x0E000000) return;
    //if(address < 0x0E010000) { *((u16*)&(Mem.sram[address-0x0E000000])) = data; return; } //only 8 bit

    return;
}


u8 GBA_MemoryRead8(u32 address)
{
    if(address < 0x00004000) if(CPU.R[R_PC] < 0x00004000) return *((u8*)&(Mem.rom_bios[address]));
    if(address < 0x02000000) return 0;
    if(address < 0x03000000) return *((u8*)&(Mem.ewram[address&0x3FFFF]));
    if(address < 0x04000000) return *((u8*)&(Mem.iwram[address&0x7FFF]));
    if(address < 0x05000000) return GBA_RegisterRead8(address);
    if(address < 0x06000000) return *((u8*)&(Mem.pal_ram[address&0x3FF]));
    if(address < 0x06018000) return *((u8*)&(Mem.vram[address-0x06000000]));
    if(address < 0x07000000) return 0;
    if(address < 0x08000000) return *((u8*)&(Mem.oam[address&0x3FF]));

    if(GBA_SaveIsEEPROM())
    {
        if(GBA_GetRomSize() > (16*1024*1024))
        {
            if(address < 0x0DFFFF00) return *((u8*)&(Mem.rom_wait2[address&0x01FFFFFF]));
            return GBA_SaveRead8(address);
        }
        else
        {
            if(address < 0x0D000000) return *((u8*)&(Mem.rom_wait2[address&0x01FFFFFF]));
            return GBA_SaveRead8(address);
        }
    }
    else //No EEPROM
    {
        if(address < 0x0E000000) return *((u8*)&(Mem.rom_wait2[address&0x01FFFFFF]));
        return GBA_SaveRead8(address);
    }

    //if(address < 0x0E010000) return *((u8*)&(Mem.sram[address&0xFFFF])); //only 8 bit
    //return 0;
}

static u16 expand8to16(u8 data)
{
    return ((u16)data)|(((u16)data)<<8);
}

void GBA_MemoryWrite8(u32 address,u8 data)
{
    if(address < 0x02000000) return;
    if(address < 0x03000000) { *((u8*)&(Mem.ewram[address&0x3FFFF])) = data; return; }
    if(address < 0x04000000) { *((u8*)&(Mem.iwram[address&0x7FFF])) = data; return; }
    if(address < 0x05000000) { GBA_RegisterWrite8(address,data); return; }
    if(address < 0x06000000) { *((u16*)&(Mem.pal_ram[address&0x3FE])) = expand8to16(data); return; }
    if(address < 0x06018000) { *((u16*)&(Mem.vram[(address&~1)-0x06000000])) = expand8to16(data); return; }
    if(address < 0x07000000) return;
    if(address < 0x08000000) { *((u16*)&(Mem.oam[address&0x3FE])) = expand8to16(data); return; }

    if(GBA_SaveIsEEPROM())
    {
        if(address < 0x0D000000) return; //for EEPROM
        GBA_SaveWrite8(address,data);
    }
    else
    {
        if(address < 0x0E000000) return;
        GBA_SaveWrite8(address,data);
    }

    //if(address < 0x0E000000) return;
    //if(address < 0x10000000) { *((u8*)&(Mem.sram[address&0xFFFF])) = data; return; } //only 8 bit
    //return;
}

//--------------------------------------------------------------------------------------------

void GBA_RegisterWrite32(u32 address, u32 data)
{
    GBA_RegisterWrite16(address,(u16)data);
    GBA_RegisterWrite16(address+2,(u16)(data>>16));
}

u32 GBA_RegisterRead32(u32 address)
{
    return ((u32)GBA_RegisterRead16(address)) | (((u32)GBA_RegisterRead16(address+2))<<16);
}

void GBA_RegisterWrite16(u32 address, u16 data)
{
    if(address & 0x00FFFC00) return; //>= 4000400

    switch(address&0x3FF)
    {
        case DISPCNT-REG_BASE:
            REG_DISPCNT = data;
            GBA_UpdateDrawScanlineFn();
            GBA_ExecutionBreak();
            return;

        case DISPSTAT-REG_BASE:
            if((data>>8)==(REG_DISPSTAT>>8)) //same lyc than before
            {
                REG_DISPSTAT = (REG_DISPSTAT&0x0007)|(data&0xFFF8);
            }
            else
            {
                REG_DISPSTAT = (REG_DISPSTAT&0x0007)|(data&0xFFF8);
                if((REG_DISPSTAT>>8) == REG_VCOUNT) { REG_DISPSTAT |= BIT(2); GBA_InterruptLCD(BIT(5)); }
                else REG_DISPSTAT &= ~BIT(2);
            }

            GBA_ExecutionBreak();
            return;

        case VCOUNT-REG_BASE:
        case KEYINPUT-REG_BASE:
            return; //Read only

        case BG0HOFS-REG_BASE: REG_BG0HOFS = data&0x1FF; return;
        case BG0VOFS-REG_BASE: REG_BG0VOFS = data&0x1FF; return;
        case BG1HOFS-REG_BASE: REG_BG1HOFS = data&0x1FF; return;
        case BG1VOFS-REG_BASE: REG_BG1VOFS = data&0x1FF; return;
        case BG2HOFS-REG_BASE: REG_BG2HOFS = data&0x1FF; return;
        case BG2VOFS-REG_BASE: REG_BG2VOFS = data&0x1FF; return;
        case BG3HOFS-REG_BASE: REG_BG3HOFS = data&0x1FF; return;
        case BG3VOFS-REG_BASE: REG_BG3VOFS = data&0x1FF; return;

        case BG2X_L-REG_BASE: REG_BG2X_L = data; GBA_VideoUpdateRegister(BG2X_L); return;
        case BG2X_H-REG_BASE: REG_BG2X_H = data&0x0FFF; GBA_VideoUpdateRegister(BG2X_H); return;
        case BG2Y_L-REG_BASE: REG_BG2Y_L = data; GBA_VideoUpdateRegister(BG2Y_L); return;
        case BG2Y_H-REG_BASE: REG_BG2Y_H = data&0x0FFF; GBA_VideoUpdateRegister(BG2Y_H); return;

        case BG3X_L-REG_BASE: REG_BG3X_L = data; GBA_VideoUpdateRegister(BG3X_L); return;
        case BG3X_H-REG_BASE: REG_BG3X_H = data&0x0FFF; GBA_VideoUpdateRegister(BG3X_H); return;
        case BG3Y_L-REG_BASE: REG_BG3Y_L = data; GBA_VideoUpdateRegister(BG3Y_L); return;
        case BG3Y_H-REG_BASE: REG_BG3Y_H = data&0x0FFF; GBA_VideoUpdateRegister(BG3Y_H); return;

        case WIN0H-REG_BASE: REG_WIN0H = data; GBA_VideoUpdateRegister(WIN0H); return;
        case WIN0V-REG_BASE: REG_WIN0V = data; GBA_VideoUpdateRegister(WIN0V); return;
        case WIN1H-REG_BASE: REG_WIN1H = data; GBA_VideoUpdateRegister(WIN1H); return;
        case WIN1V-REG_BASE: REG_WIN1V = data; GBA_VideoUpdateRegister(WIN1V); return;
        case WININ-REG_BASE: REG_WININ = data; GBA_VideoUpdateRegister(WININ); return;
        case WINOUT-REG_BASE: REG_WINOUT = data; GBA_VideoUpdateRegister(WINOUT); return;
        case MOSAIC-REG_BASE: REG_MOSAIC = data; GBA_VideoUpdateRegister(MOSAIC); return;

//        extern s32 scrclocks;
//        case BLDCNT-REG_BASE: REG_BLDCNT = data; Debug_DebugMsgArg("CNT %d",scrclocks); return;
//        case BLDALPHA-REG_BASE: REG_BLDALPHA = data; Debug_DebugMsgArg("ALPHA %d",scrclocks); return;
//        case BLDY-REG_BASE: REG_BLDY = data; Debug_DebugMsgArg("Y %d",scrclocks); return;

        case DMA0CNT_H-REG_BASE: REG_DMA0CNT_H = data; GBA_DMA0Setup(); GBA_ExecutionBreak(); return;
        case DMA1CNT_H-REG_BASE: REG_DMA1CNT_H = data; GBA_DMA1Setup(); GBA_ExecutionBreak(); return;
        case DMA2CNT_H-REG_BASE: REG_DMA2CNT_H = data; GBA_DMA2Setup(); GBA_ExecutionBreak(); return;
        case DMA3CNT_H-REG_BASE: REG_DMA3CNT_H = data; GBA_DMA3Setup(); GBA_ExecutionBreak(); return;

        case TM0CNT_L-REG_BASE: GBA_TimerSetStart0(data); GBA_ExecutionBreak(); return;
        case TM0CNT_H-REG_BASE: REG_TM0CNT_H = data; GBA_TimerSetup0(); GBA_ExecutionBreak(); return;
        case TM1CNT_L-REG_BASE: GBA_TimerSetStart1(data); GBA_ExecutionBreak(); return;
        case TM1CNT_H-REG_BASE: REG_TM1CNT_H = data; GBA_TimerSetup1(); GBA_ExecutionBreak(); return;
        case TM2CNT_L-REG_BASE: GBA_TimerSetStart2(data); GBA_ExecutionBreak(); return;
        case TM2CNT_H-REG_BASE: REG_TM2CNT_H = data; GBA_TimerSetup2(); GBA_ExecutionBreak(); return;
        case TM3CNT_L-REG_BASE: GBA_TimerSetStart3(data); GBA_ExecutionBreak(); return;
        case TM3CNT_H-REG_BASE: REG_TM3CNT_H = data; GBA_TimerSetup3(); GBA_ExecutionBreak(); return;

        case SOUND1CNT_L-REG_BASE: REG_SOUND1CNT_L = data; GBA_SoundRegWrite16(SOUND1CNT_L,data); return;
        case SOUND1CNT_H-REG_BASE: REG_SOUND1CNT_H = data; GBA_SoundRegWrite16(SOUND1CNT_H,data); return;
        case SOUND1CNT_X-REG_BASE: REG_SOUND1CNT_X = data; GBA_SoundRegWrite16(SOUND1CNT_X,data); return;
        case SOUND2CNT_L-REG_BASE: REG_SOUND2CNT_L = data; GBA_SoundRegWrite16(SOUND2CNT_L,data); return;
        case SOUND2CNT_H-REG_BASE: REG_SOUND2CNT_H = data; GBA_SoundRegWrite16(SOUND2CNT_H,data); return;
        case SOUND3CNT_L-REG_BASE: REG_SOUND3CNT_L = data; GBA_SoundRegWrite16(SOUND3CNT_L,data); return;
        case SOUND3CNT_H-REG_BASE: REG_SOUND3CNT_H = data; GBA_SoundRegWrite16(SOUND3CNT_H,data); return;
        case SOUND3CNT_X-REG_BASE: REG_SOUND3CNT_X = data; GBA_SoundRegWrite16(SOUND3CNT_X,data); return;
        case SOUND4CNT_L-REG_BASE: REG_SOUND4CNT_L = data; GBA_SoundRegWrite16(SOUND4CNT_L,data); return;
        case SOUND4CNT_H-REG_BASE: REG_SOUND4CNT_H = data; GBA_SoundRegWrite16(SOUND4CNT_H,data); return;
        case SOUNDCNT_L-REG_BASE: REG_SOUNDCNT_L = data; GBA_SoundRegWrite16(SOUNDCNT_L,data); return;
        case SOUNDCNT_H-REG_BASE: REG_SOUNDCNT_H = data; GBA_SoundRegWrite16(SOUNDCNT_H,data); return;
        case SOUNDCNT_X-REG_BASE: REG_SOUNDCNT_X &= 0x000F; REG_SOUNDCNT_X |= data&0xFFF0;
            GBA_SoundRegWrite16(SOUNDCNT_X,data&0xFFF0); return;
        case SOUNDBIAS-REG_BASE: REG_SOUNDBIAS = data; GBA_SoundRegWrite16(SOUNDBIAS,data); return;
        case FIFO_A+0-REG_BASE: REG_16(FIFO_A+0) = data; GBA_SoundRegWrite16(FIFO_A+0,data); return;
        case FIFO_A+2-REG_BASE: REG_16(FIFO_A+2) = data; GBA_SoundRegWrite16(FIFO_A+2,data); return;
        case FIFO_B+0-REG_BASE: REG_16(FIFO_B+0) = data; GBA_SoundRegWrite16(FIFO_B+0,data); return;
        case FIFO_B+2-REG_BASE: REG_16(FIFO_B+2) = data; GBA_SoundRegWrite16(FIFO_B+2,data); return;
        case WAVE_RAM+0-REG_BASE: case WAVE_RAM+2-REG_BASE:
        case WAVE_RAM+4-REG_BASE: case WAVE_RAM+6-REG_BASE:
        case WAVE_RAM+8-REG_BASE: case WAVE_RAM+10-REG_BASE:
        case WAVE_RAM+12-REG_BASE: case WAVE_RAM+14-REG_BASE:
            REG_16(address) = data;
            GBA_SoundRegWrite16(address,data);
            return;

        case IE-REG_BASE: REG_IE = data; GBA_ExecutionBreak(); return;
        case IF-REG_BASE: REG_IF &= ~data; return;
        case WAITCNT: REG_WAITCNT = data; GBA_MemoryAccessCyclesUpdate(); return;
        case IME-REG_BASE: REG_IME = data; GBA_ExecutionBreak(); return;
        case POSTFLG-REG_BASE: //POSTFLG + HALTCNT
            REG_POSTFLG = (u8)data;
            REG_HALTCNT = (u8)(data>>8);
            GBA_CPUSetHalted((u8)(data>>8));
            GBA_ExecutionBreak();
            return;

        case 0x4000128-REG_BASE:
            return;

        default:
            REG_16(address) = data;
            return;
    }
}

static const int gbaregister_canread_16[0x400/2] = {
    1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x000
    0,0,0,0,1,1,0,0, 1,0,0,0,0,0,0,0, 1,1,1,0,1,0,1,0, 1,1,1,0,1,0,1,0, //0x040
    1,1,1,0,1,0,0,0, 1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,0, //0x080
    0,0,0,1,0,0,0,0, 0,1,0,0,0,0,0,1, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x0C0
    1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0, 1,1,1,1,1,1,0,0, 1,1,1,0,0,0,0,0, //0x100
    1,0,0,0,0,0,0,0, 1,1,1,1,1,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x140
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x180
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x1C0
    1,1,1,0,1,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x200
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x240
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x280
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x2C0
    1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x300 //This register is not totally readable,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x340 //only the lower byte.
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //0x380
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0  //0x3C0
};
//  4000410h  ?    ?    ?         Undocumented - Purpose Unknown / Bug ??? 0FFh
//  4000800h  4    R/W  ?         Undocumented - Internal Memory Control (R/W)
//  4xx0800h  4    R/W  ?         Mirrors of 4000800h (repeated each 64K)

u16 GBA_RegisterRead16(u32 address)
{
    if(address & 0x00FFFC00) return 0; //>= 4000400
    if(gbaregister_canread_16[(address&0x3FF)>>1]) return REG_16(address);
    else return 0;
}

void GBA_RegisterWrite8(u32 address, u8 data) // ??????
{
    if(address == REG_POSTFLG) { Debug_DebugMsgArg("reg_write_8 REG_POSTFLG (bug)"); GBA_ExecutionBreak(); }
    //BUG: THIS WILL ENTER HALT MODE WHEN WRITING TO REG_POSTFLG
    u16 temp = GBA_RegisterRead16(address&~1);
    if(address&1) GBA_RegisterWrite16(address&~1,(temp&0x00FF) | (((u16)data)<<8));
    else GBA_RegisterWrite16(address&~1,(temp&0xFF00) | (u16)data);
}

u8 GBA_RegisterRead8(u32 address)
{
    if(address&1) return (GBA_RegisterRead16(address&~1)>>8);
    else return (GBA_RegisterRead16(address&~1)&0xFF);
}

//--------------------------------------------------------------------------------------------

u32 wait_table_seq[16] =      { 0,0,2,0, 0,0,0,0, 2,2,4,4, 8,8,4,4 }; //Defaults
u32 wait_table_nonseq[16] =   { 0,0,2,0, 0,0,0,0, 4,4,4,4, 4,4,4,4 };
/*
u32 wait_table_seq[16] =      { 0,0,1,0, 0,0,0,0, 1,1,3,3, 7,7,3,3 }; //Defaults
u32 wait_table_nonseq[16] =   { 0,0,1,0, 0,0,0,0, 3,3,3,3, 3,3,3,3 }; //Cheated clocks
*/

const s32 mem_bus_is_16[16] = { 0,0,1,0, 0,1,1,0, 1,1,1,1, 1,1,1,1 };

void GBA_MemoryAccessCyclesUpdate(void)
{
    u32 data = REG_WAITCNT;

    static const u32 sram_wait[4] = { 4,3,2,8 };
    static const u32 rom_wait_nonseq[4] = { 4,3,2,8 };
    static const u32 rom_wait_seq0[2] = { 2,1 };
    static const u32 rom_wait_seq1[2] = { 4,1 };
    static const u32 rom_wait_seq2[2] = { 8,1 };
/*
    static const u32 sram_wait[4] = { 3,2,1,7 }; //Cheated clocks
    static const u32 rom_wait_nonseq[4] = { 3,2,1,7 };
    static const u32 rom_wait_seq0[2] = { 1,0 };
    static const u32 rom_wait_seq1[2] = { 3,0 };
    static const u32 rom_wait_seq2[2] = { 7,0 };
*/
    u32 sram_clocks = sram_wait[data&3];
    wait_table_seq[14] = sram_clocks; wait_table_seq[15] = sram_clocks;
    wait_table_nonseq[14] = sram_clocks; wait_table_nonseq[15] = sram_clocks;

    u32 rom0_nonseq = rom_wait_nonseq[(data>>2)&3];
    wait_table_nonseq[8] = rom0_nonseq; wait_table_nonseq[9] = rom0_nonseq;

    u32 rom0_seq = rom_wait_seq0[(data>>4)&1];
    wait_table_seq[8] = rom0_seq; wait_table_seq[9] = rom0_seq;

    u32 rom1_nonseq = rom_wait_nonseq[(data>>5)&3];
    wait_table_nonseq[10] = rom1_nonseq; wait_table_nonseq[11] = rom1_nonseq;

    u32 rom1_seq = rom_wait_seq1[(data>>7)&1];
    wait_table_seq[10] = rom1_seq; wait_table_seq[11] = rom1_seq;

    u32 rom2_nonseq = rom_wait_nonseq[(data>>8)&3];
    wait_table_nonseq[12] = rom2_nonseq; wait_table_nonseq[13] = rom2_nonseq;

    u32 rom2_seq = rom_wait_seq2[(data>>10)&1];
    wait_table_seq[12] = rom2_seq; wait_table_seq[13] = rom2_seq;

    //phi, prefetch, etc...
    /*
  11-12 PHI Terminal Output        (0..3 = Disable, 4.19MHz, 8.38MHz, 16.78MHz)
  14    Game Pak Prefetch Buffer (Pipe) (0=Disable, 1=Enable)
  15    Game Pak Type Flag  (Read Only) (0=GBA, 1=CGB) (IN35 signal)
    */
}
