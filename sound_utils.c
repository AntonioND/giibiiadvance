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

#include "sound_utils.h"
#include "debug_utils.h"
#include "config.h"
#define SDL_BUFFER_SAMPLES (4*1024);

static Sound_CallbackPointer * _sound_callback;
static int _sound_enabled = 0;
static SDL_AudioSpec obtained_spec;

static void __sound_callback(void * userdata, Uint8 * buffer, int len)
{
    if((_sound_enabled == 0) || EmulatorConfig.snd_mute)
    {
        //nothing
    }
    else
    {
        if(_sound_callback)
        {
            _sound_callback(buffer,len);
            return;
        }
    }

    memset(buffer,0,len);
}

void Sound_Init(void)
{
    _sound_enabled = 1;

    SDL_AudioSpec desired_spec;

    desired_spec.freq = 22050;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 2;
    desired_spec.samples = SDL_BUFFER_SAMPLES;
    desired_spec.callback = __sound_callback;
    desired_spec.userdata = NULL;

    if(SDL_OpenAudio(&desired_spec, &obtained_spec) < 0)
    {
        Debug_ErrorMsgArg("Couldn't open audio: %s\n", SDL_GetError());
        _sound_enabled = 0;
        return;
    }

    //Debug_DebugMsgArg("Freq: %d\nChannels: %d\nSamples: %d",
    //               obtained_spec.freq,obtained_spec.channels,obtained_spec.samples);

    //Prepare memory...

    SDL_PauseAudio(0);
}

void Sound_SetCallback(Sound_CallbackPointer * fn)
{
    _sound_callback = fn;
}

void Sound_Enable(void)
{
    _sound_enabled = 1;
}

void Sound_Disable(void)
{
    _sound_enabled = 0;
}

void Sound_SetVolume(int vol)
{

}

/*
FILE * f = NULL;
void closewavfile(void)
{
    fseek(f,0,SEEK_END);
    u32 size = ftell(f);

    //https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

    struct PACKED {
        u32 riff_;   u32 size;   u32 wave_;

        u32 fmt__;   u32 format;   u16 format2;   u16 channels;   u32 samplerate;   u32 byterate;
        u16 blckalign;   u16 bitspersample;

        u32 data_;   u32 numsamples;   //u32 data[..];
    } wavfile = {
        (*(u32*)"RIFF"),
        size - 8, // filesize - 8
        (*(u32*)"WAVE"),

        (*(u32*)"fmt "),
        16, //16 = PCM
        1, // 1 = PCM...
        2, //channels
        22050, //sample rate
        22050 * 2 * (16/8), //byte rate = samplerate * channels * bytes per sample
        2 * (16/8), //block align = channels * bytes per sample
        16, //bits per sample

        (*(u32*)"data"),
        size - 44 // size of the following data...
     };

    fseek(f,0,SEEK_SET);
    fwrite(&wavfile,sizeof(wavfile),1,f);

    fclose(f);
}

void createwavheader(void)
{
    atexit(closewavfile);

    f = fopen("music.wav","wb");

    char dummy_header[44];
    fwrite(dummy_header,sizeof(dummy_header),1,f);
}
*/

