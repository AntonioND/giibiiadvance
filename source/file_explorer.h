// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef FILE_EXPLORER__
#define FILE_EXPLORER__

int FileExplorer_GetNumFiles(void);
void FileExplorer_SetPath(char * path);
void FileExplorer_ListFree(void);
char *FileExplorer_GetName(int index);
int FileExplorer_GetIsDir(int index);
int FileExplorer_LoadFolder(void);

// Returns 1 if dir, 0 if file
int FileExplorer_SelectEntry(char *file);
// This holds the last file selected even after FileExplorer_ListFree()
char *FileExplorer_GetResultFilePath(void);
// Current exploring path
char *FileExplorer_GetCurrentPath(void);

#endif // FILE_EXPLORER__
