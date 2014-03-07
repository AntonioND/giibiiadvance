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

#ifndef __GUI_GBA_DEBUGGER_VRAM__
#define __GUI_GBA_DEBUGGER_VRAM__

void GLWindow_GBACreatePalViewer(void);
void GLWindow_GBAPalViewerUpdate(void);
void GLWindow_GBAClosePalViewer(void);

void GLWindow_GBASprViewerUpdate(void);
void GLWindow_GBACreateSprViewer(void);
void GLWindow_GBACloseSprViewer(void);

void GLWindow_GBAMapViewerUpdate(void);
void GLWindow_GBACreateMapViewer(void);
void GLWindow_GBACloseMapViewer(void);

void GLWindow_GBATileViewerUpdate(void);
void GLWindow_GBACreateTileViewer(void);
void GLWindow_GBACloseTileViewer(void);

#endif //__GUI_GBA_DEBUGGER_VRAM__


