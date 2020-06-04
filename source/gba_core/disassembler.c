// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdio.h>
#include <string.h>

#include "../build_options.h"
#include "../font_utils.h"

#include "cpu.h"
#include "gba.h"
#include "memory.h"
#include "shifts.h"

//------------------------------------------------------------------------------

#define GBA_MAX_BREAKPOINTS 20

static u32 gba_brkpoint_addrlist[GBA_MAX_BREAKPOINTS];
static int gba_brkpoint_used[GBA_MAX_BREAKPOINTS];
static int gba_any_breakpoint_used = 0;

int GBA_DebugIsBreakpoint(u32 addr)
{
    if (gba_any_breakpoint_used == 0)
        return 0;

    for (int i = 0; i < GBA_MAX_BREAKPOINTS; i++)
    {
        if (gba_brkpoint_used[i])
        {
            if (gba_brkpoint_addrlist[i] == addr)
            {
                return 1;
            }
        }
    }
    return 0;
}

static u32 gba_last_executed_opcode = 1;

int GBA_DebugCPUIsBreakpoint(u32 addr)
{
    if (gba_any_breakpoint_used == 0)
        return 0;

    if (gba_last_executed_opcode == addr)
    {
        gba_last_executed_opcode = 1;
        return 0;
    }

    for (int i = 0; i < GBA_MAX_BREAKPOINTS; i++)
    {
        if (gba_brkpoint_used[i])
        {
            if (gba_brkpoint_addrlist[i] == addr)
            {
                gba_last_executed_opcode = addr;
                return 1;
            }
        }
    }
    return 0;
}

void GBA_DebugAddBreakpoint(u32 addr)
{
    if (GBA_DebugIsBreakpoint(addr))
        return;

    for (int i = 0; i < GBA_MAX_BREAKPOINTS; i++)
    {
        if (gba_brkpoint_used[i] == 0)
        {
            gba_brkpoint_addrlist[i] = addr;
            gba_brkpoint_used[i] = 1;
            gba_any_breakpoint_used = 1;
            return;
        }
    }
}

void GBA_DebugClearBreakpoint(u32 addr)
{
    for (int i = 0; i < GBA_MAX_BREAKPOINTS; i++)
    {
        if (gba_brkpoint_used[i])
        {
            if (gba_brkpoint_addrlist[i] == addr)
            {
                gba_brkpoint_used[i] = 0;
                break;
            }
        }
    }

    for (int i = 0; i < GBA_MAX_BREAKPOINTS; i++)
    {
        if (gba_brkpoint_used[i])
            return;
    }

    gba_any_breakpoint_used = 0;
}

void GBA_DebugClearBreakpointAll(void)
{
    for (int i = 0; i < GBA_MAX_BREAKPOINTS; i++)
        gba_brkpoint_used[i] = 0;

    gba_any_breakpoint_used = 0;
}

//------------------------------------------------------------------------------

u32 arm_check_condition(u32 cond); // In arm.c

