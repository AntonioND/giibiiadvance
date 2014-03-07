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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../general_utils.h"
#include "../font_utils.h"

#include "general.h"
#include "gameboy.h"
#include "cpu.h"
#include "debug.h"
#include "memory.h"
#include "video.h"

//----------------------------------------------------------------------------------

#define GB_MAX_BREAKPOINTS 20

static u32 gb_brkpoint_addrlist[GB_MAX_BREAKPOINTS];
static int gb_brkpoint_used[GB_MAX_BREAKPOINTS];
static int gb_any_breakpoint_used = 0;

int GB_DebugIsBreakpoint(u32 addr)
{
    if(gb_any_breakpoint_used == 0) return 0;

    int i;
    for(i = 0; i < GB_MAX_BREAKPOINTS; i++)
    {
        if(gb_brkpoint_used[i])
        {
            if(gb_brkpoint_addrlist[i] == addr)
            {
                return 1;
            }
        }
    }
    return 0;
}

static u32 gb_last_executed_opcode = 1;

int GB_DebugCPUIsBreakpoint(u32 addr)
{
    if(gb_any_breakpoint_used == 0) return 0;

    if(gb_last_executed_opcode == addr)
    {
        gb_last_executed_opcode = 1;
        return 0;
    }

    int i;
    for(i = 0; i < GB_MAX_BREAKPOINTS; i++)
    {
        if(gb_brkpoint_used[i])
        {
            if(gb_brkpoint_addrlist[i] == addr)
            {
                gb_last_executed_opcode = addr;
                return 1;
            }
        }
    }
    return 0;
}

void GB_DebugAddBreakpoint(u32 addr)
{
    if(GB_DebugIsBreakpoint(addr))
        return;

    int i;
    for(i = 0; i < GB_MAX_BREAKPOINTS; i++)
    {
        if(gb_brkpoint_used[i] == 0)
        {
            gb_brkpoint_addrlist[i] = addr;
            gb_brkpoint_used[i] = 1;
            gb_any_breakpoint_used = 1;
            return;
        }
    }
}

void GB_DebugClearBreakpoint(u32 addr)
{
    int i;
    for(i = 0; i < GB_MAX_BREAKPOINTS; i++)
    {
        if(gb_brkpoint_used[i])
        {
            if(gb_brkpoint_addrlist[i] == addr)
            {
                gb_brkpoint_used[i] = 0;
                break;
            }
        }
    }

    for(i = 0; i < GB_MAX_BREAKPOINTS; i++)
    {
        if(gb_brkpoint_used[i])
        {
            return;
        }
    }

    gb_any_breakpoint_used = 0;
}

void GB_DebugClearBreakpointAll(void)
{
    int i;
    for(i = 0; i < GB_MAX_BREAKPOINTS; i++)
    {
        gb_brkpoint_used[i] = 0;
    }

    gb_any_breakpoint_used = 0;
}

//----------------------------------------------------------------------------------

// 3 = jump relative (1 byte)
static const int debug_command_param_size[256] = {
    0, 2, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 1, 0,
    1, 2, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 1, 0,
    3, 2, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 1, 0,
    3, 2, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 2, 2, 2, 0, 1, 0, 0, 0, 2, 1, 2, 2, 1, 0,
    0, 0, 2, 0, 2, 0, 1, 0, 0, 0, 2, 0, 2, 0, 1, 0,
    1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 0,
    1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 0
};

