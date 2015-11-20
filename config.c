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
#include <stdio.h>
#include <stdlib.h>

#include "build_options.h"
#include "file_utils.h"
#include "config.h"
#include "input_utils.h"

#include "gb_core/gameboy.h"
#include "gb_core/sound.h"
#include "gb_core/gb_main.h"
#include "gb_core/video.h"

#include "gba_core/sound.h"

t_config EmulatorConfig = { //Default values...
    0, //debug_msg_enable
    2, //screen_size
    0, //load_from_boot_rom
    0, //frameskip
    0, //oglfilter
    0, //auto_close_debugger
    0, //webcam_select
    //---------
    64, //volume
    0x3F, //chn_flags
    0, //snd_mute
    //---------
    -1, //hardware_type
    SERIAL_GBPRINTER, //serial_device
    0, //enableblur
    0, //realcolors
    0x0200, //gbcam_exposure_reference

    //gb palette is not stored here, it is stored in gb_main.c
    //key config not here, either... it's in input_utils.c
};

#define CFG_DB_MSG_ENABLE "debug_msg_enable"
// "true" - "false"

#define CFG_SCREEN_SIZE "screen_size"
// unsigned integer ( "1" - "3" )

#define CFG_LOAD_BOOT_ROM "load_boot_rom"
// "true" - "false"

#define CFG_FRAMESKIP "frameskip"
// "-1" - "4"

#define CFG_OPENGL_FILTER "opengl_filter"
static const char * oglfiltertype[] = { "nearest", "linear" };

#define CFG_AUTO_CLOSE_DEBUGGER "auto_close_debugger"
// "true" - "false"

#define CFG_WEBCAM_SELECT "webcam_select"
// "0" - "9"

#define CFG_SND_CHN_ENABLE "channels_enabled"
// "#3F" 3F = flags

#define CFG_SND_VOLUME "volume"
// "#VV" 00h - 80h

#define CFG_SND_MUTE "sound_mute"
// "true" - "false"

#define CFG_HW_TYPE "hardware_type"
static const char * hwtype[] = { "Auto", "DMG", "MGB", "SGB", "SG2", "CGB", "AGB", "AGS" };

#define CFG_SERIAL_DEVICE "serial_device"
static const char * serialdevice[] = { "None", "GBPrinter" }; //, "Gameboy" };

#define CFG_ENABLE_BLUR "enable_blur"
// "true" - "false"

#define CFG_REAL_GB_COLORS "real_colors"
// "true" - "false"

#define CFG_GBCAM_EXPOSURE_REFERENCE "gbcam_exposure_reference"
// "#NNNN"

#define CFG_GB_PALETTE "gb_palette"
// "#RRGGBB"

//---------------------------------------------------------------------

