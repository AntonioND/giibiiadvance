
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "png/png_utils.h"

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        return 1;
    }

    char * filename = argv[1];

    char * buf;
    int w,h;

    if(Read_PNG(filename, &buf, &w, &h))
    {
        return 1;
    }

    FILE * fout = fopen("out.c","w");
    if(fout)
    {
        int n = 0;

        int i,j;
        for(j = 0; j < h; j++) for(i = 0; i < w; i++)
        {
            unsigned char r = buf[(j*w+i)*4+0];
            unsigned char g = buf[(j*w+i)*4+1];
            unsigned char b = buf[(j*w+i)*4+2];
            unsigned char a = buf[(j*w+i)*4+3];

            fprintf(fout,"0x%02X,0x%02X,0x%02X,0x%02X,",r,g,b,a);

            n++;
            if(n == 4)
            {
                n = 0;
                fprintf(fout,"\n");
            }
        }

        fclose(fout);
    }

    free(buf);

	return 0;
}
