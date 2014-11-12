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

#include "../build_options.h"
#include "../debug_utils.h"
#include "../gui/win_gba_debugger.h"

#include "gba.h"
#include "bios.h"
#include "cpu.h"
#include "shifts.h"
#include "memory.h"
#include "interrupts.h"
#include "disassembler.h"

//-----------------------------------------------------------------------------------------------

static inline int thumb_mul_extra_cycles(u32 Rd)
{
    register u32 temp = CPU.R[Rd];
    if(temp&BIT(31)) temp = ~temp; //all 0 or all 1
    if(temp & 0xFFFFFF00)
    {
        if(temp & 0xFFFF0000)
        {
            if(temp & 0xFF000000)
            {
                return 4;
            }
            else return 3;
        }
        else return 2;
    }
    else return 1;
}

//------------------------------------------------------------------------------------------------------

static inline u32 thumb_bit_count(u16 value)
{
    /*
    static const u32 bitcounttable[256] = {
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
    };

    return ( bitcounttable[value&0xFF] + bitcounttable[value>>8] );
*/
    //u32 count = 0;
    //int i;
    //for(i = 0; i < 16; i++) if(value & BIT(i)) count++;
    //return count;
    register u32 count = (value & 1); value >>= 1;   count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += (value & 1); value >>= 1;
    count += (value & 1); value >>= 1;  count += value;
    return count;
}

static inline void thumb_ldm(u32 address, u32 reg)
{
    CPU.R[reg] = GBA_MemoryRead32(address&~3);
}

static inline void thumb_stm(u32 address, u32 reg)
{
    GBA_MemoryWrite32(address&~3,CPU.R[reg]);
}

//-------------------------------------------------------------------------------------------------------------

#define THUMB_UNDEFINED_INSTRUCTION() \
{ \
    Debug_DebugMsgArg("Undefined instruction.\nTHUMB: [0x%08X]=0x%04X",CPU.R[R_PC],GBA_MemoryRead16(CPU.R[R_PC]));\
    GBA_CPUChangeMode(M_UNDEFINED); \
    CPU.R[14] = CPU.R[R_PC]+2; \
    CPU.SPSR = CPU.CPSR; \
    CPU.EXECUTION_MODE = EXEC_ARM; \
    CPU.CPSR &= ~((0x1F)|F_T|F_I); \
    CPU.CPSR |= M_UNDEFINED|F_I; \
    CPU.R[R_PC] = 4; \
    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC])+1; \
    return clocks; \
} //1S+1N+1I ?
//GBA_ExecutionBreak(); -> Not needed

//-------------------------------------------------------------------------------------------------------------

//#define POS(n) ((~(u32)(n))>>31)
//#define NEG(n) (((u32)(n))>>31)
//#define ADD_OVERFLOW(a,b,res) ( (NEG(a)&NEG(b)&POS(res)) || (POS(a)&POS(b)&NEG(res)) )
//#define ADD_OVERFLOW(a,b,res) ( ( ((~(a))&(~(b))&(res)) | ((a)&(b)&(~(res))) ) >> 31 )
#define ADD_OVERFLOW(a,b,res) ( ( ((~((a)|(b)))&(res)) | ((a)&(b)&(~(res))) ) >> 31 )
//#define ADD_OVERFLOW(a,b,res) ( ( ( ((~((a)|(b)))&(res)) | ((a)&(b)&(~(res))) ) >> 3 ) & BIT(28) )
//#define SUB_OVERFLOW(a,b,res) ( (POS(a)&NEG(b)&NEG(res)) || (NEG(a)&POS(b)&POS(res)) )
//#define SUB_OVERFLOW(a,b,res) ( ( ((~(a))&(b)&(res)) | ((a)&(~((b)&(res)))) ) >> 31 )

static inline void thumb_and(u16 Rd, u16 Rs)
{
    CPU.R[Rd] &= CPU.R[Rs];
    CPU.CPSR &= ~(F_Z|F_N);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N);
}

static inline void thumb_eor(u16 Rd, u16 Rs)
{
    CPU.R[Rd] ^= CPU.R[Rs];
    CPU.CPSR &= ~(F_Z|F_N);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N);
}

static inline void thumb_lsl(u16 Rd, u16 Rs)
{
    u8 carry;
    CPU.R[Rd] = lsl_shift_by_reg(CPU.R[Rd],CPU.R[Rs]&0xFF,&carry);
    CPU.CPSR &= ~(F_Z|F_N|F_C);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0);
}

static inline void thumb_lsr(u16 Rd, u16 Rs)
{
    u8 carry;
    CPU.R[Rd] = lsr_shift_by_reg(CPU.R[Rd],CPU.R[Rs]&0xFF,&carry);
    CPU.CPSR &= ~(F_Z|F_N|F_C);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0);
}

static inline void thumb_asr(u16 Rd, u16 Rs)
{
    u8 carry;
    CPU.R[Rd] = asr_shift_by_reg(CPU.R[Rd],CPU.R[Rs]&0xFF,&carry);
    CPU.CPSR &= ~(F_Z|F_N|F_C);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0);
}

static inline void thumb_adc(u16 Rd, u16 Rs)
{
    u32 t1 = CPU.R[Rd];
    u32 t2 = CPU.R[Rs];
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR&F_C)?1ULL:0ULL);
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
    CPU.CPSR |= (((u32)temp)?0:F_Z) | (((u32)temp)&F_N) | ((temp&0xFFFFFFFF00000000ULL)?F_C:0) |
        (ADD_OVERFLOW(t1,t2,(u32)temp)?F_V:0);
    CPU.R[Rd] = (u32)temp;
}

