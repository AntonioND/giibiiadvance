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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "general_utils.h"
#include "debug_utils.h"
#include "build_options.h"

//-------------------------------------------------

static char running_path[MAX_PATHLEN];
static char bios_path[MAX_PATHLEN];
static char screenshot_path[MAX_PATHLEN];

void DirSetRunningPath(char * path)
{
    char slash_char = '\0';

    s_strncpy(running_path,path,sizeof(running_path));
    int l = strlen(running_path);
    while(l > 0)
    {
        if(running_path[l] == '/')
        {
            slash_char = '/';
            running_path[l+1] = '\0';
            break;
        }
        else if(running_path[l] == '\\')
        {
            slash_char = '\\';
            running_path[l+1] = '\0';
            break;
        }
        l--;
    }

    if(l == 0)
    {
        running_path[0] = '.'; //Problems here...
        running_path[1] = '/';
        running_path[2] = '\0';
    }

    s_snprintf(bios_path,sizeof(bios_path),"%s%s%c",running_path,BIOS_FOLDER,slash_char);
    s_snprintf(screenshot_path,sizeof(screenshot_path),"%s%s%c",running_path,SCREENSHOT_FOLDER,slash_char);
}

char * DirGetRunningPath(void)
{
    return running_path;
}

char * DirGetBiosFolderPath(void)
{
    return bios_path;
}

char * DirGetScreenshotFolderPath(void)
{
    return screenshot_path;
}

//-------------------------------------------------

void FileLoad_NoError(const char * filename, void ** buffer, unsigned int * size_)
{
    FILE * datafile = fopen(filename, "rb");
    unsigned int size;
    *buffer = NULL;
    if(size_) *size_ = 0;

    if(datafile == NULL) return;

    fseek(datafile, 0, SEEK_END);
    size = ftell(datafile);
    if(size_) *size_ = size;

    if(size == 0)
    {
        fclose(datafile);
        return;
    }

    rewind(datafile);
    *buffer = malloc(size);

    if(*buffer == NULL)
    {
        fclose(datafile);
        return;
    }

    if(fread(*buffer,size,1,datafile) != 1)
    {
        fclose(datafile);
        return;
    }

    fclose(datafile);
}

void FileLoad(const char * filename, void ** buffer, unsigned int * size_)
{
    FILE * datafile = fopen(filename, "rb");
    unsigned int size;
    *buffer = NULL;
    if(size_) *size_ = 0;

    if(datafile == NULL)
    {
        Debug_ErrorMsgArg("%s couldn't be opened!",filename);
        return;
    }

    fseek(datafile, 0, SEEK_END);
    size = ftell(datafile);
    if(size_) *size_ = size;
    if(size == 0)
    {
        Debug_ErrorMsgArg("Size of %s is 0!",filename);
        fclose(datafile);
        return;
    }
    rewind(datafile);
    *buffer = malloc(size);
    if(*buffer == NULL)
    {
        Debug_ErrorMsgArg("Not enought memory to load %s!",filename);
        fclose(datafile);
        return;
    }
    if(fread(*buffer,size,1,datafile) != 1)
    {
        Debug_ErrorMsgArg("Error while reading: %s",filename);
        fclose(datafile);
        return;
    }

    fclose(datafile);
}

int FileExists(const char * filename)
{
    FILE * f = fopen(filename, "rb");
    if(f == NULL) return 0;
    fclose(f);
    return 1;
}

//-------------------------------------------------

int DirCheckExistence(char * path)
{
    struct stat s;
    int err = stat(path, &s);
    if(err == -1)
    {
        if(errno == ENOENT)
        {
            return 0; // does not exist
        }
        else
        {
            //perror("stat"); // stat error
            //exit(1);
            return 0;
        }
    }
    else
    {
        if(S_ISDIR(s.st_mode))
        {
            return 1; // it's a dir
        }
        else
        {
            Debug_ErrorMsgArg("Checking for existence of directory, but it is a file: %s",path);
            return 0; // exists but is no dir
        }
    }
}

int DirCreate(char * path)
{
#ifdef __unix__
    if(mkdir(path, S_IRWXU|S_IRGRP|S_IXGRP) == 0)
#elif defined _WIN32
    if(mkdir(path) == 0) // seriously??
#else
#warning "This hasn't been tested!"
    if(mkdir(path, S_IRWXU|S_IRGRP|S_IXGRP) == 0) // try with this
#endif
    {
        return 1;
    }
    else
    {
        //Debug_ErrorMsgArg("Couldn't create directory: %s",path);
        return 0;
    }
}

//-------------------------------------------------

static char _fu_filename[MAX_PATHLEN];

char * FU_GetNewTimestampFilename(const char * basename)
{
    int number = 0;

    time_t rawtime;
    time(&rawtime);
    struct tm * ptm = gmtime(&rawtime);

    char timestamp[50];
    s_snprintf(timestamp,sizeof(timestamp),"%04d%02d%02d_%02d%02d%02d",1900+ptm->tm_year,1+ptm->tm_mon,ptm->tm_mday,
               1+ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

    while(1)
    {
        s_snprintf(_fu_filename,sizeof(_fu_filename),"%s%s_%s_%d.png",DirGetScreenshotFolderPath(),basename,
                   timestamp,number);

        FILE * file=fopen(_fu_filename, "rb");
        if(file == NULL) break; //Ok
        number ++; //look for next free number
        fclose(file);
    }

    return _fu_filename;
}

//-------------------------------------------------

