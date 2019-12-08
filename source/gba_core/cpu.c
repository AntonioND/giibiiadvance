// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include "../build_options.h"
#include "../config.h"
#include "../debug_utils.h"

#include "cpu.h"
#include "gba.h"
#include "memory.h"

//------------------------------------------------------------------------------

_cpu_t CPU;
u32 cpu_loop_break = 0;

void GBA_CPUInit(void)
{
    memset(&CPU, 0, sizeof(CPU));
    CPU.EXECUTION_MODE = EXEC_ARM;

    if (EmulatorConfig.load_from_boot_rom) // <- To init from bios
    {
        CPU.R[R_SP] = 0x03007F00;
        CPU.R[R_LR] = 0x08000000;

        CPU.R[R_PC] = 0x00000000;
        CPU.CPSR = M_SUPERVISOR | F_IRQ_DIS | F_FIQ_DIS;
        CPU.MODE = CPU_SUPERVISOR;
    }
    else
    {
        CPU.R[R_SP] = 0x03007F00;
        CPU.R[R_LR] = 0x08000000;
        CPU.R[R_PC] = 0x08000000;
        CPU.CPSR = M_SYSTEM;
        CPU.MODE = CPU_SYSTEM;
    }

    // Default stack pointers (Set by BIOS)
    CPU.R_user[13 - 8] = 0x03007F00;
    CPU.R13_svc = 0x03007FE0;
    CPU.R13_irq = 0x03007FA0;

    CPU.OldPC = 0;

    REG_IME = 0;

    cpu_loop_break = 0;

    GBA_CPUClearHalted();
}

// USER       - 0x10
// FIQ        - 0x11
// IRQ        - 0x12
// SUPERVISOR - 0x13
// ABORT      - 0x17
// UNDEFINED  - 0x1B
// SYSTEM     - 0x1F

