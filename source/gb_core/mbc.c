// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../debug_utils.h"
#include "../general_utils.h"

#include "camera.h"
#include "cpu.h"
#include "debug.h"
#include "gameboy.h"
#include "gb_main.h"
#include "general.h"
#include "interrupts.h"
#include "memory.h"
#include "sgb.h"
#include "sound.h"
#include "video.h"

//------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------

void GB_MapperInit(void)
{
    GameBoy.Emulator.rumble = 0;

    if (GameBoy.Emulator.MemoryController == MEM_CAMERA)
    {
        //if (GB_CameraInit(EmulatorConfig.debug_msg_enable) == 0)
        //    Debug_DebugMsgArg("Camera won't be emulated.");
        GB_CameraInit();
    }

    if (GameBoy.Emulator.MemoryController == MEM_MBC7)
    {
        GameBoy.Emulator.MBC7.sensorX = GameBoy.Emulator.MBC7.sensorY = 2047;
        GB_InputSetMBC7(0, 0);
    }
}

void GB_MapperEnd(void)
{
    if (GameBoy.Emulator.MemoryController == MEM_CAMERA)
        GB_CameraEnd();
}

//------------------------------------------------------------------------------

static void GB_NoMapperWrite(unused__ u32 address, unused__ u32 value)
{
    //Debug_DebugMsgArg("NO MAPPER WROTE - %02x to %04x", value, address);
    return;
}

// Used by MBC1 and HuC-1
static void GB_MBC1Write(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
        case 0x1: // RAM Enable
            mem->RAMEnabled = ((value & 0xF) == 0xA);
            if (GameBoy.Emulator.RAM_Banks == 0)
                mem->RAMEnabled = 0;
            break;
        case 0x2:
        case 0x3: // ROM Bank Number - lower 5 bits
            value &= 0x1F;
            if (value == 0)
                value = 1;
            mem->selected_rom &= ~0x1F;
            mem->selected_rom |= value;

            mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;

            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x4:
        case 0x5: // RAM Bank Number - or - Upper Bits of ROM Bank Number
            value &= 0x3;
            if (mem->mbc_mode == 0) // ROM mode
            {
                mem->selected_rom &= 0x1F;
                mem->selected_rom |= value << 5;

                mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;

                mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            }
            else // RAM mode
            {
                mem->selected_ram = value;
                mem->selected_ram &= GameBoy.Emulator.RAM_Banks - 1;
                mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
            }
            break;
        case 0x6:
        case 0x7: // ROM/RAM Mode Select
            mem->mbc_mode = value & 1;
            break;
        case 0xA:
        case 0xB:
            if (mem->RAMEnabled == 0)
                return;
            else
                mem->RAM_Curr[address - 0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("MBC1 WROTE - %02x to %04x", value, address);
            break;
    }
}

static void GB_MBC2Write(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
        case 0x1: // RAM Enable
            if ((address & 0x0100) == 0)
            {
                mem->RAMEnabled = ((value & 0xF) == 0xA);
                if (GameBoy.Emulator.RAM_Banks == 0)
                    mem->RAMEnabled = 0;
            }
            break;
        case 0x2:
        case 0x3: // ROM Bank Number
            //if (address & 0x0100)
            {
                if (value == 0)
                    value = 1;
                mem->selected_rom = value & (GameBoy.Emulator.ROM_Banks - 1);
                if (mem->selected_rom == 0)
                    mem->selected_rom++;
                mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            }
            break;
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
            break;
        case 0xA:
        case 0xB:
            if (mem->RAMEnabled == 0)
                return;
            else
                mem->RAM_Curr[address - 0xA000] = value & 0x0F;
            break;
        default:
            //Debug_DebugMsgArg("MBC2 WROTE - %02x to %04x", value, address);
            break;
    }
}

