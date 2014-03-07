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

static int RUNNING = RUNNING_NONE;

//------------------------------------------------------------------

static int FPS;
static int frames_drawn = 0;
static SDL_TimerID FPS_timer;

static Uint32 _fps_callback_function(Uint32 interval, void *param)
{
    if(RUNNING == RUNNING_GB)
        GB_HandleTime();

    FPS = frames_drawn;
    frames_drawn = 0;
    char caption[60];
    s_snprintf(caption,sizeof(caption),"GiiBiiAdvance: %d fps - %.2f%%",FPS,(float)FPS*10.0f/6.0f);
    WH_SetCaption(WinIDMain,caption);
    return interval;
}

void FPS_TimerInit(void)
{
    FPS_timer = SDL_AddTimer(1000, _fps_callback_function, NULL);
    if(FPS_timer == 0)
    {
        Debug_LogMsgArg("FPS_TimerInit(): SDL_AddTimer() failed: %s",SDL_GetError());
        Debug_DebugMsgArg("FPS_TimerInit(): SDL_AddTimer() failed: %s",SDL_GetError());
    }
}

void FPS_TimerEnd(void)
{
    SDL_RemoveTimer(FPS_timer);
}

//------------------------------------------------------------------

int CONFIG_ZOOM = 2;

#define CONFIG_ZOOM_MIN 2
#define CONFIG_ZOOM_MAX 4

//------------------------------------------------------------------

void Win_MainClearMessage(void);

//------------------------------------------------------------------

//+----------------+
//| GBA -> 240x160 |
//| GBC -> 160x144 |
//| SGB -> 256x224 |
//+----------------+

#define SCREEN_GBA 0
#define SCREEN_GB  1
#define SCREEN_SGB 2

static int SCREEN_TYPE;

static int MENU_ENABLED = 0;
static int MENU_HAS_TO_UPDATE = 0;
static char GAME_MENU_BUFFER[256*CONFIG_ZOOM_MAX * 224*CONFIG_ZOOM_MAX * 4];


static char GAME_SCREEN_BUFFER[256*224*3]; // max possible size = SGB

int GetGameScreenTextureWidth(void)
{
    if(SCREEN_TYPE == SCREEN_GB)
        return 160;
    else if(SCREEN_TYPE == SCREEN_SGB)
        return 256;
    else if(SCREEN_TYPE == SCREEN_GBA)
        return 240;
    return 0;
}

int GetGameScreenTextureHeight(void)
{
    if(SCREEN_TYPE == SCREEN_GB)
        return 144;
    else if(SCREEN_TYPE == SCREEN_SGB)
        return 224;
    else if(SCREEN_TYPE == SCREEN_GBA)
        return 160;
    return 0;
}

int GetMenuTextureWidth(void)
{
    return 256*CONFIG_ZOOM;
}

int GetMenuTextureHeight(void)
{
    return 224*CONFIG_ZOOM;
}

void GetGameScreenTextureDump(void)
{
    memset(GAME_MENU_BUFFER,0,sizeof(GAME_MENU_BUFFER));

    if(SCREEN_TYPE == SCREEN_GBA)
    {
        ScaleImage24RGB(CONFIG_ZOOM, GAME_SCREEN_BUFFER,240,160,
                   GAME_MENU_BUFFER,GetMenuTextureWidth(),GetMenuTextureHeight());
    }
    else if(SCREEN_TYPE == SCREEN_GB)
    {
        ScaleImage24RGB(CONFIG_ZOOM, GAME_SCREEN_BUFFER,160,144,
                   GAME_MENU_BUFFER,GetMenuTextureWidth(),GetMenuTextureHeight());
    }
    else if(SCREEN_TYPE == SCREEN_SGB)
    {
        ScaleImage24RGB(CONFIG_ZOOM, GAME_SCREEN_BUFFER,256,224,
                   GAME_MENU_BUFFER,GetMenuTextureWidth(),GetMenuTextureHeight());
    }

    int w = GetMenuTextureWidth();
    int h = GetMenuTextureHeight();
    int i,j;
    for(j = 0; j < h; j++) for(i = 0; i < w; i++)
    {
        if( (i^j) & 2 )
        {
            GAME_MENU_BUFFER[(j*w+i)*3+0] = 128;
            GAME_MENU_BUFFER[(j*w+i)*3+1] = 128;
            GAME_MENU_BUFFER[(j*w+i)*3+2] = 128;
        }
    }
}

void SetGameScreen(int type)
{
    SCREEN_TYPE = type;
    WH_SetSize(WinIDMain,256*CONFIG_ZOOM,224*CONFIG_ZOOM,
               GetGameScreenTextureWidth(),GetGameScreenTextureHeight(), CONFIG_ZOOM);
}

