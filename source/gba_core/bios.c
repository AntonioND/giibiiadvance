// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../debug_utils.h"

#include "cpu.h"
#include "dma.h"
#include "memory.h"
#include "sound.h"

//------------------------------------------------------------------------------

static int gba_bios_loaded_from_file;

void GBA_BiosLoaded(int loaded)
{
    gba_bios_loaded_from_file = loaded;
}

int GBA_BiosIsLoaded(void)
{
    return gba_bios_loaded_from_file;
}

//------------------------------------------------------------------------------

void GBA_BiosEmulatedLoad(void)
{
    const unsigned int gba_bios[] = {
        0xEA000006, 0xEA000005, 0xEA00001A, 0xEA000003, 0xEA000002, 0xEA000001,
        0xEA000010, 0xEAFFFFFF, 0xE3A000D2, 0xE129F000, 0xE3A0D403, 0xE38DDC7F,
        0xE38DD0A0, 0xE3A000D3, 0xE129F000, 0xE3A0D403, 0xE38DDC7F, 0xE38DD0E0,
        0xE3A0001F, 0xE129F000, 0xE3A0D403, 0xE38DDC7F, 0xE3A0E302, 0xE1A0F00E,
        0xE92D500F, 0xE3A00301, 0xE28FE000, 0xE510F004, 0xE8BD500F, 0xE25EF004,
        0xE92D5800, 0xE14FB000, 0xE92D0800, 0xE20BB080, 0xE38BB01F, 0xE129F00B,
        0xE92D4004, 0xEB000006, 0xE8BD4004, 0xE3A0C0D3, 0xE129F00C, 0xE8BD0800,
        0xE169F00B, 0xE8BD5800, 0xE1B0F00E, 0xE92D4010, 0xE3A03000, 0xE3A04001,
        0xE3500000, 0x1B000004, 0xE5CC3301, 0xEB000002, 0x0AFFFFFC, 0xE8BD4010,
        0xE12FFF1E, 0xE3A0C301, 0xE5CC3208, 0xE15C20B8, 0xE0110002, 0x10222000,
        0x114C20B8, 0xE5CC4208, 0xE12FFF1E
    };

    memcpy(Mem.rom_bios, gba_bios, sizeof(gba_bios));
}

//------------------------------------------------------------------------------

static void GBA_SWI_SoftReset(void)
{
    u8 ret_flag = GBA_MemoryRead8(0x3007FFA);

    memset(&(Mem.iwram[0x03007E00 - 0x03000000]), 0, 0x200);
    memset(&CPU, 0, sizeof(CPU));

    CPU.EXECUTION_MODE = EXEC_ARM;
    CPU.MODE = CPU_SYSTEM;
    CPU.CPSR = M_SYSTEM;
    //GBA_CPUChangeMode(M_SYSTEM);

    CPU.R[R_SP] = 0x03007F00;
    CPU.R_user[13 - 8] = 0x03007F00;

    CPU.R13_svc = 0x03007FE0;
    CPU.R13_irq = 0x03007FA0;

    if (ret_flag)
    {
        CPU.R[R_LR] = 0x02000000;
        CPU.R[R_PC] = 0x02000000 - 4; // Don't skip an instruction
    }
    else
    {
        CPU.R[R_LR] = 0x08000000;
        CPU.R[R_PC] = 0x08000000 - 4; // Don't skip an instruction
    }
}

