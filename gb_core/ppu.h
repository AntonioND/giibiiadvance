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

#ifndef __GB_PPU__
#define __GB_PPU__

//----------------------------------------------------------------

#include "gameboy.h"

//----------------------------------------------------------------

void GB_PPUInit(void);
void GB_PPUEnd(void);

//----------------------------------------------------------------

inline void GB_PPUClockCounterReset(void);
void GB_PPUUpdateClocksClounterReference(int reference_clocks);
inline int GB_PPUGetClocksToNextEvent(void);

//----------------------------------------------------------------

inline void GB_PPUCheckStatSignal(void);
inline void GB_PPUCheckLYC(void);

//----------------------------------------------------------------

#endif // __GB_PPU__