static void GB_MBC3Write(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
        case 0x1: // RAM Enable
            mem->RAMEnabled = ((value & 0xF) == 0xA);
            if (GameBoy.Emulator.RAM_Banks == 0)
                mem->RAMEnabled = 0;
            break;
        case 0x2:
        case 0x3: // ROM Bank Number
            mem->selected_rom = value;
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;
            if (mem->selected_rom == 0)
                mem->selected_rom = 1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x4:
        case 0x5: // RAM Bank Number - or - RTC Register Select
            value &= 0x0F;
            if (value <= 7) // RAM mode
            {
                mem->selected_ram = value & (GameBoy.Emulator.RAM_Banks - 1);

                mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
                mem->mbc_mode = 1;
            }
            else if (value >= 0x8 && value <= 0xC) // RTC mode
            {
                mem->selected_ram = value;
                mem->mbc_mode = 0;
            }
            break;
        case 0x6:
        case 0x7: // Latch Clock Data
            // The correct way of doing it is writing first 0x00 and then 0x01,
            // but some games don't do that.
            if (value == 0x01)
            {
                GameBoy.Emulator.LatchedTime.sec = GameBoy.Emulator.Timer.sec;
                GameBoy.Emulator.LatchedTime.min = GameBoy.Emulator.Timer.min;
                GameBoy.Emulator.LatchedTime.hour = GameBoy.Emulator.Timer.hour;
                GameBoy.Emulator.LatchedTime.days = GameBoy.Emulator.Timer.days;
                GameBoy.Emulator.LatchedTime.carry = GameBoy.Emulator.Timer.carry;
                GameBoy.Emulator.LatchedTime.halt = GameBoy.Emulator.Timer.halt;
            }
            break;
        case 0xA: // RAM Bank 00-03, if any
        case 0xB: // RTC Register 08-0C
            if (mem->RAMEnabled == 0)
                return;

            if (mem->mbc_mode == 1) // RAM mode
            {
                mem->RAM_Curr[address - 0xA000] = value;
            }
            else // RTC REGISTER
            {
                switch (mem->selected_ram)
                {
                    case 0x08: // Sec
                        GameBoy.Emulator.Timer.sec = value;
                        return;
                    case 0x09: // Min
                        GameBoy.Emulator.Timer.min = value;
                        return;
                    case 0x0A: // Hour
                        GameBoy.Emulator.Timer.hour = value;
                        return;
                    case 0x0B: // Lower 8 bits days
                        GameBoy.Emulator.Timer.days =
                                (GameBoy.Emulator.Timer.days & (1 << 8))
                                | (value & 0xFF);
                        return;
                    case 0x0C: // carry-halt-0-0-0-0-0-upper_bit_days
                        GameBoy.Emulator.Timer.days =
                                (GameBoy.Emulator.Timer.days & 0xFF)
                                | ((value & 0x01) << 8);
                        GameBoy.Emulator.Timer.carry = (value & (1 << 7)) >> 7;
                        GameBoy.Emulator.Timer.halt = (value & (1 << 6)) >> 6;
                        GameBoy.Emulator.LatchedTime.halt =
                                GameBoy.Emulator.Timer.halt;
                        return;
                    default:
                        return;
                }
                return;
            }
            break;
        default:
            //Debug_DebugMsgArg("MBC3 WROTE- %02x to %04x", value, address);
            break;
    }
}

// Used by MBC5 (and rumble)
static void GB_MBC5Write(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
        case 0x1:
            mem->RAMEnabled = ((value & 0x0F) == 0x0A);
            if (GameBoy.Emulator.RAM_Banks == 0)
                mem->RAMEnabled = 0;
            break;
        case 0x2:
            mem->selected_rom &= 0xFF00;
            mem->selected_rom |= (value & 0xFF);
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x3:
            mem->selected_rom &= 0xFF;
            mem->selected_rom |= value << 8;
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x4:
        case 0x5:
            if (GameBoy.Emulator.MemoryController == MEM_RUMBLE)
            {
                if (value & 0x08)
                    GameBoy.Emulator.rumble = 1;
                else
                    GameBoy.Emulator.rumble = 0;
                value &= 0x7;
            }
            mem->selected_ram = value & (GameBoy.Emulator.RAM_Banks - 1);
            mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
            break;
        case 0x6: // Some games write 0 and 1 here (for MBC1 compatibility?).
        case 0x7:
            break;
        case 0xA:
        case 0xB:
            if (mem->RAMEnabled == 0)
                return;
            else
                mem->RAM_Curr[address - 0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("MBC5 WROTE - %02x to %04x", value, address);
            break;
    }
}

