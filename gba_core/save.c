/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

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
#include <stdio.h>
#include <stdlib.h>

#include "../build_options.h"
#include "../debug_utils.h"

#include "gba.h"
#include "memory.h"
#include "video.h"
#include "cpu.h"
#include "dma.h"
#include "save.h"

static const char * save_type_strings[5] = {
    "EEPROM_V",    //EEPROM 512 bytes or 8 Kbytes (4Kbit or 64Kbit)
    "SRAM_V",      //SRAM 32 Kbytes (256Kbit)
    "FLASH_V",     //FLASH 64 Kbytes (512Kbit) (ID used in older files)
    "FLASH512_V",  //FLASH 64 Kbytes (512Kbit) (ID used in newer files)
    "FLASH1M_V"    //FLASH 128 Kbytes (1Mbit)
};

typedef enum {
    SAV_EEPROM = 0,
    SAV_SRAM,
    SAV_FLASH,
    SAV_FLASH512,
    SAV_FLASH1M,
    SAV_TYPES,

    SAV_NONE,
    SAV_AUTODETECT
} _sav_types_;

static inline int str_cmp_len(const char * str1, const char * str2, int len) //1 if equal
{
    int i;
    for(i = 0; i < len; i++) if(str1[i] != str2[i]) return 0;
    return 1;
}

int SAVE_TYPE = SAV_NONE;

inline int GBA_SaveIsEEPROM(void)
{
    return (SAVE_TYPE == SAV_EEPROM) || (SAVE_TYPE == SAV_AUTODETECT);
}

void GBA_DetectSaveType(u8 * romptr, int size)
{
    int j;
    for(j = 0; j < SAV_TYPES; j++)
    {
        int i;
        for(i = 0; i < size; i++)
        {
            int len = strlen(save_type_strings[j]);
            if(i+len < size) //avoid crashing if strangely there are some characters at the end of the rom...
            {
                if(str_cmp_len((const char*)&(romptr[i]),save_type_strings[j],len))
                {
                    //Debug_DebugMsgArg(&(romptr[i]));
                    //GBA_ExecutionBreak();
                    SAVE_TYPE = j;
                    return;
                }
            }
        }
    }

    SAVE_TYPE = SAV_AUTODETECT;

    //Not detected any string...
    //Debug_DebugMsgArg("No save type string detected.");
    //GBA_ExecutionBreak();

    return;
}

u8 SRAM_BUFFER[32*1024];

u8 FLASH_BUFFER512[64*1024];
u8 FLASH_BUFFER1M[128*1024];
u8 * FLASH_1M_PTR;
u32 FLASH_STATE; // 0 = nothing, 1 = see FLASH_CMD
u32 FLASH_CMD;
u32 FLASH_CMD_STATE; //0 if nothing, 1 if 5555=0xAA, 2 if 2AAA=0x55 (ready for command)

int eeprom_detect_size;
u64 EEPROM_BUFFER[1024];
u32 EEPROM_SIZE; u32 EEPROM_ADDRESS_BUS;
u32 EEPROM_ADDRESS; u32 EEPROM_ADDRESS_MASK;
u32 EEPROM_CMD;
u32 EEPROM_CMD_LEN;
u32 EEPROM_DATA_STREAMING;
u64 EEPROM_READ_BUFFER;


static const char * savetype[SAV_TYPES+3] = {
    "EEPROM", "SRAM (32KB)", "FLASH 64KB", "FLASH 64KB", "FLASH 128KB", "-", "None", "Unknown/None. Autodetecting...\n"
};

inline const char * GBA_GetSaveTypeString(void)
{
    return savetype[SAVE_TYPE];
}

void GBA_ResetSaveBuffer(void)
{
    switch(SAVE_TYPE)
    {
        case SAV_SRAM:
            memset(SRAM_BUFFER,0,sizeof(SRAM_BUFFER));
            return;
        case SAV_FLASH:
        case SAV_FLASH512:
            FLASH_CMD = 0; FLASH_CMD_STATE = 0; FLASH_STATE = 0;
            memset(FLASH_BUFFER512,0,sizeof(FLASH_BUFFER512));
            return;
        case SAV_FLASH1M:
            FLASH_CMD = 0; FLASH_CMD_STATE = 0; FLASH_STATE = 0;
            FLASH_1M_PTR = FLASH_BUFFER1M;
            memset(FLASH_BUFFER1M,0,sizeof(FLASH_BUFFER1M));
            return;
        case SAV_EEPROM:
            eeprom_detect_size = 1;
            EEPROM_SIZE = 0; EEPROM_ADDRESS_BUS = 14;
            EEPROM_ADDRESS = 0;
            EEPROM_CMD = 0; EEPROM_CMD_LEN = 0; EEPROM_DATA_STREAMING = 0;
            memset(EEPROM_BUFFER,0,sizeof(EEPROM_BUFFER));
            return;
        case SAV_NONE:
        case SAV_AUTODETECT:
        default:
            return;
    }
}