static inline void thumb_sbc(u16 Rd, u16 Rs)
{
    u32 t1 = CPU.R[Rd];
    u32 t2 = ~CPU.R[Rs];
    u64 temp = (u64)t1 + (u64)t2 + ((CPU.CPSR&F_C)?1ULL:0ULL);
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
    CPU.CPSR |= ((u32)temp?0:F_Z) | (((u32)temp)&F_N) | ((temp&0xFFFFFFFF00000000ULL)?F_C:0) |
        (ADD_OVERFLOW(t1,t2,(u32)temp)?F_V:0);
    CPU.R[Rd] = (u32)temp;
}

static inline void thumb_ror(u16 Rd, u16 Rs)
{
    u8 carry;
    CPU.R[Rd] = ror_shift_by_reg(CPU.R[Rd],CPU.R[Rs]&0xFF,&carry);
    CPU.CPSR &= ~(F_Z|F_N|F_C);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0);
}

static inline void thumb_tst(u16 Rd, u16 Rs)
{
    u32 temp = CPU.R[Rd] & CPU.R[Rs];
    CPU.CPSR &= ~(F_Z|F_N);
    CPU.CPSR |= (temp?0:F_Z)|(temp&F_N);
}

static inline void thumb_neg(u16 Rd, u16 Rs)
{
    u32 tmp = CPU.R[Rs];
    CPU.R[Rd] = -(s32)tmp;
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z) | (CPU.R[Rd]&F_N) | (tmp?0:F_C) | (ADD_OVERFLOW(0,~tmp,CPU.R[Rd])?F_V:0);
}

static inline void thumb_cmp(u16 Rd, u16 Rs)
{
    u32 t1 = CPU.R[Rd];
    u32 t2 = ~CPU.R[Rs];
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
    CPU.CPSR |= (((u32)temp)?0:F_Z) | (((u32)temp)&F_N) | ((temp&0xFFFFFFFF00000000ULL)?F_C:0) |
        (ADD_OVERFLOW(t1,t2,(u32)temp)?F_V:0);
}

static inline void thumb_cmn(u16 Rd, u16 Rs)
{
#ifdef ENABLE_ASM_X86
    u8 carry, overflow;
    asm("add %3,%2 \n\t"
        "setc %%al \n\t"
        "seto %%bl \n\t"
        : "=a" (carry), "=b" (overflow)
        : "r" (CPU.R[Rd]), "r" (CPU.R[Rs]));
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0)|(overflow?F_V:0);
#else
    u64 temp = (u64)CPU.R[Rd] + (u64)CPU.R[Rs];
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
    CPU.CPSR |= (((u32)temp)?0:F_Z) | (((u32)temp)&F_N) | ((temp&0xFFFFFFFF00000000ULL)?F_C:0) |
        (ADD_OVERFLOW(CPU.R[Rd],CPU.R[Rs],(u32)temp)?F_V:0);
#endif
}

static inline void thumb_orr(u16 Rd, u16 Rs)
{
    CPU.R[Rd] |= CPU.R[Rs];
    CPU.CPSR &= ~(F_Z|F_N);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N);
}

static inline void thumb_mul(u16 Rd, u16 Rs)
{
    CPU.R[Rd] *= CPU.R[Rs];
    CPU.CPSR &= ~(F_Z|F_N);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N);
    //carry flag destroyed
}

static inline void thumb_bic(u16 Rd, u16 Rs)
{
    CPU.R[Rd] &= ~CPU.R[Rs];
    CPU.CPSR &= ~(F_Z|F_N);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N);
}

static inline void thumb_mvn(u16 Rd, u16 Rs)
{
    CPU.R[Rd] = ~CPU.R[Rs];
    CPU.CPSR &= ~(F_Z|F_N);
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N);
}

//-------------------------------------------------------------------------------------------------------------

