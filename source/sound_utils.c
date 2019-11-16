// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include <SDL.h>

#include "sound_utils.h"
#include "debug_utils.h"
#include "config.h"
#include "input_utils.h"

#define SDL_BUFFER_SAMPLES (1 * 1024)

static Sound_CallbackPointer *_sound_callback;
static int _sound_enabled = 0;
static SDL_AudioSpec obtained_spec;

static void __sound_callback(void *userdata, Uint8 *buffer, int len)
{
    // Don't play audio during speedup or if it is disabled in the configuration
    if ((_sound_enabled == 0) || EmulatorConfig.snd_mute ||
        Input_Speedup_Enabled())
    {
        // Nothing
    }
    else
    {
        if (_sound_callback)
        {
            _sound_callback(buffer, len);
            return;
        }
    }

    memset(buffer, 0, len);
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

    if (SDL_OpenAudio(&desired_spec, &obtained_spec) < 0)
    {
        Debug_ErrorMsgArg("Couldn't open audio: %s\n", SDL_GetError());
        _sound_enabled = 0;
        return;
    }

    //Debug_DebugMsgArg("Freq: %d\nChannels: %d\nSamples: %d",
    //                  obtained_spec.freq, obtained_spec.channels,
    //                  obtained_spec.samples);

    // Prepare memory...

    SDL_PauseAudio(0);
}

void Sound_SetCallback(Sound_CallbackPointer *fn)
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
    EmulatorConfig.volume = vol;
}

void Sound_SetEnabled(int enable)
{
    EmulatorConfig.snd_mute = enable;
}

int Sound_GetEnabled(void)
{
    return EmulatorConfig.snd_mute;
}

void Sound_SetEnabledChannels(int flags)
{
    EmulatorConfig.chn_flags = flags & 0x3F;
}

#if 0
FILE *f = NULL;
void closewavfile(void)
{
    fseek(f, 0, SEEK_END);
    u32 size = ftell(f);

    //https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

    struct PACKED {
        u32 riff_;
        u32 size;
        u32 wave_;

        u32 fmt__;
        u32 format;
        u16 format2;
        u16 channels;
        u32 samplerate;
        u32 byterate;
        u16 blckalign;
        u16 bitspersample;

        u32 data_;
        u32 numsamples;
        //u32 data[..];
    } wavfile = {
        (*(u32 *)"RIFF"),
        size - 8, // filesize - 8
        (*(u32 *)"WAVE"),

        (*(u32 *)"fmt "),
        16,    // 16 = PCM
        1,     // 1 = PCM...
        2,     // 2 channels
        22050, // Sample rate
        22050 * 2 * (16 / 8), // Byte rate = samplerate * channels * bytes per sample
        2 * (16/8), // Block align = channels * bytes per sample
        16,         // Bits per sample

        (*(u32 *)"data"),
        size - 44 // Size of the following data...
     };

    fseek(f, 0, SEEK_SET);
    fwrite(&wavfile, sizeof(wavfile), 1, f);

    fclose(f);
}

void createwavheader(void)
{
    atexit(closewavfile);

    f = fopen("music.wav", "wb");

    char dummy_header[44];
    fwrite(dummy_header, sizeof(dummy_header), 1, f);
}
#endif
