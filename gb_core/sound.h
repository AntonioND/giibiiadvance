/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __GB_SOUND__
#define __GB_SOUND__

#include "gameboy.h"

void GB_SoundInit(void);

inline int GB_SoundHardwareIsOn(void);

void GB_ToggleSound(void);

void GB_SoundUpdate(u32 clocks);

void GB_SoundRegWrite(u32 address, u32 value);

void GB_SoundResetBufferPointers(void);

void GB_SoundEnd(void);

void GB_SoundCallback(void * buffer, long len);

//----------------------------------------------------------------

inline void GB_SoundClockCounterReset(void);

void GB_SoundUpdateClocksCounterReference(int reference_clocks);

int GB_SoundGetClocksToNextEvent(void);

//----------------------------------------------------------------

void GB_SoundGetConfig(int * vol, int * chn_flags);

void GB_SoundSetConfig(int vol, int chn_flags);

//----------------------------------------------------------------

#endif //__GB_SOUND__

