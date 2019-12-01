// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include "../build_options.h"

#include "gba.h"
#include "interrupts.h"
#include "memory.h"
#include "sound.h"
#include "timers.h"

typedef struct {
    u16 start;

    s32 remainingclocks;
    s32 clockspertick;

    u16 cascade; // If != 0, Timer X is incremented when Timer X-1 overflows
    u16 irqenable;
    u16 enabled;
} _timer_t;

_timer_t Timer[4];

//----------------------------------------------------------------

static s32 min(s32 a, s32 b)
{
    return ((a < b) ? a : b);
}

//----------------------------------------------------------------

void GBA_TimerInitAll(void)
{
    memset((void *)Timer, 0, sizeof(Timer));
}

//----------------------------------------------------------------

void GBA_TimerSetStart0(u16 val)
{
    Timer[0].start = val;
}

void GBA_TimerSetStart1(u16 val)
{
    Timer[1].start = val;
}

void GBA_TimerSetStart2(u16 val)
{
    Timer[2].start = val;
}

void GBA_TimerSetStart3(u16 val)
{
    Timer[3].start = val;
}

//----------------------------------------------------------------

u16 gba_timergetstart0(void)
{
    return Timer[0].start;
}

u16 gba_timergetstart1(void)
{
    return Timer[1].start;
}

u16 gba_timergetstart2(void)
{
    return Timer[2].start;
}

u16 gba_timergetstart3(void)
{
    return Timer[3].start;
}

//----------------------------------------------------------------

static const s32 gba_timerclockspertic[4] = { 1, 64, 256, 1024 };

void GBA_TimerSetup0(void)
{
    Timer[0].cascade = REG_TM0CNT_H & BIT(2);
    //if (Timer[0].cascade) // Disable
    Timer[0].irqenable = REG_TM0CNT_H & BIT(6);
    Timer[0].enabled = REG_TM0CNT_H & BIT(7);

    Timer[0].clockspertick = gba_timerclockspertic[REG_TM0CNT_H & 3];

    if (Timer[0].enabled)
    {
        REG_TM0CNT_L = Timer[0].start;
        Timer[0].remainingclocks = Timer[0].clockspertick;
    }
}

void GBA_TimerSetup1(void)
{
    Timer[1].cascade = REG_TM1CNT_H & BIT(2);
    Timer[1].irqenable = REG_TM1CNT_H & BIT(6);
    Timer[1].enabled = REG_TM1CNT_H & BIT(7);

    Timer[1].clockspertick = gba_timerclockspertic[REG_TM1CNT_H & 3];

    if (Timer[1].enabled)
    {
        REG_TM1CNT_L = Timer[1].start;
        Timer[1].remainingclocks = Timer[1].clockspertick;
    }
}

void GBA_TimerSetup2(void)
{
    Timer[2].cascade = REG_TM2CNT_H & BIT(2);
    Timer[2].irqenable = REG_TM2CNT_H & BIT(6);
    Timer[2].enabled = REG_TM2CNT_H & BIT(7);

    Timer[2].clockspertick = gba_timerclockspertic[REG_TM2CNT_H & 3];

    if (Timer[2].enabled)
    {
        REG_TM2CNT_L = Timer[2].start;
        Timer[2].remainingclocks = Timer[2].clockspertick;
    }
}

void GBA_TimerSetup3(void)
{
    Timer[3].cascade = REG_TM3CNT_H & BIT(2);
    Timer[3].irqenable = REG_TM3CNT_H & BIT(6);
    Timer[3].enabled = REG_TM3CNT_H & BIT(7);

    Timer[3].clockspertick = gba_timerclockspertic[REG_TM3CNT_H & 3];

    if (Timer[3].enabled)
    {
        REG_TM3CNT_L = Timer[3].start;
        Timer[3].remainingclocks = Timer[3].clockspertick;
    }
}

//----------------------------------------------------------------