void SwitchToGame(void)
{
    if(MENU_ENABLED == 0) return;

    if(RUNNING == RUNNING_NONE) return;

    MENU_ENABLED = 0;

    SetGameScreen(SCREEN_TYPE);
}

void SwitchToMenu(void)
{
    if(MENU_ENABLED == 1) return;

    MENU_ENABLED = 1;
    MENU_HAS_TO_UPDATE = 1;

    WH_SetSize(WinIDMain,256*CONFIG_ZOOM,224*CONFIG_ZOOM, 0,0, 0);

    GetGameScreenTextureDump();
}

static int has_to_switch_to_game = 0;
static int has_to_switch_to_menu = 0;

void SwitchToGameDelayed(void)
{
    has_to_switch_to_game = 1;
}

void SwitchToMenuDelayed(void)
{
    has_to_switch_to_menu = 1;
}

//------------------------------------------------------------------


int GetRomType(char * name)
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

void UnloadROM(int save_data)
{
    Win_MainClearMessage();

    if(RUNNING == RUNNING_GBA)
    {
        GBA_EndRom(save_data);
        GBA_DebugClearBreakpointAll();
    }
    else if(RUNNING == RUNNING_GB)
    {
        GB_End(save_data);
        GB_DebugClearBreakpointAll();
    }
    else return;

    if(bios_buffer) free(bios_buffer);
    if(rom_buffer) free(rom_buffer);
    bios_buffer = NULL;
    rom_buffer = NULL;

    memset(GAME_SCREEN_BUFFER,0,sizeof(GAME_SCREEN_BUFFER)); //clear screen buffer
    WH_Render( WinIDMain, GAME_SCREEN_BUFFER); // clear screen
    GetGameScreenTextureDump();

    RUNNING = RUNNING_NONE;

    Sound_SetCallback(NULL);

    SwitchToMenu();

    MENU_HAS_TO_UPDATE = 1;
}

int LoadROMAutodetect(char * path)
{
    if(path == NULL) return 0;
    if(strlen(path) == 0) return 0;

    Win_MainClearMessage();

    if(RUNNING != RUNNING_NONE)
        UnloadROM(1);

    int type = GetRomType(path);

    if(type == RUNNING_NONE)
    {
        return 0;
    }
    else if(type == RUNNING_GB)
    {
        if(GB_ROMLoad(path))
        {
            if(GB_IsEnabledSGB()) SetGameScreen(SCREEN_SGB);
            else SetGameScreen(SCREEN_GB);
            RUNNING = RUNNING_GB;

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
            SwitchToGameDelayed();

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

        RUNNING = RUNNING_GBA;

        Sound_SetCallback(GBA_SoundCallback);

        SetGameScreen(SCREEN_GBA);
#if 0
        if(EmulatorConfig.frameskip == -1)
        {
            FRAMESKIP = 0;
            AUTO_FRAMESKIP_WAIT = 2;
        }

        GLWindow_MenuEnableROMCommands(1);
        GLWindow_ClearPause();
#endif
        SwitchToGameDelayed();

        return 1;
    }

    return 0;
}

//------------------------------------------------------------------

//------------------------------------------------------------------

// FILE EXPLORER WINDOW

#define FILE_EXPLORER_NUM_ELEMENTS 28

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

static int file_explorer_windows_selection = 0;

void Win_MainFileExplorerRefresh(void)
{
    if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win) == 0) return;

    GUI_ConsoleClear(&win_main_fileexpoler_con);

    FileExplorer_LoadFolder();

    int numfiles = FileExplorer_GetNumFiles();

    int start_print_index;

    if(FILE_EXPLORER_NUM_ELEMENTS >= numfiles)
        start_print_index = 0;
    else if(file_explorer_windows_selection < (FILE_EXPLORER_NUM_ELEMENTS/2))
        start_print_index = 0;
    else
        start_print_index = file_explorer_windows_selection - (FILE_EXPLORER_NUM_ELEMENTS/2);

    int i;
    for(i = 0; i < FILE_EXPLORER_NUM_ELEMENTS; i++)
    {
        int selected_file = i + start_print_index;

        if(selected_file < numfiles)
        {
            if(selected_file == file_explorer_windows_selection)
                GUI_ConsoleColorizeLine(&win_main_fileexpoler_con,i, 0xFFFFFF00);

            GUI_ConsoleModePrintf(&win_main_fileexpoler_con,0,i,"%s %s",
                                    FileExplorer_GetIsDir(selected_file) ? "<DIR>" : "     ",
                                    FileExplorer_GetName(selected_file));
        }
    }
}