// Doesn't work... or I've only tested damaged ROMs... I don't know.
// Some things copied from MESS
static void GB_MBC6Write(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
            if (address == 0x0000)
            {
                mem->RAMEnabled = ((value & 0x0A) == 0x0A);
                if (GameBoy.Emulator.RAM_Banks == 0)
                    mem->RAMEnabled = 0;
                break;
            }
            //Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x", value, address);
            break;
        case 0x2:
            if (address == 0x27FF)
            {
                mem->selected_rom = value;
                if (mem->selected_rom == 0)
                    mem->selected_rom = 1;
                mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;
            }
            else if (address == 0x2800)
            {
                if (value == 0)
                {
                    mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
                }
                //else
                //{
                //    Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",
                //                      value, address);
                //}
            }
            //else
            //{
            //    Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",
            //                      value, address);
            //}
            break;
        case 0x3:
            if (address == 0x37FF)
            {
                mem->selected_ram = value;
                mem->selected_ram &= GameBoy.Emulator.RAM_Banks - 1;
                //mem->selected_ram &= GameBoy.Emulator.ROM_Banks - 1;
            }
            else if (address == 0x3800)
            {
                if (value == 0x08)
                {
                    //mem->ROM_Base = mem->ROM_Switch[mem->selected_ram];
                    mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
                }
                //else
                //{
                //    Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",
                //                      value, address);
                //}
            }
            //else
            //{
            //    Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",
            //                      value, address);
            //}
            break;
        case 0xA:
        case 0xB:
            //Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x", value, address);
            if (mem->RAMEnabled == 0)
                return;
            else
                mem->RAM_Curr[address - 0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x", value, address);
            break;
    }
}

