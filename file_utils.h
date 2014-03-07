
#ifndef __FILE_UTILS__
#define __FILE_UTILS__

void DirSetRunningPath(char * path);
char * DirGetRunningPath(void);
char * DirGetBiosFolderPath(void);
char * DirGetScreenshotFolderPath(void);

void FileLoad_NoError(const char * filename, void ** buffer, unsigned int * size_);
void FileLoad(const char * filename, void ** buffer, unsigned int * size_);

int DirCheckExistence(char * path);
int DirCreate(char * path);

#endif // __FILE_UTILS__

