/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

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

#include "../build_options.h"
#include "../debug_utils.h"

#include "gba.h"
#include "bios.h"
#include "cpu.h"
#include "shifts.h"
#include "memory.h"
#include "interrupts.h"
#include "disassembler.h"

#include "../gui/win_gba_debugger.h"

//-----------------------------------------------------------------------------------------------

inline u32 arm_check_condition(u32 cond)
{
    if(cond == 14) return 1; //This is the most used

    switch(cond)
    {
        case 0: return (CPU.CPSR & F_Z);
        case 1: return ((CPU.CPSR & F_Z) == 0);
        case 2: return (CPU.CPSR & F_C);
        case 3: return ((CPU.CPSR & F_C) == 0);
        case 4: return (CPU.CPSR & F_N);
        case 5: return ((CPU.CPSR & F_N) == 0);
        case 6: return (CPU.CPSR & F_V);
        case 7: return ((CPU.CPSR & F_V) == 0);
        case 8: return ((CPU.CPSR & (F_C|F_Z)) == F_C);
        case 9: return ( ((CPU.CPSR & F_C) == 0) || (CPU.CPSR & F_Z) );
        case 10: return !( ((CPU.CPSR & F_N) != 0) ^ ((CPU.CPSR & F_V) != 0) );
        case 11: return ( ((CPU.CPSR & F_N) != 0) ^ ((CPU.CPSR & F_V) != 0) );
        case 12: return (  ((CPU.CPSR & F_Z) == 0) && !( ((CPU.CPSR & F_N) != 0) ^ ((CPU.CPSR & F_V) != 0) )  );
        case 13: return ( (CPU.CPSR & F_Z) || ( ((CPU.CPSR & F_N) != 0) ^ ((CPU.CPSR & F_V) != 0) ) );
        case 14: return 1;
        default:
        case 15: return 0; //(ARMv1,v2 only) (Reserved ARMv3 and up)
    }

    return 0;
}

//---------------------------------------------------------------------------------------------

#include "arm_alu.h"

//---------------------------------------------------------------------------------------------

static inline u32 arm_bit_count(u16 value)
{
    //u32 count = 0;
    //int i = 16;
    //while(i--) { count += (value & 1); value >>= 1; }
    //return count;
    u32 count = (value & 1); value >>= 1;   count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += value;
    return count;
}

static inline void arm_ldm(u32 address, u32 reg)
{
    CPU.R[reg] = GBA_MemoryRead32(address&~3);
}

static inline void arm_stm(u32 address, u32 reg)
{
    GBA_MemoryWrite32(address&~3,CPU.R[reg]+((reg==15)?12:0));
}

//---------------------------------------------------------------------------------------------

#define ARM_UNDEFINED_INSTRUCTION() \
{ \
    Debug_DebugMsgArg("Undefined instruction.\nARM: [0x%08X]=0x%08X",CPU.R[R_PC],GBA_MemoryRead32(CPU.R[R_PC]));\
    GBA_CPUChangeMode(M_UNDEFINED); \
    CPU.R[14] = CPU.R[R_PC]+4; \
    CPU.SPSR = CPU.CPSR; \
    CPU.CPSR &= ~((0x1F)|F_T|F_I); \
    CPU.CPSR |= M_UNDEFINED|F_I; \
    CPU.R[R_PC] = 0; \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + \
                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]) + 1 ; \
    return clocks; \
} // CPU.R[R_PC] = 4; CPU.R[R_PC] -= 4;
// 2S + 1I + 1N cycles
// GBA_ExecutionBreak(); -> not needed

//---------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------

