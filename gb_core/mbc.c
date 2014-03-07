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
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../general_utils.h"
#include "../debug_utils.h"

#include "gameboy.h"
#include "cpu.h"
#include "debug.h"
#include "memory.h"
#include "interrupts.h"
#include "general.h"
#include "sound.h"
#include "sgb.h"
#include "video.h"
#include "gb_main.h"

extern _GB_CONTEXT_ GameBoy;

/*
Gamegenie/Shark Cheats
----------------------

Game Shark and Gamegenie are external cartridge adapters that can be plugged
between the gameboy and the actual game cartridge. Hexadecimal codes can be
then entered for specific games, typically providing things like Infinite Sex,
255 Cigarettes, or Starting directly in Wonderland Level PRO, etc.

Gamegenie (ROM patches)
Gamegenie codes consist of nine-digit hex numbers, formatted as ABC-DEF-GHI,
the meaning of the separate digits is:
  AB    New data
  FCDE  Memory address, XORed by 0F000h
  GI    Old data, XORed by 0BAh and rotated left by two
  H     Don't know, maybe checksum and/or else
The address should be located in ROM area 0000h-7FFFh, the adapter permanently
compares address/old data with address/data being read by the game, and
replaces that data by new data if necessary. That method (more or less)
prohibits unwanted patching of wrong memory banks. Eventually it is also
possible to patch external RAM ?
Newer devices reportedly allow to specify only the first six digits
(optionally). As far as I rememeber, around three or four codes can be used
simultaneously.

Game Shark (RAM patches)
Game Shark codes consist of eight-digit hex numbers, formatted as ABCDEFGH,
the meaning of the separate digits is:
  AB    External RAM bank number
  CD    New Data
  GHEF  Memory Address (internal or external RAM, A000-DFFF)
As far as I understand, patching is implement by hooking the original VBlank
interrupt handler, and re-writing RAM values each frame. The downside is that
this method steals some CPU time, also, it cannot be used to patch program
code in ROM.
As far as I rememeber, somewhat 10-25 codes can be used simultaneously.
*/

#ifndef NO_CAMERA_EMULATION

#include <opencv/cv.h>
#include <opencv/highgui.h>

static CvCapture * capture;
//static int gbcamwindow = 0;
static int gbcamenabled = 0;
static int gbcamerafactor = 1;
#endif

void GB_CameraEnd(void)
{
#ifndef NO_CAMERA_EMULATION
    if(gbcamenabled == 0) return;

    //if(gbcamwindow) cvDestroyWindow("GiiBii - Webcam Output");

    cvReleaseCapture(&capture);

    //gbcamwindow = 0;
    gbcamenabled = 0;
#endif
}

int GB_CameraInit(int createwindow)
{
#ifndef NO_CAMERA_EMULATION
    if(gbcamenabled) return 1;

    capture = cvCaptureFromCAM(CV_CAP_ANY);
    if(!capture)
    {
        ErrorDialog("OpenCV ERROR: capture is NULL\nNo camera detected?");
        return 0;
    }

    gbcamenabled = 1;

    //gbcamwindow = createwindow;

    //if(gbcamwindow) cvNamedWindow("GiiBii - Webcam Output",CV_WINDOW_AUTOSIZE);

    IplImage * frame = cvQueryFrame(capture);
    if(!frame)
    {
        ErrorDialog("OpenCV ERROR: frame is NULL");
        GB_CameraEnd();
        return 0;
    }
    else
    {
        if( (frame->width < 16*8) || (frame->height < 14*8) )
        {
            ErrorDialog("Camera resolution too small..");
            GB_CameraEnd();
            return 0;
        }

        int xfactor = frame->width / (16*8);
        int yfactor = frame->height / (14*8);

        gbcamerafactor = (xfactor > yfactor) ? yfactor : xfactor; //min
    }

    return 1;
#else
    Debug_ErrorMsgArg("GiiBiiAdvance was compiled without camera features.");
    return 0;
#endif
}

