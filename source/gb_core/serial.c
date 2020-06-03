// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../config.h"
#include "../debug_utils.h"
#include "../file_utils.h"
#include "../general_utils.h"
#include "../png_utils.h"

#include "cpu.h"
#include "debug.h"
#include "gameboy.h"
#include "general.h"
#include "interrupts.h"
#include "serial.h"

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------

static void GB_SerialSendBit(void)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    GameBoy.Emulator.serial_transfered_bits++;

    if ((GameBoy.Emulator.serial_transfered_bits & 7) == 0)
    {
        GameBoy.Emulator.serial_enabled = 0;

        GB_InterruptsSetFlag(I_SERIAL);

        GameBoy.Emulator.SerialSend_Fn(mem->IO_Ports[SB_REG - 0xFF00]);

        mem->IO_Ports[SC_REG - 0xFF00] &= ~0x80;
        mem->IO_Ports[SB_REG - 0xFF00] = GameBoy.Emulator.SerialRecv_Fn();

        GB_CPUBreakLoop();
    }
}

//------------------------------------------------------------------------------

static int gb_serial_clock_counter = 0;

void GB_SerialClockCounterReset(void)
{
    gb_serial_clock_counter = 0;
}

static int GB_SerialClockCounterGet(void)
{
    return gb_serial_clock_counter;
}

static void GB_SerialClockCounterSet(int new_reference_clocks)
{
    gb_serial_clock_counter = new_reference_clocks;
}

void GB_SerialUpdateClocksCounterReference(int reference_clocks)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    int increment_clocks = reference_clocks - GB_SerialClockCounterGet();

    if (GameBoy.Emulator.serial_enabled)
    {
        if (mem->IO_Ports[SC_REG - 0xFF00] & 0x01) // Internal clock
        {
            int flip_clocks =
                    GameBoy.Emulator.serial_clocks_to_flip_clock_signal;

            int serial_new_clocks =
                    (GameBoy.Emulator.serial_clocks & (flip_clocks - 1))
                    + increment_clocks;

            while (serial_new_clocks >= flip_clocks)
            {
                serial_new_clocks -= flip_clocks;

                GameBoy.Emulator.serial_clock_signal ^= 1;

                if (GameBoy.Emulator.serial_clock_signal == 0) // Falling edge
                    GB_SerialSendBit();
            }
        }
    }

    GameBoy.Emulator.serial_clocks += increment_clocks;
    GameBoy.Emulator.serial_clocks &= (512 / 2) - 1;

    GB_SerialClockCounterSet(reference_clocks);
}

int GB_SerialGetClocksToNextEvent(void)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (GameBoy.Emulator.serial_enabled)
    {
        if (mem->IO_Ports[SC_REG - 0xFF00] & 0x01) // Internal clock
        {
            int clocks = GameBoy.Emulator.serial_clocks_to_flip_clock_signal;

            return clocks - (GameBoy.Emulator.serial_clocks & (clocks - 1));
        }
    }

    return 0x7FFFFFFF;
}

//------------------------------------------------------------------------------

void GB_SerialWriteSB(int reference_clocks, int value)
{
    GB_SerialUpdateClocksCounterReference(reference_clocks);
    GameBoy.Memory.IO_Ports[SB_REG - 0xFF00] = value;
}

void GB_SerialWriteSC(int reference_clocks, int value)
{
    GB_SerialUpdateClocksCounterReference(reference_clocks);

    if (value & 0x80)
    {
        GameBoy.Emulator.serial_enabled = 1;
        GameBoy.Emulator.serial_transfered_bits = 0;
        GameBoy.Emulator.serial_clock_signal = 0;

        if (value & 0x01) // Internal clock
        {
            if (GameBoy.Emulator.CGBEnabled == 1)
            {
#if 0
                int old_sc = GameBoy.Memory.IO_Ports[SC_REG - 0xFF00];
                int new_sc = value;

                // Change speed when enabled! Check if glitch
                if ((old_sc & 0x80) && ((old_sc ^ new_sc) & BIT(1)))
                {
                    int old_signal = GameBoy.Emulator.serial_clocks
                                   & (old_sc & BIT(1) ? 16 / 2 : 512 / 2);
                    int new_signal = GameBoy.Emulator.serial_clocks
                                   & (new_sc & BIT(1) ? 16 / 2 : 512 / 2);

                    if ((old_signal == 0) && (new_signal != 0))
                    {
                        GameBoy.Emulator.serial_clock_signal ^= 1;

                        // Falling edge
                        if (GameBoy.Emulator.serial_clock_signal == 0)
                            GB_SerialSendBit();
                    }
                }
#endif
                if (value & 0x02)
                    GameBoy.Emulator.serial_clocks_to_flip_clock_signal = 16 / 2;
                else
                    GameBoy.Emulator.serial_clocks_to_flip_clock_signal = 512 / 2;
            }
            else
            {
                GameBoy.Emulator.serial_clocks_to_flip_clock_signal = 512 / 2;
            }
        }
    }
    else
    {
        GameBoy.Emulator.serial_enabled = 0;
        GameBoy.Emulator.serial_transfered_bits = 0;
    }

    GameBoy.Memory.IO_Ports[SC_REG - 0xFF00] = value;
    GB_CPUBreakLoop();
}

