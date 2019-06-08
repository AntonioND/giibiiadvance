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

#ifndef __GBA_MEMORY__
#define __GBA_MEMORY__

#include "gba.h"

extern _mem_t Mem;

//----------------------------------------------------------------------

void GBA_MemoryInit(u32 * bios_ptr, u32 * rom_ptr, u32 romsize);
void GBA_MemoryEnd(void);

//----------------------------------------------------------------------

u32 GBA_MemoryReadFast32(u32 address); //doesn't do any checking
u16 GBA_MemoryReadFast16(u32 address);
u8 GBA_MemoryReadFast8(u32 address);
//void GBA_MemoryWriteFast32(u32 address,u32 data); //they don't work right
//void GBA_MemoryWriteFast16(u32 address,u16 data);
//void GBA_MemoryWriteFast8(u32 address,u8 data);

u32 GBA_MemoryRead32(u32 address);
void GBA_MemoryWrite32(u32 address,u32 data);

u16 GBA_MemoryRead16(u32 address);
void GBA_MemoryWrite16(u32 address,u16 data);

u8 GBA_MemoryRead8(u32 address);
void GBA_MemoryWrite8(u32 address,u8 data);

//----------------------------------------------------------------------

void GBA_RegisterWrite32(u32 address, u32 data);
u32 GBA_RegisterRead32(u32 address);
void GBA_RegisterWrite16(u32 address, u16 data);
u16 GBA_RegisterRead16(u32 address);
void GBA_RegisterWrite8(u32 address, u8 data);
u8 GBA_RegisterRead8(u32 address);

//----------------------------------------------------------------------

void GBA_MemoryAccessCyclesUpdate(void);

extern u32 wait_table_seq[];
extern u32 wait_table_nonseq[];
extern const s32 mem_bus_is_16[];

static u32 GBA_MemoryGetAccessCycles(u32 seq, u32 _32bit, u32 address)
{
    //if((address & 0x1FFFF) == 0) seq = 0;
    u32 index = (address>>24)&0xF;
    u32 doubletime = _32bit && mem_bus_is_16[index];
    if(seq)
    {
        return (wait_table_seq[index] + 1) << doubletime;
    }
    else
    {
        if(doubletime) return (wait_table_seq[index] + wait_table_nonseq[index] + 2);
        else return (wait_table_nonseq[index] + 1);
    }
}

static u32 GBA_MemoryGetAccessCyclesNoSeq(u32 _32bit, u32 address)
{
    u32 index = (address>>24)&0xF;
    if(_32bit && mem_bus_is_16[index]) return (wait_table_seq[index] + wait_table_nonseq[index] + 2);
    else return (wait_table_nonseq[index] + 1);
}

static u32 GBA_MemoryGetAccessCyclesSeq(u32 _32bit, u32 address)
{
    //if((address & 0x1FFFF) == 0) return GBA_MemoryGetAccessCyclesNoSeq(_32bit,address);
    u32 index = (address>>24)&0xF;
    return (wait_table_seq[index] + 1) << (_32bit && mem_bus_is_16[index]);
}

static u32 GBA_MemoryGetAccessCyclesNoSeq32(u32 address)
{
    u32 index = (address>>24)&0xF;
    if(mem_bus_is_16[index]) return (wait_table_seq[index] + wait_table_nonseq[index] + 2);
    else return (wait_table_nonseq[index] + 1);
}

static u32 GBA_MemoryGetAccessCyclesNoSeq16(u32 address)
{
    return (wait_table_nonseq[(address>>24)&0xF] + 1);
}

static u32 GBA_MemoryGetAccessCyclesSeq32(u32 address)
{
    //if((address & 0x1FFFF) == 0) return GBA_MemoryGetAccessCyclesNoSeq32(address);
    u32 index = (address>>24)&0xF;
    return (wait_table_seq[index] + 1) << (mem_bus_is_16[index]);
}

static u32 GBA_MemoryGetAccessCyclesSeq16(u32 address)
{
    //if((address & 0x1FFFF) == 0) return GBA_MemoryGetAccessCyclesNoSeq16(address);
    return (wait_table_seq[(address>>24)&0xF] + 1);
}

//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------

#define REG_BASE (0x04000000)
#define REG_8(addr)     (*((u8*)&Mem.io_regs[(addr)-REG_BASE]))
#define REG_16(addr)    (*((u16*)&Mem.io_regs[(addr)-REG_BASE]))
#define REG_32(addr)    (*((u32*)&Mem.io_regs[(addr)-REG_BASE]))

//------------------------------------------

