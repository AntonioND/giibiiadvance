// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef WIN_GBA_DEBUGGER__
#define WIN_GBA_DEBUGGER__

// win_gba_disassembler.c
// ----------------------

void Win_GBADisassemblerStartAddressSetDefault(void);
void Win_GBADisassemblerUpdate(void);
int Win_GBADisassemblerCreate(void); // Returns 1 on error
void Win_GBADisassemblerSetFocus(void);
void Win_GBADisassemblerClose(void);

// win_gba_memviewer.c
// -------------------

int Win_GBAMemViewerCreate(void); // Returns 1 on error
void Win_GBAMemViewerUpdate(void);
void Win_GBAMemViewerRender(void);
void Win_GBAMemViewerClose(void);

// win_gba_ioviewer.c
// ------------------

int Win_GBAIOViewerCreate(void); // Returns 1 on error
void Win_GBAIOViewerUpdate(void);
void Win_GBAIOViewerRender(void);
void Win_GBAIOViewerClose(void);

// win_gba_tileviewer.c
// --------------------

int Win_GBATileViewerCreate(void); // Returns 1 on error
void Win_GBATileViewerUpdate(void);
void Win_GBATileViewerClose(void);

// win_gba_mapviewer.c
// -------------------

int Win_GBAMapViewerCreate(void); // Returns 1 on error
void Win_GBAMapViewerUpdate(void);
void Win_GBAMapViewerClose(void);

// win_gba_sprviewer.c
// -------------------

int Win_GBASprViewerCreate(void); // Returns 1 on error
void Win_GBASprViewerUpdate(void);
void Win_GBASprViewerClose(void);

// win_gba_palviewer.c
// -------------------

int Win_GBAPalViewerCreate(void); // Returns 1 on error
void Win_GBAPalViewerUpdate(void);
void Win_GBAPalViewerClose(void);

#endif // WIN_GBA_DEBUGGER__
