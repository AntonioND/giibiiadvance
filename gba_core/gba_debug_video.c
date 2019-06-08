/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
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

static u32 rgb16to32(u16 color)
{
    int r = (color & 31)<<3;
    int g = ((color >> 5) & 31)<<3;
    int b = ((color >> 10) & 31)<<3;
    return (b<<16)|(g<<8)|r;
}

static int min(int a, int b)
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

void GBA_Debug_PrintTiles(char * buffer, int bufw, int bufh, int cbb, int colors, int palette)
{
    u8 * charbaseblockptr = (u8*)&Mem.vram[cbb-0x06000000];

    int i,j;
    for(i = 0; i < 256; i++) for(j = 0; j < 256; j++)
    {
        buffer[(j*bufw+i)*3+0] = ((i&16)^(j&16)) ? 0x80 : 0xB0;
        buffer[(j*bufw+i)*3+1] = ((i&16)^(j&16)) ? 0x80 : 0xB0;
        buffer[(j*bufw+i)*3+2] = ((i&16)^(j&16)) ? 0x80 : 0xB0;
    }


    if(colors == 256) //256 Colors
    {
        int jmax = (cbb == 0x06014000) ? 64 : 128; //half size

        //cbb >= 0x06010000 --> sprite
        u32 pal = (cbb >= 0x06010000) ? 256 : 0;

        for(i = 0; i < 256; i++) for(j = 0; j < jmax; j++)
        {
            u32 Index = (i/8) + (j/8)*32;
            u8 * dataptr = (u8*)&(charbaseblockptr[(Index&0x3FF)*64]);

            int data = dataptr[(i&7)+((j&7)*8)];

            u32 color = rgb16to32(((u16*)Mem.pal_ram)[data+pal]);

            buffer[(j*bufw+i)*3+0] = color & 0xFF;
            buffer[(j*bufw+i)*3+1] = (color>>8) & 0xFF;
            buffer[(j*bufw+i)*3+2] = (color>>16) & 0xFF;
        }
        for(i = 0; i < 256; i++) for(j = jmax; j < 256; j++)
        {
            if( ((i^j)&7) == 0 )
            {
                buffer[(j*bufw+i)*3+0] = 255;
                buffer[(j*bufw+i)*3+1] = 0;
                buffer[(j*bufw+i)*3+2] = 0;
            }
        }
    }
    else if(colors == 16) //16 colors
    {
        int jmax = (cbb == 0x06014000) ? 128 : 256; //half size

        //cbb >= 0x06010000 --> sprite
        u32 pal = (cbb >= 0x06010000) ? (palette+16) : palette;
        u16 * palptr = (u16*)&Mem.pal_ram[pal*2*16];

        for(i = 0; i < 256; i++) for(j = 0; j < jmax; j++)
        {
            u32 Index = (i/8) + (j/8)*32;
            u8 * dataptr = (u8*)&(charbaseblockptr[(Index&0x3FF)*32]);

            int data = dataptr[ ((i&7)+((j&7)*8))/2 ];

            if(i&1) data = data>>4;
            else data = data & 0xF;

            u32 color = rgb16to32(palptr[data]);

            buffer[(j*bufw+i)*3+0] = color & 0xFF;
            buffer[(j*bufw+i)*3+1] = (color>>8) & 0xFF;
            buffer[(j*bufw+i)*3+2] = (color>>16) & 0xFF;
        }

        for(i = 0; i < 256; i++) for(j = jmax; j < 256; j++)
        {
            if( ((i^j)&7) == 0 )
            {
                buffer[(j*bufw+i)*3+0] = 255;
                buffer[(j*bufw+i)*3+1] = 0;
                buffer[(j*bufw+i)*3+2] = 0;
            }
        }
    }
}

