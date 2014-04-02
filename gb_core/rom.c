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

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#include "../build_options.h"
#include "../general_utils.h"
#include "../debug_utils.h"
#include "../file_utils.h"
#include "../config.h"

#include "gameboy.h"
#include "rom.h"
#include "debug.h"
#include "mbc.h"
#include "video.h"
#include "general.h"
#include "licensees.h"
#include "gb_main.h"

/*
Seen:
00 - 01 - 02 - 03 - 06 - 0B - 0D - 10 - 11 - 13 - 19 - 1A - 1B - 1C - 1E - 20 - 22
97 - 99 - BE - EA - FC - FD - FE - FF

Others are bad dumps (in fact, some of those are bad dumps or strange roms...).
*/

//If a cart type is in parenthesis, I haven't seen any game that uses it, but it should use that
//controller. Cartridges with "???" have been seen, but there is not much documentation about it...
static const char * gb_memorycontrollers[256] = {
    "ROM ONLY", "MBC1", "MBC1+RAM", "MBC1+RAM+BATTERY", "Unknown", "(MBC2)", "MBC2+BATTERY", "Unknown",
    "(ROM+RAM) ", "(ROM+RAM+BATTERY)", "Unknown", "MMM01", "(MMM01+RAM)", "MMM01+RAM+BATTERY", "Unknown", "(MBC3+TIMER+BATTERY)",
    "MBC3+TIMER+RAM+BATTERY", "MBC3", "(MBC3+RAM)", "MBC3+RAM+BATTERY", "Unknown", "(MBC4)", "(MBC4+RAM)", "(MBC4+RAM+BATTERY)",
    "Unknown", "MBC5", "MBC5+RAM", "MBC5+RAM+BATTERY", "MBC5+RUMBLE", "(MBC5+RUMBLE+RAM)", "MBC5+RUMBLE+RAM+BATTERY", "Unknown",
    "MBC6+RAM+BATTERY ???", "Unknown", "MBC7+RAM+BATTERY ???", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",

    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",

    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", " ??? ",
    "Unknown", " ??? ", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", " ??? ", "Unknown",

    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", " ??? ", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "CAMERA", "BANDAI TAMA5", "HuC3", "HuC1+RAM+BATTERY"
} ;

static const u8 gb_nintendo_logo[48] = {
    0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,0x03,0x73,0x00,0x83,0x00,0x0C,0x00,0x0D,
    0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,0xDC,0xCC,0x6E,0xE6,0xDD,0xDD,0xD9,0x99,
    0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,0xDD,0xDC,0x99,0x9F,0xBB,0xB9,0x33,0x3E
};

extern _GB_CONTEXT_ GameBoy;

static int showconsole = 0;

int GB_ShowConsoleRequested(void)
{
    int ret = showconsole;
    showconsole = 0;
    return ret;
}

