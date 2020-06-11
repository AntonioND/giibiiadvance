// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef FILE_UTILS__
#define FILE_UTILS__

void DirSetRunningPath(char *path);
char *DirGetRunningPath(void);
char *DirGetBiosFolderPath(void);
char *DirGetScreenshotFolderPath(void);

void FileLoad_NoError(const char *filename, void **buffer, size_t *size_);
void FileLoad(const char *filename, void **buffer, size_t *size_);

int FileExists(const char *filename); // Returns 1 if file exists

int PathIsDir(char *path); // Returns 1 if path is a directory

int DirCheckExistence(char *path);
int DirCreate(char *path);

char *FU_GetNewTimestampFilename(const char *basename);

#endif // FILE_UTILS__