static void GBA_SWI_RegisterRamReset(void)
{
    u32 r0 = CPU.R[0];

    GBA_MemoryWrite16(SIODATA32, 0); // Always destroyed

    if (r0 & BIT(0)) // 256K on-board WRAM
    {
        memset(Mem.ewram, 0, sizeof(Mem.ewram));
    }
    if (r0 & BIT(1)) // 32K in-chip WRAM -- excluding last 200h bytes
    {
        // The range excluded is 3007E00h - 3007FFFh
        memset(Mem.iwram, 0, sizeof(Mem.iwram) - 0x200);
    }
    if (r0 & BIT(2)) // Palette
    {
        memset(Mem.pal_ram, 0, sizeof(Mem.pal_ram));
    }
    if (r0 & BIT(3)) // VRAM
    {
        memset(Mem.vram, 0, sizeof(Mem.vram));
    }
    if (r0 & BIT(4)) // OAM
    {
        memset(Mem.oam, 0, sizeof(Mem.oam));
    }
    if (r0 & BIT(5)) // Reset SIO registers
    {
        GBA_RegisterWrite32(SIODATA32, 0);
        GBA_RegisterWrite16(SIOMULTI2, 0);
        GBA_RegisterWrite16(SIOMULTI3, 0);
        GBA_RegisterWrite16(SIOCNT, 0);
        GBA_RegisterWrite16(SIOMLT_SEND, 0);

        GBA_RegisterWrite16(RCNT, 0);

        GBA_RegisterWrite16(JOYCNT, 0);
        GBA_RegisterWrite32(JOY_RECV, 0);
        GBA_RegisterWrite32(JOY_TRANS, 0);
        GBA_RegisterWrite16(JOYSTAT, 0);
    }
    if (r0 & BIT(6)) // Reset Sound registers
    {
        GBA_SoundInit();
    }
    if (r0 & BIT(7)) // Reset all other registers (except SIO, Sound)
    {
        GBA_RegisterWrite16(DISPCNT, 0);
        GBA_RegisterWrite16(GREENSWAP, 0);
        GBA_RegisterWrite16(DISPSTAT, 0);
        GBA_RegisterWrite16(VCOUNT, 0);
        GBA_RegisterWrite16(BG0CNT, 0);
        GBA_RegisterWrite16(BG1CNT, 0);
        GBA_RegisterWrite16(BG2CNT, 0);
        GBA_RegisterWrite16(BG3CNT, 0);
        GBA_RegisterWrite16(BG0HOFS, 0);
        GBA_RegisterWrite16(BG0VOFS, 0);
        GBA_RegisterWrite16(BG1HOFS, 0);
        GBA_RegisterWrite16(BG1VOFS, 0);
        GBA_RegisterWrite16(BG2HOFS, 0);
        GBA_RegisterWrite16(BG2VOFS, 0);
        GBA_RegisterWrite16(BG3HOFS, 0);
        GBA_RegisterWrite16(BG3VOFS, 0);
        GBA_RegisterWrite16(BG2PA, 1 << 8);
        GBA_RegisterWrite16(BG2PB, 0);
        GBA_RegisterWrite16(BG2PC, 0);
        GBA_RegisterWrite16(BG2PD, 1 << 8);
        GBA_RegisterWrite32(BG2X_L, 0);
        GBA_RegisterWrite32(BG2Y_L, 0);
        GBA_RegisterWrite16(BG3PA, 1 << 8);
        GBA_RegisterWrite16(BG3PB, 0);
        GBA_RegisterWrite16(BG3PC, 0);
        GBA_RegisterWrite16(BG3PD, 1 << 8);
        GBA_RegisterWrite32(BG3X_L, 0);
        GBA_RegisterWrite32(BG3Y_L, 0);
        GBA_RegisterWrite16(WIN0H, 0);
        GBA_RegisterWrite16(WIN1H, 0);
        GBA_RegisterWrite16(WIN0V, 0);
        GBA_RegisterWrite16(WIN1V, 0);
        GBA_RegisterWrite16(WININ, 0);
        GBA_RegisterWrite16(WINOUT, 0);
        GBA_RegisterWrite16(MOSAIC, 0);
        GBA_RegisterWrite16(BLDCNT, 0);
        GBA_RegisterWrite16(BLDALPHA, 0);
        GBA_RegisterWrite16(BLDY, 0);
        GBA_RegisterWrite32(DMA0SAD, 0);
        GBA_RegisterWrite32(DMA0DAD, 0);
        GBA_RegisterWrite32(DMA0CNT_L, 0);
        GBA_RegisterWrite32(DMA1SAD, 0);
        GBA_RegisterWrite32(DMA1DAD, 0);
        GBA_RegisterWrite32(DMA1CNT_L, 0);
        GBA_RegisterWrite32(DMA2SAD, 0);
        GBA_RegisterWrite32(DMA2DAD, 0);
        GBA_RegisterWrite32(DMA2CNT_L, 0);
        GBA_RegisterWrite32(DMA3SAD, 0);
        GBA_RegisterWrite32(DMA3DAD, 0);
        GBA_RegisterWrite32(DMA3CNT_L, 0);
        GBA_RegisterWrite32(TM0CNT_L, 0);
        GBA_RegisterWrite32(TM1CNT_L, 0);
        GBA_RegisterWrite32(TM2CNT_L, 0);
        GBA_RegisterWrite32(TM3CNT_L, 0);
        GBA_RegisterWrite16(KEYINPUT, 0);
        GBA_RegisterWrite16(KEYCNT, 0);
        GBA_RegisterWrite16(IE, 0);
        GBA_RegisterWrite16(IF, 0);
        GBA_RegisterWrite16(WAITCNT, 0);
        GBA_RegisterWrite16(IME, 0);
    }

    GBA_MemoryWrite16(DISPCNT, 0x0080);
}