static const char * debug_commands[256] = {
    "nop", "ld bc,#0x%04X", "ld [bc],a", "inc bc", "inc b", "dec b", "ld b,#0x%02X", "rlca",
    "ld [#0x%04X],sp", "add hl,bc", "ld a,[bc]", "dec bc", "inc c", "dec c", "ld c,#0x%02X", "rrca",
    NULL, "ld de,#0x%04X", "ld [de],a", "inc de", "inc d", "dec d", "ld d,#0x%02X", "rla",
    "jr #0x%04X", "add hl,de", "ld a,[de]", "dec de", "inc e", "dec e", "ld e,#0x%02X", "rra",
    "jr nz,#0x%04X", "ld hl,#0x%04X", "ld [hl+],a", "inc hl", "inc h", "dec h", "ld h,#0x%02X", "daa",
    "jr z,#0x%04X", "add hl,hl", "ld a,[hl+]", "dec hl", "inc l", "dec l", "ld l,#0x%02X", "cpl",
    "jr nc,#0x%04X", "ld sp,#0x%04X", "ld [hl-],a", "inc sp", "inc [hl]", "dec [hl]", "ld [hl],#0x%02X",
    "scf", "jr c,#0x%04X", "add hl,sp", "ld a,[hl-]", "dec sp", "inc a", "dec a", "ld a,#0x%02X",
    "ccf", "ld b,b", "ld b,c", "ld b,d", "ld b,e", "ld b,h", "ld b,l", "ld b,[hl]",
    "ld b,a", "ld c,b", "ld c,c", "ld c,d", "ld c,e", "ld c,h", "ld c,l", "ld c,[hl]",
    "ld c,a", "ld d,b", "ld d,c", "ld d,d", "ld d,e", "ld d,h", "ld d,l", "ld d,[hl]",
    "ld d,a", "ld e,b", "ld e,c", "ld e,d", "ld e,e", "ld e,h", "ld e,l", "ld e,[hl]",
    "ld e,a", "ld h,b", "ld h,c", "ld h,d", "ld h,e", "ld h,h", "ld h,l", "ld h,[hl]",
    "ld h,a", "ld l,b", "ld l,c", "ld l,d", "ld l,e", "ld l,h", "ld l,l", "ld l,[hl]",
    "ld l,a", "ld [hl],b", "ld [hl],c", "ld [hl],d", "ld [hl],e", "ld [hl],h", "ld [hl],l", "halt",
    "ld [hl],a", "ld a,b", "ld a,c", "ld a,d", "ld a,e", "ld a,h", "ld a,l", "ld a,[hl]",
    "ld a,a", "add a,b", "add a,c", "add a,d", "add a,e", "add a,h", "add a,l", "add a,[hl]", "add a,a",
    "adc a,b", "adc a,c", "adc a,d", "adc a,e", "adc a,h", "adc a,l", "adc a,[hl]", "adc a,a",
    "sub a,b", "sub a,c", "sub a,d", "sub a,e", "sub a,h", "sub a,l", "sub a,[hl]", "sub a,a",
    "sbc a,b", "sbc a,c", "sbc a,d", "sbc a,e", "sbc a,h", "sbc a,l", "sbc a,[hl]", "sbc a,a",
    "and a,b", "and a,c", "and a,d", "and a,e", "and a,h", "and a,l", "and a,[hl]", "and a,a",
    "xor a,b", "xor a,c", "xor a,d", "xor a,e", "xor a,h", "xor a,l", "xor a,[hl]", "xor a,a",
    "or a,b", "or a,c", "or a,d", "or a,e", "or a,h", "or a,l", "or a,[hl]", "or a,a",
    "cp a,b", "cp a,c", "cp a,d", "cp a,e", "cp a,h", "cp a,l", "cp a,[hl]", "cp a,a",
    "ret nz", "pop bc", "jp nz,#0x%04X", "jp #0x%04X", "call nz,#0x%04X", "push bc", "add a,#0x%02X", "rst #0x00",
    "ret z", "ret", "jp z,#0x%04X", NULL, "call z,#0x%04X", "call #0x%04X", "adc a,#0x%02X", "rst #0x08",
    "ret nc", "pop de", "jp nc,#0x%04X", "[!] Undefined opcode", "call nc,#0x%04X ; ->", "push de",
    "sub a,#0x%02X", "rst #0x10","ret c", "reti", "jp c,#0x%04X", "[!] Undefined opcode", "call c,#0x%04X",
    "[!] Undefined opcode", "sbc a,#0x%02X", "rst #0x18","ldh [#0xFF%02X],a", "pop hl", "ldh [#0xFF00+c],a",
    "[!] Undefined opcode", "[!] Undefined opcode", "push hl", "and a,#0x%02X", "rst #0x20","add sp,#0x%02X ",
    "jp hl", "ld [#0x%04X],a", "[!] Undefined opcode", "[!] Undefined opcode", "[!] Undefined opcode",
    "xor a,#0x%02X", "rst #0x28","ldh a,[#0xFF%02X]", "pop af", "ldh a,[#0xFF00+c]", "di", "[!] Undefined opcode",
    "push af", "or a,#0x%02X", "rst #0x30","ld hl,sp+#0x%02X", "ld sp,hl", "ld a,[#0x%04X]", "ei",
    "[!] Undefined opcode", "[!] Undefined opcode", "cp a,#0x%02X", "rst #0x38"
};

