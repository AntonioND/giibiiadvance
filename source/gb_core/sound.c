// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2021, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../config.h"
#include "../debug_utils.h"
#include "../general_utils.h"
#include "../wav_utils.h"

#include "debug.h"
#include "gameboy.h"
#include "general.h"
#include "memory.h"

// Quite a big buffer, but it works fine this way.  The bigger, the less
// possibilities to underflow, but the more delay between actions and sound
// output.
//
// Note: It must be a power of two.
#define GB_BUFFER_SIZE      (16384)
#define GB_BUFFER_SAMPLES   (GB_BUFFER_SIZE / 2)

#define GB_SAMPLE_RATE      (32 * 1024)

extern _GB_CONTEXT_ GameBoy;

static const s8 GB_SquareWave[4][32] = {
    {
        -128, -128, -128, -128, -128, -128, -128, -128,
         127,  127, -128, -128, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -128,
         127,  127, -128, -128, -128, -128, -128, -128
    },
    {
        -128, -128, -128, -128, -128, -128, -128, -128,
         127,  127,  127,  127, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -128,
         127,  127,  127,  127, -128, -128, -128, -128
    },
    {
        -128, -128, -128, -128,  127,  127,  127,  127,
         127,  127,  127,  127, -128, -128, -128, -128,
        -128, -128, -128, -128,  127,  127,  127,  127,
         127,  127,  127,  127, -128, -128, -128, -128
    },
    {
         127,  127,  127,  127,  127,  127,  127,  127,
        -128, -128, -128, -128,  127,  127,  127,  127,
         127,  127,  127,  127,  127,  127,  127,  127,
        -128, -128, -128, -128,  127,  127,  127,  127
    }
};

static s8 GB_WavePattern[32];

typedef struct
{
    struct // Tone & Sweep
    {
        u16 reg[5];

        u32 sweepstepsleft;
        u32 sweeptime; // If 0, not active
        u32 sweepinc;
        u32 sweepshift;
        u32 sweepfreq;

        u32 duty;

        u32 vol;
        u32 envactive; // If != 0, activate
        u32 envelope;
        u32 envincrease;
        u32 envstepstochange;

        u32 stepsleft; // Each step is 1/256 sec
        u32 limittime; // Activates "stepsleft"

        u32 speakerright;
        u32 speakerleft;

        u32 frequency;
        u32 frequency_steps;

        u32 samplecount;

        int out_sample;

        u32 running;
    } Chn1;

    struct // Tone
    {
        u16 reg[5];

        s32 freq;
        u32 duty;

        u32 vol;
        u32 envactive; // If != 0, activate
        u32 envelope;
        u32 envincrease;
        u32 envstepstochange;

        u32 stepsleft; // Each step is 1/256 sec
        u32 limittime; // Activates "stepsleft"

        u32 speakerright;
        u32 speakerleft;

        u32 frequency;
        u32 frequency_steps;

        u32 samplecount;

        int out_sample;

        u32 running;
    } Chn2;

    struct // Wave Output
    {
        u16 reg[5];

        u32 playing;

        u32 vol;

        u32 stepsleft; // Each step is 1/256 sec
        u32 limittime; // Activates "stepsleft"

        u32 speakerright;
        u32 speakerleft;

        int doublesize;     // 0 - 1
        int buffer_playing; // 0 - 1
        u8 wave_ram_buffer[2][16];

        u32 frequency;
        u32 frequency_steps;

        u32 samplecount;

        int out_sample;

        u32 running;
    } Chn3;

    struct // Noise
    {
        u16 reg[5];

        u32 counter_width;
        u32 lfsr_state;

        u32 vol;
        u32 envactive; // If != 0, activate
        u32 envelope;
        u32 envincrease;
        u32 envstepstochange;

        u32 stepsleft; // Each step is 1/256 sec
        u32 limittime; // Activates "stepsleft"

        u32 speakerright;
        u32 speakerleft;

        int seed;

        u32 frequency;
        u32 frequency_steps;

        int out_sample;

        u32 running;
    } Chn4;

    u32 leftvol;
    u32 rightvol;

    u32 step_clocks;

    u32 nextfreq_clocks;
    u32 nextfreq_ch4_clocks;

    u32 nextsample_clocks;

    s16 buffer[GB_SAMPLE_RATE];
    u32 buffer_write_ptr;

    // Some temporary variables to avoid doing the same calculations every time
    // a sample is going to be generated:
    int leftvol_1, rightvol_1;
    int leftvol_2, rightvol_2;
    int leftvol_3, rightvol_3;
    int leftvol_4, rightvol_4;

    u32 master_enable;
} _GB_SOUND_HARDWARE_;

static _GB_SOUND_HARDWARE_ Sound;

static int output_enabled;

int GB_SoundHardwareIsOn(void)
{
    return Sound.master_enable;
}

void GB_SoundPowerOff(void)
{
    Sound.Chn1.running = 0;
    Sound.Chn2.running = 0;
    Sound.Chn3.running = 0;
    Sound.Chn4.running = 0;
}

void GB_SoundPowerOn(void)
{
    // Reset values
    Sound.Chn1.sweeptime = 0;
    Sound.Chn1.envactive = 0;
    Sound.Chn1.limittime = 0;

    Sound.Chn2.envactive = 0;
    Sound.Chn2.limittime = 0;

    Sound.Chn3.playing = 0;
    Sound.Chn3.limittime = 0;

    Sound.Chn4.envactive = 0;
    Sound.Chn4.limittime = 0;
    Sound.Chn4.seed = 0xFF;
}

