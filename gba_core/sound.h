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

#ifndef __GBA_SOUND__
#define __GBA_SOUND__

#include "../general_utils.h"

void GBA_SoundInit(void);

inline int GBA_SoundHardwareIsOn(void);

void GB_ToggleSound(void);

u32 GBA_SoundUpdate(u32 clocks);

void GBA_SoundRegWrite16(u32 address, u16 value);

void GBA_SoundResetBufferPointers(void);

void GBA_SoundEnd(void);

void GBA_SoundCallback(void * buffer, long len);

void GBA_SoundTimerCheck(int number);

//---------------------------------

void GBA_SoundGetConfig(int * vol, int * chn_flags);

void GBA_SoundSetConfig(int vol, int chn_flags);

//---------------------------------

int gba_debug_get_psg_vol(int chan);

#endif //__GBA_SOUND__


