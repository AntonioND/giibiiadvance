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
#include <stdio.h>
#include <gl/gl.h>

#include "build_options.h"
#include "resource.h"

#include "main.h"
#include "config.h"
#include "gui_main.h"

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

static int sysstatuscreated = 0;
static HWND hWndSysStatus;

char sysstatustext[40000];

static void SystemStatusPrintInfo(void)
{
    memset(sysstatustext,0,sizeof(sysstatustext)); //Reset string
    char temp[10000];
    //-----------------------------------------------------------------
    OSVERSIONINFOEX ovi;
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&ovi);
    char * plataformidstrings[3] = {
        "VER_PLATFORM_WIN32s","VER_PLATFORM_WIN32_WINDOWS","VER_PLATFORM_WIN32_NT"
    };
    char * plataformid;
    if(ovi.dwPlatformId < 3) plataformid = plataformidstrings[ovi.dwPlatformId];
    else plataformid = "Unknown";

    char * producttypestrings[3] = {
        "VER_NT_WORKSTATION","VER_NT_DOMAIN_CONTROLLER","VER_NT_SERVER"
    };
    char * producttypes;
    if(ovi.wProductType < 3 && ovi.wProductType > 0) producttypes = producttypestrings[ovi.wProductType];
    else producttypes = "Unknown";

    sprintf(temp,
        "OSVERSIONINFOEX\r\n"
        "Major Version      = 0x%08X\r\n"
        "Minor Version      = 0x%08X\r\n"
        "Build Number       = 0x%08X\r\n"
        "Plataform ID       = %s (0x%08X)\r\n"
        "CSDVersion         = %s\r\n"
        "Service Pack Major = 0x%08X\r\n"
        "Service Pack Minor = 0x%08X\r\n"
        "Suite Mask         = 0x%08X\r\n"
        "Product type       = %s (0x%02X)\r\n",
        (unsigned int)ovi.dwMajorVersion,(unsigned int)ovi.dwMinorVersion,(unsigned int)ovi.dwBuildNumber,
        plataformid,(unsigned int)ovi.dwPlatformId,ovi.szCSDVersion,(unsigned int)ovi.wServicePackMajor,
        (unsigned int)ovi.wServicePackMinor,(unsigned int)ovi.wSuiteMask,producttypes,
        (unsigned int)ovi.wProductType
    );
    strncat(sysstatustext,temp,sizeof(sysstatustext));
    //-----------------------------------------------------------------
    strncat(sysstatustext,"WINDOWS VERSION    = ",sizeof(sysstatustext));
    if(ovi.dwMajorVersion == 5)
    {
        if(ovi.dwMinorVersion == 0)
            strncat(sysstatustext,"Windows 2000",sizeof(sysstatustext));
        else if(ovi.dwMinorVersion == 1)
            strncat(sysstatustext,"Windows XP",sizeof(sysstatustext));
        else if(ovi.dwMinorVersion == 2)
        {
            if(GetSystemMetrics(SM_SERVERR2))
                strncat(sysstatustext,"Windows Server 2003 R2",sizeof(sysstatustext));
            else
                strncat(sysstatustext,"Windows Server 2003",sizeof(sysstatustext));
        }
        else
            strncat(sysstatustext,"Unknown",sizeof(sysstatustext));
    }
    else if(ovi.dwMajorVersion == 6)
    {
        if(ovi.dwMinorVersion == 0)
        {
            if(ovi.wProductType == VER_NT_WORKSTATION)
                strncat(sysstatustext,"Windows Vista",sizeof(sysstatustext));
            else
                strncat(sysstatustext,"Windows Server 2008",sizeof(sysstatustext));
        }
        else if(ovi.dwMinorVersion == 1)
        {
            if(ovi.wProductType == VER_NT_WORKSTATION)
                strncat(sysstatustext,"Windows 7",sizeof(sysstatustext));
            else
                strncat(sysstatustext,"Windows Server 2008 R2",sizeof(sysstatustext));
        }
        else
            strncat(sysstatustext,"Unknown",sizeof(sysstatustext));
    }
    else
    {
        strncat(sysstatustext,"Unknown",sizeof(sysstatustext));
    }
    strncat(sysstatustext,"\r\n\r\n",sizeof(sysstatustext));
    //-----------------------------------------------------------------
    MEMORYSTATUS memstatus;
    memstatus.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&memstatus);
    sprintf(temp,
        "MEMORYSTATUS\r\n"
        "Memory Load = %u%%\r\n"
        "Physical    = %u/%u MB (%u%% free)\r\n"
        "PageFile    = %u/%u MB (%u%% free)\r\n"
        "Virtual     = %u/%u MB (%u%% free)\r\n"
        "\r\n",
        (unsigned int)memstatus.dwMemoryLoad,
        (unsigned int)(memstatus.dwAvailPhys/(1024*1024)),(unsigned int)(memstatus.dwTotalPhys/(1024*1024)),
        (unsigned int)(((float)memstatus.dwAvailPhys/(float)memstatus.dwTotalPhys)*100.0),
        (unsigned int)(memstatus.dwAvailPageFile/(1024*1024)),(unsigned int)(memstatus.dwTotalPageFile/(1024*1024)),
        (unsigned int)(((float)memstatus.dwAvailPageFile/(float)memstatus.dwTotalPageFile)*100.0),
        (unsigned int)(memstatus.dwAvailVirtual/(1024*1024)),(unsigned int)(memstatus.dwTotalVirtual/(1024*1024)),
        (unsigned int)(((float)memstatus.dwAvailVirtual/(float)memstatus.dwTotalVirtual)*100.0));
    strncat(sysstatustext,temp,sizeof(sysstatustext));
    //-----------------------------------------------------------------
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    char * processortype;
    if(sysinfo.dwProcessorType == PROCESSOR_INTEL_386) processortype = "INTEL_386";
    else if(sysinfo.dwProcessorType == PROCESSOR_INTEL_486) processortype = "INTEL_486";
    else if(sysinfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM) processortype = "INTEL_PENTIUM";
    else if(sysinfo.dwProcessorType == PROCESSOR_MIPS_R4000) processortype = "MIPS_R4000";
    else if(sysinfo.dwProcessorType == PROCESSOR_ALPHA_21064) processortype = "ALPHA_21064";
    else if(sysinfo.dwProcessorType == PROCESSOR_INTEL_IA64) processortype = "INTEL_IA64";
    else processortype = "UNKNOWN";
    char * arch_text[11] = {
        "INTEL", "MIPS", "ALPHA", "PPC", "SHX","ARM 5","IA64","ALPHA64","MSIL","AMD64","IA32_ON_WIN64"
    };
    char * processorarch = sysinfo.wProcessorArchitecture < 11 ?
            arch_text[sysinfo.wProcessorArchitecture]: "UNKNOWN";
    char processorflags[100];
    itoa(sysinfo.dwActiveProcessorMask,processorflags,2); //write in binary (base 2)
    sprintf(temp,
        "SYSTEM_INFO\r\n"
        "Page Size              = %u B\r\n"
        "Allocation Granularity = %u B\r\n"
        "Number of Processors   = %u\r\n"
        "Active Processor Mask  = %sb (%u)\r\n"
        "Processor Architecture = %s\r\n"
        "Processor Type         = %s\r\n"
        "Processor Revision     = %u\r\n"
        "\r\n",
        (unsigned int)sysinfo.dwPageSize,(unsigned int)sysinfo.dwAllocationGranularity,
        (unsigned int)sysinfo.dwNumberOfProcessors, processorflags, (unsigned int)sysinfo.dwActiveProcessorMask,
        processorarch, processortype,
        sysinfo.wProcessorRevision);
    strncat(sysstatustext,temp,sizeof(sysstatustext));
    //-----------------------------------------------------------------
    SYSTEM_POWER_STATUS sps;
    GetSystemPowerStatus(&sps);
    char * batstring;
    if(sps.BatteryFlag==255)
    {
        batstring = "Unknown status";
    }
    else
    {
        if(sps.BatteryFlag&128) batstring = "No batery";
        else
        {
            if(sps.BatteryFlag&8) batstring = "Charging";
            else batstring = "Discharging";
        }
    }
    unsigned int batlife = sps.BatteryLifeTime;
    unsigned int fullbatlife = sps.BatteryFullLifeTime;
    sprintf(temp,
        "SYSTEM_POWER_STATUS\r\n"
        "AC Line Status         = %s\r\n"
        "Battery Flag           = %s\r\n"
        "Battery Life Percent   = %u%%\r\n"
        "Battery Life Time      = %02u:%02u:%02u (0x%08X)\r\n"
        "Battery Full Life Time = %02u:%02u:%02u (0x%08X)\r\n"
        "\r\n",
        sps.ACLineStatus == 255 ? "Unknown" : (sps.ACLineStatus ? "Connected" : "Disconnected"),
        batstring,sps.BatteryLifePercent,
        batlife/3600, (batlife%3600)/60, batlife%60, (unsigned int)sps.BatteryLifeTime,
        fullbatlife/3600, (fullbatlife%3600)/60, fullbatlife%60, (unsigned int)sps.BatteryFullLifeTime);
    strncat(sysstatustext,temp,sizeof(sysstatustext));
    //-----------------------------------------------------------------
    sprintf(temp,
            "OTHER...\r\n"
            "GetSystemMetrics(SM_SLOWMACHINE) = %s\r\n"
            "GetCurrentProcessId() = 0x%08X\r\n"
            "IsDebuggerPresent() = %s\r\n" "\r\n",
        GetSystemMetrics(SM_SLOWMACHINE) ? "TRUE" : "FALSE",
        (unsigned int)GetCurrentProcessId(),
        IsDebuggerPresent() ? "TRUE" : "FALSE" );
    strncat(sysstatustext,temp,sizeof(sysstatustext));
    //-----------------------------------------------------------------
    sprintf(temp,
        "OPENGL\r\n"
        "GL_RENDERER   = %s\r\n"
        "GL_VERSION    = %s\r\n"
        "GL_VENDOR     = %s\r\n"
        "GL_EXTENSIONS = %s\r\n"
        "\r\n",
        (char*)glGetString(GL_RENDERER),
        (char*)glGetString(GL_VERSION),
        (char*)glGetString(GL_VENDOR),
        (char*)glGetString(GL_EXTENSIONS));
    strcat(sysstatustext,temp);
    //-----------------------------------------------------------------
    strncat(sysstatustext,"END OF REPORT\r\n",sizeof(sysstatustext));
}

