
#include <dirent.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "build_options.h"
#include "general_utils.h"

//----------------------------------------------------------------------------

static int IsValidROMType(char * name)
{
    char extension[4];
    int len = strlen(name);

    extension[3] = '\0';
    extension[2] = toupper(name[len-1]);
    extension[1] = toupper(name[len-2]);
    extension[0] = toupper(name[len-3]);
    if(strcmp(extension,"GBA") == 0) return 1;
    if(strcmp(extension,"AGB") == 0) return 1;
    if(strcmp(extension,"BIN") == 0) return 1;
    if(strcmp(extension,"GBC") == 0) return 1;
    if(strcmp(extension,"CGB") == 0) return 1;
    if(strcmp(extension,"SGB") == 0) return 1;

    extension[0] = extension[1];
    extension[1] = extension[2];
    extension[2] = '\0';
    if(strcmp(extension,"GB") == 0) return 1;

    return 0;
}

char GetFolderSeparator(char * path)
{
    if(path == NULL) return '/';
    while(*path)
    {
        if(*path == '\\') return '\\';
        if(*path == '/') return '/';
        path ++;
    }
    return '/';
}

//----------------------------------------------------------------------------

int filenum;
char fileselected[MAX_PATHLEN]; // this holds the last file selected even after FileExplorer_ListFree()
char ** filename = NULL;
int * list_isdir = NULL;
int maxfiles; // allocated space for files
int is_root = 0;
char exploring_path[MAX_PATHLEN];
int list_inited = 0;

int FileExplorer_GetNumFiles(void)
{
    return filenum;
}

void FileExplorer_SetPath(char * path)
{
    if(path)
        s_strncpy(exploring_path,path,sizeof(exploring_path));
    else
        strcpy(exploring_path,".");
}

void FileExplorer_ListFree(void)
{
    if(list_inited == 0) return;
    list_inited = 0;

	if(list_isdir) free(list_isdir);

	if(filename)
	{
		u32 a;
		for(a = 0; a < filenum; a++)
			if(filename[a]) free(filename[a]);
		free(filename);
	}

	filenum = 0;
	maxfiles = 0;
}

void FileExplorer_ListInit(int numfiles)
{
	list_isdir = malloc(numfiles*sizeof(int));
	filename = malloc(numfiles*sizeof(char*));
	maxfiles = numfiles;
	is_root = 0;
	list_inited = 1;
}

void FileExplorer_ListAdd(char * name, int isdir)
{
	if(strcmp(name,".") == 0) return;

	if(strcmp(name, "..") == 0)
	{
		is_root = 0;
		//return;
	}

	if(filenum < maxfiles)
	{
		if(isdir == 0)
		{
		    if(IsValidROMType(name) == 0)
                return;
		}

		list_isdir[filenum] = isdir;
		filename[filenum] = malloc(strlen(name)+1);
		strcpy(filename[filenum],name);
		filenum ++;
	}
}

char * FileExplorer_GetName(int index)
{
    if(index < filenum)
        return filename[index];
    return ".";
}

int FileExplorer_GetIsDir(int index)
{
    if(index < filenum)
        return list_isdir[index];
    return 0;
}

int FileExplorer_LoadFolder(void)
{
    //Debug_DebugMsg(exploring_path);

	FileExplorer_ListFree();

	DIR * pdir;

	pdir = opendir(exploring_path);
    if(pdir == NULL)
    {
        FileExplorer_ListInit(1);
        FileExplorer_ListAdd("..",1); // HACK
        return 1;
    }

	struct dirent * pent;
	struct stat statbuf;

	//Get number of files...
	u32 count = 0;
	while((pent=readdir(pdir))!=NULL)
	{
		count ++;
	}

	rewinddir(pdir);

    //Debug_DebugMsgArg("Directory files: %d", count);

    if(count == 0)
    {
        FileExplorer_ListInit(1);
        FileExplorer_ListAdd("..",1); // HACK
        return 1;
    }

	//Allocate enough space and get information...
	FileExplorer_ListInit(count);
	while(1)
	{
	    pent=readdir(pdir);
	    if(pent == NULL)
            break;

        char checkingfile[MAX_PATHLEN];
	    s_snprintf(checkingfile,sizeof(checkingfile),"%s%s",exploring_path,pent->d_name);

	    stat(checkingfile,&statbuf);
		FileExplorer_ListAdd(pent->d_name,(S_ISDIR(statbuf.st_mode))!=0);
		//Debug_DebugMsgArg("Entry: %d || <%s> dir: %d", filenum,pent->d_name,(S_ISDIR(statbuf.st_mode))!=0);
	}

	//Debug_DebugMsgArg("Shown files: %d", filenum);

	closedir(pdir);

	return filenum;
}

void FileExplorer_GoUp(void)
{
    char separator = GetFolderSeparator(exploring_path);
    int first_separator = 1;
    int l = strlen(exploring_path) - 1;
    while(l > 0)
    {
        if(exploring_path[l] == separator)
        {
            if(first_separator)
                first_separator = 0;
            else
            {
                exploring_path[l+1] = '\0';
                break;
            }
        }
        l--;
    }
}

int FileExplorer_SelectEntry(char * file)
{
    if(strcmp(file,".") == 0)
        return 1;

    if(strcmp(file,"..") == 0)
    {
        FileExplorer_GoUp();
        return 1;
    }

    char file_path[MAX_PATHLEN];
    s_snprintf(file_path,sizeof(file_path),"%s%s",exploring_path,file);
	struct stat statbuf;
	stat(file_path,&statbuf);
    //Debug_DebugMsgArg("stat(%s)",file_path);

	if(S_ISDIR(statbuf.st_mode))
	{

        char separator_str[2];
        separator_str[0] = GetFolderSeparator(exploring_path);
        separator_str[1] = '\0';
        //Debug_DebugMsgArg("Before: %s",exploring_path);
        s_strncat(exploring_path,file,sizeof(exploring_path));
        s_strncat(exploring_path,separator_str,sizeof(exploring_path));
        //Debug_DebugMsgArg("After: %s",exploring_path);

		FileExplorer_LoadFolder();

		return 1;
	}
	else
	{
		s_strncpy(fileselected,file_path,sizeof(fileselected));
		return 0;
	}

	return 0;
}

char * FileExplorer_GetResultFilePath(void)
{
	return fileselected;
}