// Information from VisualBoy Advance
static void GB_MBC7Write(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
        case 0x1:
            break;
        case 0x2:
        case 0x3:
            mem->selected_rom = value;
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;
            if (mem->selected_rom == 0)
                mem->selected_rom = 1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x4:
        case 0x5:
            if (value < 8)
            {
                mem->RAMEnabled = 0;
                mem->selected_ram = value & (GameBoy.Emulator.RAM_Banks - 1);
                mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
            }
            else
            {
                mem->RAMEnabled = 0;
            }
            break;
        case 0xA:
        case 0xB:
            if (address == 0xa080)
            {
                _GB_MB7_CART_ *mbc7 = &GameBoy.Emulator.MBC7;
                // Special processing needed
                int oldCs = mbc7->cs;
                int oldSk = mbc7->sk;

                mbc7->cs = value >> 7;
                mbc7->sk = (value >> 6) & 1;

                if (!oldCs && mbc7->cs)
                {
                    if (mbc7->state == 5)
                    {
                        if (mbc7->writeEnable)
                        {
                            mem->RAM_Curr[mbc7->address * 2] =
                                    mbc7->buffer >> 8;
                            mem->RAM_Curr[mbc7->address * 2 + 1] =
                                    mbc7->buffer & 0xFF;
                        }
                        mbc7->state = 0;
                        mbc7->value = 1;
                    }
                    else
                    {
                        mbc7->idle = true;
                        mbc7->state = 0;
                    }
                }

                if (!oldSk && mbc7->sk)
                {
                    if (mbc7->idle)
                    {
                        if (value & 0x02)
                        {
                            mbc7->idle = false;
                            mbc7->count = 0;
                            mbc7->state = 1;
                        }
                    }
                    else
                    {
                        switch (mbc7->state)
                        {
                            case 1:
                                // Receiving command
                                mbc7->buffer <<= 1;
                                mbc7->buffer |= (value & 0x02) ? 1 : 0;
                                mbc7->count++;
                                if (mbc7->count == 2)
                                {
                                    // Finished receiving command
                                    mbc7->state = 2;
                                    mbc7->count = 0;
                                    mbc7->code = mbc7->buffer & 3;
                                }
                                break;
                            case 2:
                                // Receive address
                                mbc7->buffer <<= 1;
                                mbc7->buffer |= (value & 0x02) ? 1 : 0;
                                mbc7->count++;
                                if (mbc7->count == 8)
                                {
                                    // Finish receiving
                                    mbc7->state = 3;
                                    mbc7->count = 0;
                                    mbc7->address = mbc7->buffer & 0xFF;
                                    if (mbc7->code == 0)
                                    {
                                        if ((mbc7->address >> 6) == 0)
                                        {
                                            mbc7->writeEnable = 0;
                                            mbc7->state = 0;
                                        }
                                        else if ((mbc7->address >> 6) == 3)
                                        {
                                            mbc7->writeEnable = 1;
                                            mbc7->state = 0;
                                        }
                                    }
                                }
                                break;
                            case 3:
                                mbc7->buffer <<= 1;
                                mbc7->buffer |= (value & 0x02) ? 1 : 0;
                                mbc7->count++;

                                switch (mbc7->code)
                                {
                                    case 0:
                                        if (mbc7->count == 16)
                                        {
                                            if ((mbc7->address >> 6) == 0)
                                            {
                                                mbc7->writeEnable = 0;
                                                mbc7->state = 0;
                                            }
                                            else if ((mbc7->address >> 6) == 1)
                                            {
                                                if (mbc7->writeEnable)
                                                {
                                                    for (int i = 0; i < 256; i++)
                                                    {
                                                        mem->RAM_Curr[i * 2] =
                                                                mbc7->buffer >> 8;
                                                        mem->RAM_Curr[i * 2 + 1] =
                                                                mbc7->buffer & 0xFF;
                                                    }
                                                }
                                                mbc7->state = 5;
                                            }
                                            else if ((mbc7->address >> 6) == 2)
                                            {
                                                if (mbc7->writeEnable)
                                                {
                                                    for (int i = 0; i < 256; i++)
                                                    {
                                                        u16 *p;
                                                        p = (u16 *)&mem->RAM_Curr[i * 2];
                                                        *p = 0xFFFF;
                                                    }
                                                }
                                                mbc7->state = 5;
                                            }
                                            else if ((mbc7->address >> 6) == 3)
                                            {
                                                mbc7->writeEnable = 1;
                                                mbc7->state = 0;
                                            }
                                            mbc7->count = 0;
                                        }
                                        break;
                                    case 1:
                                        if (mbc7->count == 16)
                                        {
                                            mbc7->count = 0;
                                            mbc7->state = 5;
                                            mbc7->value = 0;
                                        }
                                        break;
                                    case 2:
                                        if (mbc7->count == 1)
                                        {
                                            mbc7->state = 4;
                                            mbc7->count = 0;
                                            mbc7->buffer =
                                                    (mem->RAM_Curr[mbc7->address * 2] << 8)
                                                    | (mem->RAM_Curr[mbc7->address * 2 + 1]);
                                        }
                                        break;
                                    case 3:
                                        if (mbc7->count == 16)
                                        {
                                            mbc7->count = 0;
                                            mbc7->state = 5;
                                            mbc7->value = 0;
                                            mbc7->buffer = 0xFFFF;
                                        }
                                        break;
                                }
                                break;
                        }
                    }
                }
                if (oldSk && !mbc7->sk)
                {
                    if (mbc7->state == 4)
                    {
                        mbc7->value = (mbc7->buffer & 0x8000) ? 1 : 0;
                        mbc7->buffer <<= 1;
                        mbc7->count++;
                        if (mbc7->count == 16)
                        {
                            mbc7->count = 0;
                            mbc7->state = 0;
                        }
                    }
                }
            }

            break;
        default:
            //Debug_DebugMsgArg("MBC7 WROTE - %02x to %04x", value, address);
            break;
    }
}