void Config_Save(void)
{
    char path[MAX_PATHLEN];
    if(DirGetRunningPath()) s_snprintf(path,sizeof(path),"%sGiiBiiAdvance.ini",DirGetRunningPath());
    else s_strncpy(path,"GiiBiiAdvance.ini",sizeof(path));
    FILE * ini_file = fopen(path,"wb");
    if(ini_file == NULL) return;

    fprintf(ini_file,"[General]\n");
    fprintf(ini_file,CFG_DB_MSG_ENABLE "=%s\n",EmulatorConfig.debug_msg_enable?"true":"false");
    fprintf(ini_file,CFG_SCREEN_SIZE "=%d\n",EmulatorConfig.screen_size);
    fprintf(ini_file,CFG_LOAD_BOOT_ROM "=%s\n",EmulatorConfig.load_from_boot_rom?"true":"false");
    fprintf(ini_file,CFG_FRAMESKIP "=%d\n",EmulatorConfig.frameskip);
    fprintf(ini_file,CFG_OPENGL_FILTER "=%s\n",oglfiltertype[EmulatorConfig.oglfilter]);
    fprintf(ini_file,CFG_AUTO_CLOSE_DEBUGGER "=%s\n",EmulatorConfig.auto_close_debugger?"true":"false");
    fprintf(ini_file,CFG_WEBCAM_SELECT "=%d\n",EmulatorConfig.webcam_select);
    fprintf(ini_file,"\n");

    fprintf(ini_file,"[Sound]\n");
    int volume, chn_flags;
    GBA_SoundGetConfig(&volume,&chn_flags); //GB doesn't get all information
    fprintf(ini_file,CFG_SND_CHN_ENABLE "=#%02X\n",chn_flags);
    fprintf(ini_file,CFG_SND_VOLUME "=#%02X\n",volume);
    fprintf(ini_file,CFG_SND_MUTE "=%s\n",EmulatorConfig.snd_mute?"true":"false");
    fprintf(ini_file,"\n");

    fprintf(ini_file,"[GameBoy]\n");
    fprintf(ini_file,CFG_HW_TYPE "=%s\n",hwtype[EmulatorConfig.hardware_type+1]);
    fprintf(ini_file,CFG_SERIAL_DEVICE "=%s\n",serialdevice[EmulatorConfig.serial_device]);
    fprintf(ini_file,CFG_ENABLE_BLUR "=%s\n",EmulatorConfig.enableblur?"true":"false");
    fprintf(ini_file,CFG_REAL_GB_COLORS "=%s\n",EmulatorConfig.realcolors?"true":"false");
    fprintf(ini_file,CFG_GBCAM_EXPOSURE_REFERENCE "=#%04X\n",EmulatorConfig.gbcam_exposure_reference);

    u8 r,g,b; GB_ConfigGetPalette(&r,&g,&b);
    fprintf(ini_file,CFG_GB_PALETTE "=#%02X%02X%02X\n",r,g,b);
    fprintf(ini_file,"\n");

    fprintf(ini_file,"[Controls]\n");
    int player, key;
    for(player = 0; player < 4; player ++)
    {
        if(Input_PlayerGetEnabled(player) || (player == 0)) // P1 always enabled
        {
            int controller = Input_PlayerGetController(player);

            int save_keys = 0;

            if(controller != -1) // joystick
            {
                char * controllername = Input_GetJoystickName(Input_PlayerGetController(player));
                if(strlen(controllername) > 0)
                {
                    fprintf(ini_file,"P%d_Enabled=true\n",player+1);
                    fprintf(ini_file,"P%d_Controller=[%s]\n",player+1,controllername);
                    save_keys = 1;
                }
                else
                {
                    fprintf(ini_file,"P%d_Enabled=false\n",player+1);
                    save_keys = 0;
                }
            }
            else // keyboard
            {
                fprintf(ini_file,"P%d_Enabled=true\n",player+1);
                fprintf(ini_file,"P%d_Controller=[Keyboard]\n",player+1);
                save_keys = 1;
            }

            if(save_keys)
            {
                for(key = 0; key < P_NUM_KEYS; key ++)
                {
                    SDL_Scancode code = Input_ControlsGetKey(player,key);
                    fprintf(ini_file,"P%d_%s=%d\n",player+1,GBKeyNames[key],code);
                }
            }
        }
        else // player disabled
        {
            fprintf(ini_file,"P%d_Enabled=false\n",player+1);
        }
    }
/*
    if(Config_Controls_Get_Key(0,P_KEY_SPEEDUP) != 0)
        fprintf(ini_file,"SpeedUp=%s\n",SDL_GetKeyName(Config_Controls_Get_Key(0,P_KEY_SPEEDUP)));
    else
        fprintf(ini_file,"SpeedUp=\n");
    fprintf(ini_file,"\n");
*/
    fclose(ini_file);
}

