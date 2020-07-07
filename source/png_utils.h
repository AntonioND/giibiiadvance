// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef PNG_UTILS__
#define PNG_UTILS__

// Buffer is 32 bit (RGBA) / 24 bit (RGB), returns 0 on success
int Save_PNG(const char *filename, unsigned char *buffer,
             int width, int height, int is_rgba);

// Buffer is 32 bit (RGBA), returns 0 on success
int Read_PNG(const char *filename, unsigned char **_buffer,
             int *_width, int *_height);

#endif // PNG_UTILS__
