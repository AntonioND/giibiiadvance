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

#ifndef __GB_SERIAL__
#define __GB_SERIAL__

//--------------------------------------------------------------------------------

void GB_SerialClockCounterReset(void);
void GB_SerialUpdateClocksClounterReference(int reference_clocks);
int GB_SerialGetClocksToNextEvent(void);

//--------------------------------------------------------------------------------

void GB_SerialWriteSB(int reference_clocks, int value);
void GB_SerialWriteSC(int reference_clocks, int value);

//--------------------------------------------------------------------------------

void GB_SerialInit(void);
void GB_SerialEnd(void);

//--------------------------------------------------------------------------------

void GB_SerialPlug(int device);

//--------------------------------------------------------------------------------

#endif //__GB_SERIAL__

