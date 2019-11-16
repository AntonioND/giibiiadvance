// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include <SDL.h>

#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../general_utils.h"

#include "win_gba_debugger.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gba_core/gba.h"
#include "../gba_core/cpu.h"
#include "../gba_core/disassembler.h"
#include "../gba_core/memory.h"

//------------------------------------------------------------------------------

static int WinIDGBADis;

#define WIN_GBA_DISASSEMBLER_WIDTH  600
#define WIN_GBA_DISASSEMBLER_HEIGHT 480

static int GBADisassemblerCreated = 0;

//------------------------------------------------------------------------------

#define CPU_DISASSEMBLER_MAX_INSTRUCTIONS (39)

#define GBA_DISASM_CPU_AUTO  0
#define GBA_DISASM_CPU_ARM   1
#define GBA_DISASM_CPU_THUMB 2

static int disassemble_mode = GBA_DISASM_CPU_AUTO;

static u32 gba_disassembler_set_default_address = 0;
static u32 gba_disassembler_start_address;

//------------------------------------------------------------------------------

static _gui_console gba_disassembly_con, gba_regs_con;
static _gui_element gba_disassembly_textbox, gba_regs_textbox;

static _gui_element gba_disassembler_step_btn, gba_disassembler_goto_btn;

static _gui_element gba_disassembler_disassembly_mode_label;

static _gui_element gba_disassembler_auto_radbtn, gba_disassembler_arm_radbtn,
                    gba_disassembler_thumb_radbtn;

static _gui_element * gba_disassembler_window_gui_elements[] = {
    &gba_disassembly_textbox,
    &gba_regs_textbox,
    &gba_disassembler_step_btn,
    &gba_disassembler_goto_btn,
    &gba_disassembler_disassembly_mode_label,
    &gba_disassembler_auto_radbtn,
    &gba_disassembler_arm_radbtn,
    &gba_disassembler_thumb_radbtn,
    NULL
};

_gui_inputwindow gui_iw_gba_disassembler;

static _gui gba_disassembler_window_gui = {
    gba_disassembler_window_gui_elements,
    &gui_iw_gba_disassembler,
    NULL
};

//------------------------------------------------------------------------------

static void _win_gba_disassembler_step(void);
static void _win_gba_disassembler_goto(void);

//------------------------------------------------------------------------------

void Win_GBADisassemblerStartAddressSetDefault(void)
{
    gba_disassembler_set_default_address = 1;
}

