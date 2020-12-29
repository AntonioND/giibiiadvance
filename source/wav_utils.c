// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2020 Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug_utils.h"

#include "sound_utils.h"

// Information taken from:
//
// https://web.archive.org/web/20040317073101/http://ccrma-www.stanford.edu/courses/422/projects/WaveFormat/

#pragma pack(push, 1)
typedef struct {

    // RIFF header

    uint32_t chunk_id;          // "RIFF" == 0x46464952
    uint32_t chunk_size;        // File size - size of (chunk_id + chunk_size)
    uint32_t format;            // "WAVE" == 0x45564157

    // "fmt " subchunk

    uint32_t subchunk_1_id;     // "fmt " == 0x20746D66
    uint32_t subchunk_1_size;   // 16 for PCM
    uint16_t audio_format;      // PCM = 1
    uint16_t num_channels;      // Mono = 1, Stereo = 2
    uint32_t sample_rate;       // Samples per second
    uint32_t byte_rate;         // sample_rate * num_channels * bits_per_sample / 8
    uint16_t block_align;       // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample;   // 8 bits = 8, 16 bits = 16, etc.

    // "data" subchunk

    uint32_t subchunk_2_id;     // "data" == 0x61746164
    uint32_t subchunk_2_size;   // Size of data following this entry
    // uint8_t data[]           // Sound data is stored here

} wav_header_t;
#pragma pack(pop)

static FILE *wav_file;
static uint32_t wav_sample_rate;

// Hardcode format to 16-bit (signed), two channels

#define WAV_NUMBER_CHANNELS     (2)
#define WAV_BITS_PER_SAMPLE     (16)

void WAV_FileEnd(void)
{
    // Check if there is an open file
    if (wav_file == NULL)
        return;

    // Now that the final size is known, write the header

    fseek(wav_file, 0, SEEK_END);
    long int size = ftell(wav_file);

    wav_header_t header = {
        .chunk_id = 0x46464952,
        .chunk_size = size - sizeof(uint32_t) - sizeof(uint32_t),
        .format = 0x45564157,

        .subchunk_1_id = 0x20746D66,
        .subchunk_1_size = 16,
        .audio_format = 1,
        .num_channels = WAV_NUMBER_CHANNELS,
        .sample_rate = wav_sample_rate,
        .byte_rate = wav_sample_rate * WAV_NUMBER_CHANNELS * WAV_BITS_PER_SAMPLE / 8,
        .block_align = WAV_NUMBER_CHANNELS * WAV_BITS_PER_SAMPLE / 8,
        .bits_per_sample = WAV_BITS_PER_SAMPLE,

        .subchunk_2_id = 0x61746164,
        .subchunk_2_size = size - sizeof(wav_header_t),
    };

    fseek(wav_file, 0, SEEK_SET);

    if (fwrite(&header, sizeof(header), 1, wav_file) != 1)
        Debug_LogMsgArg("%s(): Can't write header.", __func__);

    fclose(wav_file);

    Debug_LogMsgArg("%s: File saved. Size: %ld", __func__, size);

    wav_file = NULL;
}

void WAV_FileStart(const char *path, uint32_t sample_rate)
{
    if (path == NULL)
        path = "audio.wav";

    if (wav_file)
        WAV_FileEnd();

    wav_file = fopen(path, "wb");
    if (wav_file == NULL)
    {
        Debug_LogMsgArg("%s(): Can't open file for writing: %s", __func__, path);
        return;
    }

    wav_header_t header =  { 0 };
    if (fwrite(&header, sizeof(header), 1, wav_file) != 1)
    {
        Debug_LogMsgArg("%s(): Can't allocate space for header.", __func__);
        return;
    }

    wav_sample_rate = sample_rate;

    // Close file when the program exits
    atexit(WAV_FileEnd);
}

int WAV_FileIsOpen(void)
{
    if (wav_file == NULL)
        return 0;

    return 1;
}

void WAV_FileStream(int16_t *buffer, size_t size)
{
    if (wav_file == NULL)
        return;

    if (fwrite(buffer, size, 1, wav_file) != 1)
        Debug_LogMsgArg("%s(): Failed to write data.", __func__);
}
