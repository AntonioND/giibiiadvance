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

#ifndef __GENERAL_UTILS__
#define __GENERAL_UTILS__

//----------------------------------------------------------------------------------

typedef unsigned long long int u64;
typedef signed long long int s64;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned short int u16;
typedef signed short int s16;
typedef unsigned char u8;
typedef signed char s8;

#define BIT(n) (1<<(n))

//----------------------------------------------------------------------------------

//safe versions of snprintf, strncpy and strncat that set a terminating character if needed.
//strncpy doesn't set it, and I'd say I've seen implementations of strncat and snprinf that
//doesn't, either.
int s_snprintf(char * dest, int _size, const char * msg, ...);
void s_strncpy(char * dest, const char * src, int _size);
void s_strncat(char * dest, const char * src, int _size);

void memset_rand(u8 * start, u32 _size);

u64 asciihex_to_int(const char * text); // converts an hexadecimal number in an ASCII string into integer
u64 asciidec_to_int(const char * text); // converts a decimal number in an ASCII string into integer

void ScaleImage24RGB(int zoom, char * srcbuf, int srcw, int srch, char * dstbuf, int dstw, int dsth);

//----------------------------------------------------------------------------------

#endif // __GENERAL_UTILS__