extern u32 cpu_loop_break;
inline s32 GBA_ExecuteTHUMB(s32 clocks) //returns residual clocks
{
    while(clocks > 0)
    {
        if(GBA_DebugCPUIsBreakpoint(CPU.R[R_PC]))
        {
            cpu_loop_break = 1;
            GBA_RunFor_ExecutionBreak();
            Win_GBADisassemblerSetFocus();
        }

        if(cpu_loop_break) { cpu_loop_break = 0; return clocks; }

        u32 PCseq = ((CPU.OldPC+2) == CPU.R[R_PC]);
        CPU.OldPC = CPU.R[R_PC];

        register u16 opcode = GBA_MemoryReadFast16(CPU.R[R_PC]);

        u16 ident = opcode >> 8;
        opcode &= 0x0FFF;

        switch(ident)
        {
            case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5: case 0x6: case 0x7:
            {
                //LSL Rd,Rs,#Offset
                u16 Rd = opcode & 7;
                u16 Rs = (opcode>>3) & 7;
                u16 immed = (opcode >> 6) & 0x1F;
                u8 carry;
                CPU.R[Rd] = lsl_shift_by_immed(CPU.R[Rs],immed,&carry);
                CPU.CPSR &= ~(F_Z|F_N|F_C);
                CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0x8: case 0x9: case 0xA: case 0xB: case 0xC: case 0xD: case 0xE: case 0xF:
            {
                //LSR Rd,Rs,#Offset
                u16 Rd = opcode & 7;
                u16 Rs = (opcode>>3) & 7;
                u16 immed = (opcode >> 6) & 0x1F;
                u8 carry;
                CPU.R[Rd] = lsr_shift_by_immed(CPU.R[Rs],immed,&carry);
                CPU.CPSR &= ~(F_Z|F_N|F_C);
                CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
            {
                //ASR Rd,Rs,#Offset
                u16 Rd = opcode & 7;
                u16 Rs = (opcode>>3) & 7;
                u16 immed = (opcode >> 6) & 0x1F;
                u8 carry;
                CPU.R[Rd] = asr_shift_by_immed(CPU.R[Rs],immed,&carry);
                CPU.CPSR &= ~(F_Z|F_N|F_C);
                CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0x18: case 0x19:
            {
                //ADD Rd,Rs,Rn
                u16 Rd = opcode & 7;
                u16 Rs = (opcode>>3) & 7;
                u16 Rn = (opcode>>6) & 7;
#ifdef ENABLE_ASM_X86
                u8 carry, overflow;
                asm("add %4,%3 \n\t"
                    "mov %3,%0 \n\t"
                    "setc %%al \n\t"
                    "seto %%bl \n\t"
                    : "=r" (CPU.R[Rd]), "=a" (carry), "=b" (overflow)
                    : "r" (CPU.R[Rs]), "r" (CPU.R[Rn]));
                CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
                CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0)|(overflow?F_V:0);
#else
                u64 temp = (u64)CPU.R[Rs] + (u64)CPU.R[Rn];
                CPU.R[Rd] = (u32)temp;
                CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
                CPU.CPSR |= ((temp&0xFFFFFFFF00000000ULL)?F_C:0)|(CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|
                        (ADD_OVERFLOW(CPU.R[Rs],CPU.R[Rn],CPU.R[Rd])?F_V:0);
#endif
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0x1A: case 0x1B:
            {
                //SUB Rd,Rs,Rn
                u16 Rd = opcode & 7;
                u16 Rs = (opcode>>3) & 7;
                u16 Rn = (opcode>>6) & 7;
                u64 addval = 1ULL + (u64)(u32)~CPU.R[Rn];
                u64 temp = (u64)CPU.R[Rs] + (u64)addval;
                CPU.R[Rd] = (u32)temp;
                CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
                CPU.CPSR |= ((temp&0xFFFFFFFF00000000ULL)?F_C:0)|(CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|
                        (ADD_OVERFLOW(CPU.R[Rs],(u32)(addval-1),CPU.R[Rd])?F_V:0);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0x1C: case 0x1D:
            {
                //ADD Rd,Rs,#nn
                u16 Rd = opcode & 7;
                u16 Rs = (opcode>>3) & 7;
                u32 immed = (opcode >> 6) & 0x7;
#ifdef ENABLE_ASM_X86
                u8 carry, overflow;
                asm("add %4,%3 \n\t"
                    "mov %3,%0 \n\t"
                    "setc %%al \n\t"
                    "seto %%bl \n\t"
                    : "=r" (CPU.R[Rd]), "=a" (carry), "=b" (overflow)
                    : "r" (CPU.R[Rs]), "r" (immed));
                CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
                CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0)|(overflow?F_V:0);
#else
                u64 temp = (u64)CPU.R[Rs] + (u64)immed;
                CPU.R[Rd] = (u32)temp;
                CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
                CPU.CPSR |= ((temp&0xFFFFFFFF00000000ULL)?F_C:0)|(CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|
                        (ADD_OVERFLOW(CPU.R[Rs],immed,CPU.R[Rd])?F_V:0);
#endif
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0x1E: case 0x1F:
            {
                //SUB Rd,Rs,#nn
                u16 Rd = opcode & 7;
                u16 Rs = (opcode>>3) & 7;
                u64 immed = 1ULL + (u64)(u32)~((opcode >> 6) & 0x7);
                u64 temp = (u64)CPU.R[Rs] + (u64)immed;
                CPU.R[Rd] = (u32)temp;
                CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
                CPU.CPSR |= ((temp&0xFFFFFFFF00000000ULL)?F_C:0)|(CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|
                        (ADD_OVERFLOW(CPU.R[Rs],(u32)(immed-1),CPU.R[Rd])?F_V:0);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }

#define MOV_REG_IMM(Rd) \
{ \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]);  \
    CPU.R[Rd] = opcode & 0xFF; \
    CPU.CPSR &= ~(F_Z|F_N); \
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z); \
    break; \
} //1S cycle
            //MOV Rd,#nn
            case 0x20: MOV_REG_IMM(0);
            case 0x21: MOV_REG_IMM(1);
            case 0x22: MOV_REG_IMM(2);
            case 0x23: MOV_REG_IMM(3);
            case 0x24: MOV_REG_IMM(4);
            case 0x25: MOV_REG_IMM(5);
            case 0x26: MOV_REG_IMM(6);
            case 0x27: MOV_REG_IMM(7);

#define CMP_REG_IMM(Rd) \
{ \
    u32 immed = opcode & 0xFF; \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); \
    u64 val = (u64)~immed; \
    u64 temp = (u64)CPU.R[Rd] + (u64)val + 1ULL; \
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V); \
    CPU.CPSR |= ((temp&0xFFFFFFFF00000000ULL)?F_C:0)|((u32)temp?0:F_Z)|(((u32)temp)&F_N)| \
            (ADD_OVERFLOW(CPU.R[Rd],(u32)val,(u32)temp)?F_V:0); \
    break; \
}
            //CMP Rd,#nn
            case 0x28: CMP_REG_IMM(0);
            case 0x29: CMP_REG_IMM(1);
            case 0x2A: CMP_REG_IMM(2);
            case 0x2B: CMP_REG_IMM(3);
            case 0x2C: CMP_REG_IMM(4);
            case 0x2D: CMP_REG_IMM(5);
            case 0x2E: CMP_REG_IMM(6);
            case 0x2F: CMP_REG_IMM(7);

#ifdef ENABLE_ASM_X86
#define ADD_REG_IMM(Rd) \
{ \
    u32 immed = opcode & 0xFF; \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); \
    u8 carry, overflow; \
    asm("add %3,%4 \n\t" \
        "mov %4,%0 \n\t" \
        "setc %%al \n\t" \
        "seto %%bl \n\t" \
        : "=r" (CPU.R[Rd]), "=a" (carry), "=b" (overflow) \
        : "r" (CPU.R[Rd]), "r" (immed)); \
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V); \
    CPU.CPSR |= (CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(carry?F_C:0)|(overflow?F_V:0); \
    break; \
}
#else
#define ADD_REG_IMM(Rd) \
{ \
    u32 immed = opcode & 0xFF; \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); \
    u64 temp = (u64)CPU.R[Rd] + (u64)immed; \
    u32 overflow = ADD_OVERFLOW(CPU.R[Rd],immed,temp); \
    CPU.R[Rd] = (u32)temp; \
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V); \
    CPU.CPSR |= ((temp&0xFFFFFFFF00000000LL)?F_C:0)|(CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(overflow?F_V:0); \
    break; \
}
#endif
            //ADD Rd,#nn
            case 0x30: ADD_REG_IMM(0);
            case 0x31: ADD_REG_IMM(1);
            case 0x32: ADD_REG_IMM(2);
            case 0x33: ADD_REG_IMM(3);
            case 0x34: ADD_REG_IMM(4);
            case 0x35: ADD_REG_IMM(5);
            case 0x36: ADD_REG_IMM(6);
            case 0x37: ADD_REG_IMM(7);

#define SUB_REG_IMM(Rd) \
{ \
    u32 immed = opcode & 0xFF; \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); \
    u64 val = (u64)~immed; \
    u64 temp = (u64)CPU.R[Rd] + (u64)val + 1ULL; \
    u32 overflow = ADD_OVERFLOW(CPU.R[Rd],(u32)val,(u32)temp); \
    CPU.R[Rd] = (u32)temp; \
    CPU.CPSR &= ~(F_Z|F_C|F_N|F_V); \
    CPU.CPSR |= ((temp&0xFFFFFFFF00000000ULL)?F_C:0)|(CPU.R[Rd]?0:F_Z)|(CPU.R[Rd]&F_N)|(overflow?F_V:0); \
    break; \
}
            //SUB Rd,#nn
            case 0x38: SUB_REG_IMM(0);
            case 0x39: SUB_REG_IMM(1);
            case 0x3A: SUB_REG_IMM(2);
            case 0x3B: SUB_REG_IMM(3);
            case 0x3C: SUB_REG_IMM(4);
            case 0x3D: SUB_REG_IMM(5);
            case 0x3E: SUB_REG_IMM(6);
            case 0x3F: SUB_REG_IMM(7);

            case 0x40: case 0x41: case 0x42: case 0x43:
            {
                //ALU operations
                u16 Rd = opcode&7;
                u16 Rs = (opcode>>3)&7;
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle

                //typedef void (*thumb_alu_fn)(u16,u16);
                //const thumb_alu_fn fn[16] = {
                //    thumb_and,thumb_eor,thumb_lsl,thumb_lsr,thumb_asr,thumb_adc,thumb_sbc,thumb_ror,
                //    thumb_tst,thumb_neg,thumb_cmp,thumb_cmn,thumb_orr,thumb_mul,thumb_bic,thumb_mvn };
                //fn[(opcode>>6)&0xF](Rd,Rs);

                switch((opcode>>6)&0xF)
                {
                    case 0: thumb_and(Rd,Rs); break;
                    case 1: thumb_eor(Rd,Rs); break;
                    case 2: thumb_lsl(Rd,Rs); clocks --; //1I
                        break;
                    case 3: thumb_lsr(Rd,Rs); clocks --; //1I
                        break;
                    case 4: thumb_asr(Rd,Rs); clocks --; //1I
                        break;
                    case 5: thumb_adc(Rd,Rs); break;
                    case 6: thumb_sbc(Rd,Rs); break;
                    case 7: thumb_ror(Rd,Rs); clocks --; //1I
                        break;
                    case 8: thumb_tst(Rd,Rs); break;
                    case 9: thumb_neg(Rd,Rs); break;
                    case 10: thumb_cmp(Rd,Rs); break;
                    case 11: thumb_cmn(Rd,Rs); break;
                    case 12: thumb_orr(Rd,Rs); break;
                    case 13: thumb_mul(Rd,Rs); clocks -= thumb_mul_extra_cycles(Rd); //mI
                        break;
                    case 14: thumb_bic(Rd,Rs); break;
                    case 15: thumb_mvn(Rd,Rs); break;
                    default: break; //???
                }
                break;
            }
            case 0x44:
            {
                //Hi register operations/branch exchange

                //ADD Rd,Rs
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                u16 Rd = (opcode&7) | ( (opcode>>4) & 8);
                u16 Rs = (opcode>>3)&0xF;
                CPU.R[Rd] += CPU.R[Rs] + (Rs==R_PC?4:0);
                if(Rd==R_PC)
                {
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq(1,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq(1,CPU.R[R_PC]); //1N+1S
                    CPU.R[R_PC] = (CPU.R[R_PC]-2)&~1;
                }
                break;

                //They work in low registers. Tested in real hardware (Unused Opcode #4-0)
            }
            case 0x45:
            {
                //Hi register operations/branch exchange

                //CMP Rd,Rs
                u16 Rd = (opcode&7) | ( (opcode>>4) & 8);
                u16 Rs = (opcode>>3)&0xF;
                u32 t1 = CPU.R[Rd]+(Rd==R_PC?4:0);
                u64 t2 = (u64)~(CPU.R[Rs]+(Rs==R_PC?4:0));
                u64 temp = (u64)t1 + (u64)t2 + 1ULL;
                CPU.CPSR &= ~(F_Z|F_C|F_N|F_V);
                CPU.CPSR |= ((u32)temp?0:F_Z) | (((u32)temp)&F_N) | ((temp&0xFFFFFFFF00000000ULL)?F_C:0) |
                        (ADD_OVERFLOW(t1,(u32)(t2-1),(u32)temp)?F_V:0);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;

                //They work in low registers. Tested in real hardware (Unused Opcode #4-1)
            }
            case 0x46:
            {
                //Hi register operations/branch exchange

                //MOV Rd,Rs
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                u16 Rd = (opcode&7) | ( (opcode>>4) & 8);
                u16 Rs = (opcode>>3)&0xF;
                CPU.R[Rd] = CPU.R[Rs] + ((Rs==R_PC)?4:0);
                if(Rd==R_PC)
                {
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq(1,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq(1,CPU.R[R_PC]); //1N+1S
                    CPU.R[R_PC] = (CPU.R[R_PC]-2)&~1;
                }
                break;

                //They work in low registers. Tested in real hardware (Unused Opcode #4-2)
            }
            case 0x47:
            {
                //Hi register operations/branch exchange
                if(opcode & (BIT(7)|0x7))
                {
                    //(BLX  Rs), Unpredictable (BIT(7)) / Undefined opcode (0x7)
                    THUMB_UNDEFINED_INSTRUCTION(); //Undefined Opcode #4-3 and #4-4 -- tested on hardware
                    break;
                }
                else
                {
                    //BX  Rs
                    u16 Rs = (opcode>>3)&0xF;
                    u32 val = ((Rs==R_PC)?((CPU.R[R_PC]+4)&(~2)):CPU.R[Rs]);
                    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                    if((val&BIT(0)) == 0) //Switch to ARM
                    {
                        CPU.EXECUTION_MODE = EXEC_ARM;
                        CPU.CPSR &= ~F_T;
                        CPU.R[R_PC] = val&(~3);
                        clocks -= GBA_MemoryGetAccessCyclesNoSeq(1,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq(1,CPU.R[R_PC]); //1N+1S
                        return GBA_ExecuteARM(clocks);

                    }
                    CPU.R[R_PC] = val&(~1);
                    CPU.R[R_PC] -= 2; //to avoid skipping an instruction
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq(1,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesSeq(1,CPU.R[R_PC]); //1N+1S
                    break;
                }
            }

#define LDR_REG_PC_IMM(Rd) \
{ \
    u32 offset = (opcode&0xFF)<<2; \
    u32 addr = ((CPU.R[R_PC]+4)&(~2))+offset; \
    CPU.R[Rd] = GBA_MemoryRead32(addr); \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + \
            GBA_MemoryGetAccessCyclesSeq32(addr) + 1; \
    break; \
} //1S+1N+1I
            //LDR Rd,[PC,#nn]
            case 0x48: LDR_REG_PC_IMM(0);
            case 0x49: LDR_REG_PC_IMM(1);
            case 0x4A: LDR_REG_PC_IMM(2);
            case 0x4B: LDR_REG_PC_IMM(3);
            case 0x4C: LDR_REG_PC_IMM(4);
            case 0x4D: LDR_REG_PC_IMM(5);
            case 0x4E: LDR_REG_PC_IMM(6);
            case 0x4F: LDR_REG_PC_IMM(7);

            case 0x50: case 0x51:
            {
                //STR  Rd,[Rb,Ro]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 Ro = (opcode>>6)&7;
                u32 addr = CPU.R[Rb]+CPU.R[Ro];
                GBA_MemoryWrite32(addr,CPU.R[Rd]);
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(addr); //2N
                break;
            }
            case 0x52: case 0x53:
            {
                //STRH Rd,[Rb,Ro]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 Ro = (opcode>>6)&7;
                u32 addr = CPU.R[Rb]+CPU.R[Ro];
                GBA_MemoryWrite16(addr,(u16)CPU.R[Rd]);
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr); //2N
                break;
            }
            case 0x54: case 0x55:
            {
                //STRB Rd,[Rb,Ro]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 Ro = (opcode>>6)&7;
                u32 addr = CPU.R[Rb]+CPU.R[Ro];
                GBA_MemoryWrite8(addr,(u8)CPU.R[Rd]);
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr); //2N
                break;
            }
            case 0x56: case 0x57:
            {
                //LDSB Rd,[Rb,Ro]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 Ro = (opcode>>6)&7;
                u32 addr = CPU.R[Rb]+CPU.R[Ro];
                CPU.R[Rd] = (u32)(s32)(s8)GBA_MemoryRead8(addr);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr) + 1; //1S+1N+1I
                break;
            }
            case 0x58: case 0x59:
            {
                //LDR  Rd,[Rb,Ro]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 Ro = (opcode>>6)&7;
                u32 addr = CPU.R[Rb]+CPU.R[Ro];
                CPU.R[Rd] = GBA_MemoryRead32(addr);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(addr) + 1; //1S+1N+1I
                break;
            }
            case 0x5A: case 0x5B:
            {
                //LDRH Rd,[Rb,Ro]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 Ro = (opcode>>6)&7;
                u32 addr = CPU.R[Rb]+CPU.R[Ro];
                CPU.R[Rd] = (u32)(u16)GBA_MemoryRead16(addr&~1);
                if(addr&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr) + 1; //1S+1N+1I
                break;
            }
            case 0x5C: case 0x5D:
            {
                //LDRB Rd,[Rb,Ro]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 Ro = (opcode>>6)&7;
                u32 addr = CPU.R[Rb]+CPU.R[Ro];
                CPU.R[Rd] = (u32)(u8)GBA_MemoryRead8(addr);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr) + 1; //1S+1N+1I
                break;
            }
            case 0x5E: case 0x5F:
            {
                //LDSH Rd,[Rb,Ro]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 Ro = (opcode>>6)&7;
                u32 addr = CPU.R[Rb]+CPU.R[Ro];
                if(addr&1) CPU.R[Rd] = (s32)(s8)GBA_MemoryRead8(addr);
                else CPU.R[Rd] = (s32)(s16)GBA_MemoryRead16(addr);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr) + 1; //1S+1N+1I
                break;
            }
            case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
            {
                //STR  Rd,[Rb,#nn]
                u16 Rb = (opcode>>3)&7;
                u16 Rd = opcode&7;
                u16 offset = (opcode>>4)&(0x1F<<2);
                u32 addr = CPU.R[Rb]+offset;
                GBA_MemoryWrite32(addr,CPU.R[Rd]);
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(addr); //2N
                break;
            }
            case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
            {
                //LDR  Rd,[Rb,#nn]
                u16 Rb = (opcode>>3)&7;
                u16 Rd = opcode&7;
                u16 offset = (opcode>>4)&(0x1F<<2);
                u32 addr = CPU.R[Rb]+offset;
                CPU.R[Rd] = GBA_MemoryRead32(addr);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(addr) + 1; //1S+1N+1I
                break;
            }
            case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
            {
                //STRB  Rd,[Rb,#nn]
                u16 Rb = (opcode>>3)&7;
                u16 Rd = opcode&7;
                u16 offset = (opcode>>6)&0x1F;
                u32 addr = CPU.R[Rb]+offset;
                GBA_MemoryWrite8(addr,(u8)CPU.R[Rd]);
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr); //2N
                break;
            }
            case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            {
                //LDRB  Rd,[Rb,#nn]
                u16 Rb = (opcode>>3)&7;
                u16 Rd = opcode&7;
                u16 offset = (opcode>>6)&0x1F;
                u32 addr = CPU.R[Rb]+offset;
                CPU.R[Rd] = (u32)GBA_MemoryRead8(addr);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr) + 1; //1S+1N+1I
                break;
            }
            case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
            {
                //STRH  Rd,[Rb,#nn]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 offset = (opcode>>5)&(0x1F<<1);
                u32 addr = CPU.R[Rb]+offset;
                GBA_MemoryWrite16(addr,(u16)CPU.R[Rd]);
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr); //2N
                break;
            }
            case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
            {
                //LDRH Rd,[Rb,#nn]
                u16 Rd = opcode&7;
                u16 Rb = (opcode>>3)&7;
                u16 offset = (opcode>>5)&(0x1F<<1);
                u32 addr = CPU.R[Rb]+offset;
                CPU.R[Rd] = (u32)GBA_MemoryRead16(addr);
                if(addr&1) CPU.R[Rd] = ror_immed_no_carry(CPU.R[Rd],8);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq16(addr) + 1; //1S+1N+1I
                break;
            }