void GB_SoundSaveToWAV(void)
{
    size_t available_size = Sound.buffer_write_ptr * sizeof(s16);

    // Save all available samples to a WAV file if a recording is active
    if (WAV_FileIsOpen())
        WAV_FileStream(Sound.buffer, available_size);
}

// This function is supposed to return all the samples taken during a frame. If
// the destination buffer isn't big enough, it will clear the source buffer
// anyway, to prepare it for next frame.
size_t GB_SoundGetSamplesFrame(void *buffer, size_t buffer_size)
{
    size_t available_size = Sound.buffer_write_ptr * 2;

    size_t copy_size = (available_size < buffer_size) ?
                       available_size : buffer_size;

    memcpy(buffer, Sound.buffer, copy_size);

    // Reset pointer
    Sound.buffer_write_ptr = 0;

    return copy_size;
}

void GB_SoundResetBufferPointers(void)
{
    Sound.buffer_write_ptr = 0;
}

void GB_SoundInit(void)
{
    // Prepare memory
    memset(&Sound, 0, sizeof(Sound));
    GB_SoundResetBufferPointers();

    output_enabled = 1;

    Sound.leftvol_1 = Sound.rightvol_1 = 0;
    Sound.leftvol_2 = Sound.rightvol_2 = 0;
    Sound.leftvol_3 = Sound.rightvol_3 = 0;
    Sound.leftvol_4 = Sound.rightvol_4 = 0;

    // Set registers to initial values

    GB_MemWrite8(NR52_REG, 0xF0);

    GB_MemWrite8(NR10_REG, 0x80);
    GB_MemWrite8(NR11_REG, 0xBF);
    GB_MemWrite8(NR12_REG, 0xF3);
    GB_MemWrite8(NR14_REG, 0xBF);

    GB_MemWrite8(NR21_REG, 0x3F);
    GB_MemWrite8(NR22_REG, 0x00);
    GB_MemWrite8(NR24_REG, 0xBF);

    GB_MemWrite8(NR30_REG, 0x7F);
    GB_MemWrite8(NR31_REG, 0xFF);
    GB_MemWrite8(NR32_REG, 0x9F);
    GB_MemWrite8(NR34_REG, 0xBF);

    GB_MemWrite8(NR41_REG, 0xFF);
    GB_MemWrite8(NR42_REG, 0x00);
    GB_MemWrite8(NR43_REG, 0x00);
    GB_MemWrite8(NR44_REG, 0xBF);

    GB_MemWrite8(NR50_REG, 0x77);
    GB_MemWrite8(NR51_REG, 0xF3);

    GB_SoundPowerOn();

    Sound.Chn1.running = 0;
    Sound.Chn2.running = 0;
    Sound.Chn3.running = 0;
    Sound.Chn4.running = 0;

    _GB_MEMORY_ *mem = &GameBoy.Memory;

    switch (GameBoy.Emulator.HardwareType)
    {
        case HW_GB:
        case HW_GBP:
            mem->IO_Ports[0x30] = 0xAC;
            mem->IO_Ports[0x31] = 0xDD;
            mem->IO_Ports[0x32] = 0xDA;
            mem->IO_Ports[0x33] = 0x48;
            mem->IO_Ports[0x34] = 0x36;
            mem->IO_Ports[0x35] = 0x02;
            mem->IO_Ports[0x36] = 0xCF;
            mem->IO_Ports[0x37] = 0x16;
            mem->IO_Ports[0x38] = 0x2C;
            mem->IO_Ports[0x39] = 0x04;
            mem->IO_Ports[0x3A] = 0xE5;
            mem->IO_Ports[0x3B] = 0x2C;
            mem->IO_Ports[0x3C] = 0xAC;
            mem->IO_Ports[0x3D] = 0xDD;
            mem->IO_Ports[0x3E] = 0xDA;
            mem->IO_Ports[0x3F] = 0x48;
            break;

        case HW_SGB:
        case HW_SGB2:
            mem->IO_Ports[0x30] = 0x00;
            mem->IO_Ports[0x31] = 0xFF;
            mem->IO_Ports[0x32] = 0x00;
            mem->IO_Ports[0x33] = 0xFB;
            mem->IO_Ports[0x34] = 0x00;
            mem->IO_Ports[0x35] = 0xFF;
            mem->IO_Ports[0x36] = 0x00;
            mem->IO_Ports[0x37] = 0xFF;
            mem->IO_Ports[0x38] = 0x00;
            mem->IO_Ports[0x39] = 0xFF;
            mem->IO_Ports[0x3A] = 0x00;
            mem->IO_Ports[0x3B] = 0xFF;
            mem->IO_Ports[0x3C] = 0x00;
            mem->IO_Ports[0x3D] = 0xFF;
            mem->IO_Ports[0x3E] = 0x00;
            mem->IO_Ports[0x3F] = 0xFF;
            break;

        case HW_GBC:
        case HW_GBA:
        case HW_GBA_SP:
            mem->IO_Ports[0x30] = 0x00;
            mem->IO_Ports[0x31] = 0xFF;
            mem->IO_Ports[0x32] = 0x00;
            mem->IO_Ports[0x33] = 0xFF;
            mem->IO_Ports[0x34] = 0x00;
            mem->IO_Ports[0x35] = 0xFF;
            mem->IO_Ports[0x36] = 0x00;
            mem->IO_Ports[0x37] = 0xFF;
            mem->IO_Ports[0x38] = 0x00;
            mem->IO_Ports[0x39] = 0xFF;
            mem->IO_Ports[0x3A] = 0x00;
            mem->IO_Ports[0x3B] = 0xFF;
            mem->IO_Ports[0x3C] = 0x00;
            mem->IO_Ports[0x3D] = 0xFF;
            mem->IO_Ports[0x3E] = 0x00;
            mem->IO_Ports[0x3F] = 0xFF;

            mem->IO_Ports[NR52_REG - 0xFF00] |= (1 << 0);
            break;
    }
}