static void GB_CameraTakePicture(u32 brightness)
{
    u8 inbuffer[16*8][14*8];
    u8 readybuffer[14][16][16];
    int i, j;

#ifndef NO_CAMERA_EMULATION
    //Get image
    if(gbcamenabled == 0)
    {
#endif
        //just some random output...
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++) inbuffer[i][j] = rand();
#ifndef NO_CAMERA_EMULATION
    }
    else
    {
        //get image from webcam
        IplImage * frame = cvQueryFrame(capture);
        if(!frame)
        {
            GB_CameraEnd();
            ErrorDialog("OpenCV ERROR: frame is null...\n");
        }
        else
        {
            if(frame->depth == 8 && frame->nChannels == 3)
            {
                for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
                {
                    u8 * data = &( ((u8*)frame->imageData)
                            [((j*gbcamerafactor)*frame->widthStep)+((i*gbcamerafactor)*3)] );
                    s16 value = ((u32)data[0]+(u32)data[1]+(u32)data[2])/3;
                    value += (rand()&0x7)-0x03; // a bit of noise :)
                    if(value < 0) value = 0;
                    else if(value > 255) value = 255;
                    inbuffer[i][j] = (u8)value;
                    //if(gbcamwindow) { data[0] = ~data[0]; data[1] = ~data[1]; data[2] = ~data[2]; }
                }
            }

            //if(gbcamwindow) cvShowImage("GiiBii - Webcam Output", frame );
        }
    }
#endif

    //Apply brightness
    u32 br = brightness >= 0x8000;
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        if(br == 0)
        {
            inbuffer[i][j] = (inbuffer[i][j] * brightness) / (0x8000);
        }
        else
        {
            //when brightness is very high (in gb camera, the bar must be at the top position) it
            //will start changing brightness like crazy... i don't have any idea of why does it happen.
            inbuffer[i][j] = 255 - ((255-inbuffer[i][j]) * (0xFFFF - brightness)) / (0x8000);
        }
    }

    //Convert to tiles
    memset(readybuffer,0,sizeof(readybuffer));
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        u8 outcolor = 3 - (inbuffer[i][j] >> 6);

        u8 * tile_base = readybuffer[j>>3][i>>3];

        tile_base = &tile_base[(j&7)*2];

        if(outcolor & 1) tile_base[0] |= 1<<(7-(7&i));
        if(outcolor & 2) tile_base[1] |= 1<<(7-(7&i));
    }

    //Copy to cart ram...
    memcpy(&(GameBoy.Memory.ExternRAM[0][0xA100-0xA000]),readybuffer,sizeof(readybuffer));
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

static void GB_NoMapperWrite(u32 address, u32 value)
{
    //Debug_DebugMsgArg("NO MAPPER WROTE - %02x to %04x",value,address);
    return;
}

//Used by MBC1 and HuC-1
static void GB_MBC1Write(u32 address, u32 value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
        case 0x1: //RAM Enable
            mem->RAMEnabled = ( (value & 0xF) == 0xA );
            if(GameBoy.Emulator.RAM_Banks == 0) mem->RAMEnabled = 0;
            break;
        case 0x2:
        case 0x3: //ROM Bank Number - lower 5 bits
            value &= 0x1F;
            if(value == 0) value = 1;
            mem->selected_rom &= ~0x1F;
            mem->selected_rom |= value;

            mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;

            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x4:
        case 0x5: //RAM Bank Number - or - Upper Bits of ROM Bank Number
            value &= 0x3;
            if(mem->mbc_mode == 0) //ROM MODE
            {
                mem->selected_rom &= 0x1F;
                mem->selected_rom |= value << 5;

                mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;

                mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            }
            else //RAM MODE
            {
                mem->selected_ram = value;
                mem->selected_ram &= GameBoy.Emulator.RAM_Banks - 1;
                mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
            }
            break;
        case 0x6:
        case 0x7: //ROM/RAM Mode Select
            mem->mbc_mode = value & 1;
            break;
        case 0xA:
        case 0xB:
            if(mem->RAMEnabled == 0) return;
            else mem->RAM_Curr[address-0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("MBC1 WROTE - %02x to %04x",value,address);
            break;
    }
}