static void GBA_SWI_CpuSet(void)
{
    int count = CPU.R[2] & 0x001FFFFF;
    CPU.R[2] &= ~0x001FFFFF;

    if (CPU.R[2] & BIT(26)) // 32 bit
    {
        CPU.R[0] &= ~3;
        CPU.R[1] &= ~3;

        if (CPU.R[2] & BIT(24)) // Fill
        {
            u32 fill = GBA_MemoryRead32(CPU.R[0]);
            while (count--)
            {
                GBA_MemoryWrite32(CPU.R[1], fill);
                CPU.R[1] += 4;
            }
        }
        else // Copy
        {
            while (count--)
            {
                GBA_MemoryWrite32(CPU.R[1], GBA_MemoryRead32(CPU.R[0]));
                CPU.R[1] += 4;
                CPU.R[0] += 4;
            }
        }
    }
    else // 16 bit
    {
        CPU.R[0] &= ~1;
        CPU.R[1] &= ~1;

        if (CPU.R[2] & BIT(24)) // Fill
        {
            u16 fill = GBA_MemoryRead16(CPU.R[0]);
            while (count--)
            {
                GBA_MemoryWrite16(CPU.R[1], fill);
                CPU.R[1] += 2;
            }
        }
        else // Copy
        {
            while (count--)
            {
                GBA_MemoryWrite16(CPU.R[1], GBA_MemoryRead16(CPU.R[0]));
                CPU.R[1] += 2;
                CPU.R[0] += 2;
            }
        }
    }
}

static void GBA_SWI_CpuFastSet(void)
{
    int count = CPU.R[2] & 0x001FFFF8; // Must be a multiple of 8 words
    CPU.R[2] &= ~0x001FFFFF;

    CPU.R[0] &= ~3;
    CPU.R[1] &= ~3;

    if (CPU.R[2] & BIT(24)) // Fill
    {
        u32 fill = GBA_MemoryRead32(CPU.R[0]);
        while (count--)
        {
            GBA_MemoryWrite32(CPU.R[1], fill);
            CPU.R[1] += 4;
        }
    }
    else // Copy
    {
        while (count--)
        {
            GBA_MemoryWrite32(CPU.R[1], GBA_MemoryRead32(CPU.R[0]));
            CPU.R[1] += 4;
            CPU.R[0] += 4;
        }
    }
}

static void GBA_SWI_BgAffineSet(void)
{
    int count = CPU.R[2];
    int src = CPU.R[0];
    int dst = CPU.R[1];

    while (count--)
    {
        s32 cx = GBA_MemoryRead32(src);
        src += 4;
        s32 cy = GBA_MemoryRead32(src);
        src += 4;
        float dispx = (float)(s16)GBA_MemoryRead16(src);
        src += 2;
        float dispy = (float)(s16)GBA_MemoryRead16(src);
        src += 2;
        float sx = ((float)(s16)GBA_MemoryRead16(src)) / (float)(1 << 8);
        src += 2;
        float sy = ((float)(s16)GBA_MemoryRead16(src)) / (float)(1 << 8);
        src += 2;
        float angle = (float)2.0 * (float)M_PI
                      * (((float)(u8)(GBA_MemoryRead16(src) >> 8)) / (float)0xFF);
        src += 4; // Align

        float sin_ = sin(angle);
        float cos_ = cos(angle);
        s16 A = (s16)(cos_ * (float)(1 << 8) * sx);
        s16 B = (s16)(-sin_ * (float)(1 << 8) * sx);
        s16 C = (s16)(sin_ * (float)(1 << 8) * sy);
        s16 D = (s16)(cos_ * (float)(1 << 8) * sy);

        GBA_MemoryWrite16(dst, A);
        dst += 2;
        GBA_MemoryWrite16(dst, B);
        dst += 2;
        GBA_MemoryWrite16(dst, C);
        dst += 2;
        GBA_MemoryWrite16(dst, D);
        dst += 2;

        s32 startx = cx - (s32)((cos_ * sx * dispx - sin_ * sx * dispy)
                   * (float)(1 << 8));
        s32 starty = cy - (s32)((sin_ * sy * dispx + cos_ * sy * dispy)
                   * (float)(1 << 8));

        GBA_MemoryWrite32(dst, startx);
        dst += 4;
        GBA_MemoryWrite32(dst, starty);
        dst += 4;
    }
}

static void GBA_SWI_ObjAffineSet(void)
{
    int addressadd = CPU.R[3];
    int count = CPU.R[2];
    int src = CPU.R[0];
    int dst = CPU.R[1];

    while (count--)
    {
        float sx = ((float)(s16)GBA_MemoryRead16(src)) / (float)(1 << 8);
        src += 2;
        float sy = ((float)(s16)GBA_MemoryRead16(src)) / (float)(1 << 8);
        src += 2;
        float angle = (float)2.0 * (float)M_PI
                      * (((float)(u8)(GBA_MemoryRead16(src) >> 8)) / (float)0xFF);
        src += 4; // Align

        float sin_ = sin(angle);
        float cos_ = cos(angle);
        s16 A = (s16)(cos_ * (float)(1 << 8) * sx);
        s16 B = (s16)(-sin_ * (float)(1 << 8) * sx);
        s16 C = (s16)(sin_ * (float)(1 << 8) * sy);
        s16 D = (s16)(cos_ * (float)(1 << 8) * sy);

        GBA_MemoryWrite16(dst, A);
        dst += addressadd;
        GBA_MemoryWrite16(dst, B);
        dst += addressadd;
        GBA_MemoryWrite16(dst, C);
        dst += addressadd;
        GBA_MemoryWrite16(dst, D);
        dst += addressadd;
    }
}

