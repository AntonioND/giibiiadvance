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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../general_utils.h"
#include "../font_utils.h"

#include "gameboy.h"
#include "general.h"
#include "cpu.h"
#include "debug_video.h"
#include "memory.h"
#include "video.h"

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------------------------

u32 gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

void GB_Debug_GetPalette(int is_sprite, int num, int color, u32 * red, u32 * green, u32 * blue)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    if(GameBoy.Emulator.CGBEnabled)
    {
        u32 color_;

        if(is_sprite)
        {
            color_ = GameBoy.Emulator.spr_pal[(num*8)+(color*2)] |
                     GameBoy.Emulator.spr_pal[(num*8)+(color*2)+1]<<8;
        }
        else
        {
            color_ = GameBoy.Emulator.bg_pal[(num*8)+(color*2)] |
                     GameBoy.Emulator.bg_pal[(num*8)+(color*2)+1]<<8;
        }

        *red = (color_ & 0x1F) << 3;
        *green = ((color_>>5) & 0x1F) << 3;
        *blue = ((color_>>10) & 0x1F) << 3;
    }
    else if(GameBoy.Emulator.gbc_in_gb_mode)
    {
        if(is_sprite)
        {
            if(num == 0)
            {
                u32 obp0_reg = mem->IO_Ports[OBP0_REG-0xFF00];
                u32 out_color = (obp0_reg>>(color*2))&0x3;
                out_color = gbc_getsprpalcolor(0,out_color);
                *red = (out_color & 0x1F) << 3;
                *green = ((out_color>>5) & 0x1F) << 3;
                *blue = ((out_color>>10) & 0x1F) << 3;
            }
            else if(num == 1)
            {
                u32 obp1_reg = mem->IO_Ports[OBP1_REG-0xFF00];
                u32 out_color = (obp1_reg>>(color*2))&0x3;
                out_color = gbc_getsprpalcolor(1,out_color);
                *red = (out_color & 0x1F) << 3;
                *green = ((out_color>>5) & 0x1F) << 3;
                *blue = ((out_color>>10) & 0x1F) << 3;
            }
            else
            {
                *red = *green = *blue = 0;
            }
        }
        else
        {
            if(num == 0)
            {
                u32 bgp_reg = mem->IO_Ports[BGP_REG-0xFF00];
                u32 out_color = (bgp_reg>>(color*2))&0x3;
                out_color = gbc_getbgpalcolor(0,out_color);
                *red = (out_color & 0x1F) << 3;
                *green = ((out_color>>5) & 0x1F) << 3;
                *blue = ((out_color>>10) & 0x1F) << 3;
            }
            else
            {
                *red = *green = *blue = 0;
            }
        }
    }
    else
    {
        if(is_sprite)
        {
            if(num == 0)
            {
                u32 obp0_reg = mem->IO_Ports[OBP0_REG-0xFF00];
                u32 out_color = (obp0_reg>>(color*2))&0x3;
                *red = gb_pal_colors[out_color][0];
                *green = gb_pal_colors[out_color][1];
                *blue = gb_pal_colors[out_color][2];
            }
            else if(num == 1)
            {
                u32 obp1_reg = mem->IO_Ports[OBP1_REG-0xFF00];
                u32 pal = (obp1_reg>>(color*2))&0x3;
                *red = gb_pal_colors[pal][0];
                *green = gb_pal_colors[pal][1];
                *blue = gb_pal_colors[pal][2];
            }
            else
            {
                *red = *green = *blue = 0;
            }
        }
        else
        {
            if(num == 0)
            {
                u32 bgp_reg = mem->IO_Ports[BGP_REG-0xFF00];
                u32 out_color = (bgp_reg>>(color*2))&0x3;
                *red = gb_pal_colors[out_color][0];
                *green = gb_pal_colors[out_color][1];
                *blue = gb_pal_colors[out_color][2];
            }
            else
            {
                *red = *green = *blue = 0;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

void GB_Debug_GetSpriteInfo(int sprnum, u8 * x, u8 * y, u8 * tile, u8 * info)
{
    _GB_OAM_ENTRY_ * GB_Sprite = &(((_GB_OAM_*)(GameBoy.Memory.ObjAttrMem))->Sprite[sprnum]);

    *x = GB_Sprite->X;
    *y = GB_Sprite->Y;
    *tile = GB_Sprite->Tile;
    *info = GB_Sprite->Info;
}

void GB_Debug_PrintSprite(char * buf, int bufw, int bufh, int buff_x, int buff_y, int sprite)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;
    s32 spriteheight =  8 << ((mem->IO_Ports[LCDC_REG-0xFF00] & (1<<2)) != 0);
    _GB_OAM_ENTRY_ * GB_Sprite = &(((_GB_OAM_*)(mem->ObjAttrMem))->Sprite[sprite]);
    u32 tile = GB_Sprite->Tile & ((spriteheight == 16) ? 0xFE : 0xFF);
    u8 * spr_data = &mem->VideoRAM[tile<<4]; //Bank 0

    u32 x, y;

    bool isvisible = false;
    if((GB_Sprite->X > 0) && (GB_Sprite->X < 168))
    {
        if(spriteheight == 16)
        {
            if((GB_Sprite->Y > 0) && (GB_Sprite->Y < 160)) isvisible = true;
        }
        else
        {
            if((GB_Sprite->Y > 8) && (GB_Sprite->Y < 152)) isvisible = true;
        }
    }

    if(GameBoy.Emulator.CGBEnabled)
    {
        if(GB_Sprite->Info & (1<<3)) spr_data += 0x2000; //If bank 1...

        u32 pal_index = (GB_Sprite->Info&7) * 8;

        u32 palette[4];
        palette[0] = GameBoy.Emulator.spr_pal[pal_index] | (GameBoy.Emulator.spr_pal[pal_index+1]<<8);
        palette[1] = GameBoy.Emulator.spr_pal[pal_index+2] | (GameBoy.Emulator.spr_pal[pal_index+3]<<8);
        palette[2] = GameBoy.Emulator.spr_pal[pal_index+4] | (GameBoy.Emulator.spr_pal[pal_index+5]<<8);
        palette[3] = GameBoy.Emulator.spr_pal[pal_index+6] | (GameBoy.Emulator.spr_pal[pal_index+7]<<8);

        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            int i,j;
            for(i = 0; i < 2; i ++) for(j = 0; j < 2; j++)
            {
                if(!isvisible && (((((x*2)+i)^((y*2)+j))&3)==0) )
                {
                    int xx_ = buff_x+(x*2)+i;
                    int yy_ = buff_y+(y*2)+j;
                    buf[(yy_*bufw+xx_)*3 + 0] = 255;
                    buf[(yy_*bufw+xx_)*3 + 1] = 0;
                    buf[(yy_*bufw+xx_)*3 + 2] = 0;
                }
                else
                {
                    if(color != 0) // don't display transparent color
                    {
                        int xx_ = buff_x+(x*2)+i;
                        int yy_ = buff_y+(y*2)+j;
                        buf[(yy_*bufw+xx_)*3 + 0] = (palette[color]&0x1F)<<3;
                        buf[(yy_*bufw+xx_)*3 + 1] = ((palette[color]>>5)&0x1F)<<3;
                        buf[(yy_*bufw+xx_)*3 + 2] = ((palette[color]>>10)&0x1F)<<3;
                    }
                }
            }
        }
    }
    else
    {
        u32 palette[4];
        u32 objX_reg;
        if(GB_Sprite->Info & (1<<4)) objX_reg = mem->IO_Ports[OBP1_REG-0xFF00];
        else objX_reg = mem->IO_Ports[OBP0_REG-0xFF00];

        palette[0] = objX_reg & 0x3;
		palette[1] = (objX_reg>>2) & 0x3;
		palette[2] = (objX_reg>>4) & 0x3;
		palette[3] = (objX_reg>>6) & 0x3;

        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            int i,j;
            for(i = 0; i < 2; i ++) for(j = 0; j < 2; j++)

            for(i = 0; i < 2; i ++) for(j = 0; j < 2; j++)
            {
                if(!isvisible && (((((x*2)+i)^((y*2)+j))&3)==0)  )
                {
                    int xx_ = buff_x+(x*2)+i;
                    int yy_ = buff_y+(y*2)+j;
                    buf[(yy_*bufw+xx_)*3 + 0] = 255;
                    buf[(yy_*bufw+xx_)*3 + 1] = 0;
                    buf[(yy_*bufw+xx_)*3 + 2] = 0;
                }
                else
                {
                    if(color != 0) // don't display transparent color
                    {
                        int xx_ = buff_x+(x*2)+i;
                        int yy_ = buff_y+(y*2)+j;
                        buf[(yy_*bufw+xx_)*3 + 0] = gb_pal_colors[palette[color]][0];
                        buf[(yy_*bufw+xx_)*3 + 1] = gb_pal_colors[palette[color]][1];
                        buf[(yy_*bufw+xx_)*3 + 2] = gb_pal_colors[palette[color]][2];
                    }
                }
            }
        }
    }
}

