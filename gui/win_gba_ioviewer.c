// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <SDL.h>

#include <string.h>

#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../general_utils.h"

#include "win_gba_debugger.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gba_core/gba.h"
#include "../gba_core/memory.h"
#include "../gba_core/timers.h"
#include "../gba_core/sound.h"

//-----------------------------------------------------------------------------------

static int WinIDGBAIOViewer;

#define WIN_GBA_IOVIEWER_WIDTH  593
#define WIN_GBA_IOVIEWER_HEIGHT 314

static int GBAIOViewerCreated = 0;

static int gba_ioview_selected_tab = 0;

//-----------------------------------------------------------------------------------

// Those are shared between the 6 pages
static _gui_element gba_ioview_display_tabbtn, gba_ioview_backgrounds_tabbtn, gba_ioview_dma_tabbtn,
                    gba_ioview_timers_tabbtn, gba_ioview_sound_tabbtn, gba_ioview_other_tabbtn;

//-----------------------

static _gui_console gba_ioview_display_lcdcontrol_con;
static _gui_element gba_ioview_display_lcdcontrol_textbox, gba_ioview_display_lcdcontrol_label;
static _gui_console gba_ioview_display_lcdstatus_con;
static _gui_element gba_ioview_display_lcdstatus_textbox, gba_ioview_display_lcdstatus_label;
static _gui_console gba_ioview_display_windows_con;
static _gui_element gba_ioview_display_windows_textbox, gba_ioview_display_windows_label;
static _gui_console gba_ioview_display_special_con;
static _gui_element gba_ioview_display_special_textbox, gba_ioview_display_special_label;

static _gui_element * gba_ioviwer_display_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    &gba_ioview_display_lcdcontrol_textbox, &gba_ioview_display_lcdcontrol_label,
    &gba_ioview_display_lcdstatus_textbox, &gba_ioview_display_lcdstatus_label,
    &gba_ioview_display_windows_textbox, &gba_ioview_display_windows_label,
    &gba_ioview_display_special_textbox, &gba_ioview_display_special_label,

    NULL
};

//-----------------------

static _gui_console gba_ioview_bgs_bgcontrol_con;
static _gui_element gba_ioview_bgs_bgcontrol_textbox, gba_ioview_bgs_bgcontrol_label;
static _gui_console gba_ioview_bgs_bgscroll_con;
static _gui_element gba_ioview_bgs_bgscroll_textbox, gba_ioview_bgs_bgscroll_label;
static _gui_console gba_ioview_bgs_affine_con;
static _gui_element gba_ioview_bgs_affine_textbox, gba_ioview_bgs_affine_label;
static _gui_console gba_ioview_bgs_vidmode_con;
static _gui_element gba_ioview_bgs_vidmode_textbox, gba_ioview_bgs_vidmode_label;

static _gui_element * gba_ioviwer_backgrounds_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    &gba_ioview_bgs_bgcontrol_textbox, &gba_ioview_bgs_bgcontrol_label,
    &gba_ioview_bgs_bgscroll_textbox, &gba_ioview_bgs_bgscroll_label,
    &gba_ioview_bgs_affine_textbox, &gba_ioview_bgs_affine_label,
    &gba_ioview_bgs_vidmode_textbox, &gba_ioview_bgs_vidmode_label,

    NULL
};

//-----------------------

static _gui_console gba_ioview_dma_dma0_con;
static _gui_element gba_ioview_dma_dma0_textbox, gba_ioview_dma_dma0_label;
static _gui_console gba_ioview_dma_dma1_con;
static _gui_element gba_ioview_dma_dma1_textbox, gba_ioview_dma_dma1_label;
static _gui_console gba_ioview_dma_dma2_con;
static _gui_element gba_ioview_dma_dma2_textbox, gba_ioview_dma_dma2_label;
static _gui_console gba_ioview_dma_dma3_con;
static _gui_element gba_ioview_dma_dma3_textbox, gba_ioview_dma_dma3_label;


static _gui_element * gba_ioviwer_dma_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    &gba_ioview_dma_dma0_textbox, &gba_ioview_dma_dma0_label,
    &gba_ioview_dma_dma1_textbox, &gba_ioview_dma_dma1_label,
    &gba_ioview_dma_dma2_textbox, &gba_ioview_dma_dma2_label,
    &gba_ioview_dma_dma3_textbox, &gba_ioview_dma_dma3_label,

    NULL
};

//-----------------------

static _gui_console gba_ioview_timers_tmr0_con;
static _gui_element gba_ioview_timers_tmr0_textbox, gba_ioview_timers_tmr0_label;
static _gui_console gba_ioview_timers_tmr1_con;
static _gui_element gba_ioview_timers_tmr1_textbox, gba_ioview_timers_tmr1_label;
static _gui_console gba_ioview_timers_tmr2_con;
static _gui_element gba_ioview_timers_tmr2_textbox, gba_ioview_timers_tmr2_label;
static _gui_console gba_ioview_timers_tmr3_con;
static _gui_element gba_ioview_timers_tmr3_textbox, gba_ioview_timers_tmr3_label;

static _gui_element * gba_ioviwer_timers_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    &gba_ioview_timers_tmr0_textbox, &gba_ioview_timers_tmr0_label,
    &gba_ioview_timers_tmr1_textbox, &gba_ioview_timers_tmr1_label,
    &gba_ioview_timers_tmr2_textbox, &gba_ioview_timers_tmr2_label,
    &gba_ioview_timers_tmr3_textbox, &gba_ioview_timers_tmr3_label,

    NULL
};

//-----------------------

static _gui_console gba_ioview_sound_chn1_con;
static _gui_element gba_ioview_sound_chn1_textbox, gba_ioview_sound_chn1_label;
static _gui_console gba_ioview_sound_chn2_con;
static _gui_element gba_ioview_sound_chn2_textbox, gba_ioview_sound_chn2_label;
static _gui_console gba_ioview_sound_chn3_con;
static _gui_element gba_ioview_sound_chn3_textbox, gba_ioview_sound_chn3_label;
static _gui_console gba_ioview_sound_chn4_con;
static _gui_element gba_ioview_sound_chn4_textbox, gba_ioview_sound_chn4_label;
static _gui_console gba_ioview_sound_control_con;
static _gui_element gba_ioview_sound_control_textbox, gba_ioview_sound_control_label;
static _gui_console gba_ioview_sound_waveram_con;
static _gui_element gba_ioview_sound_waveram_textbox, gba_ioview_sound_waveram_label;
static _gui_console gba_ioview_sound_pwm_con;
static _gui_element gba_ioview_sound_pwm_textbox, gba_ioview_sound_pwm_label;

static _gui_element * gba_ioviwer_sound_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    &gba_ioview_sound_chn1_textbox, &gba_ioview_sound_chn1_label,
    &gba_ioview_sound_chn2_textbox, &gba_ioview_sound_chn2_label,
    &gba_ioview_sound_chn3_textbox, &gba_ioview_sound_chn3_label,
    &gba_ioview_sound_chn4_textbox, &gba_ioview_sound_chn4_label,
    &gba_ioview_sound_control_textbox, &gba_ioview_sound_control_label,
    &gba_ioview_sound_waveram_textbox, &gba_ioview_sound_waveram_label,
    &gba_ioview_sound_pwm_textbox, &gba_ioview_sound_pwm_label,

    NULL
};

//-----------------------

static _gui_console gba_ioview_others_interrupts_con;
static _gui_element gba_ioview_others_interrupts_textbox, gba_ioview_others_interrupts_label;
static _gui_console gba_ioview_others_int_flags_con;
static _gui_element gba_ioview_others_int_flags_textbox, gba_ioview_others_int_flags_label;

