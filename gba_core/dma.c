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

#include "../build_options.h"
#include "../debug_utils.h"

#include "gba.h"
#include "memory.h"
#include "video.h"
#include "dma.h"
#include "interrupts.h"
#include "cpu.h"

typedef struct {
    u32 enabled;

    u32 srcaddr;
    u32 dstaddr;

    u32 num_chunks;

    s32 srcadd;
    s32 dstadd;
    u32 dst_reload;
    u32 repeat;

    #define START_NOW     0
    #define START_VBL     1
    #define START_HBL     2
    #define START_SPECIAL 3 //DMA0=Prohibited, DMA1/DMA2=Sound FIFO, DMA3=Video Capture
    u32 starttime;

    u32 copywords;

    s32 clocksremaining;
    s32 clockstotal;

    u32 special;
} _dma_channel_;

static _dma_channel_ DMA[4];

//--------------------------------------------------------------------------

static inline s32 min(s32 a, s32 b) { return ((a < b) ? a : b); }

//--------------------------------------------------------------------------

static s32 gba_dma_src_add[4] = { 2, -2, 0, 0 }; //gba_dma_src_add[3] = prohibited
static s32 gba_dma_dst_add[4] = { 2, -2, 0, 2 };

static int gba_dmaworking = 0;
static s32 gba_dma_extra_clocks_elapsed = 0;

void GBA_DMA0Setup(void)
{
    DMA[0].enabled = 0;
    DMA[0].starttime = 0;
    if((REG_DMA0CNT_H & BIT(15)) == 0) return;
    DMA[0].enabled = 1;

    DMA[0].num_chunks = ((u32)REG_DMA0CNT_L)&0x3FFF; if(DMA[0].num_chunks == 0) DMA[0].num_chunks = 0x4000;
    DMA[0].copywords = REG_DMA0CNT_H & BIT(10);

    DMA[0].srcaddr = REG_DMA0SAD & (DMA[0].copywords ? 0x07FFFFFC : 0x07FFFFFE);
    DMA[0].dstaddr = REG_DMA0DAD & (DMA[0].copywords ? 0x07FFFFFC : 0x07FFFFFE);

    if(DMA[0].copywords)
    {
        DMA[0].clockstotal =
            // READ CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq32(DMA[0].srcaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq32(DMA[0].srcaddr) * (DMA[0].num_chunks - 1)) +
            // WRITE CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq32(DMA[0].dstaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq32(DMA[0].dstaddr) * (DMA[0].num_chunks - 1)) +
            //PROCESSING
            2; //2I
    }
    else
    {
        DMA[0].clockstotal =
            // READ CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq16(DMA[0].srcaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq16(DMA[0].srcaddr) * (DMA[0].num_chunks - 1)) +
            // WRITE CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq16(DMA[0].dstaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq16(DMA[0].dstaddr) * (DMA[0].num_chunks - 1)) +
            //PROCESSING
            2; //2I
    }
    DMA[0].clocksremaining = DMA[0].clockstotal;
    DMA[0].srcadd = gba_dma_src_add[(REG_DMA0CNT_H>>7)&3];
    DMA[0].dstadd = gba_dma_dst_add[(REG_DMA0CNT_H>>5)&3];
    if(DMA[0].copywords) { DMA[0].srcadd <<= 1; DMA[0].dstadd <<= 1; }
    DMA[0].dst_reload = (((REG_DMA0CNT_H>>5)&3) == 3) ;
    DMA[0].starttime = (REG_DMA0CNT_H>>12)&3;
    DMA[0].repeat = REG_DMA0CNT_H & BIT(9);

    if(DMA[0].starttime == START_NOW) { gba_dmaworking = 1; GBA_ExecutionBreak(); }
    else if(DMA[0].starttime == START_SPECIAL) //PROHIBITED
    {
        Debug_DebugMsgArg("DMA 0 in mode 3 - prohibited");
        GBA_ExecutionBreak();
        DMA[0].enabled = 0;
    }

/*
    char text[64]; snprintf(text,sizeof(text),"DMA0 SETUP\nMODE: %d\nSRC: %08X (%d)\nDST: %08X (%d)\n"
        "CHUNK: %08X"
        ,DMA[0].starttime,DMA[0].srcaddr,DMA[0].srcadd,DMA[0].dstaddr,DMA[0].dstadd,
        DMA[0].num_chunks);
    MessageBox(NULL, text, "EMULATION", MB_OK);
    GBA_ExecutionBreak();
*/
}

