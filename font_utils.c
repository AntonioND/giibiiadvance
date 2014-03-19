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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "debug_utils.h"
#include "png/png_utils.h"
#include "font_utils.h"
#include "file_utils.h"
#include "build_options.h"
#include "general_utils.h"

//------------------------------------------------------------------------------------------

#define FONT_CHARS_IN_ROW    (32)
#define FONT_CHARS_IN_COLUMN (8)

static char * fnt = NULL;

//------------------------------------------------------------------------------------------

int FU_Init(void)
{
    char path[MAX_PATHLEN];
    s_snprintf(path,sizeof(path),"%sfont.png",DirGetRunningPath());
    int w,h;
    if(Read_PNG(path,&fnt,&w,&h))
    {
        return 1;
    }
    return 0;
}

void FU_End(void)
{
    if(fnt) free(fnt);
    fnt = NULL;
}

//------------------------------------------------------------------------------------------

int FU_Print(char * buffer, int bufw, int bufh, int tx, int ty, const char * txt, ...)
{
    char txtbuffer[300];
    va_list args;
    va_start(args,txt);
    int ret = vsnprintf(txtbuffer, sizeof(txtbuffer), txt, args);
    va_end(args);
    txtbuffer[sizeof(txtbuffer)-1] = '\0';

    if( (tx < 0) || (ty < 0) ) return 0;
    if((ty + FONT_HEIGHT) > bufh) return 0; // not multiline...

    int i = 0;
    while(1)
    {
        unsigned char c = txtbuffer[i++];
        if(c == '\0') break;

        if((tx + FONT_WIDTH) > bufw) break;

        int texx = (c%FONT_CHARS_IN_ROW)*FONT_WIDTH;
        int texy = (c/FONT_CHARS_IN_ROW)*FONT_HEIGHT;

        int tex_offset = (texy*(FONT_CHARS_IN_ROW*FONT_WIDTH*4)) + texx*4; // font is 4 bytes per pixel
        int buf_offset = ((ty*bufw) + tx)*3; // buffer is 3 bytes per pixel

        int x, y;
        for(y = 0; y < FONT_HEIGHT; y ++)
        {
            char * bufcopy = &(buffer[buf_offset]);
            char * texcopy = &(fnt[tex_offset]);

            for(x = 0; x < FONT_WIDTH; x++)
            {
                *bufcopy++ = *texcopy++;
                *bufcopy++ = *texcopy++;
                *bufcopy++ = *texcopy++;
                texcopy++;
            }
            tex_offset += (FONT_CHARS_IN_ROW*FONT_WIDTH*4);
            buf_offset += bufw * 3;
        }
        tx += FONT_WIDTH;
    }

/*
    int x,y;
    for(y = 0; y < 12*8; y++) for(x = 0; x < 32*7; x++)
    {
        buffer[((y*bufw+x)*3)+0] = fnt12px[(y*fnt12->pitch)+(x*4)+0];
        buffer[((y*bufw+x)*3)+1] = fnt12px[(y*fnt12->pitch)+(x*4)+1];
        buffer[((y*bufw+x)*3)+2] = fnt12px[(y*fnt12->pitch)+(x*4)+2];
    }
*/
    return ret;
}

int FU_PrintColor(char * buffer, int bufw, int bufh, int tx, int ty, int color, const char * txt, ...)
{
    char txtbuffer[300];
    va_list args;
    va_start(args,txt);
    int ret = vsnprintf(txtbuffer, sizeof(txtbuffer), txt, args);
    va_end(args);
    txtbuffer[sizeof(txtbuffer)-1] = '\0';

    if( (tx < 0) || (ty < 0) ) return 0;
    if((ty + FONT_HEIGHT) > bufh) return 0; // not multiline...

    int i = 0;
    while(1)
    {
        unsigned char c = txtbuffer[i++];
        if(c == '\0') break;

        if((tx + FONT_WIDTH) > bufw) break;

        int texx = (c%FONT_CHARS_IN_ROW)*FONT_WIDTH;
        int texy = (c/FONT_CHARS_IN_ROW)*FONT_HEIGHT;

        int tex_offset = (texy*(FONT_CHARS_IN_ROW*FONT_WIDTH*4)) + texx*4; // font is 4 bytes per pixel
        int buf_offset = ((ty*bufw) + tx)*3; // buffer is 3 bytes per pixel

        int x, y;
        for(y = 0; y < FONT_HEIGHT; y ++)
        {
            char * bufcopy = &(buffer[buf_offset]);
            char * texcopy = &(fnt[tex_offset]);

            for(x = 0; x < FONT_WIDTH; x++)
            {
                *bufcopy++ = ( (int)(unsigned char)(*texcopy++) * (color&0xFF) ) >> 8;
                *bufcopy++ = ( (int)(unsigned char)(*texcopy++) * ((color>>8)&0xFF) ) >> 8;
                *bufcopy++ = ( (int)(unsigned char)(*texcopy++) * ((color>>16)&0xFF) ) >> 8;
                texcopy++;
            }
            tex_offset += (FONT_CHARS_IN_ROW*FONT_WIDTH*4);
            buf_offset += bufw * 3;
        }
        tx += FONT_WIDTH;
    }

    return ret;
}

int FU_PrintChar(char * buffer, int bufw, int bufh, int tx, int ty, unsigned char c, int color)
{
    if( (tx < 0) || (ty < 0) ) return 0;
    if((ty + FONT_HEIGHT) > bufh) return 0;

    if(c == '\0') return 0;

    if((tx + FONT_WIDTH) > bufw) return 0;

    int texx = (c%FONT_CHARS_IN_ROW)*FONT_WIDTH;
    int texy = (c/FONT_CHARS_IN_ROW)*FONT_HEIGHT;

    int tex_offset = (texy*(FONT_CHARS_IN_ROW*FONT_WIDTH*4)) + texx*4; // font is 4 bytes per pixel
    int buf_offset = ((ty*bufw) + tx)*3; // buffer is 3 bytes per pixel

    int x, y;
    for(y = 0; y < FONT_HEIGHT; y ++)
    {
        char * bufcopy = &(buffer[buf_offset]);
        char * texcopy = &(fnt[tex_offset]);

        for(x = 0; x < FONT_WIDTH; x++)
        {
            *bufcopy++ = ( (int)(unsigned char)(*texcopy++) * (color&0xFF) ) >> 8;
            *bufcopy++ = ( (int)(unsigned char)(*texcopy++) * ((color>>8)&0xFF) ) >> 8;
            *bufcopy++ = ( (int)(unsigned char)(*texcopy++) * ((color>>16)&0xFF) ) >> 8;
            texcopy++;
        }
        tex_offset += (FONT_CHARS_IN_ROW*FONT_WIDTH*4);
        buf_offset += bufw * 3;
    }

    return 1;
}

//------------------------------------------------------------------------------------------