//------------------------------------------------------------------------------
//                                NO DEVICE

static void GB_SendNone(unused__ u32 data)
{
    return;
}

static u32 GB_RecvNone(void)
{
    return 0xFF;
}

//------------------------------------------------------------------------------
//                                GB PRINTER

// Screen is 20x18 tiles -> 20x18x16 bytes -> 20x18x16 / 0x280 = 9 (+ 1 empty)
#define GBPRINTER_NUMPACKETS (10)

typedef struct
{
    int state;
    int output;

    // 0x88 | 0x33 | CMD | COMPRESSED | SIZE LSB | SIZE MSB
    // ...DATA...
    // CHECKSUM LSB | CHECKSUM MSB | 0 | 0
    u32 cmd;
    u32 size;
    u8 *data;
    u32 dataindex;
    u8 end[4];

    u32 curpacket;
    u8 *packets[GBPRINTER_NUMPACKETS];
    u32 packetsize[GBPRINTER_NUMPACKETS];
    u32 packetcompressed[GBPRINTER_NUMPACKETS];
} _GB_PRINTER_;

_GB_PRINTER_ GB_Printer;

int printer_file_number = 0;

static void GB_PrinterPrint(void)
{
    char *endbuffer = malloc(20 * 18 * 16);
    memset(endbuffer, 0, 20 * 18 * 16);

    char *ptr = endbuffer;

    for (int i = 0; i < GBPRINTER_NUMPACKETS; i++)
    {
        if (GB_Printer.packetcompressed[i] == 0)
        {
            memcpy(ptr, GB_Printer.packets[i], GB_Printer.packetsize[i]);
            ptr += GB_Printer.packetsize[i];
        }
        else
        {
            unsigned char *src = GB_Printer.packets[i];
            int size = GB_Printer.packetsize[i];

            while (1)
            {
                int data = *src++;
                size--;

                if (data & 0x80) // Repeat
                {
                    data = (data & 0x7F) + 2;
                    memset(ptr, *src++, data);
                    size--;
                    ptr += data;
                }
                else // Normal
                {
                    data++;
                    memcpy(ptr, src, data);
                    ptr += data;
                    src += data;
                    size -= data;
                }

                if (size == 0)
                    break;

                if (size < 0)
                    Debug_ErrorMsgArg("GB_PrinterPrint: size < 0 (%d)", size);
            }
        }
    }

    char *filename = FU_GetNewTimestampFilename("gb_printer");

    u32 buf_temp[160 * 144 * 4];
    memset(buf_temp, 0, sizeof(buf_temp));
    const u32 gb_pal_colors[4][3] = {
        { 255, 255, 255 }, { 168, 168, 168 }, { 80, 80, 80 }, { 0, 0, 0 }
    };

    for (int y = 0; y < 144; y++)
    {
        for (int x = 0; x < 160; x++)
        {
            int tile = ((y >> 3) * 20) + (x >> 3);

            char *tileptr = &endbuffer[tile * 16];

            tileptr += (y & 7) * 2;

            int x_ = 7 - (x & 7);

            int color =
                    ((*tileptr >> x_) & 1) | ((((*(tileptr + 1)) >> x_) << 1) & 2);

            buf_temp[y * 160 + x] = (gb_pal_colors[color][0] << 16)
                                    | (gb_pal_colors[color][1] << 8)
                                    | gb_pal_colors[color][2];
        }
    }

    Save_PNG(filename, 160, 144, buf_temp, 0);

    printer_file_number++;
}

