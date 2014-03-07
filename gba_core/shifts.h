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

#ifndef __GBA_SHIFTS__
#define __GBA_SHIFTS__

#include "../build_options.h"

//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------

//For building immediate values in ARM mode (shift = 0~31)
//--------------------------------------------------------

static inline u32 ror_immed_no_carry(u32 value, u8 shift)
{
#ifdef USE_ASM
    asm("ror %%cl,%%eax \n\t" : "=a" (value) : "a" (value), "c" (shift));
    return value;
#else
    return ( (value >> shift) | (value<<(32-shift)) );
#endif
}

static inline u32 ror_immed(u32 value, u8 shift, u8 * carry)
{
    if(shift)
    {
#ifdef USE_ASM
        asm("ror %%cl,%%eax \n\t"
            "setc (%%ebx) \n\t"
            : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
        return value;
#else
        *carry = (value & BIT(shift-1)) != 0;
        return ( (value >> shift) | (value<<(32-shift)) );
#endif
    }
    else
    {
        *carry = (CPU.CPSR & F_C) != 0;
        return value;
    }
}

//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------

//For shifted by immediate values registers (shift = 0~31)
//--------------------------------------------------------

static inline u32 lsl_shift_by_immed(u32 value, u8 shift, u8 * carry)
{
    if(shift == 0) { *carry = (CPU.CPSR&F_C) != 0; return value; } //lsl #0
    else
    {
#ifdef USE_ASM
        asm("shl %%cl,%%eax \n\t"
            "setc (%%ebx) \n\t"
            : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
        return value;
#else
        *carry = (value&BIT(32-shift)) != 0;
        return ( value << shift );
#endif
    }
}

static inline u32 lsr_shift_by_immed(u32 value, u8 shift, u8 * carry)
{
    if(shift == 0) { *carry = (value&BIT(31)) != 0; return 0; } //lsr #32
    else
    {
#ifdef USE_ASM
        asm("shr %%cl,%%eax \n\t"
            "setc (%%ebx) \n\t"
            : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
        return value;
#else
        *carry = (value&BIT(shift-1)) != 0;
        return ( value >> shift );
#endif
    }
}

static inline u32 asr_shift_by_immed(u32 value, u8 shift, u8 * carry)
{
    if(shift == 0) { *carry = (value&BIT(31)) != 0; return (*carry) ? 0xFFFFFFFF : 0; } //asr #32
    else
    {
#ifdef USE_ASM
        asm("sar %%cl,%%eax \n\t"
            "setc (%%ebx) \n\t"
            : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
        return value;
#else
        *carry = (value&BIT(shift-1)) != 0;
        return (u32)( ((s32)value) >> shift );
#endif
    }
}

static inline u32 ror_shift_by_immed(u32 value, u8 shift, u8 * carry)
{
    if(shift == 0)  //rrx #1
    {
        *carry = value&BIT(0);
        return ( (value >> 1) | ((CPU.CPSR&F_C)?BIT(31):0) );
    }
    else
    {
#ifdef USE_ASM
        asm("ror %%cl,%%eax \n\t"
            "setc (%%ebx) \n\t"
            : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
        return value;
#else
        *carry = (value & BIT(shift-1)) != 0;
        return ( (value >> shift) | (value<<(32-shift)) );
#endif
    }
}

static inline u32 cpu_shift_by_immed(u32 type, u32 value, u8 shift, u8 * carry)
{
    switch(type&3)
    {
        case 0: return lsl_shift_by_immed(value, shift, carry);
        case 1: return lsr_shift_by_immed(value, shift, carry);
        case 2: return asr_shift_by_immed(value, shift, carry);
        case 3: return ror_shift_by_immed(value, shift, carry);
        default: return 0;
    }
    return 0;
}

//-----------------------------------------------

static inline u32 lsl_shift_by_immed_no_carry(u32 value, u8 shift)
{
    if(shift == 0) { return value; } //lsl #0
    else
    {
#ifdef USE_ASM
        asm("shl %%cl,%%eax \n\t" : "=a" (value) : "a" (value), "c" (shift));
        return value;
#else
        return ( value << shift );
#endif
    }
}

static inline u32 lsr_shift_by_immed_no_carry(u32 value, u8 shift)
{
    if(shift == 0) { return 0; } //lsr #32
    else
    {
#ifdef USE_ASM
        asm("shr %%cl,%%eax \n\t" : "=a" (value) : "a" (value), "c" (shift));
        return value;
#else
        return ( value >> shift );
#endif
    }
}

static inline u32 asr_shift_by_immed_no_carry(u32 value, u8 shift)
{
    if(shift == 0) { return (value&BIT(31)) ? 0xFFFFFFFF : 0; } //asr #32
    else
    {
#ifdef USE_ASM
        asm("sar %%cl,%%eax \n\t" : "=a" (value) : "a" (value), "c" (shift));
        return value;
#else
        return (u32)( ((s32)value) >> shift );
#endif
    }
}

static inline u32 ror_shift_by_immed_no_carry(u32 value, u8 shift)
{
    if(shift == 0) { return ( (value >> 1) | ((CPU.CPSR&F_C)?BIT(31):0) ); } //rrx #1
    else
    {
#ifdef USE_ASM
        asm("ror %%cl,%%eax \n\t" : "=a" (value) : "a" (value), "c" (shift));
        return value;
#else
        return ( (value >> shift) | (value<<(32-shift)) );
#endif
    }
}

static inline u32 cpu_shift_by_immed_no_carry(u32 type, u32 value, u8 shift)
{
    switch(type&3)
    {
        case 0: return lsl_shift_by_immed_no_carry(value,shift);
        case 1: return lsr_shift_by_immed_no_carry(value,shift);
        case 2: return asr_shift_by_immed_no_carry(value,shift);
        case 3: return ror_shift_by_immed_no_carry(value,shift);
        default: return 0;
    }
    return 0;
}

//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------

//For shifted-by-register registers (shift = 0~0xFF) - Used only in ALU operations
//--------------------------------------------------

static inline u32 lsl_shift_by_reg(u32 value, u8 shift, u8 * carry)
{
    if(shift)
    {
        if(shift < 32)
        {
#ifdef USE_ASM
            asm("shl %%cl,%%eax \n\t"
                "setc (%%ebx) \n\t"
                : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
            return value;
#else
            *carry = (value&BIT(32-shift)) != 0;
            return value<<shift;
#endif
        }
        else if(shift == 32) { *carry = (value&BIT(0)) != 0; return 0; }
        else { *carry = 0; return 0; }
    }
    else { *carry = (CPU.CPSR&F_C) != 0; return value; } //don't modify carry
}

static inline u32 lsr_shift_by_reg(u32 value, u8 shift, u8 * carry)
{
    if(shift)
    {

        if(shift < 32)
        {
#ifdef USE_ASM
            asm("shr %%cl,%%eax \n\t"
                "setc (%%ebx) \n\t"
                : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
            return value;
#else
            *carry = (value&BIT(shift-1)) != 0;
            return value>>shift;
#endif
        }
        else if(shift == 32) { *carry = (value&BIT(31)) != 0; return 0; }
        else { *carry = 0; return 0; }

    }
    else { *carry = (CPU.CPSR&F_C) != 0; return value; }
    //else { *carry = value&BIT(31); return 0; }
}

static inline u32 asr_shift_by_reg(u32 value, u8 shift, u8 * carry)
{
    if(shift)
    {
        if(shift < 32)
        {
#ifdef USE_ASM
            asm("sar %%cl,%%eax \n\t"
                "setc (%%ebx) \n\t"
                : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
            return value;
#else
            *carry = (value&BIT(shift-1)) != 0;
            return (u32)(((s32)value)>>shift);
#endif
        }
        else { *carry = (value&BIT(31)) != 0; return (u32)(((s32)value)>>31); }
    }
    else { *carry = (CPU.CPSR&F_C) != 0; return value; }
    //else { *carry = (value>>31); return (u32)(((s32)value)>>31); }
}

static inline u32 ror_shift_by_reg(u32 value, u8 shift, u8 * carry)
{
    if(shift)
    {
        shift &= 31;
        if(shift)
        {
#ifdef USE_ASM
            asm("ror %%cl,%%eax \n\t"
                "setc (%%ebx) \n\t"
                : "=a" (value) : "a" (value), "b" (carry), "c" (shift));
            return value;
#else
            *carry = (value & BIT(shift-1)) != 0;
            return ( (value >> shift) | (value<<(32-shift)) );
#endif
        }
        else
        {
            *carry = (value & BIT(31)) != 0;
            return value;
        }
    }
    else { *carry = (CPU.CPSR&F_C) != 0; return value; }
    //else { *carry = (value&1); return ((CPU.CPSR&F_C)<<2) | (value >> 1); }
}

static inline u32 cpu_shift_by_reg(u32 type, u32 value, u8 shift, u8 * carry)
{
    switch(type&3)
    {
        case 0: return lsl_shift_by_reg(value,shift,carry);
        case 1: return lsr_shift_by_reg(value,shift,carry);
        case 2: return asr_shift_by_reg(value,shift,carry);
        case 3: return ror_shift_by_reg(value,shift,carry);
        default: return 0;
    }
    return 0;
}

//-----------------------------------------------

static inline u32 lsl_shift_by_reg_no_carry(u32 value, u8 shift)
{
    if(shift)
    {
        if(shift < 32) return value<<shift;
        else return 0;
    }
    else return value;
}

static inline u32 lsr_shift_by_reg_no_carry(u32 value, u8 shift)
{
    if(shift)
    {
        if(shift < 32) return value>>shift;
        else return 0;
    }
    else return value;
    //return 0;
}

static inline u32 asr_shift_by_reg_no_carry(u32 value, u8 shift)
{
    if(shift)
    {
        if(shift < 32) return (u32)(((s32)value)>>shift);
        else return (u32)(((s32)value)>>31);
    }
    else return value;
    //else return (u32)(((s32)value)>>31);
}

static inline u32 ror_shift_by_reg_no_carry(u32 value, u8 shift)
{
/*
    if(shift)
    {
        shift &= 31;
        if(shift) return ( (value >> shift) | (value<<(32-shift)) );
        else return value;
    }
    else return ((CPU.CPSR&F_C)<<2) | (value >> 1);
*/
    shift &= 31;
#ifdef USE_ASM
    asm("ror %%cl,%%eax \n\t" : "=a" (value) : "a" (value), "c" (shift));
    return value;
#else
    if(shift) return ( (value >> shift) | (value<<(32-shift)) );
    else return value;
    //else return ((CPU.CPSR&F_C)<<2) | (value >> 1);
#endif

}

static inline u32 cpu_shift_by_reg_no_carry(u32 type, u32 value, u8 shift)
{
    switch(type&3)
    {
        case 0: return lsl_shift_by_reg_no_carry(value,shift);
        case 1: return lsr_shift_by_reg_no_carry(value,shift);
        case 2: return asr_shift_by_reg_no_carry(value,shift);
        case 3: return ror_shift_by_reg_no_carry(value,shift);
        default: return 0;
    }
    return 0;
}

//---------------------------------------------------------

static inline u32 cpu_shift_by_reg_no_carry_arm_ldr_str(u32 type, u32 value, u8 shift) // only in arm
{
    switch(type&3)
    {
        case 0: return lsl_shift_by_immed_no_carry(value,shift); // same effect ???
        case 1: return lsr_shift_by_immed_no_carry(value,shift); // shouldn't it be same as
        case 2: return asr_shift_by_immed_no_carry(value,shift); // cpu_shift_by_reg_no_carry() ??
        case 3: return ror_shift_by_immed_no_carry(value,shift);
        default: return 0;
    }
    return 0;
}

#endif //__GBA_SHIFTS__
