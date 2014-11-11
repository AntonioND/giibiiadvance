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

#ifndef __SOUND__
#define __SOUND__

#include "gameboy.h"

void GB_SoundInit(void);

void GB_SoundClockCounterReset(void);
void GB_SoundUpdateClocksClounterReference(int reference_clocks);

inline int GB_SoundHardwareIsOn(void);

void GB_ToggleSound(void);

void GB_SoundUpdate(u32 clocks);

void GB_SoundRegWrite(u32 address, u32 value);

void GB_SoundResetBufferPointers(void);

void GB_SoundEnd(void);

void GB_SoundCallback(void * buffer, long len);

//---------------------------------

void GB_SoundGetConfig(int * vol, int * chn_flags);

void GB_SoundSetConfig(int vol, int chn_flags);

//---------------------------------

#endif //__SOUND__