void GBA_DMA1Setup(void)
{
    DMA[1].enabled = 0;
    DMA[1].starttime = 0;
    if((REG_DMA1CNT_H & BIT(15)) == 0) return;
    DMA[1].enabled = 1;

    DMA[1].num_chunks = ((u32)REG_DMA1CNT_L)&0x3FFF; if(DMA[1].num_chunks == 0) DMA[1].num_chunks = 0x4000;
    DMA[1].copywords = REG_DMA1CNT_H & BIT(10);

    DMA[1].srcaddr = REG_DMA1SAD & (DMA[1].copywords ? 0x07FFFFFC : 0x07FFFFFE);
    DMA[1].dstaddr = REG_DMA1DAD & (DMA[1].copywords ? 0x07FFFFFC : 0x07FFFFFE);

    if(DMA[1].copywords)
    {
        DMA[1].clockstotal =
            // READ CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq32(DMA[1].srcaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq32(DMA[1].srcaddr) * (DMA[1].num_chunks - 1)) +
            // WRITE CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq32(DMA[1].dstaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq32(DMA[1].dstaddr) * (DMA[1].num_chunks - 1)) +
            //PROCESSING
            2; //2I
    }
    else
    {
        DMA[1].clockstotal =
            // READ CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq16(DMA[1].srcaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq16(DMA[1].srcaddr) * (DMA[1].num_chunks - 1)) +
            // WRITE CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq16(DMA[1].dstaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq16(DMA[1].dstaddr) * (DMA[1].num_chunks - 1)) +
            //PROCESSING
            2; //2I
    }
    DMA[1].clocksremaining = DMA[1].clockstotal;
    DMA[1].srcadd = gba_dma_src_add[(REG_DMA1CNT_H>>7)&3];
    DMA[1].dstadd = gba_dma_dst_add[(REG_DMA1CNT_H>>5)&3];
    if(DMA[1].copywords || (DMA[1].starttime == START_SPECIAL)) { DMA[1].srcadd <<= 1; DMA[1].dstadd <<= 1; }
    DMA[1].dst_reload = (((REG_DMA1CNT_H>>5)&3) == 3) ;
    DMA[1].starttime = (REG_DMA1CNT_H>>12)&3;
    DMA[1].repeat = REG_DMA1CNT_H & BIT(9);

    if(DMA[1].starttime == START_NOW) GBA_ExecutionBreak();
}

