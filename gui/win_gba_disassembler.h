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

#ifndef __WIN_GBA_DISASSEMBLER__
#define __WIN_GBA_DISASSEMBLER__

void Win_GBADisassemblerStartAddressSetDefault(void);
void Win_GBADisassemblerUpdate(void);
int Win_GBADisassemblerCreate(void); // returns 1 if error
void Win_GBADisassemblerSetFocus(void);
void Win_GBADisassemblerClose(void);

#endif // __WIN_GBA_DISASSEMBLER__