static const char * debug_commands_cb[256] = {
    "rlc b", "rlc c", "rlc d", "rlc e", "rlc h", "rlc l", "rlc [hl]", "rlc a",
    "rrc b", "rrc c", "rrc d", "rrc e", "rrc h", "rrc l", "rrc [hl]", "rrc a",
    "rl b", "rl c", "rl d", "rl e", "rl h", "rl l", "rl [hl]", "rl a",
    "rr b", "rr c", "rr d", "rr e", "rr h", "rr l", "rr [hl]", "rr a",
    "sla b", "sla c", "sla d", "sla e", "sla h", "sla l", "sla [hl]", "sla a",
    "sra b", "sra c", "sra d", "sra e", "sra h", "sra l", "sra [hl]", "sra a",
    "swap b", "swap c", "swap d", "swap e", "swap h", "swap l", "swap [hl]", "swap a",
    "srl b", "srl c", "srl d", "srl e", "srl h", "srl l", "srl [hl]", "srl a",
    "bit 0,b", "bit 0,c", "bit 0,d", "bit 0,e", "bit 0,h", "bit 0,l", "bit 0,[hl]", "bit 0,a",
    "bit 1,b", "bit 1,c", "bit 1,d", "bit 1,e", "bit 1,h", "bit 1,l", "bit 1,[hl]", "bit 2,a",
    "bit 2,b", "bit 2,c", "bit 2,d", "bit 2,e", "bit 2,h", "bit 2,l", "bit 2,[hl]", "bit 2,a",
    "bit 3,b", "bit 3,c", "bit 3,d", "bit 3,e", "bit 3,h", "bit 3,l", "bit 3,[hl]", "bit 3,a",
    "bit 4,b", "bit 4,c", "bit 4,d", "bit 4,e", "bit 4,h", "bit 4,l", "bit 4,[hl]", "bit 4,a",
    "bit 5,b", "bit 5,c", "bit 5,d", "bit 5,e", "bit 5,h", "bit 5,l", "bit 5,[hl]", "bit 5,a",
    "bit 6,b", "bit 6,c", "bit 6,d", "bit 6,e", "bit 6,h", "bit 6,l", "bit 6,[hl]", "bit 6,a",
    "bit 7,b", "bit 7,c", "bit 7,d", "bit 7,e", "bit 7,h", "bit 7,l", "bit 7,[hl]", "bit 7,a",
    "res 0,b", "res 0,c", "res 0,d", "res 0,e", "res 0,h", "res 0,l", "res 0,[hl]", "res 0,a",
    "res 1,b", "res 1,c", "res 1,d", "res 1,e", "res 1,h", "res 1,l", "res 1,[hl]", "res 1,a",
    "res 2,b", "res 2,c", "res 2,d", "res 2,e", "res 2,h", "res 2,l", "res 2,[hl]", "res 2,a",
    "res 3,b", "res 3,c", "res 3,d", "res 3,e", "res 3,h", "res 3,l", "res 3,[hl]", "res 3,a",
    "res 4,b", "res 4,c", "res 4,d", "res 4,e", "res 4,h", "res 4,l", "res 4,[hl]", "res 4,a",
    "res 5,b", "res 5,c", "res 5,d", "res 5,e", "res 5,h", "res 5,l", "res 5,[hl]", "res 5,a",
    "res 6,b", "res 6,c", "res 6,d", "res 6,e", "res 6,h", "res 6,l", "res 6,[hl]", "res 6,a",
    "res 7,b", "res 7,c", "res 7,d", "res 7,e", "res 7,h", "res 7,l", "res 7,[hl]", "res 7,a",
    "set 0,b", "set 0,c", "set 0,d", "set 0,e", "set 0,h", "set 0,l", "set 0,[hl]", "set 0,a",
    "set 1,b", "set 1,c", "set 1,d", "set 1,e", "set 1,h", "set 1,l", "set 1,[hl]", "set 1,a",
    "set 2,b", "set 2,c", "set 2,d", "set 2,e", "set 2,h", "set 2,l", "set 2,[hl]", "set 2,a",
    "set 3,b", "set 3,c", "set 3,d", "set 3,e", "set 3,h", "set 3,l", "set 3,[hl]", "set 3,a",
    "set 4,b", "set 4,c", "set 4,d", "set 4,e", "set 4,h", "set 4,l", "set 4,[hl]", "set 4,a",
    "set 5,b", "set 5,c", "set 5,d", "set 5,e", "set 5,h", "set 5,l", "set 5,[hl]", "set 5,a",
    "set 6,b", "set 6,c", "set 6,d", "set 6,e", "set 6,h", "set 6,l", "set 6,[hl]", "set 6,a",
    "set 7,b", "set 7,c", "set 7,d", "set 7,e", "set 7,h", "set 7,l", "set 7,[hl]", "set 7,a"
};

