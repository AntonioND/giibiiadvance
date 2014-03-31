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

#ifndef __GB_CAMERA__
#define __GB_CAMERA__

int GB_CameraInit(void);
void GB_CameraEnd(void);

int GB_CameraReadRegister(int address);
void GB_CameraWriteRegister(int address, int value);
int GB_CameraClock(int clocks); // returns clocks to next event

#endif // __GB_CAMERA__