static LRESULT CALLBACK SystemStatusProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont;

    switch(Msg)
    {
        case WM_CREATE:
        {
            SystemStatusPrintInfo();

            HWND hText = CreateWindow(TEXT("edit"), TEXT(sysstatustext),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | WS_VSCROLL | ES_READONLY,
                        2, 2, 590, 370, hWnd, NULL, hInstance, NULL);
            hFont = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, FIXED_PITCH, NULL);
            SendMessage(hText, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            break;
        }
        case WM_DESTROY:
            sysstatuscreated = 0;
            DeleteObject(hFont);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

void GLWindow_ShowSystemStatus(void)
{
    if(sysstatuscreated) { SetActiveWindow(hWndSysStatus); return; }
    sysstatuscreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = SystemStatusProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_SystemStatus";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_SystemStatus", "System Status", WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT, CW_USEDEFAULT, 600,405, hWndMain, NULL, hInstance, NULL);

	if(!hWnd) { sysstatuscreated = 0; return; }

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndSysStatus = hWnd;
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

static int aboutcreated = 0;
static HWND hWndAbout;

extern const char * about_text;

static LRESULT CALLBACK AboutProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont, hFontTitle;

    switch(Msg)
    {
        case WM_CREATE:
        {
            HWND hTextTitle = CreateWindow(TEXT("static"),TEXT("GiiBiiAdvance v" GIIBIIADVANCE_VERSION_STRING),
                        WS_CHILD | WS_VISIBLE | ES_CENTER,
                        192, 5, 220, 30, hWnd, NULL, hInstance, NULL);
            hFontTitle = CreateFont(20,0,0,0, FW_BOLD, 0, TRUE, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, FIXED_PITCH, NULL);
            SendMessage(hTextTitle, WM_SETFONT, (WPARAM)hFontTitle, MAKELPARAM(1, 0));

            HWND hText = CreateWindow(TEXT("edit"),TEXT(about_text),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | WS_VSCROLL | ES_READONLY,
                        2, 40, 590, 500, hWnd, NULL, hInstance, NULL);
            hFont = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, DEFAULT_PITCH, NULL);
            SendMessage(hText, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            break;
        }
        case WM_DESTROY:
            aboutcreated = 0;
            DeleteObject(hFont);
            DeleteObject(hFontTitle);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

void GLWindow_ShowAbout(void)
{
    if(aboutcreated) { SetActiveWindow(hWndAbout); return; }
    aboutcreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = AboutProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_About";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_About", "About", WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT, CW_USEDEFAULT, 600,575, hWndMain, NULL, hInstance, NULL);

	if(!hWnd) { aboutcreated = 0; return; }

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndAbout = hWnd;
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

static int licensecreated = 0;
static HWND hWndLicense;

extern const char * license_text;

static LRESULT CALLBACK LicenseProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont;

    switch(Msg)
    {
        case WM_CREATE:
        {
            HWND hText = CreateWindow(TEXT("edit"), TEXT(license_text),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | WS_VSCROLL | ES_READONLY,
                        2, 2, 590, 530, hWnd, NULL, hInstance, NULL);
            hFont = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, DEFAULT_PITCH, NULL);
            SendMessage(hText, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            break;
        }
        case WM_DESTROY:
            licensecreated = 0;
            DeleteObject(hFont);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

void GLWindow_ShowLicense(void)
{
    if(licensecreated) { SetActiveWindow(hWndLicense); return; }
    licensecreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = LicenseProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_License";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_License", "License", WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT, CW_USEDEFAULT, 600,565, hWndMain, NULL, hInstance, NULL);

	if(!hWnd) { licensecreated = 0; return; }

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndLicense = hWnd;
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

static int readmecreated = 0;
static HWND hWndReadme;

extern const char * readme_text;

static LRESULT CALLBACK ReadmeProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont;

    switch(Msg)
    {
        case WM_CREATE:
        {
            HWND hText = CreateWindow(TEXT("edit"), TEXT(readme_text),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | WS_VSCROLL | ES_READONLY,
                        2, 2, 590, 530, hWnd, NULL, hInstance, NULL);
            hFont = CreateFont(15,0,0,0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY, DEFAULT_PITCH, NULL);
            SendMessage(hText, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
            break;
        }
        case WM_DESTROY:
            readmecreated = 0;
            DeleteObject(hFont);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

void GLWindow_ShowReadme(void)
{
    if(readmecreated) { SetActiveWindow(hWndReadme); return; }
    readmecreated = 1;

    HWND    hWnd;
	WNDCLASSEX  WndClsEx;

	// Create the application window
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = ReadmeProcedure;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = "Class_Readme";
	WndClsEx.hInstance     = hInstance;
	WndClsEx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MY_ICON));

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the window object
	hWnd = CreateWindow("Class_Readme", "Readme", WS_BORDER | WS_CAPTION | WS_SYSMENU,
			  CW_USEDEFAULT, CW_USEDEFAULT, 600,565, hWndMain, NULL, hInstance, NULL);

	if(!hWnd) { readmecreated = 0; return; }

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	hWndReadme = hWnd;
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

