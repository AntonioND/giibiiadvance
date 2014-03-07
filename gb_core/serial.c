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

#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../general_utils.h"
#include "../file_utils.h"
#include "../debug_utils.h"

#include "general.h"
#include "gameboy.h"
#include "interrupts.h"
#include "serial.h"
#include "debug.h"

#include "../config.h"

extern _GB_CONTEXT_ GameBoy;

//FILE * debug;

void GB_SerialInit(void)
{
    GameBoy.Emulator.serial_clocks = 0;
    GameBoy.Emulator.serial_enabled = 0;

    GameBoy.Emulator.serial_device = SERIAL_NONE;
    GB_SerialPlug(EmulatorConfig.serial_device);

//    debug = fopen("serial.bin","wb");
}

inline void GB_SerialClocks(int _clocks)
{
    if(GameBoy.Emulator.serial_enabled == 0) return;

    _GB_MEMORY_ * mem = &GameBoy.Memory;

    if(mem->IO_Ports[SC_REG-0xFF00] & 0x01) //Internal clock
    {
        GameBoy.Emulator.serial_clocks += _clocks;

        if(GameBoy.Emulator.serial_clocks > GameBoy.Emulator.serial_total_clocks)
        {
            GameBoy.Emulator.serial_enabled = 0;

            GB_SetInterrupt(I_SERIAL);

            // (*) see memory.c
            GameBoy.Emulator.SerialSend_Fn(mem->IO_Ports[SB_REG-0xFF00]);

            mem->IO_Ports[SC_REG-0xFF00] &= ~0x80;
            mem->IO_Ports[SB_REG-0xFF00] = GameBoy.Emulator.SerialRecv_Fn();
        }
    }
}

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
//                                NO DEVICE

void GB_SendNone(u32 data) { return; }

u32 GB_RecvNone(void) { return 0xFF; }

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
//                                GB PRINTER

#define GBPRINTER_NUMPACKETS (10) //screen is 20x18 tiles -> 20x18x16 bytes -> 20x18x16/0x280 = 9 ( + 1 empty )

typedef struct {
    int state;
    int output;

    // 0x88-0x33-CMD-COMPRESSED-SIZE LSB-SIZE MSB-...DATA...-CHECKSUM LSB-CHECKSUM MSB-0-0
    u32 cmd;
    u32 size;
    u8 * data;
    u32 dataindex;
    u8 end[4];

    u32 curpacket;
    u8 * packets[GBPRINTER_NUMPACKETS];
    u32 packetsize[GBPRINTER_NUMPACKETS];
    u32 packetcompressed[GBPRINTER_NUMPACKETS];
    } _GB_PRINTER_;

_GB_PRINTER_ GB_Printer;

int printer_file_number = 0;

#include "../png/png_utils.h"
void GB_PrinterPrint(void)
{
    char * endbuffer = malloc(20 * 18 * 16);
    memset(endbuffer,0,20 * 18 * 16);

    char * ptr = endbuffer;

    int i;
    for(i = 0; i < GBPRINTER_NUMPACKETS; i++)
    {
        if(GB_Printer.packetcompressed[i] == 0)
        {
            memcpy(ptr,GB_Printer.packets[i],GB_Printer.packetsize[i]);
            ptr += GB_Printer.packetsize[i];
        }
        else
        {
            unsigned char * src = GB_Printer.packets[i];
            int size = GB_Printer.packetsize[i];

            while(1)
            {
                int data = *src++;
                size--;

                if(data & 0x80) //Repeat
                {
                    data = (data & 0x7F) + 2;
                    memset(ptr,*src++,data);
                    size --;
                    ptr += data;
                }
                else //Normal
                {
                    data ++;
                    memcpy(ptr,src,data);
                    ptr += data;
                    src += data;
                    size -= data;
                }

                if(size == 0) break;

                if(size < 0) Debug_ErrorMsgArg("GB_PrinterPrint: size < 0 (%d)",size);
            }
        }
    }

    char filename[MAX_PATHLEN];

    while(1)
    {
        s_snprintf(filename,sizeof(filename),"%sprinter%d.png",DirGetScreenshotFolderPath(),printer_file_number);

        FILE* file=fopen(filename, "rb");
        if(file == NULL) break; //Ok
        printer_file_number ++; //look for next free number
    }

    u32 * buf_temp = calloc(160*144*4,1);
    const u32 gb_pal_colors[4][3] = { {255,255,255}, {168,168,168}, {80,80,80}, {0,0,0} };
    int x,y;

    for(y = 0; y < 144; y++) for(x = 0; x < 160; x++)
    {
        int tile = ((y>>3) * 20) + (x>>3);

        char * tileptr = &endbuffer[tile * 16];

        tileptr += ((y&7) * 2);

        int x_ = 7-(x&7);

        int color = ( (*tileptr >> x_) & 1 ) |  ( ( ( (*(tileptr+1)) >> x_)  << 1) & 2);

        buf_temp[y*160 + x] = (gb_pal_colors[color][0]<<16)|(gb_pal_colors[color][1]<<8)|
            gb_pal_colors[color][2];
    }

    Save_PNG(filename,160,144,buf_temp,0);

    free(buf_temp);

    printer_file_number ++;
}