char SAVE_PATH[MAX_PATHLEN];

void GBA_SaveSetFilename(char * rom_path)
{
    if(strlen(rom_path) > (MAX_PATHLEN-1))
    {
        Debug_ErrorMsgArg("Path too long.");
        exit(1);
    }

    s_strncpy(SAVE_PATH,rom_path,sizeof(SAVE_PATH));
    int cursor = strlen(SAVE_PATH);

    //path should end in ".gba" or ".bin"
    SAVE_PATH[cursor-1] = 'v';
    SAVE_PATH[cursor-2] = 'a';
    SAVE_PATH[cursor-3] = 's';
    //Debug_DebugMsgArg(SAVE_PATH);
}

//--------------------------------------------------------------------------

u8 GBA_SaveRead8(u32 address)
{
    if(SAVE_TYPE == SAV_AUTODETECT)
    {
        //Debug_DebugMsgArg("Autodetect: Read byte from [%08X]", address);

        if((address >= 0x0E000000) && (address < 0x0E010000))
        {
            SAVE_TYPE = SAV_SRAM;
            //Debug_DebugMsgArg("Save type autodetect: SRAM");
            GBA_ResetSaveBuffer();
            GBA_SaveReadFile();
            ConsolePrint("SRAM (32KB)");
        }
    }

    switch(SAVE_TYPE)
    {
        case SAV_SRAM:
        {
            if((address >= 0x0E000000) && (address < 0x0E008000))
                return SRAM_BUFFER[address-0x0E000000];
            return 0;
        }
        case SAV_FLASH:
        case SAV_FLASH512:
        {
            if(FLASH_STATE == 1)
            {
                switch(FLASH_CMD)
                {
                    case 0x80: //erase command
                    case 0x10: //erase all
                    case 0x30: //erase sector
                        return 0xFF;
                    case 0x90: //(ID mode)
                    {
                        if(address == 0x0E000000) return 0x32; //device
                        if(address == 0x0E000001) return 0x1B; //manufacturer
                        break;
                    }
                    default:
                        break;
                }
            }
            if((address >= 0x0E000000) && (address < 0x0E010000))
                return FLASH_BUFFER512[address-0xE000000];
            return 0;
        }
        case SAV_FLASH1M:
        {
            if(FLASH_STATE == 1)
            {
                switch(FLASH_CMD)
                {
                    case 0x80: //erase command
                    case 0x10: //erase all
                    case 0x30: //erase sector
                        return 0xFF;
                    case 0x90: //(ID mode)
                    {
                        if(address == 0x0E000000) return 0x62; //device
                        if(address == 0x0E000001) return 0x13; //manufacturer
                        break;
                    }
                    default:
                        break;
                }
            }
            if((address >= 0x0E000000) && (address < 0x0E010000))
                return FLASH_1M_PTR[address-0xE000000];
            return 0;
        }
        case SAV_EEPROM:
        {
            //Debug_DebugMsgArg("EEPROM: Read byte from [%08X]", address);
            return 1; //ready
        }
        case SAV_NONE:
        case SAV_AUTODETECT:
        default:
            return 0;
    }
}

