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

#include <windows.h>
#include <gl/gl.h>
#include <png.h>

#include <ctype.h>
#include <stdio.h>

#include "build_options.h"
#include "resource.h"

#include "main.h"
#include "config.h"
#include "gba_core/gba.h"
#include "gba_core/save.h"
#include "gba_core/bios.h"
#include "gba_core/sound.h"
#include "gb_core/sound.h"
#include "gb_core/gb_main.h"
#include "gb_core/general.h"
#include "gb_core/gameboy.h"
#include "gui_main.h"
#include "gui_config.h"
#include "save_png.h"
#include "gui_mainloop.h"
#include "gui_gba_debugger.h"
#include "gui_gba_debugger_vram.h"
#include "gui_gb_debugger.h"
#include "gui_gb_debugger_vram.h"
#include "gui_sgb_debugger.h"
#include "gui_text_windows.h"

//---------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

int ZOOM;

char biospath[MAXPATHLEN];

HINSTANCE hInstance;
int Keys_JustPressed[256];
int Keys_Down[256];
int GLWindow_Active = TRUE;
HWND hWndMain; //Used for telling all the windows which is the parent

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

#define CM_LOADROM        100
#define CM_CLOSEROM       101
#define CM_CLOSEROMNOSAVE 102
#define CM_RESET          103
#define CM_PAUSE          104
#define CM_ROMINFO        105
#define CM_SCREENSHOT     106
#define CM_EXIT           107

#define CM_CONFIGURATION  110
#define CM_TOGGLEMUTE     111
#define CM_SYSTEMSTATUS   112

#define CM_DISASSEMBLER   120
#define CM_MEMORYVIEWER   121
#define CM_IOVIEWER       122
#define CM_TILEVIEWER     123
#define CM_MAPVIEWER      124
#define CM_SPRVIEWER      125
#define CM_PALVIEWER      126
#define CM_SGBVIEWER      127

#define CM_README         130
#define CM_LICENSE        131
#define CM_ABOUT          132

//---------------------------------------------------------------------------