void GB_SoundLoadWave(void)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    s8 *bufferptr = GB_WavePattern;

    for (int count = 0x30; count < 0x40; count++)
    {
        *bufferptr++ = (mem->IO_Ports[count] & 0xF0) - 127;
        *bufferptr++ = ((mem->IO_Ports[count] & 0xF) << 4) - 127;
    }
}

void GB_ToggleSound(void)
{
    output_enabled ^= 1;
    if (output_enabled)
        GB_SoundResetBufferPointers();
}

void GB_SoundMix(void)
{
    if (EmulatorConfig.snd_mute)
        return;

    if (Sound.master_enable == 0)
    {
        Sound.buffer[Sound.buffer_write_ptr++] = 0;
        Sound.buffer[Sound.buffer_write_ptr++] = 0;
        return;
    }
#if 0
    // Changed to the places where it is actually changed. If there are
    // problems, uncomment to see if this is the problem.
    Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                    (Sound.Chn1.vol * Sound.leftvol) : 0;
    Sound.rightvol_1 = Sound.Chn1.speakerright ?
                    (Sound.Chn1.vol * Sound.rightvol) : 0;
    Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                    (Sound.Chn2.vol * Sound.leftvol) : 0;
    Sound.rightvol_2 = Sound.Chn2.speakerright ?
                    (Sound.Chn2.vol * Sound.rightvol) : 0;
    Sound.leftvol_3 = Sound.Chn3.speakerleft ?
                    (Sound.Chn3.vol * Sound.leftvol) : 0;
    Sound.rightvol_3 = Sound.Chn3.speakerright ?
                    (Sound.Chn3.vol * Sound.rightvol) : 0;
    Sound.leftvol_4 = Sound.Chn4.speakerleft ?
                    (Sound.Chn4.vol * Sound.leftvol) : 0;
    Sound.rightvol_4 = Sound.Chn4.speakerright ?
                    (Sound.Chn4.vol * Sound.rightvol) : 0;
#endif
    s32 outvalue_left = 0, outvalue_right = 0;

    if (Sound.Chn1.running && (EmulatorConfig.chn_flags & 0x1))
    {
        outvalue_left += Sound.Chn1.out_sample * Sound.leftvol_1;
        outvalue_right += Sound.Chn1.out_sample * Sound.rightvol_1;
    }
    if (Sound.Chn2.running && (EmulatorConfig.chn_flags & 0x2))
    {
        outvalue_left += Sound.Chn2.out_sample * Sound.leftvol_2;
        outvalue_right += Sound.Chn2.out_sample * Sound.rightvol_2;
    }
    if (Sound.Chn3.running && (EmulatorConfig.chn_flags & 0x4))
    {
        outvalue_left += Sound.Chn3.out_sample * Sound.leftvol_3;
        outvalue_right += Sound.Chn3.out_sample * Sound.rightvol_3;
    }
    if (Sound.Chn4.running && (EmulatorConfig.chn_flags & 0x8))
    {
        outvalue_left += Sound.Chn4.out_sample * Sound.leftvol_4;
        outvalue_right += Sound.Chn4.out_sample * Sound.rightvol_4;
    }

    if (outvalue_left > 65535)
        outvalue_left = 65535;
    else if (outvalue_left < (-65536))
        outvalue_left = -65536;

    if (outvalue_right > 65535)
        outvalue_right = 65535;
    else if (outvalue_right < (-65536))
        outvalue_right = -65536;

    outvalue_left >>= 1;
    outvalue_right >>= 1;

    outvalue_left = (outvalue_left * EmulatorConfig.volume) / 128;
    outvalue_right = (outvalue_right * EmulatorConfig.volume) / 128;

    Sound.buffer[Sound.buffer_write_ptr++] = outvalue_left;
    Sound.buffer[Sound.buffer_write_ptr++] = outvalue_right;
}