void Win_GBADisassemblerUpdate(void)
{
    if (GBADisassemblerCreated == 0)
        return;

    if (Win_MainRunningGBA() == 0)
        return;

    _cpu_t *cpu = GBA_CPUGet();

    GUI_ConsoleClear(&gba_regs_con);
    GUI_ConsoleClear(&gba_disassembly_con);

    if (gba_disassembler_set_default_address)
    {
        gba_disassembler_set_default_address = 0;

        u32 address = cpu->R[R_PC];

        if (((disassemble_mode == GBA_DISASM_CPU_AUTO) &&
             (cpu->EXECUTION_MODE == EXEC_ARM)) ||
            (disassemble_mode == GBA_DISASM_CPU_ARM)) // ARM
        {
            if(address > ((CPU_DISASSEMBLER_MAX_INSTRUCTIONS / 2) - 2) * 4)
                address -= ((CPU_DISASSEMBLER_MAX_INSTRUCTIONS / 2) - 2) * 4;
            else
                address = 0;
        }
        else // THUMB
        {
            if(address > ((CPU_DISASSEMBLER_MAX_INSTRUCTIONS / 2) - 2) * 2)
                address -= ((CPU_DISASSEMBLER_MAX_INSTRUCTIONS / 2) - 2) * 2;
            else
                address = 0;
        }
        gba_disassembler_start_address = address;
    }

    // REGISTERS
    for (int i = 0; i < 10; i++)
        GUI_ConsoleModePrintf(&gba_regs_con, 0, i, "r%d:   %08X", i, cpu->R[i]);
    for (int i = 10; i < 16; i++)
        GUI_ConsoleModePrintf(&gba_regs_con, 0, i, "r%d:  %08X", i, cpu->R[i]);

    GUI_ConsoleModePrintf(&gba_regs_con, 0, 16, "cpsr: %08X", cpu->CPSR);
    GUI_ConsoleModePrintf(&gba_regs_con, 0, 17, "spsr: %08X", cpu->SPSR);
    GUI_ConsoleModePrintf(&gba_regs_con, 0, 18, "              ");

    char text[80];
    snprintf(text, sizeof(text), "N:%d Z:%d C:%d V:%d",
             (cpu->CPSR & F_N) != 0, (cpu->CPSR & F_Z) != 0,
             (cpu->CPSR & F_C) != 0, (cpu->CPSR & F_V) != 0);
    GUI_ConsoleModePrintf(&gba_regs_con, 0, 19, text);

    snprintf(text, sizeof(text), "I:%d F:%d     T:%d",
             (cpu->CPSR & F_I) != 0, (cpu->CPSR & F_F) != 0,
             (cpu->CPSR & F_T) != 0);
    GUI_ConsoleModePrintf(&gba_regs_con, 0, 20, text);

    static const char *cpu_modes[] = {
        "user", "fiq", "irq", "svc", "abort", "undef.", "system"
    };

    snprintf(text, sizeof(text), "Mode:%02X (%s)", cpu->CPSR & 0x1F,
             cpu_modes[cpu->MODE]);
    GUI_ConsoleModePrintf(&gba_regs_con, 0, 21, text);

    // DISASSEMBLER

    char opcode_text[128];
    char final_text[156];

    if (((disassemble_mode == GBA_DISASM_CPU_AUTO) &&
         (cpu->EXECUTION_MODE == EXEC_ARM)) ||
        (disassemble_mode == GBA_DISASM_CPU_ARM)) // ARM
    {
        u32 address = gba_disassembler_start_address; // Start address

        for (int i = 0; i < CPU_DISASSEMBLER_MAX_INSTRUCTIONS; i++)
        {
            u32 opcode = GBA_MemoryReadFast32(address);

            GBA_DisassembleARM(opcode, address, opcode_text,
                               sizeof(opcode_text));
            snprintf(final_text, sizeof(final_text), "%08X:%08X %s", address,
                     opcode, opcode_text);

            GUI_ConsoleModePrintf(&gba_disassembly_con, 0, i, final_text);

            if (GBA_DebugIsBreakpoint(address))
            {
                if(address == cpu->R[R_PC])
                {
                    GUI_ConsoleColorizeLine(&gba_disassembly_con, i,
                                            0xFFFF8000);
                }
                else
                {
                    GUI_ConsoleColorizeLine(&gba_disassembly_con, i,
                                            0xFF0000FF);
                }
            }
            else if(address == cpu->R[R_PC])
            {
                GUI_ConsoleColorizeLine(&gba_disassembly_con, i, 0xFFFFFF00);
            }

            address += 4;
        }
    }
    else // THUMB
    {
        u32 address = gba_disassembler_start_address; // Start address

        for (int i = 0; i < CPU_DISASSEMBLER_MAX_INSTRUCTIONS; i++)
        {
            u16 opcode = GBA_MemoryReadFast16(address);

            GBA_DisassembleTHUMB(opcode, address, opcode_text,
                                 sizeof(opcode_text));
            snprintf(final_text, sizeof(final_text), "%08X:%04X %s", address,
                     opcode, opcode_text);

            GUI_ConsoleModePrintf(&gba_disassembly_con, 0, i, final_text);

            if (GBA_DebugIsBreakpoint(address))
            {
                if (address == cpu->R[R_PC])
                {
                    GUI_ConsoleColorizeLine(&gba_disassembly_con, i,
                                            0xFFFF8000);
                }
                else
                {
                    GUI_ConsoleColorizeLine(&gba_disassembly_con, i,
                                            0xFF0000FF);
                }
            }
            else if(address == cpu->R[R_PC])
            {
                GUI_ConsoleColorizeLine(&gba_disassembly_con, i, 0xFFFFFF00);
            }

            address += 2;
        }
    }
}