int GB_CartridgeLoad(const u8 * pointer, const u32 rom_size)
{
    showconsole = 0; //if at the end this is 1, show console window

    ConsoleReset();

    _GB_ROM_HEADER_ * GB_Header;

    GB_Header = (void*)pointer;

    ConsolePrint("Checking cartridge...\n");

    int k;
    for(k = 0; k < 11; k++) GameBoy.Emulator.Title[k] = GB_Header->title[k];
    for(; k < 15; k++) GameBoy.Emulator.Title[k] = GB_Header->manufacturer[k-11];
    GameBoy.Emulator.Title[15] = GB_Header->cgb_flag;
    GameBoy.Emulator.Title[16] = '\0';

    if(GB_Header->old_licensee == 0x33) GameBoy.Emulator.Title[12] = '\0';

    ConsolePrint("Game title: %s\n",GameBoy.Emulator.Title);

    char * dest_name;
    switch(GB_Header->dest_code)
    {
        case 0x00: dest_name = "Japan"; break;
        case 0x01: dest_name = "Non-Japan"; break;
        default: dest_name = "Unknown"; break;
    }
    ConsolePrint("Destination: %s (%02X)\n",dest_name,GB_Header->dest_code);

    if(GB_Header->old_licensee == 0x33)
    {
        char byte1 = isprint(GB_Header->new_licensee[0]) ? GB_Header->new_licensee[0] : '.';
        char byte2 = isprint(GB_Header->new_licensee[1]) ? GB_Header->new_licensee[1] : '.';
        ConsolePrint("Licensee (new): %s (%c%c)\n",
                GB_GetLicenseeName(GB_Header->new_licensee[0],GB_Header->new_licensee[1]),
                byte1,byte2);
    }
    else
    {
        char byte1 = ((GB_Header->old_licensee >> 4) & 0x0F);
        byte1 += (byte1 < 10) ? 0x30 : (0x41 - 10);
        char byte2 = (GB_Header->old_licensee & 0x0F);
        byte2 += (byte2 < 10) ? 0x30 : (0x41 - 10);
        ConsolePrint("Licensee (old): %s (%02X)\n",
                GB_GetLicenseeName(byte1,byte2),GB_Header->old_licensee);
    }

    ConsolePrint("Rom version: %02X\n",GB_Header->rom_version);

    //THIS WILL CHOOSE THE POSSIBLE GB SYSTEMS ABLE TO RUN THE GAME
    //-------------------------------------------------------------

    ConsolePrint("GBC flag = %02X\n", GB_Header->cgb_flag);

    int enable_gb = 0;
    int enable_sgb = 0;
    int enable_gbc = 0;

    //COLOR
    if(GB_Header->cgb_flag & (1<<7))
    {
        if(GB_Header->cgb_flag == 0xC0)//GBC only
        {
            enable_gbc = 1;
        }
        else if(GB_Header->cgb_flag == 0x80) //Can use a normal GB too
        {
            enable_gbc = 1;
            enable_gb = 1;
        }
        else //Unknown
        {
            enable_gb = 1;
            enable_gbc = 1;

            ConsolePrint("[!]Unknown GBC flag...\n");
            if(EmulatorConfig.debug_msg_enable) showconsole = 1;
        }
    }
    else //GB
    {
        enable_gb = 1;
    }

    //SGB
    if(GB_Header->sgb_flag == 0x03 && GB_Header->old_licensee == 0x33)
    {
        enable_sgb = 1;
    }

    GameBoy.Emulator.selected_hardware = EmulatorConfig.hardware_type;

    if(GameBoy.Emulator.selected_hardware == -1) //AUTO
    {
        if(enable_gbc == 1) GameBoy.Emulator.HardwareType = HW_GBC;
        else if(enable_sgb == 1) GameBoy.Emulator.HardwareType = HW_SGB;
        else if(enable_gb == 1) GameBoy.Emulator.HardwareType = HW_GB;
        else GameBoy.Emulator.HardwareType = HW_GBC; // ?
    }
    else GameBoy.Emulator.HardwareType = GameBoy.Emulator.selected_hardware; //FORCE MODE

    switch(GameBoy.Emulator.HardwareType)
    {
        case HW_GB:
            ConsolePrint("Loading in GB mode...\n");
            break;
        case HW_GBP:
            ConsolePrint("Loading in GBP mode...\n");
            break;
        case HW_GBC:
            ConsolePrint("Loading in GBC mode...\n");
            break;
        case HW_GBA:
            ConsolePrint("Loading in GBA mode...\n");
            break;
        case HW_SGB:
            ConsolePrint("Loading in SGB mode...\n");
            break;
        case HW_SGB2:
            ConsolePrint("Loading in SGB2 mode...\n");
            break;
        default: //Should never happen
            Debug_ErrorMsgArg("GB_CartridgeLoad(): Trying to load in an undefined mode, should never happen.");
            return 0;
            break;
    }

    GameBoy.Emulator.gbc_in_gb_mode = 0;

    if( (GameBoy.Emulator.HardwareType == HW_GB) || (GameBoy.Emulator.HardwareType == HW_GBP) )
    {
        //Video_EnableBlur(true);
        GameBoy.Emulator.CGBEnabled = 0;
        GameBoy.Emulator.SGBEnabled = 0;
        GameBoy.Emulator.DrawScanlineFn = &GB_ScreenDrawScanline;
    }
    else if( (GameBoy.Emulator.HardwareType == HW_SGB) || (GameBoy.Emulator.HardwareType == HW_SGB2) )
    {
        //Video_EnableBlur(false);
        GameBoy.Emulator.CGBEnabled = 0;
        GameBoy.Emulator.SGBEnabled = 1;
        GameBoy.Emulator.DrawScanlineFn = &SGB_ScreenDrawScanline;
    }
    else if(GameBoy.Emulator.HardwareType == HW_GBC)
    {
        //Video_EnableBlur(true);
        GameBoy.Emulator.CGBEnabled = 1;
        GameBoy.Emulator.SGBEnabled = 0;
        GameBoy.Emulator.DrawScanlineFn = &GBC_ScreenDrawScanline;
    }
    else if(GameBoy.Emulator.HardwareType == HW_GBA)
    {
        //Video_EnableBlur(false);
        GameBoy.Emulator.CGBEnabled = 1;
        GameBoy.Emulator.SGBEnabled = 0;
        GameBoy.Emulator.DrawScanlineFn = &GBC_ScreenDrawScanline;
    }

    GameBoy.Emulator.enable_boot_rom = 0;
    GameBoy.Emulator.boot_rom_loaded = 0;

    if(EmulatorConfig.load_from_boot_rom)
    {
        //Load boot rom if any...
        char * boot_rom_filename = NULL;
        switch(GameBoy.Emulator.HardwareType)
        {
            case HW_GB: boot_rom_filename = DMG_ROM_FILENAME; break;
            case HW_GBP: boot_rom_filename = MGB_ROM_FILENAME; break;
            case HW_SGB: boot_rom_filename = SGB_ROM_FILENAME; break;
            case HW_SGB2: boot_rom_filename = SGB2_ROM_FILENAME; break;
            case HW_GBC: boot_rom_filename = CGB_ROM_FILENAME; break;
            case HW_GBA: boot_rom_filename = AGB_ROM_FILENAME; break;
        }
        if(boot_rom_filename != NULL)
        {
            if(DirGetBiosFolderPath())
            {
                char * completepath = malloc(strlen(DirGetBiosFolderPath())
                                            +strlen(boot_rom_filename)+2);
                sprintf(completepath,"%s%s",DirGetBiosFolderPath(),boot_rom_filename);
                FILE * test = fopen(completepath,"rb");
                if(test)
                {
                    fclose(test);

                    FileLoad(completepath,(void*)&GameBoy.Emulator.boot_rom,NULL);

                    if(GameBoy.Emulator.boot_rom)
                    {
                        ConsolePrint("Boot ROM loaded from %s!\n",boot_rom_filename);
                        GameBoy.Emulator.enable_boot_rom = 1;
                        GameBoy.Emulator.boot_rom_loaded = 1;
                    }
                }
                free(completepath);
            }
        }
    }

    GameBoy.Emulator.HasBattery = 0;
    GameBoy.Emulator.HasTimer = 0;

    ConsolePrint("Cartridge type: %02X - %s\n",GB_Header->cartridge_type,gb_memorycontrollers[GB_Header->cartridge_type]);

    GameBoy.Emulator.EnableBank0Switch = 0;
    GameBoy.Memory.mbc_mode = 0;

    //    CARTRIDGE TYPE
    switch(GB_Header->cartridge_type)
    {
        case 0x00: //ROM ONLY
            GameBoy.Emulator.MemoryController = MEM_NONE;
            break;
        case 0x01: //MBC1
            GameBoy.Emulator.MemoryController = MEM_MBC1;
            break;
        case 0x02: //MBC1+RAM
            GameBoy.Emulator.MemoryController = MEM_MBC1;
            break;
        case 0x03: //MBC1+RAM+BATTERY
            GameBoy.Emulator.MemoryController = MEM_MBC1;
            GameBoy.Emulator.HasBattery = 1;
            break;
        case 0x05: //MBC2
            GameBoy.Emulator.MemoryController = MEM_MBC2;
            break;
        case 0x06: //MBC2+BATTERY
            GameBoy.Emulator.MemoryController = MEM_MBC2;
            GameBoy.Emulator.HasBattery = 1;
            break;
        case 0x08: //ROM+RAM
            GameBoy.Emulator.MemoryController = MEM_NONE;
            break;
        case 0x09: //ROM+RAM+BATTERY
            GameBoy.Emulator.MemoryController = MEM_NONE;
            break;
        case 0x0B: //MMM01
            GameBoy.Emulator.MemoryController = MEM_MMM01;
            GameBoy.Emulator.EnableBank0Switch = 1;
            break;
        case 0x0C: //MMM01+RAM //I've never seen a game that uses this...
            GameBoy.Emulator.MemoryController = MEM_MMM01;
            GameBoy.Emulator.EnableBank0Switch = 1;
            break;
        case 0x0D: //MMM01+RAM+BATTERY
            GameBoy.Emulator.MemoryController = MEM_MMM01;
            GameBoy.Emulator.HasBattery = 1;
            GameBoy.Emulator.EnableBank0Switch = 1;
            break;
        case 0x0F: //MBC3+TIMER+BATTERY
            GameBoy.Emulator.MemoryController = MEM_MBC3;
            GameBoy.Emulator.HasBattery = 1;
            GameBoy.Emulator.HasTimer = 1;
            break;
        case 0x10: //MBC3+TIMER+RAM+BATTERY
            GameBoy.Emulator.MemoryController = MEM_MBC3;
            GameBoy.Emulator.HasBattery = 1;
            GameBoy.Emulator.HasTimer = 1;
            break;
        case 0x11: //MBC3
            GameBoy.Emulator.MemoryController = MEM_MBC3;
            break;
        case 0x12: //MBC3+RAM
            GameBoy.Emulator.MemoryController = MEM_MBC3;
            break;
        case 0x13: //MBC3+RAM+BATTERY
            GameBoy.Emulator.MemoryController = MEM_MBC3;
            GameBoy.Emulator.HasBattery = 1;
            break;
    /*    case 0x15: //MBC4
            GameBoy.Emulator.MemoryController = MEM_MBC4;
            break;
        case 0x16: //MBC4+RAM          //I've never seen a game that uses MBC4...
            GameBoy.Emulator.MemoryController = MEM_MBC4;
            break;
        case 0x17: //MBC4+RAM+BATTERY
            GameBoy.Emulator.MemoryController = MEM_MBC4;
            GameBoy.Emulator.HasBattery = 1;
            break;
    */    case 0x19: //MBC5
            GameBoy.Emulator.MemoryController = MEM_MBC5;
            break;
        case 0x1A: //MBC5+RAM
            GameBoy.Emulator.MemoryController = MEM_MBC5;
            break;
        case 0x1B: //MBC5+RAM+BATTERY
            GameBoy.Emulator.MemoryController = MEM_MBC5;
            GameBoy.Emulator.HasBattery = 1;
            break;
        case 0x1C: //MBC5+RUMBLE
            GameBoy.Emulator.MemoryController = MEM_RUMBLE;
            break;
        case 0x1D: //MBC5+RUMBLE+RAM
            GameBoy.Emulator.MemoryController = MEM_RUMBLE;
            break;
        case 0x1E: //MBC5+RUMBLE+RAM+BATTERY
            GameBoy.Emulator.MemoryController = MEM_RUMBLE;
            GameBoy.Emulator.HasBattery = 1;
            break;
        case 0x20: //MBC6+RAM+BATTERY ???
            GameBoy.Emulator.MemoryController = MEM_MBC6;
            GameBoy.Emulator.HasBattery = 1;
            break;
        case 0x22: //MBC7+RAM+BATTERY ???
            GameBoy.Emulator.MemoryController = MEM_MBC7;
            GameBoy.Emulator.HasBattery = 1;
            break;
        case 0xFC: //POCKET CAMERA
            GameBoy.Emulator.MemoryController = MEM_CAMERA;
            GameBoy.Emulator.HasBattery = 1;
            break;
/*
        case 0xFD: //BANDAI TAMA5
            GameBoy.Emulator.MemoryController = MEM_TAMA5;
            break;
        case 0xFE: //HuC3
            GameBoy.Emulator.MemoryController = MEM_HUC3;
            break;
*/
        case 0xFF: //HuC1+RAM+BATTERY (MBC1-like + IR PORT)
            GameBoy.Emulator.MemoryController = MEM_HUC1;
            GameBoy.Emulator.HasBattery = 1;
            break;
        default:
            ConsolePrint("[!]UNSUPPORTED CARTRIDGE\n");
            showconsole = 1;
            return 0;
    }

    GB_MapperSet(GameBoy.Emulator.MemoryController);

    //     RAM
    switch(GB_Header->ram_size)
    {
        case 0x00: GameBoy.Emulator.RAM_Banks = 0; break;
        case 0x01: GameBoy.Emulator.RAM_Banks = 1; break; //2KB
        case 0x02: GameBoy.Emulator.RAM_Banks = 1; break; //8KB
        case 0x03: GameBoy.Emulator.RAM_Banks = 4; break; //4 * 8KB
        case 0x04: GameBoy.Emulator.RAM_Banks = 16; break; //16 * 8KB
        default:
             ConsolePrint("[!]RAM SIZE UNKNOWN: %02X\n",GB_Header->ram_size);
             showconsole = 1;
             return 0;
    }

    if(GameBoy.Emulator.MemoryController == MEM_MBC2)
        GameBoy.Emulator.RAM_Banks = 1; //512 * 4 bits

    if(GameBoy.Emulator.MemoryController == MEM_MBC7)
        GameBoy.Emulator.RAM_Banks = 1;

    ConsolePrint("RAM size %02X -- %d banks\n",GB_Header->ram_size,GameBoy.Emulator.RAM_Banks);

    //      ROM
    switch(GB_Header->rom_size)
    {
        case 0x00: GameBoy.Emulator.ROM_Banks = 2; break;
        case 0x01: GameBoy.Emulator.ROM_Banks = 4; break;
        case 0x02: GameBoy.Emulator.ROM_Banks = 8; break;
        case 0x03: GameBoy.Emulator.ROM_Banks = 16; break;
        case 0x04: GameBoy.Emulator.ROM_Banks = 32; break;
        case 0x05: GameBoy.Emulator.ROM_Banks = 64; break;
        case 0x06: GameBoy.Emulator.ROM_Banks = 128; break;
        case 0x07: GameBoy.Emulator.ROM_Banks = 256; break;
        case 0x08: GameBoy.Emulator.ROM_Banks = 512; break;
    //    case 0x52: GameBoy.Emulator.ROM_Banks = 72; break; // After the general checksum, this
    //    case 0x53: GameBoy.Emulator.ROM_Banks = 80; break; // is changed to 128 to avoid
    //    case 0x54: GameBoy.Emulator.ROM_Banks = 96; break; // problems...
    //    I've never seen a game with 0x52, 0x53 or 0x54 in its header.
        default:
             ConsolePrint("[!]ROM SIZE UNKNOWN: %02X\n",GB_Header->rom_size);
             showconsole = 1;
             return 0;
    }

    ConsolePrint("ROM size %02X -- %d banks\n",GB_Header->rom_size,GameBoy.Emulator.ROM_Banks);

    if(rom_size != (GameBoy.Emulator.ROM_Banks * 16 * (1024)))
    {
        if(EmulatorConfig.debug_msg_enable) showconsole = 1;

        ConsolePrint("[!]ROM file size incorrect!\nFile size is %d B (%d KB), header says it is %d KB.\n",
                    rom_size,rom_size/1024,(GameBoy.Emulator.ROM_Banks * 16));

        if(rom_size < (GameBoy.Emulator.ROM_Banks * 16 * (1024)))
        {
            ConsolePrint("[!]File is smaller than the size the header says.\nAborting...");
            showconsole = 1;
            return 0;
        }
    }

    //             Checksums...

    //Header...
    u32 sum = 0;
    u32 count;
    for(count = 0x0134; count <= 0x014C; count ++)
        sum = sum - pointer[count] - 1;

    sum &= 0xFF;

    ConsolePrint("Header checksum: %02X - Obtained: %02X\n", GB_Header->header_checksum, sum);

    if(GB_Header->header_checksum != sum)
    {
        ConsolePrint("[!]INCORRECT! - Maybe a bad dump?\n[!]Game wouldn't work in a real GB.\n");
        if(EmulatorConfig.debug_msg_enable) showconsole = 1;
    }

    //Global...
    u32 size = GameBoy.Emulator.ROM_Banks * 16 * 1024;
    sum = 0;
    for(count = 0; count < size; count ++)
        sum += (u32)pointer[count];

    sum -= GB_Header->global_checksum & 0xFF; //Checksum bytes not included
    sum -= (GB_Header->global_checksum>>8) & 0xFF;

    sum &= 0xFFFF;
    sum = ((sum>>8)&0x00FF) | ((sum<<8)&0xFF00);

    ConsolePrint("Global checksum: %04X - Obtained: %04X\n", GB_Header->global_checksum, sum);

    if(GB_Header->global_checksum != sum)
    {
        ConsolePrint("[!]INCORRECT! - Maybe a bad dump?\n");
        if(EmulatorConfig.debug_msg_enable) showconsole = 1;
    }

    ConsolePrint("Checking Nintendo logo... ");

    if(memcmp(GB_Header->nintendologo,gb_nintendo_logo,sizeof(gb_nintendo_logo)) == 0)
        ConsolePrint("Correct!\n");
    else
    {
        ConsolePrint("\n[!]INCORRECT! - Maybe a bad dump?\n[!]Game wouldn't work in a real GB.\n");
        if(EmulatorConfig.debug_msg_enable) showconsole = 1;
    }

    //Fix some rom sizes...
//    if(GameBoy.Emulator.ROM_Banks == 72 || GameBoy.Emulator.ROM_Banks == 80 || GameBoy.Emulator.ROM_Banks == 96)
//        GameBoy.Emulator.ROM_Banks = 128;

    //  SAVE LOCATION...
    GameBoy.Emulator.Rom_Pointer = (void*)GB_Header;

    // ------- ALLOCATE MEMORY -----------

    ConsolePrint("Loading... ");

    _GB_MEMORY_ * mem = &GameBoy.Memory;

    mem->ROM_Base = (void*)GB_Header;

    int i;
    for(i=0;i<512;i++) mem->ROM_Switch[i] = NULL;
    for(i=0;i<GameBoy.Emulator.ROM_Banks;i++) mem->ROM_Switch[i] = (u8*)GB_Header + (16 * 1024 * i);

    memset(mem->VideoRAM,0, 0x4000);

    for(i=0;i<16;i++) mem->ExternRAM[i] = NULL; //Reset pointers
    for(i=0;i<GameBoy.Emulator.RAM_Banks;i++)
    {
        mem->ExternRAM[i] = malloc(8 * 1024);
        memset_rand(mem->ExternRAM[i], 8 * 1024);
    }

    memset_rand(mem->WorkRAM , 0x1000);

    if(GameBoy.Emulator.CGBEnabled == 1)
    {
        for(i=0;i<7;i++)
        {
            mem->WorkRAM_Switch[i] = malloc(4 * 1024);
            memset_rand(mem->WorkRAM_Switch[i] , 4 * 1024);
        }
        memset_rand(mem->StrangeRAM, 0x30);
    }
    else
    {
        for(i=0;i<7;i++) mem->WorkRAM_Switch[i] = NULL;
        mem->WorkRAM_Switch[0] = malloc(4 * 1024);
        memset_rand(mem->WorkRAM_Switch[0], 4 * 1024);
    }

    memset_rand(mem->ObjAttrMem,0xA0);
    memset(mem->IO_Ports,0,0x80);
    memset(mem->HighRAM,0,0x80);

    mem->InterruptMasterEnable = 0;

    mem->selected_rom = 1;
    mem->selected_ram = mem->selected_wram = mem->selected_vram = 0;
    mem->mbc_mode = 0;
    GameBoy.Emulator.rumble = 0;
    GameBoy.Emulator.MBC7.sensorX = GameBoy.Emulator.MBC7.sensorY = 2047;

    mem->VideoRAM_Curr = mem->VideoRAM;
    mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
    mem->RAM_Curr = mem->ExternRAM[0];
    mem->WorkRAM_Curr = mem->WorkRAM_Switch[0];

    if(GameBoy.Emulator.HardwareType == HW_GB)
        GB_ConfigLoadPalette();
    else if(GameBoy.Emulator.HardwareType == HW_GBP)
        GB_SetPalette(0xFF,0xFF,0xFF);

    ConsolePrint("Done!\n");

    return 1;
}

