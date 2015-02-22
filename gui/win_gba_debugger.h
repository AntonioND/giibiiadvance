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

#ifndef __WIN_GBA_DEBUGGER__
#define __WIN_GBA_DEBUGGER__

//win_gba_disassembler.c
//----------------------

void Win_GBADisassemblerStartAddressSetDefault(void);
void Win_GBADisassemblerUpdate(void);
int Win_GBADisassemblerCreate(void); // returns 1 if error
void Win_GBADisassemblerSetFocus(void);
void Win_GBADisassemblerClose(void);

//win_gba_memviewer.c
//-------------------

int Win_GBAMemViewerCreate(void); // returns 1 if error
void Win_GBAMemViewerUpdate(void);
void Win_GBAMemViewerRender(void);
void Win_GBAMemViewerClose(void);

//win_gba_ioviewer.c
//------------------

int Win_GBAIOViewerCreate(void); // returns 1 if error
void Win_GBAIOViewerUpdate(void);
void Win_GBAIOViewerRender(void);
void Win_GBAIOViewerClose(void);

//win_gba_tileviewer.c
//--------------------

int Win_GBATileViewerCreate(void); // returns 1 if error
void Win_GBATileViewerUpdate(void);
void Win_GBATileViewerClose(void);

//win_gba_mapviewer.c
//-------------------

int Win_GBAMapViewerCreate(void); // returns 1 if error
void Win_GBAMapViewerUpdate(void);
void Win_GBAMapViewerClose(void);

//win_gba_sprviewer.c
//-------------------

int Win_GBASprViewerCreate(void); // returns 1 if error
void Win_GBASprViewerUpdate(void);
void Win_GBASprViewerClose(void);

//win_gba_palviewer.c
//-------------------

int Win_GBAPalViewerCreate(void); // returns 1 if error
void Win_GBAPalViewerUpdate(void);
void Win_GBAPalViewerClose(void);

#endif // __WIN_GBA_DEBUGGER__



