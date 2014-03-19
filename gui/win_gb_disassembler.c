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

#include <SDL2/SDL.h>

#include <string.h>

#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../general_utils.h"

#include "win_gb_disassembler.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/debug.h"
#include "../gb_core/cpu.h"
#include "../gb_core/memory.h"

//-----------------------------------------------------------------------------------

static int WinIDGBDis;

#define WIN_GB_DISASSEMBLER_WIDTH  450
#define WIN_GB_DISASSEMBLER_HEIGHT 432

static int GBDisassemblerCreated = 0;

//-----------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

#define CPU_DISASSEMBLER_MAX_INSTRUCTIONS (35)
#define CPU_STACK_MAX_LINES               (19)

static int gb_cpu_line_address[CPU_DISASSEMBLER_MAX_INSTRUCTIONS];

static u32 gb_disassembler_set_default_address = 0;
static u32 gb_disassembler_start_address;

//-----------------------------------------------------------------------------------

static _gui_console gb_disassembly_con, gb_regs_con, gb_stack_con;
static _gui_element gb_disassembly_textbox, gb_regs_textbox, gb_stack_textbox;

static _gui_element gb_disassembler_step_btn, gb_disassembler_goto_btn;

static _gui_element * gb_disassembler_window_gui_elements[] = {
    &gb_disassembly_textbox,
    &gb_regs_textbox,
    &gb_stack_textbox,
    &gb_disassembler_step_btn,
    &gb_disassembler_goto_btn,
    NULL
};

_gui_inputwindow gui_iw_gb_disassembler;

static _gui gb_disassembler_window_gui = {
    gb_disassembler_window_gui_elements,
    &gui_iw_gb_disassembler,
    NULL
};

//-----------------------------------------------------------------------------------

static void _win_gb_disassembler_step(void);
static void _win_gb_disassembler_goto(void);

//-----------------------------------------------------------------------------------

void Win_GBDisassemblerStartAddressSetDefault(void)
{
    gb_disassembler_set_default_address = 1;
}