void Win_MainFileExplorerClose(void)
{
    FileExplorer_ListFree();
    GUI_WindowSetEnabled(&mainwindow_fileexplorer_win,0);
}

void Win_MainFileExplorerUp(void)
{
    FileExplorer_SelectEntry("..");
    Win_MainFileExplorerRefresh();
}

void Win_MainFileExplorerOpenIndex(int i)
{
    file_explorer_windows_selection = 0;

    //Debug_DebugMsg(FileExplorer_GetName(i));
    int is_dir = FileExplorer_SelectEntry(FileExplorer_GetName(i));
    Win_MainFileExplorerRefresh();

    if(is_dir == 0)
    {
        //Debug_DebugMsg(FileExplorer_GetResultFilePath());
        Win_MainFileExplorerClose();
        LoadROMAutodetect(FileExplorer_GetResultFilePath());
    }
}

void Win_MainFileExplorerTextBoxCallback(int x, int y)
{
    int i = y / FONT_12_HEIGHT;
    Win_MainFileExplorerOpenIndex(i);
}

void Win_MainFileExplorerOpen(void)
{
    GUI_WindowSetEnabled(&mainwindow_fileexplorer_win,1);
    FileExplorer_SetPath(DirGetRunningPath());
    Win_MainFileExplorerRefresh();
}

void Win_MainFileExplorerCreate(void)
{
    GUI_SetTextBox(&win_main_fileexpoler_textbox,&win_main_fileexpoler_con,
                   7,7,
                   FONT_12_WIDTH*64,FONT_12_HEIGHT*FILE_EXPLORER_NUM_ELEMENTS,
                   Win_MainFileExplorerTextBoxCallback);
    GUI_SetButton(&win_main_fileexpoler_up_btn,7,353,FONT_12_WIDTH*4,24,
                  "Up",Win_MainFileExplorerUp);
    GUI_SetButton(&win_main_fileexpoler_cancel_btn,399,353,FONT_12_WIDTH*8,24,
                  "Cancel",Win_MainFileExplorerClose);
}

//------------------------------------------------------------------

//------------------------------------------------------------------

void Win_MainClose(void)
{
    Win_MainFileExplorerClose();
    UnloadROM(1);
}

void Win_MainCloseNoSave(void)
{
    Win_MainFileExplorerClose();
    UnloadROM(0);
}

void Win_MainExit(void)
{
    Win_MainFileExplorerClose();
    UnloadROM(1);
    WH_CloseAll();
}

void Win_MainOpenDisassembler(void)
{
    Win_GBADisassemblerCreate();
    Win_GBDisassemblerCreate();
}

void Win_MainOpenMemViewer(void)
{
    Win_GBAMemViewerCreate();
    Win_GBMemViewerCreate();
}

void Win_MainOpenIOViewer(void)
{
    Win_GBIOViewerCreate();
}

//------------------------------------------------------------------

//Window Menu

static _gui_menu_entry mmfile_open = {"Open" , Win_MainFileExplorerOpen};
static _gui_menu_entry mmfile_separator = {" " , NULL};
static _gui_menu_entry mmfile_close = {"Close" , Win_MainClose};
static _gui_menu_entry mmfile_closenosav = {"Close without saving", Win_MainCloseNoSave};
static _gui_menu_entry mmfile_exit = {"Exit", Win_MainExit};


static _gui_menu_entry * mmfile_elements[] = {
    &mmfile_open,
    &mmfile_separator,
    &mmfile_close,
    &mmfile_closenosav,
    &mmfile_separator,
    &mmfile_exit,
    NULL
};

static _gui_menu_list main_menu_file = {
    "File",
    mmfile_elements,
};

static _gui_menu_entry mmdebug_disas = {"Disassembler (F5)" , Win_MainOpenDisassembler};
static _gui_menu_entry mmdebug_memview = {"Memory Viewer (F6)" , Win_MainOpenMemViewer};
static _gui_menu_entry mmdebug_ioview = {"I/O Viewer (F7)" , Win_MainOpenIOViewer};

static _gui_menu_entry * mmdisas_elements[] = {
    &mmdebug_disas,
    &mmdebug_memview,
    &mmdebug_ioview,
    NULL
};

static _gui_menu_list main_menu_disas = {
    "Debug",
    mmdisas_elements,
};

