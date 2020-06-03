// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GENERAL_UTILS__
#define GENERAL_UTILS__

typedef unsigned long long int u64;
typedef signed long long int s64;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned short int u16;
typedef signed short int s16;
typedef unsigned char u8;
typedef signed char s8;

#define BIT(n) (1 << (n))

#if defined(_MSC_VER)
# define unused__
#else
# define unused__ __attribute__((unused))
#endif

// Safe versions of strncpy and strncat that set a terminating character if
// needed.
void s_strncpy(char *dest, const char *src, int _size);
void s_strncat(char *dest, const char *src, int _size);

void memset_rand(u8 *start, u32 _size);

// Converts an hexadecimal number in an ASCII string into integer
u64 asciihex_to_int(const char *text);

// Converts a decimal number in an ASCII string into integer
u64 asciidec_to_int(const char *text);

void ScaleImage24RGB(int zoom, char *srcbuf, int srcw, int srch, char *dstbuf,
                     int dstw, int dsth);

#endif // GENERAL_UTILS__
