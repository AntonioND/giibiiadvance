// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include "../build_options.h"
#include "../config.h"
#include "../debug_utils.h"

#include "gba.h"
#include "memory.h"
#include "save.h"

typedef struct {
    u32 entry_point; // 32bit ARM branch opcode, eg. "B rom_start"
    // Usually: entry_point & 0xFFC00000 = 0xEA000000.
    // Most times this value is 0xEA00002E

    u8 nintendo_logo[156]; // Compressed bitmap, required!

    // 09Ch Bit 2,7 - Debugging Enable
    // This is part of the above Nintendo Logo area, and must be commonly set to
    // 21h, however, Bit 2 and Bit 7 may be set to other values.  When both bits
    // are set (ie. A5h), the FIQ/Undefined Instruction handler in the BIOS
    // becomes unlocked, the handler then forwards these exceptions to the user
    // handler in cartridge ROM (entry point defined in 80000B4h, see below).
    // Other bit combinations currently do not seem to have special functions.
    //
    // 09Eh Bit 0,1 - Cartridge Key Number LSBs  <-- GBATEK SAYS MSBs!!!
    // This is part of the above Nintendo Logo area, and must be commonly set to
    // F8h, however, Bit 0-1 may be set to other values.  During startup, the
    // BIOS performs some dummy-reads from a stream of pre-defined addresses,
    // even though these reads seem to be meaningless, they might be intended to
    // unlock a read-protection inside of commercial cartridge. There are 16
    // pre-defined address streams - selected by a 4bit key number - of which
    // the upper two bits are gained from 800009Eh Bit 0-1, and the lower two
    // bits from a checksum across header bytes 09Dh..0B7h (bytewise XORed,
    // divided by 40h).

    char game_title[12]; // Uppercase ASCII, max 12 characters
    // Space for the game title, padded with 00h (if less than 12 chars).

    char game_code[4]; // Uppercase ASCII, 4 characters

    // This is the same code as the AGB-UTTD code which is printed on the
    // package and sticker on (commercial) cartridges (excluding the leading
    // "AGB-" part).
    //   U  Unique Code          (usually "A" or "B" or special meaning)
    //   TT Short Title          (eg. "PM" for Pac Man)
    //   D  Destination/Language (usually "J" or "E" or "P" or specific language)
    // The first character (U) is usually "A" or "B", in detail:
    //   A  Normal game; Older titles (mainly 2001..2003)
    //   B  Normal game; Newer titles (2003..)
    //   C  Normal game; Not used yet, but might be used for even newer titles
    //   F  Classic NES Series (software emulated NES games)
    //   K  Yoshi and Koro Koro Puzzle (acceleration sensor)
    //   P  e-Reader (dot-code scanner)
    //   R  Warioware Twisted (cartridge with rumble and z-axis gyro sensor)
    //   U  Boktai 1 and 2 (cartridge with RTC and solar sensor)
    //   V  Drill Dozer (cartridge with rumble)
    // The second/third characters (TT) are:
    //   Usually an abbreviation of the game title (eg. "PM" for "Pac Man")
    //   (unless that gamecode was already used for another game, then TT is
    //   just random)
    // The fourth character (D) indicates Destination/Language:
    //   J  Japan          P  Europe/Elsewhere   F  French          S  Spanish
    //   E  USA/English    D  German             I  Italian

    char maker_code[2]; // Uppercase ASCII, 2 characters
    // Identifies the (commercial) developer. For example, "01"=Nintendo.

    u8 fixed_96h; // Must be 96h, required!

    // Identifies the required hardware. 00h for current GBA models.
    u8 main_unit_code;

    u8 device_type; // Usually 00h
    // Normally, this entry should be zero. With Nintendo's hardware debugger
    // Bit 7 identifies the debugging handlers entry point and size of DACS
    // (Debugging And Communication System) memory: Bit7=0: 9FFC000h/8MBIT DACS,
    // Bit7=1: 9FE2000h/1MBIT DACS. The debugging handler can be enabled in
    // 800009Ch (see above), normal cartridges do not have any memory (nor any
    // mirrors) at these addresses though.

    u8 reserved_1[7]; // Should be zero filled

    u8 software_version; // Usually 00h

    u8 complement_check; // Header checksum, required!
    // chk = 0
    // for i = 0A0h to 0BCh:
    //     chk = chk - [i]
    //     next
    // chk = (chk - 19h) and 0FFh

    u8 reserved_2[2]; // Should be zero filled

    // Multiboot

    u32 ram_entry_point; // 32bit ARM branch opcode, eg. "B ram_start"

    u8 boot_mode; // Init as 00h - BIOS overwrites this value!
    // The slave GBA download procedure overwrites this byte by a value which is
    // indicating the used multiboot transfer mode.
    //   Value  Expl.
    //   01h    Joybus mode
    //   02h    Normal mode
    //   03h    Multiplay mode
    // Typically set this byte to zero by inserting DCB 00h in your source.  Be
    // sure that your uploaded program does not contain important program code
    // or data at this location, or at the ID-byte location below.

    u8 slave_ID_number; // Init as 00h - BIOS overwrites this value!
    // If the GBA has been booted in Normal or Multiplay mode, this byte becomes
    // overwritten by the slave ID number of the local GBA (that'd be always 01h
    // for normal mode).
    //   Value  Expl.
    //   01h    Slave #1
    //   02h    Slave #2
    //   03h    Slave #3
    // Typically set this byte to zero by inserting DCB 00h in your source.
    // When booted in Joybus mode, the value is NOT changed and remains the same
    // as uploaded from the master GBA.

    u8 unused[26]; // Seems to be unused

    u32 joybus_entry_point; // 32bit ARM branch opcode, eg. "B joy_start"

    // 0E0h - Joybus mode Entry Point
    // If the GBA has been booted by using Joybus transfer mode, then the entry
    // point is located at this address rather than at 20000C0h. Either put your
    // initialization procedure directly at this address, or redirect to the
    // actual boot procedure by depositing a "B <start>" opcode here (either one
    // using 32bit ARM code). Or, if you are not intending to support joybus
    // mode (which is probably rarely used), ignore this entry.
} _gba_header_;