static void GBA_SWI_BitUnPack(void)
{
    int src = CPU.R[0];
    int dst = CPU.R[1] & ~3;
    int infoaddr = CPU.R[2] & ~3; // Aligned to 32 bit?

    int srcsize = (int)(u32)(u16)GBA_MemoryRead16(infoaddr);
    infoaddr += 2;
    u8 srcwidth = GBA_MemoryRead8(infoaddr); // (only 1,2,4,8 supported)
    infoaddr++;
    u8 dstwidth = GBA_MemoryRead8(infoaddr); // (only 1,2,4,8,16,32 supported)
    infoaddr++;
    u32 dataoffset = GBA_MemoryRead32(infoaddr);
    int zerodataflag = dataoffset & BIT(31);
    dataoffset &= ~BIT(31);

    int src_bitindex = 0, dst_bitindex = 0;
    u8 srcdata = 0;
    u32 dstdata = 0;

    while (srcsize > 0)
    {
        u32 data = 0;

        if (src_bitindex == 0)
        {
            srcdata = GBA_MemoryRead8(src);
            src++;
            srcsize--;
        }
        switch (srcwidth)
        {
            case 1:
                data = (srcdata & (1 << src_bitindex)) >> src_bitindex;
                src_bitindex++;
                break;
            case 2:
                data = (srcdata & (3 << src_bitindex)) >> src_bitindex;
                src_bitindex += 2;
                break;
            case 4:
                data = (srcdata & (0xF << src_bitindex)) >> src_bitindex;
                src_bitindex += 4;
                break;
            case 8:
                data = srcdata;
                src_bitindex += 8;
                break;
            default:
                Debug_DebugMsgArg("SWI 0x10: Invalid src width");
                GBA_ExecutionBreak();
                return;
        }
        if (src_bitindex == 8)
            src_bitindex = 0;

        if (data)
            data += dataoffset;
        else if (zerodataflag)
            data += dataoffset;

        if (dst_bitindex == 0)
            dstdata = 0;

        switch (dstwidth)
        {
            case 1:
            case 2:
            case 4:
            case 8:
            case 16:
            case 32:
                dstdata |= data << dst_bitindex;
                dst_bitindex += dstwidth;
                break;
            default:
                Debug_DebugMsgArg("SWI 0x10: Invalid dst width");
                GBA_ExecutionBreak();
                return;
        }
        if (dst_bitindex == 32)
        {
            GBA_MemoryWrite32(dst, dstdata);
            dst += 4;
            dst_bitindex = 0;
        }
    }
}

static void GBA_SWI_LZ77UnCompWram(void)
{
    int src = CPU.R[0];
    int dst = CPU.R[1];
    u32 header = GBA_MemoryRead32(src);
    src += 4;
    //if (((header >> 4) & 7) != 1)
    //{
    //    Debug_DebugMsgArg("SWI 11 -- Compression type %d -- ABORTED",
    //                      ((header >> 4) & 7));
    //    return;
    //}
    u32 size = (header >> 8) & 0x00FFFFFF;
    u8 *buffer = malloc(size + 2);
    if (buffer == NULL)
    {
        Debug_ErrorMsgArg("SWI 11 - Couldn't allocate memory");
        GBA_ExecutionBreak();
        return;
    }
    u8 *buffertmp = buffer;
    u32 total = 0;
    while (size > total)
    {
        u8 flag = GBA_MemoryRead8(src++);
        for (int i = 0; i < 8; i++)
        {
            if (flag & 0x80)
            {
                // Compressed - Copy N+3 Bytes from Dest-Disp-1 to Dest
                u16 info = ((u16)GBA_MemoryRead8(src++)) << 8;
                info |= (u16)GBA_MemoryRead8(src++);
                u32 displacement = (info & 0x0FFF);
                int num = 3 + ((info >> 12) & 0xF);
                u32 offset = total - displacement - 1;
                if (offset > total) // This also checks for negative values
                {
                    Debug_ErrorMsgArg("SWI 11 - Error while decoding");
                    GBA_ExecutionBreak();
                    free(buffer);
                    return;
                }
                while (num--)
                {
                    *buffertmp++ = ((u8 *)buffer)[offset++];
                    total++;
                    if (size <= total)
                        break;
                }
            }
            else
            {
                // Uncompressed - Copy 1 Byte from Source to Dest
                *buffertmp++ = GBA_MemoryRead8(src++);
                total++;
                if (size <= total)
                    break;
            }
            flag <<= 1;
        }
    }
    // Copy to VRAM in 16 bit blocks
    u8 *dstbuf = buffer;
    while (size--)
        GBA_MemoryWrite8(dst++, *dstbuf++);
    free(buffer);
}

