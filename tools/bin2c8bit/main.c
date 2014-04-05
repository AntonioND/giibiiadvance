#include <stdio.h>
#include <stdlib.h>

void FileLoad(const char * filename, void ** buffer, unsigned int * size_)
{
	FILE * datafile = fopen(filename, "rb");
	unsigned int size;
	*buffer = NULL;
	if(size_) *size_ = 0;

	if(datafile == NULL)
	{
	    printf("%s couldn't be opened!",filename);
        return;
    }

    fseek(datafile, 0, SEEK_END);
	size = ftell(datafile);
	if(size_) *size_ = size;
	if(size == 0)
	{
	    printf("Size of %s is 0!",filename);
        fclose(datafile);
        return;
    }
	rewind(datafile);
	*buffer = malloc(size);
	if(*buffer == NULL)
	{
	    printf("Not enought memory to load %s!",filename);
        fclose(datafile);
        return;
    }
	if(fread(*buffer,size,1,datafile) != 1)
	{
	    printf("Error while reading.");
        fclose(datafile);
        return;
    }

	fclose(datafile);
}

int main(int argc, char **argv)
{
    if(argc < 2) return 0;

    unsigned char * file;
    unsigned int size;
    FileLoad(argv[1],(void**)&file,&size);

    FILE * f = fopen("out.c","w");
    if(f==NULL)
    {
        free(file);
        return 1;
    }
    int i = 0;
    while(size>0)
    {
        fprintf(f,"0x%02X,",file[i++]);
        if((i%16) == 0) fprintf(f,"\n");
        size --;
    }
    fclose(f);

    return 0;
}
