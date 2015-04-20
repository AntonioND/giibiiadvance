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

#ifndef __GBA_TIMERS__
#define __GBA_TIMERS__

#include "gba.h"

inline void GBA_TimerInitAll(void);

inline void GBA_TimerSetStart0(u16 val);
inline void GBA_TimerSetStart1(u16 val);
inline void GBA_TimerSetStart2(u16 val);
inline void GBA_TimerSetStart3(u16 val);

inline u16 gba_timergetstart0(void); // +
inline u16 gba_timergetstart1(void); // |- for the debugger
inline u16 gba_timergetstart2(void); // |
inline u16 gba_timergetstart3(void); // *

void GBA_TimerSetup0(void);
void GBA_TimerSetup1(void);
void GBA_TimerSetup2(void);
void GBA_TimerSetup3(void);

s32 GBA_TimersUpdate(s32 clocks);

#endif //__GBA_TIMERS__