static void GBA_SWI_LZ77UnCompVram(void)
{
    int src = CPU.R[0];
    int dst = CPU.R[1];

    u32 header = GBA_MemoryRead32(src);
    src += 4;
    //if (((header >> 4) & 7) != 1)
    //{
    //    Debug_DebugMsgArg("SWI 12 -- Compression type %d -- ABORTED",
    //                      ((header >> 4) & 7));
    //    return;
    //}
    u32 size = (header >> 8) & 0x00FFFFFF;
    u16 *buffer = malloc(size + 2);
    if (buffer == NULL)
    {
        Debug_ErrorMsgArg("SWI 12 - Couldn't allocate memory");
        GBA_ExecutionBreak();
        return;
    }
    u8 *buffertmp = (u8 *)buffer;
    u32 total = 0;
    while (size > total)
    {
        u8 flag = GBA_MemoryRead8(src++);
        for (int i = 0; i < 8; i++)
        {
            if (flag & 0x80)
            {
                // Compressed - Copy N+3 Bytes from Dest-Disp-1 to Dest
                u16 info = ((u16)GBA_MemoryRead8(src++)) << 8;
                info |= (u16)GBA_MemoryRead8(src++);
                u32 displacement = (info & 0x0FFF);
                int num = 3 + ((info >> 12) & 0xF);
                u32 offset = total - displacement - 1;
                if (offset > total) // This also checks for negative values
                {
                    Debug_ErrorMsgArg("SWI 12 - Error while decoding");
                    GBA_ExecutionBreak();
                    free(buffer);
                    return;
                }
                while (num--)
                {
                    *buffertmp++ = ((u8 *)buffer)[offset++];
                    total++;
                    if (size <= total)
                        break;
                }
            }
            else
            {
                // Uncompressed - Copy 1 Byte from Source to Dest
                *buffertmp++ = GBA_MemoryRead8(src++);
                total++;
                if (size <= total)
                    break;
            }
            flag <<= 1;
        }
    }
    // Copy to VRAM in 16 bit blocks
    u16 *dstbuf = buffer;
    while (size > 0)
    {
        GBA_MemoryWrite16(dst, *dstbuf++);
        dst += 2;
        size -= 2;
    }
    free(buffer);
}

static void GBA_SWI_HuffUnComp(void)
{
    int src = CPU.R[0] & ~3;
    int dst = CPU.R[1];
    u32 header = GBA_MemoryRead32(src);
    src += 4;
    //if (((header >> 4) & 7) != 2)
    //{
    //    Debug_DebugMsgArg("SWI 13 -- Compression type %d -- ABORTED",
    //                      ((header >> 4) & 7));
    //    return;
    //}
    int chunk_size = header & 0xF; // In bits
    if ((chunk_size != 4) && (chunk_size != 8))
    {
        Debug_ErrorMsgArg("SWI 0x13 - Data size error: %d", chunk_size);
        GBA_ExecutionBreak();
        return;
    }
    int size = (header >> 8) & 0x00FFFFFF;
    u32 *buffer = malloc(size + 2);
    if (buffer == NULL)
    {
        Debug_ErrorMsgArg("SWI 13 - Couldn't allocate memory");
        GBA_ExecutionBreak();
        return;
    }
    u8 *buffertmp = (u8 *)buffer;

    u32 treesize = (((u32)GBA_MemoryRead8(src++)) * 2) + 1;
    u32 treetable = src;
    src += treesize; // Bitstream
    int total = 0;
    int bit4index = 0;
    int bitsleft = 0;
    u32 bitstream = 0;

    while (1)
    {
        int searching = 1;
        u32 nodeaddr = treetable;
        while (searching)
        {
            if (bitsleft == 0)
            {
                bitstream = GBA_MemoryRead32(src);
                src += 4;
                bitsleft = 32;
            }
            int node = bitstream >> 31; // Get bit 31
            //Debug_DebugMsgArg("Node %d", node);
            bitstream <<= 1;
            bitsleft--;
            u8 nodeinfo = GBA_MemoryRead8(nodeaddr);
            if (node && (nodeinfo & BIT(6)))
                searching = 0;
            if ((node == 0) && (nodeinfo & BIT(7)))
                searching = 0;
            nodeaddr = (nodeaddr & ~1)
                       + ((int)(nodeinfo & 0x3F)) * 2 + 2 + node;
        }
        //Debug_DebugMsgArg("Data: %02X", GBA_MemoryRead8(nodeaddr));
        if (chunk_size == 8)
        {
            *buffertmp++ = GBA_MemoryRead8(nodeaddr);
            total++;
        }
        else //if (chunk_size == 4)
        {
            if (bit4index & 1)
            {
                *buffertmp |= GBA_MemoryRead8(nodeaddr) << 4;
                buffertmp++;
                total++;
            }
            else
            {
                *buffertmp = GBA_MemoryRead8(nodeaddr);
            }
            bit4index ^= 1;
        }
        if (size <= total)
            break;
    }
    // Copy in 32 bit blocks
    u32 *dstbuf = buffer;
    while (size > 0)
    {
        GBA_MemoryWrite32(dst, *dstbuf++);
        dst += 4;
        size -= 4;
    }
    free(buffer);
}