static _gui_element * gba_ioviwer_other_elements[] = {
    &gba_ioview_display_tabbtn, &gba_ioview_backgrounds_tabbtn, &gba_ioview_dma_tabbtn,
    &gba_ioview_timers_tabbtn, &gba_ioview_sound_tabbtn, &gba_ioview_other_tabbtn,

    &gba_ioview_others_interrupts_textbox, &gba_ioview_others_interrupts_label,
    &gba_ioview_others_int_flags_textbox, &gba_ioview_others_int_flags_label,

    NULL
};

//-----------------------

static _gui gba_ioviewer_page_gui[] = {
    { gba_ioviwer_display_elements, NULL, NULL },
    { gba_ioviwer_backgrounds_elements, NULL, NULL },
    { gba_ioviwer_dma_elements, NULL, NULL },
    { gba_ioviwer_timers_elements, NULL, NULL },
    { gba_ioviwer_sound_elements, NULL, NULL },
    { gba_ioviwer_other_elements, NULL, NULL }
};

//--------------------------------------------------------------------------------------

void Win_GBAIOViewerUpdate(void)
{
    if(GBAIOViewerCreated == 0) return;

    if(Win_MainRunningGBA() == 0) return;

    #define CHECK(a) ((a)?CHR_SQUAREBLACK_MID:' ')

    switch(gba_ioview_selected_tab)
    {
        case 0: // Display
        {
            // LCD Control
            GUI_ConsoleClear(&gba_ioview_display_lcdcontrol_con);

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,0, "%04X : 000h DISPCNT",REG_DISPCNT);

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,2, "[%d] Video Mode",REG_DISPCNT&7);
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,3, "[%c] CGB Mode (Bios)",CHECK(REG_DISPCNT&BIT(3)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,4, "[%c] Display Frame Select",CHECK(REG_DISPCNT&BIT(4)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,5, "[%c] HBL Interval Free",CHECK(REG_DISPCNT&BIT(5)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,6, "[%c] 1D Character Mapping",CHECK(REG_DISPCNT&BIT(6)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,7, "[%c] Forced Blank",CHECK(REG_DISPCNT&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,8, "[%c] Display BG0",CHECK(REG_DISPCNT&BIT(8)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,9, "[%c] Display BG1",CHECK(REG_DISPCNT&BIT(9)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,10, "[%c] Display BG2",CHECK(REG_DISPCNT&BIT(10)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,11, "[%c] Display BG3",CHECK(REG_DISPCNT&BIT(11)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,12, "[%c] Display OBJ",CHECK(REG_DISPCNT&BIT(12)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,13, "[%c] Display Window 0",CHECK(REG_DISPCNT&BIT(13)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,14, "[%c] Display Window 1",CHECK(REG_DISPCNT&BIT(14)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,15, "[%c] Display OBJ Window",CHECK(REG_DISPCNT&BIT(15)));

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdcontrol_con,0,17, "%04X : 002h GREENSWP",REG_GREENSWAP);

            // LCD Status
            GUI_ConsoleClear(&gba_ioview_display_lcdstatus_con);

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,0, "%04X : 004h DISPSTAT",REG_DISPSTAT);

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,2, "[%c] V-Blank flag",CHECK(REG_DISPSTAT&BIT(0)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,3, "[%c] H-Blank flag",CHECK(REG_DISPSTAT&BIT(1)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,4, "[%c] V-Count flag",CHECK(REG_DISPSTAT&BIT(2)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,5, "[%c] V-Blank IRQ Enable",CHECK(REG_DISPSTAT&BIT(3)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,6, "[%c] H-Blank IRQ Enable",CHECK(REG_DISPSTAT&BIT(4)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,7, "[%c] V-Count IRQ Enable",CHECK(REG_DISPSTAT&BIT(5)));
            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,8, "[%3d] LYC",(REG_DISPSTAT>>8)&0xFF);

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,10, "%04X : 006h VCOUNT",REG_VCOUNT);

            GUI_ConsoleModePrintf(&gba_ioview_display_lcdstatus_con,0,12, "[%3d] LY",REG_VCOUNT);

            // Windows

            GUI_ConsoleClear(&gba_ioview_display_windows_con);

            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,0, "%04X/%04X : 040/044h WIN0[H/V]",REG_WIN0H,REG_WIN0V);
            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,1, "%04X/%04X : 042/046h WIN1[H/V]",REG_WIN1H,REG_WIN1V);

            u32 x1,x2,y1,y2;
            x1 = (REG_WIN0H>>8)&0xFF; x2 = REG_WIN0H&0xFF;
            if(x2 > 240) x2 = 240; if(x1 > x2) x2 = 240; //real bounds
            y1 = (REG_WIN0V>>8)&0xFF; y2 = REG_WIN0V&0xFF;
            if(y2 > 160) y2 = 160; if(y1 > y2) y2 = 160;
            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,3, "[%3d,%3d][%3d,%3d] Window 0",x1,y1,x2,y2);

            x1 = (REG_WIN1H>>8)&0xFF; x2 = REG_WIN1H&0xFF;
            if(x2 > 240) x2 = 240; if(x1 > x2) x2 = 240;
            y1 = (REG_WIN1V>>8)&0xFF; y2 = REG_WIN1V&0xFF;
            if(y2 > 160) y2 = 160; if(y1 > y2) y2 = 160;
            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,4, "[%3d,%3d][%3d,%3d] Window 1",x1,y1,x2,y2);

            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,6, "%04X/%04X : 048/04Ah WIN[IN/OUT]",REG_WININ,REG_WINOUT);

            u32 in0 = REG_WININ&0xFF; u32 in1 = (REG_WININ>>8)&0xFF;
            u32 out = REG_WINOUT&0xFF; u32 obj = (REG_WINOUT>>8)&0xFF;

            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,8,  "    Bg0 Bg1 Bg2 Bg3 Obj Spe");
            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,9,  "IN0 [%c] [%c] [%c] [%c] [%c] [%c]",
                                  CHECK(in0&BIT(0)), CHECK(in0&BIT(1)), CHECK(in0&BIT(2)),
                                  CHECK(in0&BIT(3)), CHECK(in0&BIT(4)), CHECK(in0&BIT(5)));
            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,10, "IN1 [%c] [%c] [%c] [%c] [%c] [%c]",
                                  CHECK(in1&BIT(0)), CHECK(in1&BIT(1)), CHECK(in1&BIT(2)),
                                  CHECK(in1&BIT(3)), CHECK(in1&BIT(4)), CHECK(in1&BIT(5)));
            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,11, "OUT [%c] [%c] [%c] [%c] [%c] [%c]",
                                  CHECK(out&BIT(0)), CHECK(out&BIT(1)), CHECK(out&BIT(2)),
                                  CHECK(out&BIT(3)), CHECK(out&BIT(4)), CHECK(out&BIT(5)));
            GUI_ConsoleModePrintf(&gba_ioview_display_windows_con,0,12, "OBJ [%c] [%c] [%c] [%c] [%c] [%c]",
                                  CHECK(obj&BIT(0)), CHECK(obj&BIT(1)), CHECK(obj&BIT(2)),
                                  CHECK(obj&BIT(3)), CHECK(obj&BIT(4)), CHECK(obj&BIT(5)));

            // Special Effects

            GUI_ConsoleClear(&gba_ioview_display_special_con);

            u32 mos = REG_MOSAIC;
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,0,0, "%04X : 04Ch MOSAIC",mos);
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,0,2, "[%2d,%2d] Spr  [%2d,%2d] Bg",
                                  ((mos>>8)&0xF)+1,((mos>>12)&0xF)+1,(mos&0xF)+1,((mos>>4)&0xF)+1);

            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,0,4, "%04X : 050h BLDCNT",REG_BLDCNT);
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,0,5, "%04X : 052h BLDALPHA",REG_BLDALPHA);
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,0,6, "%04X : 054h BLDY",REG_BLDY);

            const char * bld_modes[4] = { "None ", "Alpha", "White", "Black" };
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,27,0, "[%s] Blending Mode", bld_modes[(REG_BLDCNT>>6)&3]);
            int eva = REG_BLDALPHA&0x1F; if(eva > 16) eva = 16;
            int evb = (REG_BLDALPHA>>8)&0x1F; if(evb > 16) evb = 16;
            int evy = REG_BLDY&0x1F; if(evy > 16) evy = 16;
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,27,2, "[%2d] EVA  [%2d] EVB  [%2d] EVY",eva,evb,evy);

            u32 first = REG_BLDCNT&0x3F;
            u32 second = (REG_BLDCNT>>8)&0x3F;
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,27,4,  "    Bg0 Bg1 Bg2 Bg3 Obj BD");
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,27,5,  "1st [%c] [%c] [%c] [%c] [%c] [%c]",
                                  CHECK(first&BIT(0)), CHECK(first&BIT(1)), CHECK(first&BIT(2)),
                                  CHECK(first&BIT(3)), CHECK(first&BIT(4)), CHECK(first&BIT(5)));
            GUI_ConsoleModePrintf(&gba_ioview_display_special_con,27,6, "2nd [%c] [%c] [%c] [%c] [%c] [%c]",
                                  CHECK(second&BIT(0)), CHECK(second&BIT(1)), CHECK(second&BIT(2)),
                                  CHECK(second&BIT(3)), CHECK(second&BIT(4)), CHECK(second&BIT(5)));

            break;
        }
        case 1: // Backgrounds
        {
            int scrmode = REG_DISPCNT&7;

            //BG Control

            GUI_ConsoleClear(&gba_ioview_bgs_bgcontrol_con);

            const char * size_bg01[8][4] = { //mode(isaffine), size
                {" 256x256 ", " 512x256 ", " 256x512 ", " 512x512 "},
                {" 256x256 ", " 512x256 ", " 256x512 ", " 512x512 "},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"}
            };

            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,0,0, "%04X : 008h BG0CNT",REG_BG0CNT);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,0,2, "[%d] Priority",REG_BG0CNT&3);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,0,3, "[%08X] CBB",0x06000000+(((REG_BG0CNT>>2)&3)*0x4000));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,0,4, "[%08X] SBB",0x06000000+(((REG_BG0CNT>>8)&31)*0x800));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,0,5, "[%s] Size",size_bg01[scrmode][(REG_BG0CNT>>14)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,0,6, "[%c] Mosaic",CHECK(REG_BG0CNT&BIT(6)));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,0,7, "[%c] 256 colors",CHECK(REG_BG0CNT&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,0,8, "[%c] Overflow Wrap",CHECK(REG_BG0CNT&BIT(13)));

            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,21,0, "%04X : 00Ah BG1CNT",REG_BG1CNT);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,21,2, "[%d] Priority",REG_BG1CNT&3);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,21,3, "[%08X] CBB",0x06000000+(((REG_BG1CNT>>2)&3)*0x4000));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,21,4, "[%08X] SBB",0x06000000+(((REG_BG1CNT>>8)&31)*0x800));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,21,5, "[%s] Size",size_bg01[scrmode][(REG_BG1CNT>>14)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,21,6, "[%c] Mosaic",CHECK(REG_BG1CNT&BIT(6)));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,21,7, "[%c] 256 colors",CHECK(REG_BG1CNT&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,21,8, "[%c] Overflow Wrap",CHECK(REG_BG1CNT&BIT(13)));

            const char * size_bg2[8][4] = { //mode(isaffine), size
                {" 256x256 ", " 512x256 ", " 256x512 ", " 512x512 "},
                {" 128x128 ", " 256x256 ", " 512x512 ", "1024x1024"},
                {" 128x128 ", " 256x256 ", " 512x512 ", "1024x1024"},
                {" 240x160 ", " 240x160 ", " 240x160 ", " 240x160 "},
                {" 240x160 ", " 240x160 ", " 240x160 ", " 240x160 "},
                {" 160x128 ", " 160x128 ", " 160x128 ", " 160x128 "},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"}
            };

            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,42,0, "%04X : 00Ch BG2CNT",REG_BG2CNT);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,42,2, "[%d] Priority",REG_BG2CNT&3);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,42,3, "[%08X] CBB",0x06000000+(((REG_BG2CNT>>2)&3)*0x4000));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,42,4, "[%08X] SBB",0x06000000+(((REG_BG2CNT>>8)&31)*0x800));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,42,5, "[%s] Size",size_bg2[scrmode][(REG_BG2CNT>>14)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,42,6, "[%c] Mosaic",CHECK(REG_BG2CNT&BIT(6)));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,42,7, "[%c] 256 colors",CHECK(REG_BG2CNT&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,42,8, "[%c] Overflow Wrap",CHECK(REG_BG2CNT&BIT(13)));

            const char * size_bg3[8][4] = { //mode(isaffine), size
                {" 256x256 ", " 512x256 ", " 256x512 ", " 512x512 "},
                {"---------", "---------", "---------", "---------"},
                {" 128x128 ", " 256x256 ", " 512x512 ", "1024x1024"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"},
                {"---------", "---------", "---------", "---------"}
            };

            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,63,0, "%04X : 00Eh BG3CNT",REG_BG3CNT);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,63,2, "[%d] Priority",REG_BG3CNT&3);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,63,3, "[%08X] CBB",0x06000000+(((REG_BG3CNT>>2)&3)*0x4000));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,63,4, "[%08X] SBB",0x06000000+(((REG_BG3CNT>>8)&31)*0x800));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,63,5, "[%s] Size",size_bg3[scrmode][(REG_BG3CNT>>14)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,63,6, "[%c] Mosaic",CHECK(REG_BG3CNT&BIT(6)));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,63,7, "[%c] 256 colors",CHECK(REG_BG3CNT&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_bgs_bgcontrol_con,63,8, "[%c] Overflow Wrap",CHECK(REG_BG3CNT&BIT(13)));


            // BG Text Mode Scroll

            GUI_ConsoleClear(&gba_ioview_bgs_bgscroll_con);

            static const int bgistext[8][4] = { //mode, bgnumber
                {1,1,1,1},{1,1,0,0},{0,0,0,0},{0,0,0,0},
                {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}
            };

            if(bgistext[scrmode][0])
                GUI_ConsoleModePrintf(&gba_ioview_bgs_bgscroll_con,0,0, "[%3d,%3d] : 010/012h BG0[H/V]OFS",
                                      REG_BG0HOFS,REG_BG0VOFS);
            else
                GUI_ConsoleModePrintf(&gba_ioview_bgs_bgscroll_con,0,0, "[---,---] : 010/012h BG0[H/V]OFS");

            if(bgistext[scrmode][1])
                GUI_ConsoleModePrintf(&gba_ioview_bgs_bgscroll_con,0,2, "[%3d,%3d] : 014/016h BG1[H/V]OFS",
                                      REG_BG1HOFS,REG_BG1VOFS);
            else
                GUI_ConsoleModePrintf(&gba_ioview_bgs_bgscroll_con,0,2, "[---,---] : 014/016h BG1[H/V]OFS");

            if(bgistext[scrmode][2])
                GUI_ConsoleModePrintf(&gba_ioview_bgs_bgscroll_con,0,4, "[%3d,%3d] : 018/01Ah BG2[H/V]OFS",
                                      REG_BG2HOFS,REG_BG2VOFS);
            else
                GUI_ConsoleModePrintf(&gba_ioview_bgs_bgscroll_con,0,4, "[---,---] : 018/01Ah BG2[H/V]OFS");

            if(bgistext[scrmode][3])
                GUI_ConsoleModePrintf(&gba_ioview_bgs_bgscroll_con,0,6, "[%3d,%3d] : 01C/01Eh BG3[H/V]OFS",
                                      REG_BG3HOFS,REG_BG3VOFS);
            else
                GUI_ConsoleModePrintf(&gba_ioview_bgs_bgscroll_con,0,6, "[---,---] : 01C/01Eh BG3[H/V]OFS");

            // BG Affine Transformation

            GUI_ConsoleClear(&gba_ioview_bgs_affine_con);

            static const int bgisaffine[8][2] = { //mode, bgnumber (2,3)
                {0,0},{1,0},{1,1},{1,0},{1,0},{1,0},{0,0},{0,0}
            };

            GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,0,  " 028/02Ch BG2[X/Y]");
            GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,0, " 038/03Ch BG3[X/Y]");

            GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,5,  "020-026h BG2[PA-PD]");
            GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,5, "030-036h BG3[PA-PD]");

            if(bgisaffine[scrmode][0])
            {
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,2,  "[%08X,%08X]",REG_BG2X,REG_BG2Y);

                int bg2x = REG_BG2X; if(bg2x&BIT(27)) bg2x |= 0xF0000000;
                int bg2y = REG_BG2Y; if(bg2y&BIT(27)) bg2y |= 0xF0000000;
                char text_x[9]; snprintf(text_x,sizeof(text_x),"%.8f",((float)bg2x)/(1<<8));
                char text_y[9]; snprintf(text_y,sizeof(text_y),"%.8f",((float)bg2y)/(1<<8));
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,3, "(%s,%s)",text_x,text_y);

                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,7, "    [%04X,%04X]",REG_BG2PA,REG_BG2PB);
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,8, "    [%04X,%04X]",REG_BG2PC,REG_BG2PD);

                int pa = (s32)(s16)REG_BG2PA; int pb = (s32)(s16)REG_BG2PB;
                int pc = (s32)(s16)REG_BG2PC; int pd = (s32)(s16)REG_BG2PD;
                char text_a[7]; snprintf(text_a,sizeof(text_a),"%.8f",((float)pa)/(1<<8));
                char text_b[7]; snprintf(text_b,sizeof(text_b),"%.8f",((float)pb)/(1<<8));
                char text_c[7]; snprintf(text_c,sizeof(text_c),"%.8f",((float)pc)/(1<<8));
                char text_d[7]; snprintf(text_d,sizeof(text_d),"%.8f",((float)pd)/(1<<8));
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,9,  "  (%s,%s)",text_a,text_b);
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,10, "  (%s,%s)",text_c,text_d);
            }
            else
            {
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,2, "[--------,--------]");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,3, "(--------,--------)");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,7, "    [----,----]");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,8, "    [----,----]");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,9, "  (------,------)");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,2,10,"  (------,------)");
            }

            if(bgisaffine[scrmode][1])
            {
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,2, "[%08X,%08X]",REG_BG3X,REG_BG3Y);

                int bg3x = REG_BG3X; if(bg3x&BIT(27)) bg3x |= 0xF0000000;
                int bg3y = REG_BG3Y; if(bg3y&BIT(27)) bg3y |= 0xF0000000;
                char text_x[9]; snprintf(text_x,sizeof(text_x),"%.8f",((float)bg3x)/(1<<8));
                char text_y[9]; snprintf(text_y,sizeof(text_y),"%.8f",((float)bg3y)/(1<<8));
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,3, "(%s,%s)",text_x,text_y);

                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,7, "    [%04X,%04X]",REG_BG3PA,REG_BG3PB);
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,8, "    [%04X,%04X]",REG_BG3PC,REG_BG3PD);

                int pa = (s32)(s16)REG_BG3PA; int pb = (s32)(s16)REG_BG3PB;
                int pc = (s32)(s16)REG_BG3PC; int pd = (s32)(s16)REG_BG3PD;
                char text_a[7]; snprintf(text_a,sizeof(text_a),"%.8f",((float)pa)/(1<<8));
                char text_b[7]; snprintf(text_b,sizeof(text_b),"%.8f",((float)pb)/(1<<8));
                char text_c[7]; snprintf(text_c,sizeof(text_c),"%.8f",((float)pc)/(1<<8));
                char text_d[7]; snprintf(text_d,sizeof(text_d),"%.8f",((float)pd)/(1<<8));
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,9,  "  (%s,%s)",text_a,text_b);
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,10, "  (%s,%s)",text_c,text_d);
            }
            else
            {
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,2, "[--------,--------]");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,3, "(--------,--------)");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,7, "    [----,----]");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,8, "    [----,----]");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,9, "  (------,------)");
                GUI_ConsoleModePrintf(&gba_ioview_bgs_affine_con,26,10,"  (------,------)");
            }

            // Video Mode

            GUI_ConsoleClear(&gba_ioview_bgs_vidmode_con);

            static const char * videomodetext[8] = {
                "Mode: 0 (4 text)",
                "Mode: 1 (2 text, 1 affine)",
                "Mode: 2 (2 affine)",
                "Mode: 3 (1x bitmap 16bit)",
                "Mode: 4 (2x bitmap 8bit)",
                "Mode: 5 (2x bitmap 16bit)",
                "Mode: 6 (INVALID)",
                "Mode: 7 (INVALID)"
            };

            GUI_ConsoleModePrintf(&gba_ioview_bgs_vidmode_con,0,0,videomodetext[scrmode]);

            break;
        }
        case 2: // DMA
        {
            const char * srcincmode[4] = { "Increment ","Decrement ","Fixed     ","Prohibited" };
            const char * dstincmode[4] = { "Increment ","Decrement ","Fixed     ","Inc/Reload" };
            const char * startmode[4][4] = {
                {"Start immediately", "Start immediately", "Start immediately", "Start immediately"},
                {"  Start at VBlank", "  Start at VBlank", "  Start at VBlank", "  Start at VBlank"},
                {"  Start at HBlank", "  Start at HBlank", "  Start at HBlank", "  Start at HBlank"},
                {"       Prohibited", "       Sound FIFO", "       Sound FIFO", "    Video Capture"}
            };

            // DMA 0

            GUI_ConsoleClear(&gba_ioview_dma_dma0_con);

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,0,0, "%08X : 0B0h DMA0SAD",REG_DMA0SAD);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,0,1, "%08X : 0B4h DMA0DAD",REG_DMA0DAD);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,0,2, "%08X : 0B8h DMA0CNT",REG_DMA0CNT);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,26,0, "[%s]",srcincmode[(REG_DMA0CNT_H>>7)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,26,1, "[%s]",dstincmode[(REG_DMA0CNT_H>>5)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,26,2, "[%c] 32 bit",CHECK(REG_DMA0CNT_H&BIT(10)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,0,4, "[%5d] bytes",REG_DMA0CNT_L*((REG_DMA0CNT_H&BIT(10))?4:2));
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,26,4, "[%c] Repeat",CHECK(REG_DMA0CNT_H&BIT(9)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,0,6, "[%s] DMA Start Timing",startmode[(REG_DMA0CNT_H>>12)&3][0]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,26,8, "[%c] IRQ enable",CHECK(REG_DMA0CNT_H&BIT(14)));
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma0_con,0,8, "[%c] DMA enabled",CHECK(REG_DMA0CNT_H&BIT(15)));

            // DMA 1

            GUI_ConsoleClear(&gba_ioview_dma_dma1_con);

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,0,0, "%08X : 0BCh DMA1SAD",REG_DMA1SAD);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,0,1, "%08X : 0C0h DMA1DAD",REG_DMA1DAD);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,0,2, "%08X : 0C4h DMA1CNT",REG_DMA1CNT);

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,26,0, "[%s]",srcincmode[(REG_DMA1CNT_H>>7)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,26,1, "[%s]",dstincmode[(REG_DMA1CNT_H>>5)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,26,2, "[%c] 32 bit",CHECK(REG_DMA1CNT_H&BIT(10)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,0,4, "[%5d] bytes",REG_DMA1CNT_L*((REG_DMA1CNT_H&BIT(10))?4:2));
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,26,4, "[%c] Repeat",CHECK(REG_DMA1CNT_H&BIT(9)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,0,6, "[%s] DMA Start Timing",startmode[(REG_DMA1CNT_H>>12)&3][1]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,26,8, "[%c] IRQ enable",CHECK(REG_DMA1CNT_H&BIT(14)));
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma1_con,0,8, "[%c] DMA enabled",CHECK(REG_DMA1CNT_H&BIT(15)));

            // DMA 2

            GUI_ConsoleClear(&gba_ioview_dma_dma2_con);

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,0,0, "%08X : 0C8h DMA2SAD",REG_DMA2SAD);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,0,1, "%08X : 0CCh DMA2DAD",REG_DMA2DAD);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,0,2, "%08X : 0D0h DMA2CNT",REG_DMA2CNT);

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,26,0, "[%s]",srcincmode[(REG_DMA2CNT_H>>7)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,26,1, "[%s]",dstincmode[(REG_DMA2CNT_H>>5)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,26,2, "[%c] 32 bit",CHECK(REG_DMA2CNT_H&BIT(10)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,0,4, "[%5d] bytes",REG_DMA2CNT_L*((REG_DMA2CNT_H&BIT(10))?4:2));
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,26,4, "[%c] Repeat",CHECK(REG_DMA2CNT_H&BIT(9)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,0,6, "[%s] DMA Start Timing",startmode[(REG_DMA2CNT_H>>12)&3][2]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,26,8, "[%c] IRQ enable",CHECK(REG_DMA2CNT_H&BIT(14)));
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma2_con,0,8, "[%c] DMA enabled",CHECK(REG_DMA2CNT_H&BIT(15)));

            // DMA 3

            GUI_ConsoleClear(&gba_ioview_dma_dma3_con);

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,0,0, "%08X : 0D4h DMA3SAD",REG_DMA3SAD);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,0,1, "%08X : 0D8h DMA3DAD",REG_DMA3DAD);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,0,2, "%08X : 0DCh DMA3CNT",REG_DMA3CNT);

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,26,0, "[%s]",srcincmode[(REG_DMA3CNT_H>>7)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,26,1, "[%s]",dstincmode[(REG_DMA3CNT_H>>5)&3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,26,2, "[%c] 32 bit",CHECK(REG_DMA3CNT_H&BIT(10)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,0,4, "[%5d] bytes",REG_DMA3CNT_L*((REG_DMA3CNT_H&BIT(10))?4:2));
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,26,4, "[%c] Repeat",CHECK(REG_DMA3CNT_H&BIT(9)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,0,6, "[%s] DMA Start Timing",startmode[(REG_DMA3CNT_H>>12)&3][3]);
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,26,8, "[%c] IRQ enable",CHECK(REG_DMA3CNT_H&BIT(14)));
            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,0,8, "[%c] DMA enabled",CHECK(REG_DMA3CNT_H&BIT(15)));

            GUI_ConsoleModePrintf(&gba_ioview_dma_dma3_con,0,9, "[%c] Game Pak DRQ",CHECK(REG_DMA3CNT_H&BIT(11)));

            break;
        }
        case 3: // Timers
        {
            const char * tmrfreq[4] = { "16.78MHz", "262.2KHz", "65.54KHz", "16.38KHz" };
            const char * clkspertick[4] = { "   1", "  64", " 256", "1024" };

            // Timer 0

            GUI_ConsoleClear(&gba_ioview_timers_tmr0_con);

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,0,0, "%04X : 100h TM0CNT_L",REG_TM0CNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,0,1, "%04X : 102h TM0CNT_H",REG_TM0CNT_H);

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,23,0, "[%04X] On reload",gba_timergetstart0());
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,23,3, "[%c] Cascade",CHECK(REG_TM0CNT_H&BIT(2)));

            if(REG_TM0CNT_H&BIT(2))
            {
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,0,3, "[Invalid!] Frequency");
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,0,5, "[----] CPU clocks per tick");
            }
            else
            {
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,0,3, "[%s] Frequency",tmrfreq[REG_TM0CNT_H&3]);
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,0,5, "[%s] CPU clocks per tick",clkspertick[REG_TM0CNT_H&3]);
            }

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,0,7, "[%c] Timer Enabled",CHECK(REG_TM0CNT_H&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr0_con,23,7, "[%c] IRQ Enabled",CHECK(REG_TM0CNT_H&BIT(6)));

            // Timer 1

            GUI_ConsoleClear(&gba_ioview_timers_tmr1_con);

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,0,0, "%04X : 104h TM1CNT_L",REG_TM1CNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,0,1, "%04X : 106h TM1CNT_H",REG_TM1CNT_H);

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,23,0, "[%04X] On reload",gba_timergetstart1());
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,23,3, "[%c] Cascade",CHECK(REG_TM1CNT_H&BIT(2)));

            if(REG_TM1CNT_H&BIT(2))
            {
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,0,3, "[--------] Frequency");
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,0,5, "[----] CPU clocks per tick");
            }
            else
            {
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,0,3, "[%s] Frequency",tmrfreq[REG_TM1CNT_H&3]);
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,0,5, "[%s] CPU clocks per tick",clkspertick[REG_TM1CNT_H&3]);
            }

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,0,7, "[%c] Timer Enabled",CHECK(REG_TM1CNT_H&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr1_con,23,7, "[%c] IRQ Enabled",CHECK(REG_TM1CNT_H&BIT(6)));

            // Timer 2

            GUI_ConsoleClear(&gba_ioview_timers_tmr2_con);

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,0,0, "%04X : 108h TM2CNT_L",REG_TM2CNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,0,1, "%04X : 10Ah TM2CNT_H",REG_TM2CNT_H);

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,23,0, "[%04X] On reload",gba_timergetstart2());
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,23,3, "[%c] Cascade",CHECK(REG_TM2CNT_H&BIT(2)));

            if(REG_TM2CNT_H&BIT(2))
            {
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,0,3, "[--------] Frequency");
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,0,5, "[----] CPU clocks per tick");
            }
            else
            {
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,0,3, "[%s] Frequency",tmrfreq[REG_TM2CNT_H&3]);
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,0,5, "[%s] CPU clocks per tick",clkspertick[REG_TM2CNT_H&3]);
            }

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,0,7, "[%c] Timer Enabled",CHECK(REG_TM2CNT_H&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr2_con,23,7, "[%c] IRQ Enabled",CHECK(REG_TM2CNT_H&BIT(6)));

            // Timer 3

            GUI_ConsoleClear(&gba_ioview_timers_tmr3_con);

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,0,0, "%04X : 10Ch TM3CNT_L",REG_TM3CNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,0,1, "%04X : 10Eh TM3CNT_H",REG_TM3CNT_H);

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,23,0, "[%04X] On reload",gba_timergetstart3());
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,23,3, "[%c] Cascade",CHECK(REG_TM3CNT_H&BIT(2)));

            if(REG_TM3CNT_H&BIT(2))
            {
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,0,3, "[--------] Frequency");
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,0,5, "[----] CPU clocks per tick");
            }
            else
            {
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,0,3, "[%s] Frequency",tmrfreq[REG_TM3CNT_H&3]);
                GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,0,5, "[%s] CPU clocks per tick",clkspertick[REG_TM3CNT_H&3]);
            }

            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,0,7, "[%c] Timer Enabled",CHECK(REG_TM3CNT_H&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_timers_tmr3_con,23,7, "[%c] IRQ Enabled",CHECK(REG_TM3CNT_H&BIT(6)));

            break;
        }
        case 4: // Sound
        {
            // Channel 1

            GUI_ConsoleClear(&gba_ioview_sound_chn1_con);

            GUI_ConsoleModePrintf(&gba_ioview_sound_chn1_con,0,0, "%04X : 060h SOUND1CNT_L",REG_SOUND1CNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn1_con,0,1, "%04X : 062h SOUND1CNT_H",REG_SOUND1CNT_H);
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn1_con,0,2, "%04X : 064h SOUND1CNT_X",REG_SOUND1CNT_X);

            GUI_ConsoleModePrintf(&gba_ioview_sound_chn1_con,0,4, "[%2d] Volume",gba_debug_get_psg_vol(1));

            char text_freq[10];
            snprintf(text_freq,sizeof(text_freq),"%9.2f", 131072.0/(float)(2048-(REG_SOUND1CNT_X&0x7FF)));
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn1_con,0,5, "[%s] Frequency",text_freq);

            // Channel 2

            GUI_ConsoleClear(&gba_ioview_sound_chn2_con);

            GUI_ConsoleModePrintf(&gba_ioview_sound_chn2_con,0,0, "%04X : 068h SOUND2CNT_L",REG_SOUND2CNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn2_con,0,1, "%04X : 06Ch SOUND2CNT_H",REG_SOUND2CNT_H);

            GUI_ConsoleModePrintf(&gba_ioview_sound_chn2_con,0,4, "[%2d] Volume",gba_debug_get_psg_vol(2));

            snprintf(text_freq,sizeof(text_freq),"%9.2f", 131072.0/(float)(2048-(REG_SOUND2CNT_H&0x7FF)));
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn2_con,0,5, "[%s] Frequency",text_freq);

            // Channel 3

            GUI_ConsoleClear(&gba_ioview_sound_chn3_con);

            GUI_ConsoleModePrintf(&gba_ioview_sound_chn3_con,0,0, "%04X : 070h SOUND3CNT_L",REG_SOUND3CNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn3_con,0,1, "%04X : 072h SOUND3CNT_H",REG_SOUND3CNT_H);
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn3_con,0,2, "%04X : 074h SOUND3CNT_X",REG_SOUND3CNT_X);

            GUI_ConsoleModePrintf(&gba_ioview_sound_chn3_con,0,4, "[%2d] Volume",gba_debug_get_psg_vol(3));

            snprintf(text_freq,sizeof(text_freq),"%9.2f", 2097152.0/(float)(2048-(REG_SOUND3CNT_X&0x7FF)));
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn3_con,0,5, "[%s] Frequency",text_freq);

            // Channel 4

            GUI_ConsoleClear(&gba_ioview_sound_chn4_con);

            GUI_ConsoleModePrintf(&gba_ioview_sound_chn4_con,0,0, "%04X : 078h SOUND4CNT_L",REG_SOUND4CNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn4_con,0,1, "%04X : 07Ch SOUND4CNT_H",REG_SOUND4CNT_H);

            GUI_ConsoleModePrintf(&gba_ioview_sound_chn4_con,0,4, "[%2d] Volume",gba_debug_get_psg_vol(4));

            const s32 NoiseFreqRatio[8] = { 1048576,524288,262144,174763,131072,104858,87381,74898 };
            int freq = NoiseFreqRatio[REG_SOUND4CNT_H&7] >> (((REG_SOUND4CNT_H>>4)&3) + 1);
            GUI_ConsoleModePrintf(&gba_ioview_sound_chn4_con,0,5, "[%9d] Frequency",freq);

            // DMA Channels/Control

            GUI_ConsoleClear(&gba_ioview_sound_control_con);

            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,0, "%04X : 080h SOUNDCNT_L",REG_SOUNDCNT_L);
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,1, "%04X : 082h SOUNDCNT_H",REG_SOUNDCNT_H);
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,2, "%04X : 084h SOUNDCNT_X",REG_SOUNDCNT_X);

            u32 sndl = REG_SOUNDCNT_L;
            u32 sndh = REG_SOUNDCNT_H;

            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,4,  "   Ch1 Ch2 Ch3 Ch4 ChA ChB");
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,5,  " R [%c] [%c] [%c] [%c] [%c] [%c]",
                                  CHECK(sndl&BIT(8)), CHECK(sndl&BIT(9)), CHECK(sndl&BIT(10)),
                                  CHECK(sndl&BIT(11)), CHECK(sndh&BIT(8)), CHECK(sndh&BIT(12)));
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,6, " L [%c] [%c] [%c] [%c] [%c] [%c]",
                                  CHECK(sndl&BIT(12)), CHECK(sndl&BIT(13)), CHECK(sndl&BIT(14)),
                                  CHECK(sndl&BIT(15)), CHECK(sndh&BIT(9)), CHECK(sndh&BIT(13)));

            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,8, "[%c] Sound Master Enable",CHECK(REG_SOUNDCNT_X&BIT(7)));

            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,10, "PSG Volume");
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,11, "[%d] Right [%d] Left",sndl&7,(sndl>>4)&7);

            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,13, "[%d] Timer FIFO A", (sndh&BIT(10)) != 0);
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,14, "[%d] Timer FIFO B", (sndh&BIT(14)) != 0);

            const char * psgvol[4] = {" 25"," 50","100","---"};
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,16, "Mixer Volume %");
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,17, "[%s%%] PSG [%3d%%] A [%3d%%] B",
                                  psgvol[sndh&3],sndh&BIT(2)?100:50,sndh&BIT(3)?100:50);

            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,19, "%08X : 0A0h FIFO_A",REG_FIFO_A);
            GUI_ConsoleModePrintf(&gba_ioview_sound_control_con,0,20, "%08X : 0A4h FIFO_B",REG_FIFO_B);

            // Wave RAM

            GUI_ConsoleClear(&gba_ioview_sound_waveram_con);

            int en_0 = 0, en_1 = 0;

            if(REG_SOUND3CNT_L&BIT(5)) { en_0 = 1; en_1 = 1; }
            else
            {
                if(REG_SOUND3CNT_L&BIT(5)) { en_0 = 0; en_1 = 1; }
                else { en_0 = 1; en_1 = 0; }
            }

            u16 * wav = (u16*)REG_WAVE_RAM;
            GUI_ConsoleModePrintf(&gba_ioview_sound_waveram_con,0,0, "[%c] %04X|%04X|%04X|%04X",
                                  CHECK(en_0),wav[0],wav[1],wav[2],wav[3]);
            GUI_ConsoleModePrintf(&gba_ioview_sound_waveram_con,0,1, "[%c] %04X|%04X|%04X|%04X",
                                  CHECK(en_1),wav[4],wav[5],wav[6],wav[7]);

            // PWM Control

            GUI_ConsoleClear(&gba_ioview_sound_pwm_con);

            GUI_ConsoleModePrintf(&gba_ioview_sound_pwm_con,0,0, "%04X : 088h SOUNDBIAS",REG_SOUNDBIAS);

            const char * biasinfo[4] = { "9bit |  32.768kHz", "8bit |  65.536kHz","7bit | 131.072kHz",
                "6bit | 262.144kHz" };
            GUI_ConsoleModePrintf(&gba_ioview_sound_pwm_con,0,1, "[%s]",biasinfo[(REG_SOUNDBIAS>>14)&3]);

            break;
        }
        case 5: // Other
        {
            GUI_ConsoleClear(&gba_ioview_others_interrupts_con);

            GUI_ConsoleModePrintf(&gba_ioview_others_interrupts_con,0,0, "%04X : 208h IME",REG_IME);
            GUI_ConsoleModePrintf(&gba_ioview_others_interrupts_con,0,2, "%04X : 200h IE",REG_IE);
            GUI_ConsoleModePrintf(&gba_ioview_others_interrupts_con,0,3, "%04X : 202h IF",REG_IF);

            GUI_ConsoleClear(&gba_ioview_others_int_flags_con);

            u16 biosflags = GBA_MemoryReadFast16(0x03007FF8);

            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,0, "IE IF Bios");
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,1, "[%c][%c][%c] V-Blank",
                                  CHECK(REG_IE&BIT(0)),CHECK(REG_IF&BIT(0)),CHECK(biosflags&BIT(0)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,2, "[%c][%c][%c] H-Blank",
                                  CHECK(REG_IE&BIT(1)),CHECK(REG_IF&BIT(1)),CHECK(biosflags&BIT(1)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,3, "[%c][%c][%c] V-Count",
                                  CHECK(REG_IE&BIT(2)),CHECK(REG_IF&BIT(2)),CHECK(biosflags&BIT(2)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,4, "[%c][%c][%c] Timer 0",
                                  CHECK(REG_IE&BIT(3)),CHECK(REG_IF&BIT(3)),CHECK(biosflags&BIT(3)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,5, "[%c][%c][%c] Timer 1",
                                  CHECK(REG_IE&BIT(4)),CHECK(REG_IF&BIT(4)),CHECK(biosflags&BIT(4)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,6, "[%c][%c][%c] Timer 2",
                                  CHECK(REG_IE&BIT(5)),CHECK(REG_IF&BIT(5)),CHECK(biosflags&BIT(5)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,7, "[%c][%c][%c] Timer 3",
                                  CHECK(REG_IE&BIT(6)),CHECK(REG_IF&BIT(6)),CHECK(biosflags&BIT(6)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,8, "[%c][%c][%c] Serial",
                                  CHECK(REG_IE&BIT(7)),CHECK(REG_IF&BIT(7)),CHECK(biosflags&BIT(7)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,9, "[%c][%c][%c] DMA 0",
                                  CHECK(REG_IE&BIT(8)),CHECK(REG_IF&BIT(8)),CHECK(biosflags&BIT(8)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,10, "[%c][%c][%c] DMA 1",
                                  CHECK(REG_IE&BIT(9)),CHECK(REG_IF&BIT(9)),CHECK(biosflags&BIT(9)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,11, "[%c][%c][%c] DMA 2",
                                  CHECK(REG_IE&BIT(10)),CHECK(REG_IF&BIT(10)),CHECK(biosflags&BIT(10)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,12, "[%c][%c][%c] DMA 3",
                                  CHECK(REG_IE&BIT(11)),CHECK(REG_IF&BIT(11)),CHECK(biosflags&BIT(11)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,13, "[%c][%c][%c] Keypad",
                                  CHECK(REG_IE&BIT(12)),CHECK(REG_IF&BIT(12)),CHECK(biosflags&BIT(12)));
            GUI_ConsoleModePrintf(&gba_ioview_others_int_flags_con,0,14, "[%c][%c][%c] Game Pak",
                                  CHECK(REG_IE&BIT(13)),CHECK(REG_IF&BIT(13)),CHECK(biosflags&BIT(13)));

            break;
        }
        default:
        {
            break;
        }
    } // end switch
}
//----------------------------------------------------------------

static void _win_gba_ioviewer_tabnum_radbtn_callback(int num)
{
    gba_ioview_selected_tab = num;
}

//----------------------------------------------------------------

void Win_GBAIOViewerRender(void)
{
    if(GBAIOViewerCreated == 0) return;

    char buffer[WIN_GBA_IOVIEWER_WIDTH*WIN_GBA_IOVIEWER_HEIGHT*3];
    GUI_Draw(&(gba_ioviewer_page_gui[gba_ioview_selected_tab]),buffer,
             WIN_GBA_IOVIEWER_WIDTH,WIN_GBA_IOVIEWER_HEIGHT,1);

    WH_Render(WinIDGBAIOViewer, buffer);
}

static int _win_gba_io_viewer_callback(SDL_Event * e)
{
    if(GBAIOViewerCreated == 0) return 1;

    int redraw = GUI_SendEvent(&(gba_ioviewer_page_gui[gba_ioview_selected_tab]),e);

    int close_this = 0;

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
            close_this = 1;
    }

    if(close_this)
    {
        GBAIOViewerCreated = 0;
        WH_Close(WinIDGBAIOViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GBAIOViewerUpdate();
        Win_GBAIOViewerRender();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

int Win_GBAIOViewerCreate(void)
{
    if(Win_MainRunningGBA() == 0) return 0;

    if(GBAIOViewerCreated == 1)
    {
        WH_Focus(WinIDGBAIOViewer);
        return 0;
    }

    gba_ioview_selected_tab = 0;

    GUI_SetRadioButton(&gba_ioview_display_tabbtn,                    0,0, 14*FONT_WIDTH,18,
                  "Display", 0, 0, 1,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_backgrounds_tabbtn,  1+14*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "Backgrounds", 0, 1, 0,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_dma_tabbtn,          2+28*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "DMA", 0, 2, 0,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_timers_tabbtn,       3+42*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "Timers", 0, 3, 0,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_sound_tabbtn,        4+56*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "Sound", 0, 4, 0,_win_gba_ioviewer_tabnum_radbtn_callback);
    GUI_SetRadioButton(&gba_ioview_other_tabbtn,        5+70*FONT_WIDTH,0, 14*FONT_WIDTH,18,
                  "Other", 0, 5, 0,_win_gba_ioviewer_tabnum_radbtn_callback);

    //--------------------------------------------------------------------

    // Display

    GUI_SetLabel(&gba_ioview_display_lcdcontrol_label,
                   6,24, -1,FONT_HEIGHT, "LCD Control");
    GUI_SetTextBox(&gba_ioview_display_lcdcontrol_textbox,&gba_ioview_display_lcdcontrol_con,
                   6,42, 25*FONT_WIDTH,18*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_display_lcdstatus_label,
                   6+25*FONT_WIDTH+6,24, -1,FONT_HEIGHT, "LCD Status");
    GUI_SetTextBox(&gba_ioview_display_lcdstatus_textbox,&gba_ioview_display_lcdstatus_con,
                   6+25*FONT_WIDTH+6,42, 23*FONT_WIDTH,13*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_display_windows_label,
                   12+48*FONT_WIDTH+6,24, -1,FONT_HEIGHT, "Windows");
    GUI_SetTextBox(&gba_ioview_display_windows_textbox,&gba_ioview_display_windows_con,
                   12+48*FONT_WIDTH+6,42, 33*FONT_WIDTH,13*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_display_special_label,
                   6+25*FONT_WIDTH+6,42+13*FONT_HEIGHT+6, -1,FONT_HEIGHT, "Special Effects");
    GUI_SetTextBox(&gba_ioview_display_special_textbox,&gba_ioview_display_special_con,
                   6+25*FONT_WIDTH+6,42+13*FONT_HEIGHT+6+18, 55*FONT_WIDTH+13,7*FONT_HEIGHT, NULL);

    // Backgrounds

    GUI_SetLabel(&gba_ioview_bgs_bgcontrol_label,
                   6,24, -1,FONT_HEIGHT, "BG Control");
    GUI_SetTextBox(&gba_ioview_bgs_bgcontrol_textbox,&gba_ioview_bgs_bgcontrol_con,
                   6,42, 83*FONT_WIDTH,9*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_bgs_bgscroll_label,
                   6,42+9*FONT_HEIGHT+6, -1,FONT_HEIGHT, "BG Text Mode Scroll");
    GUI_SetTextBox(&gba_ioview_bgs_bgscroll_textbox,&gba_ioview_bgs_bgscroll_con,
                   6,42+9*FONT_HEIGHT+6+18, 33*FONT_WIDTH,8*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_bgs_affine_label,
                   6+33*FONT_WIDTH+6,42+9*FONT_HEIGHT+6, -1,FONT_HEIGHT, "BG Affine Transformation");
    GUI_SetTextBox(&gba_ioview_bgs_affine_textbox,&gba_ioview_bgs_affine_con,
                   6+33*FONT_WIDTH+6,42+9*FONT_HEIGHT+6+18, 49*FONT_WIDTH+1,11*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_bgs_vidmode_label,
                   6,42+17*FONT_HEIGHT+12+18, -1,FONT_HEIGHT, "Video Mode");
    GUI_SetTextBox(&gba_ioview_bgs_vidmode_textbox,&gba_ioview_bgs_vidmode_con,
                   6,42+17*FONT_HEIGHT+12+36, 33*FONT_WIDTH,FONT_HEIGHT, NULL);

    // DMA

    GUI_SetLabel(&gba_ioview_dma_dma0_label,
                   6,24, -1,FONT_HEIGHT, "DMA 0");
    GUI_SetTextBox(&gba_ioview_dma_dma0_textbox,&gba_ioview_dma_dma0_con,
                   6,42, 41*FONT_WIDTH,10*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_dma_dma1_label,
                   6+41*FONT_WIDTH+7,24, -1,FONT_HEIGHT, "DMA 1");
    GUI_SetTextBox(&gba_ioview_dma_dma1_textbox,&gba_ioview_dma_dma1_con,
                   6+41*FONT_WIDTH+7,42, 41*FONT_WIDTH,10*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_dma_dma2_label,
                   6,42+10*FONT_HEIGHT+6, -1,FONT_HEIGHT, "DMA 2");
    GUI_SetTextBox(&gba_ioview_dma_dma2_textbox,&gba_ioview_dma_dma2_con,
                   6,60+10*FONT_HEIGHT+6, 41*FONT_WIDTH,10*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_dma_dma3_label,
                   6+41*FONT_WIDTH+7,42+10*FONT_HEIGHT+6, -1,FONT_HEIGHT, "DMA 3");
    GUI_SetTextBox(&gba_ioview_dma_dma3_textbox,&gba_ioview_dma_dma3_con,
                   6+41*FONT_WIDTH+7,60+10*FONT_HEIGHT+6, 41*FONT_WIDTH,10*FONT_HEIGHT, NULL);

    // Timers

    GUI_SetLabel(&gba_ioview_timers_tmr0_label,
                   6,24, -1,FONT_HEIGHT, "Timer 0");
    GUI_SetTextBox(&gba_ioview_timers_tmr0_textbox,&gba_ioview_timers_tmr0_con,
                   6,42, 41*FONT_WIDTH,10*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_timers_tmr1_label,
                   6+41*FONT_WIDTH+7,24, -1,FONT_HEIGHT, "Timer 1");
    GUI_SetTextBox(&gba_ioview_timers_tmr1_textbox,&gba_ioview_timers_tmr1_con,
                   6+41*FONT_WIDTH+7,42, 41*FONT_WIDTH,10*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_timers_tmr2_label,
                   6,42+10*FONT_HEIGHT+6, -1,FONT_HEIGHT, "Timer 2");
    GUI_SetTextBox(&gba_ioview_timers_tmr2_textbox,&gba_ioview_timers_tmr2_con,
                   6,60+10*FONT_HEIGHT+6, 41*FONT_WIDTH,10*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_timers_tmr3_label,
                   6+41*FONT_WIDTH+7,42+10*FONT_HEIGHT+6, -1,FONT_HEIGHT, "Timer 3");
    GUI_SetTextBox(&gba_ioview_timers_tmr3_textbox,&gba_ioview_timers_tmr3_con,
                   6+41*FONT_WIDTH+7,60+10*FONT_HEIGHT+6, 41*FONT_WIDTH,10*FONT_HEIGHT, NULL);

    // Sound

    GUI_SetLabel(&gba_ioview_sound_chn1_label,
                   6,24, -1,FONT_HEIGHT, "Channel 1");
    GUI_SetTextBox(&gba_ioview_sound_chn1_textbox,&gba_ioview_sound_chn1_con,
                   6,42, 25*FONT_WIDTH,8*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_sound_chn2_label,
                   6,42+8*FONT_HEIGHT+6, -1,FONT_HEIGHT, "Channel 2");
    GUI_SetTextBox(&gba_ioview_sound_chn2_textbox,&gba_ioview_sound_chn2_con,
                   6,42+8*FONT_HEIGHT+6+18, 25*FONT_WIDTH,8*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_sound_chn3_label,
                   6+25*FONT_WIDTH+6,24, -1,FONT_HEIGHT, "Channel 3");
    GUI_SetTextBox(&gba_ioview_sound_chn3_textbox,&gba_ioview_sound_chn3_con,
                   6+25*FONT_WIDTH+6,42, 25*FONT_WIDTH,8*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_sound_chn4_label,
                   6+25*FONT_WIDTH+6,42+8*FONT_HEIGHT+6, -1,FONT_HEIGHT, "Channel 4");
    GUI_SetTextBox(&gba_ioview_sound_chn4_textbox,&gba_ioview_sound_chn4_con,
                   6+25*FONT_WIDTH+6,42+8*FONT_HEIGHT+6+18, 25*FONT_WIDTH,8*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_sound_control_label,
                   6+50*FONT_WIDTH+12,24, -1,FONT_HEIGHT, "DMA Channels/Control");
    GUI_SetTextBox(&gba_ioview_sound_control_textbox,&gba_ioview_sound_control_con,
                   6+50*FONT_WIDTH+12,42, 31*FONT_WIDTH,22*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_sound_waveram_label,
                   6,42+16*FONT_HEIGHT+12+18, -1,FONT_HEIGHT, "090h Wave RAM");
    GUI_SetTextBox(&gba_ioview_sound_waveram_textbox,&gba_ioview_sound_waveram_con,
                   6,42+16*FONT_HEIGHT+12+36, 25*FONT_WIDTH,2*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_sound_pwm_label,
                   6+25*FONT_WIDTH+6,42+16*FONT_HEIGHT+12+18, -1,FONT_HEIGHT, "PWM Control");
    GUI_SetTextBox(&gba_ioview_sound_pwm_textbox,&gba_ioview_sound_pwm_con,
                   6+25*FONT_WIDTH+6,42+16*FONT_HEIGHT+12+36, 25*FONT_WIDTH,2*FONT_HEIGHT, NULL);

    // Other

    GUI_SetLabel(&gba_ioview_others_interrupts_label,
                   6,24, -1,FONT_HEIGHT, "Interrupts");
    GUI_SetTextBox(&gba_ioview_others_interrupts_textbox,&gba_ioview_others_interrupts_con,
                   6,42, 19*FONT_WIDTH,5*FONT_HEIGHT, NULL);

    GUI_SetLabel(&gba_ioview_others_int_flags_label,
                   6,42+5*FONT_HEIGHT+6, -1,FONT_HEIGHT, "Flags");
    GUI_SetTextBox(&gba_ioview_others_int_flags_textbox,&gba_ioview_others_int_flags_con,
                   6,60+5*FONT_HEIGHT+6, 19*FONT_WIDTH,15*FONT_HEIGHT, NULL);

    //--------------------------------------------------------------------

    GBAIOViewerCreated = 1;

    WinIDGBAIOViewer = WH_Create(WIN_GBA_IOVIEWER_WIDTH,WIN_GBA_IOVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBAIOViewer,"GBA I/O Viewer");

    WH_SetEventCallback(WinIDGBAIOViewer,_win_gba_io_viewer_callback);

    Win_GBAIOViewerUpdate();
    Win_GBAIOViewerRender();

    return 1;
}

void Win_GBAIOViewerClose(void)
{
    if(GBAIOViewerCreated == 0)
        return;

    GBAIOViewerCreated = 0;
    WH_Close(WinIDGBAIOViewer);
}

//----------------------------------------------------------------
