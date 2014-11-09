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
#include "serial.h"
#include "gb_main.h"
#include "gb_camera.h"

#include "../gui/win_gb_disassembler.h"

extern _GB_CONTEXT_ GameBoy;

extern const u8 gb_daa_table[256*8*2]; // In file daa_table.c

/* TO DO

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

void GB_CPUInit(void)
{
    GB_CPUInterruptsInit();

    GameBoy.Emulator.cpu_microinstruction = 0;
    GameBoy.Emulator.ScreenMode = 2;
    GameBoy.Memory.IO_Ports[STAT_REG-0xFF00] = 0x02;
    GameBoy.Emulator.CurrentScanLine = 0;
    GameBoy.Emulator.CPUHalt = 0;
    GameBoy.Emulator.DoubleSpeed = 0;
    GameBoy.Emulator.halt_not_executed = 0;

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

inline void GB_CPUClock(int clocks) // the lower the number of clocks, the better
{
    GB_LCDUpdate(clocks);
    GB_TimersUpdate(clocks);
    GB_SoundUpdate(clocks);
    GB_SerialClocks(clocks);
    SGB_Clock(clocks);
    GB_CameraClock(clocks);

    GB_CPUHandleClockEvents();
}

int gb_break_execution = 0;

inline void _gb_break_to_debugger(void)
{
    gb_break_execution = 1;
    GameBoy.Emulator.FrameDrawn = 1; // This is a hack, but it works...
}

// Single Speed
// 4194304 Hz
// 0.2384185791015625 us per cycle
//
// Double Speed
// 8388608 Hz
// 0.11920928955078125 us per cycle
//
// Screen refresh:
// 59.73 Hz

void GB_CPUMicroinstructionStep(void); // in this file, the last function

void GB_CPUExecuteOscillatorClock(void)
{
    _GB_CPU_ * cpu = &GameBoy.CPU;
    _GB_MEMORY_ * mem = &GameBoy.Memory;
    _EMULATOR_INFO_ * emu = &GameBoy.Emulator;

    if(emu->cpu_microinstruction == 0)
    {
        if(GB_DebugCPUIsBreakpoint(cpu->R16.PC))
        {
            _gb_break_to_debugger();
            Win_GBDisassemblerSetFocus();
            return;
        }
    }

    int gbc_dma_working = 0;

    if(GameBoy.Emulator.CGBEnabled)
        if(emu->cpu_oscillator_clocks == 0) // wait until the instruction has finished
            if(GBC_HDMAcopy()) gbc_dma_working = 1;

    if(gbc_dma_working == 0)
        emu->cpu_oscillator_clocks ++;

    // Next instruction step every 4 clocks (2 in double speed) -- LAST THING IN THIS FUNCTION
    if( emu->cpu_oscillator_clocks >= (4 >> emu->DoubleSpeed) )
    {
        emu->cpu_oscillator_clocks -= (4 >> emu->DoubleSpeed);

        if(mem->interrupts_enable_count) // EI interrupt enable delay
        {
            mem->interrupts_enable_count = 0;
            mem->InterruptMasterEnable = 1;
        }

        //INTERRUPT HANDLING
        if(emu->cpu_microinstruction == 0)
        {
            //only enter an interrupt if last instruction has ended
            int interrupts = mem->IO_Ports[IF_REG-0xFF00] & mem->HighRAM[IE_REG-0xFF80] & 0x1F;
            if(interrupts != 0)
            {
                GameBoy.Emulator.CPUHalt = 0;

                if(mem->InterruptMasterEnable)
                {
                    emu->interrupt_executing = 1;
                }
            }
        }

        int interrupt_executed = 0;

        if(emu->interrupt_executing) // interrupts should need the same time in dual speed or normal speed!!
        {
            interrupt_executed = 1;

            switch(emu->cpu_microinstruction)
            {
                case 0:
                    GameBoy.Memory.InterruptMasterEnable = 0;
                    emu->cpu_microinstruction ++;
                    break;
                case 1:
                    cpu->R16.SP --;
                    cpu->R16.SP &= 0xFFFF;
                    GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
                    emu->cpu_microinstruction ++;
                    break;
                case 2:
                    cpu->R16.SP --;
                    cpu->R16.SP &= 0xFFFF;
                    GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
                    emu->cpu_microinstruction ++;
                    break;
                case 3:
                {
                    int interrupts = mem->IO_Ports[IF_REG-0xFF00] & mem->HighRAM[IE_REG-0xFF80] & 0x1F;
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
                    emu->cpu_microinstruction ++;
                    break;
                }
                case 4:
                    emu->interrupt_executing = 0;
                    emu->cpu_microinstruction = 0;
                    break;
                default:
                    break;
            }
        }

        if(interrupt_executed == 0)
        {
            if(emu->halt_not_executed && mem->InterruptMasterEnable)
            {
                emu->CPUHalt = 1;
                emu->halt_not_executed = 0;
            }

            if(GameBoy.Emulator.CPUHalt == 1) // halt
            {
                // HALT = Do nothing
            }
            else if(GameBoy.Emulator.CPUHalt == 2) // stop
            {
                //GameBoy.Emulator.FrameDrawn = 1; // The GB does nothing in stop mode, so
                //GameBoy.Emulator.FrameCount ++; //let's just skip frame...

                //GameBoy.Emulator.CPUHalt = 2; //Interrupts doesn't work, at least, VBL.

                u32 i = mem->IO_Ports[P1_REG-0xFF00];
                u32 result = 0;

                //if((i & 0x30) == 0x30)
                //{
                //    Debug_DebugMsgArg("STOP with no possible exit, ignored.\nPC: %04x\nROM: %d",
                //           GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                //    GameBoy.Emulator.CPUHalt = 0;
                //    return;
                //}
                int Keys = GB_Input_Get(0);
                if((i & (1<<5)) == 0) //A-B-SEL-STA
                    result |= Keys & (KEY_A|KEY_B|KEY_SELECT|KEY_START);
                if((i & (1<<4)) == 0) //PAD
                    result |= Keys & (KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT);

                if(result) GameBoy.Emulator.CPUHalt = 0;
            }

            if(GameBoy.Emulator.CPUHalt == 0)
            {
                GB_CPUMicroinstructionStep(); //Execute microinstructions
            }
        }
    }

    GB_CPUClock(1); // update everything
}

int GB_RunFor(s32 clocks) // 1 frame = 70224 clocks
{
    Win_GBDisassemblerStartAddressSetDefault();
    while(clocks --)
    {
        GB_CPUExecuteOscillatorClock();
        if(gb_break_execution)
        {
            gb_break_execution = 0;
            return 1;
        }
    }
    return 0;
}

void GB_RunForInstruction(void)
{
    Win_GBDisassemblerStartAddressSetDefault();
    do
    {
        if(GB_RunFor(4>>GameBoy.Emulator.DoubleSpeed))
            return;
    }
    while(GameBoy.Emulator.cpu_microinstruction != 0);
}

static const u32 gb_clocks_table[256] = {
    1, 3, 2, 2, 1, 1, 2, 1,  5, 2, 2, 2, 1, 1, 2, 1,
    2, 3, 2, 2, 1, 1, 2, 1,  3, 2, 2, 2, 1, 1, 2, 1,
    3, 3, 2, 2, 1, 1, 2, 1,  3, 2, 2, 2, 1, 1, 2, 1,
    3, 3, 2, 2, 3, 3, 3, 1,  3, 2, 2, 2, 1, 1, 2, 1,

    1, 1, 1, 1, 1, 1, 2, 1,  1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1,  1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1,  1, 1, 1, 1, 1, 1, 2, 1,
    2, 2, 2, 2, 2, 2, 1, 2,  1, 1, 1, 1, 1, 1, 2, 1,

    1, 1, 1, 1, 1, 1, 2, 1,  1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1,  1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1,  1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1,  1, 1, 1, 1, 1, 1, 2, 1,

    5, 3, 4, 4, 6, 4, 2, 4,  5, 4, 4, 2, 6, 6, 2, 4, // CB = 2 at least
    5, 3, 4, 0, 6, 4, 2, 4,  5, 4, 4, 0, 6, 0, 2, 4,
    3, 3, 2, 0, 0, 4, 2, 4,  4, 1, 4, 0, 0, 0, 2, 4,
    3, 3, 2, 1, 0, 4, 2, 4,  3, 2, 4, 1, 0, 0, 2, 4
};

static const u32 gb_clocks_table_cb[256] = {
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,

    2, 2, 2, 2, 2, 2,  3, 2, 2, 2, 2, 2, 2, 2,  3, 2,
    2, 2, 2, 2, 2, 2,  3, 2, 2, 2, 2, 2, 2, 2,  3, 2,
    2, 2, 2, 2, 2, 2,  3, 2, 2, 2, 2, 2, 2, 2,  3, 2,
    2, 2, 2, 2, 2, 2,  3, 2, 2, 2, 2, 2, 2, 2,  3, 2,

    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,

    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2,
    2, 2, 2, 2, 2, 2,  4, 2, 2, 2, 2, 2, 2, 2,  4, 2
};

//this can only execute one microinstruction at a time = 4 oscillator cycles (2 in dual speed mode)
//trying something strange with this will break conditional instructions
void GB_CPUMicroinstructionStep(void)
{
    _GB_CPU_ * cpu = &GameBoy.CPU;
    _GB_MEMORY_ * mem = &GameBoy.Memory;
    _EMULATOR_INFO_ * emu = &GameBoy.Emulator;

    if(emu->cpu_microinstruction == 0)
    {
        emu->opcode = (u32)(u8)GB_MemRead8(cpu->R16.PC++);
        cpu->R16.PC &= 0xFFFF;
        emu->cb_opcode = -1;
        //Debug_DebugMsgArg("Fetch %02X",emu->opcode);
    }

    switch(emu->opcode)
    {
        case 0x00: //NOP
            break;
        case 0x01: //LD BC,nn
            if(emu->cpu_microinstruction == 1)
                cpu->R8.C = GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                cpu->R8.B = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x02: //LD (BC),A
            GB_MemWrite8(cpu->R16.BC,cpu->R8.A);
            break;
        case 0x03: //INC BC
            if(emu->cpu_microinstruction == 1)
                cpu->R16.BC = (cpu->R16.BC+1) & 0xFFFF;
            break;
        case 0x04: //INC B
            cpu->R16.AF &= ~F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.B & 0xF) == 0xF);
            cpu->R8.B ++;
            cpu->F.Z = (cpu->R8.B == 0);
            break;
        case 0x05: //DEC B
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.B & 0xF) == 0x0);
            cpu->R8.B --;
            cpu->F.Z = (cpu->R8.B == 0);
            break;
        case 0x06: //LD B,n
            if(emu->cpu_microinstruction == 1)
                cpu->R8.B = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x07: //RLCA
            cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
            cpu->F.C = (cpu->R8.A & 0x80) != 0;
            cpu->R8.A = (cpu->R8.A << 1) | cpu->F.C;
            break;
        case 0x08: //LD (nn),SP
            if(emu->cpu_microinstruction == 1)
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                emu->cpu_temp_16 |= ( ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8);
            else if(emu->cpu_microinstruction == 3)
                GB_MemWrite8(emu->cpu_temp_16++,cpu->R8.SPL);
            else if(emu->cpu_microinstruction == 4)
                GB_MemWrite8(emu->cpu_temp_16,cpu->R8.SPH);
            break;
        case 0x09: //ADD HL,BC
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R16.HL + cpu->R16.BC;
                cpu->F.C = (temp > 0xFFFF);
                cpu->F.H = ( ( (cpu->R16.HL & 0x0FFF) + (cpu->R16.BC & 0x0FFF) ) > 0x0FFF );
                cpu->R16.HL = temp & 0xFFFF;
            }
            break;
        case 0x0A: //LD A,(BC)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.A = GB_MemRead8(cpu->R16.BC);
            break;
        case 0x0B: //DEC BC
            if(emu->cpu_microinstruction == 1)
                cpu->R16.BC = (cpu->R16.BC-1) & 0xFFFF;
            break;
        case 0x0C: //INC C
            cpu->R16.AF &= ~F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.C & 0xF) == 0xF);
            cpu->R8.C ++;
            cpu->F.Z = (cpu->R8.C == 0);
            break;
        case 0x0D: //DEC C
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.C & 0xF) == 0x0);
            cpu->R8.C --;
            cpu->F.Z = (cpu->R8.C == 0);
            break;
        case 0x0E: //LD C,n
            if(emu->cpu_microinstruction == 1)
                cpu->R8.C = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x0F: //RRCA
            cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
            cpu->F.C = (cpu->R8.A & 0x01) != 0;
            cpu->R8.A = (cpu->R8.A >> 1) | (cpu->F.C << 7);
            break;
        case 0x10: //STOP
            if(emu->cpu_microinstruction == 1)
            {
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
            }
            break;
        case 0x11: //LD DE,nn
            if(emu->cpu_microinstruction == 1)
                cpu->R8.E = GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                cpu->R8.D = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x12: //LD (DE),A
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(cpu->R16.DE,cpu->R8.A);
            break;
        case 0x13: //INC DE
            if(emu->cpu_microinstruction == 1)
                cpu->R16.DE = (cpu->R16.DE+1) & 0xFFFF;
            break;
        case 0x14: //INC D
            cpu->R16.AF &= ~(F_SUBTRACT);
            cpu->F.H = ( (cpu->R8.D & 0xF) == 0xF);
            cpu->R8.D ++;
            cpu->F.Z = (cpu->R8.D == 0);
            break;
        case 0x15: //DEC D
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.D & 0xF) == 0x0);
            cpu->R8.D --;
            cpu->F.Z = (cpu->R8.D == 0);
            break;
        case 0x16: //LD D,n
            if(emu->cpu_microinstruction == 1)
                cpu->R8.D = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x17: //RLA
        {
            cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
            u32 temp = cpu->F.C; //old carry flag
            cpu->F.C = (cpu->R8.A & 0x80) != 0;
            cpu->R8.A = (cpu->R8.A << 1) | temp;
            break;
        }
        case 0x18: //JR n
            if(emu->cpu_microinstruction == 1)
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                cpu->R16.PC = (cpu->R16.PC + (s8)emu->cpu_temp_16) & 0xFFFF;
            break;
        case 0x19: //ADD HL,DE
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R16.HL + cpu->R16.DE;
                cpu->F.C = ( temp > 0xFFFF );
                cpu->F.H = ( ( (cpu->R16.HL & 0xFFF) + (cpu->R16.DE & 0xFFF) ) > 0xFFF );
                cpu->R16.HL = temp & 0xFFFF;
            }
            break;
        case 0x1A: //LD A,(DE)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.A = GB_MemRead8(cpu->R16.DE);
            break;
        case 0x1B: //DEC DE
            if(emu->cpu_microinstruction == 1)
                cpu->R16.DE = (cpu->R16.DE-1) & 0xFFFF;
            break;
        case 0x1C: //INC E
            cpu->R16.AF &= ~F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.E & 0xF) == 0xF);
            cpu->R8.E ++;
            cpu->F.Z = (cpu->R8.E == 0);
            break;
        case 0x1D: //DEC E
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.E & 0xF) == 0x0);
            cpu->R8.E --;
            cpu->F.Z = (cpu->R8.E == 0);
            break;
        case 0x1E: //LD E,n
            if(emu->cpu_microinstruction == 1)
                cpu->R8.E = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x1F: //RRA
        {
            cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_ZERO);
            u32 temp = cpu->F.C; //old carry flag
            cpu->F.C = cpu->R8.A & 0x01;
            cpu->R8.A = (cpu->R8.A >> 1) | (temp << 7);
            break;
        }
        case 0x20: //JR NZ,n
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.Z == 0)
                {
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                }
                else
                {
                    cpu->R16.PC ++;
                    emu->cpu_microinstruction++; // go to the end of the instruction
                }
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.PC = (cpu->R16.PC + (s8)emu->cpu_temp_16) & 0xFFFF;
            }
            break;
        case 0x21: //LD HL,nn
            if(emu->cpu_microinstruction == 1)
                cpu->R8.L = GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                cpu->R8.H = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x22: //LD (HLI),A | LD (HL+),A | LDI (HL),A
            if(emu->cpu_microinstruction == 1)
            {
                GB_MemWrite8(cpu->R16.HL,cpu->R8.A);
                cpu->R16.HL = (cpu->R16.HL+1) & 0xFFFF;
            }
            break;
        case 0x23: //INC HL
            if(emu->cpu_microinstruction == 1)
                cpu->R16.HL = (cpu->R16.HL+1) & 0xFFFF;
            break;
        case 0x24: //INC H
            cpu->R16.AF &= ~F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.H & 0xF) == 0xF);
            cpu->R8.H ++;
            cpu->F.Z = (cpu->R8.H == 0);
            break;
        case 0x25: //DEC H
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.H & 0xF) == 0x0);
            cpu->R8.H --;
            cpu->F.Z = (cpu->R8.H == 0);
            break;
        case 0x26: //LD H,n
            if(emu->cpu_microinstruction == 1)
                cpu->R8.H = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x27: //DAA
        {
            u32 temp = ( ((u32)cpu->R8.A)<<(3+1) ) | ((((u32)cpu->R8.F>>4)&7)<<1);
            cpu->R8.A = gb_daa_table[temp];
            cpu->R8.F = gb_daa_table[temp+1];
            break;
        }
        case 0x28: //JR Z,n
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.Z)
                {
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                }
                else
                {
                    cpu->R16.PC ++;
                    emu->cpu_microinstruction++; // go to the end of the instruction
                }
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.PC = (cpu->R16.PC + (s8)emu->cpu_temp_16) & 0xFFFF;
            }
            break;
        case 0x29: //ADD HL,HL
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.C = (cpu->R16.HL & 0x8000) != 0;
                cpu->F.H = (cpu->R16.HL & 0x0800) != 0;
                cpu->R16.HL = (cpu->R16.HL << 1) & 0xFFFF;
            }
            break;
        case 0x2A: //LD A,(HLI) | LD A,(HL+) | LDI A,(HL)
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R8.A = GB_MemRead8(cpu->R16.HL);
                cpu->R16.HL = (cpu->R16.HL+1) & 0xFFFF;
            }
            break;
        case 0x2B: //DEC HL
            if(emu->cpu_microinstruction == 1)
                cpu->R16.HL = (cpu->R16.HL-1) & 0xFFFF;
            break;
        case 0x2C: //INC L
            cpu->R16.AF &= ~F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.L & 0xF) == 0xF);
            cpu->R8.L ++;
            cpu->F.Z = (cpu->R8.L == 0);
            break;
        case 0x2D: //DEC L
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.L & 0xF) == 0x0);
            cpu->R8.L --;
            cpu->F.Z = (cpu->R8.L == 0);
            break;
        case 0x2E: //LD L,n
            if(emu->cpu_microinstruction == 1)
                cpu->R8.L = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x2F: //CPL
            cpu->R16.AF |= (F_SUBTRACT|F_HALFCARRY);
            cpu->R8.A = ~cpu->R8.A;
            break;
        case 0x30: //JR NC,n
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.C == 0)
                {
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                }
                else
                {
                    cpu->R16.PC ++;
                    emu->cpu_microinstruction++; // go to the end of the instruction
                }
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.PC = (cpu->R16.PC + (s8)emu->cpu_temp_16) & 0xFFFF;
            }
            break;
        case 0x31: //LD SP,nn
            if(emu->cpu_microinstruction == 1)
                cpu->R8.SPL = GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                cpu->R8.SPH = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x32: //LD (HLD),A | LD (HL-),A | LDD (HL),A
            if(emu->cpu_microinstruction == 1)
            {
                GB_MemWrite8(cpu->R16.HL,cpu->R8.A);
                cpu->R16.HL = (cpu->R16.HL-1) & 0xFFFF;
            }
            break;
        case 0x33: //INC SP
            if(emu->cpu_microinstruction == 1)
                cpu->R16.SP = (cpu->R16.SP+1) & 0xFFFF;
            break;
        case 0x34: //INC (HL)
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                u32 temp = emu->cpu_temp_16;
                cpu->R16.AF &= ~F_SUBTRACT;
                cpu->F.H = ( (temp & 0xF) == 0xF);
                temp = (temp + 1) & 0xFF;
                cpu->F.Z = (temp == 0);
                GB_MemWrite8(cpu->R16.HL,temp);
            }
            break;
        case 0x35: //DEC (HL)
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                u32 temp = emu->cpu_temp_16;
                cpu->R16.AF |= F_SUBTRACT;
                cpu->F.H = ( (temp & 0xF) == 0x0);
                temp  = (temp - 1) & 0xFF;
                cpu->F.Z = (temp == 0);
                GB_MemWrite8(cpu->R16.HL,temp);
            }
            break;
        case 0x36: //LD (HL),n
            if(emu->cpu_microinstruction == 1)
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                GB_MemWrite8(cpu->R16.HL,emu->cpu_temp_16);
            break;
        case 0x37: //SCF
            cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
            cpu->R16.AF |= F_CARRY;
            break;
        case 0x38: //JR C,n
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.C == 1)
                {
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                }
                else
                {
                    cpu->R16.PC ++;
                    emu->cpu_microinstruction++; // go to the end of the instruction
                }
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.PC = (cpu->R16.PC + (s8)emu->cpu_temp_16) & 0xFFFF;
            }
            break;
        case 0x39: //ADD HL,SP
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = cpu->R16.HL + cpu->R16.SP;
                cpu->F.C = (temp > 0xFFFF);
                cpu->F.H = ( ( (cpu->R16.HL & 0x0FFF) + (cpu->R16.SP & 0x0FFF) ) > 0x0FFF );
                cpu->R16.HL = temp & 0xFFFF;
            }
            break;
        case 0x3A: //LD A,(HLD) | LD A,(HL-) | LDD A,(HL)
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R8.A = GB_MemRead8(cpu->R16.HL);
                cpu->R16.HL = (cpu->R16.HL-1) & 0xFFFF;
            }
            break;
        case 0x3B: //DEC SP
            if(emu->cpu_microinstruction == 1)
                cpu->R16.SP = (cpu->R16.SP-1) & 0xFFFF;
            break;
        case 0x3C: //INC A
            cpu->R16.AF &= ~F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.A & 0xF) == 0xF);
            cpu->R8.A ++;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x3D: //DEC A
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.A & 0xF) == 0x0);
            cpu->R8.A --;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x3E: //LD A,n
            if(emu->cpu_microinstruction == 1)
                cpu->R8.A = GB_MemRead8(cpu->R16.PC++);
            break;
        case 0x3F: //CCF
            cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
            cpu->F.C = !cpu->F.C;
            break;
        case 0x40: //LD B,B
            //cpu->R8.B = cpu->R8.B;
            break;
        case 0x41: //LD B,C
            cpu->R8.B = cpu->R8.C;
            break;
        case 0x42: //LD B,D
            cpu->R8.B = cpu->R8.D;
            break;
        case 0x43: //LD B,E
            cpu->R8.B = cpu->R8.E;
            break;
        case 0x44: //LD B,H
            cpu->R8.B = cpu->R8.H;
            break;
        case 0x45: //LD B,L
            cpu->R8.B = cpu->R8.L;
            break;
        case 0x46: //LD B,(HL)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.B = GB_MemRead8(cpu->R16.HL);
            break;
        case 0x47: //LD B,A
            cpu->R8.B = cpu->R8.A;
            break;
        case 0x48: //LD C,B
            cpu->R8.C = cpu->R8.B;
            break;
        case 0x49: //LD C,C
            //cpu->R8.C = cpu->R8.C;
            break;
        case 0x4A: //LD C,D
            cpu->R8.C = cpu->R8.D;
            break;
        case 0x4B: //LD C,E
            cpu->R8.C = cpu->R8.E;
            break;
        case 0x4C: //LD C,H
            cpu->R8.C = cpu->R8.H;
            break;
        case 0x4D: //LD C,L
            cpu->R8.C = cpu->R8.L;
            break;
        case 0x4E: //LD C,(HL)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.C = GB_MemRead8(cpu->R16.HL);
            break;
        case 0x4F: //LD C,A
            cpu->R8.C = cpu->R8.A;
            break;
        case 0x50: //LD D,B
            cpu->R8.D = cpu->R8.B;
            break;
        case 0x51: //LD D,C
            cpu->R8.D = cpu->R8.C;
            break;
        case 0x52: //LD D,D
            //cpu->R8.D = cpu->R8.D;
            break;
        case 0x53: //LD D,E
            cpu->R8.D = cpu->R8.E;
            break;
        case 0x54: //LD D,H
            cpu->R8.D = cpu->R8.H;
            break;
        case 0x55: //LD D,L
            cpu->R8.D = cpu->R8.L;
            break;
        case 0x56: //LD D,(HL)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.D = GB_MemRead8(cpu->R16.HL);
            break;
        case 0x57: //LD D,A
            cpu->R8.D = cpu->R8.A;
            break;
        case 0x58: //LD E,B
            cpu->R8.E = cpu->R8.B;
            break;
        case 0x59: //LD E,C
            cpu->R8.E = cpu->R8.C;
            break;
        case 0x5A: //LD E,D
            cpu->R8.E = cpu->R8.D;
            break;
        case 0x5B: //LD E,E
            //cpu->R8.E = cpu->R8.E;
            break;
        case 0x5C: //LD E,H
            cpu->R8.E = cpu->R8.H;
            break;
        case 0x5D: //LD E,L
            cpu->R8.E = cpu->R8.L;
            break;
        case 0x5E: //LD E,(HL)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.E = GB_MemRead8(cpu->R16.HL);
            break;
        case 0x5F: //LD E,A
            cpu->R8.E = cpu->R8.A;
            break;
        case 0x60: //LD H,B
            cpu->R8.H = cpu->R8.B;
            break;
        case 0x61: //LD H,C
            cpu->R8.H = cpu->R8.C;
            break;
        case 0x62: //LD H,D
            cpu->R8.H = cpu->R8.D;
            break;
        case 0x63: //LD H,E
            cpu->R8.H = cpu->R8.E;
            break;
        case 0x64: //LD H,H
            //cpu->R8.H = cpu->R8.H;
            break;
        case 0x65: //LD H,L
            cpu->R8.H = cpu->R8.L;
            break;
        case 0x66: //LD H,(HL)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.H = GB_MemRead8(cpu->R16.HL);
            break;
        case 0x67: //LD H,A
            cpu->R8.H = cpu->R8.A;
            break;
        case 0x68: //LD L,B
            cpu->R8.L = cpu->R8.B;
            break;
        case 0x69: //LD L,C
            cpu->R8.L = cpu->R8.C;
            break;
        case 0x6A: //LD L,D
            cpu->R8.L = cpu->R8.D;
            break;
        case 0x6B: //LD L,E
            cpu->R8.L = cpu->R8.E;
            break;
        case 0x6C: //LD L,H
            cpu->R8.L = cpu->R8.H;
            break;
        case 0x6D: //LD L,L
            //cpu->R8.L = cpu->R8.L;
            break;
        case 0x6E: //LD L,(HL)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.L = GB_MemRead8(cpu->R16.HL);
            break;
        case 0x6F: //LD L,A
            cpu->R8.L = cpu->R8.A;
            break;
        case 0x70: //LD (HL),B
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(cpu->R16.HL,cpu->R8.B);
            break;
        case 0x71: //LD (HL),C
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(cpu->R16.HL,cpu->R8.C);
            break;
        case 0x72: //LD (HL),D
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(cpu->R16.HL,cpu->R8.D);
            break;
        case 0x73: //LD (HL),E
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(cpu->R16.HL,cpu->R8.E);
            break;
        case 0x74: //LD (HL),H
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(cpu->R16.HL,cpu->R8.H);
            break;
        case 0x75: //LD (HL),L
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(cpu->R16.HL,cpu->R8.L);
            break;
        case 0x76: //HALT
            if(GameBoy.Memory.InterruptMasterEnable == 1)
            {
            //    if(GameBoy.Emulator.HDMAenabled == HDMA_NONE)
            //    {
                    GameBoy.Emulator.CPUHalt = 1;
            //    }
            }
            else
            {
        /*        if(mem->IO_Ports[IF_REG-0xFF00] & mem->HighRAM[IE_REG-0xFF80] & 0x1F)
                {

                    The first byte of the next instruction
                after HALT is read. The Program Counter (PC) fails to
                increment to the next memory location. As a results, the
                first byte after HALT is read again. From this point on
                the Program Counter once again operates normally

                }
                else
                {
                    //Nothing
                }
        */
                GameBoy.Emulator.halt_not_executed = 1;
            }
            break;
        case 0x77: //LD (HL),A
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(cpu->R16.HL,cpu->R8.A);
            break;
        case 0x78: //LD A,B
            cpu->R8.A = cpu->R8.B;
            break;
        case 0x79: //LD A,C
            cpu->R8.A = cpu->R8.C;
            break;
        case 0x7A: //LD A,D
            cpu->R8.A = cpu->R8.D;
            break;
        case 0x7B: //LD A,E
            cpu->R8.A = cpu->R8.E;
            break;
        case 0x7C: //LD A,H
            cpu->R8.A = cpu->R8.H;
            break;
        case 0x7D: //LD A,L
            cpu->R8.A = cpu->R8.L;
            break;
        case 0x7E: //LD A,(HL)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.A = GB_MemRead8(cpu->R16.HL);
            break;
        case 0x7F: //LD A,A
            //cpu->R8.A = cpu->R8.A;
            break;
        case 0x80: //ADD A,B
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A;

            cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.B & 0xF) ) > 0xF;

            cpu->R8.A += cpu->R8.B;
            cpu->F.Z = (cpu->R8.A == 0);

            cpu->F.C = (temp > cpu->R8.A);
            break;
        }
        case 0x81: //ADD A,C
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A;

            cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.C & 0xF) ) > 0xF;

            cpu->R8.A += cpu->R8.C;
            cpu->F.Z = (cpu->R8.A == 0);

            cpu->F.C = (temp > cpu->R8.A);
            break;
        }
        case 0x82: //ADD A,D
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A;

            cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.D & 0xF) ) > 0xF;

            cpu->R8.A += cpu->R8.D;
            cpu->F.Z = (cpu->R8.A == 0);

            cpu->F.C = (temp > cpu->R8.A);
            break;
        }
        case 0x83: //ADD A,E
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A;

            cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.E & 0xF) ) > 0xF;

            cpu->R8.A += cpu->R8.E;
            cpu->F.Z = (cpu->R8.A == 0);

            cpu->F.C = (temp > cpu->R8.A);
            break;
        }
        case 0x84: //ADD A,H
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A;

            cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.H & 0xF) ) > 0xF;

            cpu->R8.A += cpu->R8.H;
            cpu->F.Z = (cpu->R8.A == 0);

            cpu->F.C = (temp > cpu->R8.A);
            break;
        }
        case 0x85: //ADD A,L
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A;

            cpu->F.H = ( (temp & 0xF) + ((u32)cpu->R8.L & 0xF) ) > 0xF;

            cpu->R8.A += cpu->R8.L;
            cpu->F.Z = (cpu->R8.A == 0);

            cpu->F.C = (temp > cpu->R8.A);
            break;
        }
        case 0x86: //ADD A,(HL)
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~F_SUBTRACT;

                u32 temp = cpu->R8.A;
                u32 temp2 = GB_MemRead8(cpu->R16.HL);

                cpu->F.H = ( (temp & 0xF) + (temp2 & 0xF) ) > 0xF;

                cpu->R8.A += temp2;
                cpu->F.Z = (cpu->R8.A == 0);

                cpu->F.C = (temp > cpu->R8.A);
            }
            break;
        case 0x87: //ADD A,A
            cpu->R16.AF &= ~F_SUBTRACT;

            cpu->F.H = (cpu->R8.A & BIT(3)) != 0;
            cpu->F.C = (cpu->R8.A & BIT(7)) != 0;

            cpu->R8.A += cpu->R8.A;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x88: //ADC A,B
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A + cpu->R8.B + cpu->F.C;

            cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.B & 0xF) ) + cpu->F.C) > 0xF;
            cpu->F.C = (temp > 0xFF);

            temp &= 0xFF;

            cpu->R8.A = temp;
            cpu->F.Z = (temp == 0);
            break;
        }
        case 0x89: //ADC A,C
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A + cpu->R8.C + cpu->F.C;

            cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.C & 0xF) ) + cpu->F.C) > 0xF;
            cpu->F.C = (temp > 0xFF);

            temp &= 0xFF;

            cpu->R8.A = temp;
            cpu->F.Z = (temp == 0);
            break;
        }
        case 0x8A: //ADC A,D
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A + cpu->R8.D + cpu->F.C;

            cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.D & 0xF) ) + cpu->F.C) > 0xF;
            cpu->F.C = (temp > 0xFF);

            temp &= 0xFF;

            cpu->R8.A = temp;
            cpu->F.Z = (temp == 0);
            break;
        }
        case 0x8B: //ADC A,E
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A + cpu->R8.E + cpu->F.C;

            cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.E & 0xF) ) + cpu->F.C) > 0xF;
            cpu->F.C = (temp > 0xFF);

            temp &= 0xFF;

            cpu->R8.A = temp;
            cpu->F.Z = (temp == 0);
            break;
        }
        case 0x8C: //ADC A,H
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A + cpu->R8.H + cpu->F.C;

            cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.H & 0xF) ) + cpu->F.C) > 0xF;
            cpu->F.C = (temp > 0xFF);

            temp &= 0xFF;

            cpu->R8.A = temp;
            cpu->F.Z = (temp == 0);
            break;
        }
        case 0x8D: //ADC A,L
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = cpu->R8.A + cpu->R8.L + cpu->F.C;

            cpu->F.H = ( ((cpu->R8.A & 0xF) + (cpu->R8.L & 0xF) ) + cpu->F.C) > 0xF;
            cpu->F.C = (temp > 0xFF);

            temp &= 0xFF;

            cpu->R8.A = temp;
            cpu->F.Z = (temp == 0);
            break;
        }
        case 0x8E: //ADC A,(HL)
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = GB_MemRead8(cpu->R16.HL);
                u32 temp2 = cpu->R8.A + temp + cpu->F.C;

                cpu->F.H = ( ((cpu->R8.A & 0xF) + (temp & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp2 > 0xFF);

                temp2 &= 0xFF;

                cpu->R8.A = temp2;
                cpu->F.Z = (temp2 == 0);
            }
            break;
        case 0x8F: //ADC A,A
        {
            cpu->R16.AF &= ~F_SUBTRACT;

            u32 temp = ( ((u32)cpu->R8.A) << 1 ) + cpu->F.C;

            //Carry flag not needed to test this
            cpu->F.H = (cpu->R8.A & 0x08) != 0;

            cpu->F.C = (temp > 0xFF);

            temp &= 0xFF;

            cpu->R8.A = temp;
            cpu->F.Z = (temp == 0);
            break;
        }
        case 0x90: //SUB B
            cpu->R8.F = F_SUBTRACT;

            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.B & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.B;

            cpu->R8.A -= cpu->R8.B;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x91: //SUB C
            cpu->R8.F = F_SUBTRACT;

            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.C & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.C;

            cpu->R8.A -= cpu->R8.C;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x92: //SUB D
            cpu->R8.F = F_SUBTRACT;

            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.D & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.D;

            cpu->R8.A -= cpu->R8.D;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x93: //SUB E
            cpu->R8.F = F_SUBTRACT;

            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.E & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.E;

            cpu->R8.A -= cpu->R8.E;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x94: //SUB H
            cpu->R8.F = F_SUBTRACT;

            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.H & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.H;

            cpu->R8.A -= cpu->R8.H;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x95: //SUB L
            cpu->R8.F = F_SUBTRACT;

            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.L & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.L;

            cpu->R8.A -= cpu->R8.L;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0x96: //SUB (HL)
            if(emu->cpu_microinstruction == 1)
            {
                u32 temp = GB_MemRead8(cpu->R16.HL);
                cpu->R8.F = F_SUBTRACT;

                cpu->F.H = (cpu->R8.A & 0xF) < (temp & 0xF);
                cpu->F.C = (u32)cpu->R8.A < (u32)temp;

                cpu->R8.A -= temp;

                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0x97: //SUB A
            cpu->R8.F = F_SUBTRACT|F_ZERO;
            cpu->R8.A = 0;
            break;
        case 0x98: //SBC A,B
        {
            u32 temp = cpu->R8.A - cpu->R8.B - ((cpu->R8.F&F_CARRY)?1:0);
            cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.A^cpu->R8.B^temp) & 0x10 ) != 0 ;
            cpu->R8.A = temp;
            break;
        }
        case 0x99: //SBC A,C
        {
            u32 temp = cpu->R8.A - cpu->R8.C - ((cpu->R8.F&F_CARRY)?1:0);
            cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.A^cpu->R8.C^temp) & 0x10 ) != 0 ;
            cpu->R8.A = temp;
            break;
        }
        case 0x9A: //SBC A,D
        {
            u32 temp = cpu->R8.A - cpu->R8.D - ((cpu->R8.F&F_CARRY)?1:0);
            cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.A^cpu->R8.D^temp) & 0x10 ) != 0 ;
            cpu->R8.A = temp;
            break;
        }
        case 0x9B: //SBC A,E
        {
            u32 temp = cpu->R8.A - cpu->R8.E - ((cpu->R8.F&F_CARRY)?1:0);
            cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.A^cpu->R8.E^temp) & 0x10 ) != 0 ;
            cpu->R8.A = temp;
            break;
        }
        case 0x9C: //SBC A,H
        {
            u32 temp = cpu->R8.A - cpu->R8.H - ((cpu->R8.F&F_CARRY)?1:0);
            cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.A^cpu->R8.H^temp) & 0x10 ) != 0 ;
            cpu->R8.A = temp;
            break;
        }
        case 0x9D: //SBC A,L
        {
            u32 temp = cpu->R8.A - cpu->R8.L - ((cpu->R8.F&F_CARRY)?1:0);
            cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
            cpu->F.H = ( (cpu->R8.A^cpu->R8.L^temp) & 0x10 ) != 0 ;
            cpu->R8.A = temp;
            break;
        }
        case 0x9E: //SBC A,(HL)
            if(emu->cpu_microinstruction == 1)
            {
                u32 temp2 = GB_MemRead8(cpu->R16.HL);
                u32 temp = cpu->R8.A - temp2 - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^temp2^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
            }
            break;
        case 0x9F: //SBC A,A
            cpu->R16.AF = cpu->R8.F&F_CARRY ?
                    ( (0xFF<<8)|F_CARRY|F_HALFCARRY|F_SUBTRACT ) : (F_ZERO|F_SUBTRACT) ;
            break;
        case 0xA0: //AND B
            cpu->R16.AF |= F_HALFCARRY;
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
            cpu->R8.A &= cpu->R8.B;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xA1: //AND C
            cpu->R16.AF |= F_HALFCARRY;
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
            cpu->R8.A &= cpu->R8.C;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xA2: //AND D
            cpu->R16.AF |= F_HALFCARRY;
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
            cpu->R8.A &= cpu->R8.D;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xA3: //AND E
            cpu->R16.AF |= F_HALFCARRY;
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
            cpu->R8.A &= cpu->R8.E;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xA4: //AND H
            cpu->R16.AF |= F_HALFCARRY;
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
            cpu->R8.A &= cpu->R8.H;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xA5: //AND L
            cpu->R16.AF |= F_HALFCARRY;
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
            cpu->R8.A &= cpu->R8.L;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xA6: //AND (HL)
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF |= F_HALFCARRY;
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R8.A &= GB_MemRead8(cpu->R16.HL);
                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0xA7: //AND A
            cpu->R16.AF |= F_HALFCARRY;
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
            //cpu->R8.A &= cpu->R8.A;
            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xA8: //XOR B
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A ^= cpu->R8.B;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xA9: //XOR C
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A ^= cpu->R8.C;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xAA: //XOR D
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A ^= cpu->R8.D;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xAB: //XOR E
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A ^= cpu->R8.E;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xAC: //XOR H
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A ^= cpu->R8.H;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xAD: //XOR L
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A ^= cpu->R8.L;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xAE: //XOR (HL)
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

                cpu->R8.A ^= GB_MemRead8(cpu->R16.HL);

                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0xAF: //XOR A
            cpu->R16.AF = F_ZERO;
            break;
        case 0xB0: //OR B
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A |= cpu->R8.B;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xB1: //OR C
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A |= cpu->R8.C;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xB2: //OR D
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A |= cpu->R8.D;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xB3: //OR E
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A |= cpu->R8.E;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xB4: //OR H
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A |= cpu->R8.H;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xB5: //OR L
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            cpu->R8.A |= cpu->R8.L;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xB6: //OR (HL)
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

                cpu->R8.A |= GB_MemRead8(cpu->R16.HL);

                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0xB7: //OR A
            cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

            //cpu->R8.A |= cpu->R8.A;

            cpu->F.Z = (cpu->R8.A == 0);
            break;
        case 0xB8: //CP B
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.B & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.B;
            cpu->F.Z = (cpu->R8.A == cpu->R8.B);
            break;
        case 0xB9: //CP C
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.C & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.C;
            cpu->F.Z = (cpu->R8.A == cpu->R8.C);
            break;
        case 0xBA: //CP D
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.D & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.D;
            cpu->F.Z = (cpu->R8.A == cpu->R8.D);
            break;
        case 0xBB: //CP E
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.E & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.E;
            cpu->F.Z = (cpu->R8.A == cpu->R8.E);
            break;
        case 0xBC: //CP H
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.H & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.H;
            cpu->F.Z = (cpu->R8.A == cpu->R8.H);
            break;
        case 0xBD: //CP L
            cpu->R16.AF |= F_SUBTRACT;
            cpu->F.H = (cpu->R8.A & 0xF) < (cpu->R8.L & 0xF);
            cpu->F.C = (u32)cpu->R8.A < (u32)cpu->R8.L;
            cpu->F.Z = (cpu->R8.A == cpu->R8.L);
            break;
        case 0xBE: //CP (HL)
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF |= F_SUBTRACT;
                u32 temp = GB_MemRead8(cpu->R16.HL);
                cpu->F.H = (cpu->R8.A & 0xF) < (temp & 0xF);
                cpu->F.C = (u32)cpu->R8.A < temp;
                cpu->F.Z = (cpu->R8.A == temp);
            }
            break;
        case 0xBF: //CP A
            cpu->R16.AF |= (F_SUBTRACT|F_ZERO);
            cpu->R16.AF &= ~(F_HALFCARRY|F_CARRY);
            break;
        case 0xC0: //RET NZ
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.Z == 0)
                {
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.SP++);
                    cpu->R16.SP &= 0xFFFF;
                }
                else
                {
                    emu->cpu_microinstruction += 3; // go to the end of the instruction
                }
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            //else if(emu->cpu_microinstruction == 4) nothing
            break;
        case 0xC1: //POP BC
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R8.C = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R8.B = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            break;
        case 0xC2: //JP NZ,nn
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.Z == 0)
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                else
                    cpu->R16.PC++;

                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                if(cpu->F.Z == 0)
                    emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                else
                {
                    cpu->R16.PC++;
                    emu->cpu_microinstruction++;// go to the end of the instruction
                }

                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xC3: //JP nn
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)(u8)GB_MemRead8(cpu->R16.PC++) ) << 8;
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xC4: //CALL NZ,nn
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.Z == 0)
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                else
                    cpu->R16.PC++;
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                if(cpu->F.Z == 0)
                {
                    emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                }
                else
                {
                    cpu->R16.PC++;
                    emu->cpu_microinstruction += 3; // go to the end of the instruction
                }
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 4)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 5)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xC5: //PUSH BC
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.B);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.C);
            }
            //else if(emu->cpu_microinstruction == 3) //nothing
            break;
        case 0xC6: //ADD A,n
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~F_SUBTRACT;

                u32 temp = cpu->R8.A;
                u32 temp2 = GB_MemRead8(cpu->R16.PC++);

                cpu->F.H = ( (temp & 0xF) + (temp2 & 0xF) ) > 0xF;

                cpu->R8.A += temp2;
                cpu->F.Z = (cpu->R8.A == 0);

                cpu->F.C = (temp > cpu->R8.A);
            }
            break;
        case 0xC7: //RST 0x00
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = 0x0000;
            break;
        case 0xC8: //RET Z
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.Z == 1)
                {
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.SP++);
                    cpu->R16.SP &= 0xFFFF;
                }
                else
                {
                    emu->cpu_microinstruction += 3; // go to the end of the instruction
                }
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            //else if(emu->cpu_microinstruction == 4) nothing
            break;
        case 0xC9: //RET
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xCA: //JP Z,nn
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.Z == 1)
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                else
                    cpu->R16.PC++;

                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                if(cpu->F.Z == 1)
                    emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                else
                {
                    cpu->R16.PC++;
                    emu->cpu_microinstruction++;// go to the end of the instruction
                }

                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xCB:

            if(emu->cpu_microinstruction == 0)
                break;

            //Get real opcode
            if(emu->cpu_microinstruction == 1)
            {
                emu->cb_opcode = (u32)(u8)GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
            }

            switch(emu->cb_opcode)
            {
                case 0x00: //RLC B
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.B & 0x80) != 0;
                    cpu->R8.B = (cpu->R8.B << 1) | cpu->F.C;
                    cpu->F.Z = (cpu->R8.B == 0);
                    break;
                case 0x01: //RLC C
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.C & 0x80) != 0;
                    cpu->R8.C = (cpu->R8.C << 1) | cpu->F.C;
                    cpu->F.Z = (cpu->R8.C == 0);
                    break;
                case 0x02: //RLC D
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.D & 0x80) != 0;
                    cpu->R8.D = (cpu->R8.D << 1) | cpu->F.C;
                    cpu->F.Z = (cpu->R8.D == 0);
                    break;
                case 0x03: //RLC E
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.E & 0x80) != 0;
                    cpu->R8.E = (cpu->R8.E << 1) | cpu->F.C;
                    cpu->F.Z = (cpu->R8.E == 0);
                    break;
                case 0x04: //RLC H
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.H & 0x80) != 0;
                    cpu->R8.H = (cpu->R8.H << 1) | cpu->F.C;
                    cpu->F.Z = (cpu->R8.H == 0);
                    break;
                case 0x05: //RLC L
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.L & 0x80) != 0;
                    cpu->R8.L = (cpu->R8.L << 1) | cpu->F.C;
                    cpu->F.Z = (cpu->R8.L == 0);
                    break;
                case 0x06: //RLC (HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        u32 temp = emu->cpu_temp_16;
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x80) != 0;
                        temp = (temp << 1) | cpu->F.C;
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                    }
                    break;
                case 0x07: //RLC A
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.A & 0x80) != 0;
                    cpu->R8.A = (cpu->R8.A << 1) | cpu->F.C;
                    cpu->F.Z = (cpu->R8.A == 0);
                    break;

                case 0x08: //RRC B
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.B & 0x01) != 0;
                    cpu->R8.B = (cpu->R8.B >> 1) | (cpu->F.C << 7);
                    cpu->F.Z = (cpu->R8.B == 0);
                    break;
                case 0x09: //RRC C
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.C & 0x01) != 0;
                    cpu->R8.C = (cpu->R8.C >> 1) | (cpu->F.C << 7);
                    cpu->F.Z = (cpu->R8.C == 0);
                    break;
                case 0x0A: //RRC D
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.D & 0x01) != 0;
                    cpu->R8.D = (cpu->R8.D >> 1) | (cpu->F.C << 7);
                    cpu->F.Z = (cpu->R8.D == 0);
                    break;
                case 0x0B: //RRC E
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.E & 0x01) != 0;
                    cpu->R8.E = (cpu->R8.E >> 1) | (cpu->F.C << 7);
                    cpu->F.Z = (cpu->R8.E == 0);
                    break;
                case 0x0C: //RRC H
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.H & 0x01) != 0;
                    cpu->R8.H = (cpu->R8.H >> 1) | (cpu->F.C << 7);
                    cpu->F.Z = (cpu->R8.H == 0);
                    break;
                case 0x0D: //RRC L
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.L & 0x01) != 0;
                    cpu->R8.L = (cpu->R8.L >> 1) | (cpu->F.C << 7);
                    cpu->F.Z = (cpu->R8.L == 0);
                    break;
                case 0x0E: //RRC (HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        u32 temp = emu->cpu_temp_16;
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x01) != 0;
                        temp = (temp >> 1) | (cpu->F.C << 7);
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                    }
                    break;
                case 0x0F: //RRC A
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.A & 0x01) != 0;
                    cpu->R8.A = (cpu->R8.A >> 1) | (cpu->F.C << 7);
                    cpu->F.Z = (cpu->R8.A == 0);
                    break;

                case 0x10: //RL B
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.B & 0x80) != 0;
                    cpu->R8.B = (cpu->R8.B << 1) | temp;
                    cpu->F.Z = (cpu->R8.B == 0);
                    break;
                }
                case 0x11: //RL C
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.C & 0x80) != 0;
                    cpu->R8.C = (cpu->R8.C << 1) | temp;
                    cpu->F.Z = (cpu->R8.C == 0);
                    break;
                }
                case 0x12: //RL D
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.D & 0x80) != 0;
                    cpu->R8.D = (cpu->R8.D << 1) | temp;
                    cpu->F.Z = (cpu->R8.D == 0);
                    break;
                }
                case 0x13: //RL E
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.E & 0x80) != 0;
                    cpu->R8.E = (cpu->R8.E << 1) | temp;
                    cpu->F.Z = (cpu->R8.E == 0);
                    break;
                }
                case 0x14: //RL H
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.H & 0x80) != 0;
                    cpu->R8.H = (cpu->R8.H << 1) | temp;
                    cpu->F.Z = (cpu->R8.H == 0);
                    break;
                }
                case 0x15: //RL L
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.L & 0x80) != 0;
                    cpu->R8.L = (cpu->R8.L << 1) | temp;
                    cpu->F.Z = (cpu->R8.L == 0);
                    break;
                }
                case 0x16: //RL (HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        u32 temp2 = emu->cpu_temp_16;
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (temp2 & 0x80) != 0;
                        temp2 = ((temp2 << 1) | temp) & 0xFF;
                        cpu->F.Z = (temp2 == 0);
                        GB_MemWrite8(cpu->R16.HL, temp2);
                    }
                    break;
                case 0x17: //RL A
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.A & 0x80) != 0;
                    cpu->R8.A = (cpu->R8.A << 1) | temp;
                    cpu->F.Z = (cpu->R8.A == 0);
                    break;
                }

                case 0x18: //RR B
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.B & 0x01) != 0;
                    cpu->R8.B = (cpu->R8.B >> 1) | (temp << 7);
                    cpu->F.Z = (cpu->R8.B == 0);
                    break;
                }
                case 0x19: //RR C
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.C & 0x01) != 0;
                    cpu->R8.C = (cpu->R8.C >> 1) | (temp << 7);
                    cpu->F.Z = (cpu->R8.C == 0);
                    break;
                }
                case 0x1A: //RR D
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.D & 0x01) != 0;
                    cpu->R8.D = (cpu->R8.D >> 1) | (temp << 7);
                    cpu->F.Z = (cpu->R8.D == 0);
                    break;
                }
                case 0x1B: //RR E
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.E & 0x01) != 0;
                    cpu->R8.E = (cpu->R8.E >> 1) | (temp << 7);
                    cpu->F.Z = (cpu->R8.E == 0);
                    break;
                }
                case 0x1C: //RR H
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.H & 0x01) != 0;
                    cpu->R8.H = (cpu->R8.H >> 1) | (temp << 7);
                    cpu->F.Z = (cpu->R8.H == 0);
                    break;
                }
                case 0x1D: //RR L
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.L & 0x01) != 0;
                    cpu->R8.L = (cpu->R8.L >> 1) | (temp << 7);
                    cpu->F.Z = (cpu->R8.L == 0);
                    break;
                }
                case 0x1E: //RR (HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        u32 temp2 = emu->cpu_temp_16;
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        u32 temp = cpu->F.C; //old carry flag
                        cpu->F.C = (temp2 & 0x01) != 0;
                        temp2 = (temp2 >> 1) | (temp << 7);
                        cpu->F.Z = (temp2 == 0);
                        GB_MemWrite8(cpu->R16.HL, temp2);
                    }
                    break;
                case 0x1F: //RR A
                {
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    u32 temp = cpu->F.C; //old carry flag
                    cpu->F.C = (cpu->R8.A & 0x01) != 0;
                    cpu->R8.A = (cpu->R8.A >> 1) | (temp << 7);
                    cpu->F.Z = (cpu->R8.A == 0);
                    break;
                }

                case 0x20: //SLA B
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.B & 0x80) != 0;
                    cpu->R8.B = cpu->R8.B << 1;
                    cpu->F.Z = (cpu->R8.B == 0);
                    break;
                case 0x21: //SLA C
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.C & 0x80) != 0;
                    cpu->R8.C = cpu->R8.C << 1;
                    cpu->F.Z = (cpu->R8.C == 0);
                    break;
                case 0x22: //SLA D
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.D & 0x80) != 0;
                    cpu->R8.D = cpu->R8.D << 1;
                    cpu->F.Z = (cpu->R8.D == 0);
                    break;
                case 0x23: //SLA E
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.E & 0x80) != 0;
                    cpu->R8.E = cpu->R8.E << 1;
                    cpu->F.Z = (cpu->R8.E == 0);
                    break;
                case 0x24: //SLA H
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.H & 0x80) != 0;
                    cpu->R8.H = cpu->R8.H << 1;
                    cpu->F.Z = (cpu->R8.H == 0);
                    break;
                case 0x25: //SLA L
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.L & 0x80) != 0;
                    cpu->R8.L = cpu->R8.L << 1;
                    cpu->F.Z = (cpu->R8.L == 0);
                    break;
                case 0x26: //SLA (HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        u32 temp = emu->cpu_temp_16;
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x80) != 0;
                        temp = (temp << 1) & 0xFF;
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                    }
                    break;
                case 0x27: //SLA A
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.A & 0x80) != 0;
                    cpu->R8.A = cpu->R8.A << 1;
                    cpu->F.Z = (cpu->R8.A == 0);
                    break;

                case 0x28: //SRA B
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.B & 0x01) != 0;
                    cpu->R8.B = (cpu->R8.B & 0x80) | (cpu->R8.B >> 1);
                    cpu->F.Z = (cpu->R8.B == 0);
                    break;
                case 0x29: //SRA C
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.C & 0x01) != 0;
                    cpu->R8.C = (cpu->R8.C & 0x80) | (cpu->R8.C >> 1);
                    cpu->F.Z = (cpu->R8.C == 0);
                    break;
                case 0x2A: //SRA D
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.D & 0x01) != 0;
                    cpu->R8.D = (cpu->R8.D & 0x80) | (cpu->R8.D >> 1);
                    cpu->F.Z = (cpu->R8.D == 0);
                    break;
                case 0x2B: //SRA E
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.E & 0x01) != 0;
                    cpu->R8.E = (cpu->R8.E & 0x80) | (cpu->R8.E >> 1);
                    cpu->F.Z = (cpu->R8.E == 0);
                    break;
                case 0x2C: //SRA H
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.H & 0x01) != 0;
                    cpu->R8.H = (cpu->R8.H & 0x80) | (cpu->R8.H >> 1);
                    cpu->F.Z = (cpu->R8.H == 0);
                    break;
                case 0x2D: //SRA L
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.L & 0x01) != 0;
                    cpu->R8.L = (cpu->R8.L & 0x80) | (cpu->R8.L >> 1);
                    cpu->F.Z = (cpu->R8.L == 0);
                    break;
                case 0x2E: //SRA (HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        u32 temp = emu->cpu_temp_16;
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x01) != 0;
                        temp = (temp & 0x80) | (temp >> 1);
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                    }
                    break;
                case 0x2F: //SRA A
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.A & 0x01) != 0;
                    cpu->R8.A = (cpu->R8.A & 0x80) | (cpu->R8.A >> 1);
                    cpu->F.Z = (cpu->R8.A == 0);
                    break;

                case 0x30: //SWAP B
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                    cpu->R8.B = ((cpu->R8.B >> 4) | (cpu->R8.B << 4));
                    cpu->F.Z = (cpu->R8.B == 0);
                    break;
                case 0x31: //SWAP C
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                    cpu->R8.C = ((cpu->R8.C >> 4) | (cpu->R8.C << 4));
                    cpu->F.Z = (cpu->R8.C == 0);
                    break;
                case 0x32: //SWAP D
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                    cpu->R8.D = ((cpu->R8.D >> 4) | (cpu->R8.D << 4));
                    cpu->F.Z = (cpu->R8.D == 0);
                    break;
                case 0x33: //SWAP E
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                    cpu->R8.E = ((cpu->R8.E >> 4) | (cpu->R8.E << 4));
                    cpu->F.Z = (cpu->R8.E == 0);
                    break;
                case 0x34: //SWAP H
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                    cpu->R8.H = ((cpu->R8.H >> 4) | (cpu->R8.H << 4));
                    cpu->F.Z = (cpu->R8.H == 0);
                    break;
                case 0x35: //SWAP L
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                    cpu->R8.L = ((cpu->R8.L >> 4) | (cpu->R8.L << 4));
                    cpu->F.Z = (cpu->R8.L == 0);
                    break;
                case 0x36: //SWAP (HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        u32 temp = emu->cpu_temp_16;
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                        temp = ((temp >> 4) | (temp << 4)) & 0xFF;
                        GB_MemWrite8(cpu->R16.HL,temp);
                        cpu->F.Z = (temp == 0);
                    }
                    break;
                case 0x37: //SWAP A
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY|F_CARRY);
                    cpu->R8.A = ((cpu->R8.A & 0xF0) >> 4) | ((cpu->R8.A & 0x0F) << 4);
                    cpu->F.Z = (cpu->R8.A == 0);
                    break;

                case 0x38: //SRL B
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.B & 0x01) != 0;
                    cpu->R8.B = cpu->R8.B >> 1;
                    cpu->F.Z = (cpu->R8.B == 0);
                    break;
                case 0x39: //SRL C
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.C & 0x01) != 0;
                    cpu->R8.C = cpu->R8.C >> 1;
                    cpu->F.Z = (cpu->R8.C == 0);
                    break;
                case 0x3A: //SRL D
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.D & 0x01) != 0;
                    cpu->R8.D = cpu->R8.D >> 1;
                    cpu->F.Z = (cpu->R8.D == 0);
                    break;
                case 0x3B: //SRL E
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.E & 0x01) != 0;
                    cpu->R8.E = cpu->R8.E >> 1;
                    cpu->F.Z = (cpu->R8.E == 0);
                    break;
                case 0x3C: //SRL H
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.H & 0x01) != 0;
                    cpu->R8.H = cpu->R8.H >> 1;
                    cpu->F.Z = (cpu->R8.H == 0);
                    break;
                case 0x3D: //SRL L
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.L & 0x01) != 0;
                    cpu->R8.L = cpu->R8.L >> 1;
                    cpu->F.Z = (cpu->R8.L == 0);
                    break;
                case 0x3E: //SRL (HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        u32 temp = emu->cpu_temp_16;
                        cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                        cpu->F.C = (temp & 0x01) != 0;
                        temp = temp >> 1;
                        cpu->F.Z = (temp == 0);
                        GB_MemWrite8(cpu->R16.HL, temp);
                    }
                    break;
                case 0x3F: //SRL A
                    cpu->R16.AF &= ~(F_SUBTRACT|F_HALFCARRY);
                    cpu->F.C = (cpu->R8.A & 0x01) != 0;
                    cpu->R8.A = cpu->R8.A >> 1;
                    cpu->F.Z = (cpu->R8.A == 0);
                    break;

                case 0x40: //BIT 0,B
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.B & (1<<0)) == 0;
                    break;
                case 0x41: //BIT 0,C
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.C & (1<<0)) == 0;
                    break;
                case 0x42: //BIT 0,D
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.D & (1<<0)) == 0;
                    break;
                case 0x43: //BIT 0,E
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.E & (1<<0)) == 0;
                    break;
                case 0x44: //BIT 0,H
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.H & (1<<0)) == 0;
                    break;
                case 0x45: //BIT 0,L
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.L & (1<<0)) == 0;
                    break;
                case 0x46: //BIT 0,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<0)) == 0;
                    }
                    break;
                case 0x47: //BIT 0,A
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.A & (1<<0)) == 0;
                    break;

                case 0x48: //BIT 1,B
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.B & (1<<1)) == 0;
                    break;
                case 0x49: //BIT 1,C
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.C & (1<<1)) == 0;
                    break;
                case 0x4A: //BIT 1,D
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.D & (1<<1)) == 0;
                    break;
                case 0x4B: //BIT 1,E
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.E & (1<<1)) == 0;
                    break;
                case 0x4C: //BIT 1,H
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.H & (1<<1)) == 0;
                    break;
                case 0x4D: //BIT 1,L
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.L & (1<<1)) == 0;
                    break;
                case 0x4E: //BIT 1,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<1)) == 0;
                    }
                    break;
                case 0x4F: //BIT 1,A
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.A & (1<<1)) == 0;
                    break;
                case 0x50: //BIT 2,B
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.B & (1<<2)) == 0;
                    break;
                case 0x51: //BIT 2,C
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.C & (1<<2)) == 0;
                    break;
                case 0x52: //BIT 2,D
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.D & (1<<2)) == 0;
                    break;
                case 0x53: //BIT 2,E
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.E & (1<<2)) == 0;
                    break;
                case 0x54: //BIT 2,H
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.H & (1<<2)) == 0;
                    break;
                case 0x55: //BIT 2,L
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.L & (1<<2)) == 0;
                    break;
                case 0x56: //BIT 2,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<2)) == 0;
                    }
                    break;
                case 0x57: //BIT 2,A
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.A & (1<<2)) == 0;
                    break;
                case 0x58: //BIT 3,B
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.B & (1<<3)) == 0;
                    break;
                case 0x59: //BIT 3,C
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.C & (1<<3)) == 0;
                    break;
                case 0x5A: //BIT 3,D
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.D & (1<<3)) == 0;
                    break;
                case 0x5B: //BIT 3,E
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.E & (1<<3)) == 0;
                    break;
                case 0x5C: //BIT 3,H
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.H & (1<<3)) == 0;
                    break;
                case 0x5D: //BIT 3,L
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.L & (1<<3)) == 0;
                    break;
                case 0x5E: //BIT 3,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<3)) == 0;
                    }
                    break;
                case 0x5F: //BIT 3,A
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.A & (1<<3)) == 0;
                    break;
                case 0x60: //BIT 4,B
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.B & (1<<4)) == 0;
                    break;
                case 0x61: //BIT 4,C
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.C & (1<<4)) == 0;
                    break;
                case 0x62: //BIT 4,D
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.D & (1<<4)) == 0;
                    break;
                case 0x63: //BIT 4,E
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.E & (1<<4)) == 0;
                    break;
                case 0x64: //BIT 4,H
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.H & (1<<4)) == 0;
                    break;
                case 0x65: //BIT 4,L
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.L & (1<<4)) == 0;
                    break;
                case 0x66: //BIT 4,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<4)) == 0;
                    }
                    break;
                case 0x67: //BIT 4,A
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.A & (1<<4)) == 0;
                    break;
                case 0x68: //BIT 5,B
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.B & (1<<5)) == 0;
                    break;
                case 0x69: //BIT 5,C
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.C & (1<<5)) == 0;
                    break;
                case 0x6A: //BIT 5,D
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.D & (1<<5)) == 0;
                    break;
                case 0x6B: //BIT 5,E
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.E & (1<<5)) == 0;
                    break;
                case 0x6C: //BIT 5,H
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.H & (1<<5)) == 0;
                    break;
                case 0x6D: //BIT 5,L
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.L & (1<<5)) == 0;
                    break;
                case 0x6E: //BIT 5,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<5)) == 0;
                    }
                    break;
                case 0x6F: //BIT 5,A
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.A & (1<<5)) == 0;
                    break;
                case 0x70: //BIT 6,B
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.B & (1<<6)) == 0;
                    break;
                case 0x71: //BIT 6,C
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.C & (1<<6)) == 0;
                    break;
                case 0x72: //BIT 6,D
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.D & (1<<6)) == 0;
                    break;
                case 0x73: //BIT 6,E
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.E & (1<<6)) == 0;
                    break;
                case 0x74: //BIT 6,H
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.H & (1<<6)) == 0;
                    break;
                case 0x75: //BIT 6,L
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.L & (1<<6)) == 0;
                    break;
                case 0x76: //BIT 6,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<6)) == 0;
                    }
                    break;
                case 0x77: //BIT 6,A
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.A & (1<<6)) == 0;
                    break;
                case 0x78: //BIT 7,B
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.B & (1<<7)) == 0;
                    break;
                case 0x79: //BIT 7,C
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.C & (1<<7)) == 0;
                    break;
                case 0x7A: //BIT 7,D
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.D & (1<<7)) == 0;
                    break;
                case 0x7B: //BIT 7,E
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.E & (1<<7)) == 0;
                    break;
                case 0x7C: //BIT 7,H
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.H & (1<<7)) == 0;
                    break;
                case 0x7D: //BIT 7,L
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.L & (1<<7)) == 0;
                    break;
                case 0x7E: //BIT 7,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        cpu->R16.AF &= ~F_SUBTRACT;
                        cpu->R16.AF |= F_HALFCARRY;
                        cpu->F.Z = (GB_MemRead8(cpu->R16.HL) & (1<<7)) == 0;
                    }
                    break;
                case 0x7F: //BIT 7,A
                    cpu->R16.AF &= ~F_SUBTRACT;
                    cpu->R16.AF |= F_HALFCARRY;
                    cpu->F.Z = (cpu->R8.A & (1<<7)) == 0;
                    break;

                case 0x80: //RES 0,B
                    cpu->R8.B &= ~(1 << 0);
                    break;
                case 0x81: //RES 0,C
                    cpu->R8.C &= ~(1 << 0);
                    break;
                case 0x82: //RES 0,D
                    cpu->R8.D &= ~(1 << 0);
                    break;
                case 0x83: //RES 0,E
                    cpu->R8.E &= ~(1 << 0);
                    break;
                case 0x84: //RES 0,H
                    cpu->R8.H &= ~(1 << 0);
                    break;
                case 0x85: //RES 0,L
                    cpu->R8.L &= ~(1 << 0);
                    break;
                case 0x86: //RES 0,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 & (~(1 << 0)));
                    }
                    break;
                case 0x87: //RES 0,A
                    cpu->R8.A &= ~(1 << 0);
                    break;
                case 0x88: //RES 1,B
                    cpu->R8.B &= ~(1 << 1);
                    break;
                case 0x89: //RES 1,C
                    cpu->R8.C &= ~(1 << 1);
                    break;
                case 0x8A: //RES 1,D
                    cpu->R8.D &= ~(1 << 1);
                    break;
                case 0x8B: //RES 1,E
                    cpu->R8.E &= ~(1 << 1);
                    break;
                case 0x8C: //RES 1,H
                    cpu->R8.H &= ~(1 << 1);
                    break;
                case 0x8D: //RES 1,L
                    cpu->R8.L &= ~(1 << 1);
                    break;
                case 0x8E: //RES 1,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 & (~(1 << 1)));
                    }
                    break;
                case 0x8F: //RES 1,A
                    cpu->R8.A &= ~(1 << 1);
                    break;
                case 0x90: //RES 2,B
                    cpu->R8.B &= ~(1 << 2);
                    break;
                case 0x91: //RES 2,C
                    cpu->R8.C &= ~(1 << 2);
                    break;
                case 0x92: //RES 2,D
                    cpu->R8.D &= ~(1 << 2);
                    break;
                case 0x93: //RES 2,E
                    cpu->R8.E &= ~(1 << 2);
                    break;
                case 0x94: //RES 2,H
                    cpu->R8.H &= ~(1 << 2);
                    break;
                case 0x95: //RES 2,L
                    cpu->R8.L &= ~(1 << 2);
                    break;
                case 0x96: //RES 2,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 & (~(1 << 2)));
                    }
                    break;
                case 0x97: //RES 2,A
                    cpu->R8.A &= ~(1 << 2);
                    break;
                case 0x98: //RES 3,B
                    cpu->R8.B &= ~(1 << 3);
                    break;
                case 0x99: //RES 3,C
                    cpu->R8.C &= ~(1 << 3);
                    break;
                case 0x9A: //RES 3,D
                    cpu->R8.D &= ~(1 << 3);
                    break;
                case 0x9B: //RES 3,E
                    cpu->R8.E &= ~(1 << 3);
                    break;
                case 0x9C: //RES 3,H
                    cpu->R8.H &= ~(1 << 3);
                    break;
                case 0x9D: //RES 3,L
                    cpu->R8.L &= ~(1 << 3);
                    break;
                case 0x9E: //RES 3,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 & (~(1 << 3)));
                    }
                    break;
                case 0x9F: //RES 3,A
                    cpu->R8.A &= ~(1 << 3);
                    break;
                case 0xA0: //RES 4,B
                    cpu->R8.B &= ~(1 << 4);
                    break;
                case 0xA1: //RES 4,C
                    cpu->R8.C &= ~(1 << 4);
                    break;
                case 0xA2: //RES 4,D
                    cpu->R8.D &= ~(1 << 4);
                    break;
                case 0xA3: //RES 4,E
                    cpu->R8.E &= ~(1 << 4);
                    break;
                case 0xA4: //RES 4,H
                    cpu->R8.H &= ~(1 << 4);
                    break;
                case 0xA5: //RES 4,L
                    cpu->R8.L &= ~(1 << 4);
                    break;
                case 0xA6: //RES 4,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 & (~(1 << 4)));
                    }
                    break;
                case 0xA7: //RES 4,A
                    cpu->R8.A &= ~(1 << 4);
                    break;
                case 0xA8: //RES 5,B
                    cpu->R8.B &= ~(1 << 5);
                    break;
                case 0xA9: //RES 5,C
                    cpu->R8.C &= ~(1 << 5);
                    break;
                case 0xAA: //RES 5,D
                    cpu->R8.D &= ~(1 << 5);
                    break;
                case 0xAB: //RES 5,E
                    cpu->R8.E &= ~(1 << 5);
                    break;
                case 0xAC: //RES 5,H
                    cpu->R8.H &= ~(1 << 5);
                    break;
                case 0xAD: //RES 5,L
                    cpu->R8.L &= ~(1 << 5);
                    break;
                case 0xAE: //RES 5,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 & (~(1 << 5)));
                    }
                    break;
                case 0xAF: //RES 5,A
                    cpu->R8.A &= ~(1 << 5);
                    break;
                case 0xB0: //RES 6,B
                    cpu->R8.B &= ~(1 << 6);
                    break;
                case 0xB1: //RES 6,C
                    cpu->R8.C &= ~(1 << 6);
                    break;
                case 0xB2: //RES 6,D
                    cpu->R8.D &= ~(1 << 6);
                    break;
                case 0xB3: //RES 6,E
                    cpu->R8.E &= ~(1 << 6);
                    break;
                case 0xB4: //RES 6,H
                    cpu->R8.H &= ~(1 << 6);
                    break;
                case 0xB5: //RES 6,L
                    cpu->R8.L &= ~(1 << 6);
                    break;
                case 0xB6: //RES 6,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 & (~(1 << 6)));
                    }
                    break;
                case 0xB7: //RES 6,A
                    cpu->R8.A &= ~(1 << 6);
                    break;
                case 0xB8: //RES 7,B
                    cpu->R8.B &= ~(1 << 7);
                    break;
                case 0xB9: //RES 7,C
                    cpu->R8.C &= ~(1 << 7);
                    break;
                case 0xBA: //RES 7,D
                    cpu->R8.D &= ~(1 << 7);
                    break;
                case 0xBB: //RES 7,E
                    cpu->R8.E &= ~(1 << 7);
                    break;
                case 0xBC: //RES 7,H
                    cpu->R8.H &= ~(1 << 7);
                    break;
                case 0xBD: //RES 7,L
                    cpu->R8.L &= ~(1 << 7);
                    break;
                case 0xBE: //RES 7,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 & (~(1 << 7)));
                    }
                    break;
                case 0xBF: //RES 7,A
                    cpu->R8.A &= ~(1 << 7);
                    break;

                case 0xC0: //SET 0,B
                    cpu->R8.B |= (1 << 0);
                    break;
                case 0xC1: //SET 0,C
                    cpu->R8.C |= (1 << 0);
                    break;
                case 0xC2: //SET 0,D
                    cpu->R8.D |= (1 << 0);
                    break;
                case 0xC3: //SET 0,E
                    cpu->R8.E |= (1 << 0);
                    break;
                case 0xC4: //SET 0,H
                    cpu->R8.H |= (1 << 0);
                    break;
                case 0xC5: //SET 0,L
                    cpu->R8.L |= (1 << 0);
                    break;
                case 0xC6: //SET 0,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 | (1 << 0));
                    }
                    break;
                case 0xC7: //SET 0,A
                    cpu->R8.A |= (1 << 0);
                    break;
                case 0xC8: //SET 1,B
                    cpu->R8.B |= (1 << 1);
                    break;
                case 0xC9: //SET 1,C
                    cpu->R8.C |= (1 << 1);
                    break;
                case 0xCA: //SET 1,D
                    cpu->R8.D |= (1 << 1);
                    break;
                case 0xCB: //SET 1,E
                    cpu->R8.E |= (1 << 1);
                    break;
                case 0xCC: //SET 1,H
                    cpu->R8.H |= (1 << 1);
                    break;
                case 0xCD: //SET 1,L
                    cpu->R8.L |= (1 << 1);
                    break;
                case 0xCE: //SET 1,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 | (1 << 1));
                    }
                    break;
                case 0xCF: //SET 1,A
                    cpu->R8.A |= (1 << 1);
                    break;
                case 0xD0: //SET 2,B
                    cpu->R8.B |= (1 << 2);
                    break;
                case 0xD1: //SET 2,C
                    cpu->R8.C |= (1 << 2);
                    break;
                case 0xD2: //SET 2,D
                    cpu->R8.D |= (1 << 2);
                    break;
                case 0xD3: //SET 2,E
                    cpu->R8.E |= (1 << 2);
                    break;
                case 0xD4: //SET 2,H
                    cpu->R8.H |= (1 << 2);
                    break;
                case 0xD5: //SET 2,L
                    cpu->R8.L |= (1 << 2);
                    break;
                case 0xD6: //SET 2,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 | (1 << 2));
                    }
                    break;
                case 0xD7: //SET 2,A
                    cpu->R8.A |= (1 << 2);
                    break;
                case 0xD8: //SET 3,B
                    cpu->R8.B |= (1 << 3);
                    break;
                case 0xD9: //SET 3,C
                    cpu->R8.C |= (1 << 3);
                    break;
                case 0xDA: //SET 3,D
                    cpu->R8.D |= (1 << 3);
                    break;
                case 0xDB: //SET 3,E
                    cpu->R8.E |= (1 << 3);
                    break;
                case 0xDC: //SET 3,H
                    cpu->R8.H |= (1 << 3);
                    break;
                case 0xDD: //SET 3,L
                    cpu->R8.L |= (1 << 3);
                    break;
                case 0xDE: //SET 3,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 | (1 << 3));
                    }
                    break;
                case 0xDF: //SET 3,A
                    cpu->R8.A |= (1 << 3);
                    break;
                case 0xE0: //SET 4,B
                    cpu->R8.B |= (1 << 4);
                    break;
                case 0xE1: //SET 4,C
                    cpu->R8.C |= (1 << 4);
                    break;
                case 0xE2: //SET 4,D
                    cpu->R8.D |= (1 << 4);
                    break;
                case 0xE3: //SET 4,E
                    cpu->R8.E |= (1 << 4);
                    break;
                case 0xE4: //SET 4,H
                    cpu->R8.H |= (1 << 4);
                    break;
                case 0xE5: //SET 4,L
                    cpu->R8.L |= (1 << 4);
                    break;
                case 0xE6: //SET 4,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 | (1 << 4));
                    }
                    break;
                case 0xE7: //SET 4,A
                    cpu->R8.A |= (1 << 4);
                    break;
                case 0xE8: //SET 5,B
                    cpu->R8.B |= (1 << 5);
                    break;
                case 0xE9: //SET 5,C
                    cpu->R8.C |= (1 << 5);
                    break;
                case 0xEA: //SET 5,D
                    cpu->R8.D |= (1 << 5);
                    break;
                case 0xEB: //SET 5,E
                    cpu->R8.E |= (1 << 5);
                    break;
                case 0xEC: //SET 5,H
                    cpu->R8.H |= (1 << 5);
                    break;
                case 0xED: //SET 5,L
                    cpu->R8.L |= (1 << 5);
                    break;
                case 0xEE: //SET 5,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 | (1 << 5));
                    }
                    break;
                case 0xEF: //SET 5,A
                    cpu->R8.A |= (1 << 5);
                    break;
                case 0xF0: //SET 6,B
                    cpu->R8.B |= (1 << 6);
                    break;
                case 0xF1: //SET 6,C
                    cpu->R8.C |= (1 << 6);
                    break;
                case 0xF2: //SET 6,D
                    cpu->R8.D |= (1 << 6);
                    break;
                case 0xF3: //SET 6,E
                    cpu->R8.E |= (1 << 6);
                    break;
                case 0xF4: //SET 6,H
                    cpu->R8.H |= (1 << 6);
                    break;
                case 0xF5: //SET 6,L
                    cpu->R8.L |= (1 << 6);
                    break;
                case 0xF6: //SET 6,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 | (1 << 6));
                    }
                    break;
                case 0xF7: //SET 6,A
                    cpu->R8.A |= (1 << 6);
                    break;
                case 0xF8: //SET 7,B
                    cpu->R8.B |= (1 << 7);
                    break;
                case 0xF9: //SET 7,C
                    cpu->R8.C |= (1 << 7);
                    break;
                case 0xFA: //SET 7,D
                    cpu->R8.D |= (1 << 7);
                    break;
                case 0xFB: //SET 7,E
                    cpu->R8.E |= (1 << 7);
                    break;
                case 0xFC: //SET 7,H
                    cpu->R8.H |= (1 << 7);
                    break;
                case 0xFD: //SET 7,L
                    cpu->R8.L |= (1 << 7);
                    break;
                case 0xFE: //SET 7,(HL)
                    if(emu->cpu_microinstruction == 2)
                    {
                        emu->cpu_temp_16 = GB_MemRead8(cpu->R16.HL);
                    }
                    else if(emu->cpu_microinstruction == 3)
                    {
                        GB_MemWrite8(cpu->R16.HL, emu->cpu_temp_16 | (1 << 7));
                    }
                    break;
                case 0xFF: //SET 7,A
                    cpu->R8.A |= (1 << 7);
                    break;

                default:
                    // ...wtf??
                    _gb_break_to_debugger();
                    Debug_ErrorMsgArg("Unidentified opcode. CB %x\nPC: %04x\nROM: %d",
                        emu->cb_opcode,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
                    break;
            } //end of inner switch

            break;
        case 0xCC: //CALL Z,nn
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.Z == 1)
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                else
                    cpu->R16.PC++;
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                if(cpu->F.Z == 1)
                {
                    emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                }
                else
                {
                    cpu->R16.PC++;
                    emu->cpu_microinstruction += 3; // go to the end of the instruction
                }
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 4)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 5)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xCD: //CALL nn
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 4)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 5)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xCE: //ADC A,n
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~F_SUBTRACT;
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                u32 temp2 = cpu->R8.A + temp + cpu->F.C;

                cpu->F.H = ( ((cpu->R8.A & 0xF) + (temp & 0xF) ) + cpu->F.C) > 0xF;
                cpu->F.C = (temp2 > 0xFF);

                cpu->R8.A = (temp2 & 0xFF);
                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0xCF: //RST 0x08
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = 0x0008;
            break;
        case 0xD0: //RET NC
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.C == 0)
                {
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.SP++);
                    cpu->R16.SP &= 0xFFFF;
                }
                else
                {
                    emu->cpu_microinstruction += 3; // go to the end of the instruction
                }
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            //else if(emu->cpu_microinstruction == 4) nothing
            break;
        case 0xD1: //POP DE
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R8.E = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R8.D = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            break;
        case 0xD2: //JP NC,nn
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.C == 0)
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                else
                    cpu->R16.PC++;

                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                if(cpu->F.C == 0)
                    emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                else
                {
                    cpu->R16.PC++;
                    emu->cpu_microinstruction++;// go to the end of the instruction
                }

                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xD3:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. D3\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xD4: //CALL NC,nn
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.C == 0)
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                else
                    cpu->R16.PC++;
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                if(cpu->F.C == 0)
                {
                    emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                }
                else
                {
                    cpu->R16.PC++;
                    emu->cpu_microinstruction += 3; // go to the end of the instruction
                }
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 4)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 5)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xD5: //PUSH DE
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.D);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.E);
            }
            //else if(emu->cpu_microinstruction == 3) //nothing
            break;
        case 0xD6:  //SUB n
            if(emu->cpu_microinstruction == 1)
            {
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.AF |= F_SUBTRACT;

                cpu->F.H = (cpu->R8.A & 0xF) < (temp & 0xF);
                cpu->F.C = (u32)cpu->R8.A < temp;

                cpu->R8.A -= temp;

                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0xD7: //RST 0x10
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = 0x0010;
            break;
        case 0xD8: //RET C
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.C == 1)
                {
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.SP++);
                    cpu->R16.SP &= 0xFFFF;
                }
                else
                {
                    emu->cpu_microinstruction += 3; // go to the end of the instruction
                }
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            //else if(emu->cpu_microinstruction == 4) nothing
            break;
        case 0xD9: //RETI
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.SP++) ) << 8;
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
            {
                cpu->R16.PC = emu->cpu_temp_16;
                GameBoy.Memory.InterruptMasterEnable = 1;
            }
            break;
        case 0xDA: //JP C,nn
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.C == 1)
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                else
                    cpu->R16.PC++;

                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                if(cpu->F.C == 1)
                    emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                else
                {
                    cpu->R16.PC++;
                    emu->cpu_microinstruction++;// go to the end of the instruction
                }

                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xDB:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. DB\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xDC: //CALL C,nn
            if(emu->cpu_microinstruction == 1)
            {
                if(cpu->F.C == 1)
                    emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                else
                    cpu->R16.PC++;
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                if(cpu->F.C == 1)
                {
                    emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                }
                else
                {
                    cpu->R16.PC++;
                    emu->cpu_microinstruction += 3; // go to the end of the instruction
                }
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 4)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 5)
                cpu->R16.PC = emu->cpu_temp_16;
            break;
        case 0xDD:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. DD\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xDE: //SBC A,n
            if(emu->cpu_microinstruction == 1)
            {
                u32 temp2 = GB_MemRead8(cpu->R16.PC++);
                u32 temp = cpu->R8.A - temp2 - ((cpu->R8.F&F_CARRY)?1:0);
                cpu->R8.F = ((temp & ~0xFF)?F_CARRY:0)|((temp&0xFF)?0:F_ZERO)|F_SUBTRACT;
                cpu->F.H = ( (cpu->R8.A^temp2^temp) & 0x10 ) != 0 ;
                cpu->R8.A = temp;
            }
            break;
        case 0xDF: //RST 0x18
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = 0x0018;
            break;
        case 0xE0: //LDH (n),A
            if(emu->cpu_microinstruction == 1)
                emu->cpu_temp_16 = 0xFF00 + (u32)GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                GB_MemWrite8(emu->cpu_temp_16,cpu->R8.A);
            break;
        case 0xE1: //POP HL
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R8.L = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R8.H = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            break;
        case 0xE2: //LD (C),A
            if(emu->cpu_microinstruction == 1)
                GB_MemWrite8(0xFF00 + (u32)cpu->R8.C,cpu->R8.A);
            break;
        case 0xE3:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. E3\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xE4:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. E4\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xE5: //PUSH HL
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.H);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.L);
            }
            //else if(emu->cpu_microinstruction == 3) //nothing
            break;
        case 0xE6: //AND n
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY);
                cpu->R16.AF |= F_HALFCARRY;

                cpu->R8.A &= GB_MemRead8(cpu->R16.PC++);

                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0xE7: //RST 0x20
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = 0x0020;
            break;
        case 0xE8: //ADD SP,n
            if(emu->cpu_microinstruction == 1)
                emu->cpu_temp_16 = (u16)(s16)(s8)GB_MemRead8(cpu->R16.PC++); //expand sign
            //else if(emu->cpu_microinstruction == 2)
            else if(emu->cpu_microinstruction == 3)
            {
                u32 temp = emu->cpu_temp_16;

                cpu->R8.F = 0;
                cpu->F.C = ( ( (cpu->R16.SP & 0x00FF) + (temp & 0x00FF) ) > 0x00FF );
                cpu->F.H = ( ( (cpu->R16.SP & 0x000F) + (temp & 0x000F) ) > 0x000F );

                cpu->R16.SP = (cpu->R16.SP + temp) & 0xFFFF;
            }
            break;
        case 0xE9: //JP HL
            cpu->R16.PC = cpu->R16.HL;
            break;
        case 0xEA: //LD (nn),A
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                GB_MemWrite8(emu->cpu_temp_16,cpu->R8.A);
            break;
        case 0xEB:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. EB\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xEC:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. EC\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xED:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. ED\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xEE: //XOR n
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);

                cpu->R8.A ^= GB_MemRead8(cpu->R16.PC++);

                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0xEF: //RST 0x28
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = 0x0028;
            break;
        case 0xF0: //LDH A,(n)
            if(emu->cpu_microinstruction == 1)
                emu->cpu_temp_16 = 0xFF00 + (u32)GB_MemRead8(cpu->R16.PC++);
            else if(emu->cpu_microinstruction == 2)
                cpu->R8.A = GB_MemRead8(emu->cpu_temp_16);
            break;
        case 0xF1: //POP AF
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R8.F = GB_MemRead8(cpu->R16.SP++) & 0xF0;
                cpu->R16.SP &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R8.A = GB_MemRead8(cpu->R16.SP++);
                cpu->R16.SP &= 0xFFFF;
            }
            break;
        case 0xF2: //LD A,(C)
            if(emu->cpu_microinstruction == 1)
                cpu->R8.A = GB_MemRead8(0xFF00 + (u32)cpu->R8.C);
            break;
        case 0xF3: //DI
            GameBoy.Memory.InterruptMasterEnable = 0;
            GameBoy.Memory.interrupts_enable_count = 0;
            break;
        case 0xF4:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. F4\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xF5: //PUSH AF
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.A);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP--;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.F);
            }
            //else if(emu->cpu_microinstruction == 3) //nothing
            break;
        case 0xF6: //OR n
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF &= ~(F_SUBTRACT|F_CARRY|F_HALFCARRY);
                cpu->R8.A |= GB_MemRead8(cpu->R16.PC++);
                cpu->F.Z = (cpu->R8.A == 0);
            }
            break;
        case 0xF7: //RST 0x30
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = 0x0030;
            break;
        case 0xF8: //LD HL,SP+n | LDHL SP,n
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = (s32)(s8)GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                s32 temp = emu->cpu_temp_16;
                s32 res = (s32)cpu->R16.SP + temp;

                cpu->R16.HL = res & 0xFFFF;

                cpu->R8.F = 0;
                cpu->F.C = ( ( (cpu->R16.SP & 0x00FF) + (temp & 0x00FF) ) > 0x00FF );
                cpu->F.H = ( ( (cpu->R16.SP & 0x000F) + (temp & 0x000F) ) > 0x000F );
            }
            break;
        case 0xF9: //LD SP,HL
            if(emu->cpu_microinstruction == 1)
                cpu->R16.SP = cpu->R16.HL;
            break;
        case 0xFA: //LD A,(nn)
            if(emu->cpu_microinstruction == 1)
            {
                emu->cpu_temp_16 = GB_MemRead8(cpu->R16.PC++);
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 2)
            {
                emu->cpu_temp_16 |= ( (u32)GB_MemRead8(cpu->R16.PC++) ) << 8;
                cpu->R16.PC &= 0xFFFF;
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R8.A = GB_MemRead8(emu->cpu_temp_16);
            break;
        case 0xFB: //EI
            GameBoy.Memory.interrupts_enable_count = 1;
        //    GameBoy.Memory.InterruptMasterEnable = 1;
            break;
        case 0xFC:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. FC\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xFD:
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. FD\nPC: %04x\nROM: %d",
                        GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
        case 0xFE: //CP n
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.AF |= F_SUBTRACT;
                u32 temp = GB_MemRead8(cpu->R16.PC++);
                u32 temp2 = cpu->R8.A;
                cpu->F.H = (temp2 & 0xF) < (temp & 0xF);
                cpu->F.C = (temp2 < temp);
                cpu->F.Z = (temp2 == temp);
            }
            break;
        case 0xFF: //RST 0x38
            if(emu->cpu_microinstruction == 1)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCH);
            }
            else if(emu->cpu_microinstruction == 2)
            {
                cpu->R16.SP --;
                cpu->R16.SP &= 0xFFFF;
                GB_MemWrite8(cpu->R16.SP,cpu->R8.PCL);
            }
            else if(emu->cpu_microinstruction == 3)
                cpu->R16.PC = 0x0038;
            break;

        default: //wtf...?
            _gb_break_to_debugger();
            Debug_ErrorMsgArg("Unidentified opcode. %x\nPC: %04x\nROM: %d",
                        emu->opcode,GameBoy.CPU.R16.PC,GameBoy.Memory.selected_rom);
            break;
    } //end switch

    //cpu->R16.PC &= 0xFFFF;

    emu->cpu_microinstruction++;

    if(emu->cb_opcode == -1) // check if instruction completely executed
    {
        //normal instruction
        if(gb_clocks_table[emu->opcode] <= emu->cpu_microinstruction)
            emu->cpu_microinstruction = 0;
    }
    else // CB instruction
    {
        if(gb_clocks_table_cb[emu->cb_opcode] <= emu->cpu_microinstruction)
            emu->cpu_microinstruction = 0;
    }
}