void GLWindow_MenuEnableROMCommands(int enable)
{
    EnableMenuItem(GetMenu(hWndMain),CM_CLOSEROM,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_CLOSEROMNOSAVE,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_RESET,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_PAUSE,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_ROMINFO,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_SCREENSHOT,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));

    EnableMenuItem(GetMenu(hWndMain),CM_DISASSEMBLER,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_MEMORYVIEWER,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_IOVIEWER,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_TILEVIEWER,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_MAPVIEWER,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_SPRVIEWER,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_PALVIEWER,MF_BYCOMMAND|(enable?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
    EnableMenuItem(GetMenu(hWndMain),CM_SGBVIEWER,
            MF_BYCOMMAND|((enable&&GameBoy.Emulator.SGBEnabled)?MF_ENABLED:(MF_DISABLED|MF_GRAYED)));
}

static void GLWindow_CreateMenu(HWND hWnd)
{
    HMENU hMenuFile = CreateMenu();
    AppendMenu(hMenuFile, MF_STRING, CM_LOADROM, "&Load\bCTRL+L");
    AppendMenu(hMenuFile, MF_STRING, CM_CLOSEROM, "&Close\bCTRL+C");
    AppendMenu(hMenuFile, MF_STRING, CM_CLOSEROMNOSAVE, "&Close without saving");
    AppendMenu(hMenuFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuFile, MF_STRING, CM_RESET, "&Reset\bCTRL+R");
    AppendMenu(hMenuFile, MF_STRING, CM_PAUSE, "&Pause\bCTRL+P");
    AppendMenu(hMenuFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuFile, MF_STRING, CM_ROMINFO, "ROM &Information");
    AppendMenu(hMenuFile, MF_STRING, CM_SCREENSHOT, "&Screenshot\bF12");
    AppendMenu(hMenuFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuFile, MF_STRING, CM_EXIT, "&Exit\bCTRL+E");

    HMENU hMenuOptions = CreateMenu();
    AppendMenu(hMenuOptions, MF_STRING, CM_CONFIGURATION, "&Configuration");
    AppendMenu(hMenuOptions, MF_STRING, CM_TOGGLEMUTE, "&Mute Sound\bCTRL+M");
    AppendMenu(hMenuOptions, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuOptions, MF_STRING, CM_SYSTEMSTATUS, "&System Status");

    HMENU hMenuDebug = CreateMenu();
    AppendMenu(hMenuDebug, MF_STRING, CM_DISASSEMBLER, "&Disassembler\bF11");
    AppendMenu(hMenuDebug, MF_STRING, CM_MEMORYVIEWER, "&Memory Viewer");
    AppendMenu(hMenuDebug, MF_STRING, CM_IOVIEWER, "&I/O Viewer");
    AppendMenu(hMenuDebug, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuDebug, MF_STRING, CM_TILEVIEWER, "&Tile Viewer");
    AppendMenu(hMenuDebug, MF_STRING, CM_MAPVIEWER, "M&ap Viewer");
    AppendMenu(hMenuDebug, MF_STRING, CM_SPRVIEWER, "&Sprite Viewer");
    AppendMenu(hMenuDebug, MF_STRING, CM_PALVIEWER, "&Palette Viewer");
    AppendMenu(hMenuDebug, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuDebug, MF_STRING, CM_SGBVIEWER, "S&GB Viewer");

    HMENU hMenuHelp = CreateMenu();
    AppendMenu(hMenuHelp, MF_STRING, CM_README, "&Readme\bF1");
    AppendMenu(hMenuHelp, MF_STRING, CM_LICENSE, "&License");
    AppendMenu(hMenuHelp, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuHelp, MF_STRING, CM_ABOUT, "&About");

    //--------------------

    HMENU hMenuBar = CreateMenu();
    AppendMenu(hMenuBar, MF_STRING | MF_POPUP, (UINT)hMenuFile, "&File");
    AppendMenu(hMenuBar, MF_STRING | MF_POPUP, (UINT)hMenuOptions, "&Options");
    AppendMenu(hMenuBar, MF_STRING | MF_POPUP, (UINT)hMenuDebug, "&Debug");
    AppendMenu(hMenuBar, MF_STRING | MF_POPUP, (UINT)hMenuHelp, "&Help");

    SetMenu(hWnd, hMenuBar);
}

static ACCEL accelerators[] = {
    {FCONTROL|FNOINVERT|FVIRTKEY,'L',CM_LOADROM },
    {FCONTROL|FNOINVERT|FVIRTKEY,'C',CM_CLOSEROM },
    {FCONTROL|FNOINVERT|FVIRTKEY,'R',CM_RESET },
    {FCONTROL|FNOINVERT|FVIRTKEY,'P',CM_PAUSE },
    {FNOINVERT|FVIRTKEY,VK_F12,CM_SCREENSHOT},
    {FCONTROL|FNOINVERT|FVIRTKEY,'E',CM_EXIT },

    {FCONTROL|FNOINVERT|FVIRTKEY,'M',CM_TOGGLEMUTE },

    {FNOINVERT|FVIRTKEY,VK_F11,CM_DISASSEMBLER},

    {FNOINVERT|FVIRTKEY,VK_F1,CM_README}
};

static HACCEL hAccTable;

int GLWindow_HandleEvents(void)
{
    memset(Keys_JustPressed,0,sizeof(Keys_JustPressed));
    MSG msg;
    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
        if(msg.message == WM_QUIT) return 1;

        if(!TranslateAccelerator(hWndMain, hAccTable, &msg))
            TranslateMessage(&msg);

        //TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

//--------------------------------------------------------------------

#define OPENFILE_MAXPATHLEN (2048)
char openfilepath[OPENFILE_MAXPATHLEN];

char * GLWindow_OpenRom(HWND hwnd)
{
    OPENFILENAME ofn; // common dialog box structure
    char szFile[OPENFILE_MAXPATHLEN]; // buffer for file name

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd; // owner window
    ofn.lpstrFile = szFile;
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Supported Roms\0*.GBA;*.AGB;*.BIN;*.GBC;*.GB;*.CGB;*.SGB\0"
                      "GBA Roms (*.GBA;*.AGB;*.BIN)\0*.GBA;*.AGB;*.BIN\0"
                      "GBC/GB Roms (*.GBC;*.GB;*.CGB;*.SGB)\0*.GBC;*.GB;*.CGB;*.SGB\0"
                      "All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box.
    if(GetOpenFileName(&ofn)==TRUE) { lstrcpy(openfilepath,ofn.lpstrFile); return openfilepath; }
    else return NULL;
    //MessageBox(hwnd, ofn.lpstrFile, "OpenFile Dialog", MB_OK);
}

void GLWindow_SetMute(int value)
{
    EmulatorConfig.snd_mute = value;
    CheckMenuItem(GetMenu(hWndMain),CM_TOGGLEMUTE,MF_BYCOMMAND|(value?MF_CHECKED:MF_UNCHECKED));
}

void GLWindow_TogglePause(void)
{
    PAUSED ^= 1;
    if(EmulatorConfig.frameskip == -1)
    {
        FRAMESKIP = 0;
        AUTO_FRAMESKIP_WAIT = 2;
    }
    CheckMenuItem(GetMenu(hWndMain),CM_PAUSE,MF_BYCOMMAND|(PAUSED?MF_CHECKED:MF_UNCHECKED));
}

void GLWindow_ClearPause(void)
{
    PAUSED = 0;
    if(EmulatorConfig.frameskip == -1)
    {
        FRAMESKIP = 0;
        AUTO_FRAMESKIP_WAIT = 2;
    }
    CheckMenuItem(GetMenu(hWndMain),CM_PAUSE,MF_BYCOMMAND|MF_UNCHECKED);
}

int Get_Rom_Type(char * name)
{
    char extension[4];
    int len = strlen(name);

    extension[3] = '\0';
    extension[2] = toupper(name[len-1]);
    extension[1] = toupper(name[len-2]);
    extension[0] = toupper(name[len-3]);
    if(strcmp(extension,"GBA") == 0) return RUN_GBA;
    if(strcmp(extension,"AGB") == 0) return RUN_GBA;
    if(strcmp(extension,"BIN") == 0) return RUN_GBA;
    if(strcmp(extension,"GBC") == 0) return RUN_GB;
    if(strcmp(extension,"CGB") == 0) return RUN_GB;
    if(strcmp(extension,"SGB") == 0) return RUN_GB;

    extension[0] = extension[1];
    extension[1] = extension[2];
    extension[2] = '\0';
    if(strcmp(extension,"GB") == 0) return RUN_GB;

    return RUN_NONE;
}


void GLWindow_UnloadRom(int save)
{
    if(RUNNING != RUN_NONE)
    {
        if(RUNNING == RUN_GBA) GBA_EndRom(save);
        else if(RUNNING == RUN_GB) GB_End(save);

        GLWindow_ClearPause();
        GLWindow_MenuEnableROMCommands(0);

        RUNNING = RUN_NONE;

        //---------------------------

        if(EmulatorConfig.auto_close_debugger)
        {
            GLWindow_GBACloseDissasembler();
            GLWindow_GBACloseMemViewer();
            GLWindow_GBACloseIOViewer();

            GLWindow_GBAClosePalViewer();
            GLWindow_GBACloseSprViewer();
            GLWindow_GBACloseMapViewer();
            GLWindow_GBACloseTileViewer();

            GLWindow_GBCloseDissasembler();
            GLWindow_GBCloseMemViewer();
            GLWindow_GBCloseIOViewer();

            GLWindow_GBClosePalViewer();
            GLWindow_GBCloseSprViewer();
            GLWindow_GBCloseMapViewer();
            GLWindow_GBCloseTileViewer();

            GLWindow_SGBViewerClose();
        }
    }
}

void * buffer, * bios;
int GLWindow_LoadRom(char * path)
{
    if(path == NULL) return 0;
    if(strlen(path) == 0) return 0;

    GLWindow_UnloadRom(1); //close and save previous loaded ROM (if any)

    if(buffer) free(buffer);
    if(bios) free(bios);

    int rom_type = Get_Rom_Type(path);

    if(rom_type == RUN_GBA)
    {
        unsigned int size;

        char current_bios[MAXPATHLEN];
        strcpy(current_bios,biospath);
        strcat(current_bios,"\\" GBA_BIOS_FILENAME);

        FileLoad_NoError(current_bios,&bios,&size);
        if(size == 0) GBA_BiosLoaded(0);
        else GBA_BiosLoaded(1);

        FileLoad(path,&buffer,&size);
        if(buffer == NULL) return 0;
        GBA_SaveSetFilename(path);
        GBA_InitRom(bios,buffer,size);
        RUNNING = RUN_GBA;
        GLWindow_ChangeScreen(SCR_GBA);

        if(EmulatorConfig.frameskip == -1)
        {
            FRAMESKIP = 0;
            AUTO_FRAMESKIP_WAIT = 2;
        }

        GLWindow_MenuEnableROMCommands(1);
        GLWindow_ClearPause();
        return 1;
    }
    else if(rom_type == RUN_GB)
    {
        if(GB_MainLoad(path))
        {
            extern _GB_CONTEXT_ GameBoy;

            if(GameBoy.Emulator.SGBEnabled) GLWindow_ChangeScreen(SCR_SGB);
            else GLWindow_ChangeScreen(SCR_GB);
            RUNNING = RUN_GB;

            if(EmulatorConfig.frameskip == -1)
            {
                FRAMESKIP = 0;
                AUTO_FRAMESKIP_WAIT = 2;
            }

            GLWindow_MenuEnableROMCommands(1);
            GLWindow_ClearPause();
            return 1;
        }
    }

    return 0;
}

//----------------------------------------------------------------------------------------------

static LRESULT CALLBACK GLWindow_Process(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	    case WM_CREATE:
	    {
	        hAccTable = CreateAcceleratorTable((LPACCEL)&accelerators,ARRAY_NUM_ELEMENTS(accelerators));
            break;
	    }
	    case WM_KEYDOWN:
        {
            Keys_Down[wParam] = TRUE;
            Keys_JustPressed[wParam] = TRUE;
            break;
        }
        case WM_KEYUP:
        {
            Keys_Down[wParam] = FALSE;
            break;
        }
	    case WM_COMMAND:
	    {
            switch(LOWORD(wParam))
            {
                case CM_LOADROM:
                    if(MESSAGE_SHOWING) break;
                    GLWindow_LoadRom(GLWindow_OpenRom(hWnd));
                    break;
                case CM_CLOSEROM:
                    GLWindow_UnloadRom(1);
                    SendMessage(hWndMain,WM_EXITMENULOOP,0,0);
                    break;
                case CM_CLOSEROMNOSAVE:
                    GLWindow_UnloadRom(0);
                    SendMessage(hWndMain,WM_EXITMENULOOP,0,0);
                    break;
                case CM_ROMINFO:
                    if(RUNNING != RUN_NONE) ConsoleShow();
                    break;
                case CM_RESET:
                    if(RUNNING != RUN_NONE)
                    {
                        GLWindow_ClearPause();
                        if(RUNNING == RUN_GBA) GBA_Reset();
                        else if(RUNNING == RUN_GB) GB_HardReset();
                        if(EmulatorConfig.frameskip == -1)
                        {
                            FRAMESKIP = 0;
                            AUTO_FRAMESKIP_WAIT = 2;
                        }
                        SendMessage(hWndMain,WM_EXITMENULOOP,0,0);
                    }
                    break;
                case CM_PAUSE:
                    if(RUNNING != RUN_NONE) GLWindow_TogglePause();
                    SendMessage(hWndMain,WM_EXITMENULOOP,0,0);
                    break;
                case CM_SCREENSHOT:
                    if(RUNNING == RUN_GBA) GBA_Screenshot();
                    else if(RUNNING == RUN_GB) GB_Screenshot();
                    SendMessage(hWndMain,WM_EXITMENULOOP,0,0);
                    break;
                case CM_EXIT:
                    PostQuitMessage(WM_QUIT);
                    break;

                case CM_CONFIGURATION:
                    GLWindow_Configuration();
                    break;
                case CM_TOGGLEMUTE:
                    GLWindow_SetMute(EmulatorConfig.snd_mute^1);
                    SendMessage(hWndMain,WM_EXITMENULOOP,0,0);
                    break;
                case CM_SYSTEMSTATUS:
                    GLWindow_ShowSystemStatus();
                    break;

                case CM_DISASSEMBLER:
                    if(RUNNING == RUN_GB) GLWindow_GBCreateDissasembler();
                    else if(RUNNING == RUN_GBA) GLWindow_GBACreateDissasembler();
                    break;
                case CM_MEMORYVIEWER:
                    if(RUNNING == RUN_GB) GLWindow_GBCreateMemViewer();
                    else if(RUNNING == RUN_GBA) GLWindow_GBACreateMemViewer();
                    break;
                case CM_IOVIEWER:
                    if(RUNNING == RUN_GB) GLWindow_GBCreateIOViewer();
                    else if(RUNNING == RUN_GBA) GLWindow_GBACreateIOViewer();
                    break;
                case CM_TILEVIEWER:
                    if(RUNNING == RUN_GB) GLWindow_GBCreateTileViewer();
                    else if(RUNNING == RUN_GBA) GLWindow_GBACreateTileViewer();
                    break;
                case CM_MAPVIEWER:
                    if(RUNNING == RUN_GB) GLWindow_GBCreateMapViewer();
                    else if(RUNNING == RUN_GBA) GLWindow_GBACreateMapViewer();
                    break;
                case CM_SPRVIEWER:
                    if(RUNNING == RUN_GB) GLWindow_GBCreateSprViewer();
                    else if(RUNNING == RUN_GBA) GLWindow_GBACreateSprViewer();
                    break;
                case CM_PALVIEWER:
                    if(RUNNING == RUN_GB) GLWindow_GBCreatePalViewer();
                    else if(RUNNING == RUN_GBA) GLWindow_GBACreatePalViewer();
                    break;
                case CM_SGBVIEWER:
                    if(RUNNING == RUN_GB)
                        if(GameBoy.Emulator.SGBEnabled) GLWindow_SGBCreateViewer();
                    break;

                case CM_README:
                    GLWindow_ShowReadme();
                    break;
                case CM_LICENSE:
                    GLWindow_ShowLicense();
                    break;
                case CM_ABOUT:
                    GLWindow_ShowAbout();
                    break;

                default:
                    break;
            }
            break;
	    }
		case WM_ACTIVATE: // Watch For Window Activate Message
		{
			// LoWord Can Be WA_INACTIVE, WA_ACTIVE, WA_CLICKACTIVE,
			// The High-Order Word Specifies The Minimized State Of The Window Being Activated Or Deactivated.
			// A NonZero Value Indicates The Window Is Minimized.
			if ((LOWORD(wParam) != WA_INACTIVE) && !((BOOL)HIWORD(wParam)))
			{
				GLWindow_Active = TRUE;
				if(RUNNING == RUN_GBA) GBA_SoundResetBufferPointers();
				else if(RUNNING == RUN_GB) GB_SoundResetBufferPointers();
			}
			else
			{
				GLWindow_Active = FALSE;
				memset(Keys_Down,0,sizeof(Keys_Down));
			}
			return 0;
		}
		case WM_INITMENU:
        case WM_INITMENUPOPUP:
        case WM_ENTERMENULOOP:
        {
            //WM_INITMENU
            //    hmenuInit = (HMENU) wParam; // manipulador de menú a inicializar
            //WM_INITMENUPOPUP
            //    hmenuPopup = (HMENU) wParam;          // manipulador de submenú
            //    uPos = (UINT) LOWORD(lParam);         // posición del ítem del submenú
            //    fSystemMenu = (BOOL) HIWORD(lParam);  // bandera de menú de ventana
            GLWindow_Active = FALSE;
            memset(Keys_Down,0,sizeof(Keys_Down));
            SoundMasterEnable(0);
            break;
		}
		case WM_EXITMENULOOP:
		{
		    GLWindow_Active = TRUE;
		    SoundMasterEnable(1);
		    if(RUNNING == RUN_GBA) GBA_SoundResetBufferPointers();
            else if(RUNNING == RUN_GB) GB_SoundResetBufferPointers();
		    break;
		}
		case WM_PAINT:
        {
            GLWindow_SwapBuffers();
            break;
        }
		case WM_SYSCOMMAND:
		{
			switch(wParam)
			{
				case SC_SCREENSAVE: // Screensaver Trying To Start?
				case SC_MONITORPOWER: // Monitor Trying To Enter Powersave?
                    return 0; // Prevent From Happening
                default:
                    break;
			}
			break;
		}
		case WM_KILLFOCUS:
		{
		    GLWindow_Active = FALSE;
            SoundMasterEnable(0);
            break;
		}
		case WM_SETFOCUS:
		{
		    GLWindow_Active = TRUE;
            SoundMasterEnable(1);
            if(RUNNING == RUN_GBA) GBA_SoundResetBufferPointers();
            else if(RUNNING == RUN_GB) GB_SoundResetBufferPointers();
            break;
		}
		case WM_CLOSE:
		{
		    DestroyAcceleratorTable(hAccTable);
			PostQuitMessage(WM_QUIT);
			return 0;
		}
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

static HDC hDC = NULL;
static HGLRC hRC = NULL;
static HWND hWnd = NULL; //The same as hWndMain

inline void GLWindow_SwapBuffers(void)
{
    SwapBuffers(hDC); // Swap Buffers (Double Buffering)
}

void GLWindow_SetCaption(const char * text)
{
    SetWindowText(hWnd,(LPCTSTR)text);
}

static DWORD dwExStyle; // Window Extended Style
static DWORD dwStyle; // Window Style

int GLWindow_GetMenuLenghtPixels(void)
{
    int items = GetMenuItemCount(GetMenu(hWnd));
    if(items == 0) return 0;

    int len = GetSystemMetrics(SM_CXMENUSIZE) * (items-1); // gap between buttons
    int i;
    for(i = 0; i < items; i++)
    {
        RECT rc;
        GetMenuItemRect(hWnd,GetMenu(hWnd),0,&rc);
        len += rc.right - rc.left; //button lenght
    }
    return len;
}

void GLWindow_Resize(int x, int y, int usezoom)
{
    RECT rc;
    GetWindowRect(hWnd, &rc);
    int top = rc.top; //Keep old upper-left corner coordinates
    int left = rc.left;
    if(usezoom)
    {
        if(GLWindow_GetMenuLenghtPixels() > x*ZOOM) //don't let the menu use 2 lines
            rc.right = rc.left + GLWindow_GetMenuLenghtPixels();
        else
            rc.right = rc.left + x*ZOOM;
        rc.bottom = rc.top + y*ZOOM + GetSystemMetrics(SM_CYMENU);
        AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle); //Get real dimensions (including border, etc)

        rc.right = rc.right-rc.left+left; //Keep old upper-left corner coordinates
        rc.left = left;
        rc.bottom = rc.bottom-rc.top+top;
        rc.top = top;

        MoveWindow(hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
        glViewport(0,0, x*ZOOM,y*ZOOM);
    }
    else
    {
        if(GLWindow_GetMenuLenghtPixels() > x) //don't let the menu use 2 lines
            rc.right = rc.left + GLWindow_GetMenuLenghtPixels();
        else
            rc.right = rc.left + x;
        rc.bottom = rc.top + y + GetSystemMetrics(SM_CYMENU);
        AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle); //Get real dimensions (including border, etc)

        rc.right = rc.right-rc.left+left; //Keep old upper-left corner coordinates
        rc.left = left;
        rc.bottom = rc.bottom-rc.top+top;
        rc.top = top;

        MoveWindow(hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
        glViewport(0,0, x,y);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,x, y,0, -1,1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    InvalidateRect(NULL, NULL, TRUE);
    DrawMenuBar(hWnd);
    UpdateWindow(hWnd);
}

void GLWindow_ChangeScreen(int type)
{
    if(type == SCR_SGB) GLWindow_Resize(256,224, 1);
    else if(type == SCR_GB) GLWindow_Resize(160,144, 1);
    else //if(type == SCR_GBA)
    GLWindow_Resize(240,160, 1);
}

void GLWindow_SetZoom(int zoom)
{
    ZOOM = zoom;
}

void GLWindow_Kill(void) // Properly Kill The Window
{
	if(hRC) // Do We Have A Rendering Context?
	{
		if(!wglMakeCurrent(NULL,NULL)) // Are We Able To Release The DC And RC Contexts?
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);

		if(!wglDeleteContext(hRC)) // Are We Able To Delete The RC?
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);

		hRC=NULL;
	}

	if(hDC && !ReleaseDC(hWnd,hDC)) // Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;
	}

	if(hWnd && !DestroyWindow(hWnd)) // Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;
	}

	if(!UnregisterClass("OpenGL",hInstance)) // Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;
	}
}