u16 GBA_SaveRead16(u32 address)
{
    if(SAVE_TYPE == SAV_AUTODETECT)
    {
        //Debug_DebugMsgArg("Autodetect: Read halfword from [%08X]", address);

        //if(gba_dma3enabled())
        {
            SAVE_TYPE = SAV_EEPROM;
            //Debug_DebugMsgArg("Save type autodetect: EEPROM");
            GBA_ResetSaveBuffer();
            GBA_SaveReadFile();
            ConsolePrint("EEPROM");
        }
    }

    switch(SAVE_TYPE)
    {
        case SAV_EEPROM:
        {
            if(gba_dma3enabled())
            {
                //Debug_DebugMsgArg("EEPROM: DMA. Read halfword from [%08X]", address);

                if(EEPROM_CMD == 3)
                {
                    if((EEPROM_DATA_STREAMING < 69) && (EEPROM_DATA_STREAMING > 64))
                    {
                        EEPROM_DATA_STREAMING--;
                        return 0;
                    }
                    else if(EEPROM_DATA_STREAMING <= 64)
                    {
                        EEPROM_DATA_STREAMING--;
                        u16 data = (EEPROM_READ_BUFFER&((u64)1<<63))!=0;
                        EEPROM_READ_BUFFER<<=1;
                        return data;
                    }
                }
            }
            else
            {
                //Debug_DebugMsgArg("EEPROM: Read halfword from [%08X]", address);
                return 1; //ready
            }
        }

        case SAV_SRAM:
        case SAV_FLASH:
        case SAV_FLASH512:
        case SAV_FLASH1M:
        case SAV_NONE:
        case SAV_AUTODETECT:
        default:
            return 0;
    }
}