void GB_PrinterReset(void)
{
    if(GB_Printer.data != NULL)
    {
        free(GB_Printer.data);
        GB_Printer.data = NULL;
    }

    GB_Printer.curpacket = 0;

    int i;
    for(i = 0; i < GBPRINTER_NUMPACKETS; i++)
    {
        GB_Printer.packetsize[i] = 0;

        if(GB_Printer.packets[i] != NULL)
        {
            free(GB_Printer.packets[i]);
            GB_Printer.packets[i] = NULL;
            GB_Printer.packetcompressed[i] = 0;
        }
    }

    GB_Printer.state = 0;
    GB_Printer.output = 0;
}

bool GB_PrinterChecksumIsCorrect(void)
{
    //Checksum
    u32 checksum = (GB_Printer.end[0] & 0xFF) | ((GB_Printer.end[1] & 0xFF) << 8);
    u32 result = GB_Printer.cmd + GB_Printer.packetcompressed[GB_Printer.curpacket];

    if(GB_Printer.size > 0)
    {
        result += (GB_Printer.size & 0xFF) + ((GB_Printer.size >> 8) & 0xFF);
        int i;
        for(i = 0; i < GB_Printer.size; i++) result += GB_Printer.data[i];
    }

    result &= 0xFFFF;

    if(result != checksum) Debug_DebugMsgArg("GB Printer packet corrupted.\n"
                                        "Obtained: 0x%04x (0x%04x)",result,checksum);

    return (result == checksum);
}

void GB_PrinterExecute(void)
{
    switch(GB_Printer.cmd)
    {
        case 0x01: //Init
            GB_PrinterReset();
            break;

        case 0x02: //End - Print
            //data = palettes?, but... what do they do?
            GB_PrinterPrint();
            break;

        case 0x04: //Data
            if(GB_Printer.curpacket >= GBPRINTER_NUMPACKETS)
            {
                Debug_ErrorMsgArg("GB Printer packet limit reached.");
            }
            else
            {
                GB_Printer.packets[GB_Printer.curpacket] = GB_Printer.data;
                GB_Printer.packetsize[GB_Printer.curpacket++] = GB_Printer.size;
                GB_Printer.data = NULL;
            }
            break;

        case 0x0F: //Nothing
            break;
    }
}

void GB_SendPrinter(u32 data)
{
    switch(GB_Printer.state)
    {
        case 0: //Magic 1
            GB_Printer.output = 0x00;
            if(data == 0x88) GB_Printer.state = 1;
            break;
        case 1: //Magic 2
            if(data == 0x33) GB_Printer.state = 2;
            else GB_Printer.state = 0; //Reset...
            break;
        case 2: //CMD
            GB_Printer.cmd = data;
            GB_Printer.state = 3;
            break;
        case 3: //COMPRESSED
            GB_Printer.packetcompressed[GB_Printer.curpacket] = data;
            GB_Printer.state = 4;
            break;
        case 4: //Size 1
            GB_Printer.size = data;
            GB_Printer.state = 5;
            break;
        case 5: //Size 2
            GB_Printer.size = (GB_Printer.size & 0xFF) | (data << 8);

            GB_Printer.dataindex = 0;

            if(GB_Printer.size > 0)
            {
                GB_Printer.data = malloc(GB_Printer.size);
                GB_Printer.state = 6;
            }
            else
            {
                GB_Printer.state = 7;
            }
            break;
        case 6: //Data
            GB_Printer.data[GB_Printer.dataindex++] = data;

            if(GB_Printer.dataindex == GB_Printer.size)
            {
                GB_Printer.dataindex = 0;
                GB_Printer.state = 7;
            }
            break;
        case 7: //End
            GB_Printer.end[GB_Printer.dataindex++] = data;

            if(GB_Printer.dataindex == 3)
            {
                if(GB_PrinterChecksumIsCorrect())
                    GB_Printer.output = 0x81;
                else
                    GB_Printer.output = 0x00;
            }
            else
            {
                GB_Printer.output = 0x00;
            }

            if(GB_Printer.dataindex == 4)
            {
                GB_PrinterExecute();
                GB_Printer.state = 0; //Done
            }
            break;
    }

//    u8 tempss = data;
//    if( temp) fwrite(&tempss,1,1,debug);
//    Debug_DebugMsgArg("GB Printer - Received %02x",data);
    return;
}

u32 GB_RecvPrinter(void)
{
    return GB_Printer.output;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

void GB_SerialPlug(int device)
{
    if(GameBoy.Emulator.serial_device != SERIAL_NONE) GB_SerialEnd();

    GameBoy.Emulator.serial_device = device;

    switch(device)
    {
        case SERIAL_NONE:
            GameBoy.Emulator.SerialSend_Fn = &GB_SendNone;
            GameBoy.Emulator.SerialRecv_Fn = &GB_RecvNone;
            break;
        case SERIAL_GBPRINTER:
            GameBoy.Emulator.SerialSend_Fn = &GB_SendPrinter;
            GameBoy.Emulator.SerialRecv_Fn = &GB_RecvPrinter;
            GB_PrinterReset();
            break;
        case SERIAL_GAMEBOY: //TODO
        //  break;
        default:
            GameBoy.Emulator.SerialSend_Fn = &GB_SendNone;
            GameBoy.Emulator.SerialRecv_Fn = &GB_RecvNone;
            break;
    }
}

void GB_SerialEnd(void)
{
    if(GameBoy.Emulator.serial_device == SERIAL_GBPRINTER) GB_PrinterReset();

    if(GameBoy.Emulator.serial_device == SERIAL_GAMEBOY) //TODO
    {
    }

//    fclose(debug);
}