void Win_GBDisassemblerUpdate(void)
{
    if(GBDisassemblerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    GUI_ConsoleClear(&gb_regs_con);
    GUI_ConsoleClear(&gb_disassembly_con);

    //REGISTERS
    GUI_ConsoleModePrintf(&gb_regs_con,0,0,"af: %04X",GameBoy.CPU.Reg16.AF);
    GUI_ConsoleModePrintf(&gb_regs_con,0,1,"bc: %04X",GameBoy.CPU.Reg16.BC);
    GUI_ConsoleModePrintf(&gb_regs_con,0,2,"de: %04X",GameBoy.CPU.Reg16.DE);
    GUI_ConsoleModePrintf(&gb_regs_con,0,3,"hl: %04X",GameBoy.CPU.Reg16.HL);
    GUI_ConsoleModePrintf(&gb_regs_con,0,4,"sp: %04X",GameBoy.CPU.Reg16.SP);
    GUI_ConsoleModePrintf(&gb_regs_con,0,5,"pc: %04X",GameBoy.CPU.Reg16.PC);

    GUI_ConsoleModePrintf(&gb_regs_con,0,6,"        ");

    int flags = GameBoy.CPU.Reg16.AF;
    GUI_ConsoleModePrintf(&gb_regs_con,0,7,"C:%d H:%d", (flags&F_CARRY) != 0, (flags&F_HALFCARRY) != 0);
    GUI_ConsoleModePrintf(&gb_regs_con,0,8,"N:%d Z:%d", (flags&F_SUBSTRACT) != 0, (flags&F_ZERO) != 0);

    //DISASSEMBLER

    if(gb_disassembler_set_default_address)
    {
        gb_disassembler_set_default_address = 0;
#if 0
        //DOESN'T WORK
        address = GameBoy.CPU.Reg16.PC;
        while(address > 0x0001)
        {
            //If there are 2 possible instructions that are 1 byte long, the next byte is an instruction.
            if(debug_command_param_size[GB_MemRead8(address-1)] == 1)
            {
                if(debug_command_param_size[GB_MemRead8(address-2)] == 1) break;
                address -= 2;
            }
            else address -= 1;
        }
#endif
        u16 address = 0x0000; //Dissasemble everytime from the beggining... :S
        if(GameBoy.CPU.Reg16.PC > 0x0120) address = 0x0100;
        if(GameBoy.CPU.Reg16.PC > 0x4020) address = 0x4000;

        if(GameBoy.CPU.Reg16.PC > 10)
        {
            int start_address = GameBoy.CPU.Reg16.PC - 24;
            if(start_address < 0) start_address = 0;

            while(address < start_address)
                address += gb_debug_get_address_increment(address);

            while(1) //To fix cursor at one line.
            {
                int tempaddr = address, commands = 0;
                while(tempaddr < GameBoy.CPU.Reg16.PC)
                {
                    commands ++;
                    tempaddr += gb_debug_get_address_increment(tempaddr);
                }

                if(commands < (CPU_DISASSEMBLER_MAX_INSTRUCTIONS/2) ) break;
                address += gb_debug_get_address_increment(address);
            }
        }

        gb_disassembler_start_address = address;
    }

    u16 address = gb_disassembler_start_address;
    char opcode_text[128];
    int i;
    for(i = 0; i < CPU_DISASSEMBLER_MAX_INSTRUCTIONS; i++)
    {
        int step;
        strcpy(opcode_text,GB_Dissasemble(address,&step));
        GUI_ConsoleModePrintf(&gb_disassembly_con,0,i,"%04X:%s",address,opcode_text);
        gb_cpu_line_address[i] = address;

        if(GB_DebugIsBreakpoint(address))
        {
            if(address == GameBoy.CPU.Reg16.PC)
                GUI_ConsoleColorizeLine(&gb_disassembly_con, i, 0xFFFF8000);
            else
                GUI_ConsoleColorizeLine(&gb_disassembly_con, i, 0xFF0000FF);
        }
        else if(address == GameBoy.CPU.Reg16.PC)
                GUI_ConsoleColorizeLine(&gb_disassembly_con, i, 0xFFFFFF00);

        address += step;
    }

    address = GameBoy.CPU.Reg16.SP - ((CPU_STACK_MAX_LINES/2)*2);
    for(i = 0; i < CPU_STACK_MAX_LINES; i++)
    {
        GUI_ConsoleModePrintf(&gb_stack_con,0,i,"%04X:%04X",address,GB_MemRead16(address));

        if(address == GameBoy.CPU.Reg16.SP)
            GUI_ConsoleColorizeLine(&gb_stack_con, i, 0xFFFFFF00);

        address += 2;
    }
}

static void _win_gb_dissasembler_render(void)
{
    if(GBDisassemblerCreated == 0) return;

    char buffer[WIN_GB_DISASSEMBLER_WIDTH*WIN_GB_DISASSEMBLER_HEIGHT*3];
    GUI_Draw(&gb_disassembler_window_gui,buffer,WIN_GB_DISASSEMBLER_WIDTH,WIN_GB_DISASSEMBLER_HEIGHT,1);

    WH_Render(WinIDGBDis, buffer);
}

static int _win_gb_disassembler_callback(SDL_Event * e)
{
    if(GBDisassemblerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&gb_disassembler_window_gui,e);

    int close_this = 0;

    if(GUI_InputWindowIsEnabled(&gui_iw_gb_disassembler) == 0)
    {
        if(e->type == SDL_MOUSEWHEEL)
        {
            gb_disassembler_start_address -= e->wheel.y*3;
            redraw = 1;
        }
        else if( e->type == SDL_KEYDOWN)
        {
            switch( e->key.keysym.sym )
            {
                case SDLK_F7:
                    _win_gb_disassembler_step();
                    redraw = 1;
                    break;
                case SDLK_F8:
                    _win_gb_disassembler_goto();
                    redraw = 1;
                    break;

                case SDLK_DOWN:
                    gb_disassembler_start_address += 1;
                    redraw = 1;
                    break;

                case SDLK_UP:
                    gb_disassembler_start_address -= 1;
                    redraw = 1;
                    break;
            }
        }
    }

    if(e->type == SDL_WINDOWEVENT)
    {
        if(e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            redraw = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_EXPOSED)
        {
            redraw = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_CLOSE)
        {
            close_this = 1;
        }
    }
    else if( e->type == SDL_KEYDOWN)
    {
        if( e->key.keysym.sym == SDLK_ESCAPE )
        {
            if(GUI_InputWindowIsEnabled(&gui_iw_gb_disassembler))
                GUI_InputWindowClose(&gui_iw_gb_disassembler);
            else
                close_this = 1;
        }
    }

    if(close_this)
    {
        GBDisassemblerCreated = 0;
        if(GUI_InputWindowIsEnabled(&gui_iw_gb_disassembler))
            GUI_InputWindowClose(&gui_iw_gb_disassembler);
        WH_Close(WinIDGBDis);
        return 1;
    }

    if(redraw)
    {
        Win_GBDisassemblerUpdate();
        _win_gb_dissasembler_render();
        return 1;
    }

    return 0;
}

static void _win_gb_disassembly_textbox_callback(int x, int y)
{
    u32 addr = gb_cpu_line_address[ y / FONT_HEIGHT ];

    if(GB_DebugIsBreakpoint(addr) == 0)
        GB_DebugAddBreakpoint(addr);
    else
        GB_DebugClearBreakpoint(addr);
}

static int gb_debugger_register_to_change; // 0-5 -> registers. 100 -> goto

static void _win_gb_disassembly_inputwindow_callback(char * text, int is_valid)
{
    if(is_valid)
    {
        text[4] = '\0';
        u32 newvalue = asciihex_to_int(text);

        if(gb_debugger_register_to_change == 0)
            GameBoy.CPU.Reg16.AF = newvalue & 0xFFF0;
        else if(gb_debugger_register_to_change == 1)
            GameBoy.CPU.Reg16.BC = newvalue;
        else if(gb_debugger_register_to_change == 2)
            GameBoy.CPU.Reg16.DE = newvalue;
        else if(gb_debugger_register_to_change == 3)
            GameBoy.CPU.Reg16.HL = newvalue;
        else if(gb_debugger_register_to_change == 4)
            GameBoy.CPU.Reg16.SP = newvalue;
        else if(gb_debugger_register_to_change == 5)
            GameBoy.CPU.Reg16.PC = newvalue;
        else if(gb_debugger_register_to_change == 100)
        {
            gb_disassembler_start_address = newvalue - CPU_DISASSEMBLER_MAX_INSTRUCTIONS / 2;
        }
    }
}

static void _win_gb_registers_textbox_callback(int x, int y)
{
    int reg = y/FONT_HEIGHT;

    if(reg > 5)
        return;

    char * text[] = {
        "New value for af",
        "New value for bc",
        "New value for de",
        "New value for hl",
        "New value for sp",
        "New value for pc"
    };

    gb_debugger_register_to_change = reg;

    GUI_InputWindowOpen(&gui_iw_gb_disassembler,text[reg],_win_gb_disassembly_inputwindow_callback);
}

static void _win_gb_disassembler_step(void)
{
    if(GBDisassemblerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    Win_GBDisassemblerStartAddressSetDefault();

    GB_RunForInstruction();
}

static void _win_gb_disassembler_goto(void)
{
    if(GBDisassemblerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    gb_debugger_register_to_change = 100;

    GUI_InputWindowOpen(&gui_iw_gb_disassembler,"Go to address",_win_gb_disassembly_inputwindow_callback);
}

//----------------------------------------------------------------

int Win_GBDisassemblerCreate(void)
{
    if(GBDisassemblerCreated == 1)
        return 0;

    if(Win_MainRunningGB() == 0) return 0;

    GUI_SetTextBox(&gb_disassembly_textbox,&gb_disassembly_con,
                   6,6, 51*FONT_WIDTH,CPU_DISASSEMBLER_MAX_INSTRUCTIONS*FONT_HEIGHT,
                   _win_gb_disassembly_textbox_callback);
    GUI_SetTextBox(&gb_regs_textbox,&gb_regs_con,
                   6+51*FONT_WIDTH+12,6, 10*FONT_WIDTH,9*FONT_HEIGHT,
                   _win_gb_registers_textbox_callback);

    GUI_SetButton(&gb_disassembler_step_btn,6+51*FONT_WIDTH+12,6+9*FONT_HEIGHT+12,10*FONT_WIDTH,24,
                  "Step (F7)",_win_gb_disassembler_step);

    GUI_SetButton(&gb_disassembler_goto_btn,6+51*FONT_WIDTH+12,6+9*FONT_HEIGHT+48,10*FONT_WIDTH,24,
                  "Goto (F8)",_win_gb_disassembler_goto);

    GUI_SetTextBox(&gb_stack_textbox,&gb_stack_con,
                   6+51*FONT_WIDTH+12,6+9*FONT_HEIGHT+48+24+12,10*FONT_WIDTH,CPU_STACK_MAX_LINES*FONT_HEIGHT,
                   NULL);

    GUI_InputWindowClose(&gui_iw_gb_disassembler);

    GBDisassemblerCreated = 1;

    WinIDGBDis = WH_Create(WIN_GB_DISASSEMBLER_WIDTH,WIN_GB_DISASSEMBLER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBDis,"GB CPU Disassembly");

    WH_SetEventCallback(WinIDGBDis,_win_gb_disassembler_callback);

    Win_GBDisassemblerStartAddressSetDefault();

    Win_GBDisassemblerUpdate();
    _win_gb_dissasembler_render();

    return 1;
}

void Win_GBDisassemblerSetFocus(void)
{
    if(GBDisassemblerCreated == 1)
    {
        WH_Focus(WinIDGBDis);
        return;
    }

    Win_GBDisassemblerCreate();
}

void Win_GBDisassemblerClose(void)
{
    if(GBDisassemblerCreated == 0)
        return;

    GBDisassemblerCreated = 0;
    WH_Close(WinIDGBDis);
}

//----------------------------------------------------------------