#define STR_REG_SP_IMM(Rd) \
{ \
    u32 offset = (opcode&0xFF)<<2; \
    u32 addr = CPU.R[R_SP]+offset; \
    GBA_MemoryWrite32(addr,CPU.R[Rd]); \
    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(addr); \
    break; \
} //2N
            //STR  Rd,[SP,#nn]
            case 0x90: STR_REG_SP_IMM(0);
            case 0x91: STR_REG_SP_IMM(1);
            case 0x92: STR_REG_SP_IMM(2);
            case 0x93: STR_REG_SP_IMM(3);
            case 0x94: STR_REG_SP_IMM(4);
            case 0x95: STR_REG_SP_IMM(5);
            case 0x96: STR_REG_SP_IMM(6);
            case 0x97: STR_REG_SP_IMM(7);

#define LDR_REG_SP_IMM(Rd) \
{ \
    u32 offset = (opcode&0xFF)<<2; \
    u32 addr = CPU.R[R_SP]+offset; \
    CPU.R[Rd] = GBA_MemoryRead32(addr); \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + GBA_MemoryGetAccessCyclesNoSeq32(addr) + 1; \
    break; \
} //1S+1N+1I
            //LDR  Rd,[SP,#nn]
            case 0x98: LDR_REG_SP_IMM(0);
            case 0x99: LDR_REG_SP_IMM(1);
            case 0x9A: LDR_REG_SP_IMM(2);
            case 0x9B: LDR_REG_SP_IMM(3);
            case 0x9C: LDR_REG_SP_IMM(4);
            case 0x9D: LDR_REG_SP_IMM(5);
            case 0x9E: LDR_REG_SP_IMM(6);
            case 0x9F: LDR_REG_SP_IMM(7);