#define OP_NONE    0
#define RW_BC      1
#define RW_DE      2
#define RW_HL      3
#define RW_SP      4
#define RW_INST    5
#define RW_FF_INST 6
#define RW_FF_C    7
#define JMP_REL    8
#define JMP_ABS    9
#define JMP_HL     10
#define OP_CALL    11

#define COND_NZ    (1<<16)
#define COND_Z     (2<<16)
#define COND_NC    (3<<16)
#define COND_C     (4<<16)

static const int debug_commands_info[256] = {
    OP_NONE, OP_NONE, RW_BC, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    RW_INST, OP_NONE, RW_BC, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    OP_NONE, OP_NONE, RW_DE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    JMP_REL, OP_NONE, RW_DE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    JMP_REL|COND_NZ, OP_NONE, RW_HL, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    JMP_REL|COND_Z, OP_NONE, RW_HL, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    JMP_REL|COND_NC, OP_NONE, RW_HL, OP_NONE, RW_HL, RW_HL, RW_HL, OP_NONE,
    JMP_REL|COND_C, OP_NONE, RW_HL, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    RW_HL, RW_HL, RW_HL, RW_HL, RW_HL, RW_HL, OP_NONE, RW_HL,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE|COND_NZ, OP_NONE, JMP_ABS|COND_NZ, JMP_ABS, OP_CALL|COND_NZ, OP_NONE, OP_NONE, OP_CALL,
    OP_NONE|COND_Z, OP_NONE, JMP_ABS|COND_Z, OP_NONE, OP_CALL|COND_Z, OP_CALL, OP_NONE, OP_CALL,
    OP_NONE|COND_NC, OP_NONE, JMP_ABS|COND_NC, OP_NONE, OP_CALL|COND_NC, OP_NONE, OP_NONE, OP_CALL,
    OP_NONE|COND_C, OP_NONE, JMP_ABS|COND_C, OP_NONE, OP_CALL|COND_C, OP_CALL, OP_NONE, OP_CALL,
    RW_FF_INST, OP_NONE, RW_FF_C, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_CALL,
    OP_NONE, JMP_HL, RW_INST, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_CALL,
    RW_FF_INST, OP_NONE, RW_FF_C, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_CALL,
    OP_NONE, OP_NONE, RW_INST, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_CALL
};

static const int debug_commands_cb_info[256] = {
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE,
    OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, RW_HL, OP_NONE
};

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------------------------

inline int gb_debug_get_address_increment(u32 address)
{
    int temp;

    switch(address >> 12)
    {
        case 0:
            if(address > 0x103 && address < 0x150) return 1;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            temp = debug_command_param_size[GB_MemRead8(address)];
            if(temp == 3) temp = 1;
            return temp + 1;
        case 8:
        case 9:
        case 0xA:
        case 0xB:
            return 1;
        case 0xC:
        case 0xD:
        case 0xE:
            temp = debug_command_param_size[GB_MemRead8(address)];
            if(temp == 3) temp = 1;
            return temp + 1;
        case 0xF:
            if(address < 0xFF80) return 1;
            temp = debug_command_param_size[GB_MemRead8(address)];
            if(temp == 3) temp = 1;
            return temp + 1;
    }

    return 1;
}