void GB_Cartridge_Unload(void)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    if(GameBoy.Emulator.boot_rom_loaded)
    {
        free(GameBoy.Emulator.boot_rom);
        GameBoy.Emulator.boot_rom = NULL;
        GameBoy.Emulator.boot_rom_loaded = 0;
        GameBoy.Emulator.enable_boot_rom = 0;
    }

    int i;
    for(i=0;i<GameBoy.Emulator.RAM_Banks;i++) free(mem->ExternRAM[i]);

    if(GameBoy.Emulator.CGBEnabled == 1)
    {
        for(i=0;i<7;i++) free(mem->WorkRAM_Switch[i]);
    }
    else
    {
        free(mem->WorkRAM_Switch[0]);
    }

    free(GameBoy.Emulator.Rom_Pointer);
}

void GB_Cardridge_Set_Filename(char * filename)
{
    u32 len = strlen(filename);

    if(GameBoy.Emulator.save_filename)
    {
        s_strncpy(GameBoy.Emulator.save_filename,filename,sizeof(GameBoy.Emulator.save_filename));
    }

    len --;

    while(1)
    {
        if(GameBoy.Emulator.save_filename[len] == '.')
        {
            GameBoy.Emulator.save_filename[len] = '\0';
            return;
        }

        if(--len == 0) break;
    }
}

