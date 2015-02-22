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

#include <SDL2/SDL.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "win_main.h"
#include "win_utils.h"
#include "win_main_config.h"
#include "win_main_config_input.h"

#include "../config.h"
#include "../file_explorer.h"
#include "../sound_utils.h"
#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../build_options.h"
#include "../general_utils.h"
#include "../file_utils.h"
#include "../input_utils.h"

#include "../gb_core/sound.h"
#include "../gb_core/debug.h"
#include "../gb_core/gb_main.h"
#include "../gb_core/interrupts.h"
#include "../gb_core/video.h"
#include "../gb_core/general.h"
#include "../gb_core/rom.h"
#include "../gb_core/camera.h"

#include "../gba_core/gba.h"
#include "../gba_core/bios.h"
#include "../gba_core/disassembler.h"
#include "../gba_core/save.h"
#include "../gba_core/video.h"
#include "../gba_core/rom.h"
#include "../gba_core/sound.h"

#include "win_gb_debugger.h"

#include "win_gba_debugger.h"

//------------------------------------------------------------------

static int WinIDMain = -1;

//------------------------------------------------------------------

static int _win_main_frameskip = 0;
static int _win_main_frameskipcount = 0;

void Win_MainSetFrameskip(int frameskip)
{
    if(_win_main_frameskip == frameskip)
        return;

    _win_main_frameskipcount = 0;
    _win_main_frameskip = frameskip;
}

static void _win_main_update_frameskip(void)
{
    if(_win_main_frameskipcount >= _win_main_frameskip)
    {
        _win_main_frameskipcount = 0;
        return;
    }

    _win_main_frameskipcount ++;
}

static int _win_main_has_to_frameskip(void)
{
    return (_win_main_frameskipcount != 0); // skip when not 0
}

//------------------------------------------------------------------

#define RUNNING_NONE 0
#define RUNNING_GB   1
#define RUNNING_GBA  2

static int WIN_MAIN_RUNNING = RUNNING_NONE;

//------------------------------------------------------------------

static int WinMain_FPS;
static int WinMain_frames_drawn = 0;
static SDL_TimerID WinMain_FPS_timer;

static Uint32 _fps_callback_function(Uint32 interval, void *param)
{
    if(WIN_MAIN_RUNNING == RUNNING_GB)
        GB_HandleRTC();

    WinMain_FPS = WinMain_frames_drawn;
    WinMain_frames_drawn = 0;
    char caption[60];
    if(_win_main_frameskip > 0)
        s_snprintf(caption,sizeof(caption),"GiiBiiAdvance: %d fps - %.2f%% - (%d)",WinMain_FPS,
                   (float)WinMain_FPS*10.0f/6.0f,_win_main_frameskip);
    else
        s_snprintf(caption,sizeof(caption),"GiiBiiAdvance: %d fps - %.2f%%",WinMain_FPS,(float)WinMain_FPS*10.0f/6.0f);
    WH_SetCaption(WinIDMain,caption);

    return interval;
}

static void FPS_TimerInit(void)
{
    WinMain_FPS_timer = SDL_AddTimer(1000, _fps_callback_function, NULL);
    if(WinMain_FPS_timer == 0)
    {
        Debug_LogMsgArg("FPS_TimerInit(): SDL_AddTimer() failed: %s",SDL_GetError());
        Debug_DebugMsgArg("FPS_TimerInit(): SDL_AddTimer() failed: %s",SDL_GetError());
    }
}

static void FPS_TimerEnd(void)
{
    SDL_RemoveTimer(WinMain_FPS_timer);
}

//------------------------------------------------------------------

static int WIN_MAIN_CONFIG_ZOOM = 2;

#define CONFIG_ZOOM_MIN 2
#define CONFIG_ZOOM_MAX 4

//------------------------------------------------------------------

static void _win_main_clear_message(void); // in this file
static void Win_MainCloseAllSubwindows(void);

//------------------------------------------------------------------

#define SCREEN_GBA 0 // 240x160
#define SCREEN_GB  1 // 160x144
#define SCREEN_SGB 2 // 256x224

static int WIN_MAIN_SCREEN_TYPE;

static int WIN_MAIN_MENU_ENABLED = 0;
static int WIN_MAIN_MENU_HAS_TO_UPDATE = 0;
static char WIN_MAIN_GAME_MENU_BUFFER[256*CONFIG_ZOOM_MAX * 224*CONFIG_ZOOM_MAX * 4];
static char WIN_MAIN_GAME_SCREEN_BUFFER[256*224*3]; // max possible size = SGB

static int _win_main_get_game_screen_texture_width(void)
{
    if(WIN_MAIN_SCREEN_TYPE == SCREEN_GB) return 160;
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_SGB) return 256;
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_GBA) return 240;
    return 0;
}

static int _win_main_get_game_screen_texture_height(void)
{
    if(WIN_MAIN_SCREEN_TYPE == SCREEN_GB) return 144;
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_SGB) return 224;
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_GBA) return 160;
    return 0;
}

static int _win_main_get_menu_texture_width(void)
{
    return 256*WIN_MAIN_CONFIG_ZOOM;
}

static int _win_main_get_menu_texture_height(void)
{
    return 224*WIN_MAIN_CONFIG_ZOOM;
}

