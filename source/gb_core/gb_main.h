// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_GB_MAIN__
#define GB_GB_MAIN__

void GB_Input_Update(void);

int GB_ROMLoad(const char *rom_path);
void GB_End(int save);

int GB_Screen_Init(void);

void GB_RunForOneFrame(void);

int GB_IsEnabledSGB(void);

void GB_InputSet(int player, int a, int b, int st, int se,
                 int r, int l, int u, int d);
void GB_InputSetMBC7(int x, int y); // -200 to 200
void GB_InputSetMBC7Buttons(int up, int down, int right, int left);

int GB_Input_Get(int player);

#endif // GB_GB_MAIN__