void GBA_Debug_PrintTilesAlpha(char * buffer, int bufw, int bufh, int cbb, int colors, int palette)
{
    memset(buffer,0,bufw*bufh*4);

    u8 * charbaseblockptr = (u8*)&Mem.vram[cbb-0x06000000];

    int i,j;

    if(colors == 256) //256 Colors
    {
        int jmax = (cbb == 0x06014000) ? 64 : 128; //half size

        //cbb >= 0x06010000 --> sprite
        u32 pal = (cbb >= 0x06010000) ? 256 : 0;

        for(i = 0; i < 256; i++) for(j = 0; j < jmax; j++)
        {
            u32 Index = (i/8) + (j/8)*32;
            u8 * dataptr = (u8*)&(charbaseblockptr[(Index&0x3FF)*64]);

            int data = dataptr[(i&7)+((j&7)*8)];

            u32 color = rgb16to32(((u16*)Mem.pal_ram)[data+pal]);

            buffer[(j*bufw+i)*4+0] = color & 0xFF;
            buffer[(j*bufw+i)*4+1] = (color>>8) & 0xFF;
            buffer[(j*bufw+i)*4+2] = (color>>16) & 0xFF;
            buffer[(j*bufw+i)*4+3] = 255;
        }
        for(i = 0; i < 256; i++) for(j = jmax; j < 256; j++)
        {
            if( ((i^j)&7) == 0 )
            {
                buffer[(j*bufw+i)*4+0] = 255;
                buffer[(j*bufw+i)*4+1] = 0;
                buffer[(j*bufw+i)*4+2] = 0;
                buffer[(j*bufw+i)*4+3] = 255;
            }
        }
    }
    else if(colors == 16) //16 colors
    {
        int jmax = (cbb == 0x06014000) ? 128 : 256; //half size

        //cbb >= 0x06010000 --> sprite
        u32 pal = (cbb >= 0x06010000) ? (palette+16) : palette;
        u16 * palptr = (u16*)&Mem.pal_ram[pal*2*16];

        for(i = 0; i < 256; i++) for(j = 0; j < jmax; j++)
        {
            u32 Index = (i/8) + (j/8)*32;
            u8 * dataptr = (u8*)&(charbaseblockptr[(Index&0x3FF)*32]);

            int data = dataptr[ ((i&7)+((j&7)*8))/2 ];

            if(i&1) data = data>>4;
            else data = data & 0xF;

            u32 color = rgb16to32(palptr[data]);

            buffer[(j*bufw+i)*4+0] = color & 0xFF;
            buffer[(j*bufw+i)*4+1] = (color>>8) & 0xFF;
            buffer[(j*bufw+i)*4+2] = (color>>16) & 0xFF;
            buffer[(j*bufw+i)*4+3] = 255;
        }

        for(i = 0; i < 256; i++) for(j = jmax; j < 256; j++)
        {
            if( ((i^j)&7) == 0 )
            {
                buffer[(j*bufw+i)*4+0] = 255;
                buffer[(j*bufw+i)*4+1] = 0;
                buffer[(j*bufw+i)*4+2] = 0;
                buffer[(j*bufw+i)*4+3] = 255;
            }
        }
    }
}