void GBA_DMA2Setup(void)
{
    DMA[2].enabled = 0;
    DMA[2].starttime = 0;
    if((REG_DMA2CNT_H & BIT(15)) == 0) return;
    DMA[2].enabled = 1;

    DMA[2].num_chunks = ((u32)REG_DMA2CNT_L)&0x3FFF; if(DMA[2].num_chunks == 0) DMA[2].num_chunks = 0x4000;
    DMA[2].copywords = REG_DMA2CNT_H & BIT(10);

    DMA[2].srcaddr = REG_DMA2SAD & (DMA[2].copywords ? ~3 : ~1);
    DMA[2].dstaddr = REG_DMA2DAD & (DMA[2].copywords ? ~3 : ~1);

    if(DMA[2].copywords)
    {
        DMA[2].clockstotal =
            // READ CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq32(DMA[2].srcaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq32(DMA[2].srcaddr) * (DMA[2].num_chunks - 1)) +
            // WRITE CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq32(DMA[2].dstaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq32(DMA[2].dstaddr) * (DMA[2].num_chunks - 1)) +
            //PROCESSING
            2; //2I
    }
    else
    {
        DMA[2].clockstotal =
            // READ CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq16(DMA[2].srcaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq16(DMA[2].srcaddr) * (DMA[2].num_chunks - 1)) +
            // WRITE CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq16(DMA[2].dstaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq16(DMA[2].dstaddr) * (DMA[2].num_chunks - 1)) +
            //PROCESSING
            2; //2I
    }
    DMA[2].clocksremaining = DMA[2].clockstotal;
    DMA[2].srcadd = gba_dma_src_add[(REG_DMA2CNT_H>>7)&3];
    DMA[2].dstadd = gba_dma_dst_add[(REG_DMA2CNT_H>>5)&3];
    if(DMA[2].copywords || (DMA[2].starttime == START_SPECIAL)) { DMA[2].srcadd <<= 1; DMA[2].dstadd <<= 1; }
    DMA[2].dst_reload = (((REG_DMA2CNT_H>>5)&3) == 3) ;
    DMA[2].starttime = (REG_DMA2CNT_H>>12)&3;
    DMA[2].repeat = REG_DMA2CNT_H & BIT(9);

    if(DMA[2].starttime == START_NOW) GBA_ExecutionBreak();
}

void GBA_DMA3Setup(void)
{
    DMA[3].enabled = 0;
    DMA[3].starttime = 0;
    if((REG_DMA3CNT_H & BIT(15)) == 0) return;
    DMA[3].enabled = 1;

    DMA[3].num_chunks = (u32)REG_DMA3CNT_L; if(DMA[3].num_chunks == 0) DMA[3].num_chunks = 0x10000;
    DMA[3].copywords = REG_DMA3CNT_H & BIT(10);

    DMA[3].srcaddr = REG_DMA3SAD & (DMA[3].copywords ? 0x0FFFFFFC : 0x0FFFFFFE);
    DMA[3].dstaddr = REG_DMA3DAD & (DMA[3].copywords ? 0x0FFFFFFC : 0x0FFFFFFE);

    if(DMA[3].copywords)
    {
        DMA[3].clockstotal =
            // READ CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq32(DMA[3].srcaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq32(DMA[3].srcaddr) * (DMA[3].num_chunks - 1)) +
            // WRITE CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq32(DMA[3].dstaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq32(DMA[3].dstaddr) * (DMA[3].num_chunks - 1)) +
            //PROCESSING
            2; //2I
    }
    else
    {
        DMA[3].clockstotal =
            // READ CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq16(DMA[3].srcaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq16(DMA[3].srcaddr) * (DMA[3].num_chunks - 1)) +
            // WRITE CLOCKS
            GBA_MemoryGetAccessCyclesNoSeq16(DMA[3].dstaddr) +  //1N+(n-1)S
            (GBA_MemoryGetAccessCyclesSeq16(DMA[3].dstaddr) * (DMA[3].num_chunks - 1)) +
            //PROCESSING
            2; //2I
    }
    if((DMA[3].srcaddr & 0x08000000) && (DMA[3].dstaddr & 0x08000000))
        DMA[3].clockstotal += 2; //PROCESSING 4I

    DMA[3].clocksremaining = DMA[3].clockstotal;
    DMA[3].srcadd = gba_dma_src_add[(REG_DMA3CNT_H>>7)&3];
    DMA[3].dstadd = gba_dma_dst_add[(REG_DMA3CNT_H>>5)&3];
    if(DMA[3].copywords) { DMA[3].srcadd <<= 1; DMA[3].dstadd <<= 1; }
    DMA[3].dst_reload = (((REG_DMA3CNT_H>>5)&3) == 3) ;
    DMA[3].starttime = (REG_DMA3CNT_H>>12)&3;
    DMA[3].repeat = REG_DMA3CNT_H & BIT(9);

    if(REG_DMA3CNT_H & BIT(11)) { Debug_DebugMsgArg("Game Pak DRQ  - DMA3 (not emulated)"); GBA_ExecutionBreak(); }
    //NOT EMULATED -- It depends on a pin in the GBA Game Pak
    //  BIT 11: Game Pak DRQ  - DMA3 only -  (0=Normal, 1=DRQ <from> Game Pak, DMA3)
    //  [9     DMA Repeat                   (0=Off, 1=On) (Must be zero if Bit 11 set)]

    if(DMA[3].starttime == START_NOW) { gba_dmaworking = 1; GBA_ExecutionBreak(); }
/*
    char text[64]; snprintf(text,sizeof(text),"DMA3 SETUP\nMODE: %d\nSRC: %08X (%d)\nDST: %08X (%d)\n"
        "CHUNK: %08X"
        ,DMA[3].starttime,DMA[3].srcaddr,DMA[3].srcadd,DMA[3].dstaddr,DMA[3].dstadd,
        DMA[3].num_chunks);
    MessageBox(NULL, text, "EMULATION", MB_OK);
    GBA_ExecutionBreak();
*/
}