static void GB_MBC2Write(u32 address, u32 value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
        case 0x1: //RAM Enable
            if((address & 0x0100) == 0)
            {
                mem->RAMEnabled = ( (value & 0xF) == 0xA );
                if(GameBoy.Emulator.RAM_Banks == 0) mem->RAMEnabled = 0;
            }
            break;
        case 0x2:
        case 0x3: //ROM Bank Number
        //  if(address & 0x0100)
            {
                if(value == 0) value = 1;
                mem->selected_rom = value & (GameBoy.Emulator.ROM_Banks - 1);
                if(mem->selected_rom == 0) mem->selected_rom ++;
                mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            }
            break;
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
            break;
        case 0xA:
        case 0xB:
            if(mem->RAMEnabled == 0) return;
            else mem->RAM_Curr[address-0xA000] = value & 0x0F;
            break;
        default:
            //Debug_DebugMsgArg("MBC2 WROTE - %02x to %04x",value,address);
            break;
    }
}

static void GB_MBC3Write(u32 address, u32 value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
        case 0x1: //RAM Enable
            mem->RAMEnabled = ( (value & 0xF) == 0xA );
            if(GameBoy.Emulator.RAM_Banks == 0) mem->RAMEnabled = 0;
            break;
        case 0x2:
        case 0x3: //ROM Bank Number
            mem->selected_rom = value;
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks - 1;
            if(mem->selected_rom == 0) mem->selected_rom = 1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x4:
        case 0x5: //RAM Bank Number - or - RTC Register Select
            value &= 0x0F;
            if(value < 4) //RAM MODE
            {
                mem->selected_ram = value & (GameBoy.Emulator.RAM_Banks-1);

                mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
                mem->mbc_mode = 1;
            }
            else if(value > 7 && value < 0xD) //RTC MODE
            {
                mem->selected_ram = value;
                mem->mbc_mode = 0;
            }
            break;
        case 0x6:
        case 0x7: //Latch Clock Data
            //The correct way of doing it is writing first 0x00 and then 0x01, but
            //some games don't do that.
            if(value == 0x01)
            {
                GameBoy.Emulator.LatchedTime.sec = GameBoy.Emulator.Timer.sec;
                GameBoy.Emulator.LatchedTime.min = GameBoy.Emulator.Timer.min;
                GameBoy.Emulator.LatchedTime.hour = GameBoy.Emulator.Timer.hour;
                GameBoy.Emulator.LatchedTime.days = GameBoy.Emulator.Timer.days;
                GameBoy.Emulator.LatchedTime.carry = GameBoy.Emulator.Timer.carry;
                GameBoy.Emulator.LatchedTime.halt = GameBoy.Emulator.Timer.halt;
            }
            break;
        case 0xA: //RAM Bank 00-03, if any
        case 0xB: //RTC Register 08-0C
            if(mem->RAMEnabled == 0) return;

            if(mem->mbc_mode == 1) //RAM mode
                mem->RAM_Curr[address-0xA000] = value;
            else //RTC REGISTER
            {
                switch(mem->selected_ram)
                {
                    case 0x08: //Sec
                        GameBoy.Emulator.Timer.sec = value;
                        return;
                    case 0x09: //Min
                        GameBoy.Emulator.Timer.min = value;
                        return;
                    case 0x0A: //Hour
                        GameBoy.Emulator.Timer.hour = value;
                        return;
                    case 0x0B: //Lower 8 bits days
                        GameBoy.Emulator.Timer.days = (GameBoy.Emulator.Timer.days & (1<<8)) |
                                                        (value & 0xFF);
                        return;
                    case 0x0C: //carry-halt-0-0-0-0-0-upper_bit_days
                        GameBoy.Emulator.Timer.days = (GameBoy.Emulator.Timer.days & 0xFF) |
                                                        ((value & 0x01) << 8);
                        GameBoy.Emulator.Timer.carry = (value&(1<<7)) >> 7;
                        GameBoy.Emulator.Timer.halt = (value&(1<<6)) >> 6;
                        GameBoy.Emulator.LatchedTime.halt = GameBoy.Emulator.Timer.halt;
                        return;
                    default:
                        return;
                }
                return;
            }
            break;
        default:
            //Debug_DebugMsgArg("MBC3 WROTE- %02x to %04x",value,address);
            break;
    }
}