int GLWindow_Create(void)
{
    if(ZOOM < 1 || ZOOM > 3) ZOOM = 2;

	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC)GLWindow_Process;			// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON)); // Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if(!RegisterClass(&wc))
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE; // Window Extended Style
    dwStyle = //WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX; // Windows Style
        WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	// Create The Window
	if(!(hWnd=CreateWindowEx(dwExStyle, // Extended Style For The Window
            "OpenGL","GiiBiiAdvance", // Class Name,Window Title
			dwStyle | // Defined Window Style
			WS_CLIPSIBLINGS | WS_CLIPCHILDREN, // Required Window Style
			CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT, //Window Position,Width,Height
			NULL,NULL, // No Parent Window, Menu
			hInstance, // Instance
			NULL))) // Dont Pass Anything To WM_CREATE
	{
		GLWindow_Kill(); // Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	hWndMain = hWnd; //Save for the rest of the windows

	RECT rc;
    GetWindowRect(hWnd, &rc);
    if(rc.top < 100) rc.top += 100;//to prevent the window from being too near the upper-left corner
    if(rc.left < 100) rc.left += 100; //of the screen
    rc.right = rc.left + 240*ZOOM;
    rc.bottom = rc.top + 160*ZOOM + GetSystemMetrics(SM_CYMENU);
    AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
    MoveWindow(hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);

	static PIXELFORMATDESCRIPTOR pfd = { // pfd Tells Windows How We Want Things To Be
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		32,								            // Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if(!(hDC=GetDC(hWnd))) // Did We Get A Device Context?
	{
		GLWindow_Kill(); // Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!(PixelFormat=ChoosePixelFormat(hDC,&pfd))) // Did Windows Find A Matching Pixel Format?
	{
		GLWindow_Kill(); // Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd)) // Are We Able To Set The Pixel Format?
	{
		GLWindow_Kill();// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!(hRC=wglCreateContext(hDC))) // Are We Able To Get A Rendering Context?
	{
		GLWindow_Kill(); // Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!wglMakeCurrent(hDC,hRC)) // Try To Activate The Rendering Context
	{
		GLWindow_Kill(); // Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	SetForegroundWindow(hWnd);

    GLWindow_CreateMenu(hWnd);
    GLWindow_MenuEnableROMCommands(0);

    ShowWindow(hWnd,SW_SHOWDEFAULT);

    //Set up OpenGL graphics...

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

	glClearColor(0,0,0, 0);
    glViewport(0,0, 240*ZOOM,160*ZOOM);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho(0,240, 160,0, -1,1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	return TRUE;
}


