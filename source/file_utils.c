// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "build_options.h"
#include "debug_utils.h"
#include "general_utils.h"

static char running_path[MAX_PATHLEN];
static char bios_path[MAX_PATHLEN];
static char screenshot_path[MAX_PATHLEN];

void DirSetRunningPath(char *path)
{
    char slash_char = '\0';

    s_strncpy(running_path, path, sizeof(running_path));

    // Check the folder division character used by the OS and remove the last
    // one from the path, if there is any.
    int l = strlen(running_path);
    while (l > 0)
    {
        if (running_path[l] == '/')
        {
            slash_char = '/';
            running_path[l + 1] = '\0';
            break;
        }
        else if (running_path[l] == '\\')
        {
            slash_char = '\\';
            running_path[l + 1] = '\0';
            break;
        }
        l--;
    }

    if (l == 0)
    {
        // Not a single divisor was found... Try with '/' and set the running
        // path to "./".
        running_path[0] = '.';
        running_path[1] = '/';
        running_path[2] = '\0';
    }

    snprintf(bios_path, sizeof(bios_path), "%s%s%c", running_path,
             BIOS_FOLDER, slash_char);
    snprintf(screenshot_path, sizeof(screenshot_path), "%s%s%c", running_path,
             SCREENSHOT_FOLDER, slash_char);
}

char *DirGetRunningPath(void)
{
    return running_path;
}

char *DirGetBiosFolderPath(void)
{
    return bios_path;
}

char *DirGetScreenshotFolderPath(void)
{
    return screenshot_path;
}

//-------------------------------------------------

void FileLoad_NoError(const char *filename, void **buffer, unsigned int *size_)
{
    FILE *f = fopen(filename, "rb");
    unsigned int size;

    *buffer = NULL;
    if (size_)
        *size_ = 0;

    if (f == NULL)
        return;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    if (size_)
        *size_ = size;

    if (size == 0)
    {
        fclose(f);
        return;
    }

    rewind(f);
    *buffer = malloc(size);

    if (*buffer == NULL)
    {
        fclose(f);
        return;
    }

    if (fread(*buffer, size, 1, f) != 1)
    {
        fclose(f);
        free(*buffer);
        return;
    }

    fclose(f);
}

void FileLoad(const char *filename, void **buffer, unsigned int *size_)
{
    FILE *f = fopen(filename, "rb");
    unsigned int size;

    *buffer = NULL;
    if (size_)
        *size_ = 0;

    if (f == NULL)
    {
        Debug_ErrorMsgArg("%s couldn't be opened!", filename);
        return;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    if (size_)
        *size_ = size;

    if (size == 0)
    {
        Debug_ErrorMsgArg("Size of %s is 0!", filename);
        fclose(f);
        return;
    }

    rewind(f);
    *buffer = malloc(size);
    if (*buffer == NULL)
    {
        Debug_ErrorMsgArg("Not enought memory to load %s!", filename);
        fclose(f);
        return;
    }

    if (fread(*buffer, size, 1, f) != 1)
    {
        Debug_ErrorMsgArg("Error while reading: %s", filename);
        fclose(f);
        free(*buffer);
        return;
    }

    fclose(f);
}

int FileExists(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
        return 0;

    fclose(f);
    return 1;
}

//-------------------------------------------------

int DirCheckExistence(char *path)
{
    struct stat s;
    int err = stat(path, &s);
    if (err == -1)
    {
        if (errno == ENOENT)
        {
            return 0; // Does not exist
        }
        else
        {
            //perror("stat"); // Stat error
            //exit(1);
            return 0;
        }
    }
    else
    {
        if (S_ISDIR(s.st_mode))
        {
            return 1; // It's a dir
        }
        else
        {
            Debug_ErrorMsgArg("Not a directory, but a file: %s", path);
            return 0; // Exists but it isn't a directory
        }
    }
}

int DirCreate(char *path)
{
#ifdef __unix__
    if (mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP) == 0)
#elif defined _WIN32
    if (mkdir(path) == 0)
#else
#warning "This hasn't been tested!"
    if (mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP) == 0)
#endif
    {
        return 1;
    }
    else
    {
        //Debug_ErrorMsgArg("Couldn't create directory: %s", path);
        return 0;
    }
}

//-------------------------------------------------

static char _fu_filename[MAX_PATHLEN];

char *FU_GetNewTimestampFilename(const char *basename)
{
    long long int number = 0;

    time_t rawtime;
    time(&rawtime);
    struct tm *ptm = gmtime(&rawtime);

    // Generate base file name based on the current time and date
    char timestamp[50];
    snprintf(timestamp, sizeof(timestamp), "%04d%02d%02d_%02d%02d%02d",
             1900 + ptm->tm_year, 1 + ptm->tm_mon, ptm->tm_mday,
             1 + ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    // Append a number to the name so that we can take multiple screenshots the
    // same second.
    while (1)
    {
        snprintf(_fu_filename, sizeof(_fu_filename), "%s%s_%s_%lld.png",
                 DirGetScreenshotFolderPath(), basename, timestamp, number);

        FILE *file = fopen(_fu_filename, "rb");
        if (file == NULL)
            break; // This name is available
        fclose(file);

        number++;
    }

    return _fu_filename;
}