//Used by MBC5 (and rumble)
static void GB_MBC5Write(u32 address, u32 value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
        case 0x1:
            mem->RAMEnabled = ((value & 0x0F) == 0x0A);
            if(GameBoy.Emulator.RAM_Banks == 0) mem->RAMEnabled = 0;
            break;
        case 0x2:
            mem->selected_rom &= 0xFF00;
            mem->selected_rom |= (value&0xFF);
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks-1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x3:
            mem->selected_rom &= 0xFF;
            mem->selected_rom |= value << 8;
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks-1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x4:
        case 0x5:
            if(GameBoy.Emulator.MemoryController == MEM_RUMBLE)
            {
                if(value & 0x08) GameBoy.Emulator.rumble = 1;
                else GameBoy.Emulator.rumble = 0;
                value &= 0x7;
            }
            mem->selected_ram = value & (GameBoy.Emulator.RAM_Banks-1);
            mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
            break;
        case 0x6: //Why do some games write 0 and 1 here? For MBC1 compatibility?
        case 0x7:
            break;
        case 0xA:
        case 0xB:
            if(mem->RAMEnabled == 0) return;
            else mem->RAM_Curr[address-0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("MBC5 WROTE - %02x to %04x",value,address);
            break;
    }
}

//Doesn't work... or I've only tested damaged roms... I don't know.
static void GB_MBC6Write(u32 address, u32 value) // some things copied from MESS
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
            if(address == 0x0000)
            {
                mem->RAMEnabled = ((value & 0x0A) == 0x0A);
                if(GameBoy.Emulator.RAM_Banks == 0) mem->RAMEnabled = 0;
                break;
            }
            //Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",value,address);
            break;
        case 0x2:
            if(address == 0x27FF)
            {
                mem->selected_rom = value;
                if(mem->selected_rom == 0) mem->selected_rom = 1;
                mem->selected_rom &= GameBoy.Emulator.ROM_Banks-1;
            }
            else if(address == 0x2800)
            {
                if(value == 0)
                {
                    mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
                }
                //else Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",value,address);
            }
            //else Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",value,address);
            break;
        case 0x3:
            if(address == 0x37FF)
            {
                mem->selected_ram = value;
                mem->selected_ram &= GameBoy.Emulator.RAM_Banks-1;
                //mem->selected_ram &= GameBoy.Emulator.ROM_Banks-1;
            }
            else if(address == 0x3800)
            {
                if(value == 0x08)
                {
                    //mem->ROM_Base = mem->ROM_Switch[mem->selected_ram];
                    mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
                }
                //else Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",value,address);
            }
            //else Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",value,address);
            break;
        case 0xA:
        case 0xB:
            //Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",value,address);
            if(mem->RAMEnabled == 0) return;
            else mem->RAM_Curr[address-0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("MBC6 WROTE - %02x to %04x",value,address);
            break;
    }
}