static void _win_main_get_game_screen_texture_dump(void)
{
    memset(WIN_MAIN_GAME_MENU_BUFFER,0,sizeof(WIN_MAIN_GAME_MENU_BUFFER));

    if(WIN_MAIN_SCREEN_TYPE == SCREEN_GBA)
    {
        ScaleImage24RGB(WIN_MAIN_CONFIG_ZOOM, WIN_MAIN_GAME_SCREEN_BUFFER,240,160,
                   WIN_MAIN_GAME_MENU_BUFFER,_win_main_get_menu_texture_width(),_win_main_get_menu_texture_height());
    }
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_GB)
    {
        ScaleImage24RGB(WIN_MAIN_CONFIG_ZOOM, WIN_MAIN_GAME_SCREEN_BUFFER,160,144,
                   WIN_MAIN_GAME_MENU_BUFFER,_win_main_get_menu_texture_width(),_win_main_get_menu_texture_height());
    }
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_SGB)
    {
        ScaleImage24RGB(WIN_MAIN_CONFIG_ZOOM, WIN_MAIN_GAME_SCREEN_BUFFER,256,224,
                   WIN_MAIN_GAME_MENU_BUFFER,_win_main_get_menu_texture_width(),_win_main_get_menu_texture_height());
    }

    int w = _win_main_get_menu_texture_width();
    int h = _win_main_get_menu_texture_height();
    int i,j;
    for(j = 0; j < h; j++) for(i = 0; i < w; i++)
    {
        if( (i^j) & 2 )
        {
            WIN_MAIN_GAME_MENU_BUFFER[(j*w+i)*3+0] = 128;
            WIN_MAIN_GAME_MENU_BUFFER[(j*w+i)*3+1] = 128;
            WIN_MAIN_GAME_MENU_BUFFER[(j*w+i)*3+2] = 128;
        }
    }
}

static void _win_main_set_game_screen(int type)
{
    WIN_MAIN_SCREEN_TYPE = type;
    if(WinIDMain != -1)
        WH_SetSize(WinIDMain,256*WIN_MAIN_CONFIG_ZOOM,224*WIN_MAIN_CONFIG_ZOOM,
               _win_main_get_game_screen_texture_width(),_win_main_get_game_screen_texture_height(), WIN_MAIN_CONFIG_ZOOM);
}

static void _win_main_switch_to_game(void)
{
    if(WIN_MAIN_MENU_ENABLED == 0) return;

    if(WIN_MAIN_RUNNING == RUNNING_NONE) return;

    WIN_MAIN_MENU_ENABLED = 0;

    _win_main_set_game_screen(WIN_MAIN_SCREEN_TYPE);
}

static void _win_main_switch_to_menu(void)
{
    if(WIN_MAIN_MENU_ENABLED == 1) return;

    WIN_MAIN_MENU_ENABLED = 1;
    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;

    WH_SetSize(WinIDMain,256*WIN_MAIN_CONFIG_ZOOM,224*WIN_MAIN_CONFIG_ZOOM, 0,0, 0);

    _win_main_get_game_screen_texture_dump();
}

static int _win_main_has_to_switch_to_game = 0;
static int _win_main_has_to_switch_to_menu = 0;

static void _win_main_switch_to_game_delayed(void)
{
    _win_main_has_to_switch_to_game = 1;
}

static void _win_main_switch_to_menu_delayed(void)
{
    _win_main_has_to_switch_to_menu = 1;
}

//------------------------------------------------------------------

static int _win_main_get_rom_type(char * name)
{
    char extension[4];
    int len = strlen(name);

    extension[3] = '\0';
    extension[2] = toupper(name[len-1]);
    extension[1] = toupper(name[len-2]);
    extension[0] = toupper(name[len-3]);
    if(strcmp(extension,"GBA") == 0) return RUNNING_GBA;
    if(strcmp(extension,"AGB") == 0) return RUNNING_GBA;
    if(strcmp(extension,"BIN") == 0) return RUNNING_GBA;
    if(strcmp(extension,"GBC") == 0) return RUNNING_GB;
    if(strcmp(extension,"CGB") == 0) return RUNNING_GB;
    if(strcmp(extension,"SGB") == 0) return RUNNING_GB;

    extension[0] = extension[1];
    extension[1] = extension[2];
    extension[2] = '\0';
    if(strcmp(extension,"GB") == 0) return RUNNING_GB;

    return RUNNING_NONE;
}

static void * bios_buffer = NULL;
static void * rom_buffer = NULL;
static unsigned int rom_size;

static void _win_main_unload_rom(int save_data)
{
    _win_main_clear_message();

    if(WIN_MAIN_RUNNING == RUNNING_GBA)
    {
        GBA_EndRom(save_data);
        GBA_DebugClearBreakpointAll();
    }
    else if(WIN_MAIN_RUNNING == RUNNING_GB)
    {
        GB_End(save_data);
        GB_DebugClearBreakpointAll();
    }
    else return;

    if(bios_buffer) free(bios_buffer);
    if(rom_buffer) free(rom_buffer);
    bios_buffer = NULL;
    rom_buffer = NULL;

    memset(WIN_MAIN_GAME_SCREEN_BUFFER,0,sizeof(WIN_MAIN_GAME_SCREEN_BUFFER)); //clear screen buffer
    WH_Render(WinIDMain, WIN_MAIN_GAME_SCREEN_BUFFER); // clear screen
    _win_main_get_game_screen_texture_dump();

    WIN_MAIN_RUNNING = RUNNING_NONE;

    Sound_SetCallback(NULL);

    _win_main_switch_to_menu();

    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;

    if(EmulatorConfig.auto_close_debugger)
    {
        Win_GBDisassemblerClose();
        Win_GBMemViewerClose();
        Win_GBIOViewerClose();

        Win_GBTileViewerClose();
        Win_GBMapViewerClose();
        Win_GBSprViewerClose();
        Win_GBPalViewerClose();

        Win_GB_SGBViewerClose();

        Win_GBADisassemblerClose();
        Win_GBAMemViewerClose();
        Win_GBAIOViewerClose();

        Win_GBATileViewerClose();
        Win_GBAMapViewerClose();
        Win_GBASprViewerClose();
        Win_GBAPalViewerClose();
    }
}