inline int gb_debug_get_address_is_code(u32 address)
{
    const int iscode[16] = { -1,1,1,1,
                              1,1,1,1,
                              0,0,0,0,
                              1,1,1,-1 };

    int temp = iscode[address >> 12];
    //if(temp >= 0) return temp;
    if(temp == 1) return 1;

    if(address > 0x103 && address < 0x150) return 0;
    if(address >= 0xFF00 && address < 0xFF80) return 0;

    return 1;
}

//------------------------------------------------------------------------------------------------

static struct {
    char * name;
    u32 address;
} gb_io_reg_struct[] = {
    {"P1_REG",0xFF00}, {"SB_REG",0xFF01}, {"SC_REG",0xFF02}, {"DIV_REG",0xFF04}, {"TIMA_REG",0xFF05},
    {"TMA_REG",0xFF06}, {"TAC_REG",0xFF07}, {"IF_REG",0xFF0F},
    {"NR10_REG",0xFF10}, {"NR11_REG",0xFF11}, {"NR12_REG",0xFF12}, {"NR13_REG",0xFF13}, {"NR14_REG",0xFF14},
    {"NR21_REG",0xFF16}, {"NR22_REG",0xFF17}, {"NR23_REG",0xFF18}, {"NR24_REG",0xFF19},
    {"NR30_REG",0xFF1A}, {"NR31_REG",0xFF1B}, {"NR32_REG",0xFF1C}, {"NR33_REG",0xFF1D}, {"NR34_REG",0xFF1E},
    {"NR41_REG",0xFF20}, {"NR42_REG",0xFF21}, {"NR43_REG",0xFF22}, {"NR44_REG",0xFF23},
    {"NR50_REG",0xFF24}, {"NR51_REG",0xFF25}, {"NR52_REG",0xFF26},
    {"LCDC_REG",0xFF40}, {"STAT_REG",0xFF41}, {"SCY_REG",0xFF42}, {"SCX_REG",0xFF43}, {"LY_REG",0xFF44},
    {"LYC_REG",0xFF45}, {"DMA_REG",0xFF46}, {"BGP_REG",0xFF47}, {"OBP0_REG",0xFF48}, {"OBP1_REG",0xFF49},
    {"WY_REG",0xFF4A}, {"WX_REG",0xFF4B}, {"KEY1_REG",0xFF4D}, {"VBK_REG",0xFF4F},
    {"HDMA1_REG",0xFF51}, {"HDMA2_REG",0xFF52}, {"HDMA3_REG",0xFF53}, {"HDMA4_REG",0xFF54}, {"HDMA5_REG",0xFF55},
    {"RP_REG",0xFF56}, {"BCPS_REG",0xFF68}, {"BCPD_REG",0xFF69}, {"OCPS_REG",0xFF6A}, {"OCPD_REG",0xFF6B},
    {"SVBK_REG",0xFF70}, {"IE_REG",0xFFFF},
    {NULL, 0}
};

int gb_dissasemble_add_io_register_name(int reg_address, char * dest, int add_comment)
{
    int i = 0;
    while(1)
    {
        if(gb_io_reg_struct[i].name == NULL) break;
        if(gb_io_reg_struct[i].address == reg_address)
        {
            if(add_comment)
            {
                strcat(dest," ; ");
                strcat(dest,gb_io_reg_struct[i].name);
            }
            else
            {
                strcat(dest," - ");
                strcat(dest,gb_io_reg_struct[i].name);
            }
            return 1;
        }
        i++;
    }
    return 0;
}


