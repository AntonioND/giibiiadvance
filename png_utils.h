// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __PNG_UTILS__
#define __PNG_UTILS__

// buffer is 32 bit, returns 1 if error, 0 if OK
int Save_PNG(const char * file_name, int width, int height, void * buffer, int save_alpha);

// buffer is 32 bit
int Read_PNG(const char * file_name, char ** _buffer, int * _width, int * _height);

#endif //__PNG_UTILS__