static void GBA_SWI_RLUnCompWram(void)
{
    int src = CPU.R[0];
    int dst = CPU.R[1];
    u32 header = GBA_MemoryRead32(src);
    src += 4;
    //if (((header >> 4) & 7) != 3)
    //{
    //    Debug_DebugMsgArg("SWI 14 -- Compression type %d -- ABORTED",
    //                      ((header >> 4) & 7));
    //    return;
    //}
    int size = (header >> 8) & 0x00FFFFFF;
    while (size > 0)
    {
        u8 flg = GBA_MemoryRead8(src);
        src++;
        if (flg & BIT(7)) // Compressed - 1 byte repeated N times
        {
            int len = (flg & 0x7F) + 3;
            u8 data = GBA_MemoryRead8(src);
            src++;
            while (len)
            {
                GBA_MemoryWrite8(dst, data);
                dst++;
                size--;
                len--;
            }
        }
        else // N uncompressed bytes
        {
            int len = (flg & 0x7F) + 1;
            while (len)
            {
                GBA_MemoryWrite8(dst, GBA_MemoryRead8(src));
                src++;
                dst++;
                size--;
                len--;
            }
        }
    }
}

static void GBA_SWI_RLUnCompVram(void)
{
    int src = CPU.R[0];
    int dst = CPU.R[1];
    u32 header = GBA_MemoryRead32(src);
    src += 4;
    //if (((header >> 4) & 7) != 3)
    //{
    //    Debug_DebugMsgArg("SWI 15 -- Compression type %d -- ABORTED",
    //                      ((header >> 4) & 7));
    //    return;
    //}
    int size = (header >> 8) & 0x00FFFFFF;

    u16 writedata = 0;
    int curbyte = 0;

    while (size > 0)
    {
        u8 flg = GBA_MemoryRead8(src);
        src++;
        if (flg & BIT(7)) // Compressed - 1 byte repeated N times
        {
            int len = (flg & 0x7F) + 3;
            u8 data = GBA_MemoryRead8(src);
            src++;
            while (len)
            {
                if (curbyte & 1)
                {
                    writedata |= ((u16)data) << 8;
                    GBA_MemoryWrite16(dst, writedata);
                    dst += 2;
                    writedata = 0;
                }
                else
                {
                    writedata = (u16)data;
                }
                curbyte ^= 1;
                size--;
                len--;
            }
        }
        else // N uncompressed bytes
        {
            int len = (flg & 0x7F) + 1;
            while (len)
            {
                if (curbyte & 1)
                {
                    writedata |= ((u16)GBA_MemoryRead8(src)) << 8;
                    src++;
                    GBA_MemoryWrite16(dst, writedata);
                    dst += 2;
                    writedata = 0;
                }
                else
                {
                    writedata = (u16)GBA_MemoryRead8(src);
                    src++;
                }
                curbyte ^= 1;
                size--;
                len--;
            }
        }
    }
}

static void GBA_SWI_Diff8bitUnFilterWram(void)
{
    u32 src = CPU.R[0];
    u32 dst = CPU.R[1];
    u32 header = GBA_MemoryRead32(src);
    src += 4;
    //if ((header & 3) != 1)
    //{
    //    Debug_DebugMsgArg("SWI 16 -- Compression type != 8 bit -- ABORTED");
    //    return;
    //}
    //if (((header >> 4) & 7) != 8)
    //{
    //    Debug_DebugMsgArg("SWI 16 -- Compression type %d -- ABORTED",
    //                      ((header >> 4) & 7));
    //    return;
    //}
    int size = (header >> 8) & 0x00FFFFFF;
    u8 value = GBA_MemoryRead8(src);
    src++;
    GBA_MemoryWrite8(dst, value);
    dst++;
    size--;
    while (1)
    {
        value += GBA_MemoryRead8(src);
        src++;
        GBA_MemoryWrite8(dst, value);
        dst++;
        size--;
        if (size <= 0)
            break;
    }
}