void Config_Load(void)
{
    GB_ConfigSetPalette(0xB0,0xFF,0xB0);

    char path[MAX_PATHLEN];
    if(DirGetRunningPath()) s_snprintf(path,sizeof(path),"%sGiiBiiAdvance.ini",DirGetRunningPath());
    else s_strncpy(path,"GiiBiiAdvance.ini",sizeof(path));

    char * ini;
    unsigned int size;
    FileLoad_NoError(path,(void*)&ini,&size);
    if(ini == NULL) return;
    char * new_ini = realloc(ini,size+1);
    if(new_ini) ini = new_ini;
    else
    {
        free(ini);
        return;
    }

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
        if(*tmp == '-') //&& (*(tmp+1) == 1)
             EmulatorConfig.frameskip = -1;
        else
            EmulatorConfig.frameskip = *tmp - '0';

        if(EmulatorConfig.frameskip > 4) EmulatorConfig.frameskip = 4;
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

    tmp = strstr(ini,CFG_WEBCAM_SELECT);
    if(tmp)
    {
        tmp += strlen(CFG_WEBCAM_SELECT) + 1;
        EmulatorConfig.webcam_select = *tmp - '0';
        if(EmulatorConfig.webcam_select > 9) EmulatorConfig.webcam_select = 9;
    }

    //SOUND
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

    tmp = strstr(ini,CFG_GBCAM_EXPOSURE_REFERENCE);
    if(tmp)
    {
        tmp += strlen(CFG_GBCAM_EXPOSURE_REFERENCE) + 1;
        if(*tmp == '#')
        {
            tmp ++;
            char aux[5]; aux[4] = '\0';
            aux[0] = *tmp++;
            aux[1] = *tmp++;
            aux[2] = *tmp++;
            aux[3] = *tmp++;
            unsigned int value = asciihex_to_int(aux);

            if(value <= 0xFFFF)
                EmulatorConfig.gbcam_exposure_reference = value;
        }
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

    int player, key;
    for(player = 0; player < 4; player ++)
    {
        int player_enabled = 0;

        char temp_str[64];
        if(player > 0)
        {
            s_snprintf(temp_str,sizeof(temp_str),"P%d_Enabled=",player+1);
            tmp = strstr(ini,temp_str);
            if(tmp)
            {
                tmp += strlen(temp_str);
                if(strncmp(tmp,"true",strlen("true")) == 0)
                    player_enabled = 1;
            }
        }
        else // player 1 always enabled
        {
            player_enabled = 1;
        }

        Input_PlayerSetEnabled(player, player_enabled);

        if(player_enabled) // read the rest of the configuration
        {
            s_snprintf(temp_str,sizeof(temp_str),"P%d_Controller=[",player+1);
            tmp = strstr(ini,temp_str);
            if(tmp)
            {
                tmp += strlen(temp_str);
                s_strncpy(temp_str,tmp,sizeof(temp_str));
                int i;
                for(i = 0; i < sizeof(temp_str); i++) if(temp_str[i] == ']') temp_str[i] = '\0';
                //temp_str now has the name of the controller

                int index = Input_GetJoystickFromName(temp_str);
                if(index == -2) // didn't find that name, don't load the controller configuration
                {
                    // default player 1, disable other players
                    if(player != 0)
                        Input_PlayerSetEnabled(player, 0);
                }
                else
                {
                    Input_PlayerSetController(player,index);

                    //now, read keys
                    for(key = 0; key < P_NUM_KEYS; key ++)
                    {
                        s_snprintf(temp_str,sizeof(temp_str),"P%d_%s=",player+1,GBKeyNames[key]);
                        tmp = strstr(ini,temp_str);
                        if(tmp)
                        {
                            tmp += strlen(temp_str);

                            int btn;
                            if(sscanf(tmp,"%d",&btn) == 1)
                            {
                                Input_ControlsSetKey(player,key,btn);
                            }
                        }
                    }
                }

            }
            else
            {
                //Configuration not found: disable player
                if(player != 0)
                    Input_PlayerSetEnabled(player, 0);
            }
        }
    }


/*
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

