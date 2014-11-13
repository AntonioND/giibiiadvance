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
    //clocks_to_next_event = min(clocks_to_next_event,GB_SoundClocksToNextEvent());
    return clocks_to_next_event;
}

static inline void GB_ClockCountersReset(void)
{
    GB_CPUClockCounterReset();
    GB_TimersClockCounterReset();
    GB_PPUClockCounterReset();
    GB_SerialClockCounterReset();
    GB_SoundClockCounterReset();
    GB_DMAClockCounterReset();
    //GB_<...>ClockCounterReset();
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

    GB_CPUInterruptsInit();

    gb_break_cpu_loop = 0;
    gb_last_residual_clocks = 0;

    GameBoy.Emulator.ScreenMode = 2;
    GameBoy.Memory.IO_Ports[STAT_REG-0xFF00] = 0x02;
    GameBoy.Emulator.CurrentScanLine = 0;
    GameBoy.Emulator.CPUHalt = 0;
    GameBoy.Emulator.DoubleSpeed = 0;
    //GameBoy.Emulator.halt_not_executed = 0;
    GameBoy.Emulator.halt_dmg_bug = 0;

    if(GameBoy.Emulator.CGBEnabled == 1)
    {
        GameBoy.Emulator.ScreenMode = 1;
        GameBoy.Memory.IO_Ports[STAT_REG-0xFF00] = 0x01;
        GameBoy.Emulator.CurrentScanLine = 0x90;
    }

    //Registers
    if(GameBoy.Emulator.boot_rom_loaded == 0)
    {
        GameBoy.CPU.R8.F = 0xB0;

        if(GameBoy.Emulator.CGBEnabled)
        {
            GameBoy.CPU.R16.BC = 0x0000;
            GameBoy.CPU.R16.DE = 0xFF56;
            GameBoy.CPU.R16.HL = 0x000D;
        }
        else
        {
            GameBoy.CPU.R16.BC = 0x0013;
            GameBoy.CPU.R16.DE = 0x00D8;
            GameBoy.CPU.R16.HL = 0x014D;
        }

        GameBoy.CPU.R16.PC = 0x0100;
        GameBoy.CPU.R16.SP = 0xFFFE;

        switch(GameBoy.Emulator.HardwareType)
        {
            case HW_GB:
            case HW_SGB:
                GameBoy.CPU.R8.A = 0x01; // SGB or Normal Gameboy
                break;
            case HW_GBP:
            case HW_SGB2:
                GameBoy.CPU.R8.A = 0xFF; // SGB2 or Pocket Gameboy
                break;
            case HW_GBA:
                GameBoy.CPU.R8.B |= 0x01; // GBA
                //NO BREAK
            case HW_GBC:
                GameBoy.CPU.R8.A = 0x11; // CGB or GBA
                break;
        }
    }
    else
    {
        // No idea of the real initial values (except the PC one, it must be 0x0000).
        // Maybe they are random?
        GameBoy.CPU.R16.AF = 0x0000;
        GameBoy.CPU.R16.BC = 0x0000;
        GameBoy.CPU.R16.DE = 0x0000;
        GameBoy.CPU.R16.HL = 0x0000;
        GameBoy.CPU.R16.PC = 0x0000;
        GameBoy.CPU.R16.SP = 0x0000;
    }
}

//----------------------------------------------------------------

int gb_break_execution = 0;

inline void _gb_break_to_debugger(void)
{
    gb_break_execution = 1;
}

//----------------------------------------------------------------

