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
#include <gl\gl.h>

#include "build_options.h"

#include "gui_main.h"
#include "gui_config.h"
#include "gui_mainloop.h"
#include "gui_gba_debugger.h"
#include "config.h"
#include "gba_core/gba.h"
#include "gba_core/cpu.h"
#include "gba_core/interrupts.h"
#include "gba_core/sound.h"
#include "gba_core/video.h"
#include "gba_core/memory.h"
#include "gb_core/gameboy.h"
#include "gb_core/gb_main.h"
#include "gb_core/cpu.h"
#include "gb_core/video.h"
#include "gb_core/interrupts.h"
#include "gb_core/gb_main.h"
#include "gb_core/sound.h"

//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------

int PAUSED = 0;

//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------

int USE_HIGH_PERFORMANCE_TIMER = 0;
int FRAMESKIP = 0;
int AUTO_FRAMESKIP_WAIT = 0;

int FPScounter, FPS;
VOID CALLBACK FPSTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
    if(RUNNING == RUN_GB) GB_HandleTime(); //The most important thing...

    //-----------------------------

    FPS = FPScounter;
    FPScounter = 0;
    char window_text[64];
    if(RUNNING == RUN_NONE)
        strcpy(window_text,"GiiBiiAdvance");
    else
    {
        if(FRAMESKIP)
            sprintf(window_text,"GiiBiiAdvance: %d fps (%d) - %.2f%%",FPS,FRAMESKIP,(float)FPS*10.0f/6.0f);
        else
            sprintf(window_text,"GiiBiiAdvance: %d fps - %.2f%%",FPS,(float)FPS*10.0f/6.0f);
    }
    GLWindow_SetCaption(window_text);

    //-----------------------------

    if(PAUSED == 0)
    {
        if(EmulatorConfig.frameskip == -1)
        {
            if(GLWindow_Active)
            {
                if(AUTO_FRAMESKIP_WAIT) // this is used because the first seconds of emulation FPS counter doesn't work right
                {
                    AUTO_FRAMESKIP_WAIT--;
                }
                else
                {
                    if(FPS < 58) FRAMESKIP ++;
                }
            }
            else AUTO_FRAMESKIP_WAIT = 2;
        }
        else FRAMESKIP = EmulatorConfig.frameskip;
    }
}

UINT timerID;
LARGE_INTEGER ticksPerSecond, ticksPerFrame, ticksPerMs, oldtime_high; // High performance timer
static unsigned int oldtime_normal; // Normal timer
void TimerInit(void)
{
    timerID = SetTimer(NULL,0,1000,(TIMERPROC)FPSTimerProc);

    if(USE_HIGH_PERFORMANCE_TIMER)
    {
        if(QueryPerformanceFrequency(&ticksPerSecond))
        {
            ticksPerFrame.QuadPart = ticksPerSecond.QuadPart / 60;
            ticksPerMs.QuadPart = ticksPerSecond.QuadPart / 1000;
            QueryPerformanceCounter(&oldtime_high);
        }
        else
        {
            ErrorMessage("Couldn't use a high performance timer:\nQueryPerformanceFrequency() error");
            USE_HIGH_PERFORMANCE_TIMER = 0;
        }

        return;
    }

    timeBeginPeriod(1);
    oldtime_normal = timeGetTime();
}

void TimerWait(int wait)
{
    if(wait)
    {
        if(USE_HIGH_PERFORMANCE_TIMER)
        {
            LARGE_INTEGER now;
            oldtime_high.QuadPart += ticksPerFrame.QuadPart;
            QueryPerformanceCounter(&now);

            int mstowait = (oldtime_high.QuadPart-now.QuadPart) / ticksPerMs.QuadPart;
            if(mstowait > 2) Sleep(mstowait-1);
            while( now.QuadPart < oldtime_high.QuadPart ) QueryPerformanceCounter(&now);

            oldtime_high.QuadPart = now.QuadPart;
        }
        else
        {
            while( (timeGetTime() - oldtime_normal) < 16 ) Sleep(1);
            while( (timeGetTime() - oldtime_normal) < ((FPS>60) ? 17 : 16) );
            oldtime_normal = timeGetTime();
        }
    }

    FPScounter ++;
}

void TimerEnd(void)
{
    if(USE_HIGH_PERFORMANCE_TIMER)
    {
    }
    else
    {
        timeEndPeriod(1);
    }

    KillTimer(NULL,timerID);
}

//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------

