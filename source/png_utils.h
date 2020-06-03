// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef PNG_UTILS__
#define PNG_UTILS__

// Buffer is 32 bit, returns 1 if error, 0 if OK
int Save_PNG(const char *file_name, int width, int height, void *buffer,
             int save_alpha);

// Buffer is 32 bit
int Read_PNG(const char *file_name, char **_buffer, int *_width, int *_height);

#endif // PNG_UTILS__
