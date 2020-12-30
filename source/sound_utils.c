// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <string.h>

#include <SDL2/SDL.h>

#include "config.h"
#include "debug_utils.h"
#include "general_utils.h"
#include "input_utils.h"
#include "sound_utils.h"

#define SDL_BUFFER_SAMPLES  (1 * 1024)

// Number of samples to have as max in the SDL sound buffer.
#define SDL_BUFFER_SAMPLES_THRESHOLD   (SDL_BUFFER_SAMPLES * 6)

static int sound_enabled = 0;

static SDL_AudioSpec obtained_spec;
static SDL_AudioStream *stream;

static void Sound_Callback(unused__ void *userdata, Uint8 *buffer, int len)
{
    if (sound_enabled == 0)
    {
        memset(buffer, 0, len);
        return;
    }

    // Don't play audio during speedup or if it is disabled in the configuration
    if (EmulatorConfig.snd_mute || Input_Speedup_Enabled())
    {
        // Output silence
        memset(buffer, 0, len);
        SDL_AudioStreamClear(stream);
    }
    else
    {
        int available = SDL_AudioStreamAvailable(stream);

        if (available < len)
        {
            memset(buffer, 0, len);
        }
        else
        {
            int obtained = SDL_AudioStreamGet(stream, buffer, len);
            if (obtained == -1)
            {
                Debug_LogMsgArg("Failed to get converted data: %s",
                                SDL_GetError());
            }
            else
            {
                if (obtained != len)
                {
                    Debug_LogMsgArg("%s: Obtained = %d, Requested = %d",
                                    __func__, obtained, len);
                    // Clear the rest of the buffer
                    memset(&(buffer[obtained]), 0, len - obtained);
                }
            }
        }

        //Debug_LogMsgArg("Available: %d/%d",
        //                SDL_AudioStreamAvailable(stream),
        //                obtained_spec.samples);
    }
}

void Sound_ClearBuffer(void)
{
    SDL_AudioStreamClear(stream);
}

static void Sound_End(void)
{
     SDL_FreeAudioStream(stream);

     sound_enabled = 0;
}

void Sound_Init(void)
{
    sound_enabled = 0;

    SDL_AudioSpec desired_spec;

    desired_spec.freq = SDL_SAMPLERATE;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 2;
    desired_spec.samples = SDL_BUFFER_SAMPLES;
    desired_spec.callback = Sound_Callback;
    desired_spec.userdata = NULL;

    if (SDL_OpenAudio(&desired_spec, &obtained_spec) < 0)
    {
        Debug_ErrorMsgArg("Couldn't open audio: %s\n", SDL_GetError());
        return;
    }

    // Input format is int16_t, dual, 32 * 1024 Hz
    // Output format is whatever SDL_OpenAudio() returned
    stream = SDL_NewAudioStream(AUDIO_S16, 2, GBA_SAMPLERATE,
                                obtained_spec.format, obtained_spec.channels,
                                obtained_spec.freq);
    if (stream == NULL) {
        Debug_ErrorMsgArg("Failed to create audio stream: %s", SDL_GetError());
        return;
    }

    // Cleanup everything on exit of the program
    atexit(Sound_End);

    SDL_PauseAudio(0);

    sound_enabled = 1;
}

int Sound_IsBufferOverThreshold(void)
{
    if (SDL_AudioStreamAvailable(stream) > SDL_BUFFER_SAMPLES_THRESHOLD)
        return 1;

    return 0;
}

int Sound_IsBufferTooBig(void)
{
    if (SDL_AudioStreamAvailable(stream) > SDL_BUFFER_SAMPLES_THRESHOLD * 2)
        return 1;

    return 0;
}

void Sound_SendSamples(int16_t *buffer, int len)
{
    int rc = SDL_AudioStreamPut(stream, buffer, len);
    if (rc == -1)
        Debug_LogMsgArg("Failed to send samples to stream: %s", SDL_GetError());
}

void Sound_Enable(void)
{
    sound_enabled = 1;
}

void Sound_Disable(void)
{
    sound_enabled = 0;
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
