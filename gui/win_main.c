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

#include <SDL2/SDL.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "win_main.h"
#include "win_utils.h"

#include "../config.h"
#include "../file_explorer.h"
#include "../sound_utils.h"
#include "../debug_utils.h"
#include "../window_handler.h"
#include "../font_utils.h"
#include "../build_options.h"
#include "../general_utils.h"
#include "../file_utils.h"

#include "../gb_core/sound.h"
#include "../gb_core/debug.h"
#include "../gb_core/gb_main.h"
#include "../gb_core/interrupts.h"
#include "../gb_core/video.h"

#include "../gba_core/gba.h"
#include "../gba_core/bios.h"
#include "../gba_core/disassembler.h"
#include "../gba_core/save.h"
#include "../gba_core/video.h"
#include "../gba_core/rom.h"
#include "../gba_core/sound.h"

#include "win_gba_disassembler.h"
#include "win_gb_disassembler.h"
#include "win_gba_memviewer.h"
#include "win_gb_memviewer.h"
#include "win_gb_ioviewer.h"

//------------------------------------------------------------------

static int WinIDMain;

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
        GB_HandleTime();

    WinMain_FPS = WinMain_frames_drawn;
    WinMain_frames_drawn = 0;
    char caption[60];
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

static void _win_main_clear_message(void);

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
    if(WIN_MAIN_SCREEN_TYPE == SCREEN_GB)
        return 160;
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_SGB)
        return 256;
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_GBA)
        return 240;
    return 0;
}

static int _win_main_get_game_screen_texture_height(void)
{
    if(WIN_MAIN_SCREEN_TYPE == SCREEN_GB)
        return 144;
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_SGB)
        return 224;
    else if(WIN_MAIN_SCREEN_TYPE == SCREEN_GBA)
        return 160;
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
#if 0
            if(EmulatorConfig.frameskip == -1)
            {
                FRAMESKIP = 0;
                AUTO_FRAMESKIP_WAIT = 2;
            }

            GLWindow_MenuEnableROMCommands(1);
            GLWindow_ClearPause();
#endif
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
#if 0
        if(EmulatorConfig.frameskip == -1)
        {
            FRAMESKIP = 0;
            AUTO_FRAMESKIP_WAIT = 2;
        }

        GLWindow_MenuEnableROMCommands(1);
        GLWindow_ClearPause();
#endif
        _win_main_switch_to_game_delayed();

        return 1;
    }

    return 0;
}

//------------------------------------------------------------------

//------------------------------------------------------------------

// FILE EXPLORER WINDOW

#define WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS 28

static _gui_element mainwindow_fileexplorer_win;

static _gui_console win_main_fileexpoler_con;
static _gui_element win_main_fileexpoler_textbox, win_main_fileexpoler_up_btn, win_main_fileexpoler_cancel_btn;

static _gui_element * mainwindow_subwindow_fileexplorer_elements[] = {
    &win_main_fileexpoler_textbox,
    &win_main_fileexpoler_up_btn,
    &win_main_fileexpoler_cancel_btn,
    NULL
};

static _gui mainwindow_subwindow_fileexplorer_gui = {
    mainwindow_subwindow_fileexplorer_elements,
    NULL,
    NULL
};

static int _win_main_file_explorer_windows_selection = 0;

static void _win_main_file_explorer_refresh(void)
{
    if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win) == 0) return;

    GUI_ConsoleClear(&win_main_fileexpoler_con);

    FileExplorer_LoadFolder();

    int numfiles = FileExplorer_GetNumFiles();

    int start_print_index;

    if(WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS >= numfiles)
        start_print_index = 0;
    else if(_win_main_file_explorer_windows_selection < (WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS/2))
        start_print_index = 0;
    else
        start_print_index = _win_main_file_explorer_windows_selection - (WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS/2);

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
    int i = y / FONT_12_HEIGHT;
    _win_main_file_explorer_open_index(i);
}

static void _win_main_file_explorer_open(void)
{
    GUI_WindowSetEnabled(&mainwindow_fileexplorer_win,1);
    FileExplorer_SetPath(DirGetRunningPath());
    _win_main_file_explorer_refresh();
}

static void _win_main_file_explorer_create(void)
{
    GUI_SetTextBox(&win_main_fileexpoler_textbox,&win_main_fileexpoler_con,
                   7,7,
                   FONT_12_WIDTH*64,FONT_12_HEIGHT*WIN_MAIN_FILE_EXPLORER_NUM_ELEMENTS,
                   _win_main_file_explorer_text_box_callback);
    GUI_SetButton(&win_main_fileexpoler_up_btn,7,353,FONT_12_WIDTH*4,24,
                  "Up",_win_main_file_explorer_up);
    GUI_SetButton(&win_main_fileexpoler_cancel_btn,399,353,FONT_12_WIDTH*8,24,
                  "Cancel",_win_main_file_explorer_close);
}