static char text[128];
char * GB_Dissasemble(u16 addr, int * step)
{
    if((addr == GameBoy.CPU.Reg16.PC) || gb_debug_get_address_is_code(addr))
    {
        u16 op_addr = addr;

        u8 cmd = GB_MemRead8(addr++);
        int paramsize = debug_command_param_size[cmd];

        int info = 0;
        int param = 0;

        if(paramsize == 0)
        {
            *step = 1;
            sprintf(text,"%02X       %s",cmd,debug_commands[cmd]);
            info = debug_commands_info[cmd];
            param = 0;
        }
        else if(paramsize == 1)
        {
            char instr_text[64];

            addr &= 0xFFFF;
            param = (int)(u8)GB_MemRead8(addr++);
            if(debug_commands[cmd] == NULL)
            {
                if(cmd == 0x10) //stop
                {
                    strcpy(instr_text, (param == 0x00) ? "stop" : "stop ; [!] corrupted");
                    param = 0;
                    info = 0;
                }
                else if(cmd == 0xCB)
                {
                    strcpy(instr_text,debug_commands_cb[param]);
                    param = 0;
                    info = debug_commands_cb_info[param];
                }
                else
                {
                    char temp[32];
                    strcpy(temp, debug_commands[cmd]);
                    info = debug_commands_info[cmd];
                    sprintf(instr_text,temp,param);
                }
            }
            else
            {
                char temp[32];
                strcpy(temp, debug_commands[cmd]);
                info = debug_commands_info[cmd];
                sprintf(instr_text,temp,param);
            }

            sprintf(text,"%02X%02X     %s",cmd,param,instr_text);
            *step = 2;
        }
        else if(paramsize == 2)
        {
            char instr_text[64];

            addr &= 0xFFFF;
            u8 param1 = GB_MemRead8(addr++);
            addr &= 0xFFFF;
            u8 param2 = GB_MemRead8(addr++);
            param = param1 | (param2 << 8);
            char temp[32];
            strcpy(temp, debug_commands[cmd]);
            sprintf(instr_text,temp,param);
            info = debug_commands_info[cmd];
            sprintf(text,"%02X%02X%02X   %s",cmd,param1,param2,instr_text);
            *step = 3;
        }
        else if(paramsize == 3) //jump relative
        {
            param = (s8)GB_MemRead8(addr++);
            char temp[32];
            char instr_text[64];
            strcpy(temp, debug_commands[cmd]);
            sprintf(instr_text,temp,param + addr);
            info = debug_commands_info[cmd];
            sprintf(text,"%02X%02X     %s",cmd,(u8)param,instr_text);
            param = param + addr;
            *step = 2;
        }

        //add extra information
        int comment_added = 0;
        switch(info&0xFFFF)
        {
            case OP_NONE:
                break;
            case RW_BC:
                comment_added = gb_dissasemble_add_io_register_name(GameBoy.CPU.Reg16.BC,text,1);
                break;
            case RW_DE:
                comment_added = gb_dissasemble_add_io_register_name(GameBoy.CPU.Reg16.DE,text,1);
                break;
            case RW_HL:
                comment_added = gb_dissasemble_add_io_register_name(GameBoy.CPU.Reg16.HL,text,1);
                break;
            case RW_SP:
                comment_added = gb_dissasemble_add_io_register_name(GameBoy.CPU.Reg16.SP,text,1);
                break;
            case RW_INST:
                comment_added = gb_dissasemble_add_io_register_name(param,text,1);
                break;
            case RW_FF_INST:
                comment_added = gb_dissasemble_add_io_register_name(param|0xFF00,text,1);
                break;
            case RW_FF_C:
                comment_added = gb_dissasemble_add_io_register_name(((int)(u8)GameBoy.CPU.Reg8.C)|0xFF00,text,1);
                break;
            case JMP_HL:
                param = (int)(u16)GameBoy.CPU.Reg16.HL;
            case JMP_REL:
            case JMP_ABS:
                if(param > op_addr) strcat(text," ; " STR_SLIM_ARROW_DOWN);
                else if(param == op_addr) strcat(text," ; <-");
                else strcat(text," ; " STR_SLIM_ARROW_UP);
                comment_added = 1;
                break;
            case OP_CALL:
                strcat(text," ; ->");
                comment_added = 1;
                break;
        }

        if(op_addr == GameBoy.CPU.Reg16.PC)
        {
            int cond = info & 0xFFFF0000;
            if(cond)
            {
                int cond_true = 0;
                if(cond == COND_NZ)
                    cond_true = GameBoy.CPU.Flags.Zero == false;
                else if(cond == COND_Z)
                    cond_true = GameBoy.CPU.Flags.Zero == true;
                else if(cond == COND_NC)
                    cond_true = GameBoy.CPU.Flags.Carry == false;
                else if(cond == COND_C)
                    cond_true = GameBoy.CPU.Flags.Carry == true;

                if(comment_added)
                {
                    if(cond_true) strcat(text," - true");
                    else strcat(text," - false");
                }
                else
                {
                    if(cond_true) strcat(text," ; true");
                    else strcat(text," ; false");
                }
            }
        }
    }
    else //not code...
    {
        u8 data = GB_MemRead8(addr);
        sprintf(text,"%02X       db 0x%02X",data,data);
        *step = 1;
    }

    return text;
}


