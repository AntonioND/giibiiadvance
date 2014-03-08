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
#include <stdio.h>
#include <malloc.h>

#include "build_options.h"
#include "file_utils.h"
#include "config.h"

#include "gb_core/gameboy.h"
#include "gb_core/sound.h"
#include "gb_core/gb_main.h"
#include "gb_core/video.h"

#include "gba_core/sound.h"

t_config EmulatorConfig = { //Default options...
    0, //debug_msg_enable
    2, //screen_size
    0, //load_from_boot_rom
    -1, //frameskip
    0, //oglfilter
    0, //auto_close_debugger
    //---------
    100, //server_buffer_len
    64, //volume
    0x3F, //chn_flags
    0, //snd_mute
    //---------
    -1, //hardware_type
    SERIAL_GBPRINTER, //serial_device
    0, //enableblur
    0, //realcolors

    //gb palette is not saved here, it is saved in gb_main.c
    //key config not here, either...
    };
/*
const char * GBKeyNames[P_NUM_KEYS] =
    { "A", "B", "Start", "Select", "Right", "Left", "Up", "Down" };
*/
#define CFG_DB_MSG_ENABLE "debug_msg_enable"
// "true" - "false"

#define CFG_SCREEN_SIZE "screen_size"
// unsigned integer ( "1" - "3" )

#define CFG_LOAD_BOOT_ROM "load_boot_rom"
// "true" - "false"

#define CFG_FRAMESKIP "frameskip"
// "-1" - "9"

#define CFG_OPENGL_FILTER "opengl_filter"
char * oglfiltertype[] = { "nearest", "linear" };

#define CFG_AUTO_CLOSE_DEBUGGER "auto_close_debugger"
// "true" - "false"

#define CFG_SND_BUF_LEN "sound_buffer_lenght"
// unsigned integer ( "50" - "250" ) -- in steps of 50

#define CFG_SND_CHN_ENABLE "channels_enabled"
// "#3F" 3F = flags

#define CFG_SND_VOLUME "volume"
// "#VV" 00h - 80h

#define CFG_SND_MUTE "sound_mute"
// "true" - "false"

#define CFG_HW_TYPE "hardware_type"
char * hwtype[] = { "Auto", "GB", "GBP", "SGB", "SGB2", "GBC", "GBA" };

#define CFG_SERIAL_DEVICE "serial_device"
char * serialdevice[] = { "None", "GBPrinter" }; //, "Gameboy" };

#define CFG_ENABLE_BLUR "enable_blur"
// "true" - "false"

#define CFG_REAL_GB_COLORS "real_colors"
// "true" - "false"

#define CFG_GB_PALETTE "gb_palette"
// "#RRGGBB"

//---------------------------------------------------------------------