//------------------------------------------------------------------

//------------------------------------------------------------------

static void _win_main_menu_close(void)
{
    _win_main_file_explorer_close();
    _win_main_unload_rom(1);
}

static void _win_main_menu_close_no_save(void)
{
    _win_main_file_explorer_close();
    _win_main_unload_rom(0);
}

static void _win_main_menu_exit(void)
{
    _win_main_file_explorer_close();
    _win_main_unload_rom(1);
    WH_CloseAll();
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
}

//------------------------------------------------------------------

static _gui_element mainwindow_scrollable_text_window;

static void _win_main_scrollable_text_window_show_about(void)
{
    extern const char * about_text;
    GUI_SetScrollableTextWindow(&mainwindow_scrollable_text_window,25,25,
                                _win_main_get_menu_texture_width()-50,_win_main_get_menu_texture_height()-50,
                                about_text, "About");
    GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,1);
}

static void _win_main_scrollable_text_window_show_license(void)
{
    extern const char * license_text;
    GUI_SetScrollableTextWindow(&mainwindow_scrollable_text_window,25,25,
                                _win_main_get_menu_texture_width()-50,_win_main_get_menu_texture_height()-50,
                                license_text, "License");
    GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,1);
}

static void _win_main_scrollable_text_window_show_readme(void)
{
    extern const char * readme_text;
    GUI_SetScrollableTextWindow(&mainwindow_scrollable_text_window,25,25,
                                _win_main_get_menu_texture_width()-50,_win_main_get_menu_texture_height()-50,
                                readme_text, "Readme");
    GUI_ScrollableTextWindowSetEnabled(&mainwindow_scrollable_text_window,1);
}


static void _win_main_scrollable_text_window_close(void)
{
    memset(&mainwindow_scrollable_text_window,0,sizeof(mainwindow_scrollable_text_window));
}


//------------------------------------------------------------------

static _gui_menu_entry mm_separator = {" " , NULL};

//Window Menu

static _gui_menu_entry mmfile_open = {"Open" , _win_main_file_explorer_open};
static _gui_menu_entry mmfile_close = {"Close" , _win_main_menu_close};
static _gui_menu_entry mmfile_closenosav = {"Close without saving", _win_main_menu_close_no_save};
static _gui_menu_entry mmfile_exit = {"Exit", _win_main_menu_exit};


static _gui_menu_entry * mmfile_elements[] = {
    &mmfile_open,
    &mm_separator,
    &mmfile_close,
    &mmfile_closenosav,
    &mm_separator,
    &mmfile_exit,
    NULL
};

static _gui_menu_list main_menu_file = {
    "File",
    mmfile_elements,
};

static _gui_menu_entry mmoptions_configuration = {"Configuration" , NULL};

static _gui_menu_entry * mmoptions_elements[] = {
    &mmoptions_configuration,
    &mm_separator,
    NULL
};

static _gui_menu_list main_menu_options = {
    "Options",
    mmoptions_elements,
};

static _gui_menu_entry mmdebug_disas = {"Disassembler (F5)" , _win_main_menu_open_disassembler};
static _gui_menu_entry mmdebug_memview = {"Memory Viewer (F6)" , _win_main_menu_open_mem_viewer};
static _gui_menu_entry mmdebug_ioview = {"I/O Viewer (F7)" , _win_main_menu_open_io_viewer};

static _gui_menu_entry * mmdisas_elements[] = {
    &mmdebug_disas,
    &mmdebug_memview,
    &mmdebug_ioview,
    &mm_separator,
    NULL
};

static _gui_menu_list main_menu_debug = {
    "Debug",
    mmdisas_elements,
};

static _gui_menu_entry mmhelp_readme = {"Readme" , _win_main_scrollable_text_window_show_readme};
static _gui_menu_entry mmhelp_license = {"License" , _win_main_scrollable_text_window_show_license};
static _gui_menu_entry mmhelp_about = {"About" , _win_main_scrollable_text_window_show_about};

static _gui_menu_entry * mmhelp_elements[] = {
    &mmhelp_readme,
    &mmhelp_license,
    &mm_separator,
    &mmhelp_about,
    NULL
};

static _gui_menu_list main_menu_help = {
    "Help",
    mmhelp_elements,
};

