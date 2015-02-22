/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

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