static void GB_PrinterReset(void)
{
    if (GB_Printer.data != NULL)
    {
        free(GB_Printer.data);
        GB_Printer.data = NULL;
    }

    GB_Printer.curpacket = 0;

    for (int i = 0; i < GBPRINTER_NUMPACKETS; i++)
    {
        GB_Printer.packetsize[i] = 0;

        if (GB_Printer.packets[i] != NULL)
        {
            free(GB_Printer.packets[i]);
            GB_Printer.packets[i] = NULL;
            GB_Printer.packetcompressed[i] = 0;
        }
    }

    GB_Printer.state = 0;
    GB_Printer.output = 0;
}

static int GB_PrinterChecksumIsCorrect(void)
{
    // Checksum
    u32 checksum = (GB_Printer.end[0] & 0xFF)
                   | ((GB_Printer.end[1] & 0xFF) << 8);
    u32 result = GB_Printer.cmd
                 + GB_Printer.packetcompressed[GB_Printer.curpacket];

    if (GB_Printer.size > 0)
    {
        result += (GB_Printer.size & 0xFF) + ((GB_Printer.size >> 8) & 0xFF);
        for (int i = 0; i < GB_Printer.size; i++)
            result += GB_Printer.data[i];
    }

    result &= 0xFFFF;

    if (result != checksum)
    {
        Debug_DebugMsgArg("GB Printer packet corrupted.\n"
                          "Obtained: 0x%04x (0x%04x)",
                          result, checksum);
    }

    return (result == checksum);
}

static void GB_PrinterExecute(void)
{
    switch (GB_Printer.cmd)
    {
        case 0x01: // Init
            GB_PrinterReset();
            break;

        case 0x02: // End - Print
            // data = palettes?, but... what do they do?
            GB_PrinterPrint();
            break;

        case 0x04: // Data
            if (GB_Printer.curpacket >= GBPRINTER_NUMPACKETS)
            {
                Debug_ErrorMsgArg("GB Printer packet limit reached.");
            }
            else
            {
                GB_Printer.packets[GB_Printer.curpacket] = GB_Printer.data;
                GB_Printer.packetsize[GB_Printer.curpacket++] = GB_Printer.size;
                GB_Printer.data = NULL;
            }
            break;

        case 0x0F: // Nothing
            break;
    }
}

static void GB_SendPrinter(u32 data)
{
    switch (GB_Printer.state)
    {
        case 0: // Magic 1
            GB_Printer.output = 0x00;
            if (data == 0x88)
                GB_Printer.state = 1;
            break;
        case 1: // Magic 2
            if (data == 0x33)
                GB_Printer.state = 2;
            else // Reset...
                GB_Printer.state = 0;
            break;
        case 2: // CMD
            GB_Printer.cmd = data;
            GB_Printer.state = 3;
            break;
        case 3: // COMPRESSED
            GB_Printer.packetcompressed[GB_Printer.curpacket] = data;
            GB_Printer.state = 4;
            break;
        case 4: // Size 1
            GB_Printer.size = data;
            GB_Printer.state = 5;
            break;
        case 5: // Size 2
            GB_Printer.size = (GB_Printer.size & 0xFF) | (data << 8);

            GB_Printer.dataindex = 0;

            if (GB_Printer.size > 0)
            {
                GB_Printer.data = malloc(GB_Printer.size);
                GB_Printer.state = 6;
            }
            else
            {
                GB_Printer.state = 7;
            }
            break;
        case 6: // Data
            GB_Printer.data[GB_Printer.dataindex++] = data;

            if (GB_Printer.dataindex == GB_Printer.size)
            {
                GB_Printer.dataindex = 0;
                GB_Printer.state = 7;
            }
            break;
        case 7: // End
            GB_Printer.end[GB_Printer.dataindex++] = data;

            if (GB_Printer.dataindex == 3)
            {
                if (GB_PrinterChecksumIsCorrect())
                    GB_Printer.output = 0x81;
                else
                    GB_Printer.output = 0x00;
            }
            else
            {
                GB_Printer.output = 0x00;
            }

            if (GB_Printer.dataindex == 4)
            {
                GB_PrinterExecute();
                GB_Printer.state = 0; // Done
            }
            break;
    }

    return;
}