static void GB_MBC7Write(u32 address, u32 value) // copied from VisualBoy Advance (and modified)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
        case 0x1:
            break;
        case 0x2:
        case 0x3:
            mem->selected_rom = value;
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks-1;
            if(mem->selected_rom == 0) mem->selected_rom = 1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x4:
        case 0x5:
            if(value < 8)
            {
                mem->RAMEnabled = 0;
                mem->selected_ram = value & (GameBoy.Emulator.RAM_Banks-1);
                mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
            }
            else
            {
                mem->RAMEnabled = 0;
            }
            break;
        case 0xA:
        case 0xB:
            if(address == 0xa080)
            {
                _GB_MB7_CART_ * mbc7 = &GameBoy.Emulator.MBC7;
                // special processing needed
                int oldCs = mbc7->cs, oldSk = mbc7->sk;

                mbc7->cs = value>>7;
                mbc7->sk = (value>>6)&1;

                if(!oldCs && mbc7->cs)
                {
                    if(mbc7->state == 5)
                    {
                        if(mbc7->writeEnable)
                        {
                            mem->RAM_Curr[mbc7->address*2] = mbc7->buffer>>8;
                            mem->RAM_Curr[mbc7->address*2+1] = mbc7->buffer&0xff;
                        }
                        mbc7->state = 0;
                        mbc7->value = 1;
                    }
                    else
                    {
                        mbc7->idle = true;
                        mbc7->state = 0;
                    }
                }

                if(!oldSk && mbc7->sk)
                {
                    if(mbc7->idle)
                    {
                        if(value & 0x02)
                        {
                            mbc7->idle = false;
                            mbc7->count = 0;
                            mbc7->state = 1;
                        }
                    }
                    else
                    {
                        switch(mbc7->state)
                        {
                            case 1:
                                // receiving command
                                mbc7->buffer <<= 1;
                                mbc7->buffer |= (value & 0x02)?1:0;
                                mbc7->count++;
                                if(mbc7->count == 2)
                                {
                                    // finished receiving command
                                    mbc7->state = 2;
                                    mbc7->count = 0;
                                    mbc7->code = mbc7->buffer & 3;
                                }
                                break;
                            case 2:
                                // receive address
                                mbc7->buffer <<= 1;
                                mbc7->buffer |= (value&0x02)?1:0;
                                mbc7->count++;
                                if(mbc7->count==8)
                                {
                                    // finish receiving
                                    mbc7->state = 3;
                                    mbc7->count = 0;
                                    mbc7->address = mbc7->buffer&0xff;
                                    if(mbc7->code == 0)
                                    {
                                        if((mbc7->address>>6) == 0)
                                        {
                                            mbc7->writeEnable = 0;
                                            mbc7->state = 0;
                                        }
                                        else if((mbc7->address>>6) == 3)
                                        {
                                            mbc7->writeEnable = 1;
                                            mbc7->state = 0;
                                        }
                                    }
                                }
                                break;
                            case 3:
                                mbc7->buffer <<= 1;
                                mbc7->buffer |= (value&0x02)?1:0;
                                mbc7->count++;

                                switch(mbc7->code)
                                {
                                    case 0:
                                        if(mbc7->count==16)
                                        {
                                            if((mbc7->address>>6)==0)
                                            {
                                                mbc7->writeEnable = 0;
                                                mbc7->state = 0;
                                            }
                                            else if((mbc7->address>>6) == 1)
                                            {
                                                if(mbc7->writeEnable)
                                                {
                                                    int i;
                                                    for(i=0;i<256;i++)
                                                    {
                                                        mem->RAM_Curr[i*2] = mbc7->buffer >> 8;
                                                        mem->RAM_Curr[i*2+1] = mbc7->buffer & 0xff;
                                                    }
                                                }
                                                mbc7->state = 5;
                                            }
                                            else if((mbc7->address>>6) == 2)
                                            {
                                                if(mbc7->writeEnable)
                                                {
                                                    int i;
                                                    for(i=0;i<256;i++)
                                                        *((unsigned short *)&mem->RAM_Curr[i*2]) = 0xffff;
                                                }
                                                mbc7->state = 5;
                                            }
                                            else if((mbc7->address>>6) == 3)
                                            {
                                                mbc7->writeEnable = 1;
                                                mbc7->state = 0;
                                            }
                                            mbc7->count = 0;
                                        }
                                        break;
                                    case 1:
                                        if(mbc7->count == 16)
                                        {
                                            mbc7->count = 0;
                                            mbc7->state = 5;
                                            mbc7->value = 0;
                                        }
                                        break;
                                    case 2:
                                        if(mbc7->count == 1)
                                        {
                                            mbc7->state = 4;
                                            mbc7->count = 0;
                                            mbc7->buffer = (mem->RAM_Curr[mbc7->address*2]<<8)|(mem->RAM_Curr[mbc7->address*2+1]);
                                        }
                                        break;
                                    case 3:
                                        if(mbc7->count == 16)
                                        {
                                            mbc7->count = 0;
                                            mbc7->state = 5;
                                            mbc7->value = 0;
                                            mbc7->buffer = 0xffff;
                                        }
                                        break;
                                }
                                break;
                        }
                    }
                }
                if(oldSk && !mbc7->sk)
                {
                    if(mbc7->state == 4)
                    {
                        mbc7->value = (mbc7->buffer & 0x8000)?1:0;
                        mbc7->buffer <<= 1;
                        mbc7->count++;
                        if(mbc7->count == 16)
                        {
                            mbc7->count = 0;
                            mbc7->state = 0;
                        }
                    }
                }
            }

            break;
        default:
            //Debug_DebugMsgArg("MBC7 WROTE - %02x to %04x",value,address);
            break;
    }
}