s32 GBA_TimersUpdate(s32 clocks)
{
    int timer0overflowed = 0;
    int timer1overflowed = 0;
    int timer2overflowed = 0;

    s32 returnclocks = 0x7FFFFFFF;

    if (Timer[0].enabled)
    {
        //if (Timer[0].cascade == 0)
        {
            Timer[0].remainingclocks -= clocks;
            while (Timer[0].remainingclocks <= 0)
            {
                Timer[0].remainingclocks += Timer[0].clockspertick;
                REG_TM0CNT_L++;
                if (REG_TM0CNT_L == 0)
                {
                    GBA_SoundTimerCheck(0);
                    REG_TM0CNT_L = Timer[0].start;
                    if (Timer[0].irqenable)
                        GBA_CallInterrupt(BIT(3));
                    timer0overflowed = 1;
                }
            }

            returnclocks = Timer[0].remainingclocks
                           * (0x10000 - (u32)REG_TM0CNT_L);
        }
    }

    if (Timer[1].enabled)
    {
        if (Timer[1].cascade == 0)
        {
            Timer[1].remainingclocks -= clocks;
            while (Timer[1].remainingclocks <= 0)
            {
                Timer[1].remainingclocks += Timer[1].clockspertick;
                REG_TM1CNT_L++;
                if (REG_TM1CNT_L == 0)
                {
                    GBA_SoundTimerCheck(1);
                    REG_TM1CNT_L = Timer[1].start;
                    if (Timer[1].irqenable)
                        GBA_CallInterrupt(BIT(4));
                    timer1overflowed = 1;
                }
            }

            returnclocks = min(returnclocks,
                               Timer[1].remainingclocks
                                   * (0x10000 - (u32)REG_TM1CNT_L));
        }
        else if (timer0overflowed)
        {
            REG_TM1CNT_L++;
            if (REG_TM1CNT_L == 0)
            {
                REG_TM1CNT_L = Timer[1].start;
                if (Timer[1].irqenable)
                    GBA_CallInterrupt(BIT(4));
                timer1overflowed = 1;
            }
        }
    }
    if (Timer[2].enabled)
    {
        if (Timer[2].cascade == 0)
        {
            Timer[2].remainingclocks -= clocks;
            while (Timer[2].remainingclocks <= 0)
            {
                Timer[2].remainingclocks += Timer[2].clockspertick;
                REG_TM2CNT_L++;
                if (REG_TM2CNT_L == 0)
                {
                    REG_TM2CNT_L = Timer[2].start;
                    if (Timer[2].irqenable)
                        GBA_CallInterrupt(BIT(5));
                    timer2overflowed = 1;
                }
            }

            returnclocks = min(returnclocks,
                               Timer[2].remainingclocks
                                   * (0x10000 - (u32)REG_TM2CNT_L));
        }
        else if (timer1overflowed)
        {
            REG_TM2CNT_L++;
            if (REG_TM2CNT_L == 0)
            {
                REG_TM2CNT_L = Timer[2].start;
                if (Timer[2].irqenable)
                    GBA_CallInterrupt(BIT(5));
                timer2overflowed = 1;
            }
        }
    }
    if (Timer[3].enabled)
    {
        if (Timer[3].cascade == 0)
        {
            Timer[3].remainingclocks -= clocks;
            while (Timer[3].remainingclocks <= 0)
            {
                Timer[3].remainingclocks += Timer[3].clockspertick;
                REG_TM3CNT_L++;
                if (REG_TM3CNT_L == 0)
                {
                    REG_TM3CNT_L = Timer[3].start;
                    if (Timer[3].irqenable)
                        GBA_CallInterrupt(BIT(6));
                }
            }

            returnclocks = min(returnclocks,
                               Timer[3].remainingclocks
                                   * (0x10000 - (u32)REG_TM3CNT_L));
        }
        else if (timer2overflowed)
        {
            REG_TM3CNT_L++;
            if (REG_TM3CNT_L == 0)
            {
                REG_TM3CNT_L = Timer[3].start;
                if (Timer[3].irqenable)
                    GBA_CallInterrupt(BIT(6));
            }
        }
    }

    return returnclocks;
}