static u32 GB_RecvPrinter(void)
{
    return GB_Printer.output;
}

//------------------------------------------------------------------------------

void GB_SerialPlug(int device)
{
    if (GameBoy.Emulator.serial_device != SERIAL_NONE)
        GB_SerialEnd();

    GameBoy.Emulator.serial_device = device;

    switch (device)
    {
        case SERIAL_NONE:
            GameBoy.Emulator.SerialSend_Fn = &GB_SendNone;
            GameBoy.Emulator.SerialRecv_Fn = &GB_RecvNone;
            break;
        case SERIAL_GBPRINTER:
            GameBoy.Emulator.SerialSend_Fn = &GB_SendPrinter;
            GameBoy.Emulator.SerialRecv_Fn = &GB_RecvPrinter;
            GB_PrinterReset();
            break;
        case SERIAL_GAMEBOY: // TODO
            //break;
        default:
            GameBoy.Emulator.SerialSend_Fn = &GB_SendNone;
            GameBoy.Emulator.SerialRecv_Fn = &GB_RecvNone;
            break;
    }
}

//------------------------------------------------------------------------------

void GB_SerialInit(void)
{
    GameBoy.Emulator.serial_enabled = 0;
    GameBoy.Emulator.serial_clocks_to_flip_clock_signal = 512 / 2;
    GameBoy.Emulator.serial_transfered_bits = 0;
    GameBoy.Emulator.serial_clocks = 0;
    GameBoy.Emulator.serial_clock_signal = 0;

    if (GameBoy.Emulator.enable_boot_rom) // Unknown
    {
        switch (GameBoy.Emulator.HardwareType)
        {
            case HW_GB:
            case HW_GBP: // TODO: Can't verify until PPU is emulated correctly
                GameBoy.Emulator.serial_clocks = 8;
                break;

            case HW_SGB:
            case HW_SGB2: // TODO: Unknown. Can't test.
                GameBoy.Emulator.serial_clocks = 0;
                break;

            case HW_GBC: // TODO: Can't verify until PPU is emulated correctly
                // Same value for GBC in GBC or GB mode. The boot ROM starts the
                // same way.
                GameBoy.Emulator.serial_clocks = 0;
                break;

            case HW_GBA: // TODO: Can't verify until PPU is emulated correctly
            case HW_GBA_SP:
                // Same value for GBC in GBC or GB mode. The boot ROM starts the
                // same way.
                GameBoy.Emulator.serial_clocks = 0;
                break;

            default:
                GameBoy.Emulator.sys_clocks = 0;
                Debug_ErrorMsg("GB_SerialInit():\n"
                               "Unknown hardware\n"
                               "(boot ROM enabled)");
                break;
        }
    }
    else
    {
        switch (GameBoy.Emulator.HardwareType)
        {
            case HW_GB:
            case HW_GBP:
                GameBoy.Emulator.serial_clocks = 0xCC; // Verified on hardware
                break;

            case HW_SGB:
            case HW_SGB2: // TODO: Unknown. Can't test.
                GameBoy.Emulator.serial_clocks = 0;
                break;

            case HW_GBC:
                GameBoy.Emulator.serial_clocks = 0xA0; // Verified on hardware
                // TODO: GBC in GB mode? Use value corresponding to a boot
                // without any user interaction.
                break;

            case HW_GBA:
            case HW_GBA_SP: // TODO: Verify on hardware
                GameBoy.Emulator.serial_clocks = 0xA4;
                // TODO: GBC in GB mode? Use value corresponding to a boot
                // without any user interaction.
                break;

            default:
                GameBoy.Emulator.sys_clocks = 0;
                Debug_ErrorMsg("GB_SerialInit():\n"
                               "Unknown hardware");
                break;
        }
    }

    GameBoy.Emulator.serial_device = SERIAL_NONE;
    GB_SerialPlug(EmulatorConfig.serial_device);
}

void GB_SerialEnd(void)
{
    if (GameBoy.Emulator.serial_device == SERIAL_GBPRINTER)
        GB_PrinterReset();

    if (GameBoy.Emulator.serial_device == SERIAL_GAMEBOY)
    {
        // TODO
    }
}
