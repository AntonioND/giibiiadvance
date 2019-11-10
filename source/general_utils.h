// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

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

//safe versions of strncpy and strncat that set a terminating character if needed.
void s_strncpy(char * dest, const char * src, int _size);
void s_strncat(char * dest, const char * src, int _size);

void memset_rand(u8 * start, u32 _size);

u64 asciihex_to_int(const char * text); // converts an hexadecimal number in an ASCII string into integer
u64 asciidec_to_int(const char * text); // converts a decimal number in an ASCII string into integer

void ScaleImage24RGB(int zoom, char * srcbuf, int srcw, int srch, char * dstbuf, int dstw, int dsth);

//----------------------------------------------------------------------------------

#endif // __GENERAL_UTILS__