static int _win_main_load_rom_autodetect(char * path)
{
    if(path == NULL) return 0;
    if(strlen(path) == 0) return 0;

    _win_main_clear_message();

    if(WIN_MAIN_RUNNING != RUNNING_NONE)
        _win_main_unload_rom(1);

    int type = _win_main_get_rom_type(path);

    if(type == RUNNING_NONE)
    {
        return 0;
    }
    else if(type == RUNNING_GB)
    {
        if(GB_ROMLoad(path))
        {
            if(GB_IsEnabledSGB()) _win_main_set_game_screen(SCREEN_SGB);
            else _win_main_set_game_screen(SCREEN_GB);
            WIN_MAIN_RUNNING = RUNNING_GB;

            Sound_SetCallback(GB_SoundCallback);

            _win_main_switch_to_game_delayed();

            return 1;
        }
        return 0;
    }
    else if(type == RUNNING_GBA)
    {
        //there should be some error checking...

        char bios_path[MAX_PATHLEN];
        unsigned int bios_size;
        s_snprintf(bios_path,sizeof(bios_path),"%s"GBA_BIOS_FILENAME,DirGetBiosFolderPath());

        FileLoad_NoError(bios_path,&bios_buffer,&bios_size); // don't show error messages...

        if(bios_size == 0) GBA_BiosLoaded(0);
        else GBA_BiosLoaded(1);

        FileLoad(path,&rom_buffer,&rom_size);
        GBA_SaveSetFilename(path);
        GBA_InitRom(bios_buffer,rom_buffer,rom_size);

        WIN_MAIN_RUNNING = RUNNING_GBA;

        Sound_SetCallback(GBA_SoundCallback);

        _win_main_set_game_screen(SCREEN_GBA);

        _win_main_switch_to_game_delayed();

        return 1;
    }

    return 0;
}

//------------------------------------------------------------------

static void _win_main_file_explorer_close(void); // down here

static _gui_element mainwindow_scrollable_text_window;

static void _win_main_scrollable_text_window_show_about(void)
{
    Win_MainCloseAllSubwindows();

    extern const char * about_text;
    GUI_SetScrollableTextWindow(&mainwindow_scrollable_text_window,25,25,
                                _win_main_get_menu_texture_width()-50,_win_main_get_menu_texture_height()-50,
                                about_text, "About");
    GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,1);
}

static void _win_main_scrollable_text_window_show_license(void)
{
    Win_MainCloseAllSubwindows();

    extern const char * license_text;
    GUI_SetScrollableTextWindow(&mainwindow_scrollable_text_window,25,25,
                                _win_main_get_menu_texture_width()-50,_win_main_get_menu_texture_height()-50,
                                license_text, "License");
    GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,1);
}

static void _win_main_scrollable_text_window_show_readme(void)
{
    Win_MainCloseAllSubwindows();

    extern const char * readme_text;
    GUI_SetScrollableTextWindow(&mainwindow_scrollable_text_window,25,25,
                                _win_main_get_menu_texture_width()-50,_win_main_get_menu_texture_height()-50,
                                readme_text, "Readme");
    GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,1);
}

static void _win_main_scrollable_text_window_close(void)
{
    GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,0);
    memset(&mainwindow_scrollable_text_window,0,sizeof(mainwindow_scrollable_text_window));
}

//------------------------------------------------------------------

//------------------------------------------------------------------

// FILE EXPLORER WINDOW

#define WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS 28

static _gui_element mainwindow_fileexplorer_win;

static _gui_console win_main_fileexpoler_con;
static _gui_element win_main_fileexpoler_textbox, win_main_fileexpoler_up_btn, win_main_fileexpoler_cancel_btn;
static _gui_console win_main_fileexpoler_path_con;
static _gui_element win_main_fileexpoler_path_textbox;

static _gui_element * mainwindow_subwindow_fileexplorer_elements[] = {
    &win_main_fileexpoler_textbox, &win_main_fileexpoler_up_btn, &win_main_fileexpoler_cancel_btn,
    &win_main_fileexpoler_path_textbox, NULL
};

static _gui mainwindow_subwindow_fileexplorer_gui = {
    mainwindow_subwindow_fileexplorer_elements, NULL, NULL
};

static int _win_main_file_explorer_windows_selection = 0;

static int _win_main_file_explorer_get_starting_drawing_index(void)
{
    int start_print_index;

    int numfiles = FileExplorer_GetNumFiles();

    if(WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS >= numfiles)
        start_print_index = 0;
    else if(_win_main_file_explorer_windows_selection < (WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS/2))
        start_print_index = 0;
    else
        start_print_index = _win_main_file_explorer_windows_selection - (WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS/2);

    return start_print_index;
}

