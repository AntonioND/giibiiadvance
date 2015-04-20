/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __FILE_UTILS__
#define __FILE_UTILS__

void DirSetRunningPath(char * path);
char * DirGetRunningPath(void);
char * DirGetBiosFolderPath(void);
char * DirGetScreenshotFolderPath(void);

void FileLoad_NoError(const char * filename, void ** buffer, unsigned int * size_);
void FileLoad(const char * filename, void ** buffer, unsigned int * size_);

int FileExists(const char * filename); // returns 1 if file exists

int DirCheckExistence(char * path);
int DirCreate(char * path);

char * FU_GetNewTimestampFilename(const char * basename);

#endif // __FILE_UTILS__

