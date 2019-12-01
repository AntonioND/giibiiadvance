// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GBA_BIOS__
#define GBA_BIOS__

#include "gba.h"

void GBA_BiosLoaded(int loaded);
int GBA_BiosIsLoaded(void);

void GBA_BiosEmulatedLoad(void);

void GBA_Swi(u8 number);

#endif // GBA_BIOS__
