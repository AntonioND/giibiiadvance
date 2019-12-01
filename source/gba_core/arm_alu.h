// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_ARM_ALU__
#define GBA_ARM_ALU__

//#define POS(n) ((~(u32)(n)) >> 31)
//#define NEG(n) (((u32)(n)) >> 31)
//#define ADD_OVERFLOW(a, b, res) \
//              ((NEG(a) & NEG(b) & POS(res)) || (POS(a) & POS(b) & NEG(res)))
//#define ADD_OVERFLOW(a ,b, res) \
//              ((((~(a)) & (~(b)) & (res)) | ((a) & (b) & (~(res))) ) >> 31 )
#define ADD_OVERFLOW(a, b, res) \
                (( ((~((a) | (b))) & (res)) | ((a) & (b) & (~(res))) ) >> 31 )
//#define SUB_OVERFLOW(a, b, res) \
//              ((POS(a) & NEG(b) & NEG(res)) || (NEG(a) & POS(b) & POS(res)))

static void arm_and_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              & ror_immed_no_carry(val, ror_bits);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_ands_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              & ror_immed(val, ror_bits, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_eor_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              ^ ror_immed_no_carry(val, ror_bits);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_eors_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              ^ ror_immed(val, ror_bits, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_sub_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    // Like ADD, but changing the sign of the second operand
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              - ror_immed_no_carry(val, ror_bits);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_subs_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    //Like ADD, but changing the sign of the second operand
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u64 t2 = ~ror_immed_no_carry(val, ror_bits);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N)
               | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
               | (ADD_OVERFLOW(t1, (u32)t2, CPU.R[Rd]) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_rsb_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = ror_immed_no_carry(val, ror_bits)
              - (CPU.R[Rn] + (Rn == 15 ? 8 : 0));
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_rsbs_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    // Like ADD, but changing the sign of the second operand
    u64 t1 = ~(CPU.R[Rn] + (Rn == 15 ? 8 : 0));
    u32 t2 = ror_immed_no_carry(val, ror_bits);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW((u32)t1, t2, CPU.R[Rd]) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_add_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              + ror_immed_no_carry(val, ror_bits);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_adds_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u32 t2 = ror_immed_no_carry(val, ror_bits);
    u64 temp = (u64)t1 + (u64)t2;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, CPU.R[Rd]) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_adc_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              + ror_immed_no_carry(val, ror_bits) + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_adcs_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u32 t2 = ror_immed_no_carry(val, ror_bits);
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR & F_C) ? 1 : 0);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, CPU.R[Rd]) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_sbc_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              + ~ror_immed_no_carry(val, ror_bits) + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_sbcs_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u64 t2 = (u64)~ror_immed_no_carry(val, ror_bits);
    u64 temp = (u64)t1 + (u64)t2 + ((CPU.CPSR & F_C) ? 1ULL : 0ULL);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_rsc_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = ror_immed_no_carry(val, ror_bits)
              + (~(CPU.R[Rn] + (Rn == 15 ? 8 : 0))) + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_rscs_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    u32 t1 = ror_immed_no_carry(val, ror_bits);
    u64 t2 = (u64)~(CPU.R[Rn] + (Rn == 15 ? 8 : 0));
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR & F_C) ? 1 : 0);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_tst_immed(u32 Rn, u32 val, u32 ror_bits)
{
    u8 carry;
    u32 tmp = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
            & ror_immed(val, ror_bits, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (tmp ? 0 : F_Z) | (tmp & F_N) | (carry ? F_C : 0);
}

static void arm_teq_immed(u32 Rn, u32 val, u32 ror_bits)
{
    u8 carry;
    u32 tmp = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
            ^ ror_immed(val, ror_bits, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (tmp ? 0 : F_Z) | (tmp & F_N) | (carry ? F_C : 0);
}

static void arm_cmp_immed(u32 Rn, u32 val, u32 ror_bits)
{
    // Like ADD, but changing the sign of the second operand
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u32 t2 = ~ror_immed_no_carry(val, ror_bits);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= ((u32)temp ? 0 : F_Z)
              | (((u32)temp) & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
}

static void arm_cmn_immed(u32 Rn, u32 val, u32 ror_bits)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u32 t2 = ror_immed_no_carry(val, ror_bits);
    u64 temp = (u64)t1 + (u64)t2;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= ((u32)temp ? 0 : F_Z)
              | (((u32)temp) & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, (u32)temp) ? F_V : 0);
}

static void arm_orr_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              | ror_immed_no_carry(val, ror_bits);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_orrs_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              | ror_immed(val, ror_bits, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_mov_immed(u32 Rd, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = ror_immed_no_carry(val, ror_bits);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_movs_immed(u32 Rd, u32 val, u32 ror_bits)
{
    u8 carry;
    CPU.R[Rd] = ror_immed(val, ror_bits, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_bic_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              & ~ror_immed_no_carry(val, ror_bits);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_bics_immed(u32 Rd, u32 Rn, u32 val, u32 ror_bits)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              & ~ror_immed(val, ror_bits, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_mvn_immed(u32 Rd, u32 val, u32 ror_bits)
{
    CPU.R[Rd] = ~ror_immed_no_carry(val, ror_bits);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_mvns_immed(u32 Rd, u32 val, u32 ror_bits)
{
    u8 carry;
    CPU.R[Rd] = ~ror_immed(val, ror_bits, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

//------------------------------------------------------------------------------

static void arm_and_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              & cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs]);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_ands_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              & cpu_shift_by_reg(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                 CPU.R[Rs], &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_eor_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              ^ cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs]);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_eors_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              ^ cpu_shift_by_reg(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                 CPU.R[Rs], &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_sub_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              - cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs]);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_subs_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    // Like ADD, but changing the sign of the second operand
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 12 : 0);
    u64 t2 = ~cpu_shift_by_reg_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                        CPU.R[Rs]);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, CPU.R[Rd]) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_rsb_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs])
              - (CPU.R[Rn] + (Rn == 15 ? 12 : 0));
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_rsbs_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    // Like ADD, but changing the sign of the first operand
    u64 t1 = ~(CPU.R[Rn] + (Rn == 15 ? 12 : 0));
    u32 t2 = cpu_shift_by_reg_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                       CPU.R[Rs]);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW((u32)t1, t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_add_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              + cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs]);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_adds_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 12 : 0);
    u32 t2 = cpu_shift_by_reg_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                       CPU.R[Rs]);
    u64 temp = (u64)t1 + (u64)t2;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_adc_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              + cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs])
              + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_adcs_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 12 : 0);
    u32 t2 = cpu_shift_by_reg_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                       CPU.R[Rs]);
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR & F_C) ? 1 : 0);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_sbc_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              + ~cpu_shift_by_reg_no_carry(shift,
                                           CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                           CPU.R[Rs])
              + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_sbcs_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 12 : 0);
    u64 t2 = (u64)~cpu_shift_by_reg_no_carry(shift,
                                             CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                             CPU.R[Rs]);
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR & F_C) ? 1 : 0);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_rsc_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs])
              + ~(CPU.R[Rn] + (Rn == 15 ? 12 : 0)) + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_rscs_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u64 t1 = (u64)~(CPU.R[Rn] + (Rn == 15 ? 12 : 0));
    u32 t2 = cpu_shift_by_reg_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                       CPU.R[Rs]);
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR & F_C) ? 1 : 0);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW((u32)t1, t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_tst_rshiftr(u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u8 carry;
    u32 tmp = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
            & cpu_shift_by_reg(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                               CPU.R[Rs], &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (tmp ? 0 : F_Z) | (tmp & F_N) | (carry ? F_C : 0);
}

static void arm_teq_rshiftr(u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u8 carry;
    u32 tmp = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
            ^ cpu_shift_by_reg(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                               CPU.R[Rs], &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (tmp ? 0 : F_Z) | (tmp & F_N) | (carry ? F_C : 0);
}

static void arm_cmp_rshiftr(u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    // Like ADD, but changing the sign of the second operand
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 12 : 0);
    u64 t2 = ~cpu_shift_by_reg_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                        CPU.R[Rs]);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= ((u32)temp ? 0 : F_Z)
              | (((u32)temp) & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
}

static void arm_cmn_rshiftr(u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 12 : 0);
    u32 t2 = cpu_shift_by_reg_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                       CPU.R[Rs]);
    u64 temp = (u64)t1 + (u64)t2;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= ((u32)temp ? 0 : F_Z)
              | (((u32)temp) & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, (u32)temp) ? F_V : 0);
}