void GB_Debug_PrintSprites(char * buf)
{
    u32 x, y;
    for(y = 0; y < 256; y ++) for(x = 0; x < 256; x++)
    {
        buf[(y*256+x)*3 + 0] = ((x^y)&4)?192:128;
        buf[(y*256+x)*3 + 1] = ((x^y)&4)?192:128;
        buf[(y*256+x)*3 + 2] = ((x^y)&4)?192:128;
    }

    u32 i = 0;
    for(y = 0; y < 5; y ++) for(x = 0; x < 8; x ++)
        GB_Debug_PrintSprite(buf,256,256, 8+(x*32), 10+(y*52), i++);
}

void GB_Debug_PrintZoomedSprite(char * buf, int sprite)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;
    s32 spriteheight =  8 << ((mem->IO_Ports[LCDC_REG-0xFF00] & (1<<2)) != 0);
    _GB_OAM_ENTRY_ * GB_Sprite = &(((_GB_OAM_*)(mem->ObjAttrMem))->Sprite[sprite]);
    u32 tile = GB_Sprite->Tile & ((spriteheight == 16) ? 0xFE : 0xFF);
    u8 * spr_data = &mem->VideoRAM[tile<<4]; //Bank 0

    u32 x, y;
    for(y = 0; y < 16*8; y ++) for(x = 0; x < 8*8; x++)
    {
        buf[(y*(8*8)+x)*3 + 0] = ((x^y)&4)?192:128;
        buf[(y*(8*8)+x)*3 + 1] = ((x^y)&4)?192:128;
        buf[(y*(8*8)+x)*3 + 2] = ((x^y)&4)?192:128;
    }

    if(GameBoy.Emulator.CGBEnabled)
    {
        if(GB_Sprite->Info & (1<<3)) spr_data += 0x2000; //If bank 1...

        u32 pal_index = (GB_Sprite->Info&7) * 8;

        u32 palette[4];
        palette[0] = GameBoy.Emulator.spr_pal[pal_index] | (GameBoy.Emulator.spr_pal[pal_index+1]<<8);
        palette[1] = GameBoy.Emulator.spr_pal[pal_index+2] | (GameBoy.Emulator.spr_pal[pal_index+3]<<8);
        palette[2] = GameBoy.Emulator.spr_pal[pal_index+4] | (GameBoy.Emulator.spr_pal[pal_index+5]<<8);
        palette[3] = GameBoy.Emulator.spr_pal[pal_index+6] | (GameBoy.Emulator.spr_pal[pal_index+7]<<8);

        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            int i, j;
            for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
            {
                if(color != 0) // don't display transparent color
                {
                    int xx_ = (x*8)+i;
                    int yy_ = (y*8)+j;
                    buf[(yy_*(8*8)+xx_)*3 + 0] = (palette[color]&0x1F)<<3;
                    buf[(yy_*(8*8)+xx_)*3 + 1] = ((palette[color]>>5)&0x1F)<<3;
                    buf[(yy_*(8*8)+xx_)*3 + 2] = ((palette[color]>>10)&0x1F)<<3;
                }
            }
        }
    }
    else
    {
        u32 palette[4];
        u32 objX_reg;
        if(GB_Sprite->Info & (1<<4)) objX_reg = mem->IO_Ports[OBP1_REG-0xFF00];
        else objX_reg = mem->IO_Ports[OBP0_REG-0xFF00];

        palette[0] = objX_reg & 0x3;
		palette[1] = (objX_reg>>2) & 0x3;
		palette[2] = (objX_reg>>4) & 0x3;
		palette[3] = (objX_reg>>6) & 0x3;

        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            int i, j;
            for(i = 0; i < 8; i++) for(j = 0; j < 8; j++)
            {
                if(color != 0) // don't display transparent color
                {
                    int xx_ = (x*8)+i;
                    int yy_ = (y*8)+j;
                    buf[(yy_*(8*8)+xx_)*3 + 0] = gb_pal_colors[palette[color]][0];
                    buf[(yy_*(8*8)+xx_)*3 + 1] = gb_pal_colors[palette[color]][1];
                    buf[(yy_*(8*8)+xx_)*3 + 2] = gb_pal_colors[palette[color]][2];
                }
            }
        }
    }
}