//"Taito Variety Pack" works, but "Momotarou Collection 2" is more strange...
static void GB_MMM01Write(u32 address, u32 value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0: // Apply mapping
        case 0x1:
            if(GameBoy.Emulator.EnableBank0Switch == 1)
            {
                //Momotarou Collection 2 writes 3A and 7A, so this check works too
                if(value & 0x40) //Taito Pack
                {
                    mem->ROM_Base = mem->ROM_Switch[GameBoy.Emulator.MMM01.offset &
                                                    (GameBoy.Emulator.ROM_Banks - 1)];
                    mem->selected_rom = (GameBoy.Emulator.MMM01.offset + 1) &
                                        (GameBoy.Emulator.ROM_Banks - 1);
                    mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
                    GameBoy.Emulator.EnableBank0Switch = 0;
                }
            }
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x",value,address);
            break;
        case 0x2: //ROM Bank Number / New mapping offset
            if(GameBoy.Emulator.EnableBank0Switch == 1) //Taito Pack
            {
                GameBoy.Emulator.MMM01.offset =
                    0x02 // first 2 banks, used by game selection menu
                    + (value & 0x1F);
            }
            else
            {
                mem->selected_rom = (value & GameBoy.Emulator.MMM01.mask) +  //Taito Pack
                                    GameBoy.Emulator.MMM01.offset;
                mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            }
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x",value,address);
            break;
        case 0x3: //RAM Enable ?
            //Momotarou Collection 2 - i've only seen it writing to 3FFF
            mem->RAMEnabled = value & 0x01;
            if(GameBoy.Emulator.RAM_Banks == 0) mem->RAMEnabled = 0;
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x",value,address);
            break;
        case 0x4: // ?
            //Taito Pack writes 0x70 here... no idea of why does it do it...
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x",value,address);
            break;
        case 0x5:
            if(value == 0x40) GameBoy.Emulator.MMM01.mask = 0x1F;// Momotarou Collection 2
            else if(value == 0x01) GameBoy.Emulator.MMM01.mask = 0x0F; // Who knows...
            else GameBoy.Emulator.MMM01.mask = 0xFF;
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x",value,address);
            break;
        case 0x6: // New mapping mask
            if(GameBoy.Emulator.EnableBank0Switch == 1) //Taito Pack
            {
                if(value == 0x38) GameBoy.Emulator.MMM01.mask = 0x03;
                else if(value == 0x30) GameBoy.Emulator.MMM01.mask = 0x07;
                else GameBoy.Emulator.MMM01.mask = 0xFF;
            }
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x",value,address);
            break;
        case 0x7:
            if(GameBoy.Emulator.EnableBank0Switch == 1) //Momotarou Collection 2
                GameBoy.Emulator.MMM01.offset = value; // Who knows...
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x",value,address);
            break;
        case 0xA:
        case 0xB:
            if(mem->RAMEnabled == 0) return;
            else mem->RAM_Curr[address-0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("MMM01 WROTE - %02x to %04x",value,address);
            break;
    }
}