void GB_SoundRegWrite(u32 address, u32 value)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (Sound.master_enable == 0)
    {
        if ((address == NR52_REG) && (value & (1 << 7)))
        {
            GB_SoundPowerOn();
            Sound.master_enable = 1;
        }
        return;
    }

    switch (address)
    {
        // Voice 1
        // -------

        case NR10_REG:
            Sound.Chn1.reg[0] = value;
            // Don't update sweep yet...
            return;
        case NR11_REG:
            Sound.Chn1.reg[1] = value;

            Sound.Chn1.stepsleft = (64 - (value & 0x3F)) << 2;
            Sound.Chn1.duty = (value >> 6) & 3;
            return;
        case NR12_REG:
            Sound.Chn1.reg[2] = value;
            // Don't update envelope yet...

            if (Sound.Chn1.running) // "zombie mode"
            {
                if (Sound.Chn1.envactive && (Sound.Chn1.envelope == 0))
                    Sound.Chn1.vol = (Sound.Chn1.vol + 1) & 0xF;
                else if ((Sound.Chn1.reg[2] & (1 << 3)) == 0)
                    Sound.Chn1.vol = (Sound.Chn1.vol + 2) & 0xF;

                if ((Sound.Chn1.reg[2] ^ value) & (1 << 3))
                    Sound.Chn1.vol = 0x10 - Sound.Chn1.vol;

                Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                                        (Sound.Chn1.vol * Sound.leftvol) : 0;
                Sound.rightvol_1 = Sound.Chn1.speakerright ?
                                        (Sound.Chn1.vol * Sound.rightvol) : 0;
            }
            return;
        case NR13_REG:
            Sound.Chn1.reg[3] = value;
            return;
        case NR14_REG:
        {
            Sound.Chn1.reg[4] = value;

            u16 freq = (u16)Sound.Chn1.reg[3]
                     | ((u16)Sound.Chn1.reg[4] & 0x07) << 8;

            Sound.Chn1.frequency = freq;
            Sound.Chn1.frequency_steps = freq;

            Sound.Chn1.limittime = (Sound.Chn1.reg[4] & (1 << 6));

            if (value & (1 << 7))
            {
                Sound.Chn1.running = 1;

                // Update sweep
                Sound.Chn1.sweeptime = (Sound.Chn1.reg[0] >> 4) & 0x07;
                Sound.Chn1.sweepstepsleft = Sound.Chn1.sweeptime << 1;
                Sound.Chn1.sweepinc = ((Sound.Chn1.reg[0] & (1 << 3)) == 0);
                Sound.Chn1.sweepshift = Sound.Chn1.reg[0] & 0x07;
                Sound.Chn1.sweepfreq = Sound.Chn1.frequency;

                // Update envelope
                Sound.Chn1.vol = (Sound.Chn1.reg[2] >> 4);
                Sound.Chn1.envelope = Sound.Chn1.reg[2] & 0x07;
                Sound.Chn1.envactive = 1; // Sound.Chn1.envelope != 0;
                Sound.Chn1.envincrease = Sound.Chn1.reg[2] & (1 << 3);
                Sound.Chn1.envstepstochange = Sound.Chn1.envelope << 2;
                Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                                        (Sound.Chn1.vol * Sound.leftvol) : 0;
                Sound.rightvol_1 = Sound.Chn1.speakerright ?
                                        (Sound.Chn1.vol * Sound.rightvol) : 0;

                mem->IO_Ports[NR52_REG - 0xFF00] |= (1 << 0);
            }
            else
            {
                //mem->IO_Ports[NR52_REG - 0xFF00] &= ~(1 << 0);
            }
            return;
        }

        // Voice 2
        // -------

        case NR21_REG:
            Sound.Chn2.reg[1] = value;

            Sound.Chn2.stepsleft = (64 - (value & 0x3F)) << 2;
            Sound.Chn2.duty = (value >> 6) & 3;
            return;
        case NR22_REG:
            Sound.Chn2.reg[2] = value;
            // Don't update envelope yet...

            if (Sound.Chn2.running) // "zombie mode"
            {
                if (Sound.Chn2.envactive && (Sound.Chn2.envelope == 0))
                    Sound.Chn2.vol = (Sound.Chn2.vol + 1) & 0xF;
                else if ((Sound.Chn2.reg[2] & (1 << 3)) == 0)
                    Sound.Chn2.vol = (Sound.Chn2.vol + 2) & 0xF;

                if ((Sound.Chn2.reg[2] ^ value) & (1 << 3))
                    Sound.Chn2.vol = 0x10 - Sound.Chn2.vol;

                Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                                        (Sound.Chn2.vol * Sound.leftvol) : 0;
                Sound.rightvol_2 = Sound.Chn2.speakerright ?
                                        (Sound.Chn2.vol * Sound.rightvol) : 0;
            }
            return;

        case NR23_REG:
            Sound.Chn2.reg[3] = value;
            return;
        case NR24_REG:
        {
            Sound.Chn2.reg[4] = value;

            u16 freq = (u16)Sound.Chn2.reg[3]
                     | ((u16)Sound.Chn2.reg[4] & 0x07) << 8;

            Sound.Chn2.frequency = freq;
            Sound.Chn2.frequency_steps = freq;

            Sound.Chn2.limittime = (value & (1 << 6));

            if (value & (1 << 7))
            {
                Sound.Chn2.running = 1;

                // Update envelope
                Sound.Chn2.vol = (Sound.Chn2.reg[2] >> 4);
                Sound.Chn2.envelope = Sound.Chn2.reg[2] & 0x07;
                Sound.Chn2.envactive = 1; // Sound.Chn2.envelope != 0;
                Sound.Chn2.envincrease = Sound.Chn2.reg[2] & (1 << 3);
                Sound.Chn2.envstepstochange = Sound.Chn2.envelope << 2;
                Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                                        (Sound.Chn2.vol * Sound.leftvol) : 0;
                Sound.rightvol_2 = Sound.Chn2.speakerright ?
                                        (Sound.Chn2.vol * Sound.rightvol) : 0;

                mem->IO_Ports[NR52_REG - 0xFF00] |= (1 << 1);
            }
            else
            {
                //mem->IO_Ports[NR52_REG - 0xFF00] &= ~(1 << 1);
            }
            return;
        }

        // Voice 3
        // -------

        case NR30_REG:
            Sound.Chn3.reg[0] = value;

            if (value & (1 << 7))
            {
                mem->IO_Ports[NR52_REG - 0xFF00] |= (1 << 2);

                GB_SoundLoadWave();

                Sound.Chn3.samplecount = 0;
                Sound.Chn3.frequency_steps = 0;
                Sound.Chn3.playing = 1;
            }
            else
            {
                Sound.Chn3.playing = 0;
                mem->IO_Ports[NR52_REG - 0xFF00] &= ~(1 << 2);
                Sound.Chn3.running = 0;
            }
            return;
        case NR31_REG:
            Sound.Chn3.reg[1] = value;

            Sound.Chn3.stepsleft = (256 - value) << 2;
            return;
        case NR32_REG:
            Sound.Chn3.reg[2] = value;

            switch ((value >> 5) & 3)
            {
                case 0:
                    Sound.Chn3.vol = 0x0;
                    break;
                case 1:
                    Sound.Chn3.vol = 0xF;
                    break;
                case 2:
                    Sound.Chn3.vol = 0x7;
                    break;
                case 3:
                    Sound.Chn3.vol = 0x3;
                    break;
            }

            Sound.leftvol_3 = Sound.Chn3.speakerleft ?
                                        (Sound.Chn3.vol * Sound.leftvol) : 0;
            Sound.rightvol_3 = Sound.Chn3.speakerright ?
                                        (Sound.Chn3.vol * Sound.rightvol) : 0;
            return;
        case NR33_REG:
            Sound.Chn3.reg[3] = value;
            return;
        case NR34_REG:
        {
            Sound.Chn3.reg[4] = value;

            u16 freq = (u16)Sound.Chn3.reg[3]
                     | ((u16)Sound.Chn3.reg[4] & 0x07) << 8;

            Sound.Chn3.frequency = freq;
            Sound.Chn3.frequency_steps = freq;

            Sound.Chn3.limittime = (value & (1 << 6));

            if (Sound.Chn3.playing) // ?
            {
                if (value & (1 << 7))
                {
                    //Sound.Chn3.playing = 1; // ?
                    Sound.Chn3.samplecount = 0;
                    mem->IO_Ports[NR52_REG - 0xFF00] |= (1 << 2);
                    GB_SoundLoadWave();
                    Sound.Chn3.running = 1;
                }
            }
            return;
        }

        // Voice 4
        // -------

        case NR41_REG:
            Sound.Chn4.reg[1] = value;

            Sound.Chn4.stepsleft = (64 - (value & 0x3F)) << 2;
            return;
        case NR42_REG:
            Sound.Chn4.reg[2] = value;
            // Don't update envelope yet...
#if 0
            // No zombie mode in channel 4?
            if (Sound.Chn4.running) // "zombie mode"
            {
                if (Sound.Chn4.envactive && (Sound.Chn4.envelope == 0))
                    Sound.Chn4.vol = (Sound.Chn4.vol + 1) & 0xF;
                else if ((Sound.Chn4.reg[2] & (1 << 3)) == 0)
                    Sound.Chn4.vol = (Sound.Chn4.vol + 2) & 0xF;

                if ((Sound.Chn4.reg[2] ^ value) & (1 << 3))
                    Sound.Chn4.vol = 0x10 - Sound.Chn4.vol;

                Sound.leftvol_4 = Sound.Chn4.speakerleft ?
                                        (Sound.Chn4.vol * Sound.leftvol) : 0;
                Sound.rightvol_4 = Sound.Chn4.speakerright ?
                                        (Sound.Chn4.vol * Sound.rightvol) : 0;
            }
#endif
            return;
        case NR43_REG:
        {
            Sound.Chn4.reg[3] = value;

            if (value & (1 << 3))
            {
                Sound.Chn4.counter_width = 7;
                Sound.Chn4.lfsr_state = 0x7F;
            }
            else
            {
                Sound.Chn4.counter_width = 15;
                Sound.Chn4.lfsr_state = 0x7FFF;
            }

            int freq_div = (value >> 4) & 0xF;
            int div_ratio = value & 0x7;

            if (freq_div > 13)
            {
                Sound.Chn4.running = 0;
                return;
            }

            if (div_ratio > 0) // For div_ratio = 0 assume 0.5 instead
                div_ratio <<= 1;

            Sound.Chn4.frequency = div_ratio * (1 << (freq_div + 1));
            Sound.Chn4.frequency_steps = 0;
            return;
        }
        case NR44_REG:
            Sound.Chn4.reg[4] = value;

            Sound.Chn4.limittime = value & (1 << 6);

            if (value & (1 << 7))
            {
                Sound.Chn4.running = 1;

                // Update envelope
                Sound.Chn4.vol = (Sound.Chn4.reg[2] >> 4);
                Sound.Chn4.envelope = Sound.Chn4.reg[2] & 0x07;
                Sound.Chn4.envactive = 1; // Sound.Chn4.envelope != 0;
                Sound.Chn4.envincrease = Sound.Chn4.reg[2] & (1 << 3);
                Sound.Chn4.envstepstochange = Sound.Chn4.envelope << 2;
                Sound.leftvol_4 = Sound.Chn4.speakerleft ?
                                        (Sound.Chn4.vol * Sound.leftvol) : 0;
                Sound.rightvol_4 = Sound.Chn4.speakerright ?
                                        (Sound.Chn4.vol * Sound.rightvol) : 0;

                mem->IO_Ports[NR52_REG - 0xFF00] |= (1 << 3);
            }
            else
            {
                //mem->IO_Ports[NR52_REG - 0xFF00] &= ~(1 << 3);
            }
            return;

        // Control
        // -------

        case NR50_REG:
            Sound.rightvol = value & 0x7;
            Sound.leftvol = (value >> 4) & 0x7;
            Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                                        (Sound.Chn1.vol * Sound.leftvol) : 0;
            Sound.rightvol_1 = Sound.Chn1.speakerright ?
                                        (Sound.Chn1.vol * Sound.rightvol) : 0;
            Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                                        (Sound.Chn2.vol * Sound.leftvol) : 0;
            Sound.rightvol_2 = Sound.Chn2.speakerright ?
                                        (Sound.Chn2.vol * Sound.rightvol) : 0;
            Sound.leftvol_3 = Sound.Chn3.speakerleft ?
                                        (Sound.Chn3.vol * Sound.leftvol) : 0;
            Sound.rightvol_3 = Sound.Chn3.speakerright ?
                                        (Sound.Chn3.vol * Sound.rightvol) : 0;
            Sound.leftvol_4 = Sound.Chn4.speakerleft ?
                                        (Sound.Chn4.vol * Sound.leftvol) : 0;
            Sound.rightvol_4 = Sound.Chn4.speakerright ?
                                        (Sound.Chn4.vol * Sound.rightvol) : 0;
            return;
        case NR51_REG:
            Sound.Chn1.speakerright = (value & (1 << 0));
            Sound.Chn1.speakerleft = (value & (1 << 4));
            Sound.Chn2.speakerright = (value & (1 << 1));
            Sound.Chn2.speakerleft = (value & (1 << 5));
            Sound.Chn3.speakerright = (value & (1 << 2));
            Sound.Chn3.speakerleft = (value & (1 << 6));
            Sound.Chn4.speakerright = (value & (1 << 3));
            Sound.Chn4.speakerleft = (value & (1 << 7));
            Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                                        (Sound.Chn1.vol * Sound.leftvol) : 0;
            Sound.rightvol_1 = Sound.Chn1.speakerright ?
                                        (Sound.Chn1.vol * Sound.rightvol) : 0;
            Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                                        (Sound.Chn2.vol * Sound.leftvol) : 0;
            Sound.rightvol_2 = Sound.Chn2.speakerright ?
                                        (Sound.Chn2.vol * Sound.rightvol) : 0;
            Sound.leftvol_3 = Sound.Chn3.speakerleft ?
                                        (Sound.Chn3.vol * Sound.leftvol) : 0;
            Sound.rightvol_3 = Sound.Chn3.speakerright ?
                                        (Sound.Chn3.vol * Sound.rightvol) : 0;
            Sound.leftvol_4 = Sound.Chn4.speakerleft ?
                                        (Sound.Chn4.vol * Sound.leftvol) : 0;
            Sound.rightvol_4 = Sound.Chn4.speakerright ?
                                        (Sound.Chn4.vol * Sound.rightvol) : 0;
            return;
        case NR52_REG:
            if ((value & (1 << 7)) == 0)
            {
                GB_SoundPowerOff();
                Sound.master_enable = 0;
            }
            return;
        default:
            Debug_DebugMsgArg("GB Sound: [%04x]=%02x (?)\n", address, value);
            return;
    }
}