static _gui_menu_list * mmenu_lists[] = {
    &main_menu_file,
    &main_menu_options,
    &main_menu_debug,
    &main_menu_help,
    NULL
};

static _gui_menu main_menu = {
    -1,
    mmenu_lists
};

//------------------------------------------------------------------

// CONFIGURATION WINDOW

static _gui_element mainwindow_subwindow_btn, mainwindow_subwindow_btn2;

static _gui_element * mainwindow_subwindow_config_gui_elements[] = {
    &mainwindow_subwindow_btn,
    &mainwindow_subwindow_btn2,
    NULL
};

static _gui mainwindow_subwindow_config_gui = {
    mainwindow_subwindow_config_gui_elements,
    NULL,
    NULL
};

//------------------------------------------------------------------

//MAIN WINDOW GUI

static _gui_console mainwindow_show_message_con;
static _gui_element mainwindow_show_message_win;

static _gui_element mainwindow_bg;
static _gui_element mainwindow_btn;
static _gui_element mainwindow_configwin;

static _gui_element * mainwindow_gui_elements[] = {
    &mainwindow_bg,
    &mainwindow_show_message_win,
    &mainwindow_configwin,
    &mainwindow_fileexplorer_win,
    &mainwindow_scrollable_text_window,
    NULL
};

static _gui mainwindow_window_gui = {
    mainwindow_gui_elements,
    NULL,
    &main_menu
};

void mainbtncallback_openwin(void)
{
    GUI_WindowSetEnabled(&mainwindow_configwin,1);
}

//------------------------------------------------------------------

//@global function
void Win_MainShowMessage(int type, const char * text) // 0 = error, 1 = debug, 2 = console
{
    _win_main_switch_to_menu();

    if(type == 0)
        GUI_SetMessageBox(&mainwindow_show_message_win,&mainwindow_show_message_con,
                      100,100,256*2-200,224*2-200,"Error Message");
    else if(type == 1)
        GUI_SetMessageBox(&mainwindow_show_message_win,&mainwindow_show_message_con,
                      100,100,256*2-200,224*2-200,"Debug Message");
    else if(type == 2)
        GUI_SetMessageBox(&mainwindow_show_message_win,&mainwindow_show_message_con,
                      50,50,256*2-100,224*2-100,"Console");
    else return;


    GUI_MessageBoxSetEnabled(&mainwindow_show_message_win,1);

    GUI_ConsoleModePrintf(&mainwindow_show_message_con,0,0,text);
}

static void _win_main_clear_message(void)
{
    memset(&mainwindow_show_message_win,0,sizeof(mainwindow_show_message_win));
}

//------------------------------------------------------------------

static void _win_main_change_zoom(int newzoom)
{
    if( (newzoom < 2) && (newzoom > 4) )
    {
        Debug_LogMsgArg("_win_main_switch_to_menu(): Unsupported zoom level: %d",newzoom);
        return;
    }

    WIN_MAIN_CONFIG_ZOOM = newzoom;
    EmulatorConfig.screen_size = WIN_MAIN_CONFIG_ZOOM;

    if(WIN_MAIN_MENU_ENABLED)
    {
        WIN_MAIN_MENU_HAS_TO_UPDATE = 1;

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
                  _win_main_switch_to_game_delayed);
}

//------------------------------------------------------------------

void _win_main_config_window_close(void)
{
    GUI_WindowSetEnabled(&mainwindow_configwin,0);
}

//------------------------------------------------------------------

