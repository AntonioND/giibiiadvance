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

#ifndef __GUI_MAINLOOP__
#define __GUI_MAINLOOP__

void GLWindow_Mainloop(void);

extern int PAUSED;

extern int FRAMESKIP;
extern int AUTO_FRAMESKIP_WAIT; //seconds left to start autoframeskip

#define RUN_NONE 0
#define RUN_GBA  1
#define RUN_GB   2
extern int RUNNING;

#endif //__GUI_MAINLOOP__