void GBA_CPUChangeMode(u32 value)
{
    static const u32 mode_value[CPU_MODE_NUMBER] = {
        M_USER, M_FIQ, M_IRQ, M_SUPERVISOR, M_ABORT, M_UNDEFINED, M_SYSTEM
    };

    // If the mode is the same one as the current one, do nothing
    if (mode_value[CPU.MODE] == value)
        return;

    //CPU.SPSR = CPU.CPSR;

    switch (CPU.MODE) // Save current registers
    {
        case CPU_USER:
        case CPU_SYSTEM:
        {
            for (int i = 0; i < 7; i++)
                CPU.R_user[i] = CPU.R[i + 8];
            break;
        }
        case CPU_FIQ:
        {
            for (int i = 0; i < 7; i++)
                CPU.R_fiq[i] = CPU.R[i + 8];
            CPU.SPSR_fiq = CPU.SPSR;
            break;
        }
        case CPU_SUPERVISOR:
        {
            for (int i = 0; i < 5; i++)
                CPU.R_user[i] = CPU.R[i + 8];
            CPU.R13_svc = CPU.R[13];
            CPU.R14_svc = CPU.R[14];
            CPU.SPSR_svc = CPU.SPSR;
            break;
        }
        case CPU_ABORT:
        {
            for (int i = 0; i < 5; i++)
                CPU.R_user[i] = CPU.R[i + 8];
            CPU.R13_abt = CPU.R[13];
            CPU.R14_abt = CPU.R[14];
            CPU.SPSR_abt = CPU.SPSR;
            break;
        }
        case CPU_IRQ:
        {
            for (int i = 0; i < 5; i++)
                CPU.R_user[i] = CPU.R[i + 8];
            CPU.R13_irq = CPU.R[13];
            CPU.R14_irq = CPU.R[14];
            CPU.SPSR_irq = CPU.SPSR;
            break;
        }
        case CPU_UNDEFINED:
        {
            for (int i = 0; i < 5; i++)
                CPU.R_user[i] = CPU.R[i + 8];
            CPU.R13_und = CPU.R[13];
            CPU.R14_und = CPU.R[14];
            CPU.SPSR_und = CPU.SPSR;
            break;
        }
        default:
            return;
    }

    if (value == mode_value[CPU_USER])
    {
        CPU.MODE = CPU_USER;
    }
    else if (value == mode_value[CPU_FIQ])
    {
        CPU.MODE = CPU_FIQ;
    }
    else if (value == mode_value[CPU_IRQ])
    {
        CPU.MODE = CPU_IRQ;
    }
    else if (value == mode_value[CPU_SUPERVISOR])
    {
        CPU.MODE = CPU_SUPERVISOR;
    }
    else if (value == mode_value[CPU_ABORT])
    {
        CPU.MODE = CPU_ABORT;
    }
    else if (value == mode_value[CPU_UNDEFINED])
    {
        CPU.MODE = CPU_UNDEFINED;
    }
    else if (value == mode_value[CPU_SYSTEM])
    {
        CPU.MODE = CPU_SYSTEM;
    }
    else
    {
        Debug_DebugMsgArg("Trying to change to CPU mode 0x%02X (invalid).",
                          value);
        GBA_ExecutionBreak();
        return; // Error
    }

    switch (CPU.MODE) // Load new registers
    {
        case CPU_USER:
        case CPU_SYSTEM:
        {
            for (int i = 0; i < 7; i++)
                CPU.R[i + 8] = CPU.R_user[i];
            break;
        }
        case CPU_FIQ:
        {
            for (int i = 0; i < 7; i++)
                CPU.R[i + 8] = CPU.R_fiq[i];
            CPU.SPSR = CPU.SPSR_fiq;
            break;
        }
        case CPU_SUPERVISOR:
        {
            for (int i = 0; i < 5; i++)
                CPU.R[i + 8] = CPU.R_user[i];
            CPU.R[13] = CPU.R13_svc;
            CPU.R[14] = CPU.R14_svc;
            CPU.SPSR = CPU.SPSR_svc;
            break;
        }
        case CPU_ABORT:
        {
            for (int i = 0; i < 5; i++)
                CPU.R[i + 8] = CPU.R_user[i];
            CPU.R[13] = CPU.R13_abt;
            CPU.R[14] = CPU.R14_abt;
            CPU.SPSR = CPU.SPSR_abt;
            break;
        }
        case CPU_IRQ:
        {
            for (int i = 0; i < 5; i++)
                CPU.R[i + 8] = CPU.R_user[i];
            CPU.R[13] = CPU.R13_irq;
            CPU.R[14] = CPU.R14_irq;
            CPU.SPSR = CPU.SPSR_irq;
            break;
        }
        case CPU_UNDEFINED:
        {
            for (int i = 0; i < 5; i++)
                CPU.R[i + 8] = CPU.R_user[i];
            CPU.R[13] = CPU.R13_und;
            CPU.R[14] = CPU.R14_und;
            CPU.SPSR = CPU.SPSR_und;
            break;
        }
        default:
            return;
    }

    return;
}

static s32 gba_halt;

void GBA_CPUSetHalted(s32 value)
{
    if (REG_IE) // Don't halt if IE = 0 or it will never exit from halt
    {
        gba_halt = 1 + (value >> 7);
    }
    //else
    //{
    //    Debug_DebugMsgArg("STOP/HALT mode with IE=0: infinite loop.\n"
    //                      "STOP/HALT ignored.");
    //    GBA_ExecutionBreak();
    //}
    // gba_halt = 1 => halt
    // gba_halt = 2 => stop
}

s32 GBA_CPUGetHalted(void)
{
    return gba_halt;
}

void GBA_CPUClearHalted(void)
{
    gba_halt = 0;
}

s32 GBA_Execute(s32 clocks) // Returns total clocks not executed
{
    if (GBA_CPUGetHalted()) // Execute all clocks
        return 0;

    if (CPU.EXECUTION_MODE == EXEC_ARM)
        return GBA_ExecuteARM(clocks);
    else
        return GBA_ExecuteTHUMB(clocks);
}

void GBA_ExecutionBreak(void)
{
    cpu_loop_break = 1;
}