//------------------------------------------------------------------------------------------------

static void _gb_debug_print_sprite_alpha_at_position(char * buf, int bufw, int bufh, int buff_x, int buff_y, int sprite)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;
    s32 spriteheight =  8 << ((mem->IO_Ports[LCDC_REG-0xFF00] & (1<<2)) != 0);
    _GB_OAM_ENTRY_ * GB_Sprite = &(((_GB_OAM_*)(mem->ObjAttrMem))->Sprite[sprite]);
    u32 tile = GB_Sprite->Tile & ((spriteheight == 16) ? 0xFE : 0xFF);
    u8 * spr_data = &mem->VideoRAM[tile<<4]; //Bank 0

    u32 x, y;

    if(GameBoy.Emulator.CGBEnabled)
    {
        if(GB_Sprite->Info & (1<<3)) spr_data += 0x2000; //If bank 1...

        u32 pal_index = (GB_Sprite->Info&7) * 8;

        u32 palette[4];
        palette[0] = GameBoy.Emulator.spr_pal[pal_index] | (GameBoy.Emulator.spr_pal[pal_index+1]<<8);
        palette[1] = GameBoy.Emulator.spr_pal[pal_index+2] | (GameBoy.Emulator.spr_pal[pal_index+3]<<8);
        palette[2] = GameBoy.Emulator.spr_pal[pal_index+4] | (GameBoy.Emulator.spr_pal[pal_index+5]<<8);
        palette[3] = GameBoy.Emulator.spr_pal[pal_index+6] | (GameBoy.Emulator.spr_pal[pal_index+7]<<8);

        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            int i,j;
            for(i = 0; i < 2; i ++) for(j = 0; j < 2; j++)
            {
                int xx_ = buff_x+(x*2)+i;
                int yy_ = buff_y+(y*2)+j;

                if(color != 0) // don't display transparent color
                {
                    buf[(yy_*bufw+xx_)*4 + 0] = (palette[color]&0x1F)<<3;
                    buf[(yy_*bufw+xx_)*4 + 1] = ((palette[color]>>5)&0x1F)<<3;
                    buf[(yy_*bufw+xx_)*4 + 2] = ((palette[color]>>10)&0x1F)<<3;
                    buf[(yy_*bufw+xx_)*4 + 3] = 255;
                }
                else
                {
                    buf[(yy_*bufw+xx_)*4 + 0] = 0;
                    buf[(yy_*bufw+xx_)*4 + 1] = 0;
                    buf[(yy_*bufw+xx_)*4 + 2] = 0;
                    buf[(yy_*bufw+xx_)*4 + 3] = 0;
                }
            }
        }
    }
    else
    {
        u32 palette[4];
        u32 objX_reg;
        if(GB_Sprite->Info & (1<<4)) objX_reg = mem->IO_Ports[OBP1_REG-0xFF00];
        else objX_reg = mem->IO_Ports[OBP0_REG-0xFF00];

        palette[0] = objX_reg & 0x3;
		palette[1] = (objX_reg>>2) & 0x3;
		palette[2] = (objX_reg>>4) & 0x3;
		palette[3] = (objX_reg>>6) & 0x3;

        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            int i,j;
            for(i = 0; i < 2; i ++) for(j = 0; j < 2; j++)

            for(i = 0; i < 2; i ++) for(j = 0; j < 2; j++)
            {
                int xx_ = buff_x+(x*2)+i;
                int yy_ = buff_y+(y*2)+j;

                if(color != 0) // don't display transparent color
                {
                    buf[(yy_*bufw+xx_)*4 + 0] = gb_pal_colors[palette[color]][0];
                    buf[(yy_*bufw+xx_)*4 + 1] = gb_pal_colors[palette[color]][1];
                    buf[(yy_*bufw+xx_)*4 + 2] = gb_pal_colors[palette[color]][2];
                    buf[(yy_*bufw+xx_)*4 + 3] = 255;
                }
                else
                {
                    buf[(yy_*bufw+xx_)*4 + 0] = 0;
                    buf[(yy_*bufw+xx_)*4 + 1] = 0;
                    buf[(yy_*bufw+xx_)*4 + 2] = 0;
                    buf[(yy_*bufw+xx_)*4 + 3] = 0;
                }
            }
        }
    }
}

