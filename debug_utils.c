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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "general_utils.h"
#include "config.h"

#include "gui/win_main.h"

//----------------------------------------------------------------------------------

FILE * f_log;

void Debug_End(void)
{
    fclose(f_log);
}

void Debug_Init(void)
{
    f_log = fopen("log.txt","w");
    atexit(Debug_End);
}

void Debug_LogMsgArg(const char * msg, ...)
{
    va_list args;
    va_start(args,msg);
    vfprintf(f_log, msg, args);
    va_end(args);
    fputc('\n',f_log);
    return;
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

char console_buffer[5000];

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