static void _win_main_file_explorer_refresh(void)
{
    if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win) == 0) return;

    GUI_ConsoleClear(&win_main_fileexpoler_con);

    FileExplorer_LoadFolder();

    int numfiles = FileExplorer_GetNumFiles();

    int start_print_index = _win_main_file_explorer_get_starting_drawing_index();

    int i;
    for(i = 0; i < WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS; i++)
    {
        int selected_file = i + start_print_index;

        if(selected_file < numfiles)
        {
            if(selected_file == _win_main_file_explorer_windows_selection)
                GUI_ConsoleColorizeLine(&win_main_fileexpoler_con,i, 0xFFFFFF00);

            GUI_ConsoleModePrintf(&win_main_fileexpoler_con,0,i,"%s %s",
                                    FileExplorer_GetIsDir(selected_file) ? "<DIR>" : "     ",
                                    FileExplorer_GetName(selected_file));
        }
    }

    GUI_ConsoleClear(&win_main_fileexpoler_path_con);
    GUI_ConsoleModePrintf(&win_main_fileexpoler_path_con,0,0,FileExplorer_GetCurrentPath());
}

static void _win_main_file_explorer_close(void)
{
    FileExplorer_ListFree();
    GUI_WindowSetEnabled(&mainwindow_fileexplorer_win,0);
}

static void _win_main_file_explorer_up(void)
{
    FileExplorer_SelectEntry("..");
    _win_main_file_explorer_windows_selection = 0;
    _win_main_file_explorer_refresh();
}

static void _win_main_file_explorer_open_index(int i)
{
    _win_main_file_explorer_windows_selection = 0;

    //Debug_DebugMsg(FileExplorer_GetName(i));
    int is_dir = FileExplorer_SelectEntry(FileExplorer_GetName(i));

    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Debug", FileExplorer_GetName(i), NULL);

    _win_main_file_explorer_refresh();

    if(is_dir == 0)
    {
        //Debug_DebugMsg(FileExplorer_GetResultFilePath());
        _win_main_file_explorer_close();
        _win_main_load_rom_autodetect(FileExplorer_GetResultFilePath());
    }
}

static void _win_main_file_explorer_text_box_callback(int x, int y)
{
    int i = _win_main_file_explorer_get_starting_drawing_index() + (y / FONT_HEIGHT);
    _win_main_file_explorer_open_index(i);
}

static void _win_main_file_explorer_create(void)
{
    GUI_SetTextBox(&win_main_fileexpoler_textbox,&win_main_fileexpoler_con,
                   7,7,  FONT_WIDTH*64,FONT_HEIGHT*WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS,
                   _win_main_file_explorer_text_box_callback);
    GUI_SetButton(&win_main_fileexpoler_up_btn,7,353,FONT_WIDTH*4,24,
                  "Up",_win_main_file_explorer_up);
    GUI_SetButton(&win_main_fileexpoler_cancel_btn,399,353,FONT_WIDTH*8,24,
                  "Cancel",_win_main_file_explorer_close);

    GUI_SetTextBox(&win_main_fileexpoler_path_textbox,&win_main_fileexpoler_path_con,
                   7+FONT_WIDTH*4+6,353,  FONT_WIDTH*50,FONT_HEIGHT*2,
                   NULL);

    GUI_SetWindow(&mainwindow_fileexplorer_win,25,25,256*2-50,224*2-50,&mainwindow_subwindow_fileexplorer_gui,
                  "File Explorer");
}

//------------------------------------------------------------------

//------------------------------------------------------------------

static void _win_main_file_explorer_load(void)
{
    Win_MainCloseAllSubwindows();

    GUI_WindowSetEnabled(&mainwindow_fileexplorer_win,1);
    FileExplorer_SetPath(DirGetRunningPath());
    _win_main_file_explorer_refresh();
}

static void _win_main_menu_close(void)
{
    Win_MainCloseAllSubwindows();
    _win_main_unload_rom(1);
}

static void _win_main_menu_close_no_save(void)
{
    Win_MainCloseAllSubwindows();
    _win_main_unload_rom(0);
}

static void _win_main_screenshot(void)
{
    if(Win_MainRunningGBA()) GBA_Screenshot();
    if(Win_MainRunningGB()) GB_Screenshot();
}

static void _win_main_menu_exit(void)
{
    Win_MainCloseAllSubwindows();
    _win_main_unload_rom(1);
    WH_CloseAll();
}

static void _win_main_menu_toggle_pause(void)
{
    if(WIN_MAIN_MENU_ENABLED) _win_main_switch_to_game();
    else _win_main_switch_to_menu();

    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
}

static void _win_main_reset(void)
{
    if(WIN_MAIN_RUNNING != RUNNING_NONE)
    {
        if(WIN_MAIN_MENU_ENABLED)
            _win_main_switch_to_game();

        if(WIN_MAIN_RUNNING == RUNNING_GBA) GBA_Reset();
        else if(WIN_MAIN_RUNNING == RUNNING_GB) GB_HardReset();
        //if(EmulatorConfig.frameskip == -1)
        //{
            //FRAMESKIP = 0;
            //AUTO_FRAMESKIP_WAIT = 2;
        //}
    }
}

static void _win_main_show_console(void)
{
    Win_MainCloseAllSubwindows();
    ConsoleShow();
}

static void _win_main_menu_toggle_mute_sound(void)
{
    Sound_SetEnabled(!Sound_GetEnabled());
}

static void _win_main_menu_open_configuration_window(void)
{
    Win_MainCloseAllSubwindows();

    Win_MainOpenConfigWindow();
}

static void _win_main_menu_open_configure_input_window(void)
{
    Win_MainCloseAllSubwindows();

    Win_MainOpenConfigInputWindow();
}

static void _win_main_menu_open_disassembler(void)
{
    Win_GBADisassemblerCreate();
    Win_GBDisassemblerCreate();
}

