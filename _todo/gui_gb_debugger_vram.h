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

#ifndef __GUI_GB_DEBUGGER_VRAM__
#define __GUI_GB_DEBUGGER_VRAM__

void GLWindow_GBCreatePalViewer(void);
void GLWindow_GBPalViewerUpdate(void);
void GLWindow_GBClosePalViewer(void);

void GLWindow_GBSprViewerUpdate(void);
void GLWindow_GBCreateSprViewer(void);
void GLWindow_GBCloseSprViewer(void);

void GLWindow_GBMapViewerUpdate(void);
void GLWindow_GBCreateMapViewer(void);
void GLWindow_GBCloseMapViewer(void);

void GLWindow_GBTileViewerUpdate(void);
void GLWindow_GBCreateTileViewer(void);
void GLWindow_GBCloseTileViewer(void);

#endif //__GUI_GB_DEBUGGER_VRAM__


