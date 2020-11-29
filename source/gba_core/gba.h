// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA__
#define GBA__

#include "../general_utils.h"

//------------------------------------------------------------------------------

typedef enum
{
    CPU_USER = 0,
    CPU_FIQ = 1,
    CPU_IRQ = 2,
    CPU_SUPERVISOR = 3,
    CPU_ABORT = 4,
    CPU_UNDEFINED = 5,
    CPU_SYSTEM = 6,
    CPU_MODE_NUMBER = 7
} _cpu_mode_e;

#define M_USER       (0x10)
#define M_FIQ        (0x11)
#define M_IRQ        (0x12)
#define M_SUPERVISOR (0x13)
#define M_ABORT      (0x17)
#define M_UNDEFINED  (0x1B)
#define M_SYSTEM     (0x1F)

typedef enum
{
    EXEC_ARM,
    EXEC_THUMB
} _exec_mode_e;

#define R_SP (13)
#define R_LR (14)
#define R_PC (15)

//compiler doesn't like to have the same names here than in gb
#ifdef F_CARRY
# undef F_CARRY
#endif
#ifdef F_ZERO
# undef F_ZERO
#endif

#define F_SIGN     BIT(31) // (0=Not Signed, 1=Signed)
#define F_N        BIT(31)
#define F_ZERO     BIT(30) // (0=Not Zero, 1=Zero)
#define F_Z        BIT(30)
#define F_CARRY    BIT(29) // (0=No Carry, 1=Carry)
#define F_C        BIT(29)
#define F_OVERFLOW BIT(28) // (0=No Overflow, 1=Overflow)
#define F_V        BIT(28)
#define F_IRQ_DIS  BIT(7) // (0=Enable, 1=Disable)
#define F_I        BIT(7)
#define F_FIQ_DIS  BIT(6) // (0=Enable, 1=Disable)
#define F_F        BIT(6)
#define F_STATE    BIT(5) // (0=ARM, 1=THUMB) - Do not change manually!
#define F_T        BIT(5)

typedef struct
{
    u32 R[16]; // 0-15
    u32 CPSR;
    u32 SPSR;

    u32 OldPC;

    //-----------------------------------

    u32 R_user[7]; // 8-14

    u32 R_fiq[7]; // 8-14
    u32 SPSR_fiq; // FIQ

    u32 R13_svc, R14_svc; // Supervisor
    u32 SPSR_svc;

    u32 R13_abt, R14_abt; // Abort
    u32 SPSR_abt;

    u32 R13_irq, R14_irq; // IRQ
    u32 SPSR_irq;

    u32 R13_und, R14_und; // Undefined
    u32 SPSR_und;

    _cpu_mode_e MODE;
    _exec_mode_e EXECUTION_MODE;
} _cpu_t;

//------------------------------------------------------------------------------

typedef struct {
    // General Internal Memory
    u8 *rom_bios;         // 00000000-00003FFF | BIOS - (ROM)  | 16 KB
                          // 00004000-01FFFFFF | Not used      |
    ALIGNED(4)
    u8 ewram[256 * 1024]; // 02000000-0203FFFF | On-board WRAM | 256 KB (2 Wait)
                          // 02040000-02FFFFFF | Not used      |
    ALIGNED(4)
    u8 iwram[32 * 1024];  // 03000000-03007FFF | In-chip WRAM  | 32 KB
                          // 03008000-03FFFFFF | Not used      |
    ALIGNED(4)
    u8 io_regs[0x3FF];    // 04000000-040003FE | I/O Registers |
                          // 04000400-04FFFFFF | Not used      |

    // Internal Display Memory
    ALIGNED(4)
    u8 pal_ram[1024];     // 05000000-050003FF | BG/OBJ Palette RAM   | 1 KB
                          // 05000400-05FFFFFF | Not used             |
    ALIGNED(4)
    u8 vram[128 * 1024];  // 06000000-06017FFF | VRAM - Video RAM     | 96 KB
    // 128K to fix masks  // 06018000-06FFFFFF | Not used             |a
    ALIGNED(4)
    u8 oam[1024];         // 07000000-070003FF | OAM - OBJ Attributes | 1 KB
                          // 07000400-07FFFFFF | Not used             |

    // External Memory (Game Pak)
    u8 *rom_wait0;        // 08000000-09FFFFFF | Cart ROM  | 32 MB | Wait State 0
    u8 *rom_wait1;        // 0A000000-0BFFFFFF | Cart ROM  | 32 MB | Wait State 1
    u8 *rom_wait2;        // 0C000000-0DFFFFFF | Cart ROM  | 32 MB | Wait State 2
    //u8 sram[64 * 1024]; // 0E000000-0E00FFFF | Cart SRAM | 64 KB | 8-bit Bus
                          // 0E010000-0FFFFFFF | Not used  |

    // Unused Memory Area
                          // 10000000-FFFFFFFF | Not used  |
} _mem_t;

typedef struct
{
    u16 attr0;
    u16 attr1;
    u16 attr2;
    u16 dummy;
} _oam_spr_entry_t;

typedef struct
{
    u16 dummy0[3];
    s16 pa;
    u16 dummy1[3];
    s16 pb;
    u16 dummy2[3];
    s16 pc;
    u16 dummy3[3];
    s16 pd;
} _oam_matrix_entry_t;

int GBA_GetRomSize(void);

int GBA_InitRom(void *bios_ptr, void *rom_ptr, u32 romsize);
int GBA_EndRom(int save);
void GBA_Reset(void);

_cpu_t *GBA_CPUGet(void); // For the disassembler

void GBA_Screenshot(const char *path);

void GBA_RunForOneFrame(void);
void GBA_RunFor_ExecutionBreak(void);

void GBA_HandleInput(int a, int b, int l, int r, int st, int se,
                     int dr, int dl, int du, int dd);

u32 GBA_RunFor(s32 totalclocks);

void GBA_DebugStep(void);

#endif // GBA__