void GBA_SaveWrite8(u32 address, u8 data)
{
    if(SAVE_TYPE == SAV_AUTODETECT)
    {
        //Debug_DebugMsgArg("Autodetect: Write 8bit [%08X]=%02X",address,data);

        if((address == 0x0E005555) && (data == 0xAA))
        {
            SAVE_TYPE = SAV_FLASH1M; //Better this than 512, in case it switches banks
            //Debug_DebugMsgArg("Save type autodetect: FLASH");
            GBA_ResetSaveBuffer();
            GBA_SaveReadFile();
            ConsolePrint("FLASH (Default to 128KB)");
        }
        else if((address >= 0x0E000000) && (address < 0x0E010000))
        {
            SAVE_TYPE = SAV_SRAM;
            //Debug_DebugMsgArg("Save type autodetect: SRAM");
            GBA_ResetSaveBuffer();
            GBA_SaveReadFile();
            ConsolePrint("SRAM (32KB)");
        }
    }

    switch(SAVE_TYPE)
    {
        case SAV_SRAM:
        {
            if((address >= 0x0E000000) && (address < 0x0E008000))
                SRAM_BUFFER[address-0x0E000000] = data;
            return;
        }
        case SAV_FLASH:
        case SAV_FLASH512:
        {
            if(FLASH_CMD_STATE < 2)
            {
                if(address == 0x0E005555)
                {
                    if((data == 0xAA) && (FLASH_CMD_STATE == 0)) FLASH_CMD_STATE = 1;
                    else FLASH_CMD_STATE = 0;
                }
                else if(address == 0x0E002AAA)
                {
                    if((FLASH_CMD_STATE == 1) && (data == 0x55)) FLASH_CMD_STATE = 2;
                    else FLASH_CMD_STATE = 0;
                }
            }
            else if(FLASH_CMD_STATE == 2)
            {
                if( ((address&0xFF000000) == 0x0E000000) && ((address&0x00FF0FFF) == 0) && (data == 0x30) )
                {
                    //erase sector
                    if(FLASH_CMD == 0x80)
                    {
                        memset((void*)((u32)FLASH_BUFFER512+(address&0x0000F000)),0xFF,0xFFF);
                        FLASH_CMD = 0x30;
                        FLASH_STATE = 1;
                    }
                    else
                    {
                        FLASH_CMD = 0; //error?
                        FLASH_STATE = 0;
                    }
                    FLASH_CMD_STATE = 0;
                }
                else if(address == 0x0E005555) //execute command
                {
                    //if(data != 0xA0) Debug_DebugMsgArg("%02X",data);

                    FLASH_CMD_STATE = 0;

                    switch(data)
                    {
                        case 0x10: //After 80h -- (erase entire chip)
                            if(FLASH_CMD == 0x80)
                            {
                                memset(FLASH_BUFFER512,0xFF,sizeof(FLASH_BUFFER512));
                                FLASH_CMD = 0x10;
                                FLASH_STATE = 1;
                            }
                            else
                            {
                                FLASH_CMD = 0; //error?
                                FLASH_STATE = 0;
                            }
                            return;
                        case 0x80: //erase command (useless by itself)
                            FLASH_CMD = 0x80;
                            FLASH_STATE = 1;
                            return;
                        case 0x90: //(enter ID mode)
                            FLASH_CMD = 0x90;
                            FLASH_STATE = 1;
                            return;
                        case 0xA0: //(write 1 byte)
                            FLASH_CMD = 0xA0;
                            FLASH_STATE = 0;
                            FLASH_CMD_STATE = 3;
                            return;

                        case 0xF0: //(terminate ID mode)
                        default: //error?
                            FLASH_STATE = 0;
                            FLASH_CMD = 0;
                            return;
                    }
                }
                else FLASH_CMD_STATE = 0; //error?
            }
            else if(FLASH_CMD_STATE == 3) //for commands that need another instruction
            {
                if(FLASH_CMD == 0xA0) //write byte
                {
                    if((address >= 0x0E000000) && (address < 0x0E010000))
                        FLASH_BUFFER512[address-0x0E000000] = data;
                }
                FLASH_CMD_STATE = 0;
                FLASH_CMD = 0;
                FLASH_STATE = 0;
            }
            return;
        }
        case SAV_FLASH1M:
        {
            if(FLASH_CMD_STATE < 2)
            {
                if(address == 0x0E005555)
                {
                    if((data == 0xAA) && (FLASH_CMD_STATE == 0)) FLASH_CMD_STATE = 1;
                    else FLASH_CMD_STATE = 0;
                }
                else if(address == 0x0E002AAA)
                {
                    if((FLASH_CMD_STATE == 1) && (data == 0x55)) FLASH_CMD_STATE = 2;
                    else FLASH_CMD_STATE = 0;
                }
            }
            else if(FLASH_CMD_STATE == 2)
            {
                if( ((address&0xFF000000) == 0x0E000000) && ((address&0x00FF0FFF) == 0) && (data == 0x30) )
                {
                    //erase sector
                    if(FLASH_CMD == 0x80)
                    {
                        memset((void*)((u32)FLASH_1M_PTR+(address&0x0000F000)),0xFF,0xFFF);
                        FLASH_CMD = 0x30;
                        FLASH_STATE = 1;
                    }
                    else
                    {
                        FLASH_CMD = 0; //error?
                        FLASH_STATE = 0;
                    }
                    FLASH_CMD_STATE = 0;
                }
                else if(address == 0x0E005555) //execute command
                {
                    //Debug_DebugMsgArg("%02X",data);

                    FLASH_CMD_STATE = 0;

                    switch(data)
                    {
                        case 0x10: //After 80h -- (erase entire chip)
                            if(FLASH_CMD == 0x80)
                            {
                                memset(FLASH_BUFFER1M,0xFF,sizeof(FLASH_BUFFER1M));
                                FLASH_CMD = 0x10;
                                FLASH_STATE = 1;
                            }
                            else
                            {
                                FLASH_CMD = 0; //error?
                                FLASH_STATE = 0;
                            }
                            return;
                        case 0x80: //erase command (useless by itself)
                            FLASH_CMD = 0x80;
                            FLASH_STATE = 1;
                            return;
                        case 0x90: //(enter ID mode)
                            FLASH_CMD = 0x90;
                            FLASH_STATE = 1;
                            return;
                        case 0xA0: //(write 1 byte)
                            FLASH_CMD = 0xA0;
                            FLASH_STATE = 0;
                            FLASH_CMD_STATE = 3;
                            return;
                        case 0xB0: //(change bank)
                            FLASH_CMD = 0xB0;
                            FLASH_STATE = 0;
                            FLASH_CMD_STATE = 3;
                            return;

                        case 0xF0: //(terminate ID mode)
                        default: //error?
                            FLASH_STATE = 0;
                            FLASH_CMD = 0;
                            return;
                    }
                }
                else FLASH_CMD_STATE = 0; //error?
            }
            else if(FLASH_CMD_STATE == 3) //for commands that need another instruction
            {
                if(FLASH_CMD == 0xA0) //write byte
                {
                    if((address >= 0x0E000000) && (address < 0x0E010000))
                        FLASH_1M_PTR[address-0x0E000000] = data;
                }
                else if(FLASH_CMD == 0xB0) //change bank
                {
                    if(address == 0x0E000000)
                    {
                        FLASH_1M_PTR = (data&1) ? &(FLASH_BUFFER1M[0x10000]) : FLASH_BUFFER1M;
                    }
                }
                FLASH_CMD_STATE = 0;
                FLASH_CMD = 0;
                FLASH_STATE = 0;
            }
            return;
        }
        case SAV_EEPROM:
        {
            //Debug_DebugMsgArg("EEPROM: Write 8bit [%08X]=%02X",address,data);
            return;
        }
        case SAV_NONE:
        case SAV_AUTODETECT:
        default:
            return;
    }
}