//--------------------------------------------------------------------------

static inline s32 GBA_DMA0Update(s32 clocks) //return clocks to finish transfer
{
    if(DMA[0].enabled == 0) return 0x7FFFFFFF;

    if(DMA[0].starttime == START_SPECIAL)
    {
        Debug_DebugMsgArg("DMA0, MODE: START_SPECIAL (?)");
        GBA_ExecutionBreak();
        DMA[0].enabled = 0;
        return 0x7FFFFFFF;
    }

    if(DMA[0].clocksremaining == DMA[0].clockstotal)
    {
        u32 copy = 0;

        switch(DMA[0].starttime)
        {
            case START_NOW:
            {
                copy = 1;
                break;
            }
            case START_VBL:
            {
                if(screenmode != SCR_VBL) return 0x7FFFFFFF;
                if(GBA_ScreenJustChangedMode() == 0) return 0x7FFFFFFF;
                copy = 1;
                break;
            }
            case START_HBL:
            {
                if(screenmode != SCR_HBL) return 0x7FFFFFFF;
                if(GBA_ScreenJustChangedMode() == 0) return 0x7FFFFFFF;
                copy = 1;
                break;
            }
            case START_SPECIAL: return 0x7FFFFFFF; //NOT EMULATED *****************************
            default: break;
        }

        if(copy)
        {
            if(DMA[0].copywords) //copy words
            {
                int i;
                for(i = 0; i < DMA[0].num_chunks; i++)
                {
                    GBA_MemoryWrite32(DMA[0].dstaddr,GBA_MemoryRead32(DMA[0].srcaddr));
                    DMA[0].srcaddr += DMA[0].srcadd; DMA[0].dstaddr += DMA[0].dstadd;
                }
            }
            else //copy halfwords
            {
                int i;
                for(i = 0; i < DMA[0].num_chunks; i++)
                {
                    GBA_MemoryWrite16(DMA[0].dstaddr,GBA_MemoryRead16(DMA[0].srcaddr));
                    DMA[0].srcaddr += DMA[0].srcadd; DMA[0].dstaddr += DMA[0].dstadd;
                }
            }
        }
    }

    DMA[0].clocksremaining -= clocks;

    if(DMA[0].clocksremaining <= 0)
    {
        gba_dma_extra_clocks_elapsed = -DMA[0].clocksremaining;

        if(DMA[0].dst_reload) DMA[0].dstaddr = REG_DMA0DAD & (DMA[0].copywords ? ~3 : ~1);

        if(REG_DMA0CNT_H & BIT(14)) GBA_CallInterrupt(BIT(8)); //interrupt

        if(DMA[0].repeat == 0)
        {
            REG_DMA0CNT_H &= ~BIT(15);
            DMA[0].enabled = 0;
        }
        else
        {
            DMA[0].clocksremaining = DMA[0].clockstotal;

            if(DMA[0].starttime == START_NOW)
            {
                //Debug_DebugMsgArg("DMA 0 immediate mode with repeat bit set.");
                //GBA_ExecutionBreak();
                REG_DMA0CNT_H &= ~BIT(15);
                DMA[0].enabled = 0;
                return 0x7FFFFFFF;
            }
        }

        return 0x7FFFFFFF;
    }

    gba_dmaworking = 1;
    return DMA[0].clocksremaining;
}