void GBA_Debug_TilePrint64x64(char * buffer, int bufw, int bufh, int cbb, int tile, int palcolors, int selected_pal)
{
    int tiletempbuffer[8*8], tiletempvis[8*8];
    memset(tiletempvis,0,sizeof(tiletempvis));

    u8 * charbaseblockptr = (u8*)&Mem.vram[cbb-0x06000000];

    if(palcolors == 256) //256 Colors
    {
        u8 * data = (u8*)&(charbaseblockptr[(tile&0x3FF)*64]);

        //SelectedBase >= 0x06010000 --> sprite
        u32 pal = (cbb >= 0x06010000) ? 256 : 0;

        int i,j;
        for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
        {
            u8 dat_ = data[j*8 + i];

            tiletempbuffer[j*8 + i] = rgb16to32(((u16*)Mem.pal_ram)[dat_+pal]);
            tiletempvis[j*8 + i] = dat_;
        }
    }
    else if(palcolors == 16)//16 Colors
    {
        u8 * data = (u8*)&(charbaseblockptr[(tile&0x3FF)*32]);

        //cbb >= 0x06010000 --> sprite
        u32 pal = (cbb >= 0x06010000) ? (selected_pal+16) : selected_pal;
        u16 * palptr = (u16*)&Mem.pal_ram[pal*2*16];

        int i,j;
        for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
        {
            u8 dat_ = data[(j*8 + i)/2];
            if(i&1) dat_ = dat_>>4;
            else dat_ = dat_ & 0xF;

            tiletempbuffer[j*8 + i] = rgb16to32(palptr[dat_]);
            tiletempvis[j*8 + i] = dat_;
        }
    }

    //Expand to 64x64
    int i,j;
    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
    {
        buffer[(j*64+i)*3+0] = ((i&16)^(j&16)) ? 0x80 : 0xB0;
        buffer[(j*64+i)*3+1] = ((i&16)^(j&16)) ? 0x80 : 0xB0;
        buffer[(j*64+i)*3+2] = ((i&16)^(j&16)) ? 0x80 : 0xB0;
    }

    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
    {
        if(tiletempvis[(j/8)*8 + (i/8)])
        {
            u32 color = tiletempbuffer[(j/8)*8 + (i/8)];
            buffer[(j*64+i)*3+0] = color&0xFF;
            buffer[(j*64+i)*3+1] = (color>>8)&0xFF;
            buffer[(j*64+i)*3+2] = (color>>16)&0xFF;
        }
    }
}

//----------------------------------------------------------------

static u32 se_index(u32 tx, u32 ty, u32 pitch) //from tonc
{
    u32 sbb = (ty/32)*(pitch/32) + (tx/32);
    return sbb*1024 + (ty%32)*32 + tx%32;
}

static u32 se_index_affine(u32 tx, u32 ty, u32 tpitch)
{
    return (ty * tpitch) + tx;
}

