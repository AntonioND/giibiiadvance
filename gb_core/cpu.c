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
#include "../general_utils.h"
#include "../debug_utils.h"

#include "gameboy.h"
#include "cpu.h"
#include "debug.h"
#include "memory.h"
#include "interrupts.h"
#include "sound.h"
#include "general.h"
#include "sgb.h"
#include "ppu.h"
#include "serial.h"
#include "gb_main.h"
#include "camera.h"
#include "dma.h"

#include "../gui/win_gb_debugger.h"

// Single Speed
// 4194304 Hz
// 0.2384185791015625 us per clock
//
// Double Speed
// 8388608 Hz
// 0.11920928955078125 us per clock
//
// Screen refresh:
// 59.73 Hz

/* TODO

Sprite RAM Bug
--------------

There is a flaw in the GameBoy hardware that causes trash to be written to OAM
RAM if the following commands are used while their 16-bit content is in the
range of $FE00 to $FEFF:
  inc rr        dec rr          ;rr = bc,de, or hl
  ldi a,(hl)    ldd a,(hl)
  ldi (hl),a    ldd (hl),a
Only sprites 1 & 2 ($FE00 & $FE04) are not affected by these instructions.

*/

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

static int gb_last_residual_clocks;

extern const u8 gb_daa_table[256*8*2]; // In file daa_table.c

//----------------------------------------------------------------

static int gb_break_cpu_loop = 0;

inline void GB_CPUBreakLoop(void) // call when writing to a register that can generate an event!!!
{
    gb_break_cpu_loop = 1;
}

//----------------------------------------------------------------

//This is used for CPU, IRQ and GBC DMA

static int gb_cpu_clock_counter = 0;

inline void GB_CPUClockCounterReset(void)
{
    gb_cpu_clock_counter = 0;
}

inline int GB_CPUClockCounterGet(void)
{
    return gb_cpu_clock_counter;
}

inline void GB_CPUClockCounterAdd(int value)
{
    gb_cpu_clock_counter += value;
}

//----------------------------------------------------------------

static inline int min(int a, int b)
{
    return (a < b) ? a : b;
}

static inline int GB_ClocksForNextEvent(void)
{
    int clocks_to_next_event = GB_TimersGetClocksToNextEvent();
    clocks_to_next_event = min(clocks_to_next_event,GB_PPUGetClocksToNextEvent());
    clocks_to_next_event = min(clocks_to_next_event,GB_SerialGetClocksToNextEvent());
    clocks_to_next_event = min(clocks_to_next_event,GB_DMAGetClocksToNextEvent());
    clocks_to_next_event = min(clocks_to_next_event,GB_SoundGetClocksToNextEvent());
    clocks_to_next_event = min(clocks_to_next_event,GB_SoundGetClocksToNextEvent());
    //SGB?, CAMERA?

    //clocks_to_next_event should never be 0, this is enough:
    return (clocks_to_next_event | 4) & ~3; // align to 4 clocks for CPU HALT
}

static inline void GB_ClockCountersReset(void)
{
    GB_CPUClockCounterReset();
    GB_TimersClockCounterReset();
    GB_PPUClockCounterReset();
    GB_SerialClockCounterReset();
    GB_SoundClockCounterReset();
    GB_DMAClockCounterReset();
    //SGB?, CAMERA?
}

inline void GB_UpdateCounterToClocks(int reference_clocks)
{
    GB_TimersUpdateClocksClounterReference(reference_clocks);
    GB_PPUUpdateClocksClounterReference(reference_clocks);
    GB_SerialUpdateClocksClounterReference(reference_clocks);
    GB_SoundUpdateClocksClounterReference(reference_clocks);
    GB_DMAUpdateClocksClounterReference(reference_clocks);
    //SGB_Update(reference_clocks);
    //GB_CameraUpdate(reference_clocks);
}

//----------------------------------------------------------------

void GB_CPUInit(void)
{
    GB_ClockCountersReset();

    gb_break_cpu_loop = 0;
    gb_last_residual_clocks = 0;

    GameBoy.Emulator.CPUHalt = 0;
    GameBoy.Emulator.DoubleSpeed = 0;
    GameBoy.Emulator.halt_bug = 0;
    GameBoy.Emulator.cpu_change_speed_clocks = 0;

    //Registers
    if(GameBoy.Emulator.boot_rom_loaded == 0)
    {
        GameBoy.CPU.R16.SP = 0xFFFE;
        GameBoy.CPU.R16.PC = 0x0100;

        switch(GameBoy.Emulator.HardwareType)
        {
            case HW_GB: // Verified on hardware
                GameBoy.CPU.R16.AF = 0x01B0; GameBoy.CPU.R16.BC = 0x0013;
                GameBoy.CPU.R16.DE = 0x00D8; GameBoy.CPU.R16.HL = 0x014D;
                break;
            case HW_GBP: // Verified on hardware
                GameBoy.CPU.R16.AF = 0xFFB0; GameBoy.CPU.R16.BC = 0x0013;
                GameBoy.CPU.R16.DE = 0x00D8; GameBoy.CPU.R16.HL = 0x014D;
                break;
            case HW_SGB: // Obtained from boot ROM dump.
                GameBoy.CPU.R16.AF = 0x0100; GameBoy.CPU.R16.BC = 0x0014;
                GameBoy.CPU.R16.DE = 0x0000; GameBoy.CPU.R16.HL = 0xC060;
                break;
            case HW_SGB2: // Unknown, can't test. The only known value is that A is FF
                GameBoy.CPU.R16.AF = 0xFF00; GameBoy.CPU.R16.BC = 0x0014;
                GameBoy.CPU.R16.DE = 0x0000; GameBoy.CPU.R16.HL = 0xC060;
                break;
            case HW_GBC: // Verified on hardware
                if(GameBoy.Emulator.game_supports_gbc)
                {
                    GameBoy.CPU.R16.AF = 0x1180; GameBoy.CPU.R16.BC = 0x0000;
                    GameBoy.CPU.R16.DE = 0xFF56; GameBoy.CPU.R16.HL = 0x000D;
                }
                else
                {
                    GameBoy.CPU.R16.AF = 0x1100; GameBoy.CPU.R16.BC = 0x0000;
                    GameBoy.CPU.R16.DE = 0x0008; GameBoy.CPU.R16.HL = 0x007C;
                }
                break;
            case HW_GBA:
            case HW_GBA_SP: // Verified on hardware
                if(GameBoy.Emulator.game_supports_gbc)
                {
                    GameBoy.CPU.R16.AF = 0x1180; GameBoy.CPU.R16.BC = 0x0100;
                    GameBoy.CPU.R16.DE = 0xFF56; GameBoy.CPU.R16.HL = 0x000D;
                }
                else
                {
                    GameBoy.CPU.R16.AF = 0x1100; GameBoy.CPU.R16.BC = 0x0100;
                    GameBoy.CPU.R16.DE = 0x0008; GameBoy.CPU.R16.HL = 0x007C;
                }
                break;

            default:
                Debug_ErrorMsg("GB_CPUInit()\nUnknown hardware!");
                break;
        }
    }
    else
    {
        // No idea of the real initial values (except for the PC one, it must be 0x0000).
        GameBoy.CPU.R16.AF = 0x0000; GameBoy.CPU.R16.BC = 0x0000;
        GameBoy.CPU.R16.DE = 0x0000; GameBoy.CPU.R16.HL = 0x0000;
        GameBoy.CPU.R16.PC = 0x0000; GameBoy.CPU.R16.SP = 0x0000;
    }

    if(GameBoy.Emulator.CGBEnabled == 1)
        GameBoy.Memory.IO_Ports[KEY1_REG-0xFF00] = 0x7E;
}

void GB_CPUEnd(void)
{
    //nothing here
}

//----------------------------------------------------------------

int gb_break_execution = 0;

inline void _gb_break_to_debugger(void)
{
    gb_break_execution = 1;
}

//----------------------------------------------------------------

//LD r16,nnnn - 3
#define gb_ld_r16_nnnn(reg_hi,reg_low) { \
    GB_CPUClockCounterAdd(4); \
    reg_low = GB_MemRead8(cpu->R16.PC++); \
    GB_CPUClockCounterAdd(4); \
    reg_hi = GB_MemRead8(cpu->R16.PC++); \
    GB_CPUClockCounterAdd(4); \
}

//LD r8,nn - 2
#define gb_ld_r8_nn(reg8) { \
    GB_CPUClockCounterAdd(4); \
    reg8 = GB_MemRead8(cpu->R16.PC++); \
    GB_CPUClockCounterAdd(4); \
}

//LD [r16],r8 - 2
#define gb_ld_ptr_r16_r8(reg16,r8) { \
    GB_CPUClockCounterAdd(4); \
    GB_MemWrite8(reg16,r8); \
    GB_CPUClockCounterAdd(4); \
}

//LD r8,[r16] - 2
#define gb_ld_r8_ptr_r16(r8,reg16) { \
    GB_CPUClockCounterAdd(4); \
    r8 = GB_MemRead8(reg16); \
    GB_CPUClockCounterAdd(4); \
}

//INC r16 - 2
#define gb_inc_r16(reg16) { \
    reg16 = (reg16+1) & 0xFFFF; \
    GB_CPUClockCounterAdd(8); \
}

//DEC r16 - 2
#define gb_dec_r16(reg16) { \
    reg16 = (reg16-1) & 0xFFFF; \
    GB_CPUClockCounterAdd(8); \
}

//INC r8 - 1
#define gb_inc_r8(reg8) { \
    cpu->R16.AF &= ~F_SUBTRACT; \
    cpu->F.H = ( (reg8 & 0xF) == 0xF); \
    reg8 ++; \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//DEC r8 - 1
#define gb_dec_r8(reg8) { \
    cpu->R16.AF |= F_SUBTRACT; \
    cpu->F.H = ( (reg8 & 0xF) == 0x0); \
    reg8 --; \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//ADD HL,r16 - 2
#define gb_add_hl_r16(reg16) { \
    cpu->R16.AF &= ~F_SUBTRACT; \
    u32 temp = cpu->R16.HL + reg16; \
    cpu->F.C = (temp > 0xFFFF); \
    cpu->F.H = ( ( (cpu->R16.HL & 0x0FFF) + (reg16 & 0x0FFF) ) > 0x0FFF ); \
    cpu->R16.HL = temp & 0xFFFF; \
    GB_CPUClockCounterAdd(8); \
}

//ADD A,r8 - 1
#define gb_add_a_r8(reg8) { \
    cpu->R16.AF &= ~F_SUBTRACT; \
    u32 temp = cpu->R8.A; \
    cpu->F.H = ( (temp & 0xF) + ((u32)reg8 & 0xF) ) > 0xF; \
    cpu->R8.A += reg8; \
    cpu->F.Z = (cpu->R8.A == 0); \
    cpu->F.C = (temp > cpu->R8.A); \
    GB_CPUClockCounterAdd(4); \
}