void GB_Debug_PrintSpritesAlpha(char * buf)
{
    memset(buf,0,256*256*4);

    u32 x, y;
    u32 i = 0;
    for(y = 0; y < 5; y ++) for(x = 0; x < 8; x ++)
        _gb_debug_print_sprite_alpha_at_position(buf,256,256, 8+(x*32), 10+(y*52), i++);
}

void GB_Debug_PrintSpriteAlpha(char * buf, int sprite)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;
    s32 spriteheight =  8 << ((mem->IO_Ports[LCDC_REG-0xFF00] & (1<<2)) != 0);
    _GB_OAM_ENTRY_ * GB_Sprite = &(((_GB_OAM_*)(mem->ObjAttrMem))->Sprite[sprite]);
    u32 tile = GB_Sprite->Tile & ((spriteheight == 16) ? 0xFE : 0xFF);
    u8 * spr_data = &mem->VideoRAM[tile<<4]; //Bank 0

    u32 x, y;

    if(GameBoy.Emulator.CGBEnabled)
    {
        if(GB_Sprite->Info & (1<<3)) spr_data += 0x2000; //If bank 1...

        u32 pal_index = (GB_Sprite->Info&7) * 8;

        u32 palette[4];
        palette[0] = GameBoy.Emulator.spr_pal[pal_index] | (GameBoy.Emulator.spr_pal[pal_index+1]<<8);
        palette[1] = GameBoy.Emulator.spr_pal[pal_index+2] | (GameBoy.Emulator.spr_pal[pal_index+3]<<8);
        palette[2] = GameBoy.Emulator.spr_pal[pal_index+4] | (GameBoy.Emulator.spr_pal[pal_index+5]<<8);
        palette[3] = GameBoy.Emulator.spr_pal[pal_index+6] | (GameBoy.Emulator.spr_pal[pal_index+7]<<8);

        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            if(color != 0) // don't display transparent color
            {
                buf[(y*8+x)*4 + 0] = (palette[color]&0x1F)<<3;
                buf[(y*8+x)*4 + 1] = ((palette[color]>>5)&0x1F)<<3;
                buf[(y*8+x)*4 + 2] = ((palette[color]>>10)&0x1F)<<3;
                buf[(y*8+x)*4 + 3] = 255;
            }
            else
            {
                buf[(y*8+x)*4 + 0] = 0;
                buf[(y*8+x)*4 + 1] = 0;
                buf[(y*8+x)*4 + 2] = 0;
                buf[(y*8+x)*4 + 3] = 0;
            }
        }
    }
    else
    {
        u32 palette[4];
        u32 objX_reg;
        if(GB_Sprite->Info & (1<<4)) objX_reg = mem->IO_Ports[OBP1_REG-0xFF00];
        else objX_reg = mem->IO_Ports[OBP0_REG-0xFF00];

        palette[0] = objX_reg & 0x3;
		palette[1] = (objX_reg>>2) & 0x3;
		palette[2] = (objX_reg>>4) & 0x3;
		palette[3] = (objX_reg>>6) & 0x3;

        for(y = 0; y < spriteheight; y++) for(x = 0; x < 8; x ++)
        {
            u8 * data = spr_data;

            if(GB_Sprite->Info & (1<<6)) data += (spriteheight-y-1)*2;  //flip Y
            else data += y*2;

            int x_;

            if(GB_Sprite->Info & (1<<5)) x_ = 7-x; //flip X
			else x_ = x;

            x_ = 7-x_;

            u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            if(color != 0) // don't display transparent color
            {
                buf[(y*8+x)*4 + 0] = gb_pal_colors[palette[color]][0];
                buf[(y*8+x)*4 + 1] = gb_pal_colors[palette[color]][1];
                buf[(y*8+x)*4 + 2] = gb_pal_colors[palette[color]][2];
                buf[(y*8+x)*4 + 3] = 255;
            }
            else
            {
                buf[(y*8+x)*4 + 0] = 0;
                buf[(y*8+x)*4 + 1] = 0;
                buf[(y*8+x)*4 + 2] = 0;
                buf[(y*8+x)*4 + 3] = 0;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------

void GB_Debug_TileVRAMDraw(char * buffer0, int bufw0, int bufh0, char * buffer1, int bufw1, int bufh1)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;
	u32 y, x;

	for(y = 0; y < 192 ; y++)
	{
		for(x = 0; x < 128; x++)
		{
			u32 tile = (x>>3) + ((y>>3)*16);

			u8 * data = &mem->VideoRAM[tile<<4]; //Bank 0

			data += ( (y&7)*2 );

			u32 x_ = 7-(x&7);

			u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            buffer0[(y*bufw0+x)*3+0] = gb_pal_colors[color][0];
            buffer0[(y*bufw0+x)*3+1] = gb_pal_colors[color][1];
            buffer0[(y*bufw0+x)*3+2] = gb_pal_colors[color][2];
		}
	}

	for(y = 0; y < 192 ; y++)
	{
		for(x = 0; x < 128; x++)
		{
			u32 tile = (x>>3) + ((y>>3)*16);

			u8 * data = &mem->VideoRAM[tile<<4];
			data += 0x2000; //Bank 1;

			data += ( (y&7)*2 );

			u32 x_ = 7-(x&7);

			u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

            buffer1[(y*bufw1+x)*3+0] = gb_pal_colors[color][0];
            buffer1[(y*bufw1+x)*3+1] = gb_pal_colors[color][1];
            buffer1[(y*bufw1+x)*3+2] = gb_pal_colors[color][2];
		}
	}
}

#if 0
static void gb_tile_viewer_update_tile(void)
{
    int tiletempbuffer[8*8];

    static const u32 gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };

    u8 * tile_data = &GameBoy.Memory.VideoRAM[(SelTileX + (SelTileY*16))<<4]; //Bank 0
    if(SelBank) tile_data += 0x2000; //Bank 1;

	u32 y, x;
	for(y = 0; y < 8 ; y++) for(x = 0; x < 8; x++)
    {
        u8 * data = tile_data + ( (y&7)*2 );
        u32 x_ = 7-(x&7);
        u32 color = ( (*data >> x_) & 1 ) |  ( ( ( (*(data+1)) >> x_)  << 1) & 2);

        tiletempbuffer[x + y*8] = (gb_pal_colors[color][0]<<16)|(gb_pal_colors[color][1]<<8)|
                gb_pal_colors[color][2];
    }

    u32 tile = (SelTileX + (SelTileY*16));
    u32 tileindex = (tile > 255) ? (tile - 256) : (tile);
    if(GameBoy.Emulator.CGBEnabled)
    {
        char text[1000];
        sprintf(text,"Tile: %d(%d)\r\nAddr: 0x%04X\r\nBank: %d",tile,tileindex,
            0x8000 + (tile * 16),SelBank);
        SetWindowText(hTileText,(LPCTSTR)text);
    }
    else
    {
        char text[1000];
        sprintf(text,"Tile: %d(%d)\r\nAddr: 0x%04X\r\nBank: -",tile,tileindex,
            0x8000 + (tile * 16));
        SetWindowText(hTileText,(LPCTSTR)text);
    }

    //Expand to 64x64
    int i,j;
    for(i = 0; i < 64; i++) for(j = 0; j < 64; j++)
    {
        SelectedTileBuffer[j*64+i] = tiletempbuffer[(j/8)*8 + (i/8)];
    }

    //Update window
    RECT rc; rc.top = 133; rc.left = 5; rc.bottom = 133+64; rc.right = 5+64;
    InvalidateRect(hWndTileViewer, &rc, FALSE);
}
#endif

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------