static void _win_gba_disassembler_render(void)
{
    if (GBADisassemblerCreated == 0)
        return;

    char buffer[WIN_GBA_DISASSEMBLER_WIDTH * WIN_GBA_DISASSEMBLER_HEIGHT * 3];

    GUI_Draw(&gba_disassembler_window_gui, buffer,
             WIN_GBA_DISASSEMBLER_WIDTH, WIN_GBA_DISASSEMBLER_HEIGHT, 1);

    WH_Render(WinIDGBADis, buffer);
}

static int _win_gba_disassembler_callback(SDL_Event *e)
{
    if (GBADisassemblerCreated == 0)
        return 1;

    int redraw = GUI_SendEvent(&gba_disassembler_window_gui, e);

    _cpu_t *cpu = GBA_CPUGet();
    int close_this = 0;

    if (GUI_InputWindowIsEnabled(&gui_iw_gba_disassembler) == 0)
    {
        if (e->type == SDL_MOUSEWHEEL)
        {
            if (((disassemble_mode == GBA_DISASM_CPU_AUTO) &&
                 (cpu->EXECUTION_MODE == EXEC_ARM)) ||
                (disassemble_mode == GBA_DISASM_CPU_ARM)) // ARM
            {
                gba_disassembler_start_address -= e->wheel.y * 3 * 4;
            }
            else // THUMB
            {
                gba_disassembler_start_address -= e->wheel.y * 3 * 2;
            }
            redraw = 1;
        }
        else if (e->type == SDL_KEYDOWN)
        {
            switch (e->key.keysym.sym)
            {
                case SDLK_F7:
                    _win_gba_disassembler_step();
                    redraw = 1;
                    break;
                case SDLK_F8:
                    _win_gba_disassembler_goto();
                    redraw = 1;
                    break;

                case SDLK_DOWN:
                    if (((disassemble_mode == GBA_DISASM_CPU_AUTO) &&
                         (cpu->EXECUTION_MODE == EXEC_ARM)) ||
                        (disassemble_mode == GBA_DISASM_CPU_ARM)) // ARM
                    {
                        gba_disassembler_start_address += 4;
                    }
                    else // THUMB
                    {
                        gba_disassembler_start_address += 2;
                    }
                    redraw = 1;
                    break;

                case SDLK_UP:
                    if (((disassemble_mode == GBA_DISASM_CPU_AUTO) &&
                         (cpu->EXECUTION_MODE == EXEC_ARM)) ||
                        (disassemble_mode == GBA_DISASM_CPU_ARM)) // ARM
                    {
                        gba_disassembler_start_address -= 4;
                    }
                    else // THUMB
                    {
                        gba_disassembler_start_address -= 2;
                    }
                    redraw = 1;
                    break;
            }
        }
    }

    if (e->type == SDL_WINDOWEVENT)
    {
        if (e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            redraw = 1;
        }
        else if (e->window.event == SDL_WINDOWEVENT_EXPOSED)
        {
            redraw = 1;
        }
        else if (e->window.event == SDL_WINDOWEVENT_CLOSE)
        {
            close_this = 1;
        }
    }
    else if (e->type == SDL_KEYDOWN)
    {
        if (e->key.keysym.sym == SDLK_ESCAPE)
        {
            if (GUI_InputWindowIsEnabled(&gui_iw_gba_disassembler))
                GUI_InputWindowClose(&gui_iw_gba_disassembler);
            else
                close_this = 1;
        }
    }

    if (close_this)
    {
        GBADisassemblerCreated = 0;

        if (GUI_InputWindowIsEnabled(&gui_iw_gba_disassembler))
            GUI_InputWindowClose(&gui_iw_gba_disassembler);

        WH_Close(WinIDGBADis);

        return 1;
    }

    if (redraw)
    {
        Win_GBADisassemblerUpdate();
        _win_gba_disassembler_render();
        return 1;
    }

    return 0;
}