static inline s32 GBA_DMA1Update(s32 clocks) //return clocks to finish transfer
{
    if(DMA[1].enabled == 0) return 0x7FFFFFFF;

    if(DMA[1].starttime == START_SPECIAL)
    {
        //char text[64]; snprintf(text,sizeof(text),"DMA1, MODE: %d",DMA[1].starttime);
        //MessageBox(NULL, text, "EMULATION", MB_OK);
        //GBA_ExecutionBreak();
        DMA[1].enabled = 0;
        return 0x7FFFFFFF;
    }

    if(DMA[1].clocksremaining == DMA[1].clockstotal)
    {
        u32 copy = 0;

        switch(DMA[1].starttime)
        {
            case START_NOW:
            {
                copy = 1;
                break;
            }
            case START_VBL:
            {
                if(screenmode != SCR_VBL) return 0x7FFFFFFF;
                if(GBA_ScreenJustChangedMode() == 0) return 0x7FFFFFFF;
                copy = 1;
                break;
            }
            case START_HBL:
            {
                if(screenmode != SCR_HBL) return 0x7FFFFFFF;
                if(GBA_ScreenJustChangedMode() == 0) return 0x7FFFFFFF;
                copy = 1;
                break;
            }
            case START_SPECIAL: return 0x7FFFFFFF; //NOT EMULATED HERE
            default: break;
        }

        if(copy)
        {
            if(DMA[1].copywords) //copy words
            {
                int i;
                for(i = 0; i < DMA[1].num_chunks; i++)
                {
                    GBA_MemoryWrite32(DMA[1].dstaddr,GBA_MemoryRead32(DMA[1].srcaddr));
                    DMA[1].srcaddr += DMA[1].srcadd; DMA[1].dstaddr += DMA[1].dstadd;
                }
            }
            else //copy halfwords
            {
                int i;
                for(i = 0; i < DMA[1].num_chunks; i++)
                {
                    GBA_MemoryWrite16(DMA[1].dstaddr,GBA_MemoryRead16(DMA[1].srcaddr));
                    DMA[1].srcaddr += DMA[1].srcadd; DMA[1].dstaddr += DMA[1].dstadd;
                }
            }
        }
    }

    DMA[1].clocksremaining -= clocks;

    if(DMA[1].clocksremaining <= 0)
    {
        gba_dma_extra_clocks_elapsed = -DMA[1].clocksremaining;

        if(DMA[1].dst_reload) DMA[1].dstaddr = REG_DMA0DAD & (DMA[1].copywords ? ~3 : ~1);

        if(REG_DMA1CNT_H & BIT(14)) GBA_CallInterrupt(BIT(9)); //interrupt

        if(DMA[1].repeat == 0)
        {
            REG_DMA1CNT_H &= ~BIT(15);
            DMA[1].enabled = 0;
        }
        else
        {
            DMA[1].clocksremaining = DMA[1].clockstotal;

            if(DMA[1].starttime == START_NOW)
            {
                //Debug_DebugMsgArg("DMA 1 immediate mode with repeat bit set.");
                //GBA_ExecutionBreak();
                REG_DMA1CNT_H &= ~BIT(15);
                DMA[1].enabled = 0;
                return 0x7FFFFFFF;
            }
        }

        return 0x7FFFFFFF;
    }

    gba_dmaworking = 1;
    return DMA[1].clocksremaining;

    /*
    if(DMA[1].enabled == 0) return 0x7FFFFFFF;

    if(DMA[1].starttime != 3) { MessageBox(NULL, "DMA 1 enabled.", "EMULATION", MB_OK); GBA_ExecutionBreak(); }
    REG_DMA1CNT_H &= ~BIT(15);
    DMA[1].enabled = 0;

    return 0x7FFFFFFF;
    */
}