void GBA_SaveWrite16(u32 address, u16 data)
{
    if(SAVE_TYPE == SAV_AUTODETECT)
    {
        //Debug_DebugMsgArg("Autodetect: Write 16bit [%08X]=%04X",address,data);

        //if(gba_dma3enabled())
        {
            SAVE_TYPE = SAV_EEPROM;
            //Debug_DebugMsgArg("Save type autodetect: EEPROM");
            GBA_ResetSaveBuffer();
            GBA_SaveReadFile();
            ConsolePrint("EEPROM");
        }
    }

    switch(SAVE_TYPE)
    {
        case SAV_EEPROM:
        {
            if(gba_dma3enabled())
            {
                if(eeprom_detect_size)
                {
                    eeprom_detect_size = 0;
                    if(gba_dma3numchunks() == 9)
                    {
                        EEPROM_SIZE = 512;
                        EEPROM_ADDRESS_BUS = 6;
                        EEPROM_ADDRESS_MASK = 0x1FF;
                        ConsolePrint(" 512B");
                    }
                    else if(gba_dma3numchunks() == 17)
                    {
                        EEPROM_SIZE = 8*1024;
                        EEPROM_ADDRESS_BUS = 14;
                        EEPROM_ADDRESS_MASK = 0x1FFF;
                        ConsolePrint(" 8KB");
                    }
                    else
                    {
                        Debug_ErrorMsgArg("EEPROM: Failed size autodetection. %d", gba_dma3numchunks());
                        ConsolePrint("\nFailed size autodetection.");
                    }

                    //Debug_DebugMsgArg("EEPROM_SIZE: %d",EEPROM_SIZE);
                }

                //Debug_DebugMsgArg("EEPROM: Write 16bit [%08X]=%04X",address,data);

                data &= 1; address &= 0xFF;

                if(address == 0) { EEPROM_CMD_LEN = 0; EEPROM_CMD = 0; EEPROM_DATA_STREAMING = 0; }

                if(EEPROM_CMD_LEN < 2)
                {
                    EEPROM_CMD_LEN++;
                    EEPROM_CMD <<= 1; EEPROM_CMD |= data; //add bit
                    EEPROM_ADDRESS = 0;
                }
                else if(EEPROM_CMD_LEN < 2+EEPROM_ADDRESS_BUS)
                {
                    EEPROM_CMD_LEN++;
                    EEPROM_ADDRESS <<= 1; EEPROM_ADDRESS |= data; //add bit
                }
                else if(EEPROM_CMD_LEN == 2+EEPROM_ADDRESS_BUS)
                {
                    EEPROM_CMD_LEN++;

                    if(EEPROM_CMD == 3) //read
                    {
                        //SETUP READING
                        //Debug_DebugMsgArg("EEPROM: READ %X",EEPROM_ADDRESS);
                        EEPROM_CMD_LEN = 0;
                        EEPROM_DATA_STREAMING = 68;

                        u32 addr = EEPROM_ADDRESS&EEPROM_ADDRESS_MASK;
                        EEPROM_READ_BUFFER = EEPROM_BUFFER[addr];
                    }
                    else if(EEPROM_CMD == 2)
                    {
                        EEPROM_READ_BUFFER = (u64)(data & 1);
                        EEPROM_DATA_STREAMING = 1;
                    }
                    else //ERROR
                    {
                        EEPROM_CMD_LEN = 0;
                    }
                }
                else //WRITING
                {
                    if(EEPROM_DATA_STREAMING < 64)
                    {
                        EEPROM_READ_BUFFER<<=1;
                        EEPROM_READ_BUFFER |= (u64)(data & 1);
                        EEPROM_DATA_STREAMING++;
                    }
                    else if(EEPROM_DATA_STREAMING == 64)
                    {
                        u32 addr = EEPROM_ADDRESS&EEPROM_ADDRESS_MASK;
                        EEPROM_BUFFER[addr] = EEPROM_READ_BUFFER;

                        //Debug_DebugMsgArg("EEPROM: WRITE %X",EEPROM_ADDRESS);

                        EEPROM_CMD_LEN = 0;
                    }
                }
            }

            return;
        }

        case SAV_SRAM:
        case SAV_FLASH:
        case SAV_FLASH512:
        case SAV_FLASH1M:
        case SAV_NONE:
        case SAV_AUTODETECT:
        default:
            return;
    }
    return;
}