static void GBA_SWI_Diff8bitUnFilterVram(void)
{
    u32 src = CPU.R[0];
    u32 dst = CPU.R[1];
    u32 header = GBA_MemoryRead32(src);
    src += 4;
    //if ((header & 3) != 1)
    //{
    //    Debug_DebugMsgArg("SWI 17 -- Compression type != 8 bit -- ABORTED");
    //    return;
    //}
    //if (((header >> 4) & 7) != 8)
    //{
    //    Debug_DebugMsgArg("SWI 17 -- Compression type %d -- ABORTED",
    //                      ((header >> 4) & 7));
    //    return;
    //}
    int size = (header >> 8) & 0x00FFFFFF;
    u8 value = 0;
    u16 writeval = 0;
    int bytewrite = 0;
    while (1)
    {
        value += GBA_MemoryRead8(src);
        src++;

        if ((bytewrite & 1) == 0)
        {
            writeval = (u16)value;
        }
        else
        {
            writeval |= ((u16)value) << 8;
            GBA_MemoryWrite16(dst, writeval);
            dst += 2;
            size -= 2;
            if (size <= 0)
                break;
        }
        bytewrite ^= 1;
    }
}

static void GBA_SWI_Diff16bitUnFilter(void)
{
    u32 src = CPU.R[0];
    u32 dst = CPU.R[1];
    u32 header = GBA_MemoryRead32(src);
    src += 4;
    //if ((header & 3) != 2)
    //{
    //    Debug_DebugMsgArg("SWI 18 -- Compression type != 16 bit -- ABORTED");
    //    return;
    //}
    //if (((header >> 4) & 7) != 8)
    //{
    //    Debug_DebugMsgArg("SWI 18 -- Compression type %d -- ABORTED",
    //                      ((header >> 4) & 7));
    //    return;
    //}
    int size = (header >> 8) & 0x00FFFFFF;
    u16 value = GBA_MemoryRead16(src);
    src += 2;
    GBA_MemoryWrite16(dst, value);
    dst += 2;
    size -= 2;
    while (1)
    {
        value += GBA_MemoryRead16(src);
        src += 2;
        GBA_MemoryWrite16(dst, value);
        dst += 2;
        size -= 2;
        if (size <= 0)
            break;
    }
}

//------------------------------------------------------------------------------