// Returns 1 if there is a ';' in the line (previous to this or written by this)
static int gba_dissasemble_add_condition_met(int cond, u32 address, char *dest,
                                             int add_comment, int dest_size)
{
    if (CPU.R[R_PC] == address)
    {
        if (cond == 14) // Always
            return 0;

        if (add_comment)
        {
            if (arm_check_condition(cond))
                s_strncat(dest, " ; true", dest_size);
            else
                s_strncat(dest, " ; false", dest_size);
        }
        else
        {
            if (arm_check_condition(cond))
                s_strncat(dest, " - true", dest_size);
            else
                s_strncat(dest, " - false", dest_size);
        }
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------

static struct
{
    char *name;
    u32 address;
} gba_io_reg_struct[] = {
    // LCD I/O Registers
    {"DISPCNT", 0x04000000}, {"GREENSWAP", 0x04000002},
    {"DISPSTAT", 0x04000004}, {"VCOUNT", 0x04000006},
    {"BG0CNT", 0x04000008}, {"BG1CNT", 0x0400000A},
    {"BG2CNT", 0x0400000C}, {"BG3CNT", 0x0400000E},
    {"BG0HOFS", 0x04000010}, {"BG0VOFS", 0x04000012},
    {"BG1HOFS", 0x04000014}, {"BG1VOFS", 0x04000016},
    {"BG2HOFS", 0x04000018}, {"BG2VOFS", 0x0400001A},
    {"BG3HOFS", 0x0400001C}, {"BG3VOFS", 0x0400001E},
    {"BG2PA", 0x04000020}, {"BG2PB", 0x04000022},
    {"BG2PC", 0x04000024}, {"BG2PD", 0x04000026},
    {"BG2X_L", 0x04000028}, {"BG2X_H", 0x0400002A},
    {"BG2Y_L", 0x0400002C}, {"BG2Y_H", 0x0400002E},
    {"BG3PA", 0x04000030}, {"BG3PB", 0x04000032},
    {"BG3PC", 0x04000034}, {"BG3PD", 0x04000036},
    {"BG3X_L", 0x04000038}, {"BG3X_H", 0x0400003A},
    {"BG3Y_L", 0x0400003C}, {"BG3Y_H", 0x0400003E},
    {"WIN0H", 0x04000040}, {"WIN1H", 0x04000042},
    {"WIN0V", 0x04000044}, {"WIN1V", 0x04000046},
    {"WININ", 0x04000048}, {"WINOUT", 0x0400004A},
    {"MOSAIC", 0x0400004C}, {"BLDCNT", 0x04000050},
    {"BLDALPHA", 0x04000052}, {"BLDY", 0x04000054},

    // Sound Registers
    {"SOUND1CNT_L", 0x04000060}, {"SOUND1CNT_H", 0x04000062},
    {"SOUND1CNT_X", 0x04000064}, {"SOUND2CNT_L", 0x04000068},
    {"SOUND2CNT_H", 0x0400006C}, {"SOUND3CNT_L", 0x04000070},
    {"SOUND3CNT_H", 0x04000072}, {"SOUND3CNT_X", 0x04000074},
    {"SOUND4CNT_L", 0x04000078}, {"SOUND4CNT_H", 0x0400007C},
    {"SOUNDCNT_L", 0x04000080}, {"SOUNDCNT_H", 0x04000082},
    {"SOUNDCNT_X", 0x04000084}, {"SOUNDBIAS", 0x04000088},
    {"WAVE_RAM", 0x04000090}, {"WAVE_RAM", 0x04000092},
    {"WAVE_RAM", 0x04000094}, {"WAVE_RAM", 0x04000096},
    {"WAVE_RAM", 0x04000098}, {"WAVE_RAM", 0x0400009A},
    {"WAVE_RAM", 0x0400009C}, {"WAVE_RAM", 0x0400009E},
    {"FIFO_A", 0x040000A0}, {"FIFO_B", 0x040000A4},

    // DMA Transfer Channels
    {"DMA0SAD", 0x040000B0}, {"DMA0DAD", 0x040000B4},
    {"DMA0CNT_L", 0x040000B8}, {"DMA0CNT_H", 0x040000BA},
    {"DMA1SAD", 0x040000BC}, {"DMA1DAD", 0x040000C0},
    {"DMA1CNT_L", 0x040000C4}, {"DMA1CNT_H", 0x040000C6},
    {"DMA2SAD", 0x040000C8}, {"DMA2DAD", 0x040000CC},
    {"DMA2CNT_L", 0x040000D0}, {"DMA2CNT_H", 0x040000D2},
    {"DMA3SAD", 0x040000D4}, {"DMA3DAD", 0x040000D8},
    {"DMA3CNT_L", 0x040000DC}, {"DMA3CNT_H", 0x040000DE},

    // Timer Registers
    {"TM0CNT_L", 0x04000100}, {"TM0CNT_H", 0x04000102},
    {"TM1CNT_L", 0x04000104}, {"TM1CNT_H", 0x04000106},
    {"TM2CNT_L", 0x04000108}, {"TM2CNT_H", 0x0400010A},
    {"TM3CNT_L", 0x0400010C}, {"TM3CNT_H", 0x0400010E},

    // Serial Communication 1
    {"SIODATA32", 0x04000120}, {"SIOMULTI0", 0x04000120},
    {"SIOMULTI1", 0x04000122}, {"SIOMULTI2", 0x04000124},
    {"SIOMULTI3", 0x04000126}, {"SIOCNT", 0x04000128},
    {"SIOMLT_SEND", 0x0400012A}, {"SIODATA8", 0x0400012A},

    // Keypad Input
    {"KEYINPUT", 0x04000130}, {"KEYCNT", 0x04000132},

    // Serial Communication 2
    {"RCNT", 0x04000134},
    // 4000136h - IR, Ancient - Infrared Register Prototypes only,
    {"JOYCNT", 0x04000140}, {"JOY_RECV", 0x04000150},
    {"JOY_TRANS", 0x04000154}, {"JOYSTAT", 0x04000158},

    //Interrupt, Waitstate, and Power-Down Control
    {"IE", 0x04000200}, {"IF", 0x04000202},
    {"WAITCNT", 0x04000204}, {"IME", 0x04000208},
    {"POSTFLG", 0x04000300}, {"HALTCNT", 0x04000301},
    // 4000410h  ?    ?     Undocumented - Purpose Unknown / Bug ??? 0FFh
    // 4000800h  4    R/W   Undocumented - Internal Memory Control R/W
    // 4xx0800h  4    R/W   Mirrors of 4000800h repeated each 64K

    // All further addresses at 4XXXXXXh are unused and do not contain mirrors
    // of the I/O area, with the only exception that 4000800h is repeated each
    // 64K ie.  mirrored at 4010800h, 4020800h, etc.

    // Add a mirror to these addresses, the ones actually used in programs
    {"IRQ_HANDLER", 0x03007FFC}, {"IRQ_HANDLER", 0x03FFFFFC},
    {"IRQ_BIOS_FLAGS", 0x03007FF8}, {"IRQ_BIOS_FLAGS", 0x03FFFFF8},

    // Default memory usage at 03007FXX (and mirrored to 03FFFFXX)

    // Addr. Size Expl.
    // 7FFCh 4    Pointer to user IRQ handler (32bit ARM code)
    // 7FF8h 4    Interrupt Check Flag (for IntrWait/VBlankIntrWait functions)
    // 7FF4h 4    Allocated Area
    // 7FF0h 4    Pointer to Sound Buffer
    // 7FE0h 16   Allocated Area
    // 7FA0h 64   Default area for SP_svc Supervisor Stack (4 words/time)
    // 7F00h 160  Default area for SP_irq Interrupt Stack (6 words/time)

    {NULL, 0}
};

static int gba_dissasemble_add_io_register_name(u32 reg_address, char *dest,
                                                int add_comment, int dest_size)
{
    int i = 0;

    while (1)
    {
        if (gba_io_reg_struct[i].name == NULL)
            break;
        if (gba_io_reg_struct[i].address == reg_address)
        {
            if (add_comment)
            {
                s_strncat(dest, " ; ", dest_size);
                s_strncat(dest, gba_io_reg_struct[i].name, dest_size);
            }
            else
            {
                s_strncat(dest, " - ", dest_size);
                s_strncat(dest, gba_io_reg_struct[i].name, dest_size);
            }
            return 1;
        }
        i++;
    }
    return 0;
}

static struct
{
    int code;
    char *name;
} swi_name_struct[] = {
    {0x00, "SoftReset"}, {0x01, "RegisterRamReset"}, {0x02, "Halt"},
    {0x03, "Stop/Sleep"}, {0x04, "IntrWait"}, {0x05, "VBlankIntrWait"},
    {0x06, "Div"}, {0x07, "DivArm"}, {0x08, "Sqrt"}, {0x09, "ArcTan"},
    {0x0A, "ArcTan2"}, {0x0B, "CpuSet"}, {0x0C, "CpuFastSet"},
    {0x0D, "GetBiosChecksum"}, {0x0E, "BgAffineSet"}, {0x0F, "ObjAffineSet"},
    {0x10, "BitUnPack"}, {0x11, "LZ77UnCompWram"}, {0x12, "LZ77UnCompVram"},
    {0x13, "HuffUnComp"}, {0x14, "RLUnCompWram"}, {0x15, "RLUnCompVram"},
    {0x16, "Diff8bitUnFilterWram"},  {0x17, "Diff8bitUnFilterVram"},
    {0x18, "Diff16bitUnFilter"}, {0x19, "SoundBias"}, {0x1A, "SoundDriverInit"},
    {0x1B, "SoundDriverMode"}, {0x1C, "SoundDriverMain"},
    {0x1D, "SoundDriverVSync"}, {0x1E, "SoundChannelClear"},
    {0x1F, "MidiKey2Freq"}, {0x20, "SoundWhatever0"}, {0x21, "SoundWhatever1"},
    {0x22, "SoundWhatever2"}, {0x23, "SoundWhatever3"},
    {0x24, "SoundWhatever4"}, {0x25, "MultiBoot"}, {0x26, "HardReset"},
    {0x27, "CustomHalt"}, {0x28, "SoundDriverVSyncOff"},
    {0x29, "SoundDriverVSyncOn"}, {0x2A, "SoundGetJumpList"},
    {0, NULL}
};

// SWI code = 1 byte (ARM SWIs have to be adjusted to THUMB SWIs)
static int gba_disassemble_swi_name(int swi_code, char *dest, int add_comment,
                                    int dest_size)
{
    int i = 0;

    while (1)
    {
        if (swi_name_struct[i].name == NULL)
            break;
        if (swi_name_struct[i].code == swi_code)
        {
            if (add_comment)
            {
                s_strncat(dest, " ; ", dest_size);
                s_strncat(dest, swi_name_struct[i].name, dest_size);
            }
            else
            {
                s_strncat(dest, " - ", dest_size);
                s_strncat(dest, swi_name_struct[i].name, dest_size);
            }
            return 1;
        }
        i++;
    }
    return 0;
}

//------------------------------------------------------------------------------

// ldr      r0, [r1, r2]    @ Pre-indexed.             r0= *(u32*)(r1+r2)
// ldr      r0, [r1, r2]!   @ Pre-indexed,  writeback. r0= *(u32*)(r1 += r2)
// ldr      r0, [r1], r2    @ Post-indexed, writeback. r0= *(u32*)r1; r1 += r2;

static const char arm_cond[16][6] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "", "nv[!]"
};

static const char arm_shift_type[4][4] = { "lsl", "lsr", "asr", "ror" };

void GBA_DisassembleARM(u32 opcode, u32 address, char *dest, int dest_size)
{
    int arm_cond_code = (opcode >> 28) & 0xF;
    const char *cond = arm_cond[arm_cond_code];
    u32 ident = (opcode >> 25) & 7;
    opcode &= 0x01FFFFFF;

    switch (ident)
    {
        case 0:
        {
            if (opcode & BIT(4))
            {
                if (opcode & BIT(7))
                {
                    // Halfword, Doubleword, and Signed Data Transfer
                    if (opcode & (3 << 5))
                    {
                        u32 Rn = (opcode >> 16) & 0xF; // (including R15=PC+8)
                        u32 Rd = (opcode >> 12) & 0xF; // (including R15=PC+12)

                        char *sign = (opcode & BIT(23)) ? "+" : "-";

                        // Only if "Immediate as offset"
                        u32 writeresult = (opcode & BIT(22)) && (Rn == R_PC);

                        u32 addr = CPU.R[Rn];

                        char addr_text[32];
                        if (opcode & BIT(24)) // Pre-indexed
                        {
                            char *writeback = (opcode & BIT(21)) ? "!" : "";

                            if (opcode & BIT(22)) // Immediate as offset
                            {
                                // [Rn, <#{+/-}expression>]{!}
                                u32 offset = ((opcode >> 4) & 0xF0)
                                             | (opcode & 0xF);
                                if (offset)
                                {
                                    snprintf(addr_text, sizeof(addr_text),
                                             "[r%d, #%s0x%08X]%s", Rn, sign,
                                             offset, writeback);
                                }
                                else
                                {
                                    snprintf(addr_text, sizeof(addr_text),
                                             "[r%d]%s", Rn, writeback);
                                }
                                addr += (opcode & BIT(23)) ? offset : -offset;
                            }
                            else // Register as offset
                            {
                                // [Rn, {+/-}Rm]{!}
                                // 11-8 must be 0000
                                u32 Rm = opcode & 0xF; // (not including R15)
                                snprintf(addr_text, sizeof(addr_text),
                                         "[r%d, %sr%d]%s", Rn, sign, Rm,
                                         writeback);
                            }
                        }
                        else // Post-indexed
                        {
                            if (opcode & BIT(22)) // Immediate
                            {
                                // [Rn], <#{+/-}expression>
                                u32 offset = ((opcode >> 4) & 0xF0)
                                             | (opcode & 0xF);
                                if (offset)
                                {
                                    snprintf(addr_text, sizeof(addr_text),
                                             "[r%d], #%s0x%08X", Rn, sign,
                                             offset);
                                }
                                else
                                {
                                    snprintf(addr_text, sizeof(addr_text),
                                             "[r%d]", Rn);
                                }
                            }
                            else // Register as offset
                            {
                                // [Rn], {+/-}Rm
                                // 11-8 must be 0000
                                u32 Rm = opcode & 0xF; // (not including R15)
                                snprintf(addr_text, sizeof(addr_text),
                                         "[r%d], %sr%d", Rn, sign, Rm);
                            }
                        }

                        if (opcode & BIT(20)) // LDR
                        {
                            u32 op = (opcode >> 5) & 3;

                            if (op == 1)
                            {
                                // LDR{cond}H  Rd,<Address>
                                // Load Unsigned halfword (zero-extended)

                                if (writeresult)
                                {
                                    snprintf(dest, dest_size,
                                             "ldr%sh r%d, =0x%08X", cond, Rd,
                                             (u32)GBA_MemoryRead16(addr & ~1));
                                    gba_dissasemble_add_condition_met(
                                            arm_cond_code, address, dest, 1,
                                            dest_size);
                                }
                                else
                                {
                                    snprintf(dest, dest_size, "ldr%sh r%d, %s",
                                             cond, Rd, addr_text);
                                    int comment = gba_dissasemble_add_condition_met(
                                            arm_cond_code, address, dest, 1,
                                            dest_size);
                                    gba_dissasemble_add_io_register_name(addr,
                                            dest, !comment, dest_size);
                                }
                                return;
                            }
                            else if (op == 2)
                            {
                                // LDR{cond}SB Rd,<Address>
                                // Load Signed byte (sign extended)

                                if (writeresult)
                                {
                                    snprintf(dest, dest_size,
                                             "ldr%ssb r%d, =0x%08X", cond, Rd,
                                             (s32)(s8)GBA_MemoryRead8(address));
                                    gba_dissasemble_add_condition_met(
                                            arm_cond_code, address, dest, 1,
                                            dest_size);
                                }
                                else
                                {
                                    snprintf(dest, dest_size, "ldr%ssb r%d, %s",
                                             cond, Rd, addr_text);
                                    int comment = gba_dissasemble_add_condition_met(
                                            arm_cond_code, address, dest, 1,
                                            dest_size);
                                    gba_dissasemble_add_io_register_name(addr,
                                            dest, !comment, dest_size);
                                }
                                return;
                            }
                            else //if (op == 3)
                            {
                                // LDR{cond}SH Rd,<Address>
                                // Load Signed halfword (sign extended)

                                if (writeresult)
                                {
                                    snprintf(dest, dest_size,
                                             "ldr%ssh r%d, =0x%08X", cond, Rd,
                                             (s32)(s16)GBA_MemoryRead16(address & ~1));
                                    gba_dissasemble_add_condition_met(
                                            arm_cond_code, address, dest, 1,
                                            dest_size);
                                }
                                else
                                {
                                    snprintf(dest, dest_size, "ldr%ssh r%d, %s",
                                             cond, Rd, addr_text);
                                    int comment = gba_dissasemble_add_condition_met(
                                            arm_cond_code, address, dest, 1,
                                            dest_size);
                                    gba_dissasemble_add_io_register_name(addr,
                                            dest, !comment, dest_size);
                                }
                                return;
                            }
                        }
                        else // STR
                        {
                            if (((opcode >> 5) & 3) > 1)
                            {
                                s_strncpy(dest,
                                          "[!] Undefined Instruction #0-4",
                                          dest_size);
                                return;
                            }

                            // STR{cond}H  Rd,<Address>  ; Store halfword
                            snprintf(dest, dest_size, "str%sh r%d, %s", cond,
                                     Rd, addr_text);
                            int comment = gba_dissasemble_add_condition_met(
                                    arm_cond_code, address, dest, 1, dest_size);
                            gba_dissasemble_add_io_register_name(addr, dest,
                                    !comment, dest_size);
                            return;
                        }
                        return;
                    }
                    else // MUL/SWP
                    {
                        if (opcode & BIT(24)) // SWP{cond}{B} Rd,Rm,[Rn]
                        {
                            if (opcode & BIT(23)
                                || (opcode & (0xF00 | (3 << 20))))
                            {
                                s_strncpy(dest,
                                          "[!] Undefined Instruction #0-1",
                                          dest_size);
                                return;
                            }

                            char *bytemode = opcode & BIT(22) ? "b" : "";

                            u32 Rn = (opcode >> 16) & 0xF; // |
                            u32 Rd = (opcode >> 12) & 0xF; // | r0 - r14
                            u32 Rm = opcode & 0xF;         // |

                            snprintf(dest, dest_size, "swp%s%s r%d, r%d, [r%d]",
                                     cond, bytemode, Rd, Rm, Rn);
                            int comment = gba_dissasemble_add_condition_met(
                                    arm_cond_code, address, dest, 1, dest_size);
                            gba_dissasemble_add_io_register_name(CPU.R[Rn],
                                    dest, !comment, dest_size);
                            return;
                        }
                        else // Multiplication
                        {
                            u32 op = (opcode >> 21) & 7;

                            char *setcond = ((opcode & BIT(20)) != 0) ? "s" : "";

                            u32 Rd = (opcode >> 16) & 0xF; // |        (or RdHi)
                            u32 Rn = (opcode >> 12) & 0xF; // | r0-r14 (or RdLo)
                            u32 Rs = (opcode >> 8) & 0xF;  // |
                            u32 Rm = opcode & 0xF;         // |

                            switch (op)
                            {
                                case 0:
                                    snprintf(dest, dest_size,
                                             "mul%s%s r%d, r%d, r%d", cond,
                                             setcond, Rd, Rm, Rs);
                                    break;
                                case 1:
                                    snprintf(dest, dest_size,
                                             "mla%s%s r%d, r%d, r%d, r%d", cond,
                                             setcond, Rd, Rm, Rs, Rn);
                                    break;
                                case 2:
                                case 3:
                                    s_strncpy(dest,
                                              "[!] Undefined Instruction #0-2",
                                              dest_size);
                                    return;
                                case 4:
                                    snprintf(dest, dest_size,
                                             "umull%s%s r%d, r%d, r%d, r%d",
                                             cond, setcond, Rn, Rd, Rm, Rs);
                                    break;
                                case 5:
                                    snprintf(dest, dest_size,
                                             "umlal%s%s r%d, r%d, r%d, r%d",
                                             cond, setcond, Rn, Rd, Rm, Rs);
                                    break;
                                case 6:
                                    snprintf(dest, dest_size,
                                             "smull%s%s r%d, r%d, r%d, r%d",
                                             cond, setcond, Rn, Rd, Rm, Rs);
                                    break;
                                case 7:
                                    snprintf(dest, dest_size,
                                             "smlal%s%s r%d, r%d, r%d, r%d",
                                             cond, setcond, Rn, Rd, Rm, Rs);
                                    break;
                            }
                            gba_dissasemble_add_condition_met(arm_cond_code,
                                    address, dest, 1, dest_size);
                            return;
                        }
                    }
                }
                else
                {
                    if ((opcode & 0x0FFFFFF0) == 0x012FFF10)
                    {
                        // BX{cond}
                        u32 Rn = opcode & 0xF;
                        if (CPU.R[Rn] & 1) // Switch to THUMB
                        {
                            // PC=Rn-1, T=Rn.0
                            snprintf(dest, dest_size,
                                     "bx%s r%d ; Switch to THUMB", cond, Rn);
                            gba_dissasemble_add_condition_met(arm_cond_code,
                                    address, dest, 0, dest_size);
                        }
                        else
                        {
                            // PC=Rn, T=Rn.0
                            snprintf(dest, dest_size, "bx%s r%d", cond, Rn);
                            gba_dissasemble_add_condition_met(arm_cond_code,
                                    address, dest, 1, dest_size);
                        }
                        return;
                    }

                    // Data Processing, (Register shifted by Register) 2nd Operand

                    u32 op = (opcode >> 21);

                    // 8 <= op <= b
                    if (((op & 0xC) == 0x8) && ((opcode & BIT(20)) == 0))
                    {
                        s_strncpy(dest, "[!] Undefined Instruction #0-3",
                                  dest_size);
                        return;
                    }

                    // Must be 0000b for MOV/MVN.
                    u32 Rn = (opcode >> 16) & 0xF;
                    // Must be 0000b {or 1111b) for CMP/CMN/TST/TEQ{P}.
                    u32 Rd = (opcode >> 12) & 0xF;

                    char *setcond = ((opcode & BIT(20)) != 0) ? "s" : "";

                    const char *shift = arm_shift_type[(opcode >> 5) & 3];

                    u32 Rs = (opcode >> 8) & 0xF; // (R0-R14)
                    u32 Rm = opcode & 0xF;

                    switch (op)
                    {
                        case 0:
                            snprintf(dest, dest_size,
                                     "and%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 1:
                            snprintf(dest, dest_size,
                                     "eor%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 2:
                            snprintf(dest, dest_size,
                                     "sub%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 3:
                            snprintf(dest, dest_size,
                                     "rsb%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 4:
                            snprintf(dest, dest_size,
                                     "add%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 5:
                            snprintf(dest, dest_size,
                                     "adc%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 6:
                            snprintf(dest, dest_size,
                                     "sbc%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 7:
                            snprintf(dest, dest_size,
                                     "rsc%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 8:
                            snprintf(dest, dest_size,
                                     "tst%s r%d, r%d, %s r%d", cond,
                                     Rn, Rm, shift, Rs);
                            break;
                        case 9:
                            snprintf(dest, dest_size,
                                     "teq%s r%d, r%d, %s r%d", cond,
                                     Rn, Rm, shift, Rs);
                            break;
                        case 0xA:
                            snprintf(dest, dest_size,
                                     "cmp%s r%d, r%d, %s r%d", cond,
                                     Rn, Rm, shift, Rs);
                            break;
                        case 0xB:
                            snprintf(dest, dest_size,
                                     "cmn%s r%d, r%d, %s r%d", cond,
                                     Rn, Rm, shift, Rs);
                            break;
                        case 0xC:
                            snprintf(dest, dest_size,
                                     "orr%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 0xD:
                            snprintf(dest, dest_size,
                                     "mov%s%s r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rm, shift, Rs);
                            break;
                        case 0xE:
                            snprintf(dest, dest_size,
                                     "bic%s%s r%d, r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rn, Rm, shift, Rs);
                            break;
                        case 0xF:
                            snprintf(dest, dest_size,
                                     "mvn%s%s r%d, r%d, %s r%d", cond,
                                     setcond, Rd, Rm, shift, Rs);
                            break;
                    }
                    gba_dissasemble_add_condition_met(arm_cond_code, address,
                                                      dest, 1, dest_size);
                    return;
                }
            }
            else
            {
                if ((opcode & ((3 << 23) | (0x3F << 16) | 0xFFF))
                    == ((2 << 23) | (0x0F << 16)))
                {
                    u32 Rd = (opcode >> 12) & 0xF;

                    if (opcode & BIT(22)) // MRS{cond} Rd,spsr
                    {
                        snprintf(dest, dest_size, "mrs%s r%d, spsr", cond, Rd);
                        gba_dissasemble_add_condition_met(arm_cond_code,
                                address, dest, 1, dest_size);
                        return;
                    }
                    else // MRS{cond} Rd,cpsr
                    {
                        snprintf(dest, dest_size, "mrs%s r%d, cpsr", cond, Rd);
                        gba_dissasemble_add_condition_met(arm_cond_code,
                                address, dest, 1, dest_size);
                        return;
                    }
                }

                u32 val = opcode & (0x1F << 20);
                if (val == (0x12 << 20))
                {
                    // MSR{cond} cpsr{_field},Rm
                    u32 Rm = opcode & 0xF;
                    char fields[5];
                    int cursor = 0;
                    if (opcode & BIT(19))
                        fields[cursor++] = 'f'; // 31-24
                    if (opcode & BIT(18))
                        fields[cursor++] = 's';
                    if (opcode & BIT(17))
                        fields[cursor++] = 'x';
                    if (opcode & BIT(16))
                        fields[cursor++] = 'c'; // 7-0
                    fields[cursor] = '\0';

                    snprintf(dest, dest_size, "msr%s cpsr_%s, r%d", cond,
                             fields, Rm);
                    gba_dissasemble_add_condition_met(arm_cond_code, address,
                                                      dest, 1, dest_size);
                    return;
                }
                else if (val == (0x16 << 20))
                {
                    // MSR{cond} spsr{_field},Rm
                    u32 Rm = opcode & 0xF;
                    char fields[5];
                    int cursor = 0;
                    if (opcode & BIT(19))
                        fields[cursor++] = 'f'; // 31-24
                    if (opcode & BIT(18))
                        fields[cursor++] = 's';
                    if (opcode & BIT(17))
                        fields[cursor++] = 'x';
                    if (opcode & BIT(16))
                        fields[cursor++] = 'c'; // 7-0
                    fields[cursor] = '\0';

                    snprintf(dest, dest_size, "msr%s spsr_%s, r%d", cond,
                             fields, Rm);
                    gba_dissasemble_add_condition_met(arm_cond_code, address,
                                                      dest, 1, dest_size);
                    return;
                }

                // Data Processing, (Shift by Inmediate) 2nd Operand
                u32 op = (opcode >> 21);

                // 8 <= op <= b
                if (((op & 0xC) == 0x8) && ((opcode & BIT(20)) == 0))
                {
                    s_strncpy(dest, "[!] Undefined Instruction #1", dest_size);
                    return;
                }
                // Must be 0000b for MOV/MVN.
                u32 Rn = (opcode >> 16) & 0xF;
                // Must be 0000b {or 1111b) for CMP/CMN/TST/TEQ{P}.
                u32 Rd = (opcode >> 12) & 0xF;

                char *setcond = ((opcode & BIT(20)) != 0) ? "s" : "";

                u32 shiftval = (opcode >> 7) & 0x1F;
                u32 Rm = opcode & 0xF;
                char *shift = (char *)arm_shift_type[(opcode >> 5) & 3];

                int canbenop = 0;

                char temp[40];
                if (shiftval == 0)
                {
                    switch ((opcode >> 5) & 3)
                    {
                        default:
                        case 0:
                            temp[0] = '\0';
                            if ((opcode & BIT(20)) == 0)
                                canbenop = 1;
                            break;
                        case 1:
                        case 2:
                            shiftval = 32;
                            snprintf(temp, sizeof(temp), ", %s #0x%02X",
                                     shift, shiftval);
                            break;
                        case 3:
                            s_strncpy(temp, ", rrx", sizeof(temp));
                            break;
                    }
                }
                else
                {
                    snprintf(temp, sizeof(temp), ", %s #0x%02X", shift,
                             shiftval);
                }

                switch (op)
                {
                    case 0:
                        snprintf(dest, dest_size, "and%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 1:
                        snprintf(dest, dest_size, "eor%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 2:
                        snprintf(dest, dest_size, "sub%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 3:
                        snprintf(dest, dest_size, "rsb%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 4:
                        snprintf(dest, dest_size, "add%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 5:
                        snprintf(dest, dest_size, "adc%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 6:
                        snprintf(dest, dest_size, "sbc%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 7:
                        snprintf(dest, dest_size, "rsc%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 8:
                        snprintf(dest, dest_size, "tst%s r%d, r%d%s", cond, Rn,
                                 Rm, temp);
                        break;
                    case 9:
                        snprintf(dest, dest_size, "teq%s r%d, r%d%s", cond, Rn,
                                 Rm, temp);
                        break;
                    case 0xA:
                        snprintf(dest, dest_size, "cmp%s r%d, r%d%s", cond, Rn,
                                 Rm, temp);
                        break;
                    case 0xB:
                        snprintf(dest, dest_size, "cmn%s r%d, r%d%s", cond, Rn,
                                 Rm, temp);
                        break;
                    case 0xC:
                        snprintf(dest, dest_size, "orr%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 0xD:
                        if ((canbenop) && ((Rd | Rm) == 0))
                        {
                            snprintf(dest, dest_size, "nop%s", cond);
                        }
                        else
                        {
                            snprintf(dest, dest_size, "mov%s%s r%d, r%d%s",
                                     cond, setcond, Rd, Rm, temp);
                        }
                        break;
                    case 0xE:
                        snprintf(dest, dest_size, "bic%s%s r%d, r%d, r%d%s",
                                 cond, setcond, Rd, Rn, Rm, temp);
                        break;
                    case 0xF:
                        snprintf(dest, dest_size, "mvn%s%s r%d, r%d%s",
                                 cond, setcond, Rd, Rm, temp);
                        break;
                }
                gba_dissasemble_add_condition_met(arm_cond_code, address, dest,
                                                  1, dest_size);
                return;
            }
        }
        case 1:
        {
            if ((opcode & ((3 << 23) | (3 << 20))) == ((2 << 23) | (2 << 20)))
            {
                // MSR{cond} Psr{_field},Imm

                // 20-17 must be 1111b
                char *dst = opcode & BIT(22) ? "spsr" : "cpsr";
                u32 value = ror_immed_no_carry(opcode & 0xFF,
                                               ((opcode >> 8) & 0xF) << 1);
                char fields[5];
                int cursor = 0;
                if (opcode & BIT(19))
                    fields[cursor++] = 'f'; // 31-24
                if (opcode & BIT(18))
                    fields[cursor++] = 's';
                if (opcode & BIT(17))
                    fields[cursor++] = 'x';
                if (opcode & BIT(16))
                    fields[cursor++] = 'c'; // 7-0
                fields[cursor] = '\0';
                snprintf(dest, dest_size, "msr%s %s_%s, #0x%08X",
                         cond, dst, fields, value);
                gba_dissasemble_add_condition_met(arm_cond_code, address, dest,
                                                  1, dest_size);
                return;
            }

            // Data Processing, Immediate 2nd Operand
            u32 op = (opcode >> 21);

            // 8 <= op <= b
            if (((op & 0xC) == 0x8) && ((opcode & BIT(20)) == 0))
            {
                s_strncpy(dest, "[!] Undefined Instruction #1", dest_size);
                return;
            }
            // Must be 0000b for MOV/MVN.
            u32 Rn = (opcode >> 16) & 0xF;
            // Must be 0000b {or 1111b) for CMP/CMN/TST/TEQ{P}.
            u32 Rd = (opcode >> 12) & 0xF;

            char *setcond = ((opcode & BIT(20)) != 0) ? "s" : "";

            u32 val = ror_immed_no_carry(opcode & 0xFF,
                                         ((opcode >> 8) & 0xF) << 1);

            switch (op)
            {
                case 0:
                    snprintf(dest, dest_size, "and%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 1:
                    snprintf(dest, dest_size, "eor%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 2:
                    snprintf(dest, dest_size, "sub%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 3:
                    snprintf(dest, dest_size, "rsb%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 4:
                    snprintf(dest, dest_size, "add%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 5:
                    snprintf(dest, dest_size, "adc%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 6:
                    snprintf(dest, dest_size, "sbc%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 7:
                    snprintf(dest, dest_size, "rsc%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 8:
                    snprintf(dest, dest_size, "tst%s r%d, #0x%08X",
                             cond, Rn, val);
                    break;
                case 9:
                    snprintf(dest, dest_size, "teq%s r%d, #0x%08X",
                             cond, Rn, val);
                    break;
                case 0xA:
                    snprintf(dest, dest_size, "cmp%s r%d, #0x%08X",
                             cond, Rn, val);
                    break;
                case 0xB:
                    snprintf(dest, dest_size, "cmn%s r%d, #0x%08X",
                             cond, Rn, val);
                    break;
                case 0xC:
                    snprintf(dest, dest_size, "orr%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 0xD:
                    snprintf(dest, dest_size, "mov%s%s r%d, #0x%08X",
                             cond, setcond, Rd, val);
                    break;
                case 0xE:
                    snprintf(dest, dest_size, "bic%s%s r%d, r%d, #0x%08X",
                             cond, setcond, Rd, Rn, val);
                    break;
                case 0xF:
                    snprintf(dest, dest_size, "mvn%s%s r%d, #0x%08X",
                             cond, setcond, Rd, val);
                    break;
                default:
                    break;
            }
            gba_dissasemble_add_condition_met(arm_cond_code, address, dest, 1,
                                              dest_size);
            return;
        }
        case 2:
        {
            // LDR/STR -- Immediate as offseT

            u32 Rn = (opcode >> 16) & 0xF; // (including R15=PC+8)
            u32 Rd = (opcode >> 12) & 0xF; // (including R15=PC+12)
            s32 offset = opcode & 0xFFF;
            char *sign = (opcode & BIT(23)) ? "+" : "-";

            char *bytemode = opcode & BIT(22) ? "b" : "";

            char *forceuser = "";

            char addr_text[32];
            if (opcode & BIT(24)) // Pre-indexed
            {
                if (offset)
                {
                    snprintf(addr_text, sizeof(addr_text), "[r%d, #%s0x%03X]%s",
                             Rn, sign, offset, opcode & BIT(21) ? "!" : "");
                }
                else
                {
                    snprintf(addr_text, sizeof(addr_text), "[r%d]%s",
                             Rn, opcode & BIT(21) ? "!" : "");
                }
            }
            else // Post-indexed
            {
                forceuser = (opcode & BIT(21)) ? "t" : "";
                if (offset)
                {
                    snprintf(addr_text, sizeof(addr_text), "[r%d], #%s0x%03X",
                             Rn, sign, offset);
                }
                else
                {
                    snprintf(addr_text, sizeof(addr_text), "[r%d]", Rn);
                }
            }

            u32 addr = address;

            if (opcode & BIT(20)) // LDR{cond}{B}{T} Rd,<Address>
            {
                if (Rn == R_PC)
                {
                    if (opcode & BIT(24)) // Pre-indexed
                    {
                        addr += 8 + ((opcode & BIT(23)) ? offset : -offset);
                    }

                    if (opcode & BIT(22))
                    {
                        snprintf(dest, dest_size, "ldr%s%s%s r%d, =0x%08X",
                                 cond, bytemode, forceuser, Rd,
                                 GBA_MemoryRead32(addr) & 0xFF);
                    }
                    else
                    {
                        snprintf(dest, dest_size, "ldr%s%s%s r%d, =0x%08X",
                                 cond, bytemode, forceuser, Rd,
                                 GBA_MemoryRead32(addr));
                    }
                }
                else
                {
                    if (opcode & BIT(24)) // Pre-indexed
                        addr += ((opcode & BIT(23)) ? offset : -offset);
                    snprintf(dest, dest_size, "ldr%s%s%s r%d, %s", cond,
                             bytemode, forceuser, Rd, addr_text);
                }
            }
            else // STR{cond}{B}{T} Rd,<Address>
            {
                if (opcode & BIT(24)) // Pre-indexed
                    addr += ((opcode & BIT(23)) ? offset : -offset);
                snprintf(dest, dest_size, "str%s%s%s r%d, %s", cond, bytemode,
                         forceuser, Rd, addr_text);
            }
            int comment = gba_dissasemble_add_condition_met(arm_cond_code,
                    address, dest, 1, dest_size);
            gba_dissasemble_add_io_register_name(CPU.R[Rn], dest, !comment,
                                                 dest_size);
            return;
        }
        case 3:
        {
            // LDR/STR -- Shifted register as offset

            if (opcode & BIT(4))
            {
                s_strncpy(dest, "[!] Undefined Instruction", dest_size);
                return;
            }

            u32 Rn = (opcode >> 16) & 0xF; // (including R15=PC+8)
            u32 Rd = (opcode >> 12) & 0xF; // (including R15=PC+12)
            u32 Rm = opcode & 0xF;         // (not including PC=R15)

            char *sign = (opcode & BIT(23)) ? "+" : "-";
            char *bytemode = opcode & BIT(22) ? "b" : "";

            char *forceuser = "";

            char shift_text[16];
            u32 shiftval = (opcode >> 7) & 0x1F;
            char *shift = (char *)arm_shift_type[(opcode >> 5) & 3];

            if (shiftval == 0)
            {
                switch ((opcode >> 5) & 3)
                {
                    default:
                    case 0:
                        snprintf(shift_text, sizeof(shift_text), "r%d", Rm);
                        break;
                    case 1:
                    case 2:
                        shiftval = 32;
                        snprintf(shift_text, sizeof(shift_text),
                                 "r%d, %s #%02X", Rm, shift, shiftval);
                        break;
                    case 3:
                        snprintf(shift_text, sizeof(shift_text), "r%d, rrx",
                                 Rm);
                        break;
                }
            }
            else
            {
                snprintf(shift_text, sizeof(shift_text), "r%d, %s #%02X", Rm,
                         shift, shiftval);
            }

            u32 addr = CPU.R[Rn] + (Rn == 15 ? 8 : 0);

            char addr_text[32];
            if (opcode & BIT(24)) // Pre-indexed
            {
                snprintf(addr_text, sizeof(addr_text), "[r%d, %s%s]%s", Rn,
                         sign, shift_text, opcode & BIT(21) ? "!" : "");
                s32 offset = cpu_shift_by_reg_no_carry_arm_ldr_str(
                        (opcode >> 5) & 3, CPU.R[opcode & 0xF],
                        (opcode >> 7) & 0x1F);
                addr += (opcode & BIT(23)) ? offset : -offset;
            }
            else // Post-indexed
            {
                forceuser = (opcode & BIT(21)) ? "t" : "";
                snprintf(addr_text, sizeof(addr_text), "[r%d], %s%s", Rn, sign,
                         shift_text);
            }

            if (opcode & BIT(20)) // LDR{cond}{B}{T} Rd,<Address>
            {
                snprintf(dest, dest_size, "ldr%s%s%s r%d, %s", cond, bytemode,
                         forceuser, Rd, addr_text);
            }
            else // STR{cond}{B}{T} Rd,<Address>
            {
                snprintf(dest, dest_size, "str%s%s%s r%d, %s", cond, bytemode,
                         forceuser, Rd, addr_text);
            }
            int comment = gba_dissasemble_add_condition_met(arm_cond_code,
                    address, dest, 1, dest_size);
            gba_dissasemble_add_io_register_name(addr, dest, !comment,
                                                 dest_size);
            return;
        }
        case 4:
        {
#if 0
            if ((opcode & (3 << 21)) == (3 << 21))
            {
                // Unpredictable
                s_strncpy(dest, "[!] Undefined Instruction #4", dest_size);
                return;
            }
#endif
            // Block Data Transfer (LDM,STM)

            // before : after | pre : post
            char *increment_time = (opcode & BIT(24)) ? "b" : "a";
            // increment : decrement | + : -
            char *sign = (opcode & BIT(23)) ? "i" : "d";
            char *usrmod = (opcode & BIT(22)) ? "^" : "";
            char *writeback = (opcode & BIT(21)) ? "!" : "";

            u32 Rn = (opcode >> 16) & 0xF; // (not including R15)

            u32 load = opcode & BIT(20);

            int is_pop = (opcode & BIT(23)) && (!(opcode & BIT(24)))
                         && (opcode & BIT(21)) && (Rn == R_SP);
            int is_push = (!(opcode & BIT(23))) && (opcode & BIT(24))
                          && (opcode & BIT(21)) && (Rn == R_SP);

            opcode &= 0xFFFF;

            char reglist[128] = "{";
            for (int i = 0; i < 16; i++)
            {
                if (opcode & BIT(i))
                {
                    char reg[4];
                    snprintf(reg, sizeof(reg), "r%d", i);
                    s_strncat(reglist, reg, sizeof(reglist));
                    if ((opcode & BIT(i + 1)) && (opcode & BIT(i + 2)))
                    {
                        s_strncat(reglist, "-", sizeof(reglist));
                        while (opcode & BIT(i++))
                            ;
                        i -= 2;
                        snprintf(reg, sizeof(reg), "r%d", i);
                        s_strncat(reglist, reg, sizeof(reglist));
                    }

                    int j = i + 1;
                    while ((opcode & BIT(j)) == 0)
                    {
                        j++;
                        if (j == 16)
                            break;
                    }
                    if (j < 16)
                        s_strncat(reglist, ",", sizeof(reglist));
                }
            }
            s_strncat(reglist, "}", sizeof(reglist));

            if (load)
            {
                // LDM{cond}{amod} Rn{!},<Rlist>{^}
                snprintf(dest, dest_size, "ldm%s%s%s r%d%s, %s%s", cond, sign,
                         increment_time, Rn, writeback, reglist, usrmod);
                if (is_pop)
                    s_strncat(dest, " ; pop", dest_size); // ldmia r13! == pop
            }
            else
            {
                // STM{cond}{amod} Rn{!},<Rlist>{^}
                snprintf(dest, dest_size, "stm%s%s%s r%d%s, %s%s", cond, sign,
                         increment_time, Rn, writeback, reglist, usrmod);
                if (is_push)
                    s_strncat(dest, " ; push", dest_size); // stmdb r13! == push
            }
            gba_dissasemble_add_condition_met(arm_cond_code, address, dest, 1,
                                              dest_size);
            return;
        }
        case 5:
        {
            if (opcode & (1 << 24)) // BL{cond}
            {
                u32 nn = (opcode & 0x00FFFFFF);
                snprintf(dest, dest_size, "bl%s #0x%08X", cond,
                         address + 8
                                 + (((nn & BIT(23)) ? (nn | 0xFF000000) : nn) * 4));
                s_strncat(dest, " ; ->", dest_size);
                gba_dissasemble_add_condition_met(arm_cond_code, address, dest,
                                                  0, dest_size);
                return;
            }
            else // B{cond}
            {
                u32 nn = (opcode & 0x00FFFFFF);
                u32 addr_dest = address + 8
                                + (((nn & BIT(23)) ? (nn | 0xFF000000) : nn) * 4);
                snprintf(dest, dest_size, "b%s #0x%08X", cond, addr_dest);
                if (addr_dest > address)
                    s_strncat(dest, " ; " STR_SLIM_ARROW_DOWN, dest_size);
                else if (addr_dest == address)
                    s_strncat(dest, " ; <-", dest_size);
                else
                    s_strncat(dest, " ; " STR_SLIM_ARROW_UP, dest_size);
                gba_dissasemble_add_condition_met(arm_cond_code, address, dest,
                                                  0, dest_size);
                return;
            }
        }
        case 6:
        {
            // Coprocessor Data Transfers (LDC,STC)
            // Irrelevant in GBA because no coprocessor exists (except a dummy
            // CP14).

            u32 Rn = (opcode >> 16) & 0xF;
            u32 Cd = (opcode >> 12) & 0xF;
            u32 Pn = (opcode >> 8) & 0xF;
            u32 offset = (opcode & 0xFF) << 2;
            char *sign = (opcode & BIT(23)) ? "+" : "-";
            char *length = (opcode & BIT(22)) ? "l" : "";
            char *writeback = (opcode & BIT(21)) ? "!" : "";

            char addr_text[32];
            if (opcode & BIT(24)) // Pre
            {
                if (offset)
                {
                    snprintf(addr_text, sizeof(addr_text), "[r%d, #%s0x%03X]%s",
                             Rn, sign, offset, writeback);
                }
                else
                {
                    snprintf(addr_text, sizeof(addr_text), "[r%d]%s", Rn,
                             writeback);
                }
            }
            else // Post
            {
                // Always writeback?
                if (offset)
                {
                    snprintf(addr_text, sizeof(addr_text), "[r%d], #%s0x%03X",
                             Rn, sign, offset);
                }
                else
                {
                    snprintf(addr_text, sizeof(addr_text), "[r%d]", Rn);
                }
            }

            if (opcode & BIT(20)) // LDC{cond}{L} Pn,Cd,<Address>
            {
                snprintf(dest, dest_size, "ldc%s%s p%d, c%d, %s ; [!]", cond,
                         length, Pn, Cd, addr_text);
                gba_dissasemble_add_condition_met(arm_cond_code, address, dest,
                                                  1, dest_size);
                return;
            }
            else // STC{cond}{L} Pn,Cd,<Address>
            {
                snprintf(dest, dest_size, "stc%s%s p%d, c%d, %s ; [!]", cond,
                         length, Pn, Cd, addr_text);
                gba_dissasemble_add_condition_met(arm_cond_code, address, dest,
                                                  1, dest_size);
                return;
            }
        }
        case 7:
        {
            if (opcode & BIT(24))
            {
                // SWI{cond}
                snprintf(dest, dest_size, "swi%s #0x%06X", cond,
                         (opcode & 0x00FFFFFF));
                int comment = gba_dissasemble_add_condition_met(arm_cond_code,
                        address, dest, 1, dest_size);
                gba_disassemble_swi_name((opcode & 0x00FFFFFF) >> 16, dest,
                                         !comment, dest_size);
                return;
            }
            else
            {
                if (opcode & BIT(4)) // Coprocessor Register Transfers (MRC, MCR)
                {
                    // Irrelevant in GBA because no coprocessor exists (except a
                    // dummy CP14).
                    u32 CP_Opc = (opcode >> 21) & 7;
                    u32 Cn = (opcode >> 16) & 0xF;
                    u32 Rd = (opcode >> 12) & 0xF;
                    u32 Pn = (opcode >> 8) & 0xF;
                    u32 CP = (opcode >> 5) & 7;
                    u32 Cm = opcode & 0xF;

                    if (opcode & BIT(20)) // MRC{cond} Pn,<cpopc>,Rd,Cn,Cm{,<cp>}
                    {
                        snprintf(dest, dest_size,
                                 "mrc%s p%d, #0x%01X, r%d, c%d, c%d, #0x%01X ; [!]",
                                 cond, Pn, CP_Opc, Rd, Cn, Cm, CP);
                        gba_dissasemble_add_condition_met(arm_cond_code,
                                address, dest, 1, dest_size);
                        return;
                    }
                    else // MCR{cond} Pn,<cpopc>,Rd,Cn,Cm{,<cp>}
                    {
                        snprintf(dest, dest_size,
                                 "mcr%s p%d, #0x%01X, r%d, c%d, c%d, #0x%01X ; [!]",
                                 cond, Pn, CP_Opc, Rd, Cn, Cm, CP);
                        gba_dissasemble_add_condition_met(arm_cond_code,
                                address, dest, 1, dest_size);
                        return;
                    }
                }
                else // Coprocessor Data Operations (CDP)
                {
                    // Irrelevant in GBA because no coprocessor exists (except a
                    // dummy CP14).
                    u32 CP_Opc = (opcode >> 20) & 0xF;
                    u32 Cn = (opcode >> 16) & 0xF;
                    u32 Cd = (opcode >> 12) & 0xF;
                    u32 Pn = (opcode >> 8) & 0xF;
                    u32 CP = (opcode >> 5) & 7;
                    u32 Cm = opcode & 0xF;

                    snprintf(dest, dest_size,
                             "cdp%s p%d, #0x%01X, c%d, c%d, c%d, #0x%01X ; [!]",
                             cond, Pn, CP_Opc, Cd, Cn, Cm, CP);
                    gba_dissasemble_add_condition_met(arm_cond_code, address,
                                                      dest, 1, dest_size);
                    return;
                }
            }
        }
    }

    s_strncpy(dest, "Unknown Opcode. [!!!]", dest_size);
}

//------------------------------------------------------------------------------

static const char thumb_alu_operation[16][4] = {
    "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
    "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn"
};

void GBA_DisassembleTHUMB(u16 opcode, u32 address, char *dest, int dest_size)
{
    u16 ident = opcode >> 12;
    opcode &= 0x0FFF;

    switch (ident)
    {
        case 0:
        {
            u16 Rd = opcode & 7;
            u16 Rs = (opcode >> 3) & 7;
            u16 immed = (opcode >> 6) & 0x1F;
            if (opcode & BIT(11))
            {
                // LSR Rd,Rs,#Offset
                if (immed == 0)
                    immed = 32; // LSR#0: Interpreted as LSR#32
                snprintf(dest, dest_size, "lsr r%d, r%d, #0x%02X", Rd, Rs,
                         immed);
                return;
            }
            else
            {
                // LSL Rd,Rs,#Offset
                snprintf(dest, dest_size, "lsl r%d, r%d, #0x%02X", Rd, Rs,
                         immed);
                return;
            }
            break;
        }
        case 1:
        {
            if (opcode & BIT(11))
            {
                u16 Rd = opcode & 7;
                u16 Rs = (opcode >> 3) & 7;

                switch ((opcode >> 9) & 3)
                {
                    case 0: // ADD Rd,Rs,Rn
                    {
                        u16 Rn = (opcode >> 6) & 7;
                        snprintf(dest, dest_size, "add r%d, r%d, r%d",
                                 Rd, Rs, Rn);
                        return;
                    }
                    case 1: // SUB Rd,Rs,Rn
                    {
                        u16 Rn = (opcode >> 6) & 7;
                        snprintf(dest, dest_size, "sub r%d, r%d, r%d",
                                 Rd, Rs, Rn);
                        return;
                    }
                    case 2: // ADD Rd,Rs,#nn
                    {
                        u16 immed = (opcode >> 6) & 0x7;
                        if (immed)
                        {
                            snprintf(dest, dest_size, "add r%d, r%d, #0x%01X",
                                     Rd, Rs, immed);
                        }
                        else
                        {
                            snprintf(dest, dest_size, "mov r%d, r%d", Rd, Rs);
                        }
                        return;
                    }
                    case 3: // SUB Rd,Rs,#nn
                    {
                        u16 immed = (opcode >> 6) & 0x7;
                        snprintf(dest, dest_size, "sub r%d, r%d, #0x%01X",
                                 Rd, Rs, immed);
                        return;
                    }
                }
            }
            else
            {
                // ASR Rd,Rs,#Offset
                u16 Rd = opcode & 7;
                u16 Rs = (opcode >> 3) & 7;
                u16 immed = (opcode >> 6) & 0x1F;

                if (immed == 0)
                    immed = 32; // ASR#0: Interpreted as ASR#32
                snprintf(dest, dest_size, "asr r%d, r%d, #0x%02X",
                         Rd, Rs, immed);
                return;
            }
            break;
        }
        case 2:
        {
            u32 Rd = (opcode >> 8) & 7;
            u32 immed = opcode & 0xFF;

            if (opcode & BIT(11))
            {
                // CMP Rd,#nn
                snprintf(dest, dest_size, "cmp r%d, #0x%02X", Rd, immed);
                return;
            }
            else
            {
                // MOV Rd,#nn
                snprintf(dest, dest_size, "mov r%d, #0x%02X", Rd, immed);
                return;
            }
            break;
        }
        case 3:
        {
            u32 Rd = (opcode >> 8) & 7;
            u32 immed = opcode & 0xFF;

            if (opcode & BIT(11))
            {
                // SUB Rd,#nn
                snprintf(dest, dest_size, "sub r%d, #0x%02X", Rd, immed);
                return;
            }
            else
            {
                // ADD Rd,#nn
                snprintf(dest, dest_size, "add r%d, #0x%02X", Rd, immed);
                return;
            }
            break;
        }
        case 4:
        {
            if (opcode & BIT(11))
            {
                // LDR Rd,[PC,#nn]
                u16 Rd = (opcode >> 8) & 7;
                u32 offset = (opcode & 0xFF) << 2;
                //snprintf(dest, dest_size, "ldr r%d, [pc, #%03X]", Rd, offset);
                //snprintf(dest, dest_size, "ldr r%d, [pc, #0x%03X] =0x%08X",
                //         Rd, offset,
                //         GBA_MemoryRead32(((address + 4) & (~2)) + offset));
                snprintf(dest, dest_size, "ldr r%d, =0x%08X", Rd,
                         GBA_MemoryRead32(((address + 4) & (~2)) + offset));
                return;
            }
            else
            {
                if (opcode & BIT(10))
                {
                    // Hi register operations/branch exchange
                    switch ((opcode >> 8) & 3)
                    {
                        case 0:
                        {
                            if ((opcode >> 6) & 3)
                            {
                                // ADD Rd,Rs
                                u16 Rd = (opcode & 7) | ((opcode >> 4) & 8);
                                u16 Rs = (opcode >> 3) & 0xF;
                                snprintf(dest, dest_size, "add r%d, r%d",
                                         Rd, Rs);
                                return;
                            }
                            else
                            {
                                u16 Rd = (opcode & 7) | ((opcode >> 4) & 8);
                                u16 Rs = (opcode >> 3) & 0xF;
                                snprintf(dest, dest_size,
                                         "add r%d, r%d ; [!] (Unused Opcode #4-0)",
                                         Rd, Rs);
                                return;
                            }
                        }
                        case 1:
                        {
                            if ((opcode >> 6) & 3)
                            {
                                // CMP Rd,Rs
                                u16 Rd = (opcode & 7) | ((opcode >> 4) & 8);
                                u16 Rs = (opcode >> 3) & 0xF;
                                snprintf(dest, dest_size, "cmp r%d, r%d",
                                         Rd, Rs);
                                return;
                            }
                            else
                            {
                                // CMP Rd,Rs
                                u16 Rd = (opcode & 7) | ((opcode >> 4) & 8);
                                u16 Rs = (opcode >> 3) & 0xF;
                                snprintf(dest, dest_size,
                                         "cmp r%d, r%d ; [!] (Unused Opcode #4-1)",
                                         Rd, Rs);
                                return;
                            }
                        }
                        case 2:
                        {
                            if ((opcode >> 6) & 3)
                            {
                                // MOV Rd,Rs
                                u16 Rd = (opcode & 7) | ((opcode >> 4) & 8);
                                u16 Rs = (opcode >> 3) & 0xF;
                                if ((Rd == 8) && (Rs == 8))
                                {
                                    s_strncpy(dest, "nop", dest_size);
                                }
                                else
                                {
                                    snprintf(dest, dest_size, "mov r%d, r%d",
                                             Rd, Rs);
                                }
                                return;
                            }
                            else // Tested in real hardware
                            {
                                // MOV Rd,Rs
                                u16 Rd = (opcode & 7) | ((opcode >> 4) & 8);
                                u16 Rs = (opcode >> 3) & 0xF;
                                if ((Rd == 8) && (Rs == 8))
                                {
                                    s_strncpy(dest,
                                              "nop ; [!] (Unused Opcode #4-2)",
                                              dest_size);
                                }
                                else
                                {
                                    snprintf(dest, dest_size,
                                             "mov r%d, r%d ; [!] (Unused Opcode #4-2)",
                                             Rd, Rs);
                                }
                                return;
                            }
                        }
                        case 3:
                        {
                            if (opcode & 7)
                            {
                                s_strncpy(dest,
                                          "[!] BX/BLX Undefined Opcode #4-3",
                                          dest_size);
                                return;
                            }
                            else
                            {
                                if (opcode & BIT(7))
                                {
                                    // (BLX  Rs), Unpredictable
                                    s_strncpy(dest,
                                              "[!] BLX Rs - Undefined Opcode #4-4",
                                              dest_size);
                                    return;
                                }
                                else
                                {
                                    // BX  Rs
                                    u16 Rs = (opcode >> 3) & 0xF;
                                    if (CPU.R[Rs] & BIT(0))
                                    {
                                        snprintf(dest, dest_size, "bx r%d", Rs);
                                    }
                                    else
                                    {
                                        snprintf(dest, dest_size,
                                                 "bx r%d ; Switch to ARM", Rs);
                                    }
                                    return;
                                }
                            }
                        }
                    }
                    break;
                }
                else
                {
                    // ALU operations
                    u16 Rd = opcode & 7;
                    u16 Rs = (opcode >> 3) & 7;
                    u16 op = (opcode >> 6) & 0xF;
                    snprintf(dest, dest_size, "%s r%d, r%d",
                             thumb_alu_operation[op], Rd, Rs);
                    return;
                }
            }
            break;
        }
        case 5:
        {
            u16 Rd = opcode & 7;
            u16 Rb = (opcode >> 3) & 7;
            u16 Ro = (opcode >> 6) & 7;

            u16 op = (opcode >> 9) & 7;

            static const char thumb_mem_ops[8][6] = {
                "str", "strh", "strb", "ldsb", "ldr", "ldrh", "ldrb", "ldsh"
            };

            snprintf(dest, dest_size, "%s r%d, [r%d, r%d]", thumb_mem_ops[op],
                     Rd, Rb, Ro);
            gba_dissasemble_add_io_register_name(CPU.R[Rb] + CPU.R[Ro], dest, 1,
                                                 dest_size);
            return;
        }
        case 6:
        {
            u16 Rb = (opcode >> 3) & 7;
            u16 Rd = opcode & 7;
            u16 offset = (opcode >> 4) & (0x1F << 2);
            if (opcode & BIT(11))
            {
                // LDR  Rd,[Rb,#nn]
                if (offset)
                {
                    snprintf(dest, dest_size, "ldr r%d, [r%d, #0x%02X]", Rd, Rb,
                             offset);
                }
                else
                {
                    snprintf(dest, dest_size, "ldr r%d, [r%d]", Rd, Rb);
                }
            }
            else
            {
                // STR  Rd,[Rb,#nn]
                if (offset)
                {
                    snprintf(dest, dest_size, "str r%d, [r%d, #0x%02X]", Rd, Rb,
                             offset);
                }
                else
                {
                    snprintf(dest, dest_size, "str r%d, [r%d]", Rd, Rb);
                }
            }

            gba_dissasemble_add_io_register_name(CPU.R[Rb] + offset, dest, 1,
                                                 dest_size);
            return;
        }
        case 7:
        {
            u16 Rb = (opcode >> 3) & 7;
            u16 Rd = opcode & 7;
            u16 offset = (opcode >> 6) & 0x1F;
            if (opcode & BIT(11))
            {
                // LDRB Rd,[Rb,#nn]
                if (offset)
                {
                    snprintf(dest, dest_size, "ldrb r%d, [r%d, #0x%02X]",
                             Rd, Rb, offset);
                }
                else
                {
                    snprintf(dest, dest_size, "ldrb r%d, [r%d]", Rd, Rb);
                }
            }
            else
            {
                // STRB Rd,[Rb,#nn]
                if (offset)
                {
                    snprintf(dest, dest_size, "strb r%d, [r%d, #0x%02X]",
                             Rd, Rb, offset);
                }
                else
                {
                    snprintf(dest, dest_size, "strb r%d, [r%d]", Rd, Rb);
                }
            }

            gba_dissasemble_add_io_register_name(CPU.R[Rb] + offset, dest, 1,
                                                 dest_size);
            return;
        }
        case 8:
        {
            u16 Rd = opcode & 7;
            u16 Rb = (opcode >> 3) & 7;
            u16 offset = (opcode >> 5) & (0x1F << 1);
            if (opcode & BIT(11))
            {
                // LDRH Rd,[Rb,#nn]
                if (offset)
                {
                    snprintf(dest, dest_size, "ldrh r%d, [r%d, #0x%02X]",
                             Rd, Rb, offset);
                }
                else
                {
                    snprintf(dest, dest_size, "ldrh r%d, [r%d]", Rd, Rb);
                }
            }
            else
            {
                // STRH Rd,[Rb,#nn]
                if (offset)
                {
                    snprintf(dest, dest_size, "strh r%d, [r%d, #0x%02X]",
                             Rd, Rb, offset);
                }
                else
                {
                    snprintf(dest, dest_size, "strh r%d, [r%d]", Rd, Rb);
                }
            }

            gba_dissasemble_add_io_register_name(CPU.R[Rb] + offset, dest, 1,
                                                 dest_size);
            return;
        }
        case 9:
        {
            u16 Rd = (opcode >> 8) & 7;
            u16 offset = (opcode & 0xFF) << 2;
            if (opcode & BIT(11))
            {
                // LDR  Rd,[SP,#nn]
                if (offset)
                {
                    snprintf(dest, dest_size, "ldr r%d, [sp, #0x%02X]", Rd,
                             offset);
                }
                else
                {
                    snprintf(dest, dest_size, "ldr r%d, [sp]", Rd);
                }
                return;
            }
            else
            {
                // STR  Rd,[SP,#nn]
                if (offset)
                {
                    snprintf(dest, dest_size, "str r%d, [sp, #0x%02X]", Rd,
                             offset);
                }
                else
                {
                    snprintf(dest, dest_size, "str r%d, [sp]", Rd);
                }
                return;
            }
            break;
        }
        case 0xA:
        {
            u16 Rd = (opcode >> 8) & 7;
            u16 offset = (opcode & 0xFF) << 2;
            if (opcode & BIT(11))
            {
                // ADD  Rd,SP,#nn
                if (offset)
                {
                    snprintf(dest, dest_size, "add r%d, sp, #0x%03X", Rd,
                             offset);
                }
                else
                {
                    snprintf(dest, dest_size, "mov r%d, sp", Rd);
                }
                return;
            }
            else
            {
                // ADD  Rd,PC,#nn
                if (offset)
                {
                    snprintf(dest, dest_size, "add r%d, pc, #0x%03X", Rd,
                             offset);
                }
                else
                {
                    snprintf(dest, dest_size, "mov r%d, pc", Rd);
                }
                return;
            }
            break;
        }
        case 0xB:
        {
            switch ((opcode >> 9) & 7)
            {
                case 0:
                    if (opcode & BIT(8))
                    {
                        break;
                    }
                    else
                    {
                        u16 offset = (opcode & 0x7F) << 2;
                        if (opcode & BIT(7))
                        {
                            // ADD SP,#-nn
                            snprintf(dest, dest_size, "add sp, #-0x%02X",
                                     offset);
                        }
                        else
                        {
                            // ADD SP,#nn
                            snprintf(dest, dest_size, "add sp, #0x%02X",
                                     offset);
                        }
                        return;
                    }
                case 1:
                    break;
                case 2:
                {
                    // PUSH {Rlist}{LR}
                    u32 registers = (opcode & 0xFF)
                                    | ((opcode & BIT(8)) ? BIT(14) : 0);
                    char reglist[128] = "{";
                    for (int i = 0; i < 16; i++)
                    {
                        if (registers & BIT(i))
                        {
                            char reg[4];
                            snprintf(reg, sizeof(reg), "r%d", i);
                            s_strncat(reglist, reg, sizeof(reglist));
                            if ((registers & BIT(i + 1))
                                && (registers & BIT(i + 2)))
                            {
                                s_strncat(reglist, "-", sizeof(reglist));
                                while (registers & BIT(i++))
                                    ;
                                i -= 2;
                                snprintf(reg, sizeof(reg), "r%d", i);
                                s_strncat(reglist, reg, sizeof(reglist));
                            }
                            int j = i + 1;
                            while ((registers & BIT(j)) == 0)
                            {
                                j++;
                                if (j == 16)
                                    break;
                            }
                            if (j < 16)
                                s_strncat(reglist, ",", sizeof(reglist));
                        }
                    }
                    s_strncat(reglist, "}", sizeof(reglist));
                    snprintf(dest, dest_size, "push %s", reglist);
                    return;
                }
                case 3:
                    break;
                case 4:
                    break;
                case 5:
                    break;
                case 6:
                {
                    // POP {Rlist}{PC}
                    u32 registers = (opcode & 0xFF)
                                    | ((opcode & BIT(8)) ? BIT(15) : 0);
                    char reglist[128] = "{";
                    int count = 0;
                    for (int i = 0; i < 16; i++)
                    {
                        if (registers & BIT(i))
                        {
                            count++;
                            char reg[4];
                            snprintf(reg, sizeof(reg), "r%d", i);
                            s_strncat(reglist, reg, sizeof(reglist));
                            if ((registers & BIT(i + 1))
                                && (registers & BIT(i + 2)))
                            {
                                s_strncat(reglist, "-", sizeof(reglist));
                                while (registers & BIT(i++))
                                    ;
                                i -= 2;
                                if (i < 16)
                                    snprintf(reg, sizeof(reg), "r%d", i);
                                else
                                    s_strncat(reglist, "pc", sizeof(reglist));
                                s_strncat(reglist, reg, sizeof(reglist));
                            }

                            int j = i + 1;
                            while ((registers & BIT(j)) == 0)
                            {
                                j++;
                                if (j == 16)
                                    break;
                            }
                            if (j < 16)
                                s_strncat(reglist, ",", sizeof(reglist));
                        }
                    }
                    s_strncat(reglist, "}", sizeof(reglist));
                    snprintf(dest, dest_size, "pop %s", reglist);
                    if (count == 0)
                        s_strncat(dest, " ; [!] Undefined opcode!", dest_size);
                    return;
                }
                case 7:
                    if ((opcode & BIT(8)) == 0)
                    {
                        s_strncpy(dest, "[!] Undefined Opcode #B", dest_size);
                        return;
                    }
                    else
                    {
                        break;
                    }
            }

            s_strncpy(dest, "[!] Undefined Opcode #B", dest_size);
            return;
        }
        case 0xC:
        {
            char reglist[128] = "{";
            for (int i = 0; i < 8; i++)
            {
                if (opcode & BIT(i))
                {
                    char reg[4];
                    snprintf(reg, sizeof(reg), "r%d", i);
                    s_strncat(reglist, reg, sizeof(reglist));
                    if ((opcode & BIT(i + 1)) && (opcode & BIT(i + 2)))
                    {
                        s_strncat(reglist, "-", sizeof(reglist));
                        while (opcode & BIT(i++))
                            ;
                        i -= 2;
                        snprintf(reg, sizeof(reg), "r%d", i);
                        s_strncat(reglist, reg, sizeof(reglist));
                    }

                    int j = i + 1;
                    while ((opcode & BIT(j)) == 0)
                    {
                        j++;
                        if (j == 8)
                            break;
                    }
                    if (j < 8)
                        s_strncat(reglist, ",", sizeof(reglist));
                }
            }
            s_strncat(reglist, "}", sizeof(reglist));

            u16 Rb = (opcode >> 8) & 7;

            if (opcode & BIT(11))
            {
                // LDMIA Rb!,{Rlist}
                snprintf(dest, dest_size, "ldmia r%d!,%s", Rb, reglist);
                if ((opcode & 0xFF) == 0)
                    s_strncat(dest, " ; [!] Undefined opcode", dest_size);
            }
            else
            {
                // STMIA Rb!,{Rlist}
                snprintf(dest, dest_size, "stmia r%d!,%s", Rb, reglist);
                if ((opcode & 0xFF) == 0)
                    s_strncat(dest, " ; [!] Rb += 0x40", dest_size);
            }
            return;
        }
        case 0xD:
        {
            u16 cond = opcode >> 8;
            u16 data = opcode & 0xFF;
            if (cond == 15)
            {
                // SWI nn
                snprintf(dest, dest_size, "swi #0x%02X", data);
                gba_disassemble_swi_name(data, dest, 1, dest_size);
                return;
            }
            else if (cond == 14)
            {
                // Undefined opcode -- Tested in real hardware
                u32 addr_dest = address + 4 + (((s16)(s8)data) << 1);
                snprintf(dest, dest_size,
                         "b{al} #0x%08X ; [!] (Undefined Opcode #D)",
                         addr_dest);
                if (addr_dest > address)
                    s_strncat(dest, " ; " STR_SLIM_ARROW_DOWN, dest_size);
                else if (addr_dest == address)
                    s_strncat(dest, " ; <-", dest_size);
                else
                    s_strncat(dest, " ; " STR_SLIM_ARROW_UP, dest_size);
                return;
            }
            else
            {
                // B{cond} label
                u32 addr_dest = address + 4 + (((s16)(s8)data) << 1);
                snprintf(dest, dest_size, "b%s #0x%08X", arm_cond[cond],
                         addr_dest);
                if (addr_dest > address)
                    s_strncat(dest, " ; " STR_SLIM_ARROW_DOWN, dest_size);
                else if (addr_dest == address)
                    s_strncat(dest, " ; <-", dest_size);
                else
                    s_strncat(dest, " ; " STR_SLIM_ARROW_UP, dest_size);
                gba_dissasemble_add_condition_met(cond, address, dest, 0,
                                                  dest_size);
                return;
            }
            break;
        }
        case 0xE:
        {
            if (opcode & BIT(11))
            {
                s_strncpy(dest, "[!] Undefined Opcode #E", dest_size);
                return;
            }
            else
            {
                // B label
                s32 offset = (opcode & 0x3FF) << 1;
                if (offset & BIT(10))
                {
                    offset |= 0xFFFFF800;
                }
                u32 addr_dest = address + 4 + offset;
                snprintf(dest, dest_size, "b #0x%08X", addr_dest);
                if (addr_dest > address)
                    s_strncat(dest, " ; " STR_SLIM_ARROW_DOWN, dest_size);
                else if (addr_dest == address)
                    s_strncat(dest, " ; <-", dest_size);
                else
                    s_strncat(dest, " ; " STR_SLIM_ARROW_UP, dest_size);
                return;
            }
            break;
        }
        case 0xF:
        {
            // BL label
            if (opcode & BIT(11)) // Second part
            {
                u16 prev = GBA_MemoryRead16(address - 2);
                if ((prev & 0xF800) == 0xF000)
                {
                    s_strncpy(dest, "bl (2nd part)", dest_size);
                    return;
                }
                else
                {
                    s_strncpy(dest, "bl (2nd part) ; [!] corrupted!",
                              dest_size);
                    return;
                }
            }
            else // First part
            {
                u16 next = GBA_MemoryRead16(address + 2);
                if ((next & 0xF800) == 0xF800)
                {
                    u32 addr = address + 4
                               + ((((u32)opcode & 0x7FF) << 12)
                                  | ((u32)opcode & BIT(10) ? 0xFF800000 : 0))
                               + ((next & 0x7FF) << 1);
                    snprintf(dest, dest_size, "bl #0x%08X", addr);
                    s_strncat(dest, " ; ->", dest_size);
                    return;
                }
                else
                {
                    s_strncpy(dest, "bl (1st part) ; [!] corrupted!",
                              dest_size);
                    return;
                }
            }
            break;
        }
    }

    s_strncpy(dest, "Unknown Opcode. [!!!]", dest_size);
}