static void arm_orr_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              | cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs]);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_orrs_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              | cpu_shift_by_reg(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                 CPU.R[Rs], &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_mov_rshiftr(u32 Rd, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = cpu_shift_by_reg_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                          CPU.R[Rs]);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_movs_rshiftr(u32 Rd, u32 Rm, u32 shift, u32 Rs)
{
    u8 carry;
    CPU.R[Rd] = cpu_shift_by_reg(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                 CPU.R[Rs], &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_bic_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              & ~cpu_shift_by_reg_no_carry(shift,
                                           CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                           CPU.R[Rs]);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_bics_rshiftr(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 Rs)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 12 : 0))
              & ~cpu_shift_by_reg(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                  CPU.R[Rs], &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_mvn_rshiftr(u32 Rd, u32 Rm, u32 shift, u32 Rs)
{
    CPU.R[Rd] = ~cpu_shift_by_reg_no_carry(shift,
                                           CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                           CPU.R[Rs]);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_mvns_rshiftr(u32 Rd, u32 Rm, u32 shift, u32 Rs)
{
    u8 carry;
    CPU.R[Rd] = ~cpu_shift_by_reg(shift, CPU.R[Rm] + (Rm == 15 ? 12 : 0),
                                  CPU.R[Rs], &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

//------------------------------------------------------------------------------

static void arm_and_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              & cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] +(Rm == 15 ? 8 : 0),
                                            value);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_ands_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              & cpu_shift_by_immed(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                   value,&carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_eor_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              ^ cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                            value);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_eors_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              ^ cpu_shift_by_immed(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                   value,&carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_sub_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              - cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                            value);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_subs_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    // Like ADD, but changing the sign of the second operand
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u64 t2 = ~cpu_shift_by_immed_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                          value);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_rsb_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                            value)
              - (CPU.R[Rn] + (Rn == 15 ? 8 : 0));
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_rsbs_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u64 t1 = ~(CPU.R[Rn] + (Rn == 15 ? 8 : 0));
    u32 t2 = cpu_shift_by_immed_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                         value);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW((u32)t1, t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_add_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              + cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                            value);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_adds_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u32 t2 = cpu_shift_by_immed_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                         value);
    u64 temp = (u64)t1 + (u64)t2;
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_adc_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              + cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                            value)
              + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_adcs_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u32 t2 = cpu_shift_by_immed_no_carry(shift,
                                         CPU.R[Rm] + (Rm == 15 ? 8 : 0), value);
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR & F_C) ? 1 : 0);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_sbc_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              + ~cpu_shift_by_immed_no_carry(shift,
                                             CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                             value)
              + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_sbcs_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u64 t2 = (u64)~cpu_shift_by_immed_no_carry(shift,
                                               CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                               value);
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR & F_C) ? 1 :0);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_rsc_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                            value)
              + ~(CPU.R[Rn] + (Rn == 15 ? 8 : 0)) + ((CPU.CPSR & F_C) ? 1 : 0);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_rscs_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u64 t1 = (u64)~(CPU.R[Rn] + (Rn == 15 ? 8 : 0));
    u32 t2 = cpu_shift_by_immed_no_carry(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                         value);
    u64 temp = (u64)t1 + (u64)t2 + (u64)((CPU.CPSR & F_C) ? 1 : 0);
    CPU.R[Rd] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z)
              | (CPU.R[Rd] & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_tst_rshifti(u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u8 carry;
    u32 tmp = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
            & cpu_shift_by_immed(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                 value, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (tmp ? 0 : F_Z) | (tmp & F_N) | (carry ? F_C : 0);
}

static void arm_teq_rshifti(u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u8 carry;
    u32 tmp = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
            ^ cpu_shift_by_immed(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                 value, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (tmp ? 0 : F_Z) | (tmp & F_N) | (carry ? F_C : 0);
}

static void arm_cmp_rshifti(u32 Rn, u32 Rm, u32 shift, u32 value)
{
    // Like ADD, but changing the sign of the second operand
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u64 t2 = ~cpu_shift_by_immed_no_carry(shift,
                                          CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                          value);
    u64 temp = (u64)t1 + (u64)t2 + 1ULL;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= ((u32)temp ? 0 : F_Z)
              | (((u32)temp) & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, (u32)t2, (u32)temp) ? F_V : 0);
}

static void arm_cmn_rshifti(u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u32 t1 = CPU.R[Rn] + (Rn == 15 ? 8 : 0);
    u32 t2 = cpu_shift_by_immed_no_carry(shift,
                                         CPU.R[Rm] + (Rm == 15 ? 8 : 0), value);
    u64 temp = (u64)t1 + (u64)t2;
    CPU.CPSR &= ~(F_Z | F_C | F_N | F_V);
    CPU.CPSR |= ((u32)temp ? 0 : F_Z)
              | (((u32)temp) & F_N)
              | ((temp & 0xFFFFFFFF00000000ULL) ? F_C : 0)
              | (ADD_OVERFLOW(t1, t2, (u32)temp) ? F_V : 0);
}

static void arm_orr_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              | cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                            value);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_orrs_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              | cpu_shift_by_immed(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                   value, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_mov_rshifti(u32 Rd, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = cpu_shift_by_immed_no_carry(shift,
                                            CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                            value);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_movs_rshifti(u32 Rd, u32 Rm, u32 shift, u32 value)
{
    u8 carry;
    CPU.R[Rd] = cpu_shift_by_immed(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                   value, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_bic_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              & ~cpu_shift_by_immed_no_carry(shift,
                                             CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                             value);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_bics_rshifti(u32 Rd, u32 Rn, u32 Rm, u32 shift, u32 value)
{
    u8 carry;
    CPU.R[Rd] = (CPU.R[Rn] + (Rn == 15 ? 8 : 0))
              & ~cpu_shift_by_immed(shift,
                                    CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                    value, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

static void arm_mvn_rshifti(u32 Rd, u32 Rm, u32 shift, u32 value)
{
    CPU.R[Rd] = ~cpu_shift_by_immed_no_carry(shift,
                                             CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                             value);
    if (Rd == 15)
        CPU.R[R_PC] -= 4;
}

static void arm_mvns_rshifti(u32 Rd, u32 Rm, u32 shift, u32 value)
{
    u8 carry;
    CPU.R[Rd] = ~cpu_shift_by_immed(shift, CPU.R[Rm] + (Rm == 15 ? 8 : 0),
                                    value, &carry);
    CPU.CPSR &= ~(F_Z | F_C | F_N);
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N) | (carry ? F_C : 0);
    if (Rd == 15)
    {
        if (CPU.MODE != CPU_USER)
        {
            CPU.CPSR = CPU.SPSR;
            GBA_CPUChangeMode(CPU.CPSR & 0x1F);
        }
        CPU.R[R_PC] -= 4;
    }
}

//------------------------------------------------------------------------------

static int arm_signed_mul_extra_cycles(u32 Rd)
{
    u32 temp = CPU.R[Rd];

    if (temp & BIT(31))
        temp = ~temp; // All 0 or all 1

    if (temp & 0xFFFFFF00)
    {
        if (temp & 0xFFFF0000)
        {
            if (temp & 0xFF000000)
            {
                return 4;
            }
            else
            {
                return 3;
            }
        }
        else
        {
            return 2;
        }
    }
    else
    {
        return 1;
    }
}

static int arm_unsigned_mul_extra_cycles(u32 Rd)
{
    u32 temp = CPU.R[Rd]; // All 0

    if (temp & 0xFFFFFF00)
    {
        if (temp & 0xFFFF0000)
        {
            if (temp & 0xFF000000)
            {
                return 4;
            }
            else
            {
                return 3;
            }
        }
        else
        {
            return 2;
        }
    }
    else
    {
        return 1;
    }
}

static void arm_mul(u32 Rd, u32 Rm, u32 Rs)
{
    CPU.R[Rd] = (u32)(((s32)CPU.R[Rm]) * ((s32)CPU.R[Rs]));
}

static void arm_muls(u32 Rd, u32 Rm, u32 Rs)
{
    CPU.R[Rd] = ((s32)CPU.R[Rm]) * ((s32)CPU.R[Rs]);
    CPU.CPSR &= ~(F_Z | F_N); // Carry destroyed
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N);
}

static void arm_mla(u32 Rd, u32 Rm, u32 Rs, u32 Rn)
{
    CPU.R[Rd] = (u32)((((s32)CPU.R[Rm]) * ((s32)CPU.R[Rs])) + (s32)CPU.R[Rn]);
}

static void arm_mlas(u32 Rd, u32 Rm, u32 Rs, u32 Rn)
{
    CPU.R[Rd] = (((s32)CPU.R[Rm]) * ((s32)CPU.R[Rs])) + (s32)CPU.R[Rn];
    CPU.CPSR &= ~(F_Z|F_N); // Carry destroyed
    CPU.CPSR |= (CPU.R[Rd] ? 0 : F_Z) | (CPU.R[Rd] & F_N);
}

static void arm_umull(u32 RdLo, u32 RdHi, u32 Rm, u32 Rs)
{
    u64 temp = ((u64)CPU.R[Rm]) * ((u64)CPU.R[Rs]);
    CPU.R[RdHi] = (u32)(((u64)temp) >> 32);
    CPU.R[RdLo] = (u32)temp;
}

static void arm_umulls(u32 RdLo, u32 RdHi, u32 Rm, u32 Rs)
{
    u64 temp = ((u64)CPU.R[Rm]) * ((u64)CPU.R[Rs]);
    CPU.R[RdHi] = (u32)(((u64)temp) >> 32);
    CPU.R[RdLo] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_N); // Carry destroyed, V destroyed?
    CPU.CPSR |= (temp ? 0 : F_Z) | (CPU.R[RdHi] & F_N);
}

static void arm_umlal(u32 RdLo, u32 RdHi, u32 Rm, u32 Rs)
{
    u64 temp = (((u64)CPU.R[Rm]) * ((u64)CPU.R[Rs]))
             + ((((u64)CPU.R[RdHi]) << 32) | (u64)CPU.R[RdLo]);
    CPU.R[RdHi] = (u32)(((u64)temp) >> 32);
    CPU.R[RdLo] = (u32)temp;
}

static void arm_umlals(u32 RdLo, u32 RdHi, u32 Rm, u32 Rs)
{
    u64 temp = (((u64)CPU.R[Rm]) * ((u64)CPU.R[Rs]))
             + ((((u64)CPU.R[RdHi]) << 32) | (u64)CPU.R[RdLo]);
    CPU.R[RdHi] = (u32)(((u64)temp) >> 32);
    CPU.R[RdLo] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_N); // Carry destroyed, V destroyed?
    CPU.CPSR |= (temp ? 0 : F_Z) | (CPU.R[RdHi] & F_N);
}

static void arm_smull(u32 RdLo, u32 RdHi, u32 Rm, u32 Rs)
{
    s64 temp = ((s64)(s32)CPU.R[Rm]) * ((s64)(s32)CPU.R[Rs]);
    CPU.R[RdHi] = (u32)(((u64)temp) >> 32);
    CPU.R[RdLo] = (u32)temp;
}

static void arm_smulls(u32 RdLo, u32 RdHi, u32 Rm, u32 Rs)
{
    s64 temp = ((s64)(s32)CPU.R[Rm]) * ((s64)(s32)CPU.R[Rs]);
    CPU.R[RdHi] = (u32)(((u64)temp) >> 32);
    CPU.R[RdLo] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_N); // Carry destroyed, V destroyed?
    CPU.CPSR |= (temp ? 0 : F_Z) | (CPU.R[RdHi] & F_N);
}

static void arm_smlal(u32 RdLo, u32 RdHi, u32 Rm, u32 Rs)
{
    s64 temp = (((s64)(s32)CPU.R[Rm]) * ((s64)(s32)CPU.R[Rs]))
             + ((((u64)CPU.R[RdHi]) << 32) | (u64)CPU.R[RdLo]);
    CPU.R[RdHi] = (u32)(((u64)temp) >> 32);
    CPU.R[RdLo] = (u32)temp;
}

static void arm_smlals(u32 RdLo, u32 RdHi, u32 Rm, u32 Rs)
{
    s64 temp = (((s64)(s32)CPU.R[Rm]) * ((s64)(s32)CPU.R[Rs]))
             + ((((u64)CPU.R[RdHi]) << 32) | (u64)CPU.R[RdLo]);
    CPU.R[RdHi] = (u32)(((u64)temp) >> 32);
    CPU.R[RdLo] = (u32)temp;
    CPU.CPSR &= ~(F_Z | F_N); // Carry destroyed, V destroyed?
    CPU.CPSR |= (temp ? 0 : F_Z) | (CPU.R[RdHi] & F_N);
}

#endif // GBA_ARM_ALU__