static inline s32 GBA_DMA2Update(s32 clocks) //return clocks to finish transfer
{
    if(DMA[2].enabled == 0) return 0x7FFFFFFF;

    if(DMA[2].starttime == START_SPECIAL)
    {
        //char text[64]; snprintf(text,sizeof(text),"DMA1, MODE: %d",DMA[1].starttime);
        //MessageBox(NULL, text, "EMULATION", MB_OK);
        //GBA_ExecutionBreak();
        DMA[2].enabled = 0;
        return 0x7FFFFFFF;
    }

    if(DMA[2].clocksremaining == DMA[2].clockstotal)
    {
        u32 copy = 0;

        switch(DMA[2].starttime)
        {
            case START_NOW:
            {
                copy = 1;
                break;
            }
            case START_VBL:
            {
                if(screenmode != SCR_VBL) return 0x7FFFFFFF;
                if(GBA_ScreenJustChangedMode() == 0) return 0x7FFFFFFF;
                copy = 1;
                break;
            }
            case START_HBL:
            {
                if(screenmode != SCR_HBL) return 0x7FFFFFFF;
                if(GBA_ScreenJustChangedMode() == 0) return 0x7FFFFFFF;
                copy = 1;
                break;
            }
            case START_SPECIAL: return 0x7FFFFFFF; //NOT EMULATED HERE
            default: break;
        }

        if(copy)
        {
            if(DMA[2].copywords) //copy words
            {
                int i;
                for(i = 0; i < DMA[2].num_chunks; i++)
                {
                    GBA_MemoryWrite32(DMA[2].dstaddr,GBA_MemoryRead32(DMA[2].srcaddr));
                    DMA[2].srcaddr += DMA[2].srcadd; DMA[2].dstaddr += DMA[2].dstadd;
                }
            }
            else //copy halfwords
            {
                int i;
                for(i = 0; i < DMA[2].num_chunks; i++)
                {
                    GBA_MemoryWrite16(DMA[2].dstaddr,GBA_MemoryRead16(DMA[2].srcaddr));
                    DMA[2].srcaddr += DMA[2].srcadd; DMA[2].dstaddr += DMA[2].dstadd;
                }
            }
        }
    }

    DMA[2].clocksremaining -= clocks;

    if(DMA[2].clocksremaining <= 0)
    {
        gba_dma_extra_clocks_elapsed = -DMA[2].clocksremaining;

        if(DMA[2].dst_reload) DMA[2].dstaddr = REG_DMA0DAD & (DMA[2].copywords ? ~3 : ~1);

        if(REG_DMA2CNT_H & BIT(14)) GBA_CallInterrupt(BIT(10)); //interrupt

        if(DMA[2].repeat == 0)
        {
            REG_DMA2CNT_H &= ~BIT(15);
            DMA[2].enabled = 0;
        }
        else
        {
            DMA[2].clocksremaining = DMA[2].clockstotal;

            if(DMA[2].starttime == START_NOW)
            {
                //Debug_DebugMsgArg("DMA 2 immediate mode with repeat bit set.");
                //GBA_ExecutionBreak();
                REG_DMA2CNT_H &= ~BIT(15);
                DMA[2].enabled = 0;
                return 0x7FFFFFFF;
            }
        }

        return 0x7FFFFFFF;
    }

    gba_dmaworking = 1;
    return DMA[2].clocksremaining;
    /*
    if(DMA[2].enabled == 0) return 0x7FFFFFFF;

    if(DMA[2].starttime != 3) { MessageBox(NULL, "DMA 2 enabled.", "EMULATION", MB_OK); GBA_ExecutionBreak(); }
    REG_DMA2CNT_H &= ~BIT(15);
    DMA[2].enabled = 0;

    return 0x7FFFFFFF;
    */
}

