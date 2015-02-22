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
#include <SDL2/SDL_opengl.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "general_utils.h"
#include "config.h"
#include "build_options.h"
#include "file_utils.h"

#include "gui/win_main.h"

//----------------------------------------------------------------------------------

static FILE * f_log;
static int log_file_opened = 0;

void Debug_End(void)
{
    if(log_file_opened)
        fclose(f_log);

    log_file_opened = 0;
}

void Debug_Init(void)
{
    log_file_opened = 0;
    atexit(Debug_End);

    // remove previous log files
    char logpath[MAX_PATHLEN];
    s_snprintf(logpath,sizeof(logpath),"%slog.txt",DirGetRunningPath());
    if(FileExists(logpath))
    {
        remove(logpath); // if returns 0, ok
    }
}

void Debug_LogMsgArg(const char * msg, ...)
{
    if(log_file_opened == 0)
    {
        char logpath[MAX_PATHLEN];
        s_snprintf(logpath,sizeof(logpath),"%slog.txt",DirGetRunningPath());
        f_log = fopen(logpath,"w");
        if(f_log)
            log_file_opened = 1;
    }

    if(log_file_opened)
    {
        va_list args;
        va_start(args,msg);
        vfprintf(f_log, msg, args);
        va_end(args);
        fputc('\n',f_log);
    }
}

void Debug_DebugMsgArg(const char * msg, ...)
{
    char dest[2000];
    va_list args;
    va_start(args,msg);
    vsnprintf(dest, sizeof(dest), msg, args);
    va_end(args);
    dest[sizeof(dest)-1] = '\0';

    if(EmulatorConfig.debug_msg_enable == 0) return;

    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Debug", dest, NULL);
    Win_MainShowMessage(1,dest);

    return;
}

void Debug_ErrorMsgArg(const char * msg, ...)
{
    char dest[2000];
    va_list args;
    va_start(args,msg);
    vsnprintf(dest, sizeof(dest), msg, args);
    va_end(args);
    dest[sizeof(dest)-1] = '\0';

    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GiiBiiAdvance - Error", dest, NULL);
    Win_MainShowMessage(0,dest);

    return;
}

void Debug_DebugMsg(const char * msg)
{
    if(EmulatorConfig.debug_msg_enable == 0) return;

    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Debug", msg, NULL);
    Win_MainShowMessage(1,msg);

    return;
}

void Debug_ErrorMsg(const char * msg)
{
    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Error", msg, NULL);
    Win_MainShowMessage(0,msg);

    return;
}

//----------------------------------------------------------------------------------

static char console_buffer[5000];

void ConsoleReset(void)
{
    console_buffer[0] = '\0';
}

void ConsolePrint(const char * msg, ...)
{
    va_list args;
    char buffer[1024];

    va_start(args,msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);

    s_strncat(console_buffer,buffer,sizeof(console_buffer));
}

void ConsoleShow(void)
{
    Win_MainShowMessage(2,console_buffer);
    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Console", console_buffer, NULL);
}

//----------------------------------------------------------------------------------

static char sys_info_buffer[10000];

static void _sys_info_printf(const char * msg, ...)
{
    va_list args;
    char buffer[1000];

    va_start(args,msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);

    s_strncat(sys_info_buffer,buffer,sizeof(sys_info_buffer));
}

static void _sys_info_print(const char * msg)
{
    s_strncat(sys_info_buffer,msg,sizeof(sys_info_buffer));
}

static void _sys_info_reset(void)
{
    memset(sys_info_buffer,0,sizeof(sys_info_buffer));

    _sys_info_printf("SDL information:\n"
                     "----------------\n"
                     "\n"
                     "SDL_GetPlatform(): %s\n\n"
                     "SDL_GetCPUCount(): %d (Number of logical CPU cores)\n"
#if SDL_VERSION_ATLEAST(2,0,1)
                     "SDL_GetSystemRAM(): %d MB\n"
#endif
                     "SDL_GetCPUCacheLineSize(): %d kB (Cache L1)\n\n",
                     SDL_GetPlatform(), SDL_GetCPUCount(),
#if SDL_VERSION_ATLEAST(2,0,1)
                     SDL_GetSystemRAM(),
#endif
                     SDL_GetCPUCacheLineSize() );

    int total_secs, pct;
    SDL_PowerState st = SDL_GetPowerInfo(&total_secs,&pct);
    char * st_string;
    switch(st)
    {
        default:
        case SDL_POWERSTATE_UNKNOWN:
            st_string = "SDL_POWERSTATE_UNKNOWN (cannot determine power status)";
            break;
        case SDL_POWERSTATE_ON_BATTERY:
            st_string = "SDL_POWERSTATE_ON_BATTERY (not plugged in, running on battery)";
            break;
        case SDL_POWERSTATE_NO_BATTERY:
            st_string = "SDL_POWERSTATE_NO_BATTERY (plugged in, no battery available)";
            break;
        case SDL_POWERSTATE_CHARGING:
            st_string = "SDL_POWERSTATE_CHARGING (plugged in, charging battery)";
            break;
        case SDL_POWERSTATE_CHARGED:
            st_string = "SDL_POWERSTATE_CHARGED (plugged in, battery charged)";
            break;
    }

    unsigned int hours = ((unsigned int)total_secs)/3600;
    unsigned int min = (((unsigned int)total_secs)-(hours*3600))/60;
    unsigned int secs = (((unsigned int)total_secs)-(hours*3600)-(min*60));

    _sys_info_printf("SDL_GetPowerInfo():\n  %s\n  Time left: %d:%02d:%02d\n  Percentage: %3d%%\n\n",
                     st_string,hours,min,secs,pct);
#ifdef ENABLE_OPENGL
    _sys_info_printf("OpenGL information:\n"
                     "-------------------\n"
                     "\n"
                     "GL_RENDERER   = %s\n"
                     "GL_VERSION    = %s\n"
                     "GL_VENDOR     = %s\n"
                     "GL_EXTENSIONS = ",
                     (char*)glGetString(GL_RENDERER),(char*)glGetString(GL_VERSION),
                     (char*)glGetString(GL_VENDOR));
    _sys_info_print((char*)glGetString(GL_EXTENSIONS));
#endif
    _sys_info_printf("\n\nEND LOG\n");
}

void SysInfoShow(void)
{
    _sys_info_reset();

    Win_MainShowMessage(3,sys_info_buffer);
}

//----------------------------------------------------------------------------------