//--------------------------------------------------------------------------

void GB_SRAM_Save(void)
{
    if(GameBoy.Emulator.RAM_Banks == 0 || GameBoy.Emulator.HasBattery == 0) return;

    char * name = malloc(strlen(GameBoy.Emulator.save_filename) + 5);
    sprintf(name,"%s.sav",GameBoy.Emulator.save_filename);

    FILE * savefile = fopen (name,"wb+");

    if(!savefile) Debug_ErrorMsgArg("Couldn't save SRAM.");

    if(GameBoy.Emulator.MemoryController == MEM_MBC2) //512 * 4 bits
    {
        fwrite(GameBoy.Memory.ExternRAM[0], 1, 512, savefile);
    }
/*    else if(((_GB_ROM_HEADER_*)GameBoy.Emulator.Rom_Pointer)->ram_size == 1) //2 kb
    {
        fwrite(GameBoy.Memory.ExternRAM[0], 1, 2 * 1024, savefile);
    }
*/    else //Complete banks
    {
        u32 a;
        for(a = 0; a < GameBoy.Emulator.RAM_Banks; a++)
        {
            fwrite(GameBoy.Memory.ExternRAM[a], 1, 8 * 1024, savefile);
        }
    }

    fclose(savefile);
}

void GB_SRAM_Load(void)
{
    if(GameBoy.Emulator.RAM_Banks == 0 || GameBoy.Emulator.HasBattery == 0) return;

    char * name = malloc(strlen(GameBoy.Emulator.save_filename) + 5);
    sprintf(name,"%s.sav",GameBoy.Emulator.save_filename);

    FILE * savefile = fopen (name,"rb");

    if(!savefile) // No save file...
    {
        free(name);
        return;
    }

    ConsolePrint("Loading SRAM... ");

    if(GameBoy.Emulator.MemoryController == MEM_MBC2) //512 * 4 bits
    {
        int n = fread(GameBoy.Memory.ExternRAM[0], 1, 512, savefile);

        if(n != 512) ConsolePrint("Error while reading SRAM: %d bytes read",n);
    }
/*    else if(((_GB_ROM_HEADER_*)GameBoy.Emulator.Rom_Pointer)->ram_size == 1) //2 kb
    {
        fread(GameBoy.Memory.ExternRAM[0], 1, 2 * 1024, savefile);
    }
*/    else //Complete banks
    {
        u32 a;
        for(a = 0; a < GameBoy.Emulator.RAM_Banks; a++)
        {
            int n = fread(GameBoy.Memory.ExternRAM[a], 1, 8 * 1024, savefile);
            if(n != 8 * 1024) ConsolePrint("Error while reading SRAM bank %d: %d bytes read",a,n);
        }
    }

    fclose(savefile);
    free(name);

    ConsolePrint("Done!\n");
}

