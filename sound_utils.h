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

#ifndef __SOUND_UTILS__
#define __SOUND_UTILS__

typedef void (Sound_CallbackPointer)(void*,long);

void Sound_Init(void);

void Sound_SetCallback(Sound_CallbackPointer * fn);

void Sound_Enable(void);
void Sound_Disable(void);

void Sound_SetVolume(int vol);

void Sound_SetEnabled(int enable);
int Sound_GetEnabled(void);

void Sound_SetEnabledChannels(int flags);

#endif // __SOUND_UTILS__

