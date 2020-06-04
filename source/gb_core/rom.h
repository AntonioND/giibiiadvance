// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_ROM__
#define GB_ROM__

#include "gameboy.h"

#pragma pack(push, 1)
typedef struct
{
    u8 padding[0x100];
    u8 entrypoint[4]; // Usually NOP + JP nn
    u8 nintendologo[0x30];
    // CE ED 66 66 CC 0D 00 0B 03 73 00 83 00 0C 00 0D
    // 00 08 11 1F 88 89 00 0E DC CC 6E E6 DD DD D9 99
    // BB BB 67 63 6E 0E EC CC DD DC 99 9F BB B9 33 3E

    u8 title[0xB];
    u8 manufacturer[4]; // In older games, manufacturer and cgb_flag are part
    u8 cgb_flag;        // of the title.
    // 80h - Game supports CGB functions, but works on old gameboys also.
    // C0h - Game works on CGB only (physically the same as 80h).

    u8 new_licensee[2]; // For games newer than SGB
    u8 sgb_flag;
    // 00h = No SGB functions (Normal Gameboy or CGB only game)
    // 03h = Game supports SGB functions

    u8 cartridge_type;

    u8 rom_size; // The actual ROM size is "32KB << N"

    u8 ram_size;
    // When using a MBC2 chip 00h must be specified in this entry, even though
    // the MBC2 includes a built-in RAM of 512 x 4 bits.

    u8 dest_code;
    // 00h - Japanese
    // 01h - Non-Japanese

    u8 old_licensee;
    // 33h signalizes that the New License Code in header bytes 0144-0145
    // [new_licensee] (Super GameBoy functions won't work if <> $33).

    u8 rom_version; // Usually 00h
    u8 header_checksum;
    // The checksum is calculated as follows:
    //    x=0:FOR i=0134h TO 014Ch:x=x-MEM[i]-1:NEXT
    // The lower 8 bits of the result must be the same than the value in this
    // entry. The game won't work if this checksum is incorrect.

    u16 global_checksum;
    // Contains a 16 bit checksum (upper byte first) across the whole cartridge
    // ROM. Produced by adding all bytes of the cartridge (except for the two
    // checksum bytes). The Gameboy doesn't verify this checksum.

} _GB_ROM_HEADER_;
#pragma pack(pop)

int GB_ShowConsoleRequested(void);
int GB_CartridgeLoad(const u8 *pointer, const u32 rom_size);
void GB_Cartridge_Unload(void);

void GB_Cardridge_Set_Filename(const char *filename);

void GB_SRAM_Save(void);
void GB_SRAM_Load(void);

#endif // GB_ROM__
