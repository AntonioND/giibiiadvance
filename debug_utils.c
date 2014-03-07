
#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "general_utils.h"

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

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Debug", dest, NULL);

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

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GiiBiiAdvance - Error", dest, NULL);

    return;
}

void Debug_DebugMsg(const char * msg)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Debug", msg, NULL);

    return;
}

void Debug_ErrorMsg(const char * msg)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Error", msg, NULL);

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
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "GiiBiiAdvance - Console", console_buffer, NULL);
}

//----------------------------------------------------------------------------------
