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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#include "build_options.h"

#include "gui_main.h"
#include "config.h"
#include "gui_mainloop.h"
#include "gba_core/gba.h"
#include "gba_core/cpu.h"
#include "gba_core/interrupts.h"
#include "gba_core/video.h"
#include "gba_core/memory.h"
#include "gba_core/sound.h"
#include "gb_core/sound.h"

//--------------------------------------------------
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

    strcat(console_buffer,buffer);
}

void ConsoleShow(void)
{
    MessageBox(NULL, console_buffer, "Console", MB_OK);
}


//-------------------------------------------------

char runningfolder[MAXPATHLEN];
char * GetRunningFolder(void)
{
    return runningfolder;
}

void memset_rand(u8 * start, u32 size)
{
	while(size--) *start++ = rand();
}

u64 asciihextoint(const char * text)
{
    long int value = 0, i = 0;
    while(1)
    {
        char char_ = toupper(text[i++]); //end of string
        if(char_ == '\0') return value;
        else if(char_ >= '0' && char_ <= '9') value = (value <<4) + (char_ - '0');
        else if(char_ >= 'A' && char_ <= 'F') value = (value <<4) +  (char_ - 'A' + 10);
        else return 0xFFFFFFFFFFFFFFFFULL;
    }
}

u64 asciidectoint(const char * text)
{
    long int value = 0, i = 0;
    while(1)
    {
        char char_ = toupper(text[i++]); //end of string
        if(char_ == '\0') return value;
        else if(char_ >= '0' && char_ <= '9') value = (value*10) + (char_ - '0');
        else return 0xFFFFFFFFFFFFFFFFULL;
    }
}

int MESSAGE_SHOWING = 0;

void DebugMessage(const char * msg, ...)
{
    va_list args;
	char buffer[1024];

	va_start(args,msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);

    if(EmulatorConfig.debug_msg_enable) {
        MESSAGE_SHOWING = 1;
        MessageBox(NULL, buffer, "Debug", MB_OK);
        MESSAGE_SHOWING = 0;
    }
}

void ErrorMessage(const char * msg, ...)
{
    va_list args;
	char buffer[1024];

	va_start(args,msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);

    MESSAGE_SHOWING = 1;
    MessageBox(NULL, buffer, "Error", MB_OK|MB_ICONSTOP);
    MESSAGE_SHOWING = 0;
}


void FileLoad_NoError(const char * filename, void ** buffer, unsigned int * size_)
{
	FILE * datafile = fopen(filename, "rb");
	unsigned int size;
	*buffer = NULL;
	if(size_) *size_ = 0;

	if(datafile == NULL) return;

    fseek(datafile, 0, SEEK_END);
	size = ftell(datafile);
	if(size_) *size_ = size;

	if(size == 0) return;

	rewind(datafile);
	*buffer = malloc(size);

	if(*buffer == NULL)
	{
        fclose(datafile);
        return;
    }
	if(fread(*buffer,size,1,datafile) != 1)
	{
        fclose(datafile);
        return;
    }

	fclose(datafile);
}


void FileLoad(const char * filename, void ** buffer, unsigned int * size_)
{
	FILE * datafile = fopen(filename, "rb");
	unsigned int size;
	*buffer = NULL;
	if(size_) *size_ = 0;

	if(datafile == NULL)
	{
	    char msg[2048];
	    sprintf(msg,"%s couldn't be opened!",filename);
	    MessageBox(NULL, msg, "File open error", MB_OK);
        return;
    }

    fseek(datafile, 0, SEEK_END);
	size = ftell(datafile);
	if(size_) *size_ = size;
	if(size == 0)
	{
	    char msg[2048];
	    sprintf(msg,"Size of %s is 0!",filename);
	    MessageBox(NULL, msg, "File open error", MB_OK);
        fclose(datafile);
        return;
    }
	rewind(datafile);
	*buffer = malloc(size);
	if(*buffer == NULL)
	{
	    char msg[2048];
	    sprintf(msg,"Not enought memory to load %s!",filename);
	    MessageBox(NULL, msg, "File open error", MB_OK);
        fclose(datafile);
        return;
    }
	if(fread(*buffer,size,1,datafile) != 1)
	{
	    MessageBox(NULL, "Error while reading.", "File open error", MB_OK);
        fclose(datafile);
        return;
    }

	fclose(datafile);
}

//-------------------------------------------------

int DirCheckExistence(char * path)
{
    struct stat s;
    int err = stat(path, &s);
    if(err == -1)
    {
        if(errno == ENOENT)
        {
            return 0; // does not exist
        }
        else
        {
            //perror("stat"); // stat error
            //exit(1);
            return 0;
        }
    }
    else
    {
        if(S_ISDIR(s.st_mode))
        {
            return 1; // it's a dir
        }
        else
        {
            ErrorMessage("Checking for existence of directory, but it is a file: %s",path);
            return 0; // exists but is no dir
        }
    }
}

