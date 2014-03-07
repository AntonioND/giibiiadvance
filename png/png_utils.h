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

#ifndef __PNG_UTILS__
#define __PNG_UTILS__

// buffer is 32 bit, returns 1 if error, 0 if OK
int Save_PNG(const char * file_name, int width, int height, void * buffer, int save_alpha);

// buffer is 32 bit
int Read_PNG(const char * file_name, char ** _buffer, int * _width, int * _height);

#endif //__PNG_UTILS__