//---------------------------------------------------------------

void GBA_SaveWriteFile(void)
{
    switch(SAVE_TYPE)
    {
        case SAV_SRAM:
        {
            FILE * f = fopen(SAVE_PATH,"wb");
            if(f == NULL)
            {
                Debug_ErrorMsgArg("Couldn't open file for saving.");
                return;
            }
            if(fwrite(SRAM_BUFFER, sizeof(SRAM_BUFFER), 1, f) != 1)
            {
                Debug_ErrorMsgArg("Couldn't save data to file.");
            }
            fclose(f);
            return;
        }
        case SAV_FLASH:
        case SAV_FLASH512:
        {
            FILE * f = fopen(SAVE_PATH,"wb");
            if(f == NULL)
            {
                Debug_ErrorMsgArg("Couldn't open file for saving.");
                return;
            }
            if(fwrite(FLASH_BUFFER512, sizeof(FLASH_BUFFER512), 1, f) != 1)
            {
                Debug_ErrorMsgArg("Couldn't save data to file.");
            }
            fclose(f);
            return;
        }
        case SAV_FLASH1M:
        {
            FILE * f = fopen(SAVE_PATH,"wb");
            if(f == NULL)
            {
                Debug_ErrorMsgArg("Couldn't open file for saving.");
                return;
            }
            if(fwrite(FLASH_BUFFER1M, sizeof(FLASH_BUFFER1M), 1, f) != 1)
            {
                Debug_ErrorMsgArg("Couldn't save data to file.");
            }
            fclose(f);
            return;
        }
        case SAV_EEPROM:
        {
            FILE * f = fopen(SAVE_PATH,"wb");
            if(f == NULL)
            {
                Debug_ErrorMsgArg("Couldn't open file for saving.");
                return;
            }
            if(fwrite(EEPROM_BUFFER, 1, EEPROM_SIZE, f) != EEPROM_SIZE)
            {
                Debug_ErrorMsgArg("Couldn't save data to file.");
            }
            fclose(f);
            return;
        }
        case SAV_NONE:
        case SAV_AUTODETECT:
        default:
            return;
    }
}

void GBA_SaveReadFile(void)
{
    switch(SAVE_TYPE)
    {
        case SAV_SRAM:
        {
            FILE * f = fopen(SAVE_PATH,"rb");
            if(f == NULL) return; //Maybe file didn't exist -- or error...
            if(fread(SRAM_BUFFER, sizeof(SRAM_BUFFER), 1, f) != 1)
            {
                Debug_ErrorMsgArg("Couldn't read data from file.");
            }
            fclose(f);
            return;
        }
        case SAV_FLASH:
        case SAV_FLASH512:
        {
            FILE * f = fopen(SAVE_PATH,"rb");
            if(f == NULL) return; //Maybe file didn't exist -- or error...
            if(fread(FLASH_BUFFER512, sizeof(FLASH_BUFFER512), 1, f) != 1)
            {
                Debug_ErrorMsgArg("Couldn't read data from file.");
            }
            fclose(f);
            return;
        }
        case SAV_FLASH1M:
        {
            FILE * f = fopen(SAVE_PATH,"rb");
            if(f == NULL) return; //Maybe file didn't exist -- or error...
            if(fread(FLASH_BUFFER1M, sizeof(FLASH_BUFFER1M), 1, f) != 1)
            {
                //Debug_ErrorMsgArg("Couldn't read data from file.");
                //Maybe save type autodetection said it is FLASH1M , but it is FLASH512 and
                //tried to read 1M...
            }
            fclose(f);
            return;
        }
        case SAV_EEPROM:
        {
            FILE * f = fopen(SAVE_PATH,"rb");
            if(f == NULL) return; //Maybe file didn't exist -- or error...

            fseek(f, 0, SEEK_END);
            u32 size = ftell(f);
            rewind(f);

            if((size != 512) && (size != 8*1024)) Debug_ErrorMsgArg("Corrupted saved data (wrong size).");

            if(fread(EEPROM_BUFFER, 1, size, f) != size)
            {
                Debug_ErrorMsgArg("Couldn't read data from file.");
            }
            fclose(f);
            return;
        }
        case SAV_NONE:
        case SAV_AUTODETECT:
        default:
            return;
    }
}
