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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../build_options.h"
#include "../general_utils.h"
#include "../file_utils.h"
#include "../debug_utils.h"

#include "gameboy.h"
#include "rom.h"
#include "general.h"
#include "video.h"
#include "cpu.h"
#include "sound.h"
#include "sgb.h"

#include "../png/png_utils.h"
#include "gb_main.h"

extern _GB_CONTEXT_ GameBoy;

int GB_Input_Get(int player);
void GB_Input_Update(void);

//---------------------------------

int GB_MainLoad(const char * rom_path)
{
    void * ptr; u32 size;
    FileLoad(rom_path,&ptr,&size);

    if(ptr)
    {
        if(Cartridge_Load(ptr,size))
        {
            //Init after loading the cartridge to set the hardware type value and allow
            //GB_Screen_Init choose the correct dimensions for the texture.

            Cardridge_Set_Filename((char*)rom_path);

            SRAM_Load();
            RTC_Load();
            GB_PowerOn();
            GB_Frameskip(0);
            return 1;
        }

        Debug_ErrorMsgArg("Error while loading cartridge.\n"
                    "Read the console output for details.");

        free(ptr);
        return 0;
    }

    Debug_ErrorMsgArg("Couldn't load data from %s.",rom_path);

    return 0;
}

void GB_End(int save)
{
    if(save)
    {
        SRAM_Save();
        RTC_Save();
    }
    GB_PowerOff();
    Cartridge_Unload();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

int __gb_GetFrameDrawnAndClearFlag(void)
{
    int ret = GameBoy.Emulator.FrameDrawn;
    GameBoy.Emulator.FrameDrawn = 0;
    return ret;
}

void GB_RunForOneFrame(void)
{
    while(__gb_GetFrameDrawnAndClearFlag() == 0) GB_RunForInstruction();
}

int GB_IsEnabledSGB(void)
{
    return GameBoy.Emulator.SGBEnabled;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

int scr_texture_loaded;

extern _GB_CONTEXT_ GameBoy;
extern u16 framebuffer[2][256 * 224];
extern u32 cur_fb;
extern u32 blur;
extern u32 realcolors;

int GB_Screen_Init(void)
{
    memset(framebuffer[0],0,sizeof(framebuffer[0]));
    memset(framebuffer[1],0,sizeof(framebuffer[1]));
    return 0;
}

inline void GB_Screen_WritePixel(char * buffer, int x, int y, int r, int g, int b)
{
    u8 * p = (u8*)buffer + (y*160 + x) * 3;
    *p++ = r; *p++ = g; *p = b;
}

inline void GB_Screen_WritePixelSGB(char * buffer, int x, int y, int r, int g, int b)
{
    u8 * p = (u8*)buffer + (y*(160+96) + x) * 3;
    *p++ = r; *p++ = g; *p = b;
}

void gb_scr_writebuffer_sgb(char * buffer)
{
    int last_fb = cur_fb ^ 1;
    int i,j;
    for(i = 0; i < 256; i++) for(j = 0; j < 224; j++)
    {
        int data = framebuffer[last_fb][j*256 + i];
        int r = data & 0x1F; int g = (data>>5) & 0x1F; int b = (data>>10) & 0x1F;
        GB_Screen_WritePixelSGB(buffer, i,j, r<<3,g<<3,b<<3);
    }
}

void gb_scr_writebuffer_dmg_cgb(char * buffer)
{
    int last_fb = cur_fb ^ 1;
    int i,j;
    for(i = 0; i < 160; i++) for(j = 0; j < 144; j++)
    {
        int data = framebuffer[last_fb][j*256 + i];
        int r = data & 0x1F; int g = (data>>5) & 0x1F; int b = (data>>10) & 0x1F;
        GB_Screen_WritePixel(buffer, i,j, r<<3,g<<3,b<<3);
    }
}

void gb_scr_writebuffer_dmg_cgb_blur(char * buffer)
{
    int i,j;
    for(i = 0; i < 160; i++) for(j = 0; j < 144; j++)
    {
        int data1 = framebuffer[0][j*256 + i];
        int r1 = data1 & 0x1F; int g1 = (data1>>5) & 0x1F; int b1 = (data1>>10) & 0x1F;
        int data2 = framebuffer[1][j*256 + i];
        int r2 = data2 & 0x1F; int g2 = (data2>>5) & 0x1F; int b2 = (data2>>10) & 0x1F;
        int r = (r1+r2)<<2; int g = (g1+g2)<<2; int b = (b1+b2)<<2;
        GB_Screen_WritePixel(buffer, i,j, r,g,b);
    }
}

//Real colors:
// R = ((r * 13 + g * 2 + b) >> 1)
// G = ((g * 3 + b) << 1)
// B = ((r * 3 + g * 2 + b * 11) >> 1)

void gb_scr_writebuffer_dmg_cgb_realcolors(char * buffer)
{
    int last_fb = cur_fb ^ 1;
    int i,j;
    for(i = 0; i < 160; i++) for(j = 0; j < 144; j++)
    {
        int data = framebuffer[last_fb][j*256 + i];
        int r = data & 0x1F; int g = (data>>5) & 0x1F; int b = (data>>10) & 0x1F;
        int _r =  ((r * 13 + g * 2 + b) >> 1);
        int _g = (g * 3 + b) << 1;
        int _b = ((r * 3 + g * 2 + b * 11) >> 1);
        GB_Screen_WritePixel(buffer, i,j, _r,_g,_b);
    }
}

void gb_scr_writebuffer_dmg_cgb_blur_realcolors(char * buffer)
{
    int i,j;
    for(i = 0; i < 160; i++) for(j = 0; j < 144; j++)
    {
        for(i = 0; i < 160; i++) for(j = 0; j < 144; j++)
        {
            int data1 = framebuffer[0][j*256 + i];
            int r1 = data1 & 0x1F; int g1 = (data1>>5) & 0x1F; int b1 = (data1>>10) & 0x1F;
            int data2 = framebuffer[1][j*256 + i];
            int r2 = data2 & 0x1F; int g2 = (data2>>5) & 0x1F; int b2 = (data2>>10) & 0x1F;
            int r = (r1+r2)>>1; int g = (g1+g2)>>1; int b = (b1+b2)>>1;
            int _r =  ((r * 13 + g * 2 + b) >> 1);
            int _g = (g * 3 + b) << 1;
            int _b = ((r * 3 + g * 2 + b * 11) >> 1);
            GB_Screen_WritePixel(buffer, i,j, _r,_g,_b);
        }
    }
}

void GB_Screen_WriteBuffer_24RGB(char * buffer)
{
    if( (GameBoy.Emulator.HardwareType == HW_SGB) || (GameBoy.Emulator.HardwareType == HW_SGB2) )
    {
        gb_scr_writebuffer_sgb(buffer);
    }
    else if(GameBoy.Emulator.HardwareType == HW_GBA)
    {
        if(blur) gb_scr_writebuffer_dmg_cgb_blur(buffer);
        else gb_scr_writebuffer_dmg_cgb(buffer);
    }
    else
    {
        if(blur)
        {
            if(realcolors) gb_scr_writebuffer_dmg_cgb_blur_realcolors(buffer);
            else gb_scr_writebuffer_dmg_cgb_blur(buffer);
        }
        else
        {
            if(realcolors) gb_scr_writebuffer_dmg_cgb_realcolors(buffer);
            else gb_scr_writebuffer_dmg_cgb(buffer);
        }
    }
}

int Keys[4];

void GB_InputSet(int player, int a, int b, int st, int se, int r, int l, int u, int d)
{
    Keys[player] = 0;
    if(l) Keys[player] |= KEY_LEFT;
    if(u) Keys[player] |= KEY_UP;
    if(r) Keys[player] |= KEY_RIGHT;
    if(d) Keys[player] |= KEY_DOWN;
    if(a) Keys[player] |= KEY_A;
    if(b) Keys[player] |= KEY_B;
    if(st) Keys[player] |= KEY_START;
    if(se) Keys[player] |= KEY_SELECT;
}

void GB_InputSetMBC7(int x, int y)
{
    GameBoy.Emulator.MBC7.sensorY = 2047 + y;
    GameBoy.Emulator.MBC7.sensorX = 2047 + x;
}

void GB_InputSetMBC7Buttons(int up, int down, int right, int left)
{
    if(up)
    {
        if(GameBoy.Emulator.MBC7.sensorY < 2047 - 50)
            GameBoy.Emulator.MBC7.sensorY += 100;
        else if(GameBoy.Emulator.MBC7.sensorY < 2047 + 100)
            GameBoy.Emulator.MBC7.sensorY += 2;
    }
    else if(down)
    {
        if(GameBoy.Emulator.MBC7.sensorY > 2047 + 50)
            GameBoy.Emulator.MBC7.sensorY -= 100;
        else if(GameBoy.Emulator.MBC7.sensorY > 2047 - 100)
            GameBoy.Emulator.MBC7.sensorY -= 2;
    }
    else
    {
        if(GameBoy.Emulator.MBC7.sensorY > 2047) GameBoy.Emulator.MBC7.sensorY -= 1;
        else GameBoy.Emulator.MBC7.sensorY += 1;
    }

    if(left)
    {
        if(GameBoy.Emulator.MBC7.sensorX < 2047 + 100)
            GameBoy.Emulator.MBC7.sensorX += 2;
    }
    else if(right)
    {
        if(GameBoy.Emulator.MBC7.sensorX > 2047 - 100)
            GameBoy.Emulator.MBC7.sensorX -= 2;
    }
    else
    {
        if(GameBoy.Emulator.MBC7.sensorX > 2047) GameBoy.Emulator.MBC7.sensorX -= 1;
        else GameBoy.Emulator.MBC7.sensorX += 1;
    }
}

int GB_Input_Get(int player)
{
    return Keys[player];
}

static int screenshot_file_number = 0;

void GB_Screenshot(void)
{
	char filename[MAX_PATHLEN];

	while(1)
	{
		s_snprintf(filename,sizeof(filename),"%sgb_screenshot%d.png",DirGetScreenshotFolderPath(),
             screenshot_file_number);

		FILE * file = fopen(filename, "rb");
		if(file == NULL) break; //Ok
		screenshot_file_number ++; //look for next free number
		fclose(file);
	}

	int width, height;

	if(GameBoy.Emulator.SGBEnabled)
	{
	    width = 256; height = 224;
	}
	else
	{
	    width = 160; height = 144;
	}

    u32 * buf_temp = calloc(width*height*4,1);
    int last_fb = cur_fb ^ 1;
	int x, y;
	for(y = 0; y < height; y ++) for(x = 0; x < width; x ++)
	{
	    u32 data = framebuffer[last_fb][y*256 + x];
	    buf_temp[y*width + x] = ((data&0x1F)<<3)|((((data>>5)&0x1F)<<3)<<8)|
            ((((data>>10)&0x1F)<<3)<<16);
	}

    Save_PNG(filename,width,height,buf_temp,0);
    free(buf_temp);
}

u32 pal_red,pal_green,pal_blue;

void menu_get_gb_palette(u8 * red, u8 * green, u8 * blue)
{
    *red = pal_red;
    *green = pal_green;
    *blue = pal_blue;
}

void menu_set_gb_palette(u8 red, u8 green, u8 blue)
{
    pal_red = red;
    pal_green = green;
    pal_blue = blue;
}

void menu_load_gb_palete_from_config(void)
{
    GB_SetPalette(pal_red,pal_green,pal_blue);
}