void GBA_Swi(u8 number)
{
    switch (number)
    {
        case 0x00: // SoftReset
        {
            GBA_SWI_SoftReset();
            return;
        }
        case 0x01: // RegisterRamReset
        {
            GBA_SWI_RegisterRamReset();
            return;
        }
        case 0x02: // Halt
        {
            GBA_MemoryWrite8(HALTCNT, 0x00);
            return;
        }
        case 0x03: // Stop
        {
            GBA_MemoryWrite8(HALTCNT, 0x80);
            return;
        }
        case 0x04: // IntrWait
        {
            Debug_ErrorMsgArg("GBA_SWI(): SWI 0x04 mustn't get here.");
            return;
        }
        case 0x05: // VBlankIntrWait
        {
            Debug_ErrorMsgArg("GBA_SWI(): SWI 0x05 mustn't get here.");
            return;
        }
        case 0x06: // Div
        {
            if (CPU.R[1] == 0)
            {
                Debug_DebugMsgArg("SWI 0x06: Division by 0");
                return;
            }
            s32 num = (s32)CPU.R[0] / (s32)CPU.R[1];
            s32 mod = (s32)CPU.R[0] % (s32)CPU.R[1];
            CPU.R[0] = num;
            CPU.R[1] = mod;
            CPU.R[3] = abs(num);
            return;
        }
        case 0x07: // DivArm
        {
            if (CPU.R[0] == 0)
            {
                Debug_DebugMsgArg("SWI 0x07: Division by 0");
                return;
            }
            s32 num = (s32)CPU.R[1] / (s32)CPU.R[0];
            s32 mod = (s32)CPU.R[1] % (s32)CPU.R[0];
            CPU.R[0] = num;
            CPU.R[1] = mod;
            CPU.R[3] = abs(num);
            return;
        }
        case 0x08: // Sqrt
        {
            CPU.R[0] = (u16)sqrt((u32)CPU.R[0]);
            return;
        }
        case 0x09: // ArcTan
        {
            // This is what the BIOS does... Not accurate at the bounds.
            s32 r1 = -((((s32)CPU.R[0]) * ((s32)CPU.R[0])) >> 14);
            s32 r3 = ((((s32)0xA9) * r1) >> 14) + 0x390;
            r3 = ((r1 * r3) >> 14) + 0x91C;
            r3 = ((r1 * r3) >> 14) + 0xFB6;
            r3 = ((r1 * r3) >> 14) + 0x16AA;
            r3 = ((r1 * r3) >> 14) + 0x2081;
            r3 = ((r1 * r3) >> 14) + 0x3651;
            r3 = ((r1 * r3) >> 14) + 0xA259;
            CPU.R[0] = (((s32)CPU.R[0]) * r3) >> 16;

            // Emulated: Accurate always
            // r0: Tan, 16bit: 1bit sign, 1bit integral part, 14bit decimal part
            // Return: "-PI/2 < THETA < PI/2" in a range of C000h-4000h.
            // CPU.R[0] = (s32)(atan(((float)(s16)(u16)CPU.R[0])
            //                       / (float)(1 << 14))
            //                  * (((float)0x4000) / ((float)M_PI_2)));
            return;
        }
        case 0x0A: // ArcTan2
        {
            float x = ((float)(s16)CPU.R[0]) / (float)(1 << 14);
            float y = ((float)(s16)CPU.R[1]) / (float)(1 << 14);
            CPU.R[0] = (u32)(u16)(s16)(s32)
                (atan2(y, x) * (((float)0xFFFF) / ((float)2.0 * (float)M_PI)));
            // Return: 0000h-FFFFh for 0 <= THETA < 2PI.
            return;
        }
        case 0x0B: // CpuSet
        {
            GBA_SWI_CpuSet();
            return;
        }
        case 0x0C: // CpuFastSet
        {
            GBA_SWI_CpuFastSet();
            return;
        }
        case 0x0D: // GetBiosChecksum
        {
            CPU.R[0] = 0xBAAE187F; // 0xBAAE1880 DS in GBA mode
            // The only difference is that the byte at [3F0Ch] is changed from
            // 00h to 01h
            return;
        }
        case 0x0E: // BgAffineSet
        {
            GBA_SWI_BgAffineSet();
            return;
        }
        case 0x0F: // ObjAffineSet
        {
            GBA_SWI_ObjAffineSet();
            return;
        }
        case 0x10: // BitUnPack
        {
            GBA_SWI_BitUnPack();
            return;
        }
        case 0x11: // LZ77UnCompWram -- 8 bit
        {
            GBA_SWI_LZ77UnCompWram();
            return;
        }
        case 0x12: // LZ77UnCompVram -- 16 bit
        {
            GBA_SWI_LZ77UnCompVram();
            return;
        }
        case 0x13: // HuffUnComp
        {
            GBA_SWI_HuffUnComp();
            return;
        }
        case 0x14: // RLUnCompWram -- 8 bit
        {
            GBA_SWI_RLUnCompWram();
            return;
        }
        case 0x15: // RLUnCompVram -- 16 bit
        {
            GBA_SWI_RLUnCompVram();
            return;
        }
        case 0x16: // Diff8bitUnFilterWram
        {
            GBA_SWI_Diff8bitUnFilterWram();
            return;
        }
        case 0x17: // Diff8bitUnFilterVram
        {
            GBA_SWI_Diff8bitUnFilterVram();
            return;
        }
        case 0x18: // Diff16bitUnFilter
        {
            GBA_SWI_Diff16bitUnFilter();
            return;
        }
        case 0x19: // SoundBias
        {
            int level = CPU.R[0] ? 0x200 : 0;
            // Ignore delay
            GBA_MemoryWrite16(SOUNDBIAS, (REG_SOUNDBIAS & 0xFC00) | level);
            return;
        }
        case 0x1A: // SoundDriverInit
        case 0x1B: // SoundDriverMode
        case 0x1C: // SoundDriverMain
        case 0x1D: // SoundDriverVSync
        case 0x1E: // SoundChannelClear
        case 0x1F: // MidiKey2Freq
        case 0x20: // |
        case 0x21: // |
        case 0x22: // |- SoundWhatever0..4
        case 0x23: // |
        case 0x24: // |
        case 0x25: // MultiBoot
            break;
        case 0x26: // HardReset
        {
            GBA_Swi(0);
            CPU.R[0] = 0xFF;
            GBA_Swi(1);
            CPU.R[R_LR] = 0x00000000;
            CPU.R[R_PC] = 0x00000000; // Enough...
            GBA_RegisterWrite16(TM0CNT_L, 0xFF6F);
            return;
        }
        case 0x27: // CustomHalt
        {
            GBA_MemoryWrite8(HALTCNT, CPU.R[2] & 0xFF);
            return;
        }
        case 0x28: // SoundDriverVSyncOff
        case 0x29: // SoundDriverVSyncOn
        case 0x2A: // SoundGetJumpList
            break;

        default:
            break;
    }

    Debug_ErrorMsgArg(
            "SWI 0x%02X -- Not emulated!!\n"
            "Please, get a GBA BIOS file, name it \"" GBA_BIOS_FILENAME
            "\" and place it in the \"bios\" folder.",
            number);
    GBA_ExecutionBreak();
}