#define ADD_REG_PC_IMM(Rd) \
{ \
    u32 offset = (opcode&0xFF)<<2; \
    CPU.R[Rd] = ((CPU.R[R_PC]+4)&(~2))+offset; \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); \
    break; \
} //1S cycle
            //ADD  Rd,PC,#nn
            case 0xA0: ADD_REG_PC_IMM(0);
            case 0xA1: ADD_REG_PC_IMM(1);
            case 0xA2: ADD_REG_PC_IMM(2);
            case 0xA3: ADD_REG_PC_IMM(3);
            case 0xA4: ADD_REG_PC_IMM(4);
            case 0xA5: ADD_REG_PC_IMM(5);
            case 0xA6: ADD_REG_PC_IMM(6);
            case 0xA7: ADD_REG_PC_IMM(7);

#define ADD_REG_SP_IMM(Rd) \
{ \
    u32 offset = (opcode&0xFF)<<2; \
    CPU.R[Rd] = CPU.R[R_SP]+offset; \
    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); \
    break; \
} //1S cycle
            //ADD  Rd,SP,#nn
            case 0xA8: ADD_REG_SP_IMM(0);
            case 0xA9: ADD_REG_SP_IMM(1);
            case 0xAA: ADD_REG_SP_IMM(2);
            case 0xAB: ADD_REG_SP_IMM(3);
            case 0xAC: ADD_REG_SP_IMM(4);
            case 0xAD: ADD_REG_SP_IMM(5);
            case 0xAE: ADD_REG_SP_IMM(6);
            case 0xAF: ADD_REG_SP_IMM(7);

            case 0xB0:
            {
                s32 offset = (opcode & 0x7F)<<2;
                if(opcode & BIT(7)) //ADD  SP,#-nn
                    CPU.R[R_SP] -= offset;
                else //ADD  SP,#nn
                    CPU.R[R_SP] += offset;
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0xB1: case 0xB2: case 0xB3:
            {
                THUMB_UNDEFINED_INSTRUCTION(); //Undefined Opcode #B -- tested in hardware
                break;
            }
            case 0xB4:
            {
                //PUSH {Rlist}
                u32 registers = (opcode & 0xFF);
                u32 bitcount = thumb_bit_count(registers);
                u32 address = CPU.R[R_SP] - (bitcount<<2);
                CPU.R[R_SP] = address;
                int i;
                for(i = 0; i < 8; i++) if(registers & BIT(i)) { thumb_stm(address,i); address += 4; }
                if(bitcount) clocks -= (GBA_MemoryGetAccessCyclesSeq32(address)*(bitcount-1)) +
                                    GBA_MemoryGetAccessCyclesNoSeq32(address); //(n-1)S+1N
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]); //1N cycle
                break;
            }
            case 0xB5:
            {
                //PUSH {Rlist,LR}
                u32 registers = (opcode & 0xFF) | BIT(R_LR);
                u32 bitcount = thumb_bit_count(registers);
                u32 address = CPU.R[R_SP] - (bitcount<<2);
                CPU.R[R_SP] = address;
                int i;
                for(i = 0; i < 16; i++) if(registers & BIT(i)) { thumb_stm(address,i); address += 4; }
                clocks -= (GBA_MemoryGetAccessCyclesSeq32(address)*(bitcount-1)) +
                                    GBA_MemoryGetAccessCyclesNoSeq32(address) +
                                    GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]); //(n-1)S+2N
                break;
            }
            case 0xB6: case 0xB7: case 0xB8: case 0xB9: case 0xBA: case 0xBB:
            {
                THUMB_UNDEFINED_INSTRUCTION(); //Undefined opcode #B -- tested on hardware
                break;
            }
            case 0xBC:
            {
                //POP {Rlist}
                u32 registers = (opcode & 0xFF);
                if(registers == 0)
                {
                    //Empty rlist triggers Undefined instruction exception. -- tested on hardware (not completely sure)
                    THUMB_UNDEFINED_INSTRUCTION(); //Undefined Opcode
                }
                else
                {
                    int i, count = 0;
                    for(i = 0; i < 8; i++) if(registers & BIT(i)) { thumb_ldm(CPU.R[R_SP],i); CPU.R[R_SP] += 4; count++; }
                    clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + 1;
                    if(count) clocks -= GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_SP]) +
                                        (GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_SP]) * (count-1)); //nS+1N+1I
                }
                break;

            }
            case 0xBD:
            {
                //POP {Rlist,PC}
                u32 registers = (opcode & 0xFF) | BIT(R_PC);
                int i, count = 0;
                for(i = 0; i < 16; i++) if(registers & BIT(i)) { thumb_ldm(CPU.R[R_SP],i); CPU.R[R_SP] += 4; count++; }
                CPU.R[R_PC] = (CPU.R[R_PC]-2)&(~1); //don't skip an instruction, don't change to arm
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]) + 1 +
                            GBA_MemoryGetAccessCyclesNoSeq32(CPU.R[R_SP]) +
                                    (GBA_MemoryGetAccessCyclesSeq32(CPU.R[R_SP]) * (count-1)) +
                                    GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]) +
                                    GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //(n+1)S+2N+1I (POP PC)
                break;
                //Empty rlist DOESN'T trigger Undefined instruction exception. -- tested on hardware
            }
            case 0xBE:
            {
                THUMB_UNDEFINED_INSTRUCTION(); //Undefined opcode #B -- tested on hardware
                break;
            }
            case 0xBF:
            {
                THUMB_UNDEFINED_INSTRUCTION(); //Undefined opcode #B -- tested on hardware
                break;
            }

