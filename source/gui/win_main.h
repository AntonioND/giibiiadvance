// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef WIN_MAIN__
#define WIN_MAIN__

int Win_MainCreate(char *rom_path); // returns 1 if error
void Win_MainRender(void);
int Win_MainRunningGBA(void);
int Win_MainRunningGB(void);
void Win_MainLoopHandle(void);

void Win_MainChangeZoom(int newzoom);
void Win_MainSetFrameskip(int frameskip); // in win_main.c

// Type: 0 = error, 1 = debug, 2 = console, 3 = sys info
void Win_MainShowMessage(int type, const char *text);

#endif // WIN_MAIN__