//LCD I/O Registers
#define DISPCNT             (0x04000000) //R/W
#define GREENSWAP           (0x04000002) //R/W
#define DISPSTAT            (0x04000004) //R/W
#define VCOUNT              (0x04000006) //R
#define BG0CNT              (0x04000008) //R/W
#define BG1CNT              (0x0400000A) //R/W
#define BG2CNT              (0x0400000C) //R/W
#define BG3CNT              (0x0400000E) //R/W
#define BG0HOFS             (0x04000010) //W
#define BG0VOFS             (0x04000012) //W
#define BG1HOFS             (0x04000014) //W
#define BG1VOFS             (0x04000016) //W
#define BG2HOFS             (0x04000018) //W
#define BG2VOFS             (0x0400001A) //W
#define BG3HOFS             (0x0400001C) //W
#define BG3VOFS             (0x0400001E) //W
#define BG2PA               (0x04000020) //W
#define BG2PB               (0x04000022) //W
#define BG2PC               (0x04000024) //W
#define BG2PD               (0x04000026) //W
#define BG2X_L              (0x04000028) //W
#define BG2X_H              (0x0400002A) //W
#define BG2Y_L              (0x0400002C) //W
#define BG2Y_H              (0x0400002E) //W
#define BG3PA               (0x04000030) //W
#define BG3PB               (0x04000032) //W
#define BG3PC               (0x04000034) //W
#define BG3PD               (0x04000036) //W
#define BG3X_L              (0x04000038) //W
#define BG3X_H              (0x0400003A) //W
#define BG3Y_L              (0x0400003C) //W
#define BG3Y_H              (0x0400003E) //W
#define WIN0H               (0x04000040) //W
#define WIN1H               (0x04000042) //W
#define WIN0V               (0x04000044) //W
#define WIN1V               (0x04000046) //W
#define WININ               (0x04000048) //R/W
#define WINOUT              (0x0400004A) //R/W
#define MOSAIC              (0x0400004C) //W
#define BLDCNT              (0x04000050) //R/W
#define BLDALPHA            (0x04000052) //W
#define BLDY                (0x04000054) //W

//Sound Registers
#define SOUND1CNT_L         (0x04000060) //R/W
#define SOUND1CNT_H         (0x04000062) //R/W
#define SOUND1CNT_X         (0x04000064) //R/W
#define SOUND2CNT_L         (0x04000068) //R/W
#define SOUND2CNT_H         (0x0400006C) //R/W
#define SOUND3CNT_L         (0x04000070) //R/W
#define SOUND3CNT_H         (0x04000072) //R/W
#define SOUND3CNT_X         (0x04000074) //R/W
#define SOUND4CNT_L         (0x04000078) //R/W
#define SOUND4CNT_H         (0x0400007C) //R/W
#define SOUNDCNT_L          (0x04000080) //R/W
#define SOUNDCNT_H          (0x04000082) //R/W
#define SOUNDCNT_X          (0x04000084) //R/W
#define SOUNDBIAS           (0x04000088) //BIOS
#define WAVE_RAM            (0x04000090) //R/W -- 2x10h bytes
#define FIFO_A              (0x040000A0) //W
#define FIFO_B              (0x040000A4) //W

//DMA Transfer Channels
#define DMA0SAD             (0x040000B0) //W
#define DMA0DAD             (0x040000B4) //W
#define DMA0CNT_L           (0x040000B8) //W
#define DMA0CNT_H           (0x040000BA) //R/W
#define DMA1SAD             (0x040000BC) //W
#define DMA1DAD             (0x040000C0) //W
#define DMA1CNT_L           (0x040000C4) //W
#define DMA1CNT_H           (0x040000C6) //R/W
#define DMA2SAD             (0x040000C8) //W
#define DMA2DAD             (0x040000CC) //W
#define DMA2CNT_L           (0x040000D0) //W
#define DMA2CNT_H           (0x040000D2) //R/W
#define DMA3SAD             (0x040000D4) //W
#define DMA3DAD             (0x040000D8) //W
#define DMA3CNT_L           (0x040000DC) //W
#define DMA3CNT_H           (0x040000DE) //R/W

//Timer Registers
#define TM0CNT_L            (0x04000100) //R/W
#define TM0CNT_H            (0x04000102) //R/W
#define TM1CNT_L            (0x04000104) //R/W
#define TM1CNT_H            (0x04000106) //R/W
#define TM2CNT_L            (0x04000108) //R/W
#define TM2CNT_H            (0x0400010A) //R/W
#define TM3CNT_L            (0x0400010C) //R/W
#define TM3CNT_H            (0x0400010E) //R/W