void GB_SoundEnd(void)
{

}

//----------------------------------------------------------------

static int gb_sound_clock_counter = 0;

void GB_SoundClockCounterReset(void)
{
    gb_sound_clock_counter = 0;
}

static int GB_SoundClockCounterGet(void)
{
    return gb_sound_clock_counter;
}

static void GB_SoundClockCounterSet(int new_reference_clocks)
{
    gb_sound_clock_counter = new_reference_clocks;
}

static u32 min4(u32 a, u32 b, u32 c, u32 d)
{
    u32 x = (a < b) ? a : b;
    u32 y = (c < d) ? c : d;
    return (x < y) ? x : y;
}

void GB_SoundUpdateClocksCounterReference(int reference_clocks)
{
    _GB_MEMORY_ *mem = &GameBoy.Memory;

    if (reference_clocks < GB_SoundClockCounterGet())
        return;

    u32 clocks = reference_clocks - GB_SoundClockCounterGet();

    // Every 16384 clocks update hardware, every 128 generate sample

    while (1)
    {
        u32 next_step_clocks = 16384 - Sound.step_clocks;
        u32 next_sample_clocks = 128 - Sound.nextsample_clocks;
        u32 next_freq_clocks = 2 - Sound.nextfreq_clocks;
        u32 next_freq_ch4_clocks = 4 - Sound.nextfreq_ch4_clocks;

        u32 next_clocks = min4(next_step_clocks, next_sample_clocks,
                               next_freq_clocks, next_freq_ch4_clocks);

        if (next_clocks > clocks)
        {
            Sound.step_clocks += clocks;
            Sound.nextsample_clocks += clocks;
            Sound.nextfreq_clocks += clocks;
            Sound.nextfreq_ch4_clocks += clocks;

            GB_SoundClockCounterSet(reference_clocks);

            return;
        }

        clocks -= next_clocks;

        Sound.step_clocks += next_clocks;
        Sound.nextsample_clocks += next_clocks;
        Sound.nextfreq_clocks += next_clocks;
        Sound.nextfreq_ch4_clocks += next_clocks;

        // Step event for all channels

        if (Sound.step_clocks >= 16384)
        {
            Sound.step_clocks = 0;

            // Channel 1
            if (Sound.Chn1.running)
            {
                if (Sound.Chn1.limittime)
                {
                    if (Sound.Chn1.stepsleft > 0)
                    {
                        Sound.Chn1.stepsleft--;
                    }
                    else
                    {
                        Sound.Chn1.running = 0;
                        Sound.Chn1.envactive = 0;
                        mem->IO_Ports[NR52_REG - 0xFF00] &= ~(1 << 0);
                    }
                }
                if (Sound.Chn1.envactive && Sound.Chn1.envelope)
                {
                    if (Sound.Chn1.envstepstochange == 0)
                    {
                        Sound.Chn1.envstepstochange = Sound.Chn1.envelope << 2;

                        if (Sound.Chn1.envincrease)
                        {
                            if (Sound.Chn1.vol < 0x0F)
                            {
                                Sound.Chn1.vol++;
                                Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                                        (Sound.Chn1.vol * Sound.leftvol) : 0;
                                Sound.rightvol_1 = Sound.Chn1.speakerright ?
                                        (Sound.Chn1.vol * Sound.rightvol) : 0;
                            }
                            else
                            {
                                Sound.Chn1.envactive = 0;
                            }
                        }
                        else
                        {
                            if (Sound.Chn1.vol > 0)
                            {
                                Sound.Chn1.vol--;
                                Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                                        (Sound.Chn1.vol * Sound.leftvol) : 0;
                                Sound.rightvol_1 = Sound.Chn1.speakerright ?
                                        (Sound.Chn1.vol * Sound.rightvol) : 0;
                            }
                            else
                            {
                                Sound.Chn1.envactive = 0;
                            }
                        }
                    }
                    else
                    {
                        Sound.Chn1.envstepstochange--;
                    }
                }
                if (Sound.Chn1.sweeptime > 0)
                {
                    if (Sound.Chn1.sweepstepsleft == 0)
                    {
                        Sound.Chn1.sweepstepsleft = Sound.Chn1.sweeptime << 1;

                        if (Sound.Chn1.sweepinc)
                        {
                            Sound.Chn1.sweepfreq += Sound.Chn1.sweepfreq
                                                  / (1 << Sound.Chn1.sweepshift);
                        }
                        else
                        {
                            Sound.Chn1.sweepfreq -= Sound.Chn1.sweepfreq
                                                  / (1 << Sound.Chn1.sweepshift);
                        }

                        Sound.Chn1.frequency = Sound.Chn1.sweepfreq;
                    }
                    else
                    {
                        Sound.Chn1.sweepstepsleft--;
                    }
                }
            }

            // Channel 2
            if (Sound.Chn2.running)
            {
                if (Sound.Chn2.limittime)
                {
                    if (Sound.Chn2.stepsleft > 0)
                    {
                        Sound.Chn2.stepsleft--;
                    }
                    else
                    {
                        Sound.Chn2.running = 0;
                        Sound.Chn2.envactive = 0;
                        mem->IO_Ports[NR52_REG - 0xFF00] &= ~(1 << 1);
                    }
                }
                if (Sound.Chn2.envactive && Sound.Chn2.envelope)
                {
                    if (Sound.Chn2.envstepstochange == 0)
                    {
                        Sound.Chn2.envstepstochange = Sound.Chn2.envelope << 2;

                        if (Sound.Chn2.envincrease)
                        {
                            if (Sound.Chn2.vol < 0x0F)
                            {
                                Sound.Chn2.vol++;
                                Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                                        (Sound.Chn2.vol * Sound.leftvol) : 0;
                                Sound.rightvol_2 = Sound.Chn2.speakerright ?
                                        (Sound.Chn2.vol * Sound.rightvol) : 0;
                            }
                            else
                            {
                                Sound.Chn2.envactive = 0;
                            }
                        }
                        else
                        {
                            if (Sound.Chn2.vol > 0)
                            {
                                Sound.Chn2.vol--;
                                Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                                        (Sound.Chn2.vol * Sound.leftvol) : 0;
                                Sound.rightvol_2 = Sound.Chn2.speakerright ?
                                        (Sound.Chn2.vol * Sound.rightvol) : 0;
                            }
                            else
                            {
                                Sound.Chn2.envactive = 0;
                            }
                        }
                    }
                    else
                    {
                        Sound.Chn2.envstepstochange--;
                    }
                }
            }

            // Channel 3
            if (Sound.Chn3.running)
            {
                if (Sound.Chn3.stepsleft > 0)
                {
                    Sound.Chn3.stepsleft--;
                }
                else if (Sound.Chn3.limittime)
                {
                    Sound.Chn3.running = 0;
                    mem->IO_Ports[NR52_REG - 0xFF00] &= ~(1 << 2);
                }
            }

            // Channel 4
            if (Sound.Chn4.running)
            {
                if (Sound.Chn4.limittime)
                {
                    if (Sound.Chn4.stepsleft > 0)
                    {
                        Sound.Chn4.stepsleft--;
                    }
                    else
                    {
                        Sound.Chn4.running = 0;
                        Sound.Chn4.envactive = 0;
                        mem->IO_Ports[NR52_REG - 0xFF00] &= ~(1 << 3);
                    }
                }
                if (Sound.Chn4.envactive && Sound.Chn4.envelope)
                {
                    if (Sound.Chn4.envstepstochange == 0)
                    {
                        Sound.Chn4.envstepstochange = Sound.Chn4.envelope << 2;

                        if (Sound.Chn4.envincrease)
                        {
                            if (Sound.Chn4.vol < 0x0F)
                            {
                                Sound.Chn4.vol++;
                                Sound.leftvol_4 = Sound.Chn4.speakerleft ?
                                        (Sound.Chn4.vol * Sound.leftvol) : 0;
                                Sound.rightvol_4 = Sound.Chn4.speakerright ?
                                        (Sound.Chn4.vol * Sound.rightvol) : 0;
                            }
                            else
                            {
                                Sound.Chn4.envactive = 0;
                            }
                        }
                        else
                        {
                            if (Sound.Chn4.vol > 0)
                            {
                                Sound.Chn4.vol--;
                                Sound.leftvol_4 = Sound.Chn4.speakerleft ?
                                        (Sound.Chn4.vol * Sound.leftvol) : 0;
                                Sound.rightvol_4 = Sound.Chn4.speakerright ?
                                        (Sound.Chn4.vol * Sound.rightvol) : 0;
                            }
                            else
                            {
                                Sound.Chn4.envactive = 0;
                            }
                        }
                    }
                    else
                    {
                        Sound.Chn4.envstepstochange--;
                    }
                }
            }
        }

        // Frequency change of channels 1, 2 and 3

        if (Sound.nextfreq_clocks >= 2)
        {
            Sound.nextfreq_clocks = 0;

            // Channel 1

            Sound.Chn1.frequency_steps++;
            if (Sound.Chn1.frequency_steps >= 2048)
            {
                Sound.Chn1.frequency_steps = Sound.Chn1.frequency;

                Sound.Chn1.out_sample =
                    GB_SquareWave[Sound.Chn1.duty][Sound.Chn1.samplecount];

                Sound.Chn1.samplecount++;
                Sound.Chn1.samplecount %= 32;
            }

            // Channel 2

            Sound.Chn2.frequency_steps++;
            if (Sound.Chn2.frequency_steps >= 2048)
            {
                Sound.Chn2.frequency_steps = Sound.Chn2.frequency;

                Sound.Chn2.out_sample =
                    GB_SquareWave[Sound.Chn2.duty][Sound.Chn2.samplecount];

                Sound.Chn2.samplecount++;
                Sound.Chn2.samplecount %= 32;
            }

            // Channel 3

            Sound.Chn3.frequency_steps++;
            if (Sound.Chn3.frequency_steps >= 2048)
            {
                Sound.Chn3.frequency_steps = Sound.Chn3.frequency;

                Sound.Chn3.out_sample = GB_WavePattern[Sound.Chn3.samplecount];

                Sound.Chn3.samplecount++;
                Sound.Chn3.samplecount %= 32;
            }
        }

        // Frequency change of channel 4

        if (Sound.nextfreq_ch4_clocks >= 4)
        {
            Sound.nextfreq_ch4_clocks = 0;

            if (Sound.Chn4.running)
            {
                Sound.Chn4.frequency_steps++;

                if (Sound.Chn4.frequency_steps >= Sound.Chn4.frequency)
                {
                    Sound.Chn4.frequency_steps = 0;

                    // Channel 4

                    if (Sound.Chn4.counter_width == 7)
                    {
                        if (Sound.Chn4.lfsr_state & 1)
                        {
                            Sound.Chn4.lfsr_state >>= 1;
                            Sound.Chn4.lfsr_state ^= 0x60;
                            Sound.Chn4.out_sample = 127;
                        }
                        else
                        {
                            Sound.Chn4.lfsr_state >>= 1;
                            Sound.Chn4.out_sample = -128;
                        }
                    }
                    else if (Sound.Chn4.counter_width == 15)
                    {
                        if (Sound.Chn4.lfsr_state & 1)
                        {
                            Sound.Chn4.lfsr_state >>= 1;
                            Sound.Chn4.lfsr_state ^= 0x6000;
                            Sound.Chn4.out_sample = 127;
                        }
                        else
                        {
                            Sound.Chn4.lfsr_state >>= 1;
                            Sound.Chn4.out_sample = -128;
                        }
                    }
                }
            }
        }

        // Mix samples

        if (output_enabled)
        {
            if (Sound.nextsample_clocks >= 128)
            {
                Sound.nextsample_clocks = 0;
                GB_SoundMix();
            }
        }
    }
}

int GB_SoundGetClocksToNextEvent(void)
{
    int clocks_to_next_event = 0x7FFFFFFF;

    // TODO

    return clocks_to_next_event;
}

//----------------------------------------------------------------

void GB_SoundGetConfig(int *vol, int *chn_flags)
{
    *vol = EmulatorConfig.volume;
    *chn_flags = EmulatorConfig.chn_flags;
}

void GB_SoundSetConfig(int vol, int chn_flags)
{
    EmulatorConfig.volume = vol;
    EmulatorConfig.chn_flags &= 0x30;
    EmulatorConfig.chn_flags |= chn_flags;
}
