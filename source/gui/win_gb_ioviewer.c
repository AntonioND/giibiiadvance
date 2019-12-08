// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include <SDL.h>

#include "../debug_utils.h"
#include "../font_utils.h"
#include "../general_utils.h"
#include "../window_handler.h"

#include "win_gb_debugger.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/memory.h"

//------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------

static int WinIDGBIOViewer;

#define WIN_GB_IOVIEWER_WIDTH  517
#define WIN_GB_IOVIEWER_HEIGHT 262

static int GBIOViewerCreated = 0;

//------------------------------------------------------------------------------

static _gui_console gb_ioview_screen_con;
static _gui_element gb_ioview_screen_textbox, gb_ioview_screen_label;

static _gui_console gb_ioview_gbpal_con;
static _gui_element gb_ioview_gbpal_textbox, gb_ioview_gbpal_label;

static _gui_console gb_ioview_gbcpal_con;
static _gui_element gb_ioview_gbcpal_textbox, gb_ioview_gbcpal_label;

static _gui_console gb_ioview_other_con;
static _gui_element gb_ioview_other_textbox, gb_ioview_other_label;

static _gui_console gb_ioview_gbcdma_con;
static _gui_element gb_ioview_gbcdma_textbox, gb_ioview_gbcdma_label;

static _gui_console gb_ioview_sndchannel1_con;
static _gui_element gb_ioview_sndchannel1_textbox, gb_ioview_sndchannel1_label;

static _gui_console gb_ioview_sndchannel2_con;
static _gui_element gb_ioview_sndchannel2_textbox, gb_ioview_sndchannel2_label;

static _gui_console gb_ioview_sndchannel3_con;
static _gui_element gb_ioview_sndchannel3_textbox, gb_ioview_sndchannel3_label;

static _gui_console gb_ioview_sndchannel4_con;
static _gui_element gb_ioview_sndchannel4_textbox, gb_ioview_sndchannel4_label;

static _gui_console gb_ioview_sndcontrol_con;
static _gui_element gb_ioview_sndcontrol_textbox, gb_ioview_sndcontrol_label;

static _gui_console gb_ioview_dmainfo_con;
static _gui_element gb_ioview_dmainfo_textbox, gb_ioview_dmainfo_label;

static _gui_console gb_ioview_mbc_con;
static _gui_element gb_ioview_mbc_textbox, gb_ioview_mbc_label;

static _gui_console gb_ioview_cpuspeed_con;
static _gui_element gb_ioview_cpuspeed_textbox;

static _gui_console gb_ioview_irq_con;
static _gui_element gb_ioview_irq_textbox, gb_ioview_irq_label;

static _gui_console gb_ioview_clocks_con;
static _gui_element gb_ioview_clocks_textbox, gb_ioview_clocks_label;

static _gui_element *gb_ioviwer_window_gui_elements[] = {
    &gb_ioview_screen_textbox, &gb_ioview_screen_label,
    &gb_ioview_gbpal_textbox, &gb_ioview_gbpal_label,
    &gb_ioview_gbcpal_textbox, &gb_ioview_gbcpal_label,
    &gb_ioview_other_textbox, &gb_ioview_other_label,
    &gb_ioview_gbcdma_textbox, &gb_ioview_gbcdma_label,
    &gb_ioview_sndchannel1_textbox, &gb_ioview_sndchannel1_label,
    &gb_ioview_sndchannel2_textbox, &gb_ioview_sndchannel2_label,
    &gb_ioview_sndchannel3_textbox, &gb_ioview_sndchannel3_label,
    &gb_ioview_sndchannel4_textbox, &gb_ioview_sndchannel4_label,
    &gb_ioview_sndcontrol_textbox, &gb_ioview_sndcontrol_label,
    &gb_ioview_dmainfo_textbox, &gb_ioview_dmainfo_label,
    &gb_ioview_mbc_textbox, &gb_ioview_mbc_label,
    &gb_ioview_cpuspeed_textbox,
    &gb_ioview_irq_textbox, &gb_ioview_irq_label,
    &gb_ioview_clocks_textbox, &gb_ioview_clocks_label,
    NULL
};

