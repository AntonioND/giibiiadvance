// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef BUILD_OPTIONS__
#define BUILD_OPTIONS__

// Changing this file will force to recompile everything.

#ifndef MAX_PATHLEN
#define MAX_PATHLEN (2048)
#endif

#define ARRAY_NUM_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

//---------------------------------------------------------------
//----------------------- GENERAL DEFINES -----------------------
//---------------------------------------------------------------

#define GIIBIIADVANCE_VERSION_MAJOR (0)
#define GIIBIIADVANCE_VERSION_MINOR (2)
#define GIIBIIADVANCE_VERSION_PATCH (0)
#define GIIBIIADVANCE_VERSION ((GIIBIIADVANCE_VERSION_MAJOR<<16) | (GIIBIIADVANCE_VERSION_MINOR<<8) | \
                             (GIIBIIADVANCE_VERSION_PATCH))

#define GIIBIIADVANCE_VERSION_STRING "0.2.X"

#define GIIBIIADVANCE_COPYRIGHT_STRING "Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)"
#define GIIBIIADVANCE_COPYRIGHT_STRING_ASCII "Copyright (C) 2011-2015 Antonio Ni" STR_NTILDE_MINUS "o D" \
                                                STR_IACUTE_MINUS "az (AntonioND)"

//---------------------------------------------------------------
//----------------------- EMULATOR DEFINES ----------------------
//---------------------------------------------------------------

#define BIOS_FOLDER "bios"
#define SCREENSHOT_FOLDER "screenshots"

//-----------------------------------------
//----------- GAMEBOY EMULATION -----------
//-----------------------------------------

//#define VRAM_MEM_CHECKING // Don't even try to enable this...

#define DMG_ROM_FILENAME "dmg_rom.bin"
#define MGB_ROM_FILENAME "mgb_rom.bin"
#define SGB_ROM_FILENAME "sgb_rom.bin"
#define SGB2_ROM_FILENAME "sgb2_rom.bin"
#define CGB_ROM_FILENAME "cgb_rom.bin"
#define AGB_ROM_FILENAME "agb_rom.bin"
#define AGS_ROM_FILENAME "ags_rom.bin"

//-------------------------------------------------
//----------- GAMEBOY ADVANCE EMULATION -----------
//-------------------------------------------------

#define GBA_BIOS_FILENAME "gba_bios.bin"

//---------------------------------------------------------------------

#endif // BUILD_OPTIONS__