void Config_Save(void)
{
    char path[MAX_PATHLEN];
    if(DirGetRunningPath()) sprintf(path,"%sGiiBiiAdvance.ini",DirGetRunningPath());
    else strcpy(path,"GiiBiiAdvance.ini");
    FILE * ini_file = fopen(path,"wb");
    if(ini_file == NULL) return;

    fprintf(ini_file,"[General]\r\n");
    fprintf(ini_file,CFG_DB_MSG_ENABLE "=%s\r\n",EmulatorConfig.debug_msg_enable?"true":"false");
    fprintf(ini_file,CFG_SCREEN_SIZE "=%d\r\n",EmulatorConfig.screen_size);
    fprintf(ini_file,CFG_LOAD_BOOT_ROM "=%s\r\n",EmulatorConfig.load_from_boot_rom?"true":"false");
    fprintf(ini_file,CFG_FRAMESKIP "=%d\r\n",EmulatorConfig.frameskip);
    fprintf(ini_file,CFG_OPENGL_FILTER "=%s\r\n",oglfiltertype[EmulatorConfig.oglfilter]);
    fprintf(ini_file,CFG_AUTO_CLOSE_DEBUGGER "=%s\r\n",EmulatorConfig.auto_close_debugger?"true":"false");
    fprintf(ini_file,"\r\n");

    fprintf(ini_file,"[Sound]\r\n");
    fprintf(ini_file,CFG_SND_BUF_LEN "=%03d\r\n",EmulatorConfig.server_buffer_len);
    int volume, chn_flags;
    GBA_SoundGetConfig(&volume,&chn_flags); //GB doesn't get all information
    fprintf(ini_file,CFG_SND_CHN_ENABLE "=#%02X\r\n",chn_flags);
    fprintf(ini_file,CFG_SND_VOLUME "=#%02X\r\n",volume);
    fprintf(ini_file,CFG_SND_MUTE "=%s\r\n",EmulatorConfig.snd_mute?"true":"false");
    fprintf(ini_file,"\r\n");

    fprintf(ini_file,"[GameBoy]\r\n");
    fprintf(ini_file,CFG_HW_TYPE "=%s\r\n",hwtype[EmulatorConfig.hardware_type+1]);
    fprintf(ini_file,CFG_SERIAL_DEVICE "=%s\r\n",serialdevice[EmulatorConfig.serial_device]);
    fprintf(ini_file,CFG_ENABLE_BLUR "=%s\r\n",EmulatorConfig.enableblur?"true":"false");
    fprintf(ini_file,CFG_REAL_GB_COLORS "=%s\r\n",EmulatorConfig.realcolors?"true":"false");
    u8 r,g,b; GB_ConfigGetPalette(&r,&g,&b);
    fprintf(ini_file,CFG_GB_PALETTE "=#%02X%02X%02X\r\n",r,g,b);
    fprintf(ini_file,"\r\n");

/*
    fprintf(ini_file,"[Controls]\r\n");
    int player, key;
    for(player = 0; player < 4; player ++) for(key = 0; key < P_NUM_KEYS; key ++)
    {
        char * keyname = SDL_GetKeyName(Config_Controls_Get_Key(player,key));
        if(strcmp(keyname,"unknown key") != 0)
            fprintf(ini_file,"P%d_%s=%s\r\n",player+1,GBKeyNames[key],keyname);
    }
    if(Config_Controls_Get_Key(0,P_KEY_SPEEDUP) != 0)
        fprintf(ini_file,"SpeedUp=%s\r\n",SDL_GetKeyName(Config_Controls_Get_Key(0,P_KEY_SPEEDUP)));
    else
        fprintf(ini_file,"SpeedUp=\r\n");
    fprintf(ini_file,"\r\n");
*/
    fclose(ini_file);
}

