// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __WIN_GB_DEBUGGER__
#define __WIN_GB_DEBUGGER__

//win_gb_disassembler.c
//---------------------

void Win_GBDisassemblerStartAddressSetDefault(void);
void Win_GBDisassemblerUpdate(void);
int Win_GBDisassemblerCreate(void); // returns 1 if error
void Win_GBDisassemblerSetFocus(void);
void Win_GBDisassemblerClose(void);

//win_gb_memviewer.c
//------------------

int Win_GBMemViewerCreate(void); // returns 1 if error
void Win_GBMemViewerUpdate(void);
void Win_GBMemViewerRender(void);
void Win_GBMemViewerClose(void);

//win_gb_ioviewer.c
//-----------------

int Win_GBIOViewerCreate(void); // returns 1 if error
void Win_GBIOViewerUpdate(void);
void Win_GBIOViewerRender(void);
void Win_GBIOViewerClose(void);

//win_gb_tileviewer.c
//-------------------

int Win_GBTileViewerCreate(void); // returns 1 if error
void Win_GBTileViewerUpdate(void);
void Win_GBTileViewerClose(void);

//win_gb_mapviewer.c
//------------------

int Win_GBMapViewerCreate(void); // returns 1 if error
void Win_GBMapViewerUpdate(void);
void Win_GBMapViewerClose(void);

//win_gb_sprviewer.c
//------------------

int Win_GBSprViewerCreate(void); // returns 1 if error
void Win_GBSprViewerUpdate(void);
void Win_GBSprViewerClose(void);

//win_gb_palviewer.c
//------------------

int Win_GBPalViewerCreate(void); // returns 1 if error
void Win_GBPalViewerUpdate(void);
void Win_GBPalViewerClose(void);

//win_gb_sgbviewer.c
//------------------

int Win_GB_SGBViewerCreate(void); // returns 1 if error
void Win_GB_SGBViewerUpdate(void);
void Win_GB_SGBViewerClose(void);

//win_gb_gbcamviewer.c
//--------------------

int Win_GB_GBCameraViewerCreate(void); // returns 1 if error
void Win_GB_GBCameraViewerUpdate(void);
void Win_GB_GBCameraViewerClose(void);

#endif // __WIN_GB_DEBUGGER__