//Serial Communication (1)
#define SIODATA32           (0x04000120) //R/W
#define SIOMULTI0           (0x04000120) //R/W
#define SIOMULTI1           (0x04000122) //R/W
#define SIOMULTI2           (0x04000124) //R/W
#define SIOMULTI3           (0x04000126) //R/W
#define SIOCNT              (0x04000128) //R/W
#define SIOMLT_SEND         (0x0400012A) //R/W
#define SIODATA8            (0x0400012A) //R/W

//Keypad Input
#define KEYINPUT            (0x04000130) //R
#define KEYCNT              (0x04000132) //R/W

//Serial Communication (2)
#define RCNT                (0x04000134) //R/W
// 4000136h  -    -    IR        Ancient - Infrared Register (Prototypes only)
#define JOYCNT              (0x04000140) //R/W
#define JOY_RECV            (0x04000150) //R/W
#define JOY_TRANS           (0x04000154) //R/W
#define JOYSTAT             (0x04000158) //R/?

//Interrupt, Waitstate, and Power-Down Control
#define IE                  (0x04000200) //R/W
#define IF                  (0x04000202) //R/W
#define WAITCNT             (0x04000204) //R/W
#define IME                 (0x04000208) //R/W
#define POSTFLG             (0x04000300) //R/W
#define HALTCNT             (0x04000301) //W
// 4000410h  ?    ?    ?         Undocumented - Purpose Unknown / Bug ??? 0FFh
// 4000800h  4    R/W  ?         Undocumented - Internal Memory Control (R/W)
// 4xx0800h  4    R/W  ?         Mirrors of 4000800h (repeated each 64K)

// All further addresses at 4XXXXXXh are unused and do not contain mirrors of the
// I/O area, with the only exception that 4000800h is repeated each 64K (ie.
// mirrored at 4010800h, 4020800h, etc.)

//------------------------------------------

