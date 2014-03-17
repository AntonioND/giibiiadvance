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

#include <string.h>

#include "gba.h"
#include "memory.h"

//----------------------------------------------------------------

static inline u32 rgb16to32(u16 color)
{
    int r = (color & 31)<<3;
    int g = ((color >> 5) & 31)<<3;
    int b = ((color >> 10) & 31)<<3;
    return (b<<16)|(g<<8)|r;
}

static inline int min(int a, int b)
{
    return (a < b) ? a : b;
}

//----------------------------------------------------------------

void GBA_Debug_PrintZoomedSpriteAt(int spritenum, int buf_has_alpha_channel, char * buffer, int bufw, int bufh,
                                   int posx, int posy, int sizex, int sizey)
{
    //Temp buffer
    int sprbuffer[64*64];
    int sprbuffer_vis[64*64]; memset(sprbuffer_vis,0,sizeof(sprbuffer_vis));

    static const int spr_size[4][4][2] = { //shape, size, (x,y)
        {{8,8},{16,16},{32,32},{64,64}}, //Square
        {{16,8},{32,8},{32,16},{64,32}}, //Horizontal
        {{8,16},{8,32},{16,32},{32,64}}, //Vertical
        {{0,0},{0,0},{0,0},{0,0}} //Prohibited
    };

    _oam_spr_entry_t * spr = &(((_oam_spr_entry_t*)Mem.oam)[spritenum]);

    u16 attr0 = spr->attr0;
    u16 attr1 = spr->attr1;
    u16 attr2 = spr->attr2;
    u16 shape = attr0>>14;
    u16 size = attr1>>14;
    int sx = spr_size[shape][size][0];
    int sy = spr_size[shape][size][1];
    u16 tilebaseno = attr2&0x3FF;

    if(attr0&BIT(13)) //256 colors
    {
        tilebaseno >>= 1; //in 256 mode, they need double space

        u16 * palptr = (u16*)&(Mem.pal_ram[256*2]);

        //Start drawing
        int xdiff, ydiff;
        for(ydiff = 0; ydiff < sy; ydiff++) for(xdiff = 0; xdiff < sx; xdiff++)
        {
            u32 tileadd = 0;
            if(REG_DISPCNT & BIT(6)) //1D
            {
                int tilex = xdiff>>3;
                int tiley = ydiff>>3;
                tileadd = tilex + (tiley*sx/8);
            }
            else //2D
            {
                int tilex = xdiff>>3;
                int tiley = ydiff>>3;
                tileadd = tilex + (tiley*16);
            }

            u32 tileindex = tilebaseno+tileadd;
            u8 * tile_ptr = (u8*)&(Mem.vram[0x10000+(tileindex*64)]);

            int _x = xdiff & 7;
            int _y = ydiff & 7;

            u8 data = tile_ptr[_x+(_y*8)];

            if(data)
            {
                sprbuffer[ydiff*64 + xdiff] = rgb16to32(palptr[data]);
                sprbuffer_vis[ydiff*64 + xdiff] = 1;
            }
        }
    }
    else //16 colors
    {
        u16 palno = attr2>>12;
        u16 * palptr = (u16*)&Mem.pal_ram[512+(palno*32)];

        //Start drawing
        int xdiff, ydiff;
        for(ydiff = 0; ydiff < sy; ydiff++) for(xdiff = 0; xdiff < sx; xdiff++)
        {
            u32 tileadd = 0;
            if(REG_DISPCNT & BIT(6)) //1D
            {
                int tilex = xdiff>>3;
                int tiley = ydiff>>3;
                tileadd = tilex + (tiley*sx/8);
            }
            else //2D
            {
                int tilex = xdiff>>3;
                int tiley = ydiff>>3;
                tileadd = tilex + (tiley*32);
            }

            u32 tileindex = tilebaseno+tileadd;
            u8 * tile_ptr = (u8*)&(Mem.vram[0x10000+(tileindex*32)]);

            int _x = xdiff & 7;
            int _y = ydiff & 7;

            u8 data = tile_ptr[(_x/2)+(_y*4)];

            if(_x&1) data = data>>4;
            else data = data & 0xF;

            if(data)
            {
                sprbuffer[ydiff*64 + xdiff] = rgb16to32(palptr[data]);
                sprbuffer_vis[ydiff*64 + xdiff] = 1;
            }
        }
    }

    //Expand/copy to real buffer
    int factor = min(sizex/sx,sizey/sy);

    if(buf_has_alpha_channel)
    {
        int i,j;
        for(i = 0; i < sizex; i++) for(j = 0; j < sizey; j++)
        {
            int bufx = i + posx;
            int bufy = j + posy;

            if(sprbuffer_vis[(j/factor)*64 + (i/factor)])
            {
                buffer[((bufy*bufw)+bufx)*4+0] = sprbuffer[(j/factor)*64 + (i/factor)]&0xFF;
                buffer[((bufy*bufw)+bufx)*4+1] = (sprbuffer[(j/factor)*64 + (i/factor)]>>8)&0xFF;
                buffer[((bufy*bufw)+bufx)*4+2] = (sprbuffer[(j/factor)*64 + (i/factor)]>>16)&0xFF;
                buffer[((bufy*bufw)+bufx)*4+3] = 255;
            }
            else
            {
                buffer[((bufy*bufw)+bufx)*4+0] = 0;
                buffer[((bufy*bufw)+bufx)*4+1] = 0;
                buffer[((bufy*bufw)+bufx)*4+2] = 0;
                buffer[((bufy*bufw)+bufx)*4+3] = 0;
            }
        }
    }
    else
    {
        int i,j;
        for(i = 0; i < sizex; i++) for(j = 0; j < sizey; j++)
        {
            int bufx = i + posx;
            int bufy = j + posy;

            if(sprbuffer_vis[(j/factor)*64 + (i/factor)])
            {
                buffer[((bufy*bufw)+bufx)*3+0] = sprbuffer[(j/factor)*64 + (i/factor)]&0xFF;
                buffer[((bufy*bufw)+bufx)*3+1] = (sprbuffer[(j/factor)*64 + (i/factor)]>>8)&0xFF;
                buffer[((bufy*bufw)+bufx)*3+2] = (sprbuffer[(j/factor)*64 + (i/factor)]>>16)&0xFF;
            }
            else
            {
                if( (i>=(sx*factor)) || (j>=(sy*factor)) ) if( ((i^j)&7) == 0 )
                {
                    buffer[((bufy*bufw)+bufx)*3+0] = 255;
                    buffer[((bufy*bufw)+bufx)*3+1] = 0;
                    buffer[((bufy*bufw)+bufx)*3+2] = 0;
                }
            }
        }
    }
}

//starting sprite number = page*64
void GBA_Debug_PrintSpritesPage(int page, int buf_has_alpha_channel, char * buffer, int bufw, int bufh)
{
    int i,j;
    if(buf_has_alpha_channel)
    {
        memset(buffer,0,bufw*bufh*4);
    }
    else
    {
        for(i = 0; i < bufw; i++) for(j = 0; j < bufh; j++)
        {
            buffer[(j*bufw+i)*3+0] = ((i&32)^(j&32)) ? 0x80 : 0xB0;
            buffer[(j*bufw+i)*3+1] = ((i&32)^(j&32)) ? 0x80 : 0xB0;
            buffer[(j*bufw+i)*3+2] = ((i&32)^(j&32)) ? 0x80 : 0xB0;
        }
    }

    for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
    {
        int sprnum = (page*64) + (j*8) + i;

        GBA_Debug_PrintZoomedSpriteAt(sprnum,buf_has_alpha_channel,buffer,bufw,bufh,
                                (i*(64+16))+16,(j*(64+16))+16,64,64);
    }

}

//----------------------------------------------------------------