static _gui gb_ioviewer_window_gui = {
    gb_ioviwer_window_gui_elements,
    NULL,
    NULL
};

void Win_GBIOViewerUpdate(void)
{
    if (GBIOViewerCreated == 0)
        return;

    if (Win_MainRunningGB() == 0)
        return;

    GUI_ConsoleClear(&gb_ioview_screen_con);
    GUI_ConsoleClear(&gb_ioview_gbpal_con);
    GUI_ConsoleClear(&gb_ioview_gbcpal_con);
    GUI_ConsoleClear(&gb_ioview_other_con);
    GUI_ConsoleClear(&gb_ioview_gbcdma_con);
    GUI_ConsoleClear(&gb_ioview_sndchannel1_con);
    GUI_ConsoleClear(&gb_ioview_sndchannel2_con);
    GUI_ConsoleClear(&gb_ioview_sndchannel3_con);
    GUI_ConsoleClear(&gb_ioview_sndchannel4_con);
    GUI_ConsoleClear(&gb_ioview_sndcontrol_con);
    GUI_ConsoleClear(&gb_ioview_dmainfo_con);
    GUI_ConsoleClear(&gb_ioview_mbc_con);
    GUI_ConsoleClear(&gb_ioview_cpuspeed_con);
    GUI_ConsoleClear(&gb_ioview_irq_con);
    GUI_ConsoleClear(&gb_ioview_clocks_con);

    // Screen
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 0, "40h LCDC - %02X",
                          GB_MemReadReg8(0xFF40));
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 1, "41h STAT - %02X",
                          GB_MemReadReg8(0xFF41));
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 2, "42h SCY  - %02X",
                          GB_MemReadReg8(0xFF42));
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 3, "43h SCX  - %02X",
                          GB_MemReadReg8(0xFF43));
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 4, "44h LY   - %02X",
                          GB_MemReadReg8(0xFF44));
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 5, "45h LYC  - %02X",
                          GB_MemReadReg8(0xFF45));
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 6, "46h DMA  - %02X",
                          GB_MemReadReg8(0xFF46));
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 7, "4Ah WY   - %02X",
                          GB_MemReadReg8(0xFF4A));
    GUI_ConsoleModePrintf(&gb_ioview_screen_con, 0, 8, "4Bh WX   - %02X",
                          GB_MemReadReg8(0xFF4B));

    // GB Palettes
    GUI_ConsoleModePrintf(&gb_ioview_gbpal_con, 0, 0, "47h BGP  - %02X",
                          GB_MemReadReg8(0xFF47));
    GUI_ConsoleModePrintf(&gb_ioview_gbpal_con, 0, 1, "48h OBP0 - %02X",
                          GB_MemReadReg8(0xFF48));
    GUI_ConsoleModePrintf(&gb_ioview_gbpal_con, 0, 2, "49h OBP1 - %02X",
                          GB_MemReadReg8(0xFF49));

    // GBC Palettes
    GUI_ConsoleModePrintf(&gb_ioview_gbcpal_con, 0, 0, "68h BCPS - %02X",
                          GB_MemReadReg8(0xFF68));
    GUI_ConsoleModePrintf(&gb_ioview_gbcpal_con, 0, 1, "69h BCPD - %02X",
                          GB_MemReadReg8(0xFF69));
    GUI_ConsoleModePrintf(&gb_ioview_gbcpal_con, 0, 2, "6Ah OCPS - %02X",
                          GB_MemReadReg8(0xFF6A));
    GUI_ConsoleModePrintf(&gb_ioview_gbcpal_con, 0, 3, "6Bh OCPD - %02X",
                          GB_MemReadReg8(0xFF6B));

    // Other
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 0, "00h P1   - %02X",
                          GB_MemReadReg8(0xFF00));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 1, "01h SB   - %02X",
                          GB_MemReadReg8(0xFF01));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 2, "02h SC   - %02X",
                          GB_MemReadReg8(0xFF02));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 3, "04h DIV  - %02X",
                          GB_MemReadReg8(0xFF04));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 4, "05h TIMA - %02X",
                          GB_MemReadReg8(0xFF05));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 5, "06h TMA  - %02X",
                          GB_MemReadReg8(0xFF06));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 6, "07h TAC  - %02X",
                          GB_MemReadReg8(0xFF07));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 7, "0Fh IF   - %02X",
                          GB_MemReadReg8(0xFF0F));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 8, "4Dh KEY1 - %02X",
                          GB_MemReadReg8(0xFF4D));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 9, "4Fh VBK  - %02X",
                          GB_MemReadReg8(0xFF4F));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 10, "56h RP   - %02X",
                          GB_MemReadReg8(0xFF56));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 11, "70h SVBK - %02X",
                          GB_MemReadReg8(0xFF70));
    GUI_ConsoleModePrintf(&gb_ioview_other_con, 0, 12, "FFh IE   - %02X",
                          GB_MemRead8(0xFFFF));

    // GBC DMA
    GUI_ConsoleModePrintf(&gb_ioview_gbcdma_con, 0, 0, "51h SRCH - %02X",
                          GB_MemReadReg8(0xFF51));
    GUI_ConsoleModePrintf(&gb_ioview_gbcdma_con, 0, 1, "52h SRCL - %02X",
                          GB_MemReadReg8(0xFF52));
    GUI_ConsoleModePrintf(&gb_ioview_gbcdma_con, 0, 2, "53h DSTH - %02X",
                          GB_MemReadReg8(0xFF53));
    GUI_ConsoleModePrintf(&gb_ioview_gbcdma_con, 0, 3, "54h DSTL - %02X",
                          GB_MemReadReg8(0xFF54));
    GUI_ConsoleModePrintf(&gb_ioview_gbcdma_con, 0, 4, "55h CTRL - %02X",
                          GB_MemReadReg8(0xFF55));

    // Sound channel 1
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel1_con, 0, 0, "10h NR10 - %02X",
                          GB_MemReadReg8(0xFF10));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel1_con, 0, 1, "11h NR11 - %02X",
                          GB_MemReadReg8(0xFF11));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel1_con, 0, 2, "12h NR12 - %02X",
                          GB_MemReadReg8(0xFF12));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel1_con, 0, 3, "13h NR13 - %02X",
                          GB_MemReadReg8(0xFF13));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel1_con, 0, 4, "14h NR14 - %02X",
                          GB_MemReadReg8(0xFF14));

    // Sound channel 2
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel2_con, 0, 0, "             ");
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel2_con, 0, 1, "16h NR21 - %02X",
                          GB_MemReadReg8(0xFF16));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel2_con, 0, 2, "17h NR22 - %02X",
                          GB_MemReadReg8(0xFF17));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel2_con, 0, 3, "18h NR23 - %02X",
                          GB_MemReadReg8(0xFF18));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel2_con, 0, 4, "19h NR24 - %02X",
                          GB_MemReadReg8(0xFF19));

    // Sound channel 3
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel3_con, 0, 0, "1Ah NR30 - %02X",
                          GB_MemReadReg8(0xFF1A));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel3_con, 0, 1, "1Bh NR31 - %02X",
                          GB_MemReadReg8(0xFF1B));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel3_con, 0, 2, "1Ch NR32 - %02X",
                          GB_MemReadReg8(0xFF1C));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel3_con, 0, 3, "1Dh NR33 - %02X",
                          GB_MemReadReg8(0xFF1D));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel3_con, 0, 4, "1Eh NR34 - %02X",
                          GB_MemReadReg8(0xFF1E));

    // Sound channel 4
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel4_con, 0, 0, "             ");
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel4_con, 0, 1, "20h NR41 - %02X",
                          GB_MemReadReg8(0xFF20));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel4_con, 0, 2, "21h NR42 - %02X",
                          GB_MemReadReg8(0xFF21));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel4_con, 0, 3, "22h NR43 - %02X",
                          GB_MemReadReg8(0xFF22));
    GUI_ConsoleModePrintf(&gb_ioview_sndchannel4_con, 0, 4, "23h NR44 - %02X",
                          GB_MemReadReg8(0xFF23));

    // Sound control
    GUI_ConsoleModePrintf(&gb_ioview_sndcontrol_con, 0, 0, "24h NR50 - %02X",
                          GB_MemReadReg8(0xFF24));
    GUI_ConsoleModePrintf(&gb_ioview_sndcontrol_con, 0, 1, "25h NR51 - %02X",
                          GB_MemReadReg8(0xFF25));
    GUI_ConsoleModePrintf(&gb_ioview_sndcontrol_con, 0, 2, "26h NR52 - %02X",
                          GB_MemReadReg8(0xFF26));

    // GBC DMA Information
    char *mode = (GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_NONE) ? "None" :
                    ((GameBoy.Emulator.GBC_DMA_enabled == GBC_DMA_GENERAL) ?
                        "GDMA" : "HDMA");
    GUI_ConsoleModePrintf(&gb_ioview_dmainfo_con, 0, 0, "Mode:   %s", mode);
    GUI_ConsoleModePrintf(&gb_ioview_dmainfo_con, 0, 1, "Source: %04X",
                          GameBoy.Emulator.gdma_src);
    GUI_ConsoleModePrintf(&gb_ioview_dmainfo_con, 0, 2, "Dest:   %04X",
                          GameBoy.Emulator.gdma_dst);
    GUI_ConsoleModePrintf(&gb_ioview_dmainfo_con, 0, 3, "Size:   %04X",
                          GameBoy.Emulator.gdma_bytes_left);

    // MBC Information
    GUI_ConsoleModePrintf(&gb_ioview_mbc_con, 0, 0, "MBC: %d",
                          GameBoy.Emulator.MemoryController);
    GUI_ConsoleModePrintf(&gb_ioview_mbc_con, 0, 1, "ROM: %d(%X)",
                          GameBoy.Memory.selected_rom, GameBoy.Memory.selected_rom);
    GUI_ConsoleModePrintf(&gb_ioview_mbc_con, 0, 2, "RAM: %d(%X)-%s",
                          GameBoy.Memory.selected_ram, GameBoy.Memory.selected_ram,
                          GameBoy.Memory.RAMEnabled ? "En" : "Dis");
    GUI_ConsoleModePrintf(&gb_ioview_mbc_con, 0, 3, "Mode: %d",
                          GameBoy.Memory.mbc_mode);

    // CPU Speed
    GUI_ConsoleModePrintf(&gb_ioview_cpuspeed_con, 0, 0,
                          (GameBoy.Emulator.DoubleSpeed == 1) ?
                          "Speed: 8Mhz (double)" : "Speed: 4Mhz (normal)");

    // IRQs
    GUI_ConsoleModePrintf(&gb_ioview_irq_con, 0, 0, "IME: %d HLT: %d",
                          GameBoy.Memory.InterruptMasterEnable,
                          GameBoy.Emulator.CPUHalt);

    int __ie = GB_MemRead8(IE_REG);
    int __if = GB_MemRead8(IF_REG);
    GUI_ConsoleModePrintf(&gb_ioview_irq_con, 0, 1, "       IE IF");
    GUI_ConsoleModePrintf(&gb_ioview_irq_con, 0, 2, "VBLANK [%c][%c]",
                          (__ie & 1) ? CHR_SQUAREBLACK_MID : ' ',
                          (__if & 1) ? CHR_SQUAREBLACK_MID : ' ');
    GUI_ConsoleModePrintf(&gb_ioview_irq_con, 0, 3, "STAT   [%c][%c]",
                          (__ie & 2) ? CHR_SQUAREBLACK_MID : ' ',
                          (__if & 2) ? CHR_SQUAREBLACK_MID : ' ');
    GUI_ConsoleModePrintf(&gb_ioview_irq_con, 0, 4, "TIMER  [%c][%c]",
                          (__ie & 4) ? CHR_SQUAREBLACK_MID : ' ',
                          (__if & 4) ? CHR_SQUAREBLACK_MID : ' ');
    GUI_ConsoleModePrintf(&gb_ioview_irq_con, 0, 5, "SERIAL [%c][%c]",
                          (__ie & 8) ? CHR_SQUAREBLACK_MID : ' ',
                          (__if & 8) ? CHR_SQUAREBLACK_MID : ' ');
    GUI_ConsoleModePrintf(&gb_ioview_irq_con, 0, 6, "JOYPAD [%c][%c]",
                          (__ie & 16) ? CHR_SQUAREBLACK_MID : ' ',
                          (__if & 16) ? CHR_SQUAREBLACK_MID : ' ');

    // Clocks
    GUI_ConsoleModePrintf(&gb_ioview_clocks_con, 0, 0, "LY: %d",
                          GameBoy.Emulator.ly_clocks);
    if (GameBoy.Emulator.timer_enabled)
    {
        GUI_ConsoleModePrintf(&gb_ioview_clocks_con, 0, 1, "TIMA: %d",
            GameBoy.Emulator.sys_clocks & GameBoy.Emulator.timer_overflow_mask);
    }
    else
    {
        GUI_ConsoleModePrintf(&gb_ioview_clocks_con, 0, 1, "TIMA: (%d)",
            GameBoy.Emulator.sys_clocks & GameBoy.Emulator.timer_overflow_mask);
    }

    GUI_ConsoleModePrintf(&gb_ioview_clocks_con, 0, 2, "  out of %d",
                          GameBoy.Emulator.timer_overflow_mask + 1);
    GUI_ConsoleModePrintf(&gb_ioview_clocks_con, 0, 3, "DIV: %d",
                          GameBoy.Emulator.sys_clocks & 0xFF);
    GUI_ConsoleModePrintf(&gb_ioview_clocks_con, 0, 4, "SIO: %3d (%d)",
        GameBoy.Emulator.serial_clocks
            & (GameBoy.Emulator.serial_clocks_to_flip_clock_signal - 1),
        GameBoy.Emulator.serial_clock_signal);
    GUI_ConsoleModePrintf(&gb_ioview_clocks_con, 0, 5, "  out of %3d",
        GameBoy.Emulator.serial_clocks_to_flip_clock_signal);
}

