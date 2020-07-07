// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GENERAL_UTILS__
#define GENERAL_UTILS__

#include <stddef.h>
#include <stdint.h>

typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint8_t u8;
typedef int8_t s8;

#define BIT(n) (1 << (n))

#if defined(_MSC_VER)
# define unused__
#else
# define unused__ __attribute__((unused))
#endif

#if defined(_MSC_VER)
# define ALIGNED(x) __declspec(align(x))
#else
# define ALIGNED(x) __attribute__((aligned(x)))
#endif

// Safe versions of strncpy and strncat that set a terminating character if
// needed.
void s_strncpy(char *dest, const char *src, size_t _size);
void s_strncat(char *dest, const char *src, size_t _size);

void memset_rand(u8 *start, u32 _size);

// Converts an hexadecimal number in an ASCII string into integer
u64 asciihex_to_int(const char *text);

// Converts a decimal number in an ASCII string into integer
u64 asciidec_to_int(const char *text);

void ScaleImage24RGB(int zoom, char *srcbuf, int srcw, int srch, char *dstbuf,
                     int dstw, int dsth);

#endif // GENERAL_UTILS__