static void _win_main_menu_open_mem_viewer(void)
{
    Win_GBAMemViewerCreate();
    Win_GBMemViewerCreate();
}

static void _win_main_menu_open_io_viewer(void)
{
    Win_GBIOViewerCreate();
    Win_GBAIOViewerCreate();
}

static void _win_main_menu_open_tile_viewer(void)
{
    Win_GBATileViewerCreate();
    Win_GBTileViewerCreate();
}

static void _win_main_menu_open_map_viewer(void)
{
    Win_GBAMapViewerCreate();
    Win_GBMapViewerCreate();
}

static void _win_main_menu_open_spr_viewer(void)
{
    Win_GBASprViewerCreate();
    Win_GBSprViewerCreate();
}

static void _win_main_menu_open_pal_viewer(void)
{
    Win_GBAPalViewerCreate();
    Win_GBPalViewerCreate();
}

static void _win_main_menu_open_sgb_viewer(void)
{
    Win_GB_SGBViewerCreate();
}

static void _win_main_menu_open_gbcamera_viewer(void)
{
    Win_GB_GBCameraViewerCreate();
}


//------------------------------------------------------------------

//Window Menu

static _gui_menu_entry mm_separator = {" " , NULL, 1};

static _gui_menu_entry mmfile_open = {"Load (CTRL+L)" , _win_main_file_explorer_load, 1};
static _gui_menu_entry mmfile_close = {"Close (CTRL+C)" , _win_main_menu_close, 1};
static _gui_menu_entry mmfile_closenosav = {"Close without saving", _win_main_menu_close_no_save, 1};
static _gui_menu_entry mmfile_reset = {"Reset (CTRL+R)", _win_main_reset, 1};
static _gui_menu_entry mmfile_pause = {"Pause (CTRL+P)", _win_main_menu_toggle_pause, 1};
static _gui_menu_entry mmfile_rominfo = {"Show Console (Rom Info.)", _win_main_show_console, 1};
static _gui_menu_entry mmfile_screenshot = {"Screenshot (F12)", _win_main_screenshot, 1};
static _gui_menu_entry mmfile_exit = {"Exit (CTRL+E)", _win_main_menu_exit, 1};

static _gui_menu_entry * mmfile_elements[] = {
    &mmfile_open, &mmfile_close, &mmfile_closenosav, &mm_separator, &mmfile_reset, &mmfile_pause,
    &mm_separator, &mmfile_rominfo, &mmfile_screenshot, &mm_separator, &mmfile_exit, NULL
};

static _gui_menu_list main_menu_file = {
    "File", mmfile_elements,
};

static _gui_menu_entry mmoptions_configuration = {"Configuration" , _win_main_menu_open_configuration_window, 1};
static _gui_menu_entry mmoptions_configure_joysticks = {"Input Configuration" , _win_main_menu_open_configure_input_window, 1};
static _gui_menu_entry mmoptions_mutesound = {"Mute Sound (CTRL+M)" , _win_main_menu_toggle_mute_sound, 1};
static _gui_menu_entry mmoptions_sysinfo = {"System Information" , SysInfoShow, 1};

static _gui_menu_entry * mmoptions_elements[] = {
    &mmoptions_configuration, &mmoptions_configure_joysticks,
    &mm_separator, &mmoptions_mutesound, &mm_separator, &mmoptions_sysinfo, NULL
};

static _gui_menu_list main_menu_options = {
    "Options", mmoptions_elements,
};

static _gui_menu_entry mmdebug_disas = {"Disassembler (F5)" , _win_main_menu_open_disassembler, 1};
static _gui_menu_entry mmdebug_memview = {"Memory Viewer (F6)" , _win_main_menu_open_mem_viewer, 1};
static _gui_menu_entry mmdebug_ioview = {"I/O Viewer (F7)" , _win_main_menu_open_io_viewer, 1};

static _gui_menu_entry mmdebug_tileview = {"Tile Viewer" , _win_main_menu_open_tile_viewer, 1};
static _gui_menu_entry mmdebug_mapview = {"Map Viewer" , _win_main_menu_open_map_viewer, 1};
static _gui_menu_entry mmdebug_sprview = {"Sprite Viewer" , _win_main_menu_open_spr_viewer, 1};
static _gui_menu_entry mmdebug_palview = {"Palette Viewer" , _win_main_menu_open_pal_viewer, 1};

static _gui_menu_entry mmdebug_sgbview = {"SGB Viewer" , _win_main_menu_open_sgb_viewer, 1};
static _gui_menu_entry mmdebug_gbcameraview = {"GB Camera Viewer" , _win_main_menu_open_gbcamera_viewer, 1};

static _gui_menu_entry * mmdisas_elements[] = {
    &mmdebug_disas, &mmdebug_memview, &mmdebug_ioview, &mm_separator, &mmdebug_tileview, &mmdebug_mapview,
    &mmdebug_sprview, &mmdebug_palview, &mm_separator, &mmdebug_sgbview, &mmdebug_gbcameraview, NULL
};

static _gui_menu_list main_menu_debug = {
    "Debug", mmdisas_elements,
};

static _gui_menu_entry mmhelp_readme = {"Readme (F1)" , _win_main_scrollable_text_window_show_readme, 1};
static _gui_menu_entry mmhelp_license = {"License" , _win_main_scrollable_text_window_show_license, 1};
static _gui_menu_entry mmhelp_about = {"About" , _win_main_scrollable_text_window_show_about, 1};