//--------------------------------------------------------------------------

void GB_RTC_Save(void)
{
    if(GameBoy.Emulator.HasTimer == 0) return;

    char * name = malloc(strlen(GameBoy.Emulator.save_filename) + 5);
    sprintf(name,"%s.rtc",GameBoy.Emulator.save_filename);

    time_t current_time = time(NULL);

    FILE * savefile = fopen (name,"wb+");

    if(!savefile)
    {
        Debug_ErrorMsgArg("Couldn't save rtc data.");
        free(name);
        return;
    }

    fwrite(&GameBoy.Emulator.Timer, 1, sizeof(_GB_MB3_TIMER_), savefile);
    fwrite(&current_time, 1, sizeof(time_t), savefile);

    fclose(savefile);
    free(name);
}

void GB_RTC_Load(void)
{
    if(GameBoy.Emulator.HasTimer == 0) return;

    char * name = malloc(strlen(GameBoy.Emulator.save_filename) + 5);
    sprintf(name,"%s.rtc",GameBoy.Emulator.save_filename);

    time_t current_time = time(NULL);
    time_t old_time;

    FILE * savefile = fopen (name,"rb");

    if(!savefile) return; // No save file...

    ConsolePrint("Loading RTC data... ");

    int n = fread(&GameBoy.Emulator.Timer, 1, sizeof(_GB_MB3_TIMER_), savefile);
    if(n != sizeof(_GB_MB3_TIMER_))
        ConsolePrint("Error while loading MBC3 data: %d bytes read",n);

    n = fread(&old_time, 1, sizeof(time_t), savefile);
    if(n != sizeof(_GB_MB3_TIMER_))
        ConsolePrint("Error while loading RTC data: %d bytes read",n);

    fclose(savefile);

    if(GameBoy.Emulator.Timer.halt == 1) return; //Nothing else to do...

    time_t time_increased = current_time - old_time;

    GameBoy.Emulator.Timer.sec += time_increased % 60;
    if(GameBoy.Emulator.Timer.sec > 59)
    {
        GameBoy.Emulator.Timer.sec -= 60;
        time_increased += 60;
    }

    GameBoy.Emulator.Timer.min += (time_increased / 60) % 60;
    if(GameBoy.Emulator.Timer.min > 59)
    {
        GameBoy.Emulator.Timer.min -= 60;
        time_increased += 3600;
    }

    GameBoy.Emulator.Timer.hour += (time_increased / 3600) % 24;
    if(GameBoy.Emulator.Timer.hour > 23)
    {
        GameBoy.Emulator.Timer.hour -= 24;
        time_increased += 3600 * 24;
    }

    GameBoy.Emulator.Timer.days += (time_increased / (3600 * 24));
    while(GameBoy.Emulator.Timer.days > 511)
    {
        GameBoy.Emulator.Timer.days &= 511;
        GameBoy.Emulator.Timer.carry = 1;
    }

    ConsolePrint("Done!\n");
}