static int Win_MainEventCallback(SDL_Event * e)
{
    if(WIN_MAIN_MENU_ENABLED)
        WIN_MAIN_MENU_HAS_TO_UPDATE |= GUI_SendEvent(&mainwindow_window_gui,e);

    int close_this = 0;

    if( e->type == SDL_KEYDOWN )
    {
        switch( e->key.keysym.sym )
        {
            case SDLK_F5:
                _win_main_menu_open_disassembler();
                break;
            case SDLK_F6:
                _win_main_menu_open_mem_viewer();
                break;
            case SDLK_F7:
                _win_main_menu_open_io_viewer();
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

            //case SDLK_p:
            //    if(SDL_GetModState()|KMOD_CTRL)
            //    {
            //        if(MENU_ENABLED)
            //            _win_main_switch_to_game();
            //        else
            //            _win_main_switch_to_menu();
            //    }
            //    break;

            case SDLK_1:
                _win_main_change_zoom(2);
                return 1;
            case SDLK_2:
                _win_main_change_zoom(3);
                return 1;
            case SDLK_3:
                _win_main_change_zoom(4);
                return 1;

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
    _win_main_change_zoom(EmulatorConfig.screen_size);

    GUI_SetButton(&mainwindow_subwindow_btn,10,50,7*FONT_12_WIDTH,FONT_12_HEIGHT,"CLOSE",_win_main_config_window_close);
    GUI_SetButton(&mainwindow_subwindow_btn2,10,70,7*FONT_12_WIDTH,FONT_12_HEIGHT,"TEST",NULL);

    GUI_SetButton(&mainwindow_btn,0,0,30*FONT_12_WIDTH,15*FONT_12_HEIGHT,"MAIN GUI",mainbtncallback_openwin);
    GUI_SetWindow(&mainwindow_configwin,100,50,100,200,&mainwindow_subwindow_config_gui,"TEST");

    _win_main_file_explorer_create();
    GUI_SetWindow(&mainwindow_fileexplorer_win,25,25,256*2-50,224*2-50,&mainwindow_subwindow_fileexplorer_gui,
                  "File Explorer");

    _win_main_scrollable_text_window_close();
    _win_main_clear_message();

    WinIDMain = WH_Create(256*WIN_MAIN_CONFIG_ZOOM,224*WIN_MAIN_CONFIG_ZOOM, 0,0, 0);

    WIN_MAIN_MENU_ENABLED = 1;
    WIN_MAIN_MENU_HAS_TO_UPDATE = 1;

    WH_SetCaption(WinIDMain,"GiiBiiAdvance");

    if( WinIDMain == -1 )
    {
        Debug_LogMsgArg( "Win_MainCreate(): Window could not be created!" );
        return 1;
    }

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
            WinMain_frames_drawn ++;
            //FU_Print12(GAME_SCREEN_BUFFER,_win_main_get_game_screen_texture_width(),_win_main_get_game_screen_texture_height(),0,0,
            //           "FPS: %d",FPS);
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

static void GBA_KeyboardGamepadGetInput(void)
{
    const Uint8 * state = SDL_GetKeyboardState(NULL);

    int a = state[SDL_SCANCODE_X];
    int b = state[SDL_SCANCODE_Z];
    int l = state[SDL_SCANCODE_A];
    int r = state[SDL_SCANCODE_S];
    int st = state[SDL_SCANCODE_RETURN];
    int se = state[SDL_SCANCODE_RSHIFT];
    int dr = state[SDL_SCANCODE_RIGHT];
    int dl = state[SDL_SCANCODE_LEFT];
    int du = state[SDL_SCANCODE_UP];
    int dd = state[SDL_SCANCODE_DOWN];

    GBA_HandleInput(a, b, l, r, st, se, dr, dl, du, dd);
}

static void GB_KeyboardGamepadGetInput(void)
{
    const Uint8 * state = SDL_GetKeyboardState(NULL);

    int a = state[SDL_SCANCODE_X];
    int b = state[SDL_SCANCODE_Z];
    int st = state[SDL_SCANCODE_RETURN];
    int se = state[SDL_SCANCODE_RSHIFT];
    int dr = state[SDL_SCANCODE_RIGHT];
    int dl = state[SDL_SCANCODE_LEFT];
    int du = state[SDL_SCANCODE_UP];
    int dd = state[SDL_SCANCODE_DOWN];

    GB_InputSet(0, a, b, st, se, dr, dl, du, dd);
}

//@global function
int Win_MainRunningGBA(void)
{
    return WIN_MAIN_RUNNING == RUNNING_GBA;
}

//@global function
int Win_MainRunningGB(void)
{
    return WIN_MAIN_RUNNING == RUNNING_GB;
}

//@global function
void Win_MainLoopHandle(void)
{
    if(WH_HasKeyboardFocus(WinIDMain) && (WIN_MAIN_MENU_ENABLED == 0))
    {
        Sound_Enable();

        if(WIN_MAIN_RUNNING == RUNNING_GBA)
        {
            if(GBA_ShowConsoleRequested())
            {
                ConsoleShow();
                return;
            }

            Win_GBADisassemblerStartAddressSetDefault();

            GBA_KeyboardGamepadGetInput();
            GBA_RunForOneFrame();
            GBA_ConvertScreenBufferTo24RGB(WIN_MAIN_GAME_SCREEN_BUFFER);
             //GBA_UpdateFrameskip();
        }
        else if(WIN_MAIN_RUNNING == RUNNING_GB)
        {
            GB_KeyboardGamepadGetInput();
            GB_RunForOneFrame();
            GB_Screen_WriteBuffer_24RGB(WIN_MAIN_GAME_SCREEN_BUFFER);
        }
    }
    else
    {
        Sound_Disable();
    }
}