static _gui_menu_list * mmenu_lists[] = {
    &main_menu_file,
    &main_menu_disas,
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

void Win_MainShowMessage(int type, const char * text) // 0 = error, 1 = debug, 2 = console
{
    SwitchToMenu();

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

void Win_MainClearMessage(void)
{
    memset(&mainwindow_show_message_win,0,sizeof(mainwindow_show_message_win));
}

//------------------------------------------------------------------

void ChangeZoom(int newzoom)
{
    if( (newzoom < 2) && (newzoom > 4) )
    {
        Debug_LogMsgArg("SwitchToMenu(): Unsupported zoom level: %d",newzoom);
        return;
    }

    CONFIG_ZOOM = newzoom;
    EmulatorConfig.screen_size = CONFIG_ZOOM;

    if(MENU_ENABLED)
    {
        MENU_HAS_TO_UPDATE = 1;

        WH_SetSize(WinIDMain,256*CONFIG_ZOOM,224*CONFIG_ZOOM, 0,0, 0);
    }
    else
    {
        SetGameScreen(SCREEN_TYPE);
    }

    //change captured image

    GetGameScreenTextureDump();

    GUI_SetBitmap(&mainwindow_bg,0,0,
                  GetMenuTextureWidth(),GetMenuTextureHeight(),GAME_MENU_BUFFER,
                  SwitchToGameDelayed);
}

//------------------------------------------------------------------

int Win_MainEventCallback(SDL_Event * e)
{
    if(MENU_ENABLED)
        MENU_HAS_TO_UPDATE |= GUI_SendEvent(&mainwindow_window_gui,e);

    int close_this = 0;

    if( e->type == SDL_KEYDOWN )
    {
        switch( e->key.keysym.sym )
        {
            case SDLK_F5:
                Win_MainOpenDisassembler();
                break;
            case SDLK_F6:
                Win_MainOpenMemViewer();
                break;
            case SDLK_F7:
                Win_MainOpenIOViewer();
                break;

            case SDLK_UP:
                if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win))
                {
                    if(file_explorer_windows_selection > 0)
                        file_explorer_windows_selection --;
                    Win_MainFileExplorerRefresh();
                    MENU_HAS_TO_UPDATE = 1;
                }
                break;
            case SDLK_DOWN:
                if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win))
                {
                    if(file_explorer_windows_selection < (FileExplorer_GetNumFiles()-1))
                        file_explorer_windows_selection ++;
                    Win_MainFileExplorerRefresh();
                    MENU_HAS_TO_UPDATE = 1;
                }
                break;
            case SDLK_RETURN:
                if(GUI_WindowGetEnabled(&mainwindow_fileexplorer_win))
                {
                    Win_MainFileExplorerOpenIndex(file_explorer_windows_selection);
                    MENU_HAS_TO_UPDATE = 1;
                }
                break;

            //case SDLK_p:
            //    if(SDL_GetModState()|KMOD_CTRL)
            //    {
            //        if(MENU_ENABLED)
            //            SwitchToGame();
            //        else
            //            SwitchToMenu();
            //    }
            //    break;

            case SDLK_1:
                ChangeZoom(2);
                return 1;
            case SDLK_2:
                ChangeZoom(3);
                return 1;
            case SDLK_3:
                ChangeZoom(4);
                return 1;

            case SDLK_ESCAPE:
                WH_CloseAll();
                return 1;
        }
    }
    else if(e->type == SDL_MOUSEWHEEL)
    {
        file_explorer_windows_selection -= e->wheel.y*3;

        if(file_explorer_windows_selection < 0)
            file_explorer_windows_selection = 0;
        if(file_explorer_windows_selection > (FileExplorer_GetNumFiles()-1) )
            file_explorer_windows_selection = FileExplorer_GetNumFiles() - 1;

        MENU_HAS_TO_UPDATE = 1;
        Win_MainFileExplorerRefresh();
    }
    else if(e->type == SDL_MOUSEBUTTONDOWN)
    {
        if(MENU_ENABLED == 0)
        {
            SwitchToMenuDelayed();
        }
        MENU_HAS_TO_UPDATE = 1;
   }
    else if(e->type == SDL_MOUSEBUTTONUP)
    {
        MENU_HAS_TO_UPDATE = 1;
    }
    else if(e->type == SDL_WINDOWEVENT)
    {
        if(e->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            SwitchToMenu();
            MENU_HAS_TO_UPDATE = 1;
        }
        if(e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            MENU_HAS_TO_UPDATE = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_EXPOSED)
        {
            MENU_HAS_TO_UPDATE = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_CLOSE)
        {
            close_this = 1;
        }
    }
    else if(e->type == SDL_DROPFILE)
    {
        LoadROMAutodetect(e->drop.file);
        //Debug_LogMsgArg("Drag file: %s",e->drop.file);
        SDL_free(e->drop.file);
    }
    if(has_to_switch_to_game)
    {
        has_to_switch_to_game = 0;
        has_to_switch_to_menu = 0;
        SwitchToGame();
    }
    else if(has_to_switch_to_menu)
    {
        has_to_switch_to_game = 0;
        has_to_switch_to_menu = 0;
        SwitchToMenu();
    }

    if(close_this)
    {
        UnloadROM(1); // save things
        WH_CloseAll();
        //Debug_LogMsgArg("callback: closing win 0");
        return 1;
    }

    return 0;
}