#define REG_DISPCNT             REG_16(DISPCNT)
#define REG_GREENSWAP           REG_16(GREENSWAP)
#define REG_DISPSTAT            REG_16(DISPSTAT)
#define REG_VCOUNT              REG_16(VCOUNT)
#define REG_BG0CNT              REG_16(BG0CNT)
#define REG_BG1CNT              REG_16(BG1CNT)
#define REG_BG2CNT              REG_16(BG2CNT)
#define REG_BG3CNT              REG_16(BG3CNT)
#define REG_BG0HOFS             REG_16(BG0HOFS)
#define REG_BG0VOFS             REG_16(BG0VOFS)
#define REG_BG1HOFS             REG_16(BG1HOFS)
#define REG_BG1VOFS             REG_16(BG1VOFS)
#define REG_BG2HOFS             REG_16(BG2HOFS)
#define REG_BG2VOFS             REG_16(BG2VOFS)
#define REG_BG3HOFS             REG_16(BG3HOFS)
#define REG_BG3VOFS             REG_16(BG3VOFS)
#define REG_BG2PA               REG_16(BG2PA)
#define REG_BG2PB               REG_16(BG2PB)
#define REG_BG2PC               REG_16(BG2PC)
#define REG_BG2PD               REG_16(BG2PD)
#define REG_BG2X_L              REG_16(BG2X_L)
#define REG_BG2X_H              REG_16(BG2X_H)
#define REG_BG2X                REG_32(BG2X_L)
#define REG_BG2Y_L              REG_16(BG2Y_L)
#define REG_BG2Y_H              REG_16(BG2Y_H)
#define REG_BG2Y                REG_32(BG2Y_L)
#define REG_BG3PA               REG_16(BG3PA)
#define REG_BG3PB               REG_16(BG3PB)
#define REG_BG3PC               REG_16(BG3PC)
#define REG_BG3PD               REG_16(BG3PD)
#define REG_BG3X_L              REG_16(BG3X_L)
#define REG_BG3X_H              REG_16(BG3X_H)
#define REG_BG3X                REG_32(BG3X_L)
#define REG_BG3Y_L              REG_16(BG3Y_L)
#define REG_BG3Y_H              REG_16(BG3Y_H)
#define REG_BG3Y                REG_32(BG3Y_L)
#define REG_WIN0H               REG_16(WIN0H)
#define REG_WIN1H               REG_16(WIN1H)
#define REG_WIN0V               REG_16(WIN0V)
#define REG_WIN1V               REG_16(WIN1V)
#define REG_WININ               REG_16(WININ)
#define REG_WINOUT              REG_16(WINOUT)
#define REG_MOSAIC              REG_16(MOSAIC)
#define REG_BLDCNT              REG_16(BLDCNT)
#define REG_BLDALPHA            REG_16(BLDALPHA)
#define REG_BLDY                REG_16(BLDY)
#define REG_SOUND1CNT_L         REG_16(SOUND1CNT_L)
#define REG_SOUND1CNT_H         REG_16(SOUND1CNT_H)
#define REG_SOUND1CNT_X         REG_16(SOUND1CNT_X)
#define REG_SOUND2CNT_L         REG_16(SOUND2CNT_L)
#define REG_SOUND2CNT_H         REG_16(SOUND2CNT_H)
#define REG_SOUND3CNT_L         REG_16(SOUND3CNT_L)
#define REG_SOUND3CNT_H         REG_16(SOUND3CNT_H)
#define REG_SOUND3CNT_X         REG_16(SOUND3CNT_X)
#define REG_SOUND4CNT_L         REG_16(SOUND4CNT_L)
#define REG_SOUND4CNT_H         REG_16(SOUND4CNT_H)
#define REG_SOUNDCNT_L          REG_16(SOUNDCNT_L)
#define REG_SOUNDCNT_H          REG_16(SOUNDCNT_H)
#define REG_SOUNDCNT_X          REG_16(SOUNDCNT_X)
#define REG_SOUNDBIAS           REG_16(SOUNDBIAS)
#define REG_WAVE_RAM            ((u8*)&Mem.io_regs[WAVE_RAM-REG_BASE])
#define REG_FIFO_A              REG_32(FIFO_A)
#define REG_FIFO_B              REG_32(FIFO_B)
#define REG_DMA0SAD             REG_32(DMA0SAD)
#define REG_DMA0DAD             REG_32(DMA0DAD)
#define REG_DMA0CNT_L           REG_16(DMA0CNT_L)
#define REG_DMA0CNT_H           REG_16(DMA0CNT_H)
#define REG_DMA0CNT             REG_32(DMA0CNT_L)
#define REG_DMA1SAD             REG_32(DMA1SAD)
#define REG_DMA1DAD             REG_32(DMA1DAD)
#define REG_DMA1CNT_L           REG_16(DMA1CNT_L)
#define REG_DMA1CNT_H           REG_16(DMA1CNT_H)
#define REG_DMA1CNT             REG_32(DMA1CNT_L)
#define REG_DMA2SAD             REG_32(DMA2SAD)
#define REG_DMA2DAD             REG_32(DMA2DAD)
#define REG_DMA2CNT_L           REG_16(DMA2CNT_L)
#define REG_DMA2CNT_H           REG_16(DMA2CNT_H)
#define REG_DMA2CNT             REG_32(DMA2CNT_L)
#define REG_DMA3SAD             REG_32(DMA3SAD)
#define REG_DMA3DAD             REG_32(DMA3DAD)
#define REG_DMA3CNT_L           REG_16(DMA3CNT_L)
#define REG_DMA3CNT_H           REG_16(DMA3CNT_H)
#define REG_DMA3CNT             REG_32(DMA3CNT_L)
#define REG_TM0CNT_L            REG_16(TM0CNT_L)
#define REG_TM0CNT_H            REG_16(TM0CNT_H)
#define REG_TM1CNT_L            REG_16(TM1CNT_L)
#define REG_TM1CNT_H            REG_16(TM1CNT_H)
#define REG_TM2CNT_L            REG_16(TM2CNT_L)
#define REG_TM2CNT_H            REG_16(TM2CNT_H)
#define REG_TM3CNT_L            REG_16(TM3CNT_L)
#define REG_TM3CNT_H            REG_16(TM3CNT_H)
#define REG_SIODATA32           REG_32(SIODATA32)
#define REG_SIOMULTI0           REG_16(SIOMULTI0)
#define REG_SIOMULTI1           REG_16(SIOMULTI1)
#define REG_SIOMULTI2           REG_16(SIOMULTI2)
#define REG_SIOMULTI3           REG_16(SIOMULTI3)
#define REG_SIOCNT              REG_16(SIOCNT)
#define REG_SIOMLT_SEND         REG_16(SIOMLT_SEND)
#define REG_SIODATA8            REG_16(SIODATA8)
#define REG_KEYINPUT            REG_16(KEYINPUT)
#define REG_KEYCNT              REG_16(KEYCNT)
#define REG_RCNT                REG_16(RCNT)

#define REG_JOYCNT              REG_16(JOYCNT)
#define REG_JOY_RECV            REG_32(JOY_RECV)
#define REG_JOY_TRANS           REG_32(JOY_TRANS)
#define REG_JOYSTAT             REG_16(JOYSTAT)
#define REG_IE                  REG_16(IE)
#define REG_IF                  REG_16(IF)
#define REG_WAITCNT             REG_16(WAITCNT)
#define REG_IME                 REG_16(IME)
#define REG_POSTFLG             REG_8(0x04000300)
#define REG_HALTCNT             REG_8(0x04000301)

//----------------------------------------------------------------------

#endif //__GBA_MEMORY__