static void _win_gba_disassembly_textbox_callback(int x, int y)
{
    _cpu_t *cpu = GBA_CPUGet();

    int instr_size;

    if (((disassemble_mode == GBA_DISASM_CPU_AUTO) &&
         (cpu->EXECUTION_MODE == EXEC_ARM)) ||
        (disassemble_mode == GBA_DISASM_CPU_ARM)) // ARM
    {
        instr_size = 4;
    }
    else // THUMB
    {
        instr_size = 2;
    }

    u32 addr = gba_disassembler_start_address
             + ((y / FONT_HEIGHT) * instr_size);

    if(GBA_DebugIsBreakpoint(addr) == 0)
        GBA_DebugAddBreakpoint(addr);
    else
        GBA_DebugClearBreakpoint(addr);
}

static int gba_debugger_register_to_change; // 0-17 -> registers. 100 -> goto

static void _win_gba_disassembly_inputwindow_callback(char *text, int is_valid)
{
    if (is_valid)
    {
        text[8] = '\0';
        u32 newvalue = asciihex_to_int(text);

        _cpu_t *cpu = GBA_CPUGet();

        if (gba_debugger_register_to_change < 16)
        {
            cpu->R[gba_debugger_register_to_change] = newvalue;
        }
        else if (gba_debugger_register_to_change == 16)
        {
            cpu->CPSR = newvalue;

            if (cpu->CPSR & F_T)
                cpu->EXECUTION_MODE = EXEC_THUMB;
            else
                cpu->EXECUTION_MODE = EXEC_ARM;

            GBA_CPUChangeMode(newvalue & 0x1F);
        }
        else if (gba_debugger_register_to_change == 17)
        {
            cpu->SPSR = newvalue;
        }
        else if (gba_debugger_register_to_change == 100)
        {
            int increment;
            if (((disassemble_mode == GBA_DISASM_CPU_AUTO) &&
                 (cpu->EXECUTION_MODE == EXEC_ARM)) ||
                (disassemble_mode == GBA_DISASM_CPU_ARM)) // ARM
            {
                increment = 4;
            }
            else // THUMB
            {
                increment = 2;
            }

            gba_disassembler_start_address = newvalue
                        - ((CPU_DISASSEMBLER_MAX_INSTRUCTIONS / 2) * increment);
        }
    }
}

static void _win_gba_registers_textbox_callback(int x, int y)
{
    int reg = y / FONT_HEIGHT;

    char text[30];

    if (reg < 16)
        snprintf(text, sizeof(text), "New value for r%d", reg);
    else if (reg == 16)
        snprintf(text, sizeof(text), "New value for cpsr");
    else if (reg == 17)
        snprintf(text, sizeof(text), "New value for spsr");
    else
        return;

    gba_debugger_register_to_change = reg;

    GUI_InputWindowOpen(&gui_iw_gba_disassembler, text,
                        _win_gba_disassembly_inputwindow_callback);
}

static void _win_gba_cpu_mode_radbtn_callback(int btn_id)
{
    disassemble_mode = btn_id;
    Win_GBADisassemblerStartAddressSetDefault();
    Win_GBADisassemblerUpdate();
}

static void _win_gba_disassembler_step(void)
{
    if (GBADisassemblerCreated == 0)
        return;

    if (Win_MainRunningGBA() == 0)
        return;

    Win_GBADisassemblerStartAddressSetDefault();

    GBA_DebugStep();

    Win_GBAMemViewerUpdate();
    Win_GBAMemViewerRender();

    Win_GBAIOViewerUpdate();
    Win_GBAIOViewerRender();
}