static inline void GLWindow_GBAHandleInput(void)
{
    u16 input = 0;
    if(Keys_Down['X'] == 0) input |= BIT(0);
    if(Keys_Down['Z'] == 0) input |= BIT(1);
    if(Keys_Down[VK_SHIFT] == 0) input |= BIT(2); //VK_RSHIFT
    if(Keys_Down[VK_RETURN] == 0) input |= BIT(3);
    if(Keys_Down[VK_RIGHT] == 0) input |= BIT(4);
    if(Keys_Down[VK_LEFT] == 0) input |= BIT(5);
    if(Keys_Down[VK_UP] == 0) input |= BIT(6);
    if(Keys_Down[VK_DOWN] == 0) input |= BIT(7);
    if(Keys_Down['S'] == 0) input |= BIT(8);
    if(Keys_Down['A'] == 0) input |= BIT(9);
    REG_KEYINPUT = input;
}

//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------

int RUNNING = RUN_NONE;
void * SCREEN_TEXTURE;

void GLWindow_Mainloop(void)
{
    USE_HIGH_PERFORMANCE_TIMER = EmulatorConfig.highperformancetimer;

    TimerInit();

    SCREEN_TEXTURE = calloc(256*224,4); //max possible size
    int scr_texture_loaded = 0;
    GLuint scr_texture;

    //do { GBA_RunFor(1); } while(GBA_MemoryReadFast16(CPU.R[R_PC]) != 0xDF05); //swi 0x05
    //CPU.R[R_PC] = 0x00000000;
    //do { GBA_RunFor(1); } while(CPU.R[R_PC] != 0x03000020); GLWindow_GBACreateDissasembler();

    while(1)
    {
        if(GLWindow_HandleEvents()) break;

        if(GLWindow_Active && (PAUSED == 0))
        {
            if(RUNNING == RUN_GBA)
            {
                GLWindow_GBADisassemblerStartAddressSetDefault();

                GLWindow_GBAHandleInput();
                GBA_CheckKeypadInterrupt();

                GBA_RunFor(280896); //clocksperframe = 280896

                if(Keys_Down[VK_SPACE])
                {
                    GBA_SetFrameskip(10);
                    GBA_SoundResetBufferPointers();
                }
                else GBA_SetFrameskip(FRAMESKIP);

                if(GBA_HasToSkipFrame()==0)
                {
                    GBA_ConvertScreenBufferTo32RGB(SCREEN_TEXTURE);

                    glClear(GL_COLOR_BUFFER_BIT); //Clear screen

                    if(scr_texture_loaded) glDeleteTextures(1,&scr_texture);
                    scr_texture_loaded = 1;

                    glGenTextures(1,&scr_texture);
                    glBindTexture(GL_TEXTURE_2D,scr_texture);
                    if(EmulatorConfig.oglfilter)
                    {
                        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                    }
                    else
                    {
                        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
                    }
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

                    glTexImage2D(GL_TEXTURE_2D,0,4,240,160, 0,GL_RGBA,GL_UNSIGNED_BYTE,SCREEN_TEXTURE);

                    glBindTexture(GL_TEXTURE_2D,scr_texture);
                    glColor3f(1.0,1.0,1.0);
                    glBegin( GL_QUADS );
                        glTexCoord2f(0,0); // Top-left vertex
                        glVertex3f(0,0,0);

                        glTexCoord2f(1,0); // Bottom-left vertex
                        glVertex3f(240,0,0);

                        glTexCoord2f(1,1); // Bottom-right vertex
                        glVertex3f(240,160,0);

                        glTexCoord2f(0,1); // Top-right vertex
                        glVertex3f(0,160,0);
                    glEnd();

                    GLWindow_SwapBuffers();
                }

                GBA_UpdateFrameskip();

                //GLWindow_MemViewerUpdate();
                //GLWindow_IOViewerUpdate();
                //GLWindow_DisassemblerUpdate();

                TimerWait(Keys_Down[VK_SPACE] == 0);
            }
            else if(RUNNING == RUN_GB)
            {
                extern _GB_CONTEXT_ GameBoy;

                GB_Input_Update();

                if(Keys_JustPressed[VK_SPACE]) GB_Frameskip(100);
                if(Keys_Down[VK_SPACE]) GB_SoundResetBufferPointers();
                else GB_Frameskip(FRAMESKIP);

                //GB_RunFor(70224);
                while(GameBoy.Emulator.FrameDrawn == 0) GB_RunForInstruction();
                GameBoy.Emulator.FrameDrawn = 0;

                if(GB_HaveToFrameskip() == 0)
                {
                    GB_Screen_WriteBuffer();

                    glClear(GL_COLOR_BUFFER_BIT); //Clear screen

                    if(scr_texture_loaded) glDeleteTextures(1,&scr_texture);
                    scr_texture_loaded = 1;

                    glGenTextures(1,&scr_texture);
                    glBindTexture(GL_TEXTURE_2D,scr_texture);
                    if(EmulatorConfig.oglfilter)
                    {
                        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                    }
                    else
                    {
                        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
                    }
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

                    extern _GB_CONTEXT_ GameBoy;
                    if(GameBoy.Emulator.SGBEnabled)
                        glTexImage2D(GL_TEXTURE_2D,0,4,256,224, 0,GL_RGBA,GL_UNSIGNED_BYTE,SCREEN_TEXTURE);
                    else
                        glTexImage2D(GL_TEXTURE_2D,0,4,160,144, 0,GL_RGBA,GL_UNSIGNED_BYTE,SCREEN_TEXTURE);

                    glPushMatrix();

                    if(GameBoy.Emulator.rumble)
                    {
                        float trns_x = (((float)rand()/(float)RAND_MAX)-0.5) * 2.0;
                        float trns_y = (((float)rand()/(float)RAND_MAX)-0.5) * 2.0;
                        glTranslatef(trns_x,trns_y,0);
                    }

                    if(GameBoy.Emulator.SGBEnabled)
                    {
                        glBindTexture(GL_TEXTURE_2D,scr_texture);
                        glColor3f(1.0,1.0,1.0);
                        glBegin( GL_QUADS );
                            glTexCoord2f(0,0); // Top-left vertex
                            glVertex3f(0,0,0);

                            glTexCoord2f(1,0); // Bottom-left vertex
                            glVertex3f(256,0,0);

                            glTexCoord2f(1,1); // Bottom-right vertex
                            glVertex3f(256,224,0);

                            glTexCoord2f(0,1); // Top-right vertex
                            glVertex3f(0,224,0);
                        glEnd();
                    }
                    else
                    {
                        glBindTexture(GL_TEXTURE_2D,scr_texture);
                        glColor3f(1.0,1.0,1.0);
                        glBegin( GL_QUADS );
                            glTexCoord2f(0,0); // Top-left vertex
                            glVertex3f(0,0,0);

                            glTexCoord2f(1,0); // Bottom-left vertex
                            glVertex3f(160,0,0);

                            glTexCoord2f(1,1); // Bottom-right vertex
                            glVertex3f(160,144,0);

                            glTexCoord2f(0,1); // Top-right vertex
                            glVertex3f(0,144,0);
                        glEnd();
                    }

                    glPopMatrix();

                    GLWindow_SwapBuffers();
                }

                GB_FrameskipUpdate();

                TimerWait(Keys_Down[VK_SPACE] == 0);
            }
            else
            {
                glClear(GL_COLOR_BUFFER_BIT); //Clear screen
                GLWindow_SwapBuffers();
                Sleep(100);
            }
        }
        else
        {
            Sleep(1); // Allow the CPU to rest a bit :P
        }
/*
if(RUNNING == RUN_GB)
{
    if(Keys_Down['Q']) { int j = 10; while(j--) { GLWindow_GBDisassemblerStep(); } GLWindow_GBDisassemblerUpdate(); }
    if(Keys_Down['W']) { int j = 100; while(j--) { GLWindow_GBDisassemblerStep(); } GLWindow_GBDisassemblerUpdate(); }
    if(Keys_Down['E']) { int j = 1000; while(j--) { GLWindow_GBDisassemblerStep(); } GLWindow_GBDisassemblerUpdate(); }
}
*/
/*
if(RUNNING == RUN_GBA)
{
        if(Keys_Down['Q']) { int k = 10; while(k--) GLWindow_GBADisassemblerStep(); GLWindow_GBADisassemblerUpdate(); }
        if(Keys_Down['W']) { int k = 100; while(k--) GLWindow_GBADisassemblerStep(); GLWindow_GBADisassemblerUpdate(); }
        if(Keys_Down['R']) { int k = 1000; while(k--) GLWindow_GBADisassemblerStep(); GLWindow_GBADisassemblerUpdate(); }
}
*/
    }

    GLWindow_UnloadRom(1);

    if(scr_texture_loaded) glDeleteTextures(1,&scr_texture);

    TimerEnd();
}