// "Taito Variety Pack" works, but "Momotarou Collection 2" is very strange...
static void GB_MMM01Write(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0: // Apply mapping
        case 0x1:
            if (GameBoy.Emulator.EnableBank0Switch == 1)
            {
                // Momotarou Collection 2 writes 3A and 7A, so this check works
                // too
                if (value & 0x40) // Taito Pack
                {
                    mem->ROM_Base =
                            mem->ROM_Switch[GameBoy.Emulator.MMM01.offset
                            & (GameBoy.Emulator.ROM_Banks - 1)];
                    mem->selected_rom = (GameBoy.Emulator.MMM01.offset + 1)
                                        & (GameBoy.Emulator.ROM_Banks - 1);
                    mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
                    GameBoy.Emulator.EnableBank0Switch = 0;
                }
            }
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x", value, address);
            break;
        case 0x2: // ROM Bank Number / New mapping offset
            if (GameBoy.Emulator.EnableBank0Switch == 1) // Taito Pack
            {
                GameBoy.Emulator.MMM01.offset =
                        0x02 // first 2 banks, used by game selection menu
                        + (value & 0x1F);
            }
            else
            {
                // Taito Pack
                mem->selected_rom = (value & GameBoy.Emulator.MMM01.mask)
                                    + GameBoy.Emulator.MMM01.offset;
                mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            }
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x", value, address);
            break;
        case 0x3: // RAM Enable ?
            // Momotarou Collection 2 - i've only seen it writing to 3FFF
            mem->RAMEnabled = value & 0x01;
            if (GameBoy.Emulator.RAM_Banks == 0)
                mem->RAMEnabled = 0;
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x", value, address);
            break;
        case 0x4: // ?
            // Taito Pack writes 0x70 here... no idea of why does it do it...
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x", value, address);
            break;
        case 0x5:
            if (value == 0x40)
                GameBoy.Emulator.MMM01.mask = 0x1F; // Momotarou Collection 2
            else if (value == 0x01)
                GameBoy.Emulator.MMM01.mask = 0x0F; // Who knows...
            else
                GameBoy.Emulator.MMM01.mask = 0xFF;
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x", value, address);
            break;
        case 0x6: // New mapping mask
            if (GameBoy.Emulator.EnableBank0Switch == 1) // Taito Pack
            {
                if (value == 0x38)
                    GameBoy.Emulator.MMM01.mask = 0x03;
                else if (value == 0x30)
                    GameBoy.Emulator.MMM01.mask = 0x07;
                else
                    GameBoy.Emulator.MMM01.mask = 0xFF;
            }
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x", value, address);
            break;
        case 0x7:
            // Momotarou Collection 2
            if (GameBoy.Emulator.EnableBank0Switch == 1)
                GameBoy.Emulator.MMM01.offset = value; // Who knows...
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x", value, address);
            break;
        case 0xA:
        case 0xB:
            if (mem->RAMEnabled == 0)
                return;

            mem->RAM_Curr[address - 0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x", value, address);
            break;
    }
}

static void GB_CameraWrite(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (address >> 12)
    {
        case 0x0:
        case 0x1: // Enable writing only! - Reading is always allowed (?)
            mem->RAMEnabled = ((value & 0x0F) == 0x0A);
            if (GameBoy.Emulator.RAM_Banks == 0)
                mem->RAMEnabled = 0;
            break;
        case 0x2: // ROM change. Bank 0 is allowed
            mem->selected_rom &= 0xFF00;
            mem->selected_rom |= (value & 0xFF);
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x3:
            break;
        case 0x4:
            // Any value with bit 4 set to '1' will set register mode
            if (value & 0x10)
            {
                mem->mbc_mode = 1;
            }
            else
            {
                mem->mbc_mode = 0;
                mem->selected_ram = value & (GameBoy.Emulator.RAM_Banks - 1);
                mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
            }
            break;
        case 0x5:
        case 0x6:
        case 0x7:
            break;
        case 0xA:
        case 0xB:
            if (mem->mbc_mode == 1)
            {
                GB_CameraWriteRegister(address, value);
                break;
            }

            // If not in register mode, try to access RAM
            if (mem->RAMEnabled == 0)
                return;

            mem->RAM_Curr[address - 0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("CAMERA WROTE - %02x to %04x", value, address);
            break;
    }
}

//------------------------------------------------------------------------------

static u32 GB_NoMapperRead(unused__ u32 address)
{
    //Debug_DebugMsgArg("NO MAPPER READ - %04x", address);
    return 0xFF;
}

// Used by MBC1, HuC-1, MBC5 (and rumble), MBC6, MMM01
static u32 GB_MBC1Read(u32 address)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if ((GameBoy.Emulator.RAM_Banks == 0) || (mem->RAMEnabled == 0))
        return 0xFF;

    return mem->RAM_Curr[address - 0xA000];
}

static u32 GB_MBC2Read(u32 address)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if ((GameBoy.Emulator.RAM_Banks == 0) || (mem->RAMEnabled == 0))
        return 0xFF;

    return mem->RAM_Curr[address - 0xA000] | 0xF0;
}