static inline s32 GBA_DMA3Update(s32 clocks) //return clocks to finish transfer
{
    if(DMA[3].enabled == 0) return 0x7FFFFFFF;

    if(DMA[3].starttime == START_SPECIAL)
    {
        Debug_DebugMsgArg("DMA3, MODE: START_SPECIAL -- NOT EMULATED");
        GBA_ExecutionBreak();
        DMA[3].enabled = 0;
        return 0x7FFFFFFF;
    }

    if(DMA[3].clocksremaining == DMA[3].clockstotal)
    {
        u32 copy = 0;

        switch(DMA[3].starttime)
        {
            case START_NOW:
            {
                copy = 1;
                break;
            }
            case START_VBL:
            {
                if(screenmode != SCR_VBL) return 0x7FFFFFFF;
                if(GBA_ScreenJustChangedMode() == 0) return 0x7FFFFFFF;
                copy = 1;
                break;
            }
            case START_HBL:
            {
                if(screenmode != SCR_HBL) return 0x7FFFFFFF;
                if(GBA_ScreenJustChangedMode() == 0) return 0x7FFFFFFF;
                copy = 1;
                break;
            }
            case START_SPECIAL: return 0x7FFFFFFF; // TODO: NOT EMULATED *****************************
            default: break;
        }

        if(copy)
        {
            //MessageBox(NULL, "DMA 3 copy", "EMULATION", MB_OK);
            //GBA_ExecutionBreak();

            //char text[64]; snprintf(text,sizeof(text),"DMA3\nSRC: %08X\nDST: %08X\nCHUNCKS: %08X",
            //    DMA[3].srcaddr,DMA[3].dstaddr,DMA[3].num_chunks);
            //MessageBox(NULL, text, "EMULATION", MB_OK);
            //GBA_ExecutionBreak();

            if(DMA[3].copywords) //copy words
            {
                int i;
                for(i = 0; i < DMA[3].num_chunks; i++)
                {
                    GBA_MemoryWrite32(DMA[3].dstaddr,GBA_MemoryRead32(DMA[3].srcaddr));
                    DMA[3].srcaddr += DMA[3].srcadd; DMA[3].dstaddr += DMA[3].dstadd;
                }
            }
            else //copy halfwords
            {
                int i;
                for(i = 0; i < DMA[3].num_chunks; i++)
                {
                    GBA_MemoryWrite16(DMA[3].dstaddr,GBA_MemoryRead16(DMA[3].srcaddr));
                    DMA[3].srcaddr += DMA[3].srcadd; DMA[3].dstaddr += DMA[3].dstadd;
                }
            }
        }
    }

    DMA[3].clocksremaining -= clocks;

    if(DMA[3].clocksremaining <= 0)
    {
        gba_dma_extra_clocks_elapsed = -DMA[3].clocksremaining;

        if(DMA[3].dst_reload) DMA[3].dstaddr = REG_DMA3DAD & (DMA[3].copywords ? ~3 : ~1);

        if(REG_DMA3CNT_H & BIT(14)) GBA_CallInterrupt(BIT(11)); //interrupt

        if(DMA[3].repeat == 0)
        {
            REG_DMA3CNT_H &= ~BIT(15);
            DMA[3].enabled = 0;
            //MessageBox(NULL, "DMA 3 clear", "EMULATION", MB_OK);
            //GBA_ExecutionBreak();
        }
        else
        {
            DMA[3].clocksremaining = DMA[3].clockstotal;

            if(DMA[3].starttime == START_NOW)
            {
                //Debug_DebugMsgArg("WARNING: DMA 3, immediate mode with repeat bit set.");
                //GBA_ExecutionBreak();
                REG_DMA3CNT_H &= ~BIT(15);
                DMA[3].enabled = 0;
                return 0x7FFFFFFF;
            }
        }

        return 0x7FFFFFFF;
    }

    gba_dmaworking = 1;
    return DMA[3].clocksremaining;
}