//ADC A,r8 - 1
#define gb_adc_a_r8(reg8) { \
    cpu->R16.AF &= ~F_SUBTRACT; \
    u32 temp = cpu->R8.A + reg8 + cpu->F.C; \
    cpu->F.H = ( ((cpu->R8.A & 0xF) + (reg8 & 0xF) ) + cpu->F.C) > 0xF; \
    cpu->F.C = (temp > 0xFF); \
    temp &= 0xFF; \
    cpu->R8.A = temp; \
    cpu->F.Z = (temp == 0); \
    GB_CPUClockCounterAdd(4); \
}

//SUB A,r8 - 1
#define gb_sub_a_r8(reg8) { \
    cpu->R8.F = F_SUBTRACT; \
    cpu->F.H = (cpu->R8.A & 0xF) < (reg8 & 0xF); \
    cpu->F.C = (u32)cpu->R8.A < (u32)reg8; \
    cpu->R8.A -= reg8; \
    cpu->F.Z = (cpu->R8.A == 0); \
    GB_CPUClockCounterAdd(4); \
}

//SBC A,r8 - 1
#define gb_sbc_a_r8(reg8) { \
    u32 temp = cpu->R8.A - reg8 - ((cpu->R8.F&F_CARRY)?1:0); \
    cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT; \
    cpu->F.H = ( (cpu->R8.A^reg8^temp) & 0x10 ) != 0 ; \
    cpu->R8.A = temp; \
    GB_CPUClockCounterAdd(4); \
}

//AND A,r8 - 1
#define gb_and_a_r8(reg8) { \
    cpu->R16.AF |= F_HALFCARRY; \
    cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY); \
    cpu->R8.A &= reg8; \
    cpu->F.Z = (cpu->R8.A == 0); \
    GB_CPUClockCounterAdd(4); \
}

//XOR A,r8 - 1
#define gb_xor_a_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY); \
    cpu->R8.A ^= reg8; \
    cpu->F.Z = (cpu->R8.A == 0); \
    GB_CPUClockCounterAdd(4); \
}

//OR A,r8 - 1
#define gb_or_a_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY); \
    cpu->R8.A |= reg8; \
    cpu->F.Z = (cpu->R8.A == 0); \
    GB_CPUClockCounterAdd(4); \
}

//CP A,r8 - 1
#define gb_cp_a_r8(reg8) { \
    cpu->R16.AF |= F_SUBTRACT; \
    cpu->F.H = (cpu->R8.A & 0xF) < (reg8 & 0xF); \
    cpu->F.C = (u32)cpu->R8.A < (u32)reg8; \
    cpu->F.Z = (cpu->R8.A == reg8); \
    GB_CPUClockCounterAdd(4); \
}

//RST nnnn - 4
#define gb_rst_nnnn(addr) { \
    GB_CPUClockCounterAdd(4); \
    cpu->R16.SP --; \
    cpu->R16.SP &= 0xFFFF; \
    GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH); \
    GB_CPUClockCounterAdd(4); \
    cpu->R16.SP --; \
    cpu->R16.SP &= 0xFFFF; \
    GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL); \
    GB_CPUClockCounterAdd(4); \
    cpu->R16.PC = addr; \
    GB_CPUClockCounterAdd(4); \
}

//PUSH r16 - 4
#define gb_push_r16(reg_hi,reg_low) { \
    GB_CPUClockCounterAdd(4); \
    cpu->R16.SP--; \
    cpu->R16.SP &= 0xFFFF; \
    GB_MemWrite8(cpu->R16.SP,reg_hi); \
    GB_CPUClockCounterAdd(4); \
    cpu->R16.SP--; \
    cpu->R16.SP &= 0xFFFF; \
    GB_MemWrite8(cpu->R16.SP,reg_low); \
    GB_CPUClockCounterAdd(8); \
}

//POP r16 - 4
#define gb_pop_r16(reg_hi,reg_low) { \
    GB_CPUClockCounterAdd(4); \
    reg_low = GB_MemRead8(cpu->R16.SP++); \
    cpu->R16.SP &= 0xFFFF; \
    GB_CPUClockCounterAdd(4); \
    reg_hi = GB_MemRead8(cpu->R16.SP++); \
    cpu->R16.SP &= 0xFFFF; \
    GB_CPUClockCounterAdd(4); \
}

//CALL cond,nnnn - 6/3
#define gb_call_cond_nnnn(cond) { \
    if(cond) \
    { \
        GB_CPUClockCounterAdd(4); \
        u32 temp = GB_MemRead8(cpu->R16.PC++); \
        GB_CPUClockCounterAdd(4); \
        temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8; \
        GB_CPUClockCounterAdd(4); \
        cpu->R16.SP --; \
        cpu->R16.SP &= 0xFFFF; \
        GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH); \
        GB_CPUClockCounterAdd(4); \
        cpu->R16.SP --; \
        cpu->R16.SP &= 0xFFFF; \
        GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL); \
        GB_CPUClockCounterAdd(4); \
        cpu->R16.PC = temp; \
        GB_CPUClockCounterAdd(4); \
    } \
    else \
    { \
        cpu->R16.PC += 2; \
        cpu->R16.PC &= 0xFFFF; \
        GB_CPUClockCounterAdd(12); \
    } \
}

//RET cond - 5/2
#define gb_ret_cond(cond) { \
    if(cond) \
    { \
        GB_CPUClockCounterAdd(4); \
        u32 temp = GB_MemRead8(cpu->R16.SP++); \
        cpu->R16.SP &= 0xFFFF; \
        GB_CPUClockCounterAdd(4); \
        temp |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8; \
        cpu->R16.SP &= 0xFFFF; \
        GB_CPUClockCounterAdd(4); \
        cpu->R16.PC = temp; \
        GB_CPUClockCounterAdd(8); \
    } \
    else \
    { \
        GB_CPUClockCounterAdd(8); \
    } \
}

//JP cond,nnnn - 4/3
#define gb_jp_cond_nnnn(cond) { \
    if(cond) \
    { \
        GB_CPUClockCounterAdd(4); \
        u32 temp = GB_MemRead8(cpu->R16.PC++); \
        GB_CPUClockCounterAdd(4); \
        temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8; \
        GB_CPUClockCounterAdd(4); \
        cpu->R16.PC = temp; \
        GB_CPUClockCounterAdd(4); \
    } \
    else \
    { \
        cpu->R16.PC += 2; \
        cpu->R16.PC &= 0xFFFF; \
        GB_CPUClockCounterAdd(12); \
    } \
}

//JR cond,nn - 3/2
#define gb_jr_cond_nn(cond) { \
    if(cond) \
    { \
        GB_CPUClockCounterAdd(4); \
        u32 temp = GB_MemRead8(cpu->R16.PC++); \
        cpu->R16.PC = (cpu->R16.PC + (s8)temp) & 0xFFFF; \
        GB_CPUClockCounterAdd(8); \
    } \
    else \
    { \
        cpu->R16.PC ++; \
        GB_CPUClockCounterAdd(8); \
    } \
}

//RLC r8 - 2
#define gb_rlc_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY); \
    cpu->F.C = (reg8 & 0x80) != 0; \
    reg8 = (reg8 << 1) | cpu->F.C; \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//RRC r8 - 2
#define gb_rrc_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY); \
    cpu->F.C = (reg8 & 0x01) != 0; \
    reg8 = (reg8 >> 1) | (cpu->F.C << 7); \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//RL r8 - 2
#define gb_rl_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY); \
    u32 temp = cpu->F.C; \
    cpu->F.C = (reg8 & 0x80) != 0; \
    reg8 = (reg8 << 1) | temp; \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//RR r8 - 2
#define gb_rr_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY); \
    u32 temp = cpu->F.C; \
    cpu->F.C = (reg8 & 0x01) != 0; \
    reg8 = (reg8 >> 1) | (temp << 7); \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//SLA r8 - 2
#define gb_sla_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY); \
    cpu->F.C = (reg8 & 0x80) != 0; \
    reg8 = reg8 << 1; \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//SRA r8 - 2
#define gb_sra_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY); \
    cpu->F.C = (reg8 & 0x01) != 0; \
    reg8 = (reg8 & 0x80) | (reg8 >> 1); \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//SWAP r8 - 2
#define gb_swap_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY); \
    reg8 = ((reg8 >> 4) | (reg8 << 4)); \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//SRL r8 - 2
#define gb_srl_r8(reg8) { \
    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY); \
    cpu->F.C = (reg8 & 0x01) != 0; \
    reg8 = reg8 >> 1; \
    cpu->F.Z = (reg8 == 0); \
    GB_CPUClockCounterAdd(4); \
}

//BIT n,r8 - 2
#define gb_bit_n_r8(bitn,reg8) { \
    cpu->R16.AF &= ~F_SUBTRACT; \
    cpu->R16.AF |= F_HALFCARRY; \
    cpu->F.Z = (reg8 & (1<<bitn)) == 0; \
    GB_CPUClockCounterAdd(4); \
}

//BIT n,[HL] - 3
#define gb_bit_n_ptr_hl(bitn) { \
    GB_CPUClockCounterAdd(4); \
    cpu->R16.AF &= ~F_SUBTRACT; \
    cpu->R16.AF |= F_HALFCARRY; \
    cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<bitn)) == 0; \
    GB_CPUClockCounterAdd(4); \
}

//RES n,r8 - 2
#define gb_res_n_r8(bitn,reg8) { \
    reg8 &= ~(1 << bitn); \
    GB_CPUClockCounterAdd(4); \
}

//RES n,[HL] - 4
#define gb_res_n_ptr_hl(bitn) { \
    GB_CPUClockCounterAdd(4); \
    u32 temp = GB_MemRead8(cpu->R16.HL); \
    GB_CPUClockCounterAdd(4); \
    GB_MemWrite8(cpu->R16.HL, temp & (~(1 << bitn))); \
    GB_CPUClockCounterAdd(4); \
}

//SET n,r8 - 2
#define gb_set_n_r8(bitn,reg8) { \
    reg8 |= (1 << bitn); \
    GB_CPUClockCounterAdd(4); \
}

//SET n,[HL] - 4
#define gb_set_n_ptr_hl(bitn) { \
    GB_CPUClockCounterAdd(4); \
    u32 temp = GB_MemRead8(cpu->R16.HL); \
    GB_CPUClockCounterAdd(4); \
    GB_MemWrite8(cpu->R16.HL, temp | (1 << bitn)); \
    GB_CPUClockCounterAdd(4); \
}

//Undefined opcode - *
#define gb_undefined_opcode(op) { \
    GB_CPUClockCounterAdd(4); \
    cpu->R16.PC--; \
    _gb_break_to_debugger(); \
    Debug_ErrorMsgArg("Undefined opcode. 0x02X\nPC: %04X\nROM: %d", \
                op,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom); \
}