//----------------------------------------------------------------

void Win_GBIOViewerRender(void)
{
    if (GBIOViewerCreated == 0)
        return;

    char buffer[WIN_GB_IOVIEWER_WIDTH * WIN_GB_IOVIEWER_HEIGHT * 3];
    GUI_Draw(&gb_ioviewer_window_gui, buffer, WIN_GB_IOVIEWER_WIDTH,
             WIN_GB_IOVIEWER_HEIGHT, 1);

    WH_Render(WinIDGBIOViewer, buffer);
}

static int _win_gb_io_viewer_callback(SDL_Event *e)
{
    if (GBIOViewerCreated == 0)
        return 1;

    int redraw = GUI_SendEvent(&gb_ioviewer_window_gui, e);

    int close_this = 0;

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
            close_this = 1;
    }

    if (close_this)
    {
        GBIOViewerCreated = 0;
        WH_Close(WinIDGBIOViewer);
        return 1;
    }

    if (redraw)
    {
        Win_GBIOViewerUpdate();
        Win_GBIOViewerRender();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

int Win_GBIOViewerCreate(void)
{
    if (Win_MainRunningGB() == 0)
        return 0;

    if (GBIOViewerCreated == 1)
    {
        WH_Focus(WinIDGBIOViewer);
        return 0;
    }

    GUI_SetLabel(&gb_ioview_screen_label,
                 6, 6, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "Screen");
    GUI_SetTextBox(&gb_ioview_screen_textbox, &gb_ioview_screen_con,
                   6, 20, 13 * FONT_WIDTH + 2, 9 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_gbpal_label,
                 6, 136, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "GB Palettes");
    GUI_SetTextBox(&gb_ioview_gbpal_textbox, &gb_ioview_gbpal_con,
                   6, 150, 13 * FONT_WIDTH + 2, 3 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_gbcpal_label,
                 6, 194, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "GBC Palettes");
    GUI_SetTextBox(&gb_ioview_gbcpal_textbox, &gb_ioview_gbcpal_con,
                   6, 208, 13 * FONT_WIDTH + 2, 4 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_other_label,
                 109, 6, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "Other");
    GUI_SetTextBox(&gb_ioview_other_textbox, &gb_ioview_other_con,
                   109, 20, 13 * FONT_WIDTH + 2, 13 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_gbcdma_label,
                 109, 182, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "GBC DMA");
    GUI_SetTextBox(&gb_ioview_gbcdma_textbox, &gb_ioview_gbcdma_con,
                   109, 196, 13 * FONT_WIDTH + 2, 5 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_sndchannel1_label,
                 212, 6, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "Snd Channel 1");
    GUI_SetTextBox(&gb_ioview_sndchannel1_textbox, &gb_ioview_sndchannel1_con,
                   212, 20, 13 * FONT_WIDTH + 2, 5 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_sndchannel2_label,
                 315, 6, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "Snd Channel 2");
    GUI_SetTextBox(&gb_ioview_sndchannel2_textbox, &gb_ioview_sndchannel2_con,
                   315, 20, 13 * FONT_WIDTH + 2, 5 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_sndchannel3_label,
                 212, 88, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "Snd Channel 3");
    GUI_SetTextBox(&gb_ioview_sndchannel3_textbox, &gb_ioview_sndchannel3_con,
                   212, 102, 13 * FONT_WIDTH + 2, 5 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_sndchannel4_label,
                 315, 88, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "Snd Channel 4");
    GUI_SetTextBox(&gb_ioview_sndchannel4_textbox, &gb_ioview_sndchannel4_con,
                   315, 102, 13 * FONT_WIDTH + 2, 5 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_sndcontrol_label,
                 418, 6, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "Snd Control");
    GUI_SetTextBox(&gb_ioview_sndcontrol_textbox, &gb_ioview_sndcontrol_con,
                   418, 20, 13 * FONT_WIDTH + 2, 3 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_dmainfo_label,
                 212, 170, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "GBC DMA Info.");
    GUI_SetTextBox(&gb_ioview_dmainfo_textbox, &gb_ioview_dmainfo_con,
                   212, 184, 13 * FONT_WIDTH + 2, 4 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_mbc_label,
                 315, 170, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "MBC Info.");
    GUI_SetTextBox(&gb_ioview_mbc_textbox, &gb_ioview_mbc_con,
                   315, 184, 13 * FONT_WIDTH + 2, 4 * FONT_HEIGHT, NULL);

    GUI_SetTextBox(&gb_ioview_cpuspeed_textbox, &gb_ioview_cpuspeed_con,
                   212, 244, 28 * FONT_WIDTH, FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_irq_label,
                 418, 62, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "IRQs");
    GUI_SetTextBox(&gb_ioview_irq_textbox, &gb_ioview_irq_con,
                   418, 78, 13 * FONT_WIDTH + 2, 7 * FONT_HEIGHT, NULL);

    GUI_SetLabel(&gb_ioview_clocks_label,
                 418, 168, 13 * FONT_WIDTH + 2, FONT_HEIGHT, "Clocks");
    GUI_SetTextBox(&gb_ioview_clocks_textbox, &gb_ioview_clocks_con,
                   418, 184, 13 * FONT_WIDTH + 2, 6 * FONT_HEIGHT, NULL);

    GBIOViewerCreated = 1;

    WinIDGBIOViewer = WH_Create(WIN_GB_IOVIEWER_WIDTH,
                                WIN_GB_IOVIEWER_HEIGHT, 0, 0, 0);
    WH_SetCaption(WinIDGBIOViewer, "GB I/O Viewer");

    WH_SetEventCallback(WinIDGBIOViewer, _win_gb_io_viewer_callback);

    Win_GBIOViewerUpdate();
    Win_GBIOViewerRender();

    return 1;
}

void Win_GBIOViewerClose(void)
{
    if (GBIOViewerCreated == 0)
        return;

    GBIOViewerCreated = 0;
    WH_Close(WinIDGBIOViewer);
}