int DirCreate(char * path)
{
    //if(mkdir(path, S_IRWXU|S_IRGRP|S_IXGRP) == 0)
    if(mkdir(path) == 0)
    {
        return 1;
    }
    else
    {
        ErrorMessage("Couldn't create directory: %s",path);
        return 0;
    }
}

//---------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------

#define	REPLAY_RATE				22050
#define	REPLAY_DEPTH			16
#define	REPLAY_CHANNELS     	2 // Dual
#define	REPLAY_SAMPLELEN		((REPLAY_DEPTH/8)*REPLAY_CHANNELS)
#define	REPLAY_NBSOUNDBUFFER    3 // Triple buffer

typedef void (*USER_CALLBACK) (void *pBuffer,long bufferLen);

typedef struct{
    int m_bServerRunning;
    HWND m_hWnd;
    long m_bufferSize;
    long m_currentBuffer;
    HWAVEOUT m_hWaveOut;
    WAVEHDR m_waveHeader[REPLAY_NBSOUNDBUFFER];
    void * m_pSoundBuffer[REPLAY_NBSOUNDBUFFER];
    USER_CALLBACK m_pUserCallback;
} CSoundServer;

CSoundServer Server;

void CSoundServer_fillNextBuffer(void)
{
    // check if the buffer is already prepared (should not !)
    if(Server.m_waveHeader[Server.m_currentBuffer].dwFlags&WHDR_PREPARED)
        waveOutUnprepareHeader(Server.m_hWaveOut,&Server.m_waveHeader[Server.m_currentBuffer],sizeof(WAVEHDR));

    // Call the user function to fill the buffer with anything you want ! :-)
    if(Server.m_pUserCallback)
        Server.m_pUserCallback(Server.m_pSoundBuffer[Server.m_currentBuffer],Server.m_bufferSize);

    // Prepare the buffer to be sent to the WaveOut API
    Server.m_waveHeader[Server.m_currentBuffer].lpData = (char*)Server.m_pSoundBuffer[Server.m_currentBuffer];
    Server.m_waveHeader[Server.m_currentBuffer].dwBufferLength = Server.m_bufferSize;
    waveOutPrepareHeader(Server.m_hWaveOut,&Server.m_waveHeader[Server.m_currentBuffer],sizeof(WAVEHDR));

    // Send the buffer the the WaveOut queue
    waveOutWrite(Server.m_hWaveOut,&Server.m_waveHeader[Server.m_currentBuffer],sizeof(WAVEHDR));

    Server.m_currentBuffer++;
    if(Server.m_currentBuffer >= REPLAY_NBSOUNDBUFFER) Server.m_currentBuffer = 0;
}

// Internal WaveOut API callback function. We just call our sound handler ("playNextBuffer")
static void CALLBACK waveOutProc(HWAVEOUT hwo,UINT uMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2)
{
    if(uMsg == WOM_DONE) CSoundServer_fillNextBuffer();
}

void CSoundServer_close(void) //Always hangs the emulator...
{
    if(Server.m_bServerRunning)
    {
        Server.m_pUserCallback = NULL;
        waveOutReset(Server.m_hWaveOut);
        int i;
        for(i=0;i<REPLAY_NBSOUNDBUFFER;i++)
        {
            if(Server.m_waveHeader[Server.m_currentBuffer].dwFlags & WHDR_PREPARED)
                waveOutUnprepareHeader(Server.m_hWaveOut,&Server.m_waveHeader[i],sizeof(WAVEHDR));
            free(Server.m_pSoundBuffer[i]);
        }
        waveOutClose(Server.m_hWaveOut);
        Server.m_bServerRunning = FALSE;
    }
}

int CSoundServer_open(USER_CALLBACK pUserCallback,long totalBufferedSoundLen)
{
    CSoundServer_close();

    //Server.m_pUserCallback = NULL;
    Server.m_bServerRunning = 0;
    Server.m_currentBuffer = 0;

    Server.m_pUserCallback = pUserCallback;
    Server.m_bufferSize = ((totalBufferedSoundLen * REPLAY_RATE) / 1000) * REPLAY_SAMPLELEN;
    Server.m_bufferSize /= REPLAY_NBSOUNDBUFFER;

    WAVEFORMATEX wfx;
    wfx.wFormatTag = 1; // PCM standart.
    wfx.nChannels = REPLAY_CHANNELS;
    wfx.nSamplesPerSec = REPLAY_RATE;
    wfx.nAvgBytesPerSec = REPLAY_RATE*REPLAY_SAMPLELEN*REPLAY_CHANNELS;
    wfx.nBlockAlign = REPLAY_SAMPLELEN;
    wfx.wBitsPerSample = REPLAY_DEPTH;
    wfx.cbSize = 0;
    MMRESULT errCode = waveOutOpen(&Server.m_hWaveOut,WAVE_MAPPER,&wfx,(DWORD)waveOutProc,
        (DWORD)&Server, // User data.
        (DWORD)CALLBACK_FUNCTION);

    if(errCode != MMSYSERR_NOERROR) return 0;

    // Alloc the sample buffers.
    int i;
    for(i=0;i<REPLAY_NBSOUNDBUFFER;i++)
    {
        Server.m_pSoundBuffer[i] = malloc(Server.m_bufferSize);
        memset(&Server.m_waveHeader[i],0,sizeof(WAVEHDR));
    }

    // Fill all the sound buffers
    Server.m_currentBuffer = 0;
    for(i=0;i<REPLAY_NBSOUNDBUFFER;i++)
        CSoundServer_fillNextBuffer();

    Server.m_bServerRunning = 1;
    return 1;
}