static int GB_IRQExecute(void)
{
    _GB_CPU_ * cpu = &GameBoy.CPU;
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    int executed_clocks = 0;

    int interrupts = mem->IO_Ports[IF_REG-0xFF00] & mem->HighRAM[IE_REG-0xFF80] & 0x1F;
    if(interrupts != 0)
    {
        if(mem->InterruptMasterEnable) // Execute interrupt
        {
            GameBoy.Emulator.CPUHalt = 0;

            // interrupts should need the same time in dual speed or normal speed!!

            GB_CPUClockCounterAdd(4 << GameBoy.Emulator.DoubleSpeed);
            GameBoy.Memory.InterruptMasterEnable = 0;
            GB_CPUClockCounterAdd(4 << GameBoy.Emulator.DoubleSpeed);
            cpu->R16.SP --;
            cpu->R16.SP &= 0xFFFF;
            GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            GB_CPUClockCounterAdd(4 << GameBoy.Emulator.DoubleSpeed);
            cpu->R16.SP --;
            cpu->R16.SP &= 0xFFFF;
            GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            GB_CPUClockCounterAdd(4 << GameBoy.Emulator.DoubleSpeed);
            {
                if(interrupts & I_VBLANK)
                { cpu->R16.PC = 0x0040; mem->IO_Ports[IF_REG-0xFF00] &= ~I_VBLANK; }
                else if(interrupts & I_STAT)
                { cpu->R16.PC = 0x0048; mem->IO_Ports[IF_REG-0xFF00] &= ~I_STAT; }
                else if(interrupts & I_TIMER)
                { cpu->R16.PC = 0x0050; mem->IO_Ports[IF_REG-0xFF00] &= ~I_TIMER; }
                else if(interrupts & I_SERIAL)
                { cpu->R16.PC = 0x0058; mem->IO_Ports[IF_REG-0xFF00] &= ~I_SERIAL; }
                else //if(interrupts & I_JOYPAD)
                { cpu->R16.PC = 0x0060; mem->IO_Ports[IF_REG-0xFF00] &= ~I_JOYPAD; }
            }
            GB_CPUClockCounterAdd(4 << GameBoy.Emulator.DoubleSpeed);

            executed_clocks = 20 << GameBoy.Emulator.DoubleSpeed;
        }
    }

    //if(GameBoy.Emulator.halt_not_executed && mem->InterruptMasterEnable)
    //{
    //    GameBoy.Emulator.CPUHalt = 1;
    //    GameBoy.Emulator.halt_not_executed = 0;
    //}

    return executed_clocks;
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

        if(GameBoy.Emulator.halt_dmg_bug)
        {
            GameBoy.Emulator.halt_dmg_bug = 0;
            cpu->R16.PC--;
            cpu->R16.PC &= 0xFFFF;
        }

        switch(opcode)
        {
            case 0x00: //NOP - 1
                GB_CPUClockCounterAdd(4);
                break;
            case 0x01: //LD BC,nn - 3
                GB_CPUClockCounterAdd(4);
                cpu->R8.C = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                cpu->R8.B = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x02: //LD (BC),A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.BC,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x03: //INC BC - 2
                cpu->R16.BC = (cpu->R16.BC+1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x04: //INC B - 1
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.B & 0xF) == 0xF);
                cpu->R8.B ++;
                cpu->F.Z = (cpu->R8.B == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x05: //DEC B - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.B & 0xF) == 0x0);
                cpu->R8.B --;
                cpu->F.Z = (cpu->R8.B == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x06: //LD B,n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.B = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x07: //RLCA - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
                cpu->F.C = (cpu->R8.A & 0x80) != 0;
                cpu->R8.A = (cpu->R8.A << 1) | cpu->F.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x08: //LD (nn),SP - 5
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
            case 0x09: //ADD HL,BC - 2
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R16.HL + cpu->R16.BC;
                cpu->F.C = (temp > 0xFFFF);
                cpu->F.H = ( ( (cpu->R16.HL & 0x0FFF) + (cpu->R16.BC & 0x0FFF) ) > 0x0FFF );
                cpu->R16.HL = temp & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            }
            case 0x0A: //LD A,(BC) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.BC);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x0B: //DEC BC - 2
                cpu->R16.BC = (cpu->R16.BC-1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x0C: //INC C - 1
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.C & 0xF) == 0xF);
                cpu->R8.C ++;
                cpu->F.Z = (cpu->R8.C == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x0D: //DEC C - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.C & 0xF) == 0x0);
                cpu->R8.C --;
                cpu->F.Z = (cpu->R8.C == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x0E: //LD C,n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.C = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x0F: //RRCA - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
                cpu->F.C = (cpu->R8.A & 0x01) != 0;
                cpu->R8.A = (cpu->R8.A >> 1) | (cpu->F.C << 7);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x10: //STOP - 1*
                GB_CPUClockCounterAdd(4);
                if(GB_MemRead8(cpu->R16.PC++) != 0)
                    Debug_DebugMsgArg("Corrupted stop.\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);

                if(GameBoy.Emulator.CGBEnabled == 0)
                {
                    GameBoy.Emulator.CPUHalt = 2;
                }
                else //Switch to double speed mode (CGB)
                {
                    if(mem->IO_Ports[KEY1_REG-0xFF00]&1)
                    {
                        GameBoy.Emulator.DoubleSpeed ^= 1;
                        mem->IO_Ports[KEY1_REG-0xFF00] = GameBoy.Emulator.DoubleSpeed<<7;
                    }
                    else
                    {
                        GameBoy.Emulator.CPUHalt = 2;
                    }
                }
                break;
            case 0x11: //LD DE,nn - 3
                GB_CPUClockCounterAdd(4);
                cpu->R8.E = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                cpu->R8.D = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x12: //LD (DE),A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.DE,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x13: //INC DE - 2
                cpu->R16.DE = (cpu->R16.DE+1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x14: //INC D - 1
                cpu->R16.AF &= ~(F_SUBTRACT);
                cpu->F.H = ( (cpu->R8.D & 0xF) == 0xF);
                cpu->R8.D ++;
                cpu->F.Z = (cpu->R8.D == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x15: //DEC D - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.D & 0xF) == 0x0);
                cpu->R8.D --;
                cpu->F.Z = (cpu->R8.D == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x16: //LD D,n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.D = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x17: //RLA - 1
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
                u32 temp = cpu->F.C; //old carry flag
                cpu->F.C = (cpu->R8.A & 0x80) != 0;
                cpu->R8.A = (cpu->R8.A << 1) | temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x18: //JR n - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = (cpu->R16.PC + (s8)temp) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x19: //ADD HL,DE - 2
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R16.HL + cpu->R16.DE;
                cpu->F.C = ( temp > 0xFFFF );
                cpu->F.H = ( ( (cpu->R16.HL & 0xFFF) + (cpu->R16.DE & 0xFFF) ) > 0xFFF );
                cpu->R16.HL = temp & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            }
            case 0x1A: //LD A,(DE) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.DE);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x1B: //DEC DE - 2
                cpu->R16.DE = (cpu->R16.DE-1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x1C: //INC E - 1
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.E & 0xF) == 0xF);
                cpu->R8.E ++;
                cpu->F.Z = (cpu->R8.E == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x1D: //DEC E - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.E & 0xF) == 0x0);
                cpu->R8.E --;
                cpu->F.Z = (cpu->R8.E == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x1E: //LD E,n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.E = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x1F: //RRA - 1
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
                u32 temp = cpu->F.C; //old carry flag
                cpu->F.C = cpu->R8.A & 0x01;
                cpu->R8.A = (cpu->R8.A >> 1) | (temp << 7);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x20: //JR NZ,n - 3/2
                if(cpu->F.Z == 0)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    cpu->R16.PC = (cpu->R16.PC + (s8)temp) & 0xFFFF;
                    GB_CPUClockCounterAdd(8);
                }
                else
                {
                    cpu->R16.PC ++;
                    GB_CPUClockCounterAdd(8);
                }
                break;
            case 0x21: //LD HL,nn - 3
                GB_CPUClockCounterAdd(4);
                cpu->R8.L = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                cpu->R8.H = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x22: //LD (HL+),A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.A);
                cpu->R16.HL = (cpu->R16.HL+1) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x23: //INC HL - 2
                cpu->R16.HL = (cpu->R16.HL+1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x24: //INC H - 1
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.H & 0xF) == 0xF);
                cpu->R8.H ++;
                cpu->F.Z = (cpu->R8.H == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x25: //DEC H - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.H & 0xF) == 0x0);
                cpu->R8.H --;
                cpu->F.Z = (cpu->R8.H == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x26: //LD H,n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.H = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x27: //DAA - 1
            {
                u32 temp = ( ((u32)cpu->R8.A)<<(3+1) ) | ((((u32)cpu->R8.F>>4)&7)<<1);
                cpu->R8.A = gb_daa_table[temp];
                cpu->R8.F = gb_daa_table[temp+1];
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x28: //JR Z,n - 3/2
                if(cpu->F.Z)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    cpu->R16.PC = (cpu->R16.PC + (s8)temp) & 0xFFFF;
                    GB_CPUClockCounterAdd(8);
                }
                else
                {
                    cpu->R16.PC ++;
                    GB_CPUClockCounterAdd(8);
                }
                break;
            case 0x29: //ADD HL,HL - 2
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.C = (cpu->R16.HL & 0x8000) != 0;
                cpu->F.H = (cpu->R16.HL & 0x0800) != 0;
                cpu->R16.HL = (cpu->R16.HL << 1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x2A: //LD A,(HL+) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.HL);
                cpu->R16.HL = (cpu->R16.HL+1) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x2B: //DEC HL - 2
                cpu->R16.HL = (cpu->R16.HL-1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x2C: //INC L - 1
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.L & 0xF) == 0xF);
                cpu->R8.L ++;
                cpu->F.Z = (cpu->R8.L == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x2D: //DEC L - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.L & 0xF) == 0x0);
                cpu->R8.L --;
                cpu->F.Z = (cpu->R8.L == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x2E: //LD L,n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.L = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x2F: //CPL - 1
                cpu->R16.AF |= (F_SUBTRACT|F_HALFCARRY);
                cpu->R8.A = ~cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x30: //JR NC,n - 3/2
                if(cpu->F.C == 0)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    cpu->R16.PC = (cpu->R16.PC + (s8)temp) & 0xFFFF;
                    GB_CPUClockCounterAdd(8);
                }
                else
                {
                    cpu->R16.PC ++;
                    GB_CPUClockCounterAdd(8);
                }
                break;
            case 0x31: //LD SP,nn - 3
                GB_CPUClockCounterAdd(4);
                cpu->R8.SPL = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                cpu->R8.SPH = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x32: //LD (HL-),A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.A);
                cpu->R16.HL = (cpu->R16.HL-1) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x33: //INC SP - 2
                cpu->R16.SP = (cpu->R16.SP+1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x34: //INC (HL) - 3
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
            case 0x35: //DEC (HL) - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (temp & 0xF) == 0x0);
                temp  = (temp - 1) & 0xFF;
                cpu->F.Z = (temp == 0);
                GB_MemWrite8(cpu->R16.HL,temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x36: //LD (HL),n - 3
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
            case 0x38: //JR C,n - 3/2
                if(cpu->F.C == 1)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    cpu->R16.PC = (cpu->R16.PC + (s8)temp) & 0xFFFF;
                    GB_CPUClockCounterAdd(8);
                }
                else
                {
                    cpu->R16.PC ++;
                    GB_CPUClockCounterAdd(8);
                }
                break;
            case 0x39: //ADD HL,SP - 2
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R16.HL + cpu->R16.SP;
                cpu->F.C = (temp > 0xFFFF);
                cpu->F.H = ( ( (cpu->R16.HL & 0x0FFF) + (cpu->R16.SP & 0x0FFF) ) > 0x0FFF );
                cpu->R16.HL = temp & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            }
            case 0x3A: //LD A,(HL-) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.HL);
                cpu->R16.HL = (cpu->R16.HL-1) & 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x3B: //DEC SP - 2
                cpu->R16.SP = (cpu->R16.SP-1) & 0xFFFF;
                GB_CPUClockCounterAdd(8);
                break;
            case 0x3C: //INC A - 1
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A & 0xF) == 0xF);
                cpu->R8.A ++;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x3D: //DEC A - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A & 0xF) == 0x0);
                cpu->R8.A --;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x3E: //LD A,n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x3F: //CCF - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                cpu->F.C = !cpu->F.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x40: //LD B,B - 1
                //cpu->R8.B = cpu->R8.B;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x41: //LD B,C - 1
                cpu->R8.B = cpu->R8.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x42: //LD B,D - 1
                cpu->R8.B = cpu->R8.D;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x43: //LD B,E - 1
                cpu->R8.B = cpu->R8.E;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x44: //LD B,H - 1
                cpu->R8.B = cpu->R8.H;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x45: //LD B,L - 1
                cpu->R8.B = cpu->R8.L;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x46: //LD B,(HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.B = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x47: //LD B,A - 1
                cpu->R8.B = cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x48: //LD C,B - 1
                cpu->R8.C = cpu->R8.B;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x49: //LD C,C - 1
                //cpu->R8.C = cpu->R8.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x4A: //LD C,D - 1
                cpu->R8.C = cpu->R8.D;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x4B: //LD C,E - 1
                cpu->R8.C = cpu->R8.E;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x4C: //LD C,H - 1
                cpu->R8.C = cpu->R8.H;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x4D: //LD C,L - 1
                cpu->R8.C = cpu->R8.L;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x4E: //LD C,(HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.C = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x4F: //LD C,A - 1
                cpu->R8.C = cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x50: //LD D,B - 1
                cpu->R8.D = cpu->R8.B;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x51: //LD D,C - 1
                cpu->R8.D = cpu->R8.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x52: //LD D,D - 1
                //cpu->R8.D = cpu->R8.D;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x53: //LD D,E - 1
                cpu->R8.D = cpu->R8.E;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x54: //LD D,H - 1
                cpu->R8.D = cpu->R8.H;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x55: //LD D,L - 1
                cpu->R8.D = cpu->R8.L;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x56: //LD D,(HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.D = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x57: //LD D,A - 1
                cpu->R8.D = cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x58: //LD E,B - 1
                cpu->R8.E = cpu->R8.B;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x59: //LD E,C - 1
                cpu->R8.E = cpu->R8.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x5A: //LD E,D - 1
                cpu->R8.E = cpu->R8.D;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x5B: //LD E,E - 1
                //cpu->R8.E = cpu->R8.E;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x5C: //LD E,H - 1
                cpu->R8.E = cpu->R8.H;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x5D: //LD E,L - 1
                cpu->R8.E = cpu->R8.L;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x5E: //LD E,(HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.E = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x5F: //LD E,A - 1
                cpu->R8.E = cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x60: //LD H,B - 1
                cpu->R8.H = cpu->R8.B;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x61: //LD H,C - 1
                cpu->R8.H = cpu->R8.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x62: //LD H,D - 1
                cpu->R8.H = cpu->R8.D;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x63: //LD H,E - 1
                cpu->R8.H = cpu->R8.E;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x64: //LD H,H - 1
                //cpu->R8.H = cpu->R8.H;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x65: //LD H,L - 1
                cpu->R8.H = cpu->R8.L;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x66: //LD H,(HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.H = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x67: //LD H,A - 1
                cpu->R8.H = cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x68: //LD L,B - 1
                cpu->R8.L = cpu->R8.B;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x69: //LD L,C - 1
                cpu->R8.L = cpu->R8.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x6A: //LD L,D - 1
                cpu->R8.L = cpu->R8.D;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x6B: //LD L,E - 1
                cpu->R8.L = cpu->R8.E;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x6C: //LD L,H - 1
                cpu->R8.L = cpu->R8.H;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x6D: //LD L,L - 1
                //cpu->R8.L = cpu->R8.L;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x6E: //LD L,(HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.L = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x6F: //LD L,A - 1
                cpu->R8.L = cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x70: //LD (HL),B - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.B);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x71: //LD (HL),C - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.C);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x72: //LD (HL),D - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.D);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x73: //LD (HL),E - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.E);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x74: //LD (HL),H - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.H);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x75: //LD (HL),L - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.L);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x76: //HALT - 1*
                GB_CPUClockCounterAdd(4);
                if(GameBoy.Memory.InterruptMasterEnable == 1)
                {
                //    if(GameBoy.Emulator.HDMAenabled == HDMA_NONE)
                //    {
                        GameBoy.Emulator.CPUHalt = 1;
                //    }
                }
                else
                {
                    if( ! ( (GameBoy.Emulator.HardwareType == HW_GBC) || (GameBoy.Emulator.HardwareType == HW_GBA) ) )
                    //if(GameBoy.Emulator.CGBEnabled == 0)
                    {
                        if(mem->IO_Ports[IF_REG-0xFF00] & mem->HighRAM[IE_REG-0xFF80] & 0x1F)
                        {
                            GameBoy.Emulator.halt_dmg_bug = 1;
                        }
                        else
                        {
                            //Nothing
                            //GameBoy.Emulator.halt_not_executed = 1;
                        }
                    }
                    else
                    {
                        //GameBoy.Emulator.halt_not_executed = 1;
                    }
                }
                break;
            case 0x77: //LD (HL),A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(cpu->R16.HL,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x78: //LD A,B - 1
                cpu->R8.A = cpu->R8.B;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x79: //LD A,C - 1
                cpu->R8.A = cpu->R8.C;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x7A: //LD A,D - 1
                cpu->R8.A = cpu->R8.D;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x7B: //LD A,E - 1
                cpu->R8.A = cpu->R8.E;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x7C: //LD A,H - 1
                cpu->R8.A = cpu->R8.H;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x7D: //LD A,L - 1
                cpu->R8.A = cpu->R8.L;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x7E: //LD A,(HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.HL);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x7F: //LD A,A - 1
                //cpu->R8.A = cpu->R8.A;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x80: //ADD A,B - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A;
                cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.B & 0xF) ) > 0xF;
                cpu->R8.A += cpu->R8.B;
                cpu->F.Z = (cpu->R8.A == 0);
                cpu->F.C = (temp > cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x81: //ADD A,C - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A;
                cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.C & 0xF) ) > 0xF;
                cpu->R8.A += cpu->R8.C;
                cpu->F.Z = (cpu->R8.A == 0);
                cpu->F.C = (temp > cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x82: //ADD A,D - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A;
                cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.D & 0xF) ) > 0xF;
                cpu->R8.A += cpu->R8.D;
                cpu->F.Z = (cpu->R8.A == 0);
                cpu->F.C = (temp > cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x83: //ADD A,E - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A;
                cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.E & 0xF) ) > 0xF;
                cpu->R8.A += cpu->R8.E;
                cpu->F.Z = (cpu->R8.A == 0);
                cpu->F.C = (temp > cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x84: //ADD A,H - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A;
                cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.H & 0xF) ) > 0xF;
                cpu->R8.A += cpu->R8.H;
                cpu->F.Z = (cpu->R8.A == 0);
                cpu->F.C = (temp > cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x85: //ADD A,L - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A;
                cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.L & 0xF) ) > 0xF;
                cpu->R8.A += cpu->R8.L;
                cpu->F.Z = (cpu->R8.A == 0);
                cpu->F.C = (temp > cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x86: //ADD A,(HL) - 2
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
            case 0x88: //ADC A,B - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A + cpu->R8.B + cpu->F.C;
                cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.B & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp > 0xFF);
                temp &= 0xFF;
                cpu->R8.A = temp;
                cpu->F.Z = (temp == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x89: //ADC A,C - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A + cpu->R8.C + cpu->F.C;
                cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.C & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp > 0xFF);
                temp &= 0xFF;
                cpu->R8.A = temp;
                cpu->F.Z = (temp == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x8A: //ADC A,D - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A + cpu->R8.D + cpu->F.C;
                cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.D & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp > 0xFF);
                temp &= 0xFF;
                cpu->R8.A = temp;
                cpu->F.Z = (temp == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x8B: //ADC A,E - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A + cpu->R8.E + cpu->F.C;
                cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.E & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp > 0xFF);
                temp &= 0xFF;
                cpu->R8.A = temp;
                cpu->F.Z = (temp == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x8C: //ADC A,H - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A + cpu->R8.H + cpu->F.C;
                cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.H & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp > 0xFF);
                temp &= 0xFF;
                cpu->R8.A = temp;
                cpu->F.Z = (temp == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x8D: //ADC A,L - 1
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R8.A + cpu->R8.L + cpu->F.C;
                cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.L & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp > 0xFF);
                temp &= 0xFF;
                cpu->R8.A = temp;
                cpu->F.Z = (temp == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x8E: //ADC A,(HL) - 2
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
            case 0x90: //SUB B - 1
                cpu->R8.F = F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.B & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.B;
                cpu->R8.A -= cpu->R8.B;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x91: //SUB C - 1
                cpu->R8.F = F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.C & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.C;
                cpu->R8.A -= cpu->R8.C;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x92: //SUB D - 1
                cpu->R8.F = F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.D & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.D;
                cpu->R8.A -= cpu->R8.D;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x93: //SUB E - 1
                cpu->R8.F = F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.E & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.E;
                cpu->R8.A -= cpu->R8.E;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x94: //SUB H - 1
                cpu->R8.F = F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.H & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.H;
                cpu->R8.A -= cpu->R8.H;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x95: //SUB L - 1
                cpu->R8.F = F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.L & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.L;
                cpu->R8.A -= cpu->R8.L;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0x96: //SUB (HL) - 2
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
            case 0x97: //SUB A - 1
                cpu->R8.F = F_SUBTRACT|F_ZERO;
                cpu->R8.A = 0;
                GB_CPUClockCounterAdd(4);
                break;
            case 0x98: //SBC A,B - 1
            {
                u32 temp = cpu->R8.A - cpu->R8.B - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^cpu->R8.B^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x99: //SBC A,C - 1
            {
                u32 temp = cpu->R8.A - cpu->R8.C - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^cpu->R8.C^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x9A: //SBC A,D - 1
            {
                u32 temp = cpu->R8.A - cpu->R8.D - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^cpu->R8.D^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x9B: //SBC A,E - 1
            {
                u32 temp = cpu->R8.A - cpu->R8.E - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^cpu->R8.E^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x9C: //SBC A,H - 1
            {
                u32 temp = cpu->R8.A - cpu->R8.H - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^cpu->R8.H^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x9D: //SBC A,L - 1
            {
                u32 temp = cpu->R8.A - cpu->R8.L - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^cpu->R8.L^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0x9E: //SBC A,(HL) - 2
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
                cpu->R16.AF = cpu->R8.F&F_CARRY ?
                        ( (0xFF<<8)|F_CARRY|F_HALFCARRY|F_SUBTRACT ) : (F_ZERO|F_SUBTRACT) ;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA0: //AND B - 1
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= cpu->R8.B;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA1: //AND C - 1
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= cpu->R8.C;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA2: //AND D - 1
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= cpu->R8.D;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA3: //AND E - 1
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= cpu->R8.E;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA4: //AND H - 1
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= cpu->R8.H;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA5: //AND L - 1
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= cpu->R8.L;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA6: //AND (HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= GB_MemRead8(cpu->R16.HL);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA7: //AND A - 1
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                //cpu->R8.A &= cpu->R8.A;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA8: //XOR B - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= cpu->R8.B;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xA9: //XOR C - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= cpu->R8.C;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xAA: //XOR D - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= cpu->R8.D;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xAB: //XOR E - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= cpu->R8.E;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xAC: //XOR H - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= cpu->R8.H;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xAD: //XOR L - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= cpu->R8.L;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xAE: //XOR (HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= GB_MemRead8(cpu->R16.HL);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xAF: //XOR A - 1
                cpu->R16.AF = F_ZERO;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB0: //OR B - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= cpu->R8.B;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB1: //OR C - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= cpu->R8.C;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB2: //OR D - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= cpu->R8.D;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB3: //OR E - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= cpu->R8.E;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB4: //OR H - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= cpu->R8.H;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB5: //OR L - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= cpu->R8.L;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB6: //OR (HL) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= GB_MemRead8(cpu->R16.HL);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB7: //OR A - 1
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                //cpu->R8.A |= cpu->R8.A;
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB8: //CP B - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.B & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.B;
                cpu->F.Z = (cpu->R8.A == cpu->R8.B);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xB9: //CP C - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.C & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.C;
                cpu->F.Z = (cpu->R8.A == cpu->R8.C);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xBA: //CP D - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.D & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.D;
                cpu->F.Z = (cpu->R8.A == cpu->R8.D);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xBB: //CP E - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.E & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.E;
                cpu->F.Z = (cpu->R8.A == cpu->R8.E);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xBC: //CP H - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.H & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.H;
                cpu->F.Z = (cpu->R8.A == cpu->R8.H);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xBD: //CP L - 1
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.L & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.L;
                cpu->F.Z = (cpu->R8.A == cpu->R8.L);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xBE: //CP (HL) - 2
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
            case 0xBF: //CP A - 1
                cpu->R16.AF |= (F_SUBTRACT|F_ZERO);
                cpu->R16.AF &= ~(F_HALFCARRY|F_CARRY);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xC0: //RET NZ - 5/2
                if(cpu->F.Z == 0)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.SP++);
                    cpu->R16.SP &= 0xFFFF;
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                    cpu->R16.SP &= 0xFFFF;
                    GB_CPUClockCounterAdd(4);
                    cpu->R16.PC = temp;
                    GB_CPUClockCounterAdd(8);
                }
                else
                {
                    GB_CPUClockCounterAdd(8);
                }
                break;
            case 0xC1: //POP BC - 3
                GB_CPUClockCounterAdd(4);
                cpu->R8.C = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R8.B = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xC2: //JP NZ,nn - 4/3
                if(cpu->F.Z == 0)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                    GB_CPUClockCounterAdd(4);
                    cpu->R16.PC = temp;
                    GB_CPUClockCounterAdd(4);
                }
                else
                {
                    cpu->R16.PC += 2;
                    cpu->R16.PC &= 0xFFFF;
                    GB_CPUClockCounterAdd(12);
                }
                break;
            case 0xC3: //JP nn - 4
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
            case 0xC4: //CALL NZ,nn - 6/3
                if(cpu->F.Z == 0)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
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
                }
                else
                {
                    cpu->R16.PC += 2;
                    cpu->R16.PC &= 0xFFFF;
                    GB_CPUClockCounterAdd(12);
                }
                break;
            case 0xC5: //PUSH BC - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.B);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.C);
                GB_CPUClockCounterAdd(8);
                break;
            case 0xC6: //ADD A,n - 2
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
            case 0xC7: //RST 0x00 - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = 0x0000;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xC8: //RET Z - 5/2
                if(cpu->F.Z)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.SP++);
                    cpu->R16.SP &= 0xFFFF;
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                    cpu->R16.SP &= 0xFFFF;
                    GB_CPUClockCounterAdd(4);
                    cpu->R16.PC = temp;
                    GB_CPUClockCounterAdd(8);
                }
                else
                {
                    GB_CPUClockCounterAdd(8);
                }
                break;
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
            case 0xCA: //JP Z,nn - 4/3
                if(cpu->F.Z)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                    GB_CPUClockCounterAdd(4);
                    cpu->R16.PC = temp;
                    GB_CPUClockCounterAdd(4);
                }
                else
                {
                    cpu->R16.PC += 2;
                    cpu->R16.PC &= 0xFFFF;
                    GB_CPUClockCounterAdd(12);
                }
                break;
            case 0xCB:
                GB_CPUClockCounterAdd(4);
                opcode = (u32)(u8)GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;

                switch(opcode)
                {
                    case 0x00: //RLC B - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.B & 0x80) != 0;
                        cpu->R8.B = (cpu->R8.B << 1) | cpu->F.C;
                        cpu->F.Z = (cpu->R8.B == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x01: //RLC C - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.C & 0x80) != 0;
                        cpu->R8.C = (cpu->R8.C << 1) | cpu->F.C;
                        cpu->F.Z = (cpu->R8.C == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x02: //RLC D - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.D & 0x80) != 0;
                        cpu->R8.D = (cpu->R8.D << 1) | cpu->F.C;
                        cpu->F.Z = (cpu->R8.D == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x03: //RLC E - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.E & 0x80) != 0;
                        cpu->R8.E = (cpu->R8.E << 1) | cpu->F.C;
                        cpu->F.Z = (cpu->R8.E == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x04: //RLC H - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.H & 0x80) != 0;
                        cpu->R8.H = (cpu->R8.H << 1) | cpu->F.C;
                        cpu->F.Z = (cpu->R8.H == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x05: //RLC L - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.L & 0x80) != 0;
                        cpu->R8.L = (cpu->R8.L << 1) | cpu->F.C;
                        cpu->F.Z = (cpu->R8.L == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x06: //RLC (HL) - 4
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
                    case 0x07: //RLC A - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.A & 0x80) != 0;
                        cpu->R8.A = (cpu->R8.A << 1) | cpu->F.C;
                        cpu->F.Z = (cpu->R8.A == 0);
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0x08: //RRC B - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.B & 0x01) != 0;
                        cpu->R8.B = (cpu->R8.B >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (cpu->R8.B == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x09: //RRC C - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.C & 0x01) != 0;
                        cpu->R8.C = (cpu->R8.C >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (cpu->R8.C == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x0A: //RRC D - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.D & 0x01) != 0;
                        cpu->R8.D = (cpu->R8.D >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (cpu->R8.D == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x0B: //RRC E - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.E & 0x01) != 0;
                        cpu->R8.E = (cpu->R8.E >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (cpu->R8.E == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x0C: //RRC H - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.H & 0x01) != 0;
                        cpu->R8.H = (cpu->R8.H >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (cpu->R8.H == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x0D: //RRC L - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.L & 0x01) != 0;
                        cpu->R8.L = (cpu->R8.L >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (cpu->R8.L == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x0E: //RRC (HL) - 4
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
                    case 0x0F: //RRC A - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.A & 0x01) != 0;
                        cpu->R8.A = (cpu->R8.A >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (cpu->R8.A == 0);
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0x10: //RL B - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.B & 0x80) != 0;
                        cpu->R8.B = (cpu->R8.B << 1) | temp;
                        cpu->F.Z = (cpu->R8.B == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x11: //RL C - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.C & 0x80) != 0;
                        cpu->R8.C = (cpu->R8.C << 1) | temp;
                        cpu->F.Z = (cpu->R8.C == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x12: //RL D - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.D & 0x80) != 0;
                        cpu->R8.D = (cpu->R8.D << 1) | temp;
                        cpu->F.Z = (cpu->R8.D == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x13: //RL E - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.E & 0x80) != 0;
                        cpu->R8.E = (cpu->R8.E << 1) | temp;
                        cpu->F.Z = (cpu->R8.E == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x14: //RL H - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.H & 0x80) != 0;
                        cpu->R8.H = (cpu->R8.H << 1) | temp;
                        cpu->F.Z = (cpu->R8.H == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x15: //RL L - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.L & 0x80) != 0;
                        cpu->R8.L = (cpu->R8.L << 1) | temp;
                        cpu->F.Z = (cpu->R8.L == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x16: //RL (HL) - 4
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
                    case 0x17: //RL A - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.A & 0x80) != 0;
                        cpu->R8.A = (cpu->R8.A << 1) | temp;
                        cpu->F.Z = (cpu->R8.A == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }

                    case 0x18: //RR B - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.B & 0x01) != 0;
                        cpu->R8.B = (cpu->R8.B >> 1) | (temp << 7);
                        cpu->F.Z = (cpu->R8.B == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x19: //RR C - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.C & 0x01) != 0;
                        cpu->R8.C = (cpu->R8.C >> 1) | (temp << 7);
                        cpu->F.Z = (cpu->R8.C == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x1A: //RR D - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.D & 0x01) != 0;
                        cpu->R8.D = (cpu->R8.D >> 1) | (temp << 7);
                        cpu->F.Z = (cpu->R8.D == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x1B: //RR E - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.E & 0x01) != 0;
                        cpu->R8.E = (cpu->R8.E >> 1) | (temp << 7);
                        cpu->F.Z = (cpu->R8.E == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x1C: //RR H - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.H & 0x01) != 0;
                        cpu->R8.H = (cpu->R8.H >> 1) | (temp << 7);
                        cpu->F.Z = (cpu->R8.H == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x1D: //RR L - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.L & 0x01) != 0;
                        cpu->R8.L = (cpu->R8.L >> 1) | (temp << 7);
                        cpu->F.Z = (cpu->R8.L == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x1E: //RR (HL) - 4
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
                    case 0x1F: //RR A - 2
                    {
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (cpu->R8.A & 0x01) != 0;
                        cpu->R8.A = (cpu->R8.A >> 1) | (temp << 7);
                        cpu->F.Z = (cpu->R8.A == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    }

                    case 0x20: //SLA B - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.B & 0x80) != 0;
                        cpu->R8.B = cpu->R8.B << 1;
                        cpu->F.Z = (cpu->R8.B == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x21: //SLA C - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.C & 0x80) != 0;
                        cpu->R8.C = cpu->R8.C << 1;
                        cpu->F.Z = (cpu->R8.C == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x22: //SLA D - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.D & 0x80) != 0;
                        cpu->R8.D = cpu->R8.D << 1;
                        cpu->F.Z = (cpu->R8.D == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x23: //SLA E - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.E & 0x80) != 0;
                        cpu->R8.E = cpu->R8.E << 1;
                        cpu->F.Z = (cpu->R8.E == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x24: //SLA H - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.H & 0x80) != 0;
                        cpu->R8.H = cpu->R8.H << 1;
                        cpu->F.Z = (cpu->R8.H == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x25: //SLA L - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.L & 0x80) != 0;
                        cpu->R8.L = cpu->R8.L << 1;
                        cpu->F.Z = (cpu->R8.L == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x26: //SLA (HL) - 4
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
                    case 0x27: //SLA A - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.A & 0x80) != 0;
                        cpu->R8.A = cpu->R8.A << 1;
                        cpu->F.Z = (cpu->R8.A == 0);
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0x28: //SRA B - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.B & 0x01) != 0;
                        cpu->R8.B = (cpu->R8.B & 0x80) | (cpu->R8.B >> 1);
                        cpu->F.Z = (cpu->R8.B == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x29: //SRA C - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.C & 0x01) != 0;
                        cpu->R8.C = (cpu->R8.C & 0x80) | (cpu->R8.C >> 1);
                        cpu->F.Z = (cpu->R8.C == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x2A: //SRA D - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.D & 0x01) != 0;
                        cpu->R8.D = (cpu->R8.D & 0x80) | (cpu->R8.D >> 1);
                        cpu->F.Z = (cpu->R8.D == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x2B: //SRA E - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.E & 0x01) != 0;
                        cpu->R8.E = (cpu->R8.E & 0x80) | (cpu->R8.E >> 1);
                        cpu->F.Z = (cpu->R8.E == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x2C: //SRA H - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.H & 0x01) != 0;
                        cpu->R8.H = (cpu->R8.H & 0x80) | (cpu->R8.H >> 1);
                        cpu->F.Z = (cpu->R8.H == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x2D: //SRA L - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.L & 0x01) != 0;
                        cpu->R8.L = (cpu->R8.L & 0x80) | (cpu->R8.L >> 1);
                        cpu->F.Z = (cpu->R8.L == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x2E: //SRA (HL) - 4
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
                    case 0x2F: //SRA A - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.A & 0x01) != 0;
                        cpu->R8.A = (cpu->R8.A & 0x80) | (cpu->R8.A >> 1);
                        cpu->F.Z = (cpu->R8.A == 0);
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0x30: //SWAP B - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        cpu->R8.B = ((cpu->R8.B >> 4) | (cpu->R8.B << 4));
                        cpu->F.Z = (cpu->R8.B == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x31: //SWAP C - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        cpu->R8.C = ((cpu->R8.C >> 4) | (cpu->R8.C << 4));
                        cpu->F.Z = (cpu->R8.C == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x32: //SWAP D - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        cpu->R8.D = ((cpu->R8.D >> 4) | (cpu->R8.D << 4));
                        cpu->F.Z = (cpu->R8.D == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x33: //SWAP E - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        cpu->R8.E = ((cpu->R8.E >> 4) | (cpu->R8.E << 4));
                        cpu->F.Z = (cpu->R8.E == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x34: //SWAP H - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        cpu->R8.H = ((cpu->R8.H >> 4) | (cpu->R8.H << 4));
                        cpu->F.Z = (cpu->R8.H == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x35: //SWAP L - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        cpu->R8.L = ((cpu->R8.L >> 4) | (cpu->R8.L << 4));
                        cpu->F.Z = (cpu->R8.L == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x36: //SWAP (HL) - 4
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
                    case 0x37: //SWAP A - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        cpu->R8.A = ((cpu->R8.A & 0xF0) >> 4) | ((cpu->R8.A & 0x0F) << 4);
                        cpu->F.Z = (cpu->R8.A == 0);
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0x38: //SRL B - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.B & 0x01) != 0;
                        cpu->R8.B = cpu->R8.B >> 1;
                        cpu->F.Z = (cpu->R8.B == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x39: //SRL C - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.C & 0x01) != 0;
                        cpu->R8.C = cpu->R8.C >> 1;
                        cpu->F.Z = (cpu->R8.C == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x3A: //SRL D - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.D & 0x01) != 0;
                        cpu->R8.D = cpu->R8.D >> 1;
                        cpu->F.Z = (cpu->R8.D == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x3B: //SRL E - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.E & 0x01) != 0;
                        cpu->R8.E = cpu->R8.E >> 1;
                        cpu->F.Z = (cpu->R8.E == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x3C: //SRL H - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.H & 0x01) != 0;
                        cpu->R8.H = cpu->R8.H >> 1;
                        cpu->F.Z = (cpu->R8.H == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x3D: //SRL L - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.L & 0x01) != 0;
                        cpu->R8.L = cpu->R8.L >> 1;
                        cpu->F.Z = (cpu->R8.L == 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x3E: //SRL (HL) - 4
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
                    case 0x3F: //SRL A - 2
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (cpu->R8.A & 0x01) != 0;
                        cpu->R8.A = cpu->R8.A >> 1;
                        cpu->F.Z = (cpu->R8.A == 0);
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0x40: //BIT 0,B - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.B & (1<<0)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x41: //BIT 0,C - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.C & (1<<0)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x42: //BIT 0,D - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.D & (1<<0)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x43: //BIT 0,E - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.E & (1<<0)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x44: //BIT 0,H - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.H & (1<<0)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x45: //BIT 0,L - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.L & (1<<0)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x46: //BIT 0,(HL) - 3
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<0)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x47: //BIT 0,A - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.A & (1<<0)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0x48: //BIT 1,B - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.B & (1<<1)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x49: //BIT 1,C - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.C & (1<<1)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x4A: //BIT 1,D - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.D & (1<<1)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x4B: //BIT 1,E - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.E & (1<<1)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x4C: //BIT 1,H - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.H & (1<<1)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x4D: //BIT 1,L - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.L & (1<<1)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x4E: //BIT 1,(HL) - 3
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<1)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x4F: //BIT 1,A - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.A & (1<<1)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x50: //BIT 2,B - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.B & (1<<2)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x51: //BIT 2,C - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.C & (1<<2)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x52: //BIT 2,D - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.D & (1<<2)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x53: //BIT 2,E - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.E & (1<<2)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x54: //BIT 2,H - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.H & (1<<2)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x55: //BIT 2,L - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.L & (1<<2)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x56: //BIT 2,(HL) - 3
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<2)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x57: //BIT 2,A - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.A & (1<<2)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x58: //BIT 3,B - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.B & (1<<3)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x59: //BIT 3,C - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.C & (1<<3)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x5A: //BIT 3,D - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.D & (1<<3)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x5B: //BIT 3,E - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.E & (1<<3)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x5C: //BIT 3,H - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.H & (1<<3)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x5D: //BIT 3,L - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.L & (1<<3)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x5E: //BIT 3,(HL) - 3
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<3)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x5F: //BIT 3,A - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.A & (1<<3)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x60: //BIT 4,B - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.B & (1<<4)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x61: //BIT 4,C - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.C & (1<<4)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x62: //BIT 4,D - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.D & (1<<4)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x63: //BIT 4,E - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.E & (1<<4)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x64: //BIT 4,H - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.H & (1<<4)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x65: //BIT 4,L - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.L & (1<<4)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x66: //BIT 4,(HL) - 3
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<4)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x67: //BIT 4,A - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.A & (1<<4)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x68: //BIT 5,B - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.B & (1<<5)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x69: //BIT 5,C - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.C & (1<<5)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x6A: //BIT 5,D - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.D & (1<<5)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x6B: //BIT 5,E - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.E & (1<<5)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x6C: //BIT 5,H - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.H & (1<<5)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x6D: //BIT 5,L - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.L & (1<<5)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x6E: //BIT 5,(HL) - 3
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<5)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x6F: //BIT 5,A - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.A & (1<<5)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x70: //BIT 6,B - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.B & (1<<6)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x71: //BIT 6,C - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.C & (1<<6)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x72: //BIT 6,D - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.D & (1<<6)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x73: //BIT 6,E - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.E & (1<<6)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x74: //BIT 6,H - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.H & (1<<6)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x75: //BIT 6,L - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.L & (1<<6)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x76: //BIT 6,(HL) - 3
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<6)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x77: //BIT 6,A - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.A & (1<<6)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x78: //BIT 7,B - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.B & (1<<7)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x79: //BIT 7,C - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.C & (1<<7)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x7A: //BIT 7,D - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.D & (1<<7)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x7B: //BIT 7,E - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.E & (1<<7)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x7C: //BIT 7,H - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.H & (1<<7)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x7D: //BIT 7,L - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.L & (1<<7)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x7E: //BIT 7,(HL) - 3
                        GB_CPUClockCounterAdd(4);
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<7)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x7F: //BIT 7,A - 2
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (cpu->R8.A & (1<<7)) == 0;
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0x80: //RES 0,B - 2
                        cpu->R8.B &= ~(1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x81: //RES 0,C - 2
                        cpu->R8.C &= ~(1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x82: //RES 0,D - 2
                        cpu->R8.D &= ~(1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x83: //RES 0,E - 2
                        cpu->R8.E &= ~(1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x84: //RES 0,H - 2
                        cpu->R8.H &= ~(1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x85: //RES 0,L - 2
                        cpu->R8.L &= ~(1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x86: //RES 0,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp & (~(1 << 0)));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x87: //RES 0,A - 2
                        cpu->R8.A &= ~(1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x88: //RES 1,B - 2
                        cpu->R8.B &= ~(1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x89: //RES 1,C - 2
                        cpu->R8.C &= ~(1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x8A: //RES 1,D - 2
                        cpu->R8.D &= ~(1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x8B: //RES 1,E - 2
                        cpu->R8.E &= ~(1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x8C: //RES 1,H - 2
                        cpu->R8.H &= ~(1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x8D: //RES 1,L - 2
                        cpu->R8.L &= ~(1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x8E: //RES 1,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp & (~(1 << 1)));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x8F: //RES 1,A - 2
                        cpu->R8.A &= ~(1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x90: //RES 2,B - 2
                        cpu->R8.B &= ~(1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x91: //RES 2,C - 2
                        cpu->R8.C &= ~(1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x92: //RES 2,D - 2
                        cpu->R8.D &= ~(1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x93: //RES 2,E - 2
                        cpu->R8.E &= ~(1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x94: //RES 2,H - 2
                        cpu->R8.H &= ~(1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x95: //RES 2,L - 2
                        cpu->R8.L &= ~(1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x96: //RES 2,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp & (~(1 << 2)));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x97: //RES 2,A - 2
                        cpu->R8.A &= ~(1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x98: //RES 3,B - 2
                        cpu->R8.B &= ~(1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x99: //RES 3,C - 2
                        cpu->R8.C &= ~(1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x9A: //RES 3,D - 2
                        cpu->R8.D &= ~(1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x9B: //RES 3,E - 2
                        cpu->R8.E &= ~(1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x9C: //RES 3,H - 2
                        cpu->R8.H &= ~(1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x9D: //RES 3,L - 2
                        cpu->R8.L &= ~(1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0x9E: //RES 3,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp & (~(1 << 3)));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0x9F: //RES 3,A - 2
                        cpu->R8.A &= ~(1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA0: //RES 4,B - 2
                        cpu->R8.B &= ~(1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA1: //RES 4,C - 2
                        cpu->R8.C &= ~(1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA2: //RES 4,D - 2
                        cpu->R8.D &= ~(1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA3: //RES 4,E - 2
                        cpu->R8.E &= ~(1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA4: //RES 4,H - 2
                        cpu->R8.H &= ~(1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA5: //RES 4,L - 2
                        cpu->R8.L &= ~(1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA6: //RES 4,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp & (~(1 << 4)));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xA7: //RES 4,A - 2
                        cpu->R8.A &= ~(1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA8: //RES 5,B - 2
                        cpu->R8.B &= ~(1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xA9: //RES 5,C - 2
                        cpu->R8.C &= ~(1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xAA: //RES 5,D - 2
                        cpu->R8.D &= ~(1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xAB: //RES 5,E - 2
                        cpu->R8.E &= ~(1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xAC: //RES 5,H - 2
                        cpu->R8.H &= ~(1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xAD: //RES 5,L - 2
                        cpu->R8.L &= ~(1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xAE: //RES 5,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp & (~(1 << 5)));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xAF: //RES 5,A - 2
                        cpu->R8.A &= ~(1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB0: //RES 6,B - 2
                        cpu->R8.B &= ~(1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB1: //RES 6,C - 2
                        cpu->R8.C &= ~(1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB2: //RES 6,D - 2
                        cpu->R8.D &= ~(1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB3: //RES 6,E - 2
                        cpu->R8.E &= ~(1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB4: //RES 6,H - 2
                        cpu->R8.H &= ~(1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB5: //RES 6,L - 2
                        cpu->R8.L &= ~(1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB6: //RES 6,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp & (~(1 << 6)));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xB7: //RES 6,A - 2
                        cpu->R8.A &= ~(1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB8: //RES 7,B - 2
                        cpu->R8.B &= ~(1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xB9: //RES 7,C - 2
                        cpu->R8.C &= ~(1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xBA: //RES 7,D - 2
                        cpu->R8.D &= ~(1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xBB: //RES 7,E - 2
                        cpu->R8.E &= ~(1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xBC: //RES 7,H - 2
                        cpu->R8.H &= ~(1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xBD: //RES 7,L - 2
                        cpu->R8.L &= ~(1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xBE: //RES 7,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp & (~(1 << 7)));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xBF: //RES 7,A - 2
                        cpu->R8.A &= ~(1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;

                    case 0xC0: //SET 0,B - 2
                        cpu->R8.B |= (1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xC1: //SET 0,C - 2
                        cpu->R8.C |= (1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xC2: //SET 0,D - 2
                        cpu->R8.D |= (1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xC3: //SET 0,E - 2
                        cpu->R8.E |= (1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xC4: //SET 0,H - 2
                        cpu->R8.H |= (1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xC5: //SET 0,L - 2
                        cpu->R8.L |= (1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xC6: //SET 0,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp | (1 << 0));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xC7: //SET 0,A - 2
                        cpu->R8.A |= (1 << 0);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xC8: //SET 1,B - 2
                        cpu->R8.B |= (1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xC9: //SET 1,C - 2
                        cpu->R8.C |= (1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xCA: //SET 1,D - 2
                        cpu->R8.D |= (1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xCB: //SET 1,E - 2
                        cpu->R8.E |= (1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xCC: //SET 1,H - 2
                        cpu->R8.H |= (1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xCD: //SET 1,L - 2
                        cpu->R8.L |= (1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xCE: //SET 1,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp | (1 << 1));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xCF: //SET 1,A - 2
                        cpu->R8.A |= (1 << 1);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD0: //SET 2,B - 2
                        cpu->R8.B |= (1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD1: //SET 2,C - 2
                        cpu->R8.C |= (1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD2: //SET 2,D - 2
                        cpu->R8.D |= (1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD3: //SET 2,E - 2
                        cpu->R8.E |= (1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD4: //SET 2,H - 2
                        cpu->R8.H |= (1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD5: //SET 2,L - 2
                        cpu->R8.L |= (1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD6: //SET 2,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp | (1 << 2));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xD7: //SET 2,A - 2
                        cpu->R8.A |= (1 << 2);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD8: //SET 3,B - 2
                        cpu->R8.B |= (1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xD9: //SET 3,C - 2
                        cpu->R8.C |= (1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xDA: //SET 3,D - 2
                        cpu->R8.D |= (1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xDB: //SET 3,E - 2
                        cpu->R8.E |= (1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xDC: //SET 3,H - 2
                        cpu->R8.H |= (1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xDD: //SET 3,L - 2
                        cpu->R8.L |= (1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xDE: //SET 3,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp | (1 << 3));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xDF: //SET 3,A - 2
                        cpu->R8.A |= (1 << 3);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE0: //SET 4,B - 2
                        cpu->R8.B |= (1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE1: //SET 4,C - 2
                        cpu->R8.C |= (1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE2: //SET 4,D - 2
                        cpu->R8.D |= (1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE3: //SET 4,E - 2
                        cpu->R8.E |= (1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE4: //SET 4,H - 2
                        cpu->R8.H |= (1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE5: //SET 4,L - 2
                        cpu->R8.L |= (1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE6: //SET 4,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp | (1 << 4));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xE7: //SET 4,A - 2
                        cpu->R8.A |= (1 << 4);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE8: //SET 5,B - 2
                        cpu->R8.B |= (1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xE9: //SET 5,C - 2
                        cpu->R8.C |= (1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xEA: //SET 5,D - 2
                        cpu->R8.D |= (1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xEB: //SET 5,E - 2
                        cpu->R8.E |= (1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xEC: //SET 5,H - 2
                        cpu->R8.H |= (1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xED: //SET 5,L - 2
                        cpu->R8.L |= (1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xEE: //SET 5,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp | (1 << 5));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xEF: //SET 5,A - 2
                        cpu->R8.A |= (1 << 5);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF0: //SET 6,B - 2
                        cpu->R8.B |= (1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF1: //SET 6,C - 2
                        cpu->R8.C |= (1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF2: //SET 6,D - 2
                        cpu->R8.D |= (1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF3: //SET 6,E - 2
                        cpu->R8.E |= (1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF4: //SET 6,H - 2
                        cpu->R8.H |= (1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF5: //SET 6,L - 2
                        cpu->R8.L |= (1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF6: //SET 6,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp | (1 << 6));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xF7: //SET 6,A - 2
                        cpu->R8.A |= (1 << 6);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF8: //SET 7,B - 2
                        cpu->R8.B |= (1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xF9: //SET 7,C - 2
                        cpu->R8.C |= (1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xFA: //SET 7,D - 2
                        cpu->R8.D |= (1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xFB: //SET 7,E - 2
                        cpu->R8.E |= (1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xFC: //SET 7,H - 2
                        cpu->R8.H |= (1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xFD: //SET 7,L - 2
                        cpu->R8.L |= (1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;
                    case 0xFE: //SET 7,(HL) - 4
                    {
                        GB_CPUClockCounterAdd(4);
                        u32 temp = GB_MemRead8(cpu->R16.HL);
                        GB_CPUClockCounterAdd(4);
                        GB_MemWrite8(cpu->R16.HL, temp | (1 << 7));
                        GB_CPUClockCounterAdd(4);
                        break;
                    }
                    case 0xFF: //SET 7,A - 2
                        cpu->R8.A |= (1 << 7);
                        GB_CPUClockCounterAdd(4);
                        break;

                    default:
                        // ...wtf??
                        GB_CPUClockCounterAdd(4);
                        _gb_break_to_debugger();
                        Debug_ErrorMsgArg("Unidentified opcode. CB %x\nPC: %04x\nROM: %d",
                            opcode,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                        break;
                } //end of inner switch

                break;
            case 0xCC: //CALL Z,nn - 6/3
                if(cpu->F.Z)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
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
                }
                else
                {
                    cpu->R16.PC += 2;
                    cpu->R16.PC &= 0xFFFF;
                    GB_CPUClockCounterAdd(12);
                }
                break;
            case 0xCD: //CALL nn - 6
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
            case 0xCE: //ADC A,n - 2
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
            case 0xCF: //RST 0x08 - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = 0x0008;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xD0: //RET NC - 5/2
                if(cpu->F.C == 0)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.SP++);
                    cpu->R16.SP &= 0xFFFF;
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                    cpu->R16.SP &= 0xFFFF;
                    GB_CPUClockCounterAdd(4);
                    cpu->R16.PC = temp;
                    GB_CPUClockCounterAdd(8);
                }
                else
                {
                    GB_CPUClockCounterAdd(8);
                }
                break;
            case 0xD1: //POP DE - 3
                GB_CPUClockCounterAdd(4);
                cpu->R8.E = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R8.D = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xD2: //JP NC,nn - 4/3
                if(cpu->F.C == 0)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                    GB_CPUClockCounterAdd(4);
                    cpu->R16.PC = temp;
                    GB_CPUClockCounterAdd(4);
                }
                else
                {
                    cpu->R16.PC += 2;
                    cpu->R16.PC &= 0xFFFF;
                    GB_CPUClockCounterAdd(12);
                }
                break;
            case 0xD3: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. D3\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xD4: //CALL NC,nn - 6/3
                if(cpu->F.C == 0)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
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
                }
                else
                {
                    cpu->R16.PC += 2;
                    cpu->R16.PC &= 0xFFFF;
                    GB_CPUClockCounterAdd(12);
                }
                break;
            case 0xD5: //PUSH DE - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.D);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.E);
                GB_CPUClockCounterAdd(8);
                break;
            case 0xD6:  //SUB n - 2
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
            case 0xD7: //RST 0x10 - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = 0x0010;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xD8: //RET C - 5/2
                if(cpu->F.C)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.SP++);
                    cpu->R16.SP &= 0xFFFF;
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                    cpu->R16.SP &= 0xFFFF;
                    GB_CPUClockCounterAdd(4);
                    cpu->R16.PC = temp;
                    GB_CPUClockCounterAdd(8);
                }
                else
                {
                    GB_CPUClockCounterAdd(8);
                }
                break;
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
            case 0xDA: //JP C,nn - 4/3
                if(cpu->F.C)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                    GB_CPUClockCounterAdd(4);
                    cpu->R16.PC = temp;
                    GB_CPUClockCounterAdd(4);
                }
                else
                {
                    cpu->R16.PC += 2;
                    cpu->R16.PC &= 0xFFFF;
                    GB_CPUClockCounterAdd(12);
                }
                break;
            case 0xDB: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. DB\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xDC: //CALL C,nn - 6/3
                if(cpu->F.C)
                {
                    GB_CPUClockCounterAdd(4);
                    u32 temp = GB_MemRead8(cpu->R16.PC++);
                    GB_CPUClockCounterAdd(4);
                    temp |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
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
                }
                else
                {
                    cpu->R16.PC += 2;
                    cpu->R16.PC &= 0xFFFF;
                    GB_CPUClockCounterAdd(12);
                }
                break;
            case 0xDD: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. DD\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xDE: //SBC A,n - 2
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
            case 0xDF: //RST 0x18 - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = 0x0018;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xE0: //LDH (n),A - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = 0xFF00 + (u32)GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(temp,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xE1: //POP HL - 3
                GB_CPUClockCounterAdd(4);
                cpu->R8.L = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R8.H = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xE2: //LD (C),A - 2
                GB_CPUClockCounterAdd(4);
                GB_MemWrite8(0xFF00 + (u32)cpu->R8.C,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xE3: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. E3\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xE4: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. E4\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xE5: //PUSH HL - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.H);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.L);
                GB_CPUClockCounterAdd(8);
                break;
            case 0xE6: //AND n - 2
            {
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R8.A &= GB_MemRead8(cpu->R16.PC++);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xE7: //RST 0x20 - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = 0x0020;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xE8: //ADD SP,n - 4
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
            case 0xEA: //LD (nn),A - 4
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
            case 0xEB: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. EB\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xEC: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. EC\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xED: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. ED\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xEE: //XOR n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A ^= GB_MemRead8(cpu->R16.PC++);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xEF: //RST 0x28 - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = 0x0028;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF0: //LDH A,(n) - 3
            {
                GB_CPUClockCounterAdd(4);
                u32 temp = 0xFF00 + (u32)GB_MemRead8(cpu->R16.PC++);
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(temp);
                GB_CPUClockCounterAdd(4);
                break;
            }
            case 0xF1: //POP AF - 3
                GB_CPUClockCounterAdd(4);
                cpu->R8.F = GB_MemRead8(cpu->R16.SP++) & 0xF0;
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF2: //LD A,(C) - 2
                GB_CPUClockCounterAdd(4);
                cpu->R8.A = GB_MemRead8(0xFF00 + (u32)cpu->R8.C);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF3: //DI - 1
                GameBoy.Memory.InterruptMasterEnable = 0;
                GameBoy.Memory.interrupts_enable_count = 0;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF4: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. F4\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xF5: //PUSH AF - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.A);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.F);
                GB_CPUClockCounterAdd(8);
                break;
            case 0xF6: //OR n - 2
                GB_CPUClockCounterAdd(4);
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= GB_MemRead8(cpu->R16.PC++);
                cpu->F.Z = (cpu->R8.A == 0);
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF7: //RST 0x30 - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = 0x0030;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xF8: //LD HL,SP+n - 3
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
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP = cpu->R16.HL;
                GB_CPUClockCounterAdd(4);
                break;
            case 0xFA: //LD A,(nn) - 4
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
            case 0xFC: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. FC\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xFD: // Undefined - *
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. FD\nPC: %04x\nROM: %d",
                            GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                break;
            case 0xFE: //CP n - 2
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
            case 0xFF: //RST 0x38 - 4
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                GB_CPUClockCounterAdd(4);
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                GB_CPUClockCounterAdd(4);
                cpu->R16.PC = 0x0038;
                GB_CPUClockCounterAdd(4);
                break;

            default: //wtf...?
                GB_CPUClockCounterAdd(4);
                _gb_break_to_debugger();
                Debug_ErrorMsgArg("Unidentified opcode. %x\nPC: %04x\nROM: %d",
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

            int dma_executed_clocks = GB_DMAExecute(clocks_to_next_event); // GB_CPUClockCounterAdd() internal
            if(dma_executed_clocks == 0)
            {
                int irq_executed_clocks = GB_IRQExecute(); // GB_CPUClockCounterAdd() internal
                if(irq_executed_clocks == 0)
                {
                    if(GameBoy.Emulator.CPUHalt == 0)
                    {
                        executed_clocks = GB_CPUExecute(clocks_to_next_event); // GB_CPUClockCounterAdd() internal
                    }
                    else
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