static void GB_CameraWrite(u32 address, u32 value)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    switch(address >> 12)
    {
        case 0x0:
        case 0x1: //Enable WRITING ONLY!
            mem->RAMEnabled = ((value & 0x0F) == 0x0A);
            if(GameBoy.Emulator.RAM_Banks == 0) mem->RAMEnabled = 0;
            break;
        case 0x2: //ROM change.
            mem->selected_rom &= 0xFF00;
            mem->selected_rom |= (value&0xFF);
            mem->selected_rom &= GameBoy.Emulator.ROM_Banks-1;
            if(mem->selected_rom == 0) mem->selected_rom = 1;
            mem->ROM_Curr = mem->ROM_Switch[mem->selected_rom];
            break;
        case 0x3:
            break;
        case 0x4:
            if(value == 0x10)
            {
                mem->mbc_mode = 1;
            }
            else
            {
                mem->mbc_mode = 0;
                mem->selected_ram = value & (GameBoy.Emulator.RAM_Banks-1);
                mem->RAM_Curr = mem->ExternRAM[mem->selected_ram];
            }
            break;
        case 0x5:
        case 0x6:
        case 0x7:
            break;
        case 0xA:
        case 0xB:
            if(mem->mbc_mode == 1)
            {
                _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

                u32 reg = address-0xA000;

                //printf("[%04X]=%02X\n",(u32)(u16)address,(u32)(u8)value);

                if(reg < 0x36)
                {
                    //Registers:
                    //0 - execute command/take picture?
                    //1,2 - brightness (1 MSB, 2 LSB)
                    //3 - counter?
                    //4 - ? (fixed)
                    //5 - ? (fixed)
                    //6..35h - individual tile filter? 48 = 8*6 contrast seems to change all
                    //         registers...
                    cam->reg[reg] = value;

                    //Take picture...
                    if(reg == 0)
                    {
                        if(value == 0x3) //execute command? take picture?
                        {
                            u32 brightness = cam->reg[1] + (cam->reg[2]<<8);
                            GB_CameraTakePicture(brightness);
                        }
                        //else Debug_DebugMsgArg("CAMERA WROTE - %02x to %04x",value,address);
                    }
                }
                //Debug_DebugMsgArg("CAMERA WROTE - %02x to %04x",value,address);
            }

            if(mem->RAMEnabled == 0) return;
            else mem->RAM_Curr[address-0xA000] = value;
            break;
        default:
            //Debug_DebugMsgArg("CAMERA WROTE - %02x to %04x",value,address);
            break;
    }
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

static u32 GB_NoMapperRead(u32 address)
{
    //Debug_DebugMsgArg("NO MAPPER READ - %04x",address);
    return 0xFF;
}

//Used by MBC1, HuC-1, MBC5 (and rumble), MBC6, MMM01
static u32 GB_MBC1Read(u32 address)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;
    if(GameBoy.Emulator.RAM_Banks == 0 || mem->RAMEnabled == 0) return 0xFF;
    return mem->RAM_Curr[address-0xA000];
}

static u32 GB_MBC2Read(u32 address)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;
    if(GameBoy.Emulator.RAM_Banks == 0 || mem->RAMEnabled == 0) return 0xFF;
    return ( (mem->RAM_Curr[address-0xA000]) & 0x0F );
}

