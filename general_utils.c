

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "general_utils.h"

//----------------------------------------------------------------------------------

int s_snprintf(char * dest, int _size, const char * msg, ...)
{
    va_list args;
	va_start(args,msg);
	int ret = vsnprintf(dest, _size, msg, args);
	va_end(args);
    dest[_size-1] = '\0';
    return ret;
}

void s_strncpy(char * dest, const char * src, int _size)
{
	strncpy(dest,src,_size);
    dest[_size-1] = '\0';
}

void s_strncat(char * dest, const char * src, int _size)
{
	strncat(dest,src,_size);
    dest[_size-1] = '\0';
}

//----------------------------------------------------------------------------------

void memset_rand(u8 * start, u32 _size)
{
	while(_size--) *start++ = rand();
}

//----------------------------------------------------------------------------------

u64 asciihex_to_int(const char * text)
{
    long int value = 0, i = 0;
    while(1)
    {
        char char_ = toupper(text[i++]); //end of string
        if(char_ == '\0') return value;
        else if(char_ >= '0' && char_ <= '9') value = (value * 16) + (char_ - '0');
        else if(char_ >= 'a' && char_ <= 'f') value = (value * 16) +  (char_ - 'a' + 10);
        else if(char_ >= 'A' && char_ <= 'F') value = (value * 16) +  (char_ - 'A' + 10);
        else return 0xFFFFFFFFFFFFFFFFULL;
    }
}

u64 asciidec_to_int(const char * text)
{
    long int value = 0, i = 0;
    while(1)
    {
        char char_ = toupper(text[i++]); //end of string
        if(char_ == '\0') return value;
        else if(char_ >= '0' && char_ <= '9') value = (value*10) + (char_ - '0');
        else return 0xFFFFFFFFFFFFFFFFULL;
    }
}

//----------------------------------------------------------------------------------

void ScaleImage24RGB(int zoom, char * srcbuf, int srcw, int srch, char * dstbuf, int dstw, int dsth)
{
    int dest_x_offset = (dstw-(srcw*zoom)) / 2;
    int dest_y_offset = (dsth-(srch*zoom)) / 2;

    int dest_x_end = dest_x_offset + srcw*zoom;
    int dest_y_end = dest_y_offset + srch*zoom;

    int srcx = 0;
    int srcy = 0;

    int srcx_inc_count = 0;
    int srcy_inc_count = 0;

    //int destx = dest_x_offset;
    int desty = dest_y_offset;

    for( ; desty < dest_y_end; desty ++)
    {
        srcx = 0;
        int destx = dest_x_offset;
        char * dstline = &(dstbuf[ (desty*dstw + destx) * 3]);
        char * srcline = &(srcbuf[ (srcy*srcw + srcx) * 3 ]);

        for( ; destx < dest_x_end; destx++)
        {
            *dstline++ = *(srcline+0);
            *dstline++ = *(srcline+1);
            *dstline++ = *(srcline+2);

            srcx_inc_count++;
            if(srcx_inc_count == zoom)
            {
                srcline += 3;
                srcx_inc_count = 0;
                srcx++;
            }
        }

        srcy_inc_count++;
        if(srcy_inc_count == zoom)
        {
            srcy_inc_count = 0;
            srcy++;
        }
    }
}

//----------------------------------------------------------------------------------


