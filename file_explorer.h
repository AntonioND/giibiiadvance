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

#ifndef __FILE_EXPLORER__
#define __FILE_EXPLORER__

//---------------------------------------------------------------------

int FileExplorer_GetNumFiles(void);
void FileExplorer_SetPath(char * path);
void FileExplorer_ListFree(void);
char * FileExplorer_GetName(int index);
int FileExplorer_GetIsDir(int index);
int FileExplorer_LoadFolder(void);
int FileExplorer_SelectEntry(char * file); // returns 1 if dir, 0 if file
char * FileExplorer_GetResultFilePath(void); // this holds the last file selected even after FileExplorer_ListFree()


//---------------------------------------------------------------------

#endif //__FILE_EXPLORER__

