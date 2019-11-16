// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef CONFIG__
#define CONFIG__

typedef struct {
    int debug_msg_enable;
    int screen_size;
    // GBA always tries to load the BIOS, this only skips the initial logo
    int load_from_boot_rom;
    int frameskip; // -1 = auto, 0-9 = fixed frameskip
    int oglfilter;
    int auto_close_debugger;
    unsigned int webcam_select; // 0 = CV_CAP_ANY

    //Sound
    //-----
    int volume;
    int chn_flags;
    int snd_mute;

    //GameBoy
    //-------
    int hardware_type;
    int serial_device;
    int enableblur;
    int realcolors;
    unsigned int gbcam_exposure_reference;

    // The GB palette is not stored here, it is stored in gb_main.c

    // The input configuration is in input_utils.c

} t_config;

extern t_config EmulatorConfig;

void Config_Save(void);
void Config_Load(void);

#endif // CONFIG__