static void _win_gba_disassembler_goto(void)
{
    if (GBADisassemblerCreated == 0)
        return;

    if (Win_MainRunningGBA() == 0)
        return;

    gba_debugger_register_to_change = 100;

    GUI_InputWindowOpen(&gui_iw_gba_disassembler, "Go to address",
                        _win_gba_disassembly_inputwindow_callback);
}

//----------------------------------------------------------------

int Win_GBADisassemblerCreate(void)
{
    if (Win_MainRunningGBA() == 0)
        return 0;

    if (GBADisassemblerCreated == 1)
    {
        WH_Focus(WinIDGBADis);
        return 0;
    }

    GUI_SetTextBox(&gba_disassembly_textbox, &gba_disassembly_con, 6, 6,
            66 * FONT_WIDTH,CPU_DISASSEMBLER_MAX_INSTRUCTIONS * FONT_HEIGHT,
            _win_gba_disassembly_textbox_callback);
    GUI_SetTextBox(&gba_regs_textbox, &gba_regs_con,
            6 + 66 * FONT_WIDTH + 12, 6, 16 * FONT_WIDTH, 22 * FONT_HEIGHT,
            _win_gba_registers_textbox_callback);

    GUI_SetButton(&gba_disassembler_step_btn, 6 + 66 * FONT_WIDTH + 12, 280,
                  16 * FONT_WIDTH, 24, "Step (F7)", _win_gba_disassembler_step);

    GUI_SetButton(&gba_disassembler_goto_btn, 6 + 66 * FONT_WIDTH + 12, 316,
                  16 * FONT_WIDTH, 24, "Goto (F8)", _win_gba_disassembler_goto);

    GUI_SetLabel(&gba_disassembler_disassembly_mode_label,
                 6 + 66 * FONT_WIDTH + 12, 366, 16 * FONT_WIDTH, 24,
                 "Disassembly mode");

    GUI_SetRadioButton(&gba_disassembler_auto_radbtn,
                       6 + 66 * FONT_WIDTH + 12, 388,
                       16 * FONT_WIDTH, 24,
                       "Auto", 0, GBA_DISASM_CPU_AUTO, 1,
                       _win_gba_cpu_mode_radbtn_callback);
    GUI_SetRadioButton(&gba_disassembler_arm_radbtn,
                       6 + 66 * FONT_WIDTH + 12, 418,
                       16 * FONT_WIDTH, 24,
                       "ARM", 0, GBA_DISASM_CPU_ARM, 0,
                       _win_gba_cpu_mode_radbtn_callback);
    GUI_SetRadioButton(&gba_disassembler_thumb_radbtn,
                       6 + 66 * FONT_WIDTH + 12, 448,
                       16 * FONT_WIDTH, 24,
                       "THUMB", 0, GBA_DISASM_CPU_THUMB, 0,
                       _win_gba_cpu_mode_radbtn_callback);

    GUI_InputWindowClose(&gui_iw_gba_disassembler);

    GBADisassemblerCreated = 1;

    disassemble_mode = GBA_DISASM_CPU_AUTO;

    Win_GBADisassemblerStartAddressSetDefault();

    WinIDGBADis = WH_Create(WIN_GBA_DISASSEMBLER_WIDTH,
                            WIN_GBA_DISASSEMBLER_HEIGHT, 0, 0, 0);
    WH_SetCaption(WinIDGBADis, "GBA CPU Disassembly");

    WH_SetEventCallback(WinIDGBADis, _win_gba_disassembler_callback);

    Win_GBADisassemblerUpdate();
    _win_gba_disassembler_render();

    return 1;
}

void Win_GBADisassemblerSetFocus(void)
{
    if (GBADisassemblerCreated == 1)
    {
        WH_Focus(WinIDGBADis);
        return;
    }

    Win_GBADisassemblerCreate();
}

void Win_GBADisassemblerClose(void)
{
    if (GBADisassemblerCreated == 0)
        return;

    GBADisassemblerCreated = 0;
    WH_Close(WinIDGBADis);
}