static u32 GB_MBC3Read(u32 address)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    if(mem->RAMEnabled == 0) return 0xFF;

    if(mem->mbc_mode == 1) //RAM mode
    {
        if(GameBoy.Emulator.RAM_Banks == 0) return 0xFF;
        return mem->RAM_Curr[address-0xA000];
    }
    else //RTC mode
    {
        switch(mem->selected_ram)
        {
            case 0x08: //Sec
                return GameBoy.Emulator.LatchedTime.sec;
            case 0x09: //Min
                return GameBoy.Emulator.LatchedTime.min;
            case 0x0A: //Hour
                return GameBoy.Emulator.LatchedTime.hour;
            case 0x0B: //Lower 8 bits days
                return GameBoy.Emulator.LatchedTime.days & 0xFF;
            case 0x0C: //carry-halt- - - - - -upper bit days
                return ( (GameBoy.Emulator.LatchedTime.days >> 8) |
                        (GameBoy.Emulator.LatchedTime.halt << 6) |
                        (GameBoy.Emulator.LatchedTime.carry << 7) );
            default:
                return 0xFF;
        }
    }

    return 0xFF;
}

static u32 GB_MBC7Read(u32 address)
{
    switch(address & 0xa0f0)
    {
        case 0xa000:
        case 0xa010:
        case 0xa060:
        case 0xa070:
            return 0;
        case 0xa020:
            // sensor X low byte
            return GameBoy.Emulator.MBC7.sensorX & 255;
        case 0xa030:
            // sensor X high byte
            return GameBoy.Emulator.MBC7.sensorX >> 8;
        case 0xa040:
            // sensor Y low byte
            return GameBoy.Emulator.MBC7.sensorY & 255;
        case 0xa050:
            // sensor Y high byte
            return GameBoy.Emulator.MBC7.sensorY >> 8;
        case 0xa080:
            return GameBoy.Emulator.MBC7.value;
        default:
            return 0xFF;
    }
    return 0xFF;
}

static u32 GB_CameraRead(u32 address)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    if(mem->mbc_mode == 1) //CAM registers
    {
        if(address == 0xA000) return 0; //'ready' register -- always ready
        return 0xFF; //others are write-only? they are never read so the value doesn't matter...
    }
    else //RAM -- image captured by camera goes to bank 0
    {
        return mem->RAM_Curr[address-0xA000];
    }
    return 0xFF;
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

void GB_MapperSet(int type)
{
    switch(type)
    {
        default:
        case MEM_NONE:
        case MEM_MBC4:
        case MEM_HUC3:
        case MEM_TAMA5:
            GameBoy.Memory.MapperWrite = &GB_NoMapperWrite;
            GameBoy.Memory.MapperRead = &GB_NoMapperRead;
            break;
        case MEM_MBC1:
        case MEM_HUC1:
            GameBoy.Memory.MapperWrite = &GB_MBC1Write;
            GameBoy.Memory.MapperRead = &GB_MBC1Read;
            break;
        case MEM_MBC2:
            GameBoy.Memory.MapperWrite = &GB_MBC2Write;
            GameBoy.Memory.MapperRead = &GB_MBC2Read;
            break;
        case MEM_MBC3:
            GameBoy.Memory.MapperWrite = &GB_MBC3Write;
            GameBoy.Memory.MapperRead = &GB_MBC3Read;
            break;
        case MEM_MBC5:
        case MEM_RUMBLE:
            GameBoy.Memory.MapperWrite = &GB_MBC5Write;
            GameBoy.Memory.MapperRead = &GB_MBC1Read;
            break;
        case MEM_MBC6:
            GameBoy.Memory.MapperWrite = &GB_MBC6Write;
            GameBoy.Memory.MapperRead = &GB_MBC1Read;
            break;
        case MEM_MBC7:
            GameBoy.Memory.MapperWrite = &GB_MBC7Write;
            GameBoy.Memory.MapperRead = &GB_MBC7Read;
            break;
        case MEM_MMM01:
            GameBoy.Memory.MapperWrite = &GB_MMM01Write;
            GameBoy.Memory.MapperRead = &GB_MBC1Read;
            break;
        case MEM_CAMERA:
            GameBoy.Memory.MapperWrite = &GB_CameraWrite;
            GameBoy.Memory.MapperRead = &GB_CameraRead;
            break;
    }
}