extern u32 cpu_loop_break;
inline s32 GBA_ExecuteARM(s32 clocks) //returns residual clocks
{
    //static const u32 reg_add_4[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,4};
    //static const u32 reg_add_8[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,8};

    while(clocks > 0)
    {
        if(GBA_DebugCPUIsBreakpoint(CPU.R[R_PC]))
        {
            cpu_loop_break = 1;
            GBA_RunFor_ExecutionBreak();
            Win_GBADisassemblerSetFocus();
        }

        if(cpu_loop_break) { cpu_loop_break = 0; return clocks; }

        //CPU.R[R_PC] &= ~3;

        u32 PCseq = ((CPU.OldPC+4) == CPU.R[R_PC]);
        CPU.OldPC = CPU.R[R_PC];

        u32 opcode = GBA_MemoryReadFast32(CPU.R[R_PC]);

        if(arm_check_condition((opcode >> 28)&0xF))
        {
            u32 ident = (opcode >> 25) & 7;
            opcode &= 0x01FFFFFF;

            switch(ident)
            {
                case 0:
                {
                    if(opcode & BIT(4))
                    {
                        if(opcode & BIT(7))
                        {
                            //X.24.23.22:21.20.6.5
                            u32 switchval = ((opcode>>5)&3) | ((opcode>>18)&0x7C);
                            switch(switchval)
                            {
                                // Multiplication
                                //----------------
                                //u32 Rd = (opcode>>16)&0xF; // |            (or RdHi)
                                //u32 Rn = (opcode>>12)&0xF; // | r0 - r14   (or RdLo)
                                //u32 Rs = (opcode>>8)&0xF;  // |
                                //u32 Rm = opcode&0xF;       // |
                                case 0x00: //MUL{cond}
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_signed_mul_extra_cycles((opcode>>16)&0xF); //1S+mI
                                    arm_mul((opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x04: //MUL{cond}S
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_signed_mul_extra_cycles((opcode>>16)&0xF); //1S+mI
                                    arm_muls((opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x08: //MLA{cond}
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_signed_mul_extra_cycles((opcode>>16)&0xF) + 1; //1S+mI+1I
                                    arm_mla((opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF,(opcode>>12)&0xF);
                                    break;
                                }
                                case 0x0C: //MLA{cond}S
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_signed_mul_extra_cycles((opcode>>16)&0xF) + 1; //1S+mI+1I
                                    arm_mlas((opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF,(opcode>>12)&0xF);
                                    break;
                                }
                                case 0x10: case 0x14: case 0x18: case 0x1C:
                                {
                                    CPU.R[R_PC] -= 4; //Undefined Instruction #0-2
                                    clocks -= 100; // DON'T TRAP!!! ???
                                    break;
                                }
                                case 0x20: //UMULL{cond}
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_unsigned_mul_extra_cycles((opcode>>16)&0xF) + 1; //1S+mI+1I
                                    arm_umull((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x24: //UMULL{cond}S
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_unsigned_mul_extra_cycles((opcode>>16)&0xF) + 1; //1S+mI+1I
                                    arm_umulls((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x28: //UMLAL{cond}
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_unsigned_mul_extra_cycles((opcode>>16)&0xF) + 2; //1S+mI+2I
                                    arm_umlal((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x2C: //UMLAL{cond}S
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_unsigned_mul_extra_cycles((opcode>>16)&0xF) + 2; //1S+mI+2I
                                    arm_umlals((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x30: //SMULL{cond}
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                            arm_signed_mul_extra_cycles((opcode>>16)&0xF) + 1; //1S+mI+1I
                                    arm_smull((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x34: //SMULL{cond}S
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                            arm_signed_mul_extra_cycles((opcode>>16)&0xF) + 1; //1S+mI+1I
                                    arm_smulls((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x38: //SMLAL{cond}
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_signed_mul_extra_cycles((opcode>>16)&0xF) + 2; //1S+mI+2I
                                    arm_smlal((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }
                                case 0x3C: //SMLAL{cond}S
                                {
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        arm_signed_mul_extra_cycles((opcode>>16)&0xF) + 2; //1S+mI+2I
                                    arm_smlals((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>8)&0xF);
                                    break;
                                }

                                // Single Data Swap (SWP)
                                //-------------------------

                                case 0x60: case 0x64: case 0x68: case 0x6C:
                                case 0x70: case 0x74: case 0x78: case 0x7C:
                                {
                                    CPU.R[R_PC] -= 4; //Undefined Instruction #0-1
                                    clocks -= 100; // DON'T TRAP!!! ???
                                    break;
                                }

                                case 0x40: case 0x44: case 0x48: case 0x4C:
                                {
                                    if( (opcode & (0xF00|(3<<20))) )
                                    {
                                        CPU.R[R_PC] -= 4; //Undefined Instruction #0-1
                                        clocks -= 100; // DON'T TRAP!!! ???
                                        break;
                                    }
                                    //SWP{cond} Rd,Rm,[Rn]
                                    u32 val = CPU.R[opcode&0xF];
                                    u32 addr = CPU.R[(opcode>>16)&0xF];
                                    CPU.R[(opcode>>12)&0xF] = GBA_MemoryRead32(addr);
                                    GBA_MemoryWrite32(addr,val);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        (2 * GBA_MemoryGetAccessCyclesNoSeq32(addr)) + 1; //1S+2N+1I

                                    break;
                                }
                                case 0x50: case 0x54: case 0x58: case 0x5C:
                                {
                                    if( (opcode & (0xF00|(3<<20))) )
                                    {
                                        CPU.R[R_PC] -= 4; //Undefined Instruction #0-1
                                        clocks -= 100; // DON'T TRAP!!! ???
                                        break;
                                    }
                                    //SWP{cond}B Rd,Rm,[Rn]
                                    u32 val = CPU.R[opcode&0xF];
                                    u32 addr = CPU.R[(opcode>>16)&0xF];
                                    CPU.R[(opcode>>12)&0xF] = (u32)(u8)GBA_MemoryRead8(addr);
                                    GBA_MemoryWrite8(addr,(u8)val);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]) +
                                        (2 * GBA_MemoryGetAccessCyclesNoSeq16(addr)) + 1; //1S+2N+1I
                                    break;
                                }

                                // Halfword and Signed Data Transfer
                                //-----------------------------------

                                //Execution Time: For Normal LDR, 1S+1N+1I.
                                //                For LDR PC, 2S+2N+1I. For STRH 2N.

                                //STR: opcodes 2 and 3, undefined -- 0 reserved for SWP and MULs
                                case 0x02: case 0x0A: case 0x03: case 0x0B:
                                case 0x12: case 0x1A: case 0x13: case 0x1B:
                                case 0x22: case 0x2A: case 0x23: case 0x2B:
                                case 0x32: case 0x3A: case 0x33: case 0x3B:
                                case 0x42: case 0x4A: case 0x43: case 0x4B:
                                case 0x52: case 0x5A: case 0x53: case 0x5B:
                                case 0x62: case 0x6A: case 0x63: case 0x6B:
                                case 0x72: case 0x7A: case 0x73: case 0x7B:
                                {
                                    CPU.R[R_PC] -= 4; //Undefined Instruction
                                    clocks -= 100; // DON'T TRAP!!! ???
                                    break;
                                }
                                //In post-indexing mode, writeback always enabled
                                case 0x01: case 0x09: //STR{cond}H  Rd,[Rn],-Rm
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    CPU.R[Rn] = address-CPU.R[Rm];
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x05: case 0x0D: //LDR{cond}H  Rd,[Rn],-Rm
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    CPU.R[Rn] = address-CPU.R[Rm];
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x06: case 0x0E: //LDR{cond}SB Rd,[Rn],-Rm
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    CPU.R[Rn] = address-CPU.R[Rm];
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x07: case 0x0F: //LDR{cond}SH Rd,[Rn],-Rm
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    CPU.R[Rn] = address-CPU.R[Rm];
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x11: case 0x19: //STR{cond}H  Rd,[Rn],-#
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    CPU.R[Rn] = address-offset;
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x15: case 0x1D: //LDR{cond}H  Rd,[Rn],-#
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    CPU.R[Rn] = address-offset;
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x16: case 0x1E: //LDR{cond}SB Rd,[Rn],-#
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    CPU.R[Rn] = address-offset;
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x17: case 0x1F: //LDR{cond}SH Rd,[Rn],-#
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    CPU.R[Rn] = address-offset;
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x21: case 0x29: //STR{cond}H  Rd,[Rn],Rm
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    CPU.R[Rn] = address+CPU.R[Rm];
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x25: case 0x2D: //LDR{cond}H  Rd,[Rn],Rm
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    CPU.R[Rn] = address+CPU.R[Rm];
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x26: case 0x2E: //LDR{cond}SB Rd,[Rn],Rm
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    CPU.R[Rn] = address+CPU.R[Rm];
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x27: case 0x2F: //LDR{cond}SH Rd,[Rn],Rm
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    CPU.R[Rn] = address+CPU.R[Rm];
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x31: case 0x39: //STR{cond}H  Rd,[Rn],#
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    CPU.R[Rn] = address+offset;
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x35: case 0x3D: //LDR{cond}H  Rd,[Rn],#
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    CPU.R[Rn] = address+offset;
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x36: case 0x3E: //LDR{cond}SB Rd,[Rn],#
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    CPU.R[Rn] = address+offset;
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x37: case 0x3F: //LDR{cond}SH Rd,[Rn],#
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    CPU.R[Rn] = address+offset;
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                //In pre-indexing, writeback and no-writeback versions are different
                                case 0x41: //STR{cond}H  Rd,[Rn,-Rm]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address -= CPU.R[Rm];
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    break;
                                }
                                case 0x45: //LDR{cond}H  Rd,[Rn,-Rm]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address -= CPU.R[Rm];
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x46: //LDR{cond}SB Rd,[Rn,-Rm]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address -= CPU.R[Rm];
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x47: //LDR{cond}SH Rd,[Rn,-Rm]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address -= CPU.R[Rm];
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x49: //STR{cond}H  Rd,[Rn,-Rm]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address -= CPU.R[Rm];
                                    CPU.R[Rn] = address;
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x4D: //LDR{cond}H  Rd,[Rn,-Rm]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address -= CPU.R[Rm];
                                    CPU.R[Rn] = address;
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x4E: //LDR{cond}SB Rd,[Rn,-Rm]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address -= CPU.R[Rm];
                                    CPU.R[Rn] = address;
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x4F: //LDR{cond}SH Rd,[Rn,-Rm]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address -= CPU.R[Rm];
                                    CPU.R[Rn] = address;
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x51: //STR{cond}H  Rd,[Rn,-#]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address -= offset;
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    break;
                                }
                                case 0x55: //LDR{cond}H  Rd,[Rn,-#]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address -= offset;
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x56: //LDR{cond}SB Rd,[Rn,-#]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address -= offset;
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x57: //LDR{cond}SH Rd,[Rn,-#]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address -= offset;
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x59: //STR{cond}H  Rd,[Rn,-#]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address -= offset;
                                    CPU.R[Rn] = address;
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x5D: //LDR{cond}H  Rd,[Rn,-#]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address -= offset;
                                    CPU.R[Rn] = address;
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x5E: //LDR{cond}SB Rd,[Rn,-#]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address -= offset;
                                    CPU.R[Rn] = address;
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x5F: //LDR{cond}SH Rd,[Rn,-#]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address -= offset;
                                    CPU.R[Rn] = address;
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x61: //STR{cond}H  Rd,[Rn,Rm]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address += CPU.R[Rm];
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    break;
                                }
                                case 0x65: //LDR{cond}H  Rd,[Rn,Rm]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address += CPU.R[Rm];
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x66: //LDR{cond}SB Rd,[Rn,Rm]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address += CPU.R[Rm];
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x67: //LDR{cond}SH Rd,[Rn,Rm]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address += CPU.R[Rm];
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x69: //STR{cond}H  Rd,[Rn,Rm]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address += CPU.R[Rm];
                                    CPU.R[Rn] = address;
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x6D: //LDR{cond}H  Rd,[Rn,Rm]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address += CPU.R[Rm];
                                    CPU.R[Rn] = address;
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x6E: //LDR{cond}SB Rd,[Rn,Rm]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address += CPU.R[Rm];
                                    CPU.R[Rn] = address;
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x6F: //LDR{cond}SH Rd,[Rn,Rm]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 Rm = opcode&0xF; //(not including R15)
                                    address += CPU.R[Rm];
                                    CPU.R[Rn] = address;
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x71: //STR{cond}H  Rd,[Rn,#]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address += offset;
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    break;
                                }
                                case 0x75: //LDR{cond}H  Rd,[Rn,#]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address += offset;
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x76: //LDR{cond}SB Rd,[Rn,#]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address += offset;
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x77: //LDR{cond}SH Rd,[Rn,#]
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address += offset;
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    break;
                                }
                                case 0x79: //STR{cond}H  Rd,[Rn,#]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address += offset;
                                    CPU.R[Rn] = address;
                                    GBA_MemoryWrite16(address,CPU.R[Rd]+((Rd==R_PC)?12:0));
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesNoSeq16(address); //2N
                                    if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x7D: //LDR{cond}H  Rd,[Rn,#]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address += offset;
                                    CPU.R[Rn] = address;
                                    CPU.R[Rd] = (u32)GBA_MemoryRead16(address&~1);
                                    if(address&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x7E: //LDR{cond}SB Rd,[Rn,#]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address += offset;
                                    CPU.R[Rn] = address;
                                    CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                case 0x7F: //LDR{cond}SH Rd,[Rn,#]!
                                {
                                    u32 Rn = (opcode>>16)&0xF; //(including R15=PC+8)
                                    u32 Rd = (opcode>>12)&0xF; //(including R15=PC+12)
                                    u32 address = CPU.R[Rn]+((Rn==R_PC)?8:0);
                                    u32 offset = ((opcode>>4)&0xF0)|(opcode&0xF);
                                    address += offset;
                                    CPU.R[Rn] = address;
                                    if(address&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(address);
                                    else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(address);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                                    if(Rd == 15)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                        CPU.R[R_PC] -= 4;
                                    }
                                    else if(Rn == R_PC) CPU.R[R_PC]-=4;
                                    break;
                                }
                                default: //???
                                {
                                    CPU.R[R_PC] -= 4;
                                    clocks -= 100;
                                    break;
                                }
                            }
                            break;
                        }
                        else
                        {
                            switch(opcode >> 20)
                            {
                                case 0x12: //BX{cond} Rn
                                {
                                    if((opcode & 0x000FFFF0) == 0x000FFF10)
                                    {
                                        u32 Rn = opcode&0xF;
                                        if(CPU.R[Rn] & 1) //Switch to THUMB
                                        {
                                            CPU.EXECUTION_MODE = EXEC_THUMB;
                                            CPU.CPSR |= F_T;
                                            CPU.R[R_PC] = (CPU.R[Rn]+(Rn==R_PC?8:0))&(~1);
                                            clocks -= (2 * GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC)) +
                                                GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]); //2S+1N

                                            return GBA_ExecuteTHUMB(clocks);
                                        }
                                        else CPU.R[R_PC] = (CPU.R[Rn]+(Rn==R_PC?8:0))&(~3);
                                        CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                                        clocks -= (2 * GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC)) +
                                                GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]); //2S+1N
                                        break;
                                    }
                                    else
                                    {
                                        CPU.R[R_PC] -= 4; //Undefined Instruction
                                        clocks -= 100; // DON'T TRAP!!! ???
                                        break;
                                    }
                                }

                                //Data Processing, (Register shifted by Register) 2nd Operand
                                //-----------------------------------------------------------

                                //Execution time: 1S+1I+y
                                //if 8~B -> y = 0;
                                //else if (Rd=R15). y=1S+1N
                                case 0x00: arm_and_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x01: arm_ands_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x02: arm_eor_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x03: arm_eors_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x04: arm_sub_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x05: arm_subs_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x06: arm_rsb_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x07: arm_rsbs_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x08: arm_add_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x09: arm_adds_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x0A: arm_adc_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x0B: arm_adcs_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x0C: arm_sbc_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x0D: arm_sbcs_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x0E: arm_rsc_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x0F: arm_rscs_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x11: arm_tst_rshiftr((opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        break;
                                case 0x13: arm_teq_rshiftr((opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        break;
                                case 0x15: arm_cmp_rshiftr((opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        break;
                                case 0x17: arm_cmn_rshiftr((opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        break;
                                case 0x18: arm_orr_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x19: arm_orrs_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x1A: arm_mov_rshiftr((opcode>>12)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x1B: arm_movs_rshiftr((opcode>>12)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x1C: arm_bic_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x1D: arm_bics_rshiftr((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x1E: arm_mvn_rshiftr((opcode>>12)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x1F: arm_mvns_rshiftr((opcode>>12)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>8)&0xF);
                                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                        if(((opcode>>12)&0xF) == 0xF)
                                        {
                                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                                GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                            if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                        }
                                        break;
                                case 0x10: case 0x14: case 0x16:
                                    CPU.R[R_PC] -= 4; //Undefined Instruction
                                    clocks -= 100; // DON'T TRAP!!! ???
                                    break;
                                default: //???
                                    CPU.R[R_PC] -= 4;
                                    clocks -= 100;
                                    break;
                            }
                            break;
                        }
                    }
                    else
                    {
                        switch(opcode >> 20)
                        {
                            //PSR Transfer (MRS, MSR)
                            //-----------------------

                            case 0x10: //MRS{cond} Rd,cpsr
                            {
                            //    if( (opcode & ((0x3F<<16)|0xFFF)) == ((0x0F<<16)) )
                            //    {
                                CPU.R[(opcode>>12)&0xF] = CPU.CPSR;
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                            //    }
                            //    else  { Undefined Instruction -- DON'T TRAP!!! ??? }
                                break;
                            }
                            case 0x14: //MRS{cond} Rd,spsr
                            {
                            //    if( (opcode & ((0x3F<<16)|0xFFF)) == ((0x0F<<16)) )
                            //    {
                                if(CPU.MODE == CPU_USER || CPU.MODE == CPU_SYSTEM)
                                {
                                    Debug_DebugMsgArg("mrs Rd,spsr -- in user/system mode.\npc = 0x%08X",CPU.R[R_PC]);
                                    GBA_ExecutionBreak();
                                }
                                else
                                    CPU.R[(opcode>>12)&0xF] = CPU.SPSR;
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                            //    }
                            //    else  { Undefined Instruction -- DON'T TRAP!!! ??? }
                                break;
                            }
                            case 0x12: // MSR{cond} cpsr{_field},Rm
                            {
                                u32 value = CPU.R[opcode&0xF];
                                u32 result = 0;
                                if(opcode & BIT(19)) { result |= value&0xFF000000; CPU.CPSR &= 0x00FFFFFF; }
                                if(CPU.MODE != CPU_USER)
                                {
                                    if(opcode & BIT(18)) { result |= value&0x00FF0000; CPU.CPSR &= 0xFF00FFFF; }
                                    if(opcode & BIT(17)) { result |= value&0x0000FF00; CPU.CPSR &= 0xFFFF00FF; }
                                    if(opcode & BIT(16))
                                    {
                                        result |= value&0x000000FF;
                                        CPU.CPSR &= 0xFFFFFF00;
                                        CPU.CPSR |= result;
                                        GBA_CPUChangeMode(result&0x1F);
                                    }
                                    else { CPU.CPSR |= result; }
                                    GBA_ExecutionBreak();
                                }
                                else { CPU.CPSR |= result; }
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                break;
                            }
                            case 0x16: // MSR{cond} spsr{_field},Rm
                            {
                                if(CPU.MODE == CPU_USER || CPU.MODE == CPU_SYSTEM)
                                {
                                    Debug_DebugMsgArg("msr spsr,Rm -- in user/system mode.\npc = 0x%08X",CPU.R[R_PC]);
                                    GBA_ExecutionBreak();
                                    clocks--;
                                    break;
                                }
                                u32 value = CPU.R[opcode&0xF];
                                u32 result = 0;
                                if(opcode & BIT(19)) { result |= value&0xFF000000; CPU.SPSR &= 0x00FFFFFF; }
                                if(opcode & BIT(18)) { result |= value&0x00FF0000; CPU.SPSR &= 0xFF00FFFF; }
                                if(opcode & BIT(17)) { result |= value&0x0000FF00; CPU.SPSR &= 0xFFFF00FF; }
                                if(opcode & BIT(16)) { result |= value&0x000000FF; CPU.SPSR &= 0xFFFFFF00; }
                                CPU.SPSR |= result;
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                break;
                            }

                            //Data Processing, (Shift by Inmediate) 2nd Operand
                            //-------------------------------------------------

                            //Execution time: 1S+y
                            //if 8~B -> y = 0;
                            //else if (Rd=R15). y=1S+1N
                            case 0x00: arm_and_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x01: arm_ands_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x02: arm_eor_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x03: arm_eors_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x04: arm_sub_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x05: arm_subs_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x06: arm_rsb_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x07: arm_rsbs_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x08: arm_add_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x09: arm_adds_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x0A: arm_adc_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x0B: arm_adcs_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x0C: arm_sbc_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x0D: arm_sbcs_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x0E: arm_rsc_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x0F: arm_rscs_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x11: arm_tst_rshifti((opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    break;
                            case 0x13: arm_teq_rshifti((opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    break;
                            case 0x15: arm_cmp_rshifti((opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    break;
                            case 0x17: arm_cmn_rshifti((opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    break;
                            case 0x18: arm_orr_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x19: arm_orrs_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x1A: arm_mov_rshifti((opcode>>12)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x1B: arm_movs_rshifti((opcode>>12)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x1C: arm_bic_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x1D: arm_bics_rshifti((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x1E: arm_mvn_rshifti((opcode>>12)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            case 0x1F: arm_mvns_rshifti((opcode>>12)&0xF,opcode&0xF,(opcode>>5)&3,(opcode>>7)&0x1F);
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                    if(((opcode>>12)&0xF) == 0xF)
                                    {
                                        clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                            GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                        if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                    }
                                    break;
                            default: //???
                            {
                                CPU.R[R_PC] -= 4;
                                clocks -= 100;
                                break;
                            }
                        }
                        break;
                    }
                    break;
                }
                case 1:
                {
                    switch(opcode >> 20)
                    {
                        //PSR Transfer (MSR)
                        //-----------------------

                        case 0x12: // MSR{cond} CPSR{_field},Imm
                        {
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                            u32 value = ror_immed_no_carry(opcode&0xFF,((opcode>>8)&0xF)<<1);
                            u32 result = 0;
                            if(opcode & BIT(19)) { result |= value&0xFF000000; CPU.CPSR &= 0x00FFFFFF; }
                            if(CPU.MODE != CPU_USER)
                            {
                                if(opcode & BIT(18)) { result |= value&0x00FF0000; CPU.CPSR &= 0xFF00FFFF; }
                                if(opcode & BIT(17)) { result |= value&0x0000FF00; CPU.CPSR &= 0xFFFF00FF; }
                                if(opcode & BIT(16))
                                {
                                    result |= value&0x000000FF;
                                    CPU.CPSR &= 0xFFFFFF00;
                                    CPU.CPSR |= result;
                                    GBA_CPUChangeMode(result&0x1F);
                                }
                                else { CPU.CPSR |= result; }
                                GBA_ExecutionBreak();
                            }
                            else { CPU.CPSR |= result; }
                            break;
                        }
                        case 0x16: // MSR{cond} SPSR{_field},Imm
                        {
                            if(CPU.MODE == CPU_USER || CPU.MODE == CPU_SYSTEM)
                            {
                                Debug_DebugMsgArg("msr spsr,Inm -- in user/system mode.\npc = 0x%08X",CPU.R[R_PC]);
                                GBA_ExecutionBreak();
                                clocks--;
                                break;
                            }
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                            u32 value = ror_immed_no_carry(opcode&0xFF,((opcode>>8)&0xF)<<1);
                            u32 result = 0;
                            if(opcode & BIT(19)) { result |= value&0xFF000000; CPU.SPSR &= 0x00FFFFFF; }
                            if(opcode & BIT(18)) { result |= value&0x00FF0000; CPU.SPSR &= 0xFF00FFFF; }
                            if(opcode & BIT(17)) { result |= value&0x0000FF00; CPU.SPSR &= 0xFFFF00FF; }
                            if(opcode & BIT(16)) { result |= value&0x000000FF; CPU.SPSR &= 0xFFFFFF00; }
                            CPU.SPSR |= result;
                            break;
                        }

                        //Data Processing, Immediate 2nd Operand
                        //--------------------------------------

                        //Execution time: 1S+y
                        //if 8~B -> y = 0;
                        //else if (Rd=R15). y=1S+1N
                        case 0x00: arm_and_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x01: arm_ands_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x02: arm_eor_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x03: arm_eors_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x04: arm_sub_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x05: arm_subs_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x06: arm_rsb_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x07: arm_rsbs_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x08: arm_add_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x09: arm_adds_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x0A: arm_adc_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x0B: arm_adcs_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x0C: arm_sbc_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x0D: arm_sbcs_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x0E: arm_rsc_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x0F: arm_rscs_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x11: arm_tst_immed((opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                break;
                        case 0x13: arm_teq_immed((opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                break;
                        case 0x15: arm_cmp_immed((opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                break;
                        case 0x17: arm_cmn_immed((opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                break;
                        case 0x18: arm_orr_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x19: arm_orrs_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x1A: arm_mov_immed((opcode>>12)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x1B: arm_movs_immed((opcode>>12)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x1C: arm_bic_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x1D: arm_bics_immed((opcode>>12)&0xF,(opcode>>16)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x1E: arm_mvn_immed((opcode>>12)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x1F: arm_mvns_immed((opcode>>12)&0xF,opcode&0xFF,(opcode>>7)&0x1E);
                                clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
                                if(((opcode>>12)&0xF) == 0xF)
                                {
                                    clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //1S+1N
                                    if(CPU.CPSR&F_T) { CPU.EXECUTION_MODE = EXEC_THUMB; CPU.R[R_PC] += 4; return GBA_ExecuteTHUMB(clocks); }
                                }
                                break;
                        case 0x10: case 0x14:
                            ARM_UNDEFINED_INSTRUCTION(); //Undefined Instruction
                            break;
                         default: //???
                            CPU.R[R_PC] -= 4;
                            clocks -= 100;
                            break;
                    }
                    break;
                }
                case 2:
                {
                    // LDR/STR -- INMEDIATE AS OFFSET
                    u32 Rn = ((opcode>>16)&0xF); //(including R15=PC+8)
                    u32 Rd = ((opcode>>12)&0xF); //(including R15=PC+12)
                    s32 offset = opcode & 0xFFF;

                    switch(opcode>>20)
                    {
                        case 0x00: // STR{cond} Rd, [Rn], -#
                        case 0x02: // STR{cond}T Rd, [Rn], -#
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            CPU.R[Rn] = address - offset;
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            break;
                        }
                        case 0x01: // LDR{cond} Rd, [Rn], -#
                        case 0x03: // LDR{cond}T Rd, [Rn], -#
                        {
                            //int oldmode = CPU.CPSR&0x1F;
                            //GBA_CPUChangeMode(M_USER);GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            CPU.R[Rn] = address - offset;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }

                            //GBA_CPUChangeMode(oldmode);
                            break;
                        }
                        case 0x04: // STR{cond}B Rd, [Rn], -#
                        case 0x06: // STR{cond}BT Rd, [Rn], -#
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            CPU.R[Rn] = address - offset;
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            break;
                        }
                        case 0x05: // LDR{cond}B Rd, [Rn], -#
                        case 0x07: // LDR{cond}BT Rd, [Rn], -#
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            CPU.R[Rn] = address - offset;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x08: // STR{cond} Rd, [Rn], #
                        case 0x0A: // STR{cond}T Rd, [Rn], #
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            CPU.R[Rn] = address + offset;
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            break;
                        }
                        case 0x09: // LDR{cond} Rd, [Rn], #
                        case 0x0B: // LDR{cond}T Rd, [Rn], #
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            CPU.R[Rn] = address + offset;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x0C: // STR{cond}B Rd, [Rn], #
                        case 0x0E: // STR{cond}BT Rd, [Rn], #
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            CPU.R[Rn] = address + offset;
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            break;
                        }
                        case 0x0D: // LDR{cond}B Rd, [Rn], #
                        case 0x0F: // LDR{cond}BT Rd, [Rn], #
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            CPU.R[Rn] = address + offset;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x10: // STR{cond} Rd, [Rn, -#]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            break;
                        }
                        case 0x11: // LDR{cond} Rd, [Rn, -#]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x12: // STR{cond} Rd, [Rn, -#]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            CPU.R[Rn] = address;
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            break;
                        }
                        case 0x13: // LDR{cond} Rd, [Rn, -#]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            CPU.R[Rn] = address;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x14: // STR{cond}B Rd, [Rn, -#]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            break;
                        }
                        case 0x15: // LDR{cond}B Rd, [Rn, -#]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x16: // STR{cond}B Rd, [Rn, -#]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            CPU.R[Rn] = address;
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            break;
                        }
                        case 0x17: // LDR{cond}B Rd, [Rn, -#]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            CPU.R[Rn] = address;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x18: // STR{cond} Rd, [Rn, #]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            break;
                        }
                        case 0x19: // LDR{cond} Rd, [Rn, #]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x1A: // STR{cond} Rd, [Rn, #]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            CPU.R[Rn] = address;
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            break;
                        }
                        case 0x1B: // LDR{cond} Rd, [Rn, #]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            CPU.R[Rn] = address;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x1C: // STR{cond}B Rd, [Rn, #]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            break;
                        }
                        case 0x1D: // LDR{cond}B Rd, [Rn, #]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x1E: // STR{cond}B Rd, [Rn, #]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            CPU.R[Rn] = address;
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            break;
                        }
                        case 0x1F: // LDR{cond}B Rd, [Rn, #]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            CPU.R[Rn] = address;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        default: //???
                            CPU.R[R_PC] -= 4;
                            clocks -= 100;
                            break;
                    }
                    break;
                }
                case 3:
                {
                    if(opcode & BIT(4)) //Undefined Instruction
                    {
                        ARM_UNDEFINED_INSTRUCTION();
                        break;
                    }

                    // LDR/STR -- SHIFTED REGISTER AS OFFSET
                    u32 Rn = ((opcode>>16)&0xF); //(including R15=PC+8)
                    u32 Rd = ((opcode>>12)&0xF); //(including R15=PC+12)
                    //u32 Rm = opcode&0xF; //(not including PC=R15)

                    s32 offset = cpu_shift_by_reg_no_carry_arm_ldr_str(
                                                (opcode>>5)&3,CPU.R[opcode&0xF],(opcode>>7)&0x1F);

                    switch(opcode>>20)
                    {
                        case 0x00: // STR{cond} Rd, [Rn], -Op2
                        case 0x02: // STR{cond}T Rd, [Rn], -Op2
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            CPU.R[Rn] = address - offset;
                            break;
                        }
                        case 0x01: // LDR{cond} Rd, [Rn], -Op2
                        case 0x03: // LDR{cond}T Rd, [Rn], -Op2
                        {
                            //int oldmode = CPU.CPSR&0x1F;
                            //GBA_CPUChangeMode(M_USER);GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            CPU.R[Rn] = address - offset;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }

                            //GBA_CPUChangeMode(oldmode);
                            break;
                        }
                        case 0x04: // STR{cond}B Rd, [Rn], -Op2
                        case 0x06: // STR{cond}BT Rd, [Rn], -Op2
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            CPU.R[Rn] = address - offset;
                            break;
                        }
                        case 0x05: // LDR{cond}B Rd, [Rn], -Op2
                        case 0x07: // LDR{cond}BT Rd, [Rn], -Op2
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            CPU.R[Rn] = address - offset;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x08: // STR{cond} Rd, [Rn], Op2
                        case 0x0A: // STR{cond}T Rd, [Rn], Op2
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            CPU.R[Rn] = address + offset;
                            break;
                        }
                        case 0x09: // LDR{cond} Rd, [Rn], Op2
                        case 0x0B: // LDR{cond}T Rd, [Rn], Op2
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            CPU.R[Rn] = address + offset;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x0C: // STR{cond}B Rd, [Rn], Op2
                        case 0x0E: // STR{cond}BT Rd, [Rn], Op2
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            CPU.R[Rn] = address + offset;
                            break;
                        }
                        case 0x0D: // LDR{cond}B Rd, [Rn], Op2
                        case 0x0F: // LDR{cond}BT Rd, [Rn], Op2
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0);
                            CPU.R[Rn] = address + offset;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x10: // STR{cond} Rd, [Rn, -Op2]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            break;
                        }
                        case 0x11: // LDR{cond} Rd, [Rn, -Op2]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x12: // STR{cond} Rd, [Rn, -Op2]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            CPU.R[Rn] = address;
                            break;
                        }
                        case 0x13: // LDR{cond} Rd, [Rn, -Op2]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            CPU.R[Rn] = address;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x14: // STR{cond}B Rd, [Rn, -Op2]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            break;
                        }
                        case 0x15: // LDR{cond}B Rd, [Rn, -Op2]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x16: // STR{cond}B Rd, [Rn, -Op2]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            CPU.R[Rn] = address;
                            break;
                        }
                        case 0x17: // LDR{cond}B Rd, [Rn, -Op2]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) - offset;
                            CPU.R[Rn] = address;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x18: // STR{cond} Rd, [Rn, Op2]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            break;
                        }
                        case 0x19: // LDR{cond} Rd, [Rn, Op2]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x1A: // STR{cond} Rd, [Rn, Op2]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            GBA_MemoryWrite32(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq32(address); //2N cycles
                            CPU.R[Rn] = address;
                            break;
                        }
                        case 0x1B: // LDR{cond} Rd, [Rn, Op2]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            CPU.R[Rn] = address;
                            CPU.R[Rd] = GBA_MemoryRead32(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x1C: // STR{cond}B Rd, [Rn, Op2]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            break;
                        }
                        case 0x1D: // LDR{cond}B Rd, [Rn, Op2]
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        case 0x1E: // STR{cond}B Rd, [Rn, Op2]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            GBA_MemoryWrite8(address,CPU.R[Rd]+(Rd==15?12:0));
                            clocks -= 2 * GBA_MemoryGetAccessCyclesNoSeq16(address); //2N cycles
                            CPU.R[Rn] = address;
                            break;
                        }
                        case 0x1F: // LDR{cond}B Rd, [Rn, Op2]!
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0) + offset;
                            CPU.R[Rn] = address;
                            CPU.R[Rd] = GBA_MemoryRead8(address);
                            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq16(address) + 1;
                            if(Rd == 15)
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4;
                            }
                            break;
                        }
                        default: //???
                            CPU.R[R_PC] -= 4;
                            clocks -= 100;
                            break;
                    }
                    break;
                }
                case 4:
                {
                    /*
                    if( (opcode & (3<<21)) == (3<<21) ) //????
                    {
                        //Unpredictable
                        CPU.R[R_PC] -= 4; //Undefined Instruction Op24
                        clocks -= 100;
                        break;
                    }
                    */

                    //Block Data Transfer (LDM,STM)

                    u32 Rn = (opcode>>16)&0xF; //(not including R15)

                    //Execution Time: For normal LDM, nS+1N+1I. For LDM PC, (n+1)S+2N+1I.
                    //                For STM (n-1)S+2N.
                    //                Where n is the number of words transferred.

                    if(Rn == 15)
                    {
                        Debug_DebugMsgArg("At %08X LDM/STM with base = PC.",CPU.R[R_PC]);
                        GBA_ExecutionBreak();
                    }

                    //if(opcode & (1 << Rn))
                    //{
                    //    Debug_DebugMsgArg("At %08X writeback with base included.",CPU.R[R_PC]);
                    //    GBA_ExecutionBreak();
                    //}

// From ARM 7TDMI Data Sheet
//
//    4.11.6 Inclusion of the base in the register list
//
//    When write-back is specified, the base is written back at the end of the second cycle
//    of the instruction. During a STM, the first register is written out at the start of the
//    second cycle. A STM which includes storing the base, with the base as the first register
//    to be stored, will therefore store the unchanged value, whereas with the base second
//    or later in the transfer order, will store the modified value. A LDM will always overwrite
//    the updated base if the base is in the list.


                    switch(opcode>>20)
                    {
                        case 0x00: // STM{cond}DA Rn,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_stm(address,i);
                            }

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x01: // LDM{cond}DA Rn,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_ldm(address,i);
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x02: // STM{cond}DA Rn!,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int isfirst = 1;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_stm(address,i);
                                if(isfirst) { CPU.R[Rn] = address - 4; isfirst = 0; }
                            }

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x03: // LDM{cond}DA Rn!,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;
                            CPU.R[Rn] = address;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_ldm(address,i);
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x04: // STM{cond}DA Rn,<Rlist>^
                        {
                            u32 oldcpu = CPU.CPSR & 0x1F;
                            GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_stm(address,i);
                            }

                            GBA_CPUChangeMode(oldcpu);

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x05: // LDM{cond}DA Rn,<Rlist>^
                        {
                            u32 oldcpu = 0; //for the compiler warning
                            if(opcode & BIT(15)) CPU.CPSR = CPU.SPSR; //don't change registers
                            else { oldcpu = CPU.CPSR & 0x1F;  GBA_CPUChangeMode(M_USER); } //change registers

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_ldm(address,i);
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                GBA_CPUChangeMode(oldcpu); //change registers

                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x06: // STM{cond}DA Rn!,<Rlist>^
                        {
                            u32 oldcpu = CPU.CPSR & 0x1F;
                            GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int isfirst = 1;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_stm(address,i);
                                if(isfirst) { CPU.R[Rn] = address - 4; isfirst = 0; }
                            }

                            GBA_CPUChangeMode(oldcpu);

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x07: // LDM{cond}DA Rn!,<Rlist>^
                        {
                            u32 oldcpu = 0; //for the compiler warning
                            if(opcode & BIT(15)) CPU.CPSR = CPU.SPSR; //don't change registers
                            else { oldcpu = CPU.CPSR & 0x1F;  GBA_CPUChangeMode(M_USER); } //change registers

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;
                            CPU.R[Rn] = address;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_ldm(address,i);
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                GBA_CPUChangeMode(oldcpu); //change registers

                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x08: // STM{cond}IA Rn,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_stm(address,i);
                                address += 4;
                            }

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x09: // LDM{cond}IA Rn,<Rlist>
                        {
                            u32 address = CPU.R[Rn] + (Rn==15?8:0); // tested in real hardware
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_ldm(address,i);
                                address += 4;
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x0A: // STM{cond}IA Rn!,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int isfirst = 1;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_stm(address,i);
                                address += 4;
                                if(isfirst) { CPU.R[Rn] = address-4+bitcount*4; isfirst = 0; }
                            }

                            //CPU.R[Rn] = address;

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x0B: // LDM{cond}IA Rn!,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            if(Rn!=15) CPU.R[Rn] += bitcount*4; // tested in real hardware
                            else address += 8;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_ldm(address,i);
                                address += 4;
                            }

                            //CPU.R[Rn] = address;

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x0C: // STM{cond}IA Rn,<Rlist>^
                        {
                            u32 oldcpu = CPU.CPSR & 0x1F;
                            GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_stm(address,i);
                                address += 4;
                            }

                            GBA_CPUChangeMode(oldcpu);

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x0D: // LDM{cond}IA Rn,<Rlist>^
                        {
                            u32 oldcpu = 0; //for the compiler warning
                            if(opcode & BIT(15)) CPU.CPSR = CPU.SPSR; //don't change registers
                            else { oldcpu = CPU.CPSR & 0x1F;  GBA_CPUChangeMode(M_USER); } //change registers

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_ldm(address,i);
                                address += 4;
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                GBA_CPUChangeMode(oldcpu);
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x0E: // STM{cond}IA Rn!,<Rlist>^
                        {
                            u32 oldcpu = CPU.CPSR & 0x1F;
                            GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int isfirst = 1;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_stm(address,i);
                                address += 4;
                                if(isfirst) { CPU.R[Rn] = address-4+bitcount*4; isfirst = 0; }
                            }

                            //CPU.R[Rn] = address;

                            GBA_CPUChangeMode(oldcpu);

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x0F: // LDM{cond}IA Rn!,<Rlist>^
                        {
                            u32 oldcpu = 0; //for the compiler warning
                            if(opcode & BIT(15)) CPU.CPSR = CPU.SPSR; //don't change registers
                            else { oldcpu = CPU.CPSR & 0x1F;  GBA_CPUChangeMode(M_USER); } //change registers

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            CPU.R[Rn] += bitcount*4;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_ldm(address,i);
                                address += 4;
                            }

                            //CPU.R[Rn] = address;

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                GBA_CPUChangeMode(oldcpu);
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x10: // STM{cond}DB Rn,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_stm(address,i);
                                address += 4;
                            }

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x11: // LDM{cond}DB Rn,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_ldm(address,i);
                                address += 4;
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x12: // STM{cond}DB Rn!,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            //CPU.R[Rn] = address;

                            int isfirst = 1;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_stm(address,i);
                                address += 4;
                                if(isfirst) { CPU.R[Rn] = address - 4; isfirst = 0; }
                            }

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x13: // LDM{cond}DB Rn!,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;
                            CPU.R[Rn] = address;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_ldm(address,i);
                                address += 4;
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x14: // STM{cond}DB Rn,<Rlist>^
                        {
                            u32 oldcpu = CPU.CPSR & 0x1F;
                            GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_stm(address,i);
                                address += 4;
                            }

                            GBA_CPUChangeMode(oldcpu);

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x15: // LDM{cond}DB Rn,<Rlist>^
                        {
                            u32 oldcpu = 0; //for the compiler warning
                            if(opcode & BIT(15)) CPU.CPSR = CPU.SPSR; //don't change registers
                            else { oldcpu = CPU.CPSR & 0x1F;  GBA_CPUChangeMode(M_USER); } //change registers

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_ldm(address,i);
                                address += 4;
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                GBA_CPUChangeMode(oldcpu); //change registers

                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x16: // STM{cond}DB Rn!,<Rlist>^
                        {
                            u32 oldcpu = CPU.CPSR & 0x1F;
                            GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;

                            //CPU.R[Rn] = address;

                            int isfirst = 1;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_stm(address,i);
                                address += 4;
                                if(isfirst) { CPU.R[Rn] = address - 4; isfirst = 0; }
                            }

                            GBA_CPUChangeMode(oldcpu);

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x17: // LDM{cond}DB Rn!,<Rlist>^
                        {
                            u32 oldcpu = 0; //for the compiler warning
                            if(opcode & BIT(15)) CPU.CPSR = CPU.SPSR; //don't change registers
                            else { oldcpu = CPU.CPSR & 0x1F;  GBA_CPUChangeMode(M_USER); } //change registers

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            address -= bitcount<<2;
                            CPU.R[Rn] = address;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                arm_ldm(address,i);
                                address += 4;
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                GBA_CPUChangeMode(oldcpu); //change registers

                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x18: // STM{cond}IB Rn,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_stm(address,i);
                            }

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x19: // LDM{cond}IB Rn,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_ldm(address,i);
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x1A: // STM{cond}IB Rn!,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int isfirst = 1;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_stm(address,i);
                                if(isfirst) { CPU.R[Rn] = address-4+bitcount*4; isfirst = 0; }
                            }

                            //CPU.R[Rn] = address;

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x1B: // LDM{cond}IB Rn!,<Rlist>
                        {
                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            CPU.R[Rn] += bitcount*4;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_ldm(address,i);
                            }

                            //CPU.R[Rn] = address;

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x1C: // STM{cond}IB Rn,<Rlist>^
                        {
                            u32 oldcpu = CPU.CPSR & 0x1F;
                            GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_stm(address,i);
                            }

                            GBA_CPUChangeMode(oldcpu);

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x1D: // LDM{cond}IB Rn,<Rlist>^
                        {
                            u32 oldcpu = 0; //for the compiler warning
                            if(opcode & BIT(15)) CPU.CPSR = CPU.SPSR; //don't change registers
                            else { oldcpu = CPU.CPSR & 0x1F;  GBA_CPUChangeMode(M_USER); } //change registers

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_ldm(address,i);
                            }

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                GBA_CPUChangeMode(oldcpu);
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        case 0x1E: // STM{cond}IB Rn!,<Rlist>^
                        {
                            u32 oldcpu = CPU.CPSR & 0x1F;
                            GBA_CPUChangeMode(M_USER);

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);

                            int isfirst = 1;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_stm(address,i);
                                if(isfirst) { CPU.R[Rn] = address-4+bitcount*4; isfirst = 0; }
                            }

                            //CPU.R[Rn] = address;

                            GBA_CPUChangeMode(oldcpu);

                            clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1));
                            break;
                        }
                        case 0x1F: // LDM{cond}IB Rn!,<Rlist>^
                        {
                            u32 oldcpu = 0; //for the compiler warning
                            if(opcode & BIT(15)) CPU.CPSR = CPU.SPSR; //don't change registers
                            else { oldcpu = CPU.CPSR & 0x1F;  GBA_CPUChangeMode(M_USER); } //change registers

                            u32 address = CPU.R[Rn];
                            u32 bitcount = arm_bit_count(opcode&0xFFFF);
                            CPU.R[Rn] += bitcount*4;

                            int i;
                            for(i = 0; i < 16; i++) if(opcode & BIT(i))
                            {
                                address += 4;
                                arm_ldm(address,i);
                            }

                            //CPU.R[Rn] = address;

                            if(opcode & BIT(15))
                            {
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1 +
                                        GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]);
                                CPU.R[R_PC] -= 4; //to avoid skipping an instruction
                            }
                            else
                            {
                                GBA_CPUChangeMode(oldcpu);
                                clocks -= GBA_MemoryGetAccessCyclesNoSeq32(address) + (GBA_MemoryGetAccessCyclesSeq32(address) * (bitcount-1)) + 1;
                            }
                            break;
                        }
                        default: //???
                            CPU.R[R_PC] -= 4;
                            clocks -= 100;
                            break;
                    }
                    break;
                }
                case 5:
                {
                    if(opcode & BIT(24)) //BL{cond}
                    {
                        CPU.R[R_LR] = CPU.R[R_PC]+4; //return address
                    }
                    //else //B{cond}

                    u32 nn = (opcode & 0x00FFFFFF);
                    CPU.R[R_PC] = CPU.R[R_PC]+8+( ((nn & BIT(23)) ? (nn|0xFF000000) : nn)*4 );
                    CPU.R[R_PC] -= 4; //to avoid skipping an instruction

                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //2S + 1N cycles
                    break;
                }
                case 6:
                {
                    //Coprocessor Data Transfers (LDC,STC)
                    //Irrelevant in GBA because no coprocessor exists.
                    ARM_UNDEFINED_INSTRUCTION();
                    break;
                }
                case 7:
                {
                    if(opcode & BIT(24))
                    {
                        //SWI{cond}
                        if(GBA_BiosIsLoaded()==0)
                        {
                            u32 swinummber = (opcode>>16)&0xFF;
                            if(swinummber == 5)
                            {
                                CPU.R[0] = 1;
                                CPU.R[1] = 1;
                                swinummber = 4;
                            }
                            if(swinummber != 4)
                            {
                                GBA_Swi(swinummber);
                                clocks -= 50; // delay some time... not accurate at all
                                break;
                            }
                        }

                        GBA_CPUChangeMode(M_SUPERVISOR);
                        CPU.R[14] = CPU.R[R_PC]+4; // save return address
                        CPU.SPSR = CPU.CPSR; //save CPSR flags
                        CPU.CPSR &= ~((0x1F)|F_T|F_I); //Enter svc, ARM state, IRQs disabled
                        CPU.CPSR |= M_SUPERVISOR|F_I;
                        //CPU.R[R_PC] = 8; CPU.R[R_PC] -= 4; // PC=VVVV0008h  ;jump to SWI vector address
                        CPU.R[R_PC] = 4;
                        clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_PC]) +
                        GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_PC]); //2S + 1N cycles
                        break;
                    }
                    else
                    {
                        if(opcode & BIT(4)) //Coprocessor Register Transfers (MRC, MCR)
                        {
                            //The GBA allow to access CP14 (unlike as for CP0..CP13 & CP15, access
                            //to CP14 doesn't generate any exceptions), however, the ICEbreaker module
                            //appears to be disabled (or completely unimplemented), any reads from
                            //P14,0,Rd,C0,C0,0 through P14,7,Rd,C15,C15,7 are simply returning the prefetched
                            //opcode value from [$+8].
                            if((opcode&(0xF<<8)) != 0xE<<8)
                            {
                                ARM_UNDEFINED_INSTRUCTION();
                                break;
                            }
                            else
                            {
                                Debug_DebugMsgArg("CP14 ICEbreaker used!");
                                GBA_ExecutionBreak();

                                if(opcode & BIT(20)) //MRC{cond} Pn,<cpopc>,Rd,Cn,Cm{,<cp>}   ;move from CoPro to ARM
                                {
                                    u32 Rd = (opcode>>12)&0xF;
                                    u32 data = GBA_MemoryRead32(CPU.R[R_PC]+8);

                                    if(Rd == 15)
                                    {
                                        CPU.CPSR &= 0x0FFFFFFF;
                                        CPU.CPSR |= data & 0xF0000000;
                                    }
                                    else
                                    {
                                        CPU.R[Rd] = data;
                                    }

                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + 1 + 1; //1S+(b+1)I+1C cycles (b = 0?)
                                    break;
                                }
                                else //MCR{cond} Pn,<cpopc>,Rd,Cn,Cm{,<cp>}   ;move from ARM to CoPro
                                {
                                    //no effect (?)
                                    clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.OldPC) + 0 + 1; //1S+bI+1C cycles (b = 0?)
                                    break;
                                }
                            }
                        }
                        else //Coprocessor Data Operations (CDP)
                        {
                            ARM_UNDEFINED_INSTRUCTION();
                        }
                        break;
                    }
                    break;
                }
            }
        }
        else
        {
            clocks -= GBA_MemoryGetAccessCycles(PCseq,1,CPU.R[R_PC]); //1S cycle
        }

        CPU.R[R_PC] += 4;
    }

    return clocks;
}