//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

u32 gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };

void GB_Debug_Get_Palette(int is_sprite, int num, int color, u32 * red, u32 * green, u32 * blue)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    if(GameBoy.Emulator.CGBEnabled)
    {
        u32 color_;

        if(is_sprite)
        {
            color_ = GameBoy.Emulator.spr_pal[(num*8)+(color*2)] |
                     GameBoy.Emulator.spr_pal[(num*8)+(color*2)+1]<<8;
        }
        else
        {
            color_ = GameBoy.Emulator.bg_pal[(num*8)+(color*2)] |
                     GameBoy.Emulator.bg_pal[(num*8)+(color*2)+1]<<8;
        }

        *red = (color_ & 0x1F) << 3;
        *green = ((color_>>5) & 0x1F) << 3;
        *blue = ((color_>>10) & 0x1F) << 3;
    }
    else if(GameBoy.Emulator.gbc_in_gb_mode)
    {
        if(is_sprite)
        {
            if(num == 0)
            {
                u32 obp0_reg = mem->IO_Ports[OBP0_REG-0xFF00];
                u32 out_color = (obp0_reg>>(color*2))&0x3;
                out_color = gbc_getsprpalcolor(0,out_color);
                *red = (out_color & 0x1F) << 3;
                *green = ((out_color>>5) & 0x1F) << 3;
                *blue = ((out_color>>10) & 0x1F) << 3;
            }
            else if(num == 1)
            {
                u32 obp1_reg = mem->IO_Ports[OBP1_REG-0xFF00];
                u32 out_color = (obp1_reg>>(color*2))&0x3;
                out_color = gbc_getsprpalcolor(1,out_color);
                *red = (out_color & 0x1F) << 3;
                *green = ((out_color>>5) & 0x1F) << 3;
                *blue = ((out_color>>10) & 0x1F) << 3;
            }
            else
            {
                *red = *green = *blue = 0;
            }
        }
        else
        {
            if(num == 0)
            {
                u32 bgp_reg = mem->IO_Ports[BGP_REG-0xFF00];
                u32 out_color = (bgp_reg>>(color*2))&0x3;
                out_color = gbc_getbgpalcolor(0,out_color);
                *red = (out_color & 0x1F) << 3;
                *green = ((out_color>>5) & 0x1F) << 3;
                *blue = ((out_color>>10) & 0x1F) << 3;
            }
            else
            {
                *red = *green = *blue = 0;
            }
        }
    }
    else
    {
        if(is_sprite)
        {
            if(num == 0)
            {
                u32 obp0_reg = mem->IO_Ports[OBP0_REG-0xFF00];
                u32 out_color = (obp0_reg>>(color*2))&0x3;
                *red = gb_pal_colors[out_color][0];
                *green = gb_pal_colors[out_color][1];
                *blue = gb_pal_colors[out_color][2];
            }
            else if(num == 1)
            {
                u32 obp1_reg = mem->IO_Ports[OBP1_REG-0xFF00];
                u32 pal = (obp1_reg>>(color*2))&0x3;
                *red = gb_pal_colors[pal][0];
                *green = gb_pal_colors[pal][1];
                *blue = gb_pal_colors[pal][2];
            }
            else
            {
                *red = *green = *blue = 0;
            }
        }
        else
        {
            if(num == 0)
            {
                u32 bgp_reg = mem->IO_Ports[BGP_REG-0xFF00];
                u32 out_color = (bgp_reg>>(color*2))&0x3;
                *red = gb_pal_colors[out_color][0];
                *green = gb_pal_colors[out_color][1];
                *blue = gb_pal_colors[out_color][2];
            }
            else
            {
                *red = *green = *blue = 0;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------