static _gui_menu_entry * mmhelp_elements[] = {
    &mmhelp_readme, &mmhelp_license, &mm_separator, &mmhelp_about, NULL
};

static _gui_menu_list main_menu_help = {
    "Help", mmhelp_elements,
};

static _gui_menu_list * mmenu_lists[] = {
    &main_menu_file, &main_menu_options, &main_menu_debug, &main_menu_help, NULL
};

static _gui_menu main_menu = {
    -1, mmenu_lists
};

//------------------------------------------------------------------

static void _win_main_menu_update_elements_enabled(void)
{
    if(WIN_MAIN_RUNNING == RUNNING_NONE)
    {
        mmfile_close.enabled = 0;
        mmfile_closenosav.enabled = 0;
        mmfile_reset.enabled = 0;
        mmfile_pause.enabled = 0;
        mmfile_screenshot.enabled = 0;

        mmoptions_mutesound.enabled = 0;

        mmdebug_disas.enabled = 0;
        mmdebug_memview.enabled = 0;
        mmdebug_ioview.enabled = 0;

        mmdebug_tileview.enabled = 0;
        mmdebug_mapview.enabled = 0;
        mmdebug_sprview.enabled = 0;
        mmdebug_palview.enabled = 0;

        mmdebug_sgbview.enabled = 0;
        mmdebug_gbcameraview.enabled = 0;
    }
    else
    {
        mmfile_close.enabled = 1;
        mmfile_closenosav.enabled = 1;
        mmfile_reset.enabled = 1;
        mmfile_pause.enabled = 1;
        mmfile_screenshot.enabled = 1;

        mmoptions_mutesound.enabled = 1;

        mmdebug_disas.enabled = 1;
        mmdebug_memview.enabled = 1;
        mmdebug_ioview.enabled = 1;

        mmdebug_tileview.enabled = 1;
        mmdebug_mapview.enabled = 1;
        mmdebug_sprview.enabled = 1;
        mmdebug_palview.enabled = 1;

        mmdebug_sgbview.enabled = 0;
        mmdebug_gbcameraview.enabled = 0;

        if(WIN_MAIN_RUNNING == RUNNING_GB)
        {
            if(GB_EmulatorIsEnabledSGB())
                mmdebug_sgbview.enabled = 1;

            if(GB_MapperIsGBCamera())
                mmdebug_gbcameraview.enabled = 1;
        }
        else if(WIN_MAIN_RUNNING == RUNNING_GBA)
        {
            // other debugger windows for gba
        }
    }
}

//------------------------------------------------------------------

//------------------------------------------------------------------

//MAIN WINDOW GUI

static _gui_console mainwindow_show_message_con;
static _gui_element mainwindow_show_message_win;

static _gui_element mainwindow_bg;

static _gui_element * mainwindow_gui_elements[] = {
    &mainwindow_bg,
    &mainwindow_configwin, // in win_main_config.c
    &mainwindow_config_input_win, // in win_main_config_input.c
    &mainwindow_fileexplorer_win,
    &mainwindow_scrollable_text_window,
    &mainwindow_show_message_win, // least priority = always drawn
    NULL
};

static _gui mainwindow_window_gui = {
    mainwindow_gui_elements,
    NULL,
    &main_menu
};

//------------------------------------------------------------------

//@global function
void Win_MainShowMessage(int type, const char * text) // 0 = error, 1 = debug, 2 = console, 3 = sys info
{
    _win_main_switch_to_menu();

    if(type == 0)
    {
        GUI_SetMessageBox(&mainwindow_show_message_win,&mainwindow_show_message_con,
                      100,100,256*2-200,224*2-200,"Error Message");
        GUI_MessageBoxSetEnabled(&mainwindow_show_message_win,1);
        GUI_ConsoleModePrintf(&mainwindow_show_message_con,0,0,text);
    }
    else if(type == 1)
    {
        GUI_SetMessageBox(&mainwindow_show_message_win,&mainwindow_show_message_con,
                      100,100,256*2-200,224*2-200,"Debug Message");
        GUI_MessageBoxSetEnabled(&mainwindow_show_message_win,1);
        GUI_ConsoleModePrintf(&mainwindow_show_message_con,0,0,text);
    }
    else if(type == 2)
    {
        Win_MainCloseAllSubwindows();

    //    GUI_SetMessageBox(&mainwindow_show_message_win,&mainwindow_show_message_con,
    //                  50,50,256*2-100,224*2-100,"Console");
        GUI_SetScrollableTextWindow(&mainwindow_scrollable_text_window,25,25,
                                _win_main_get_menu_texture_width()-50,_win_main_get_menu_texture_height()-50,
                                text, "Console");
        GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,1);
    }
    else if(type == 3)
    {
        Win_MainCloseAllSubwindows();

        GUI_SetScrollableTextWindow(&mainwindow_scrollable_text_window,25,25,
                                _win_main_get_menu_texture_width()-50,_win_main_get_menu_texture_height()-50,
                                text, "System Information");
        GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,1);
    }
    else return;
}

static void _win_main_clear_message(void)
{
    memset(&mainwindow_show_message_win,0,sizeof(mainwindow_show_message_win));
}

//------------------------------------------------------------------

int _win_main_background_image_callback(int x, int y)
{
    _win_main_switch_to_game_delayed();
    return 1;
}

//------------------------------------------------------------------

static void Win_MainCloseAllSubwindows(void)
{
    _win_main_clear_message();
    _win_main_file_explorer_close();
    _win_main_scrollable_text_window_close();
    Win_MainCloseConfigWindow();
    Win_MainCloseConfigInputWindow();
}