static u32 GB_MBC3Read(u32 address)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (mem->RAMEnabled == 0)
        return 0xFF;

    if (mem->mbc_mode == 1) // RAM mode
    {
        if (GameBoy.Emulator.RAM_Banks == 0)
            return 0xFF;

        return mem->RAM_Curr[address - 0xA000];
    }
    else // RTC mode
    {
        switch (mem->selected_ram)
        {
            case 0x08: // Sec
                return GameBoy.Emulator.LatchedTime.sec;
            case 0x09: // Min
                return GameBoy.Emulator.LatchedTime.min;
            case 0x0A: // Hour
                return GameBoy.Emulator.LatchedTime.hour;
            case 0x0B: // Lower 8 bits days
                return GameBoy.Emulator.LatchedTime.days & 0xFF;
            case 0x0C: // carry-halt- - - - - -upper bit days
                return (GameBoy.Emulator.LatchedTime.days >> 8)
                       | (GameBoy.Emulator.LatchedTime.halt << 6)
                       | (GameBoy.Emulator.LatchedTime.carry << 7);
            default:
                return 0xFF;
        }
    }

    return 0xFF;
}

static u32 GB_MBC7Read(u32 address)
{
    switch (address & 0xa0f0)
    {
        case 0xA000:
        case 0xA010:
        case 0xA060:
        case 0xA070:
            return 0;
        case 0xA020:
            // Sensor X low byte
            return GameBoy.Emulator.MBC7.sensorX & 255;
        case 0xA030:
            // Sensor X high byte
            return GameBoy.Emulator.MBC7.sensorX >> 8;
        case 0xA040:
            // Sensor Y low byte
            return GameBoy.Emulator.MBC7.sensorY & 255;
        case 0xA050:
            // Sensor Y high byte
            return GameBoy.Emulator.MBC7.sensorY >> 8;
        case 0xA080:
            return GameBoy.Emulator.MBC7.value;
        default:
            return 0xFF;
    }
    return 0xFF;
}

static u32 GB_CameraRead(u32 address)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (mem->mbc_mode == 1) // CAM registers
    {
        return GB_CameraReadRegister(address);
    }
    else // RAM -- image captured by camera goes to bank 0
    {
        return mem->RAM_Curr[address - 0xA000];
    }
    return 0xFF;
}

//------------------------------------------------------------------------------

void GB_MapperSet(int type)
{
    switch (type)
    {
        default:
        case MEM_NONE:
        case MEM_MBC4:
        case MEM_HUC3:
        case MEM_TAMA5:
            GameBoy.Memory.MapperWrite = &GB_NoMapperWrite;
            GameBoy.Memory.MapperRead = &GB_NoMapperRead;
            break;
        case MEM_MBC1:
        case MEM_HUC1:
            GameBoy.Memory.MapperWrite = &GB_MBC1Write;
            GameBoy.Memory.MapperRead = &GB_MBC1Read;
            break;
        case MEM_MBC2:
            GameBoy.Memory.MapperWrite = &GB_MBC2Write;
            GameBoy.Memory.MapperRead = &GB_MBC2Read;
            break;
        case MEM_MBC3:
            GameBoy.Memory.MapperWrite = &GB_MBC3Write;
            GameBoy.Memory.MapperRead = &GB_MBC3Read;
            break;
        case MEM_MBC5:
        case MEM_RUMBLE:
            GameBoy.Memory.MapperWrite = &GB_MBC5Write;
            GameBoy.Memory.MapperRead = &GB_MBC1Read;
            break;
        case MEM_MBC6:
            GameBoy.Memory.MapperWrite = &GB_MBC6Write;
            GameBoy.Memory.MapperRead = &GB_MBC1Read;
            break;
        case MEM_MBC7:
            GameBoy.Memory.MapperWrite = &GB_MBC7Write;
            GameBoy.Memory.MapperRead = &GB_MBC7Read;
            break;
        case MEM_MMM01:
            GameBoy.Memory.MapperWrite = &GB_MMM01Write;
            GameBoy.Memory.MapperRead = &GB_MBC1Read;
            break;
        case MEM_CAMERA:
            GameBoy.Memory.MapperWrite = &GB_CameraWrite;
            GameBoy.Memory.MapperRead = &GB_CameraRead;
            break;
    }
}