void Win_close(void)
{
    GUI_WindowSetEnabled(&mainwindow_configwin,0);
}

int Win_MainCreate(char * rom_path)
{
    ChangeZoom(EmulatorConfig.screen_size);

    GUI_SetButton(&mainwindow_subwindow_btn,10,50,7*FONT_12_WIDTH,FONT_12_HEIGHT,"CLOSE",Win_close);
    GUI_SetButton(&mainwindow_subwindow_btn2,10,70,7*FONT_12_WIDTH,FONT_12_HEIGHT,"TEST",NULL);

    GUI_SetButton(&mainwindow_btn,0,0,30*FONT_12_WIDTH,15*FONT_12_HEIGHT,"MAIN GUI",mainbtncallback_openwin);
    GUI_SetWindow(&mainwindow_configwin,100,50,100,200,&mainwindow_subwindow_config_gui,"TEST");

    Win_MainFileExplorerCreate();
    GUI_SetWindow(&mainwindow_fileexplorer_win,25,25,256*2-50,224*2-50,&mainwindow_subwindow_fileexplorer_gui,
                  "File Explorer");

    Win_MainClearMessage();

    WinIDMain = WH_Create(256*CONFIG_ZOOM,224*CONFIG_ZOOM, 0,0, 0);

    MENU_ENABLED = 1;
    MENU_HAS_TO_UPDATE = 1;

    WH_SetCaption(WinIDMain,"GiiBiiAdvance");

    if( WinIDMain == -1 )
    {
        Debug_LogMsgArg( "Win_MainCreate(): Window could not be created!" );
        return 1;
    }

    WH_SetEventCallback(WinIDMain,Win_MainEventCallback);
    WH_SetEventMainWindow(WinIDMain);

    FPS_TimerInit();

    RUNNING = RUNNING_NONE;

    //if inited the emulator with a game...
    if(rom_path)
    {
        if(strlen(rom_path) > 0)
        {
            LoadROMAutodetect(rom_path);
        }
    }

    return 0;
}

static char __buffer_when_menu[256*4*224*4*3];
void Win_MainRender(void)
{
    if(MENU_ENABLED == 0)
    {
        if(RUNNING != RUNNING_NONE)
        {
            frames_drawn ++;
            //FU_Print12(GAME_SCREEN_BUFFER,GetGameScreenTextureWidth(),GetGameScreenTextureHeight(),0,0,
            //           "FPS: %d",FPS);
            WH_Render(WinIDMain, GAME_SCREEN_BUFFER);
        }
    }
    else
    {
        if(MENU_HAS_TO_UPDATE)
        {
            MENU_HAS_TO_UPDATE = 0;
            GUI_Draw(&mainwindow_window_gui,__buffer_when_menu,GetMenuTextureWidth(),GetMenuTextureHeight(),1);
            WH_Render( WinIDMain, __buffer_when_menu );
        }
    }
}

void GBA_KeyboardGamepadGetInput(void)
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

void GB_KeyboardGamepadGetInput(void)
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

int Win_MainRunningGBA(void)
{
    return RUNNING == RUNNING_GBA;
}

int Win_MainRunningGB(void)
{
    return RUNNING == RUNNING_GB;
}

void Win_MainLoopHandle(void)
{
    if(WH_HasKeyboardFocus(WinIDMain) && (MENU_ENABLED == 0))
    {
        Sound_Enable();

        if(RUNNING == RUNNING_GBA)
        {
            if(GBA_ShowConsoleRequested())
            {
                ConsoleShow();
                return;
            }

            Win_GBADisassemblerStartAddressSetDefault();

            GBA_KeyboardGamepadGetInput();
            GBA_RunForOneFrame();
            GBA_ConvertScreenBufferTo24RGB(GAME_SCREEN_BUFFER);
             //GBA_UpdateFrameskip();
        }
        else if(RUNNING == RUNNING_GB)
        {
            GB_KeyboardGamepadGetInput();
            GB_RunForOneFrame();
            GB_Screen_WriteBuffer_24RGB(GAME_SCREEN_BUFFER);
        }
    }
    else
    {
        Sound_Disable();
    }
}