#define STMIA(Rb) \
{ \
    u32 address = CPU.R[Rb]; \
    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC]); \
    int bitcount = 0; \
    int i; for(i = 0; i < 8; i++) if(opcode & BIT(i)) { thumb_stm(address,i); address += 4; bitcount++; } \
    if(bitcount) clocks -= (GBA_MemoryGetAccessCyclesSeq32(address)*(bitcount-1)) + \
                            GBA_MemoryGetAccessCyclesNoSeq32(address); \
    else CPU.R[Rb] += 0x40; \
    CPU.R[Rb] = address; \
    break; \
} //Execution Time: (n-1)S+2N
//empty rlist adds 0x40 to base register -- tested on hardware

            //STMIA Rb!,{Rlist}
            case 0xC0: STMIA(0);
            case 0xC1: STMIA(1);
            case 0xC2: STMIA(2);
            case 0xC3: STMIA(3);
            case 0xC4: STMIA(4);
            case 0xC5: STMIA(5);
            case 0xC6: STMIA(6);
            case 0xC7: STMIA(7);

#define LDMIA(Rb) \
{ \
    if((opcode & 0xFF) == 0) \
    { \
        THUMB_UNDEFINED_INSTRUCTION(); \
    } \
    else \
    { \
        u32 address = CPU.R[Rb]; \
        clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); \
        int bitcount = 0; \
        int i; for(i = 0; i < 8; i++) if(opcode & BIT(i)) { thumb_ldm(address,i); address += 4; bitcount++; } \
        if(bitcount) clocks -= (GBA_MemoryGetAccessCyclesSeq32(address)*(bitcount-1)) + \
                                GBA_MemoryGetAccessCyclesNoSeq32(address) + 1; \
        CPU.R[Rb] = address; \
        break; \
    } \
} //Execution Time: nS+1N+1I
//empty rlist == undefined instruction. tested on hardware, but not sure about it...

            //LDMIA Rb!,{Rlist}
            case 0xC8: LDMIA(0);
            case 0xC9: LDMIA(1);
            case 0xCA: LDMIA(2);
            case 0xCB: LDMIA(3);
            case 0xCC: LDMIA(4);
            case 0xCD: LDMIA(5);
            case 0xCE: LDMIA(6);
            case 0xCF: LDMIA(7);

            case 0xD0:
            {
                //BEQ label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if(CPU.CPSR & F_Z)
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD1:
            {
                //BNE label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if( (CPU.CPSR & F_Z) == 0 )
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD2:
            {
                //BCS label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if(CPU.CPSR & F_C)
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD3:
            {
                //BCC label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if( (CPU.CPSR & F_C) == 0 )
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD4:
            {
                //BMI label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if(CPU.CPSR & F_N)
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD5:
            {
                //BPL label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if( (CPU.CPSR & F_N) == 0 )
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD6:
            {
                //BVS label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if(CPU.CPSR & F_V)
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD7:
            {
                //BVC label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if( (CPU.CPSR & F_V) == 0 )
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD8:
            {
                //BHI label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if((CPU.CPSR & (F_C|F_Z)) == F_C)
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xD9:
            {
                //BLS label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if( ((CPU.CPSR & F_C) == 0) || (CPU.CPSR & F_Z) )
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xDA:
            {
                //BGE label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if(!( ((CPU.CPSR & F_N) != 0) ^ ((CPU.CPSR & F_V) != 0) ))
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xDB:
            {
                //BLT label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if( ((CPU.CPSR & F_N) != 0) ^ ((CPU.CPSR & F_V) != 0) )
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xDC:
            {
                //BGT label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if(  ((CPU.CPSR & F_Z) == 0) && !( ((CPU.CPSR & F_N) != 0) ^ ((CPU.CPSR & F_V) != 0) )  )
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xDD:
            {
                //BLE label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                if( (CPU.CPSR & F_Z) || ( ((CPU.CPSR & F_N) != 0) ^ ((CPU.CPSR & F_V) != 0) ) )
                {
                    u16 data = opcode & 0xFF;
                    CPU.R[R_PC] = CPU.R[R_PC]+4+( ((s32)(s8)data) << 1 );
                    clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                    CPU.R[R_PC] -= 2;
                }
                break;
            }
            case 0xDE:
            {
                //B{cond} with cond = always -- Tested in real hardware
                THUMB_UNDEFINED_INSTRUCTION(); //Undefined opcode #D
                break;
            }
            case 0xDF:
            {
                //SWI nn
                if(GBA_BiosIsLoaded()==0)
                {
                    u32 swinummber = opcode&0xFF;
                    if(swinummber == 5)
                    {
                        CPU.R[0] = 1;
                        CPU.R[1] = 1;
                        swinummber = 4;
                    }
                    if(swinummber != 4)
                    {
                        CPU.R[R_PC] +=2;
                        GBA_Swi(swinummber);
                        clocks -= 50;
                        return clocks;
                        //return GBA_ExecuteARM(clocks);
                    }
                }

                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                CPU.R14_svc = CPU.R[R_PC]+2; //save return address
                CPU.SPSR_svc = CPU.CPSR; //save CPSR flags
                GBA_CPUChangeMode(M_SUPERVISOR); //Enter svc, ARM state, IRQs disabled
                CPU.EXECUTION_MODE = EXEC_ARM;
                CPU.CPSR &= ~(F_T|F_I|0x1F);
                CPU.CPSR |= M_SUPERVISOR|F_I;
                //CPU.R[R_PC] = 0x00000008; CPU.R[R_PC] -= 2; //jump to SWI vector address
                CPU.R[R_PC] = 0x00000008;
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                return clocks;
                //return GBA_ExecuteARM(clocks);
            }
            case 0xE0: case 0xE1: case 0xE2: case 0xE3:
            {
                //B label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                s32 offset = (opcode&0x3FF)<<1;
                CPU.R[R_PC] = CPU.R[R_PC]+4+offset;
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                CPU.R[R_PC] -= 2;
                break;
            }
            case 0xE4: case 0xE5: case 0xE6: case 0xE7:
            {
                //B label
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                s32 offset = (opcode&0x3FF)<<1;
                offset |= 0xFFFFF800;
                CPU.R[R_PC] = CPU.R[R_PC]+4+offset;
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                CPU.R[R_PC] -= 2;
                break;
            }
            case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEE: case 0xEF:
            {
                THUMB_UNDEFINED_INSTRUCTION(); //Undefined Opcode #E -- tested on hardware
                break;
            }
            case 0xF0: case 0xF1: case 0xF2: case 0xF3:
            {
                //BL label -- First part
                //LR = PC+4+(nn SHL 12)
                CPU.R[R_LR] = CPU.R[R_PC] + 4 + (((u32)opcode & 0x7FF) << 12);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0xF4: case 0xF5: case 0xF6: case 0xF7:
            {
                //BL label -- First part
                //LR = PC+4+(nn SHL 12)
                CPU.R[R_LR] = CPU.R[R_PC] + 4 + ((((u32)opcode & 0x7FF) << 12) | 0xFF800000);
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                break;
            }
            case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFE: case 0xFF:
            {
                //BL label -- Second part
                //PC = LR + (nn SHL 1), and LR = PC+2 OR 1
                clocks -= GBA_MemoryGetAccessCycles(PCseq,0,CPU.R[R_PC]); //1S cycle
                u32 temp = CPU.R[R_LR] + (((u32)opcode & 0x7FF) << 1);
                CPU.R[R_LR] = (CPU.R[R_PC] + 2) | 1;
                CPU.R[R_PC] = temp;
                clocks -= GBA_MemoryGetAccessCyclesNoSeq16(CPU.R[R_PC])+GBA_MemoryGetAccessCyclesSeq16(CPU.R[R_PC]); //1S+1N
                CPU.R[R_PC] -= 2;
                break;
            }
        }

        CPU.R[R_PC] +=2; //CPU.R[R_PC] = (CPU.R[R_PC] + 2) & ~1;
    }

    return clocks;
}