int sndenabled = 1;

//#include <math.h>
void SoundCallback(void * pSoundBuffer,long bufferLen)
{
/*
#define	TWO_PI			(3.1415926f * 2.f)
#define	SIN_STEP		((TWO_PI * 440.f) / 44100.f)
static	float			sinPos = 0.f;

    // Convert params, assuming we create a 16bits, mono waveform.
    signed short *pSample = (signed short*)pSoundBuffer;
    long	nbSample = bufferLen / sizeof(signed short);
    int i;
    for ( i=0;i<nbSample;i+=2)
    {
        *pSample++ = (signed short)(16384.f * sin(sinPos));
        *pSample++ = (signed short)(16384.f * sin(sinPos*3));
        sinPos += SIN_STEP;
    }
    if(sinPos >= TWO_PI) sinPos -= TWO_PI;
    return;
*/
    if(sndenabled && (Keys_Down[VK_SPACE] == 0) && (PAUSED == 0))
    {
        if(RUNNING == RUN_GBA)
        {
            GBA_SoundCallback(pSoundBuffer,bufferLen);
            return;
        }
        else if(RUNNING == RUN_GB)
        {
            GB_SoundCallback(pSoundBuffer,bufferLen);
            return;
        }
    }

    memset(pSoundBuffer,0,bufferLen);
}

void SoundMasterEnable(int enable)
{
    sndenabled = enable;
}

//---------------------------------------------------------------------------------------

static void atexit_fn(void)
{
    GLWindow_UnloadRom(1); //just in case, shouldn't be needed: it is called after exiting mainloop
    Config_Save(); //THIS IS NEEDED
}

#include "splash.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
    RUNNING = RUN_NONE;

    //GetCommandLine() -> Path (in quotes) + Arguments
    //lpCmdLine -> Arguments

    //Get bios folder
    //---------------
    strcpy(biospath,GetCommandLine());

    //For WINE compatibility? ----------------------------
    char quotescharacter = '\0';
    if(biospath[0] == '\"') quotescharacter = '\"';
    else if(biospath[0] == '\'') quotescharacter = '\'';
    //----------------------------------------------------

    //get emulator folder
    int i = 1;
    if(biospath[0] == quotescharacter)
    {
        int len = strlen(biospath) - 1;
        int j;
        for(j = 0; j < len; j++) biospath[j] = biospath[j+1];
        while( (biospath[i++] != quotescharacter) && (i < MAXPATHLEN) );
    }
    else while( (biospath[i++] != ' ') && (i < MAXPATHLEN) );
    if( i == MAXPATHLEN )
    {
        MessageBox(NULL,"Error while parsing command line.","Command Line",MB_OK);
        return 0;
    }
    while( (biospath[i--] != '\\') && (i > 0) );
    biospath[i+1] = '\0';
    strcpy(runningfolder,biospath);

    Config_Load();

    strcat(biospath,"\\bios");

    atexit(atexit_fn);

    char screenshotpath[MAXPATHLEN];
    sprintf(screenshotpath,"%s\\screenshots",runningfolder);

    if(DirCheckExistence(screenshotpath) == 0) DirCreate(screenshotpath);
    if(DirCheckExistence(biospath) == 0) DirCreate(biospath);

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------

    if(!GLWindow_Create()) return 0;

#ifdef ENABLE_SPLASH
    GLWindow_SplashScreen();
#endif

    if(CSoundServer_open(SoundCallback,EmulatorConfig.server_buffer_len) == 0) ErrorMessage("Couldn't init sound system!");

    if(strlen(lpCmdLine) != 0)
    {
        char rompath[MAXPATHLEN];

        strcpy(rompath,lpCmdLine);

        if(rompath[0] == quotescharacter)
        {
            int len = strlen(rompath) - 2;
            int j;
            for(j = 0; j < len; j++) rompath[j] = rompath[j+1];
            rompath[j] = '\0';
        }

        GLWindow_LoadRom(rompath);
    }

    GLWindow_Mainloop();

	GLWindow_Kill();

    //CSoundServer_close(); //Crashes...

	return 0;
}

/*

void Debug_Show_WINAPI_LastError(void)
{
    LPTSTR errorText = NULL;

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR)&errorText,0,NULL);

    if(errorText != NULL)
    {
        DebugMessage(errorText);
        LocalFree(errorText);
    }
}

//----------------------------------

char texto[128], cad[64];
case WM_KEYDOWN:
GetKeyNameText(lParam, cad, 64);
sprintf(texto, "Tecla %s (%d) pulsada", cad, (int)wParam);
break;
*/

