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

#ifndef __MAIN__
#define __MAIN__

//-----------------------------------------------------------
//-----------------------------------------------------------
typedef unsigned long long int u64;
typedef signed long long int s64;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned short int u16;
typedef signed short int s16;
typedef unsigned char u8;
typedef signed char s8;

#define BIT(n) (1<<(n))
//-----------------------------------------------------------

void ConsoleReset(void);
void ConsolePrint(const char * msg, ...);
void ConsoleShow(void);

//-----------------------------------------------------------

void SoundMasterEnable(int enable);

char * GetRunningFolder(void);

void memset_rand(u8 * start, u32 size);

u64 asciihextoint(const char * text);
u64 asciidectoint(const char * text);

void DebugMessage(const char * msg, ...);
void ErrorMessage(const char * msg, ...);
extern int MESSAGE_SHOWING;

void FileLoad_NoError(const char * filename, void ** buffer, unsigned int * size_);
void FileLoad(const char * filename, void ** buffer, unsigned int * size_);

int DirCheckExistence(char * path);
int DirCreate(char * path);

//-----------------------------------------------------------

#endif //__MAIN__