void Config_Load(void)
{
    GB_ConfigSetPalette(0xB0,0xFF,0xB0);

    char path[MAX_PATHLEN];
    if(DirGetRunningPath()) sprintf(path,"%sGiiBiiAdvance.ini",DirGetRunningPath());
    else strcpy(path,"GiiBiiAdvance.ini");

    char * ini;
    unsigned int size;
    FileLoad_NoError(path,(void*)&ini,&size);
    if(ini == NULL) return;
    ini = realloc(ini,size+1);
    ini[size] = '\0';

    char * tmp = strstr(ini,CFG_DB_MSG_ENABLE);
    if(tmp)
    {
        tmp += strlen(CFG_DB_MSG_ENABLE) + 1;
        if(strncmp(tmp,"true",strlen("true")) == 0)
            EmulatorConfig.debug_msg_enable = 1;
        else
            EmulatorConfig.debug_msg_enable = 0;
    }

    tmp = strstr(ini,CFG_SCREEN_SIZE);
    if(tmp)
    {
        tmp += strlen(CFG_SCREEN_SIZE) + 1;
        EmulatorConfig.screen_size = atoi(tmp);
        if(EmulatorConfig.screen_size > 4) EmulatorConfig.screen_size = 4;
        else if(EmulatorConfig.screen_size < 2) EmulatorConfig.screen_size = 2;
    }

    tmp = strstr(ini,CFG_LOAD_BOOT_ROM);
    if(tmp)
    {
        tmp += strlen(CFG_LOAD_BOOT_ROM) + 1;
        if(strncmp(tmp,"true",strlen("true")) == 0)
            EmulatorConfig.load_from_boot_rom = 1;
        else
            EmulatorConfig.load_from_boot_rom = 0;
    }

    EmulatorConfig.frameskip = 0;
    tmp = strstr(ini,CFG_FRAMESKIP);
    if(tmp)
    {
        tmp += strlen(CFG_FRAMESKIP) + 1;
        if(*tmp == '-') //*(tmp+1) == 1
             EmulatorConfig.frameskip = -1;
        else
            EmulatorConfig.frameskip = *tmp - '0';
    }

    tmp = strstr(ini,CFG_OPENGL_FILTER);
    if(tmp)
    {
        tmp += strlen(CFG_OPENGL_FILTER) + 1;

        int i, result = 0;
        for(i = 0; i < ARRAY_NUM_ELEMENTS(oglfiltertype); i ++)
            if(strncmp(tmp,oglfiltertype[i],strlen(oglfiltertype[i])) == 0)
                result = i;

        EmulatorConfig.oglfilter = result;
    }

    tmp = strstr(ini,CFG_AUTO_CLOSE_DEBUGGER);
    if(tmp)
    {
        tmp += strlen(CFG_AUTO_CLOSE_DEBUGGER) + 1;
        if(strncmp(tmp,"true",strlen("true")) == 0)
            EmulatorConfig.auto_close_debugger = 1;
        else
            EmulatorConfig.auto_close_debugger = 0;
    }

    //SOUND
    tmp = strstr(ini,CFG_SND_BUF_LEN);
    if(tmp)
    {
        tmp += strlen(CFG_SND_BUF_LEN) + 1;

        char aux[4]; aux[3] = '\0';
        aux[0] = *tmp++;
        aux[1] = *tmp++;
        aux[2] = *tmp;

        EmulatorConfig.server_buffer_len = asciidec_to_int(aux);
        if(EmulatorConfig.server_buffer_len < 50) EmulatorConfig.server_buffer_len = 50;
        else if(EmulatorConfig.server_buffer_len > 250) EmulatorConfig.server_buffer_len = 250;
    }

    int vol = 64, chn_flags = 0x3F;
    tmp = strstr(ini,CFG_SND_CHN_ENABLE);
    if(tmp)
    {
        tmp += strlen(CFG_SND_CHN_ENABLE) + 1;
        if(*tmp == '#')
        {
            tmp ++;
            char aux[3]; aux[2] = '\0';
            aux[0] = *tmp; tmp++;
            aux[1] = *tmp;
            chn_flags = asciihex_to_int(aux);
        }
    }

    tmp = strstr(ini,CFG_SND_VOLUME);
    if(tmp)
    {
        tmp += strlen(CFG_SND_VOLUME) + 1;
        if(*tmp == '#')
        {
            tmp ++;
            char aux[3]; aux[2] = '\0';
            aux[0] = *tmp; tmp++;
            aux[1] = *tmp;
            vol = asciihex_to_int(aux);
        }
    }

    GB_SoundSetConfig(vol,chn_flags);
    GBA_SoundSetConfig(vol,chn_flags);

    tmp = strstr(ini,CFG_SND_MUTE);
    if(tmp)
    {
        tmp += strlen(CFG_SND_MUTE) + 1;
        if(strncmp(tmp,"true",strlen("true")) == 0)
            EmulatorConfig.snd_mute = 1;
        else
            EmulatorConfig.snd_mute = 0;
    }

    //GAMEBOY
    tmp = strstr(ini,CFG_HW_TYPE);
    if(tmp)
    {
        tmp += strlen(CFG_HW_TYPE) + 1;

        int i, result = -1;
        for(i = 0; i < ARRAY_NUM_ELEMENTS(hwtype); i ++)
            if(strncmp(tmp,hwtype[i],strlen(hwtype[i])) == 0)
                result = i;

        EmulatorConfig.hardware_type = result - 1;
    }

    tmp = strstr(ini,CFG_SERIAL_DEVICE);
    if(tmp)
    {
        tmp += strlen(CFG_SERIAL_DEVICE) + 1;

        int i, result = 0;
        for(i = 0; i < ARRAY_NUM_ELEMENTS(serialdevice); i ++)
            if(strncmp(tmp,serialdevice[i],strlen(serialdevice[i])) == 0)
                result = i;

        EmulatorConfig.serial_device = result;
    }

    tmp = strstr(ini,CFG_ENABLE_BLUR);
    if(tmp)
    {
        tmp += strlen(CFG_ENABLE_BLUR) + 1;
        if(strncmp(tmp,"true",strlen("true")) == 0)
            EmulatorConfig.enableblur = 1;
        else
            EmulatorConfig.enableblur = 0;

        GB_EnableBlur(EmulatorConfig.enableblur);
    }

    tmp = strstr(ini,CFG_REAL_GB_COLORS);
    if(tmp)
    {
        tmp += strlen(CFG_REAL_GB_COLORS) + 1;
        if(strncmp(tmp,"true",strlen("true")) == 0)
            EmulatorConfig.realcolors = 1;
        else
            EmulatorConfig.realcolors = 0;

        GB_EnableRealColors(EmulatorConfig.realcolors);
    }

    tmp = strstr(ini,CFG_GB_PALETTE);
    if(tmp)
    {
        tmp += strlen(CFG_GB_PALETTE) + 1;
        if(*tmp == '#')
        {
            tmp ++;

            int i;
            char aux[3]; aux[2] = '\0';
            for(i = 0; i < 2; i++) { aux[i] = *tmp; tmp ++; } u8 r = asciihex_to_int(aux);
            for(i = 0; i < 2; i++) { aux[i] = *tmp; tmp ++; } u8 g = asciihex_to_int(aux);
            for(i = 0; i < 2; i++) { aux[i] = *tmp; tmp ++; } u8 b = asciihex_to_int(aux);

            //Debug_DebugMsgArg("%02x %02x %02x",r,g,b);

            GB_ConfigSetPalette(r,g,b);
        }
    }

/*
    int player, key;
    for(player = 0; player < 4; player ++) for(key = 0; key < P_NUM_KEYS; key ++)
    {
        char temp_str[64];
        sprintf(temp_str,"P%d_%s",player+1,GBKeyNames[key]);

        tmp = strstr(ini,temp_str);
        if(tmp)
        {
            tmp += strlen(temp_str) + 1;

            SDLKey k;
            SDLKey result = SDLK_LAST; int result_len = 0;

            for(k = SDLK_FIRST; k < SDLK_LAST; k++)
            {
                int len = strlen(SDL_GetKeyName(k));
                if(strncmp(tmp,SDL_GetKeyName(k),len) == 0)
                {
                    if(len > result_len)
                    {
                        result_len = len;
                        result = k;
                    }
                }
            }

            if(result < SDLK_LAST)
            {
                Config_Controls_Set_Key(player,key,result);
            }
        }
    }
    tmp = strstr(ini,"SpeedUp");
    if(tmp)
    {
        //if exists, and it is empty, set it to none.
        Config_Controls_Set_Key(0,P_KEY_SPEEDUP,0);

        tmp += strlen("SpeedUp") + 1;

        SDLKey k;
        SDLKey result = SDLK_LAST; int result_len = 0;

        for(k = SDLK_FIRST; k < SDLK_LAST; k++)
        {
            int len = strlen(SDL_GetKeyName(k));
            if(strncmp(tmp,SDL_GetKeyName(k),len) == 0)
            {
                if(len > result_len)
                {
                    result_len = len;
                    result = k;
                }
            }
        }

        if(result < SDLK_LAST)
        {
            Config_Controls_Set_Key(0,P_KEY_SPEEDUP,result);
        }
    }
*/

    free(ini);
}

//---------------------------------------------------------------------
/*
SDLKey player_key[4][P_NUM_KEYS] = { //This is the default configuration
    { SDLK_x,SDLK_z,SDLK_RETURN,SDLK_RSHIFT,SDLK_RIGHT,SDLK_LEFT,SDLK_UP,SDLK_DOWN },
    { 0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0 }
    };

SDLKey player_key_speedup = SDLK_SPACE;

//----------------------------

inline void Config_Controls_Set_Key(int player, _key_control_enum_ keyindex, SDLKey keysym)
{
    if(keyindex == P_KEY_SPEEDUP) player_key_speedup = keysym;
    else player_key[player][keyindex] = keysym;
}

inline SDLKey Config_Controls_Get_Key(int player, _key_control_enum_ keyindex)
{
    if(keyindex == P_KEY_SPEEDUP) return player_key_speedup;
    return player_key[player][keyindex];
}

//---------------------------------------------------------------------

*/