//bgmode => 1 = text, 2 = affine, 3,4,5 = bmp mode 3,4,5
void GBA_Debug_PrintBackgroundAlpha(char * buffer, int bufw, int bufh, u16 control, int bgmode, int page)
{
    if(bgmode == 0) return; // shouldn't happen

    memset(buffer,0,bufw*bufh*4);

    if(bgmode == 1) //text
    {
        static const u32 text_bg_size[4][2] = { {256,256}, {512,256}, {256,512}, {512,512} };

        u8 * charbaseblockptr = (u8*)&Mem.vram[((control>>2)&3) * (16*1024)];
        u16 * scrbaseblockptr = (u16*)&Mem.vram[((control>>8)&0x1F) * (2*1024)];

        u32 sizex = text_bg_size[control>>14][0];
        u32 sizey = text_bg_size[control>>14][1];

        if(control & BIT(7)) //256 colors
        {
            int i, j;
            for(i = 0; i < sizex; i++) for(j = 0; j < sizey; j++)
            {
                u32 index = se_index(i/8,j/8,sizex/8);
                u16 SE = scrbaseblockptr[index];
                //screen entry data
                //0-9 tile id
                //10-hflip
                //11-vflip
                int _x = i & 7;
                if(SE & BIT(10)) _x = 7-_x; //hflip

                int _y = j & 7;
                if(SE & BIT(11)) _y = 7-_y; //vflip

                int data = charbaseblockptr[((SE&0x3FF)*64)  +  (_x+(_y*8))];

                u32 color = rgb16to32(((u16*)Mem.pal_ram)[data]);
                buffer[(j*bufw+i)*4+0] = color & 0xFF;
                buffer[(j*bufw+i)*4+1] = (color>>8) & 0xFF;
                buffer[(j*bufw+i)*4+2] = (color>>16) & 0xFF;
                buffer[(j*bufw+i)*4+3] = data ? 0xFF : 0;
            }
        }
        else //16 colors
        {
            int i, j;
            for(i = 0; i < sizex; i++) for(j = 0; j < sizey; j++)
            {
                u32 index = se_index(i/8,j/8,sizex/8);
                u16 SE = scrbaseblockptr[index];
                //screen entry data
                //0-9 tile id
                //10-hflip
                //11-vflip
                //12-15-pal
                u16 * palptr = (u16*)&Mem.pal_ram[(SE>>12)*(2*16)];

                int _x = i & 7;
                if(SE & BIT(10)) _x = 7-_x; //hflip

                int _y = j & 7;
                if(SE & BIT(11)) _y = 7-_y; //vflip

                u32 data = charbaseblockptr[((SE&0x3FF)*32)  +  ((_x/2)+(_y*4))];

                if(_x&1) data = data>>4;
                else data = data & 0xF;

                u32 color = rgb16to32(palptr[data]);
                buffer[(j*bufw+i)*4+0] = color & 0xFF;
                buffer[(j*bufw+i)*4+1] = (color>>8) & 0xFF;
                buffer[(j*bufw+i)*4+2] = (color>>16) & 0xFF;
                buffer[(j*bufw+i)*4+3] = data ? 0xFF : 0;
            }
        }
    }
    else if(bgmode == 2) //affine
    {
        static const u32 affine_bg_size[4] = { 128, 256, 512, 1024 };

        u8 * charbaseblockptr = (u8*)&Mem.vram[((control>>2)&3) * (16*1024)];
        u8 * scrbaseblockptr = (u8*)&Mem.vram[((control>>8)&0x1F) * (2*1024)];

        u32 size = affine_bg_size[control>>14];
        u32 tilesize = size/8;

        //always 256 color

        int i, j;
        for(i = 0; i < size; i++) for(j = 0; j < size; j++)
        {
            int _x = i & 7;
            int _y = j & 7;

            u32 index = se_index_affine(i/8,j/8,tilesize);
            u8 SE = scrbaseblockptr[index];
            u16 data = charbaseblockptr[(SE*64) + (_x+(_y*8))];

            u32 color = rgb16to32(((u16*)Mem.pal_ram)[data]);
            buffer[(j*bufw+i)*4+0] = color & 0xFF;
            buffer[(j*bufw+i)*4+1] = (color>>8) & 0xFF;
            buffer[(j*bufw+i)*4+2] = (color>>16) & 0xFF;
            buffer[(j*bufw+i)*4+3] = data ? 0xFF : 0;
        }
    }
    else if(bgmode == 3) //bg2 mode 3
    {
        u16 * srcptr = (u16*)&Mem.vram;

        int i,j;
        for(i = 0; i < 240; i++) for(j = 0; j < 160; j++)
        {
            u16 data = srcptr[i+240*j];
            u32 color = rgb16to32(data);
            buffer[(j*bufw+i)*4+0] = color & 0xFF;
            buffer[(j*bufw+i)*4+1] = (color>>8) & 0xFF;
            buffer[(j*bufw+i)*4+2] = (color>>16) & 0xFF;
            buffer[(j*bufw+i)*4+3] = 0xFF;
        }
    }
    else if(bgmode == 4) //bg2 mode 4
    {
        u8 * srcptr = (u8*)&Mem.vram[page?0xA000:0];

        int i,j;
        for(i = 0; i < 240; i++) for(j = 0; j < 160; j++)
        {
            u16 data = ((u16*)Mem.pal_ram)[srcptr[i+240*j]];
            u32 color = rgb16to32(data);
            buffer[(j*bufw+i)*4+0] = color & 0xFF;
            buffer[(j*bufw+i)*4+1] = (color>>8) & 0xFF;
            buffer[(j*bufw+i)*4+2] = (color>>16) & 0xFF;
            buffer[(j*bufw+i)*4+3] = 0xFF;
        }
    }
    else if(bgmode == 5) //bg2 mode 5
    {
        u16 * srcptr = (u16*)&Mem.vram[page?0xA000:0];

        int i,j;
        for(i = 0; i < 160; i++) for(j = 0; j < 128; j++)
        {
            u16 data = srcptr[i+160*j];
            u32 color = rgb16to32(data);
            buffer[(j*bufw+i)*4+0] = color & 0xFF;
            buffer[(j*bufw+i)*4+1] = (color>>8) & 0xFF;
            buffer[(j*bufw+i)*4+2] = (color>>16) & 0xFF;
            buffer[(j*bufw+i)*4+3] = 0xFF;
        }
    }
}

//----------------------------------------------------------------