//------------------------------------------------------------------

void Win_MainChangeZoom(int newzoom)
{
    if( (newzoom < 2) && (newzoom > 4) )
    {
        Debug_LogMsgArg("Win_MainChangeZoom(): Unsupported zoom level: %d",newzoom);
        return;
    }

    WIN_MAIN_CONFIG_ZOOM = newzoom;
    EmulatorConfig.screen_size = WIN_MAIN_CONFIG_ZOOM;

    if(WIN_MAIN_MENU_ENABLED)
    {
        WIN_MAIN_MENU_HAS_TO_UPDATE = 1;

        if(WinIDMain != -1)
            WH_SetSize(WinIDMain,256*WIN_MAIN_CONFIG_ZOOM,224*WIN_MAIN_CONFIG_ZOOM, 0,0, 0);
    }
    else
    {
        _win_main_set_game_screen(WIN_MAIN_SCREEN_TYPE);
    }

    //change captured image

    _win_main_get_game_screen_texture_dump();

    GUI_SetBitmap(&mainwindow_bg,0,0,
                  _win_main_get_menu_texture_width(),_win_main_get_menu_texture_height(),WIN_MAIN_GAME_MENU_BUFFER,
                  _win_main_background_image_callback);
}

//------------------------------------------------------------------

static int Win_MainEventCallback(SDL_Event * e)
{
    if(WIN_MAIN_MENU_ENABLED)
    {
        _win_main_menu_update_elements_enabled();
        WIN_MAIN_MENU_HAS_TO_UPDATE |= GUI_SendEvent(&mainwindow_window_gui,e);
    }

    int close_this = 0;

    if( e->type == SDL_KEYDOWN )
    {
        switch( e->key.keysym.sym )
        {
            case SDLK_F1:
                _win_main_scrollable_text_window_show_readme();
                WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
                break;
            case SDLK_F5:
                _win_main_menu_open_disassembler();
                break;
            case SDLK_F6:
                _win_main_menu_open_mem_viewer();
                break;
            case SDLK_F7:
                _win_main_menu_open_io_viewer();
                break;
            case SDLK_F12:
                _win_main_screenshot();
                break;

            case SDLK_UP:
                if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win))
                {
                    if(_win_main_file_explorer_windows_selection > 0)
                        _win_main_file_explorer_windows_selection --;
                    _win_main_file_explorer_refresh();
                    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
                }
                break;
            case SDLK_DOWN:
                if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win))
                {
                    if(_win_main_file_explorer_windows_selection < (FileExplorer_GetNumFiles()-1))
                        _win_main_file_explorer_windows_selection ++;
                    _win_main_file_explorer_refresh();
                    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
                }
                break;
            case SDLK_RETURN:
                if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win))
                {
                    _win_main_file_explorer_open_index(_win_main_file_explorer_windows_selection);
                    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
                }
                break;
            case SDLK_BACKSPACE:
                if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win))
                {
                    _win_main_file_explorer_up();
                    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
                }
                break;
            case SDLK_p:
                if(SDL_GetModState()&KMOD_CTRL) _win_main_menu_toggle_pause();
                break;
            case SDLK_m:
                if(SDL_GetModState()&KMOD_CTRL) _win_main_menu_toggle_mute_sound();
                break;
            case SDLK_l:
                if(SDL_GetModState()&KMOD_CTRL)
                {
                    if(WIN_MAIN_MENU_ENABLED == 0)
                        _win_main_switch_to_menu();

                    _win_main_file_explorer_load();

                    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
                }
                break;
            case SDLK_c:
                if(SDL_GetModState()&KMOD_CTRL) _win_main_menu_close();
                break;
            case SDLK_e:
                if(SDL_GetModState()&KMOD_CTRL) _win_main_menu_exit();
                break;
            case SDLK_r:
                if(SDL_GetModState()&KMOD_CTRL) _win_main_reset();
                break;

            case SDLK_ESCAPE:
                WH_CloseAll();
                return 1;
        }
    }
    else if(e->type == SDL_MOUSEWHEEL)
    {
        _win_main_file_explorer_windows_selection -= e->wheel.y*3;

        if(_win_main_file_explorer_windows_selection < 0)
            _win_main_file_explorer_windows_selection = 0;
        if(_win_main_file_explorer_windows_selection > (FileExplorer_GetNumFiles()-1) )
            _win_main_file_explorer_windows_selection = FileExplorer_GetNumFiles() - 1;

        WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
        _win_main_file_explorer_refresh();
    }
    else if(e->type == SDL_MOUSEBUTTONDOWN)
    {
        if(WIN_MAIN_MENU_ENABLED == 0)
        {
            _win_main_switch_to_menu_delayed();
        }
        WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
   }
    else if(e->type == SDL_MOUSEBUTTONUP)
    {
        WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
    }
    else if(e->type == SDL_WINDOWEVENT)
    {
        if(e->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            _win_main_switch_to_menu();
            WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
        }
        if(e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_EXPOSED)
        {
            WIN_MAIN_MENU_HAS_TO_UPDATE = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_CLOSE)
        {
            close_this = 1;
        }
    }
    else if(e->type == SDL_DROPFILE)
    {
        _win_main_load_rom_autodetect(e->drop.file);
        //Debug_LogMsgArg("Drag file: %s",e->drop.file);
        SDL_free(e->drop.file);
    }
    if(_win_main_has_to_switch_to_game)
    {
        _win_main_has_to_switch_to_game = 0;
        _win_main_has_to_switch_to_menu = 0;
        _win_main_switch_to_game();
    }
    else if(_win_main_has_to_switch_to_menu)
    {
        _win_main_has_to_switch_to_game = 0;
        _win_main_has_to_switch_to_menu = 0;
        _win_main_switch_to_menu();
    }

    if(close_this)
    {
        _win_main_unload_rom(1); // save things
        WH_CloseAll();
        //Debug_LogMsgArg("callback: closing win 0");
        return 1;
    }

    return 0;
}