//--------------------------------------------------------------------------

s32 GBA_DMAUpdate(s32 clocks)
{
    gba_dma_extra_clocks_elapsed = 0;
    gba_dmaworking = 0;

    s32 tempclocks;

    if(DMA[0].enabled)
    {
        tempclocks = GBA_DMA0Update(clocks);
        if(gba_dmaworking) return tempclocks;
    }

    if(DMA[1].enabled)
    {
        tempclocks = GBA_DMA1Update(clocks);
        if(gba_dmaworking) return tempclocks;
    }

    if(DMA[2].enabled)
    {
        tempclocks = GBA_DMA2Update(clocks);
        if(gba_dmaworking) return tempclocks;
    }

    //if(DMA[3].enabled) tempclocks = GBA_DMA3Update(clocks);
    //if(gba_dmaworking) return tempclocks;
    if(DMA[3].enabled)
    {
        tempclocks = GBA_DMA3Update(clocks);
        if(gba_dmaworking) return tempclocks;
    }

    return 0x7FFFFFFF;
}

inline int GBA_DMAisWorking(void)
{
    return gba_dmaworking;
}

inline int gba_dma3numchunks(void)
{
    return DMA[3].num_chunks;
}

inline int gba_dma3enabled(void)
{
    return DMA[3].enabled;
}

inline s32 GBA_DMAGetExtraClocksElapsed(void)
{
    return gba_dma_extra_clocks_elapsed;
}

//---------------------------------------------------------

void GBA_DMASoundRequestData(int A, int B)
{
    //this should check if another dma is running and return without copying

    if(DMA[1].starttime == START_SPECIAL)
    {
        if( (A && (DMA[1].dstaddr==FIFO_A)) || (B && (DMA[1].dstaddr==FIFO_B)) )
        {
            //copy words
            int i; for(i = 0; i < 4; i++)
            {
            //    if(GBA_MemoryRead32(DMA[1].srcaddr)!=0) //Debug_DebugMsgArg("%08X",DMA[1].srcaddr);
            //    //Debug_DebugMsgArg("[%08x]= %08X",DMA[1].srcaddr,GBA_MemoryRead32(DMA[1].srcaddr));
            //    Debug_DebugMsgArg("%08X",GBA_MemoryRead32(DMA[1].srcaddr));
                GBA_MemoryWrite32(DMA[1].dstaddr,GBA_MemoryRead32(DMA[1].srcaddr));
                DMA[1].srcaddr += DMA[1].srcadd;
            }
            if(REG_DMA2CNT_H & BIT(14)) GBA_CallInterrupt(BIT(9));
        }
    }

    if(DMA[2].starttime == START_SPECIAL)
    {
        if( (A && (DMA[2].dstaddr==FIFO_A)) || (B && (DMA[2].dstaddr==FIFO_B)) )
        {
            //copy words
            int i; for(i = 0; i < 4; i++)
            {
                GBA_MemoryWrite32(DMA[2].dstaddr,GBA_MemoryRead32(DMA[2].srcaddr));
                DMA[2].srcaddr += DMA[2].srcadd;
            }
            if(REG_DMA2CNT_H & BIT(14)) GBA_CallInterrupt(BIT(10));
        }
    }
}

//---------------------------------------------------------

