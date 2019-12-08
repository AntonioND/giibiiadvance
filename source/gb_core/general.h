// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_GENERAL__
#define GB_GENERAL__

void GB_PowerOn(void);  // This function doesn't allocate anything
void GB_PowerOff(void); // This function doesn't free anything

void GB_HardReset(void);

int GB_EmulatorIsEnabledSGB(void);

int GB_RumbleEnabled(void);

#endif // GB_GENERAL__