//----------------------------------------------------------------

static int GB_CPUExecute(int clocks) // returns executed clocks
{
    _GB_CPU_ * cpu = &GameBoy.CPU;
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int previous_clocks_counter = GB_CPUClockCounterGet();
    int finish_clocks = GB_CPUClockCounterGet() + clocks; // if nothing interesting happens before, stop here

    while(GB_CPUClockCounterGet() < finish_clocks)
    {
        if(GB_DebugCPUIsBreakpoint(cpu->R16.PC))
        {
            _gb_break_to_debugger();
            Win_GBDisassemblerSetFocus();
            break;
        }

        if(mem->interrupts_enable_count) // EI interrupt enable delay
        {
            mem->interrupts_enable_count = 0;
            mem->InterruptMasterEnable = 1;
            GB_CPUBreakLoop(); // don't break right now, break after this instruction
        }

        u8 opcode = (u8)GB_MemRead8(cpu->R16.PC++);
        cpu->R16.PC &= 0xFFFF;

        if(GameBoy.Emulator.halt_bug)
        {
            GameBoy.Emulator.halt_bug = 0;
            cpu->R16.PC--;
            cpu->R16.PC &= 0xFFFF;
        }

        switch(opcode)
        {
            case 0x00: GB_CPUClockCounterAdd(4); break; //NOP - 1
            case 0x01: gb_ld_r16_nnnn(cpu->R8.B,cpu->R8.C); break; //LD BC,nnnn - 3
            case 0x02: gb_ld_ptr_r16_r8(cpu->R16.BC,cpu->R8.A); break; //LD [BC],A - 2
            case 0x03: gb_inc_r16(cpu->R16.BC); break; //INC BC - 2
            case 0x04: gb_inc_r8(cpu->R8.B); break; //INC B - 1
            case 0x05: gb_dec_r8(cpu->R8.B); break; //DEC B - 1
            case 0x06: gb_ld_r8_nn(cpu->R8.B); break; //LD B,n - 2
            case 0x07: //RLCA - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
                cpu->F.C = (cpu->R8.A & 0x80) != 0;
                cpu->R8.A = (cpu->R8.A << 1) | cpu->F.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x08: //LD [nnnn],SP - 5
            {
                GB_CPUClockCounterAdd(4);
                u16 temp = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                temp |= ( ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8);
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(temp++,cpu->R8.SPL);
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(temp,cpu->R8.SPH);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x09: gb_add_hl_r16(cpu->R16.BC); break; //ADD HL,BC - 2
            case 0x0A: gb_ld_r8_ptr_r16(cpu->R8.A,cpu->R16.BC); break; //LD A,[BC] - 2
            case 0x0B: gb_dec_r16(cpu->R16.BC); break; //DEC BC - 2
            case 0x0C: gb_inc_r8(cpu->R8.C); break; //INC C - 1
            case 0x0D: gb_dec_r8(cpu->R8.C); break; //DEC C - 1
            case 0x0E: gb_ld_r8_nn(cpu->R8.C);  break; //LD C,nn - 2
            case 0x0F: //RRCA - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
                cpu->F.C = (cpu->R8.A & 0x01) != 0;
                cpu->R8.A = (cpu->R8.A >> 1) | (cpu->F.C << 7);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x10: //STOP - 1*
                GB_CPUClockCounterAdd(4);
                if(GB_MemRead8(cpu->R16.PC++) != 0)
                    Debug_DebugMsgArg("Corrupted stop.\nPC: %04X\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                GB_CPUClockCounterAdd(4);

                if(GameBoy.Emulator.CGBEnabled == 0)
                {
                    GameBoy.Emulator.CPUHalt = 2;
                }
                else //Switch to double speed mode (CGB)
                {
                    if(mem->IO_Ports[KEY1_REG-0xFF00]&1)
                    {
                        GameBoy.Emulator.cpu_change_speed_clocks = 128*1024 - 84;
                        // same clocks when switching to double and single speed modes
                        // The 84 clocks subtracted could be because of glitching during the speed switch.
                        GameBoy.Emulator.DoubleSpeed ^= 1;
                        mem->IO_Ports[KEY1_REG-0xFF00] = GameBoy.Emulator.DoubleSpeed<<7;
                    }
                    else
                    {
                        GameBoy.Emulator.CPUHalt = 2;
                    }
                }
                GB_CPUBreakLoop();
                break;
            case 0x11: gb_ld_r16_nnnn(cpu->R8.D,cpu->R8.E); break; //LD DE,nnnn - 3
            case 0x12: gb_ld_ptr_r16_r8(cpu->R16.DE,cpu->R8.A); break; //LD [DE],A - 2
            case 0x13: gb_inc_r16(cpu->R16.DE); break; //INC DE - 2
            case 0x14: gb_inc_r8(cpu->R8.D); break; //INC D - 1
            case 0x15: gb_dec_r8(cpu->R8.D); break; //DEC D - 1
            case 0x16: gb_ld_r8_nn(cpu->R8.D); break; //LD D,nn - 2
            case 0x17: //RLA - 1
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
                u32 temp = cpu->F.C; //old carry flag
                cpu->F.C = (cpu->R8.A & 0x80) != 0;
                cpu->R8.A = (cpu->R8.A << 1) | temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x18: //JR nn - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = (cpu->R16.PC + (s8)temp) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x19: gb_add_hl_r16(cpu->R16.DE); break; //ADD HL,DE - 2
            case 0x1A: gb_ld_r8_ptr_r16(cpu->R8.A,cpu->R16.DE); break; //LD A,[DE] - 2
            case 0x1B: gb_dec_r16(cpu->R16.DE); break; //DEC DE - 2
            case 0x1C: gb_inc_r8(cpu->R8.E); break; //INC E - 1
            case 0x1D: gb_dec_r8(cpu->R8.E); break; //DEC E - 1
            case 0x1E: gb_ld_r8_nn(cpu->R8.E); break; //LD E,nn - 2
            case 0x1F: //RRA - 1
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
                u32 temp = cpu->F.C; //old carry flag
                cpu->F.C = cpu->R8.A & 0x01;
                cpu->R8.A = (cpu->R8.A >> 1) | (temp << 7);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x20: gb_jr_cond_nn(cpu->F.Z == 0); break; //JR NZ,nn - 3/2
            case 0x21: gb_ld_r16_nnnn(cpu->R8.H,cpu->R8.L); break; //LD HL,nnnn - 3
            case 0x22: //LD [HL+],A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.A);
                cpu->R16.HL = (cpu->R16.HL+1) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x23: gb_inc_r16(cpu->R16.HL); break; //INC HL - 2
            case 0x24: gb_inc_r8(cpu->R8.H); break; //INC H - 1
            case 0x25: gb_dec_r8(cpu->R8.H); break; //DEC H - 1
            case 0x26: gb_ld_r8_nn(cpu->R8.H); break; //LD H,nn - 2
            case 0x27: //DAA - 1
            {
                u32 temp = ( ((u32)cpu->R8.A)<<(3+1) ) | ((((u32)cpu->R8.F>>4)&7)<<1);
                cpu->R8.A = gb_daa_table[temp];
                cpu->R8.F = gb_daa_table[temp+1];
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x28: gb_jr_cond_nn(cpu->F.Z); break; //JR Z,nn - 3/2
            case 0x29: //ADD HL,HL - 2
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.C = (cpu->R16.HL & 0x8000) != 0;
                cpu->F.H = (cpu->R16.HL & 0x0800) != 0;
                cpu->R16.HL = (cpu->R16.HL << 1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x2A: //LD A,[HL+] - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.HL);
                cpu->R16.HL = (cpu->R16.HL+1) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x2B: gb_dec_r16(cpu->R16.HL); break; //DEC HL - 2
            case 0x2C: gb_inc_r8(cpu->R8.L); break; //INC L - 1
            case 0x2D: gb_dec_r8(cpu->R8.L); break; //DEC L - 1
            case 0x2E: gb_ld_r8_nn(cpu->R8.L); break; //LD L,nn - 2
            case 0x2F: //CPL - 1
                cpu->R16.AF |= (F_SUBTRACT|F_HALFCARRY);
                cpu->R8.A = ~cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x30: gb_jr_cond_nn(cpu->F.C == 0); break; //JR NC,nn - 3/2
            case 0x31: gb_ld_r16_nnnn(cpu->R8.SPH,cpu->R8.SPL); break; //LD SP,nnnn - 3
            case 0x32: //LD [HL-],A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.A);
                cpu->R16.HL = (cpu->R16.HL-1) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x33: gb_inc_r16(cpu->R16.SP); break; //INC SP - 2
            case 0x34: //INC [HL] - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = ( (temp & 0xF) == 0xF);
                temp = (temp + 1) & 0xFF;
                cpu->F.Z = (temp == 0);
                GB_MemWrite8(cpu->R16.HL,temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x35: //DEC [HL] - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (temp & 0xF) == 0x0);
                temp = (temp - 1) & 0xFF;
                cpu->F.Z = (temp == 0);
                GB_MemWrite8(cpu->R16.HL,temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x36: //LD [HL],n - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x37: //SCF - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                cpu->R16.AF |= F_CARRY;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x38: gb_jr_cond_nn(cpu->F.C); break; //JR C,nn - 3/2
            case 0x39: gb_add_hl_r16(cpu->R16.SP); break; //ADD HL,SP - 2
            case 0x3A: //LD A,[HL-] - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.HL);
                cpu->R16.HL = (cpu->R16.HL-1) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x3B: gb_dec_r16(cpu->R16.SP); break; //DEC SP - 2
            case 0x3C: gb_inc_r8(cpu->R8.A); break; //INC A - 1
            case 0x3D: gb_dec_r8(cpu->R8.A); break; //DEC A - 1
            case 0x3E: gb_ld_r8_nn(cpu->R8.A); break; //LD A,n - 2
            case 0x3F: //CCF - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                cpu->F.C = !cpu->F.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x40: GB_CPUClockCounterAdd(4); break; //LD B,B - 1
            case 0x41: cpu->R8.B = cpu->R8.C; GB_CPUClockCounterAdd(4); break; //LD B,C - 1
            case 0x42: cpu->R8.B = cpu->R8.D; GB_CPUClockCounterAdd(4); break; //LD B,D - 1
            case 0x43: cpu->R8.B = cpu->R8.E; GB_CPUClockCounterAdd(4); break; //LD B,E - 1
            case 0x44: cpu->R8.B = cpu->R8.H; GB_CPUClockCounterAdd(4); break; //LD B,H - 1
            case 0x45: cpu->R8.B = cpu->R8.L; GB_CPUClockCounterAdd(4); break; //LD B,L - 1
            case 0x46: gb_ld_r8_ptr_r16(cpu->R8.B,cpu->R16.HL); break; //LD B,[HL] - 2
            case 0x47: cpu->R8.B = cpu->R8.A; GB_CPUClockCounterAdd(4); break; //LD B,A - 1
            case 0x48: cpu->R8.C = cpu->R8.B; GB_CPUClockCounterAdd(4); break; //LD C,B - 1
            case 0x49: GB_CPUClockCounterAdd(4); break; //LD C,C - 1
            case 0x4A: cpu->R8.C = cpu->R8.D; GB_CPUClockCounterAdd(4); break; //LD C,D - 1
            case 0x4B: cpu->R8.C = cpu->R8.E; GB_CPUClockCounterAdd(4); break; //LD C,E - 1
            case 0x4C: cpu->R8.C = cpu->R8.H; GB_CPUClockCounterAdd(4); break; //LD C,H - 1
            case 0x4D: cpu->R8.C = cpu->R8.L; GB_CPUClockCounterAdd(4); break; //LD C,L - 1
            case 0x4E: gb_ld_r8_ptr_r16(cpu->R8.C,cpu->R16.HL); break; //LD C,[HL] - 2
            case 0x4F: cpu->R8.C = cpu->R8.A; GB_CPUClockCounterAdd(4); break; //LD C,A - 1
            case 0x50: cpu->R8.D = cpu->R8.B; GB_CPUClockCounterAdd(4); break; //LD D,B - 1
            case 0x51: cpu->R8.D = cpu->R8.C; GB_CPUClockCounterAdd(4); break; //LD D,C - 1
            case 0x52: GB_CPUClockCounterAdd(4); break; //LD D,D - 1
            case 0x53: cpu->R8.D = cpu->R8.E; GB_CPUClockCounterAdd(4); break; //LD D,E - 1
            case 0x54: cpu->R8.D = cpu->R8.H; GB_CPUClockCounterAdd(4); break; //LD D,H - 1
            case 0x55: cpu->R8.D = cpu->R8.L; GB_CPUClockCounterAdd(4); break; //LD D,L - 1
            case 0x56: gb_ld_r8_ptr_r16(cpu->R8.D,cpu->R16.HL); break; //LD D,[HL] - 2
            case 0x57: cpu->R8.D = cpu->R8.A; GB_CPUClockCounterAdd(4); break; //LD D,A - 1
            case 0x58: cpu->R8.E = cpu->R8.B; GB_CPUClockCounterAdd(4); break; //LD E,B - 1
            case 0x59: cpu->R8.E = cpu->R8.C; GB_CPUClockCounterAdd(4); break; //LD E,C - 1
            case 0x5A: cpu->R8.E = cpu->R8.D; GB_CPUClockCounterAdd(4); break; //LD E,D - 1
            case 0x5B: GB_CPUClockCounterAdd(4); break; //LD E,E - 1
            case 0x5C: cpu->R8.E = cpu->R8.H; GB_CPUClockCounterAdd(4); break; //LD E,H - 1
            case 0x5D: cpu->R8.E = cpu->R8.L; GB_CPUClockCounterAdd(4); break; //LD E,L - 1
            case 0x5E: gb_ld_r8_ptr_r16(cpu->R8.E,cpu->R16.HL); break; //LD E,[HL] - 2
            case 0x5F: cpu->R8.E = cpu->R8.A; GB_CPUClockCounterAdd(4); break; //LD E,A - 1
            case 0x60: cpu->R8.H = cpu->R8.B; GB_CPUClockCounterAdd(4); break; //LD H,B - 1
            case 0x61: cpu->R8.H = cpu->R8.C; GB_CPUClockCounterAdd(4); break; //LD H,C - 1
            case 0x62: cpu->R8.H = cpu->R8.D; GB_CPUClockCounterAdd(4); break; //LD H,D - 1
            case 0x63: cpu->R8.H = cpu->R8.E; GB_CPUClockCounterAdd(4); break; //LD H,E - 1
            case 0x64: GB_CPUClockCounterAdd(4); break; //LD H,H - 1
            case 0x65: cpu->R8.H = cpu->R8.L; GB_CPUClockCounterAdd(4); break; //LD H,L - 1
            case 0x66: gb_ld_r8_ptr_r16(cpu->R8.H,cpu->R16.HL); break; //LD H,[HL] - 2
            case 0x67: cpu->R8.H = cpu->R8.A; GB_CPUClockCounterAdd(4); break; //LD H,A - 1
            case 0x68: cpu->R8.L = cpu->R8.B; GB_CPUClockCounterAdd(4); break; //LD L,B - 1
            case 0x69: cpu->R8.L = cpu->R8.C; GB_CPUClockCounterAdd(4); break; //LD L,C - 1
            case 0x6A: cpu->R8.L = cpu->R8.D; GB_CPUClockCounterAdd(4); break; //LD L,D - 1
            case 0x6B: cpu->R8.L = cpu->R8.E; GB_CPUClockCounterAdd(4); break; //LD L,E - 1
            case 0x6C: cpu->R8.L = cpu->R8.H; GB_CPUClockCounterAdd(4); break; //LD L,H - 1
            case 0x6D: GB_CPUClockCounterAdd(4); break; //LD L,L - 1
            case 0x6E: gb_ld_r8_ptr_r16(cpu->R8.L,cpu->R16.HL); break; //LD L,[HL] - 2
            case 0x6F: cpu->R8.L = cpu->R8.A; GB_CPUClockCounterAdd(4); break; //LD L,A - 1
            case 0x70: gb_ld_ptr_r16_r8(cpu->R16.HL,cpu->R8.B); break; //LD [HL],B - 2
            case 0x71: gb_ld_ptr_r16_r8(cpu->R16.HL,cpu->R8.C); break; //LD [HL],C - 2
            case 0x72: gb_ld_ptr_r16_r8(cpu->R16.HL,cpu->R8.D); break; //LD [HL],D - 2
            case 0x73: gb_ld_ptr_r16_r8(cpu->R16.HL,cpu->R8.E); break; //LD [HL],E - 2
            case 0x74: gb_ld_ptr_r16_r8(cpu->R16.HL,cpu->R8.H); break; //LD [HL],H - 2
            case 0x75: gb_ld_ptr_r16_r8(cpu->R16.HL,cpu->R8.L); break; //LD [HL],L - 2
            case 0x76: //HALT - 1*
                GB_CPUClockCounterAdd(4);
                if(GameBoy.Memory.InterruptMasterEnable == 1)
                {
                    GameBoy.Emulator.CPUHalt = 1;
                }
                else
                {
                    if(mem->IO_Ports[IF_REG-0xFF00] & mem->HighRAM[IE_REG-0xFF80] & 0x1F)
                    {
                        //The halt bug seems to happen even in GBC, not only DMG
                        GameBoy.Emulator.halt_bug = 1;
                    }
                    else
                    {
                        GameBoy.Emulator.CPUHalt = 1;
                    }
                }
                GB_CPUBreakLoop();
                break;
            case 0x77: gb_ld_ptr_r16_r8(cpu->R16.HL,cpu->R8.A); break; //LD [HL],A - 2
            case 0x78: cpu->R8.A = cpu->R8.B; GB_CPUClockCounterAdd(4); break; //LD A,B - 1
            case 0x79: cpu->R8.A = cpu->R8.C; GB_CPUClockCounterAdd(4); break; //LD A,C - 1
            case 0x7A: cpu->R8.A = cpu->R8.D; GB_CPUClockCounterAdd(4); break; //LD A,D - 1
            case 0x7B: cpu->R8.A = cpu->R8.E; GB_CPUClockCounterAdd(4); break; //LD A,E - 1
            case 0x7C: cpu->R8.A = cpu->R8.H; GB_CPUClockCounterAdd(4); break; //LD A,H - 1
            case 0x7D: cpu->R8.A = cpu->R8.L; GB_CPUClockCounterAdd(4); break; //LD A,L - 1
            case 0x7E: gb_ld_r8_ptr_r16(cpu->R8.A,cpu->R16.HL); break; //LD A,[HL] - 2
            case 0x7F: GB_CPUClockCounterAdd(4); break; //LD A,A - 1
            case 0x80: gb_add_a_r8(cpu->R8.B); break; //ADD A,B - 1
            case 0x81: gb_add_a_r8(cpu->R8.C); break; //ADD A,C - 1
            case 0x82: gb_add_a_r8(cpu->R8.D); break; //ADD A,D - 1
            case 0x83: gb_add_a_r8(cpu->R8.E); break; //ADD A,E - 1
            case 0x84: gb_add_a_r8(cpu->R8.H); break; //ADD A,H - 1
            case 0x85: gb_add_a_r8(cpu->R8.L); break; //ADD A,L - 1
            case 0x86: //ADD A,[HL] - 2
            {
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A;
                u32 temp2 = GB_MemRead8(cpu->R16.HL);
                cpu->F.H = ( (temp & 0xF) + (temp2 & 0xF) ) > 0xF;
                cpu->R8.A += temp2;
                cpu->F.Z = (cpu->R8.A == 0);
                cpu->F.C = (temp > cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x87: //ADD A,A - 1
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & BIT(3)) != 0;
                cpu->F.C = (cpu->R8.A & BIT(7)) != 0;
                cpu->R8.A += cpu->R8.A;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x88: gb_adc_a_r8(cpu->R8.B); break; //ADC A,B - 1
            case 0x89: gb_adc_a_r8(cpu->R8.C); break; //ADC A,C - 1
            case 0x8A: gb_adc_a_r8(cpu->R8.D); break; //ADC A,D - 1
            case 0x8B: gb_adc_a_r8(cpu->R8.E); break; //ADC A,E - 1
            case 0x8C: gb_adc_a_r8(cpu->R8.H); break; //ADC A,H - 1
            case 0x8D: gb_adc_a_r8(cpu->R8.L); break; //ADC A,L - 1
            case 0x8E: //ADC A,[HL] - 2
            {
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = GB_MemRead8(cpu->R16.HL);
                u32 temp2 = cpu->R8.A + temp + cpu->F.C;
                cpu->F.H = ( ((cpu->R8.A & 0xF) + (temp & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp2 > 0xFF);
                temp2 &= 0xFF;
                cpu->R8.A = temp2;
                cpu->F.Z = (temp2 == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x8F: //ADC A,A - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = ( ((u32)cpu->R8.A) << 1 ) + cpu->F.C;
                //Carry flag not needed to test this
                cpu->F.H = (cpu->R8.A & 0x08) != 0;
                cpu->F.C = (temp > 0xFF);
                temp &= 0xFF;
                cpu->R8.A = temp;
                cpu->F.Z = (temp == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x90: gb_sub_a_r8(cpu->R8.B); break; //SUB A,B - 1
            case 0x91: gb_sub_a_r8(cpu->R8.C); break; //SUB A,C - 1
            case 0x92: gb_sub_a_r8(cpu->R8.D); break; //SUB A,D - 1
            case 0x93: gb_sub_a_r8(cpu->R8.E); break; //SUB A,E - 1
            case 0x94: gb_sub_a_r8(cpu->R8.H); break; //SUB A,H - 1
            case 0x95: gb_sub_a_r8(cpu->R8.L); break; //SUB A,L - 1
            case 0x96: //SUB A,[HL] - 2
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.HL);
                cpu->R8.F = F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (temp & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)temp;
                cpu->R8.A -= temp;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x97: //SUB A,A - 1
                cpu->R8.F = F_SUBTRACT|F_ZERO;
                cpu->R8.A = 0;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x98: gb_sbc_a_r8(cpu->R8.B); break; //SBC A,B - 1
            case 0x99: gb_sbc_a_r8(cpu->R8.C); break; //SBC A,C - 1
            case 0x9A: gb_sbc_a_r8(cpu->R8.D); break; //SBC A,D - 1
            case 0x9B: gb_sbc_a_r8(cpu->R8.E); break; //SBC A,E - 1
            case 0x9C: gb_sbc_a_r8(cpu->R8.H); break; //SBC A,H - 1
            case 0x9D: gb_sbc_a_r8(cpu->R8.L); break; //SBC A,L - 1
            case 0x9E: //SBC A,[HL] - 2
            {
                GB_CPUClockCounterAdd(4);
                u32 temp2 = GB_MemRead8(cpu->R16.HL);
                u32 temp = cpu->R8.A - temp2 - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^temp2^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x9F: //SBC A,A - 1
                cpu->R16.AF = (cpu->R8.F&F_CARRY) ?
                        ( (0xFF<<8)|F_CARRY|F_HALFCARRY|F_SUBTRACT ) : (F_ZERO|F_SUBTRACT) ;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA0: gb_and_a_r8(cpu->R8.B); break; //AND A,B - 1
            case 0xA1: gb_and_a_r8(cpu->R8.C); break; //AND A,C - 1
            case 0xA2: gb_and_a_r8(cpu->R8.D); break; //AND A,D - 1
            case 0xA3: gb_and_a_r8(cpu->R8.E); break; //AND A,E - 1
            case 0xA4: gb_and_a_r8(cpu->R8.H); break; //AND A,H - 1
            case 0xA5: gb_and_a_r8(cpu->R8.L); break; //AND A,L - 1
            case 0xA6: //AND A,[HL] - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= GB_MemRead8(cpu->R16.HL);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA7: //AND A,A - 1
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                //cpu->R8.A &= cpu->R8.A;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA8: gb_xor_a_r8(cpu->R8.B); break; //XOR A,B - 1
            case 0xA9: gb_xor_a_r8(cpu->R8.C); break; //XOR A,C - 1
            case 0xAA: gb_xor_a_r8(cpu->R8.D); break; //XOR A,D - 1
            case 0xAB: gb_xor_a_r8(cpu->R8.E); break; //XOR A,E - 1
            case 0xAC: gb_xor_a_r8(cpu->R8.H); break; //XOR A,H - 1
            case 0xAD: gb_xor_a_r8(cpu->R8.L); break; //XOR A,L - 1
            case 0xAE: //XOR A,[HL] - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= GB_MemRead8(cpu->R16.HL);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xAF: //XOR A,A - 1
                cpu->R16.AF = F_ZERO;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB0: gb_or_a_r8(cpu->R8.B); break; //OR A,B - 1
            case 0xB1: gb_or_a_r8(cpu->R8.C); break; //OR A,C - 1
            case 0xB2: gb_or_a_r8(cpu->R8.D); break; //OR A,D - 1
            case 0xB3: gb_or_a_r8(cpu->R8.E); break; //OR A,E - 1
            case 0xB4: gb_or_a_r8(cpu->R8.H); break; //OR A,H - 1
            case 0xB5: gb_or_a_r8(cpu->R8.L); break; //OR A,L - 1
            case 0xB6: //OR A,[HL] - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= GB_MemRead8(cpu->R16.HL);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB7: //OR A,A - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                //cpu->R8.A |= cpu->R8.A;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB8: gb_cp_a_r8(cpu->R8.B); break; //CP A,B - 1
            case 0xB9: gb_cp_a_r8(cpu->R8.C); break; //CP A,C - 1
            case 0xBA: gb_cp_a_r8(cpu->R8.D); break; //CP A,D - 1
            case 0xBB: gb_cp_a_r8(cpu->R8.E); break; //CP A,E - 1
            case 0xBC: gb_cp_a_r8(cpu->R8.H); break; //CP A,H - 1
            case 0xBD: gb_cp_a_r8(cpu->R8.L); break; //CP A,L - 1
            case 0xBE: //CP A,[HL] - 2
            {
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF |= F_SUBTRACT;
                u32 temp = GB_MemRead8(cpu->R16.HL);
                cpu->F.H = (cpu->R8.A & 0xF) < (temp & 0xF);
                cpu->F.C = (u32)cpu->R8.A < temp;
                cpu->F.Z = (cpu->R8.A == temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xBF: //CP A,A - 1
                cpu->R16.AF |= (F_SUBTRACT|F_ZERO);
                cpu->R16.AF &= ~(F_HALFCARRY|F_CARRY);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xC0: gb_ret_cond(cpu->F.Z == 0); break; //RET NZ - 5/2
            case 0xC1: gb_pop_r16(cpu->R8.B,cpu->R8.C); break; //POP BC - 3
            case 0xC2: gb_jp_cond_nnnn(cpu->F.Z == 0); break; //JP NZ,nnnn - 4/3
            case 0xC3: //JP nnnn - 4
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                temp |= ( (u32)(u8)GB_MemRead8(cpu->R16.PC++) ) << 8;
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xC4: gb_call_cond_nnnn(cpu->F.Z == 0); break; //CALL NZ,nnnn - 6/3
            case 0xC5: gb_push_r16(cpu->R8.B,cpu->R8.C); break; //PUSH BC - 4
            case 0xC6: //ADD A,nn - 2
            {
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A;
                u32 temp2 = GB_MemRead8(cpu->R16.PC++);
                cpu->F.H = ( (temp & 0xF) + (temp2 & 0xF) ) > 0xF;
                cpu->R8.A += temp2;
                cpu->F.Z = (cpu->R8.A == 0);
                cpu->F.C = (temp > cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xC7: gb_rst_nnnn(0x0000); break; //RST 0x0000 - 4
            case 0xC8: gb_ret_cond(cpu->F.Z); break; //RET Z - 5/2
            case 0xC9: //RET - 4
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                temp |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xCA: gb_jp_cond_nnnn(cpu->F.Z); break; //JP Z,nnnn - 4/3
            case 0xCB:
                GB_CPUClockCounterAdd(4);
                opcode = (u32)(u8)GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;

                switch(opcode)
                {
                    case 0x00: gb_rlc_r8(cpu->R8.B); break; //RLC B - 2
                    case 0x01: gb_rlc_r8(cpu->R8.C); break; //RLC C - 2
                    case 0x02: gb_rlc_r8(cpu->R8.D); break; //RLC D - 2
                    case 0x03: gb_rlc_r8(cpu->R8.E); break; //RLC E - 2
                    case 0x04: gb_rlc_r8(cpu->R8.H); break; //RLC H - 2
                    case 0x05: gb_rlc_r8(cpu->R8.L); break; //RLC L - 2
                    case 0x06: //RLC [HL] - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x80) != 0;
                        temp = (temp << 1) | cpu->F.C;
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x07: gb_rlc_r8(cpu->R8.A); break; //RLC A - 2

                    case 0x08: gb_rrc_r8(cpu->R8.B); break; //RRC B - 2
                    case 0x09: gb_rrc_r8(cpu->R8.C); break; //RRC C - 2
                    case 0x0A: gb_rrc_r8(cpu->R8.D); break; //RRC D - 2
                    case 0x0B: gb_rrc_r8(cpu->R8.E); break; //RRC E - 2
                    case 0x0C: gb_rrc_r8(cpu->R8.H); break; //RRC H - 2
                    case 0x0D: gb_rrc_r8(cpu->R8.L); break; //RRC L - 2
                    case 0x0E: //RRC [HL] - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x01) != 0;
                        temp = (temp >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x0F: gb_rrc_r8(cpu->R8.A); break; //RRC A - 2

                    case 0x10: gb_rl_r8(cpu->R8.B); break; //RL B - 2
                    case 0x11: gb_rl_r8(cpu->R8.C); break; //RL C - 2
                    case 0x12: gb_rl_r8(cpu->R8.D); break; //RL D - 2
                    case 0x13: gb_rl_r8(cpu->R8.E); break; //RL E - 2
                    case 0x14: gb_rl_r8(cpu->R8.H); break; //RL H - 2
                    case 0x15: gb_rl_r8(cpu->R8.L); break; //RL L - 2
                    case 0x16: //RL [HL] - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp2 = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (temp2 & 0x80) != 0;
                        temp2 = ((temp2 << 1) | temp) & 0xFF;
                        cpu->F.Z = (temp2 == 0);
                        GB_MemWrite8(cpu->R16.HL, temp2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x17: gb_rl_r8(cpu->R8.A); break; //RL A - 2

                    case 0x18: gb_rr_r8(cpu->R8.B); break; //RR B - 2
                    case 0x19: gb_rr_r8(cpu->R8.C); break; //RR C - 2
                    case 0x1A: gb_rr_r8(cpu->R8.D); break; //RR D - 2
                    case 0x1B: gb_rr_r8(cpu->R8.E); break; //RR E - 2
                    case 0x1C: gb_rr_r8(cpu->R8.H); break; //RR H - 2
                    case 0x1D: gb_rr_r8(cpu->R8.L); break; //RR L - 2
                    case 0x1E: //RR [HL] - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp2 = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (temp2 & 0x01) != 0;
                        temp2 = (temp2 >> 1) | (temp << 7);
                        cpu->F.Z = (temp2 == 0);
                        GB_MemWrite8(cpu->R16.HL, temp2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x1F: gb_rr_r8(cpu->R8.A); break; //RR A - 2

                    case 0x20: gb_sla_r8(cpu->R8.B); break; //SLA B - 2
                    case 0x21: gb_sla_r8(cpu->R8.C); break; //SLA C - 2
                    case 0x22: gb_sla_r8(cpu->R8.D); break; //SLA D - 2
                    case 0x23: gb_sla_r8(cpu->R8.E); break; //SLA E - 2
                    case 0x24: gb_sla_r8(cpu->R8.H); break; //SLA H - 2
                    case 0x25: gb_sla_r8(cpu->R8.L); break; //SLA L - 2
                    case 0x26: //SLA [HL] - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x80) != 0;
                        temp = (temp << 1) & 0xFF;
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x27: gb_sla_r8(cpu->R8.A); break; //SLA A - 2

                    case 0x28: gb_sra_r8(cpu->R8.B); break; //SRA B - 2
                    case 0x29: gb_sra_r8(cpu->R8.C); break; //SRA C - 2
                    case 0x2A: gb_sra_r8(cpu->R8.D); break; //SRA D - 2
                    case 0x2B: gb_sra_r8(cpu->R8.E); break; //SRA E - 2
                    case 0x2C: gb_sra_r8(cpu->R8.H); break; //SRA H - 2
                    case 0x2D: gb_sra_r8(cpu->R8.L); break; //SRA L - 2
                    case 0x2E: //SRA [HL] - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x01) != 0;
                        temp = (temp & 0x80) | (temp >> 1);
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x2F: gb_sra_r8(cpu->R8.A); break; //SRA A - 2

                    case 0x30: gb_swap_r8(cpu->R8.B); break; //SWAP B - 2
                    case 0x31: gb_swap_r8(cpu->R8.C); break; //SWAP C - 2
                    case 0x32: gb_swap_r8(cpu->R8.D); break; //SWAP D - 2
                    case 0x33: gb_swap_r8(cpu->R8.E); break; //SWAP E - 2
                    case 0x34: gb_swap_r8(cpu->R8.H); break; //SWAP H - 2
                    case 0x35: gb_swap_r8(cpu->R8.L); break; //SWAP L - 2
                    case 0x36: //SWAP [HL] - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        temp = ((temp >> 4) | (temp << 4)) & 0xFF;
                        GB_MemWrite8(cpu->R16.HL,temp);
                        cpu->F.Z = (temp == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x37: gb_swap_r8(cpu->R8.A); break; //SWAP A - 2

                    case 0x38: gb_srl_r8(cpu->R8.B); break; //SRL B - 2
                    case 0x39: gb_srl_r8(cpu->R8.C); break; //SRL C - 2
                    case 0x3A: gb_srl_r8(cpu->R8.D); break; //SRL D - 2
                    case 0x3B: gb_srl_r8(cpu->R8.E); break; //SRL E - 2
                    case 0x3C: gb_srl_r8(cpu->R8.H); break; //SRL H - 2
                    case 0x3D: gb_srl_r8(cpu->R8.L); break; //SRL L - 2
                    case 0x3E: //SRL [HL] - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x01) != 0;
                        temp = temp >> 1;
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x3F: gb_srl_r8(cpu->R8.A); break; //SRL A - 2

                    case 0x40: gb_bit_n_r8(0,cpu->R8.B); break; //BIT 0,B - 2
                    case 0x41: gb_bit_n_r8(0,cpu->R8.C); break; //BIT 0,C - 2
                    case 0x42: gb_bit_n_r8(0,cpu->R8.D); break; //BIT 0,D - 2
                    case 0x43: gb_bit_n_r8(0,cpu->R8.E); break; //BIT 0,E - 2
                    case 0x44: gb_bit_n_r8(0,cpu->R8.H); break; //BIT 0,H - 2
                    case 0x45: gb_bit_n_r8(0,cpu->R8.L); break; //BIT 0,L - 2
                    case 0x46: gb_bit_n_ptr_hl(0); break; //BIT 0,[HL] - 3
                    case 0x47: gb_bit_n_r8(0,cpu->R8.A); break; //BIT 0,A - 2
                    case 0x48: gb_bit_n_r8(1,cpu->R8.B); break; //BIT 1,B - 2
                    case 0x49: gb_bit_n_r8(1,cpu->R8.C); break; //BIT 1,C - 2
                    case 0x4A: gb_bit_n_r8(1,cpu->R8.D); break; //BIT 1,D - 2
                    case 0x4B: gb_bit_n_r8(1,cpu->R8.E); break; //BIT 1,E - 2
                    case 0x4C: gb_bit_n_r8(1,cpu->R8.H); break; //BIT 1,H - 2
                    case 0x4D: gb_bit_n_r8(1,cpu->R8.L); break; //BIT 1,L - 2
                    case 0x4E: gb_bit_n_ptr_hl(1); break; //BIT 1,[HL] - 3
                    case 0x4F: gb_bit_n_r8(1,cpu->R8.A); break; //BIT 1,A - 2
                    case 0x50: gb_bit_n_r8(2,cpu->R8.B); break; //BIT 2,B - 2
                    case 0x51: gb_bit_n_r8(2,cpu->R8.C); break; //BIT 2,C - 2
                    case 0x52: gb_bit_n_r8(2,cpu->R8.D); break; //BIT 2,D - 2
                    case 0x53: gb_bit_n_r8(2,cpu->R8.E); break; //BIT 2,E - 2
                    case 0x54: gb_bit_n_r8(2,cpu->R8.H); break; //BIT 2,H - 2
                    case 0x55: gb_bit_n_r8(2,cpu->R8.L); break; //BIT 2,L - 2
                    case 0x56: gb_bit_n_ptr_hl(2); break; //BIT 2,[HL] - 3
                    case 0x57: gb_bit_n_r8(2,cpu->R8.A); break; //BIT 2,A - 2
                    case 0x58: gb_bit_n_r8(3,cpu->R8.B); break; //BIT 3,B - 2
                    case 0x59: gb_bit_n_r8(3,cpu->R8.C); break; //BIT 3,C - 2
                    case 0x5A: gb_bit_n_r8(3,cpu->R8.D); break; //BIT 3,D - 2
                    case 0x5B: gb_bit_n_r8(3,cpu->R8.E); break; //BIT 3,E - 2
                    case 0x5C: gb_bit_n_r8(3,cpu->R8.H); break; //BIT 3,H - 2
                    case 0x5D: gb_bit_n_r8(3,cpu->R8.L); break; //BIT 3,L - 2
                    case 0x5E: gb_bit_n_ptr_hl(3); break; //BIT 3,[HL] - 3
                    case 0x5F: gb_bit_n_r8(3,cpu->R8.A); break; //BIT 3,A - 2
                    case 0x60: gb_bit_n_r8(4,cpu->R8.B); break; //BIT 4,B - 2
                    case 0x61: gb_bit_n_r8(4,cpu->R8.C); break; //BIT 4,C - 2
                    case 0x62: gb_bit_n_r8(4,cpu->R8.D); break; //BIT 4,D - 2
                    case 0x63: gb_bit_n_r8(4,cpu->R8.E); break; //BIT 4,E - 2
                    case 0x64: gb_bit_n_r8(4,cpu->R8.H); break; //BIT 4,H - 2
                    case 0x65: gb_bit_n_r8(4,cpu->R8.L); break; //BIT 4,L - 2
                    case 0x66: gb_bit_n_ptr_hl(4); break; //BIT 4,[HL] - 3
                    case 0x67: gb_bit_n_r8(4,cpu->R8.A); break; //BIT 4,A - 2
                    case 0x68: gb_bit_n_r8(5,cpu->R8.B); break; //BIT 5,B - 2
                    case 0x69: gb_bit_n_r8(5,cpu->R8.C); break; //BIT 5,C - 2
                    case 0x6A: gb_bit_n_r8(5,cpu->R8.D); break; //BIT 5,D - 2
                    case 0x6B: gb_bit_n_r8(5,cpu->R8.E); break; //BIT 5,E - 2
                    case 0x6C: gb_bit_n_r8(5,cpu->R8.H); break; //BIT 5,H - 2
                    case 0x6D: gb_bit_n_r8(5,cpu->R8.L); break; //BIT 5,L - 2
                    case 0x6E: gb_bit_n_ptr_hl(5); break; //BIT 5,[HL] - 3
                    case 0x6F: gb_bit_n_r8(5,cpu->R8.A); break; //BIT 5,A - 2
                    case 0x70: gb_bit_n_r8(6,cpu->R8.B); break; //BIT 6,B - 2
                    case 0x71: gb_bit_n_r8(6,cpu->R8.C); break; //BIT 6,C - 2
                    case 0x72: gb_bit_n_r8(6,cpu->R8.D); break; //BIT 6,D - 2
                    case 0x73: gb_bit_n_r8(6,cpu->R8.E); break; //BIT 6,E - 2
                    case 0x74: gb_bit_n_r8(6,cpu->R8.H); break; //BIT 6,H - 2
                    case 0x75: gb_bit_n_r8(6,cpu->R8.L); break; //BIT 6,L - 2
                    case 0x76: gb_bit_n_ptr_hl(6); break; //BIT 6,[HL] - 3
                    case 0x77: gb_bit_n_r8(6,cpu->R8.A); break; //BIT 6,A - 2
                    case 0x78: gb_bit_n_r8(7,cpu->R8.B); break; //BIT 7,B - 2
                    case 0x79: gb_bit_n_r8(7,cpu->R8.C); break; //BIT 7,C - 2
                    case 0x7A: gb_bit_n_r8(7,cpu->R8.D); break; //BIT 7,D - 2
                    case 0x7B: gb_bit_n_r8(7,cpu->R8.E); break; //BIT 7,E - 2
                    case 0x7C: gb_bit_n_r8(7,cpu->R8.H); break; //BIT 7,H - 2
                    case 0x7D: gb_bit_n_r8(7,cpu->R8.L); break; //BIT 7,L - 2
                    case 0x7E: gb_bit_n_ptr_hl(7); break; //BIT 7,[HL] - 3
                    case 0x7F: gb_bit_n_r8(7,cpu->R8.A); break; //BIT 7,A - 2

                    case 0x80: gb_res_n_r8(0,cpu->R8.B); break; //RES 0,B - 2
                    case 0x81: gb_res_n_r8(0,cpu->R8.C); break; //RES 0,C - 2
                    case 0x82: gb_res_n_r8(0,cpu->R8.D); break; //RES 0,D - 2
                    case 0x83: gb_res_n_r8(0,cpu->R8.E); break; //RES 0,E - 2
                    case 0x84: gb_res_n_r8(0,cpu->R8.H); break; //RES 0,H - 2
                    case 0x85: gb_res_n_r8(0,cpu->R8.L); break; //RES 0,L - 2
                    case 0x86: gb_res_n_ptr_hl(0); break; //RES 0,[HL] - 4
                    case 0x87: gb_res_n_r8(0,cpu->R8.A); break; //RES 0,A - 2
                    case 0x88: gb_res_n_r8(1,cpu->R8.B); break; //RES 1,B - 2
                    case 0x89: gb_res_n_r8(1,cpu->R8.C); break; //RES 1,C - 2
                    case 0x8A: gb_res_n_r8(1,cpu->R8.D); break; //RES 1,D - 2
                    case 0x8B: gb_res_n_r8(1,cpu->R8.E); break; //RES 1,E - 2
                    case 0x8C: gb_res_n_r8(1,cpu->R8.H); break; //RES 1,H - 2
                    case 0x8D: gb_res_n_r8(1,cpu->R8.L); break; //RES 1,L - 2
                    case 0x8E: gb_res_n_ptr_hl(1); break; //RES 1,[HL] - 4
                    case 0x8F: gb_res_n_r8(1,cpu->R8.A); break; //RES 1,A - 2
                    case 0x90: gb_res_n_r8(2,cpu->R8.B); break; //RES 2,B - 2
                    case 0x91: gb_res_n_r8(2,cpu->R8.C); break; //RES 2,C - 2
                    case 0x92: gb_res_n_r8(2,cpu->R8.D); break; //RES 2,D - 2
                    case 0x93: gb_res_n_r8(2,cpu->R8.E); break; //RES 2,E - 2
                    case 0x94: gb_res_n_r8(2,cpu->R8.H); break; //RES 2,H - 2
                    case 0x95: gb_res_n_r8(2,cpu->R8.L); break; //RES 2,L - 2
                    case 0x96: gb_res_n_ptr_hl(2); break; //RES 2,[HL] - 4
                    case 0x97: gb_res_n_r8(2,cpu->R8.A); break; //RES 2,A - 2
                    case 0x98: gb_res_n_r8(3,cpu->R8.B); break; //RES 3,B - 2
                    case 0x99: gb_res_n_r8(3,cpu->R8.C); break; //RES 3,C - 2
                    case 0x9A: gb_res_n_r8(3,cpu->R8.D); break; //RES 3,D - 2
                    case 0x9B: gb_res_n_r8(3,cpu->R8.E); break; //RES 3,E - 2
                    case 0x9C: gb_res_n_r8(3,cpu->R8.H); break; //RES 3,H - 2
                    case 0x9D: gb_res_n_r8(3,cpu->R8.L); break; //RES 3,L - 2
                    case 0x9E: gb_res_n_ptr_hl(3); break; //RES 3,[HL] - 4
                    case 0x9F: gb_res_n_r8(3,cpu->R8.A); break; //RES 3,A - 2
                    case 0xA0: gb_res_n_r8(4,cpu->R8.B); break; //RES 4,B - 2
                    case 0xA1: gb_res_n_r8(4,cpu->R8.C); break; //RES 4,C - 2
                    case 0xA2: gb_res_n_r8(4,cpu->R8.D); break; //RES 4,D - 2
                    case 0xA3: gb_res_n_r8(4,cpu->R8.E); break; //RES 4,E - 2
                    case 0xA4: gb_res_n_r8(4,cpu->R8.H); break; //RES 4,H - 2
                    case 0xA5: gb_res_n_r8(4,cpu->R8.L); break; //RES 4,L - 2
                    case 0xA6: gb_res_n_ptr_hl(4); break; //RES 4,[HL] - 4
                    case 0xA7: gb_res_n_r8(4,cpu->R8.A); break; //RES 4,A - 2
                    case 0xA8: gb_res_n_r8(5,cpu->R8.B); break; //RES 5,B - 2
                    case 0xA9: gb_res_n_r8(5,cpu->R8.C); break; //RES 5,C - 2
                    case 0xAA: gb_res_n_r8(5,cpu->R8.D); break; //RES 5,D - 2
                    case 0xAB: gb_res_n_r8(5,cpu->R8.E); break; //RES 5,E - 2
                    case 0xAC: gb_res_n_r8(5,cpu->R8.H); break; //RES 5,H - 2
                    case 0xAD: gb_res_n_r8(5,cpu->R8.L); break; //RES 5,L - 2
                    case 0xAE: gb_res_n_ptr_hl(5); break; //RES 5,[HL] - 4
                    case 0xAF: gb_res_n_r8(5,cpu->R8.A); break; //RES 5,A - 2
                    case 0xB0: gb_res_n_r8(6,cpu->R8.B); break; //RES 6,B - 2
                    case 0xB1: gb_res_n_r8(6,cpu->R8.C); break; //RES 6,C - 2
                    case 0xB2: gb_res_n_r8(6,cpu->R8.D); break; //RES 6,D - 2
                    case 0xB3: gb_res_n_r8(6,cpu->R8.E); break; //RES 6,E - 2
                    case 0xB4: gb_res_n_r8(6,cpu->R8.H); break; //RES 6,H - 2
                    case 0xB5: gb_res_n_r8(6,cpu->R8.L); break; //RES 6,L - 2
                    case 0xB6: gb_res_n_ptr_hl(6); break; //RES 6,[HL] - 4
                    case 0xB7: gb_res_n_r8(6,cpu->R8.A); break; //RES 6,A - 2
                    case 0xB8: gb_res_n_r8(7,cpu->R8.B); break; //RES 7,B - 2
                    case 0xB9: gb_res_n_r8(7,cpu->R8.C); break; //RES 7,C - 2
                    case 0xBA: gb_res_n_r8(7,cpu->R8.D); break; //RES 7,D - 2
                    case 0xBB: gb_res_n_r8(7,cpu->R8.E); break; //RES 7,E - 2
                    case 0xBC: gb_res_n_r8(7,cpu->R8.H); break; //RES 7,H - 2
                    case 0xBD: gb_res_n_r8(7,cpu->R8.L); break; //RES 7,L - 2
                    case 0xBE: gb_res_n_ptr_hl(7); break; //RES 7,[HL] - 4
                    case 0xBF: gb_res_n_r8(7,cpu->R8.A); break; //RES 7,A - 2

                    case 0xC0: gb_set_n_r8(0,cpu->R8.B); break; //SET 0,B - 2
                    case 0xC1: gb_set_n_r8(0,cpu->R8.C); break; //SET 0,C - 2
                    case 0xC2: gb_set_n_r8(0,cpu->R8.D); break; //SET 0,D - 2
                    case 0xC3: gb_set_n_r8(0,cpu->R8.E); break; //SET 0,E - 2
                    case 0xC4: gb_set_n_r8(0,cpu->R8.H); break; //SET 0,H - 2
                    case 0xC5: gb_set_n_r8(0,cpu->R8.L); break; //SET 0,L - 2
                    case 0xC6: gb_set_n_ptr_hl(0); break; //SET 0,[HL] - 4
                    case 0xC7: gb_set_n_r8(0,cpu->R8.A); break; //SET 0,A - 2
                    case 0xC8: gb_set_n_r8(1,cpu->R8.B); break; //SET 1,B - 2
                    case 0xC9: gb_set_n_r8(1,cpu->R8.C); break; //SET 1,C - 2
                    case 0xCA: gb_set_n_r8(1,cpu->R8.D); break; //SET 1,D - 2
                    case 0xCB: gb_set_n_r8(1,cpu->R8.E); break; //SET 1,E - 2
                    case 0xCC: gb_set_n_r8(1,cpu->R8.H); break; //SET 1,H - 2
                    case 0xCD: gb_set_n_r8(1,cpu->R8.L); break; //SET 1,L - 2
                    case 0xCE: gb_set_n_ptr_hl(1); break; //SET 1,[HL] - 4
                    case 0xCF: gb_set_n_r8(1,cpu->R8.A); break; //SET 1,A - 2
                    case 0xD0: gb_set_n_r8(2,cpu->R8.B); break; //SET 2,B - 2
                    case 0xD1: gb_set_n_r8(2,cpu->R8.C); break; //SET 2,C - 2
                    case 0xD2: gb_set_n_r8(2,cpu->R8.D); break; //SET 2,D - 2
                    case 0xD3: gb_set_n_r8(2,cpu->R8.E); break; //SET 2,E - 2
                    case 0xD4: gb_set_n_r8(2,cpu->R8.H); break; //SET 2,H - 2
                    case 0xD5: gb_set_n_r8(2,cpu->R8.L); break; //SET 2,L - 2
                    case 0xD6: gb_set_n_ptr_hl(2); break; //SET 2,[HL] - 4
                    case 0xD7: gb_set_n_r8(2,cpu->R8.A); break; //SET 2,A - 2
                    case 0xD8: gb_set_n_r8(3,cpu->R8.B); break; //SET 3,B - 2
                    case 0xD9: gb_set_n_r8(3,cpu->R8.C); break; //SET 3,C - 2
                    case 0xDA: gb_set_n_r8(3,cpu->R8.D); break; //SET 3,D - 2
                    case 0xDB: gb_set_n_r8(3,cpu->R8.E); break; //SET 3,E - 2
                    case 0xDC: gb_set_n_r8(3,cpu->R8.H); break; //SET 3,H - 2
                    case 0xDD: gb_set_n_r8(3,cpu->R8.L); break; //SET 3,L - 2
                    case 0xDE: gb_set_n_ptr_hl(3); break; //SET 3,[HL] - 4
                    case 0xDF: gb_set_n_r8(3,cpu->R8.A); break; //SET 3,A - 2
                    case 0xE0: gb_set_n_r8(4,cpu->R8.B); break; //SET 4,B - 2
                    case 0xE1: gb_set_n_r8(4,cpu->R8.C); break; //SET 4,C - 2
                    case 0xE2: gb_set_n_r8(4,cpu->R8.D); break; //SET 4,D - 2
                    case 0xE3: gb_set_n_r8(4,cpu->R8.E); break; //SET 4,E - 2
                    case 0xE4: gb_set_n_r8(4,cpu->R8.H); break; //SET 4,H - 2
                    case 0xE5: gb_set_n_r8(4,cpu->R8.L); break; //SET 4,L - 2
                    case 0xE6: gb_set_n_ptr_hl(4); break; //SET 4,[HL] - 4
                    case 0xE7: gb_set_n_r8(4,cpu->R8.A); break; //SET 4,A - 2
                    case 0xE8: gb_set_n_r8(5,cpu->R8.B); break; //SET 5,B - 2
                    case 0xE9: gb_set_n_r8(5,cpu->R8.C); break; //SET 5,C - 2
                    case 0xEA: gb_set_n_r8(5,cpu->R8.D); break; //SET 5,D - 2
                    case 0xEB: gb_set_n_r8(5,cpu->R8.E); break; //SET 5,E - 2
                    case 0xEC: gb_set_n_r8(5,cpu->R8.H); break; //SET 5,H - 2
                    case 0xED: gb_set_n_r8(5,cpu->R8.L); break; //SET 5,L - 2
                    case 0xEE: gb_set_n_ptr_hl(5); break; //SET 5,[HL] - 4
                    case 0xEF: gb_set_n_r8(5,cpu->R8.A); break; //SET 5,A - 2
                    case 0xF0: gb_set_n_r8(6,cpu->R8.B); break; //SET 6,B - 2
                    case 0xF1: gb_set_n_r8(6,cpu->R8.C); break; //SET 6,C - 2
                    case 0xF2: gb_set_n_r8(6,cpu->R8.D); break; //SET 6,D - 2
                    case 0xF3: gb_set_n_r8(6,cpu->R8.E); break; //SET 6,E - 2
                    case 0xF4: gb_set_n_r8(6,cpu->R8.H); break; //SET 6,H - 2
                    case 0xF5: gb_set_n_r8(6,cpu->R8.L); break; //SET 6,L - 2
                    case 0xF6: gb_set_n_ptr_hl(6); break; //SET 6,[HL] - 4
                    case 0xF7: gb_set_n_r8(6,cpu->R8.A); break; //SET 6,A - 2
                    case 0xF8: gb_set_n_r8(7,cpu->R8.B); break; //SET 7,B - 2
                    case 0xF9: gb_set_n_r8(7,cpu->R8.C); break; //SET 7,C - 2
                    case 0xFA: gb_set_n_r8(7,cpu->R8.D); break; //SET 7,D - 2
                    case 0xFB: gb_set_n_r8(7,cpu->R8.E); break; //SET 7,E - 2
                    case 0xFC: gb_set_n_r8(7,cpu->R8.H); break; //SET 7,H - 2
                    case 0xFD: gb_set_n_r8(7,cpu->R8.L); break; //SET 7,L - 2
                    case 0xFE: gb_set_n_ptr_hl(7); break; //SET 7,[HL] - 4
                    case 0xFF: gb_set_n_r8(7,cpu->R8.A); break; //SET 7,A - 2

                    default:
                        // ...wtf??
                        GB_CPUClockCounterAdd(4);
                        _gb_break_to_debugger();
                        Debug_ErrorMsgArg("Unidentified opcode. 0xCB 0x%X\nPC: %04X\nROM: %d",
                            opcode,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                        break;
                } //end of inner switch
                break;
            case 0xCC: gb_call_cond_nnnn(cpu->F.Z); break; //CALL Z,nnnn - 6/3
            case 0xCD: //CALL nnnn - 6
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xCE: //ADC A,nn - 2
            {
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                u32 temp2 = cpu->R8.A + temp + cpu->F.C;
                cpu->F.H = ( ((cpu->R8.A & 0xF) + (temp & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp2 > 0xFF);
                cpu->R8.A = (temp2 & 0xFF);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xCF: gb_rst_nnnn(0x0008); break; //RST 0x0008 - 4
            case 0xD0: gb_ret_cond(cpu->F.C == 0); break; //RET NC - 5/2
            case 0xD1: gb_pop_r16(cpu->R8.D,cpu->R8.E); break; //POP DE - 3
            case 0xD2: gb_jp_cond_nnnn(cpu->F.C == 0); break; //JP NC,nnnn - 4/3
            case 0xD3: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xD4: gb_call_cond_nnnn(cpu->F.C == 0); break; //CALL NC,nnnn - 6/3
            case 0xD5: gb_push_r16(cpu->R8.D,cpu->R8.E); break; //PUSH DE - 4
            case 0xD6:  //SUB A,nn - 2
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (temp & 0xF);
                cpu->F.C = (u32)cpu->R8.A < temp;
                cpu->R8.A -= temp;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xD7: gb_rst_nnnn(0x0010); break; //RST 0x0010 - 4
            case 0xD8: gb_ret_cond(cpu->F.C); break; //RET C - 5/2
            case 0xD9: //RETI - 4
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                temp |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = temp;
                GameBoy.Memory.InterruptMasterEnable = 1;
                GB_CPUClockCounterAdd(4);
                GB_CPUBreakLoop();
                break;
            }
            case 0xDA: gb_jp_cond_nnnn(cpu->F.C); break; //JP C,nnnn - 4/3
            case 0xDB: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xDC: gb_call_cond_nnnn(cpu->F.C); break; //CALL C,nnnn - 6/3
            case 0xDD: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xDE: //SBC A,nn - 2
            {
                GB_CPUClockCounterAdd(4);
                u32 temp2 = GB_MemRead8(cpu->R16.PC++);
                u32 temp = cpu->R8.A - temp2 - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^temp2^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xDF: gb_rst_nnnn(0x0018); break; //RST 0x0018 - 4
            case 0xE0: //LD [0xFF00+nn],A - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = 0xFF00 + (u32)GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(temp,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xE1: gb_pop_r16(cpu->R8.H,cpu->R8.L); break; //POP HL - 3
            case 0xE2: //LD [0xFF00+C],A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(0xFF00 + (u32)cpu->R8.C,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xE3: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xE4: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xE5: gb_push_r16(cpu->R8.H,cpu->R8.L); break; //PUSH HL - 4
            case 0xE6: //AND A,nn - 2
            {
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R8.A &= GB_MemRead8(cpu->R16.PC++);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xE7: gb_rst_nnnn(0x0020); break; //RST 0x0020 - 4
            case 0xE8: //ADD SP,nn - 4
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = (u16)(s16)(s8)GB_MemRead8(cpu->R16.PC++); //expand sign
                GB_CPUClockCounterAdd(8);
                cpu->R8.F = 0;
                cpu->F.C = ( ( (cpu->R16.SP & 0x00FF) + (temp & 0x00FF) ) > 0x00FF );
                cpu->F.H = ( ( (cpu->R16.SP & 0x000F) + (temp & 0x000F) ) > 0x000F );
                cpu->R16.SP = (cpu->R16.SP + temp) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xE9: //JP HL - 1
                cpu->R16.PC = cpu->R16.HL;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xEA: //LD [nnnn],A - 4
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(temp,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xEB: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xEC: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xED: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xEE: //XOR A,nn - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= GB_MemRead8(cpu->R16.PC++);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xEF: gb_rst_nnnn(0x0028); break; //RST 0x0028 - 4

            case 0xF0: //LD A,[0xFF00+nn] - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = 0xFF00 + (u32)GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xF1: //POP AF - 3
                gb_pop_r16(cpu->R8.A,cpu->R8.F);
                cpu->R8.F &= 0xF0; // Lower 4 bits are always 0
                break;
            case 0xF2: //LD A,[0xFF00+C] - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(0xFF00 + (u32)cpu->R8.C);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF3: //DI - 1
                GameBoy.Memory.InterruptMasterEnable = 0;
                GameBoy.Memory.interrupts_enable_count = 0;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF4: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xF5: gb_push_r16(cpu->R8.A,cpu->R8.F); break; //PUSH AF - 4
            case 0xF6: //OR A,nn - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= GB_MemRead8(cpu->R16.PC++);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF7: gb_rst_nnnn(0x0030); break; //RST 0x0030 - 4
            case 0xF8: //LD HL,SP+nn - 3
            {
                GB_CPUClockCounterAdd(4);
                s32 temp = (s32)(s8)GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                s32 res = (s32)cpu->R16.SP + temp;
                cpu->R16.HL = res & 0xFFFF;
                cpu->R8.F = 0;
                cpu->F.C = ( ( (cpu->R16.SP & 0x00FF) + (temp & 0x00FF) ) > 0x00FF );
                cpu->F.H = ( ( (cpu->R16.SP & 0x000F) + (temp & 0x000F) ) > 0x000F );
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xF9: //LD SP,HL - 2
                cpu->R16.SP = cpu->R16.HL;
                GB_CPUClockCounterAdd(8);
                break;
            case 0xFA: //LD A,[nnnn] - 4
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                cpu->R16.PC &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xFB: //EI - 1
                GameBoy.Memory.interrupts_enable_count = 1;
                //GameBoy.Memory.InterruptMasterEnable = 1;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xFC: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xFD: gb_undefined_opcode(opcode); break; // Undefined - *
            case 0xFE: //CP A,nn - 2
            {
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF |= F_SUBTRACT;
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                u32 temp2 = cpu->R8.A;
                cpu->F.H = (temp2 & 0xF) < (temp & 0xF);
                cpu->F.C = (temp2 < temp);
                cpu->F.Z = (temp2 == temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xFF: gb_rst_nnnn(0x0038); break; //RST 0x0038 - 4

            default: //wtf...?
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. 0x%X\nPC: %04X\nROM: %d",
                            opcode,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
        } //end switch

        if(gb_break_cpu_loop) // some event happened - handle it outside
        {
            gb_break_cpu_loop = 0;
            break;
        }

        //debug break function
        if(gb_break_execution) // something important has happened - exit from execution. Don't set this flag to 0 here!
            break;

    } // end while

    return GB_CPUClockCounterGet() - previous_clocks_counter;
}

//----------------------------------------------------------------

//returns 1 if breakpoint executed
int GB_RunFor(s32 run_for_clocks) // 1 frame = 70224 clocks
{
    gb_break_execution = 0;

    Win_GBDisassemblerStartAddressSetDefault();
    run_for_clocks += gb_last_residual_clocks;
    if(run_for_clocks < 0) run_for_clocks = 1;

    GB_ClockCountersReset();

    while(1)
    {
        int clocks_to_next_event = GB_ClocksForNextEvent();
        clocks_to_next_event = min(clocks_to_next_event, run_for_clocks);

        if(clocks_to_next_event > 0)
        {
            int executed_clocks = 0;

            if(GameBoy.Emulator.cpu_change_speed_clocks)
            {
                if(clocks_to_next_event >= GameBoy.Emulator.cpu_change_speed_clocks)
                {
                    executed_clocks = GameBoy.Emulator.cpu_change_speed_clocks;
                    GameBoy.Emulator.cpu_change_speed_clocks = 0;
                    GameBoy.Emulator.CPUHalt = 0; // exit change speed mode
                }
                else
                {
                    executed_clocks = clocks_to_next_event;
                    GameBoy.Emulator.cpu_change_speed_clocks -= clocks_to_next_event;
                }
                GB_CPUClockCounterAdd(executed_clocks);
            }
            else
            {
                int dma_executed_clocks = GB_DMAExecute(clocks_to_next_event); // GB_CPUClockCounterAdd() internal
                if(dma_executed_clocks == 0)
                {
                    int irq_executed_clocks = GB_InterruptsExecute(); // GB_CPUClockCounterAdd() internal
                    if(irq_executed_clocks == 0)
                    {
                        if(GameBoy.Emulator.CPUHalt == 0) // no halt
                        {
                            executed_clocks = GB_CPUExecute(clocks_to_next_event); // GB_CPUClockCounterAdd() internal
                        }
                        else // halt or stop
                        {
                            executed_clocks = clocks_to_next_event;
                            GB_CPUClockCounterAdd(clocks_to_next_event);
                        }
                    }
                    else
                    {
                        executed_clocks = irq_executed_clocks;
                    }
                }
                else
                {
                    executed_clocks = dma_executed_clocks;
                }
            }

            run_for_clocks -= executed_clocks;
        }

        GB_UpdateCounterToClocks(GB_CPUClockCounterGet());

        if( (run_for_clocks <= 0) || GameBoy.Emulator.FrameDrawn )
        {
            gb_last_residual_clocks = run_for_clocks;
            GameBoy.Emulator.FrameDrawn = 0;
            return 0;
        }

        if(gb_break_execution)
        {
            gb_last_residual_clocks = 0;
            return 1;
        }
    }

    //Should never reach this point
    return 0;
}

void GB_RunForInstruction(void)
{
    gb_last_residual_clocks = 0;
    GB_RunFor(4);
}

//----------------------------------------------------------------