// Note: Regardless of the entry point used, the CPU is initially set into
// system mode.

//------------------------------------------------------------------------------

static int showconsole = 0;

int GBA_ShowConsoleRequested(void)
{
    int ret = showconsole;
    showconsole = 0;
    return ret;
}

void GBA_HeaderCheck(void *rom)
{
    // If at the end of the function this is 1, show console window
    showconsole = 0;

    _gba_header_ *header = (_gba_header_ *)rom;

    ConsoleReset();

    ConsolePrint("Checking cartridge...\n");

    char text[13];
    memcpy(text, header->game_title, 12);
    text[12] = '\0';
    ConsolePrint("Game title: %s\n", text);

    memcpy(text,header->game_code, 4);
    text[4] = '\0';
    ConsolePrint("Game code: %s\n", text);

    memcpy(text,header->maker_code, 2);
    text[2] = '\0';
    ConsolePrint("Maker code: %s\n", text);

    const u8 nintendo_logo[156] = {
        0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21, 0x3D, 0x84, 0x82, 0x0A,
        0x84, 0xE4, 0x09, 0xAD, 0x11, 0x24, 0x8B, 0x98, 0xC0, 0x81, 0x7F, 0x21,
        0xA3, 0x52, 0xBE, 0x19, 0x93, 0x09, 0xCE, 0x20, 0x10, 0x46, 0x4A, 0x4A,
        0xF8, 0x27, 0x31, 0xEC, 0x58, 0xC7, 0xE8, 0x33, 0x82, 0xE3, 0xCE, 0xBF,
        0x85, 0xF4, 0xDF, 0x94, 0xCE, 0x4B, 0x09, 0xC1, 0x94, 0x56, 0x8A, 0xC0,
        0x13, 0x72, 0xA7, 0xFC, 0x9F, 0x84, 0x4D, 0x73, 0xA3, 0xCA, 0x9A, 0x61,
        0x58, 0x97, 0xA3, 0x27, 0xFC, 0x03, 0x98, 0x76, 0x23, 0x1D, 0xC7, 0x61,
        0x03, 0x04, 0xAE, 0x56, 0xBF, 0x38, 0x84, 0x00, 0x40, 0xA7, 0x0E, 0xFD,
        0xFF, 0x52, 0xFE, 0x03, 0x6F, 0x95, 0x30, 0xF1, 0x97, 0xFB, 0xC0, 0x85,
        0x60, 0xD6, 0x80, 0x25, 0xA9, 0x63, 0xBE, 0x03, 0x01, 0x4E, 0x38, 0xE2,
        0xF9, 0xA2, 0x34, 0xFF, 0xBB, 0x3E, 0x03, 0x44, 0x78, 0x00, 0x90, 0xCB,
        0x88, 0x11, 0x3A, 0x94, 0x65, 0xC0, 0x7C, 0x63, 0x87, 0xF0, 0x3C, 0xAF,
        0xD6, 0x25, 0xE4, 0x8B, 0x38, 0x0A, 0xAC, 0x72, 0x21, 0xD4, 0xF8, 0x07
    };

    // 09Ch Bit 2,7 - Debugging Enable
    // This is part of the above Nintendo Logo area, and must be commonly set to
    // 21h, however, Bit 2 and Bit 7 may be set to other values.
    // When both bits are set (ie. A5h), the FIQ/Undefined Instruction handler
    // in the BIOS becomes unlocked, the handler then forwards these exceptions
    // to the user handler in cartridge ROM (entry point defined in 80000B4h,
    // see below).  Other bit combinations currently do not seem to have special
    // functions.

    // 09Eh Bit 0,1 - Cartridge Key Number LSBs  <-- GBATEK SAYS MSBs!!!
    // This is part of the above Nintendo Logo area, and must be commonly set to
    // F8h, however, Bit 0-1 may be set to other values.
    // During startup, the BIOS performs some dummy-reads from a stream of
    // pre-defined addresses, even though these reads seem to be meaningless,
    // they might be intended to unlock a read-protection inside of commercial
    // cartridge. There are 16 pre-defined address streams - selected by a 4bit
    // key number - of which the upper two bits are gained from 800009Eh Bit
    // 0-1, and the lower two bits from a checksum across header bytes
    // 09Dh..0B7h (bytewise XORed, divided by 40h).

    int logo_bad = 0;
    for (int i = 0; i < 156; i++)
        logo_bad |= (nintendo_logo[i] != header->nintendo_logo[i]);
    ConsolePrint("Nintendo Logo... %s\n", (logo_bad == 0) ? "OK" : "BAD [!]");
    if (logo_bad)
    {
        if (EmulatorConfig.debug_msg_enable)
            showconsole = 1;
    }

    u8 debugging_enable = header->nintendo_logo[0x9C - 0x04];
    if (((debugging_enable & (BIT(2) | BIT(7))) == BIT(2))
        || ((debugging_enable & (BIT(2) | BIT(7))) == BIT(7)))
    {
        if (EmulatorConfig.debug_msg_enable)
            showconsole = 1;
        ConsolePrint("[!]Header byte 0x9C = 0x%02X -> Debug (?)\n",
                     debugging_enable);
    }
    else if ((debugging_enable & (BIT(2) | BIT(7))) == (BIT(2) | BIT(7)))
    {
        if (EmulatorConfig.debug_msg_enable)
            showconsole = 1;
        ConsolePrint("[!]Header byte 0x9C = 0x%02X -> Debugging enabled!\n",
                     debugging_enable);
    }

    u8 cartridge_key_number_LSBs = header->nintendo_logo[0x9E - 0x04];
    if (cartridge_key_number_LSBs & (BIT(0) | BIT(1)))
    {
        if (EmulatorConfig.debug_msg_enable)
            showconsole = 1;
        ConsolePrint("[!]Cartridge Key Number LSBs (byte 0x9E) = 0x%02X\n",
                     cartridge_key_number_LSBs);
    }

    ConsolePrint("Fixed 0x96... %s\n",
                 header->fixed_96h == 0x96 ? "OK" : "BAD [!]");
    if (header->fixed_96h != 0x96)
    {
        if (EmulatorConfig.debug_msg_enable)
            showconsole = 1;
    }

    ConsolePrint("Main unit code: 0x%02X\n", header->main_unit_code);

    ConsolePrint("Device type: 0x%02X\n", header->device_type);
    if ((debugging_enable & (BIT(2) | BIT(7))) == (BIT(2) | BIT(7)))
    {
        ConsolePrint("[!]Debug handler is: ");
        // Handler for Undefined instruction, Abort(prefetch), Abort(data),
        // Reserved

        // DACS = Debugging And Communication System
        if (header->device_type & 0x80)
            ConsolePrint("0x9FE2000/1MBIT DACS\n");
        else
            ConsolePrint("0x9FFC000/8MBIT DACS\n");
    }

    ConsolePrint("Software version: 0x%02X\n", header->software_version);

    u8 check = 0;
    for (int i = 0xA0; i <= 0xBC; i++)
        check -= (((u8 *)rom)[i]);
    check -= 0x19;

    ConsolePrint("Complement check: 0x%02X -- Obtained: 0x%02X -- %s\n",
                 header->complement_check, check,
                 header->complement_check == check ? "OK" : "BAD [!]");
    if (header->complement_check != check)
    {
        if (EmulatorConfig.debug_msg_enable)
            showconsole = 1;
    }

    ConsolePrint("Save type: %s", GBA_GetSaveTypeString());
}