//@global function
int Win_MainCreate(char * rom_path)
{
    _win_main_file_explorer_create();

    Win_MainCreateConfigWindow(); // Only configure positions of elements, this won't open them
    Win_MainCreateConfigInputWindow();

    Win_MainCloseAllSubwindows();
    //_win_main_scrollable_text_window_close();
    //_win_main_clear_message();

    Win_MainChangeZoom(EmulatorConfig.screen_size);
    Win_MainSetFrameskip(EmulatorConfig.frameskip);

    WinIDMain = WH_Create(256*WIN_MAIN_CONFIG_ZOOM,224*WIN_MAIN_CONFIG_ZOOM, 0,0, 0);
    if( WinIDMain == -1 )
    {
        Debug_LogMsgArg( "Win_MainCreate(): Window could not be created!" );
        return 1;
    }

    WIN_MAIN_MENU_ENABLED = 1;
    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;

    WH_SetCaption(WinIDMain,"GiiBiiAdvance");

    WH_SetEventCallback(WinIDMain,Win_MainEventCallback);
    WH_SetEventMainWindow(WinIDMain);

    FPS_TimerInit();
    atexit(FPS_TimerEnd);

    WIN_MAIN_RUNNING = RUNNING_NONE;

    //if inited the emulator with a game...
    if(rom_path)
    {
        if(strlen(rom_path) > 0)
        {
            _win_main_load_rom_autodetect(rom_path);
        }
    }

    return 0;
}

static char _win_main_buffer_when_menu[256*4*224*4*3];

//@global function
void Win_MainRender(void)
{
    if(WIN_MAIN_MENU_ENABLED == 0)
    {
        if(WIN_MAIN_RUNNING != RUNNING_NONE)
        {
            WH_Render(WinIDMain, WIN_MAIN_GAME_SCREEN_BUFFER);
        }
    }
    else
    {
        if(WIN_MAIN_MENU_HAS_TO_UPDATE)
        {
            WIN_MAIN_MENU_HAS_TO_UPDATE = 0;
            GUI_Draw(&mainwindow_window_gui,_win_main_buffer_when_menu,
                     _win_main_get_menu_texture_width(),_win_main_get_menu_texture_height(),1);
            WH_Render( WinIDMain, _win_main_buffer_when_menu );
        }
    }
}


//@global function
int Win_MainRunningGBA(void)
{
    return (WIN_MAIN_RUNNING == RUNNING_GBA);
}

//@global function
int Win_MainRunningGB(void)
{
    return (WIN_MAIN_RUNNING == RUNNING_GB);
}

//@global function
void Win_MainLoopHandle(void)
{
    int speedup = Input_Speedup_Enabled();

    if(speedup)
        Win_MainSetFrameskip(10);
    else
        Win_MainSetFrameskip(EmulatorConfig.frameskip);

    if(WH_HasKeyboardFocus(WinIDMain) && (WIN_MAIN_MENU_ENABLED == 0))
    {
        //if( GUI_WindowGetEnabled(&mainwindow_configwin) || GUI_WindowGetEnabled(&mainwindow_fileexplorer_win) ||
        //    GUI_ScrollableTextWindowGetEnabled(&mainwindow_scrollable_text_window) ||
        //    GUI_MessageBoxGetEnabled(&mainwindow_show_message_win) )
        if( GUI_MessageBoxGetEnabled(&mainwindow_show_message_win) )
        {
            _win_main_switch_to_menu();
            Sound_Disable();
            return;
        }

        Sound_Enable();

        if(WIN_MAIN_RUNNING == RUNNING_GBA)
        {
            if(GBA_ShowConsoleRequested())
            {
                ConsoleShow();
                return;
            }

            Win_GBADisassemblerStartAddressSetDefault();

            if(speedup) GBA_SoundResetBufferPointers();

            GBA_SkipFrame(_win_main_has_to_frameskip());

            Input_Update_GBA();
            GBA_RunForOneFrame();

            if(_win_main_has_to_frameskip() == 0)
                GBA_ConvertScreenBufferTo24RGB(WIN_MAIN_GAME_SCREEN_BUFFER);

            _win_main_update_frameskip();

            WinMain_frames_drawn ++;
        }
        else if(WIN_MAIN_RUNNING == RUNNING_GB)
        {
            if(GB_ShowConsoleRequested())
            {
                ConsoleShow();
                return;
            }

            if(speedup) GB_SoundResetBufferPointers();

            GB_SkipFrame(_win_main_has_to_frameskip());

            Input_Update_GB();

            if(GB_RumbleEnabled())
                Input_RumbleEnable();

            GB_RunForOneFrame();
            if(_win_main_has_to_frameskip() == 0)
                GB_Screen_WriteBuffer_24RGB(WIN_MAIN_GAME_SCREEN_BUFFER);

            _win_main_update_frameskip();

            WinMain_frames_drawn ++;
        }
    }
    else
    {
        Sound_Disable();
    }
}

