#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char * argv[])
{
    if(argc < 2) return 1;

    unsigned char * d;
    unsigned int s;
    FileLoad_NoError(argv[1],(void**)&d,&s);
    if(s > 0)
    {
        int l = (s&0x1FFF)+4;


        FILE * f = fopen("out.bin","wb");
        if(f)
        {
            int i;
            for(i = 0; i < l; i++)
            {
                unsigned char x,y,z,w;
                x = d[i+0x0000];
                y = d[i+0x2000];
                z = d[i+0x4000];
                w = d[i+0x6000];
                fwrite(&x,1,1,f);
                fwrite(&y,1,1,f);
                fwrite(&z,1,1,f);
                fwrite(&w,1,1,f);
            }

            fclose(f);
        }

        free(d);
    }

    return 0;
}
