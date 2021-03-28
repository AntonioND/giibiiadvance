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
#include "../wav_utils.h"

#include "cpu.h"
#include "dma.h"
#include "memory.h"
#include "sound.h"

// Quite a big buffer, but it works fine this way.  The bigger, the less
// possibilities to underflow, but the more delay between actions and sound
// output.
//
// Note: It must be a power of two.
#define GBA_BUFFER_SIZE     (16384)
#define GBA_BUFFER_SAMPLES  (GBA_BUFFER_SIZE / 2)

#define GBA_SAMPLE_RATE     (32 * 1024)

static const s8 GBA_SquareWave[4][32] = {
    {
        -128, -128,  127,  127, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -128,
        -128, -128,  127,  127, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -128,
    },
    {
         127,  127,  127,  127, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -128,
         127,  127,  127,  127, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -128,
    },
    {
         127,  127,  127,  127,  127,  127,  127,  127,
        -128, -128, -128, -128, -128, -128, -128, -128,
         127,  127,  127,  127,  127,  127,  127,  127,
        -128, -128, -128, -128, -128, -128, -128, -128,
    },
    {
        -128, -128, -128, -128,  127,  127,  127,  127,
         127,  127,  127,  127,  127,  127,  127,  127,
        -128, -128, -128, -128,  127,  127,  127,  127,
         127,  127,  127,  127,  127,  127,  127,  127,
    }
};

static s8 GBA_WavePattern[64];

typedef struct
{
    struct // Tone & Sweep
    {
        u16 reg[3];

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
        u16 reg[3];

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
        u16 reg[3];

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

#define FIFO_BUFFER_SIZE 128

    struct // DMA A
    {
        int vol;

        u32 speakerright;
        u32 speakerleft;

        u32 timer; // Timer 0 - 1

        int cursample;
        int cursamplewrite;
        u8 buffer[FIFO_BUFFER_SIZE];
        int datalen;

        u8 out_sample;

        u32 running;
    } FifoA;

    struct // DMA B
    {
        int vol;

        u32 speakerright;
        u32 speakerleft;

        u32 timer; // Timer 0 - 1

        int cursample;
        int cursamplewrite;
        u8 buffer[FIFO_BUFFER_SIZE];
        int datalen;

        u8 out_sample;

        u32 running;
    } FifoB;

    u32 leftvol;
    u32 rightvol;

    u32 step_clocks;

    int PSG_master_volume;

    u32 nextfreq_clocks;
    u32 nextfreq_ch4_clocks;

    u32 nextsample_clocks;

    s16 buffer[GBA_SAMPLE_RATE];
    u32 buffer_write_ptr;

    // Some temporary variables to avoid doing the same calculations every time
    // a sample is going to be generated:
    int leftvol_1, rightvol_1;
    int leftvol_2, rightvol_2;
    int leftvol_3, rightvol_3;
    int leftvol_4, rightvol_4;

    int leftvol_A, rightvol_A;
    int leftvol_B, rightvol_B;

    u32 master_enable;
} _GBA_SOUND_HARDWARE_;

static _GBA_SOUND_HARDWARE_ Sound;

static int output_enabled;

int GBA_SoundHardwareIsOn(void)
{
    return Sound.master_enable;
}

void GBA_SoundPowerOff(void)
{
    GBA_SoundRegWrite16(SOUND1CNT_L, 0);
    GBA_SoundRegWrite16(SOUND1CNT_H, 0);
    GBA_SoundRegWrite16(SOUND1CNT_X, 0);
    GBA_SoundRegWrite16(SOUND2CNT_L, 0);
    GBA_SoundRegWrite16(SOUND2CNT_H, 0);
    GBA_SoundRegWrite16(SOUND3CNT_L, 0);
    GBA_SoundRegWrite16(SOUND3CNT_H, 0);
    GBA_SoundRegWrite16(SOUND3CNT_X, 0);
    GBA_SoundRegWrite16(SOUND4CNT_L, 0);
    GBA_SoundRegWrite16(SOUND4CNT_H, 0);
    GBA_SoundRegWrite16(SOUNDCNT_L, 0);
    GBA_SoundRegWrite16(SOUNDCNT_H, 0);
    //GBA_SoundRegWrite16(SOUNDCNT_X, 0); This causes an infinite loop!

    Sound.Chn1.running = 0;
    Sound.Chn2.running = 0;
    Sound.Chn3.running = 0;
    Sound.Chn4.running = 0;

    Sound.FifoA.running = 0;
    Sound.FifoB.running = 0;
}

void GBA_SoundPowerOn(void)
{
    // Reset this values...
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

    Sound.FifoA.out_sample = 0;
    Sound.FifoA.cursample = 0;
    Sound.FifoA.cursamplewrite = 0;
    memset(Sound.FifoA.buffer, 0, sizeof(Sound.FifoA.buffer));
    Sound.FifoA.datalen = 0;

    Sound.FifoB.out_sample = 0;
    Sound.FifoB.cursample = 0;
    Sound.FifoB.cursamplewrite = 0;
    memset(Sound.FifoB.buffer, 0, sizeof(Sound.FifoB.buffer));
    Sound.FifoB.datalen = 0;

    // While Bit 7 is cleared, both PSG and FIFO sounds are disabled, and all
    // PSG registers (4000060h..4000081h) are reset to zero (and must be
    // re-initialized after re-enabling sound). However, registers 4000082h and
    // 4000088h are kept read/write-able (of which, 4000082h has no function
    // when sound is off, whilst 4000088h does work even when sound is off).

    GBA_SoundRegWrite16(SOUND1CNT_L, 0);
    GBA_SoundRegWrite16(SOUND1CNT_H, 0);
    GBA_SoundRegWrite16(SOUND1CNT_X, 0);
    GBA_SoundRegWrite16(SOUND2CNT_L, 0);
    GBA_SoundRegWrite16(SOUND2CNT_H, 0);
    GBA_SoundRegWrite16(SOUND3CNT_L, 0);
    GBA_SoundRegWrite16(SOUND3CNT_H, 0);
    GBA_SoundRegWrite16(SOUND3CNT_X, 0);
    GBA_SoundRegWrite16(SOUND4CNT_L, 0);
    GBA_SoundRegWrite16(SOUND4CNT_H, 0);
    GBA_SoundRegWrite16(SOUNDCNT_L, 0);
    //GBA_RegisterWrite16(SOUNDCNT_H, 0);
    //GBA_SoundRegWrite16(SOUNDCNT_X, 0);
    GBA_SoundRegWrite16(SOUNDBIAS, 0x200);
}

u16 *GBA_SoundGetWaveRAMTwoBuffers(void)
{
    return (u16 *)&Sound.Chn3.wave_ram_buffer[0][0];
}

void GBA_SoundSaveToWAV(void)
{
    size_t available_size = Sound.buffer_write_ptr * sizeof(s16);

    // Save all available samples to a WAV file if a recording is active
    if (WAV_FileIsOpen())
        WAV_FileStream(Sound.buffer, available_size);
}

// This function is supposed to return all the samples taken during a frame. If
// the destination buffer isn't big enough, it will clear the source buffer
// anyway, to prepare it for next frame.
size_t GBA_SoundGetSamplesFrame(void *buffer, size_t buffer_size)
{
    size_t available_size = Sound.buffer_write_ptr * sizeof(s16);

    size_t copy_size = (available_size < buffer_size) ?
                       available_size : buffer_size;

    memcpy(buffer, Sound.buffer, copy_size);

    // Reset pointer
    Sound.buffer_write_ptr = 0;

    return copy_size;
}

void GBA_SoundResetBufferPointers(void)
{
    Sound.buffer_write_ptr = 0;
}

void GBA_SoundInit(void)
{
    // Prepare memory
    memset(&Sound, 0, sizeof(Sound));
    GBA_SoundResetBufferPointers();
    output_enabled = 1;

    Sound.leftvol_1 = Sound.rightvol_1 = 0;
    Sound.leftvol_2 = Sound.rightvol_2 = 0;
    Sound.leftvol_3 = Sound.rightvol_3 = 0;
    Sound.leftvol_4 = Sound.rightvol_4 = 0;
    Sound.leftvol_A = Sound.rightvol_A = 0;
    Sound.leftvol_B = Sound.rightvol_B = 0;

    Sound.FifoA.cursample = 0;
    Sound.FifoA.cursamplewrite = 0;
    Sound.FifoA.datalen = 0;
    memset(Sound.FifoA.buffer, 0, sizeof(Sound.FifoA.buffer));
    Sound.FifoB.cursample = 0;
    Sound.FifoB.cursamplewrite = 0;
    Sound.FifoB.datalen = 0;
    memset(Sound.FifoB.buffer, 0, sizeof(Sound.FifoB.buffer));

    // Set registers to initial values
    GBA_SoundRegWrite16(SOUND1CNT_L, 0);
    GBA_SoundRegWrite16(SOUND1CNT_H, 0);
    GBA_SoundRegWrite16(SOUND1CNT_X, 0);
    GBA_SoundRegWrite16(SOUND2CNT_L, 0);
    GBA_SoundRegWrite16(SOUND2CNT_H, 0);
    GBA_SoundRegWrite16(SOUND3CNT_L, 0);
    GBA_SoundRegWrite16(SOUND3CNT_H, 0);
    GBA_SoundRegWrite16(SOUND3CNT_X, 0);
    GBA_SoundRegWrite16(SOUND4CNT_L, 0);
    GBA_SoundRegWrite16(SOUND4CNT_H, 0);
    GBA_SoundRegWrite16(SOUNDCNT_L, 0);
    GBA_RegisterWrite16(SOUNDCNT_H, 0x880E);
    GBA_SoundRegWrite16(SOUNDCNT_X, 0);

    GBA_SoundRegWrite16(SOUNDBIAS, 0x200);

    GBA_SoundPowerOn();

    Sound.Chn1.running = 0;
    Sound.Chn2.running = 0;
    Sound.Chn3.running = 0;
    Sound.Chn4.running = 0;

    Sound.FifoA.running = 0;
    Sound.FifoB.running = 0;

    for (int i = 0; i < 16; i += 2)
    {
        Sound.Chn3.wave_ram_buffer[0][i] = 0x00;
        Sound.Chn3.wave_ram_buffer[0][i + 1] = 0x00;

        Sound.Chn3.wave_ram_buffer[1][i] = 0x00;
        Sound.Chn3.wave_ram_buffer[1][i + 1] = 0x00;
    }
}

void GBA_SoundLoadWave(void)
{
    s8 *bufferptr = GBA_WavePattern;

    if (Sound.Chn3.doublesize)
    {
        u8 *srcptr = Sound.Chn3.wave_ram_buffer[Sound.Chn3.buffer_playing ^ 1];

        for (int count = 0; count < 0x10; count++)
        {
            *bufferptr++ = (srcptr[count] & 0xF0) - 127;
            *bufferptr++ = ((srcptr[count] & 0x0F) << 4) - 127;
        }

        srcptr = Sound.Chn3.wave_ram_buffer[Sound.Chn3.buffer_playing];

        for (int count = 0; count < 0x10; count++)
        {
            *bufferptr++ = (srcptr[count] & 0xF0) - 127;
            *bufferptr++ = ((srcptr[count] & 0x0F) << 4) - 127;
        }
    }
    else // Repeat the same 2 times in the buffer
    {
        u8 *srcptr = Sound.Chn3.wave_ram_buffer[Sound.Chn3.buffer_playing];

        for (int count = 0; count < 0x10; count++)
        {
            *bufferptr++ = (srcptr[count] & 0xF0) - 127;
            *bufferptr++ = ((srcptr[count] & 0x0F) << 4) - 127;
        }

        for (int count = 0; count < 0x10; count++)
        {
            *bufferptr++ = (srcptr[count] & 0xF0) - 127;
            *bufferptr++ = ((srcptr[count] & 0x0F) << 4) - 127;
        }
    }
}

void GBA_ToggleSound(void)
{
    output_enabled ^= 1;
    if (output_enabled)
        GBA_SoundResetBufferPointers();
}

void GBA_SoundMix(void)
{
    if (EmulatorConfig.snd_mute)
        return;

    if (Sound.master_enable == 0)
    {
        Sound.buffer[Sound.buffer_write_ptr++] = 0;
        Sound.buffer[Sound.buffer_write_ptr++] = 0;
        return;
    }

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
    // PSG_master_volume = 0..2
    outvalue_left = (outvalue_left * Sound.PSG_master_volume) >> 1;
    outvalue_right = (outvalue_right * Sound.PSG_master_volume) >> 1;
    // -128..128 * 0..8 * 0..16 = -16384 .. +16384
    // Each PSG channel -> -16384 .. +16384

    if (Sound.FifoA.running && (EmulatorConfig.chn_flags & 0x10))
    {
        int out_A = (int)(s8)Sound.FifoA.out_sample;
        outvalue_left += out_A * Sound.leftvol_A * 256;   // leftvol_A = 0..2
        outvalue_right += out_A * Sound.rightvol_A * 256; // "* 2" ????
        //Debug_DebugMsgArg("%d - %d %d - %d %d", Sound.FifoA.out_sample,
        //                  Sound.leftvol_A, Sound.rightvol_A,
        //                  outvalue_left, outvalue_right);
    }
    if (Sound.FifoB.running && (EmulatorConfig.chn_flags & 0x20))
    {
        int out_B = (int)(s8)Sound.FifoB.out_sample;
        outvalue_left += out_B * Sound.leftvol_B * 256;   // leftvol_B = 0..2
        outvalue_right += out_B * Sound.rightvol_B * 256; // "* 2" ????
    }
    // -128..128 * 0..2 * 256 = -65536 .. +65536
    // FIFO channels -> -65536 .. +65536

    // Add everything, total -> -81920 .. +81920 -- clamp to -65536 .. +65536
    // Clamp to bias / 200h * 65536 ??

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

static u32 min4(u32 a, u32 b, u32 c, u32 d)
{
    u32 x = (a < b) ? a : b;
    u32 y = (c < d) ? c : d;
    return (x < y) ? x : y;
}

u32 GBA_SoundUpdate(u32 clocks)
{
    while (1)
    {
        u32 next_step_clocks = 65536 - Sound.step_clocks;
        u32 next_sample_clocks = 512 - Sound.nextsample_clocks;
        u32 next_freq_clocks = 8 - Sound.nextfreq_clocks;
        u32 next_freq_ch4_clocks = 16 - Sound.nextfreq_ch4_clocks;

        u32 next_clocks = min4(next_step_clocks, next_sample_clocks,
                               next_freq_clocks, next_freq_ch4_clocks);

        if (next_clocks > clocks)
        {
            Sound.step_clocks += clocks;
            Sound.nextsample_clocks += clocks;
            Sound.nextfreq_clocks += clocks;
            Sound.nextfreq_ch4_clocks += clocks;

            // Return clocks to next envelope/sweep/length event
            return 65536 - Sound.step_clocks;
        }

        clocks -= next_clocks;

        Sound.step_clocks += next_clocks;
        Sound.nextsample_clocks += next_clocks;
        Sound.nextfreq_clocks += next_clocks;
        Sound.nextfreq_ch4_clocks += next_clocks;

        // Step event for all channels

        if (Sound.step_clocks >= 65536)
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
                        REG_SOUNDCNT_X &= ~(1 << 0);
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

                        int val = Sound.Chn1.sweepfreq >> Sound.Chn1.sweepshift;

                        if (Sound.Chn1.sweepinc)
                        {
                            Sound.Chn1.sweepfreq += val;
                            if (Sound.Chn1.sweepfreq > 2047)
                            {
                                Sound.Chn1.running = 0;
                                REG_SOUNDCNT_X &= ~(1 << 0);
                            }
                        }
                        else
                        {
                            Sound.Chn1.sweepfreq -= val;
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
                        REG_SOUNDCNT_X &= ~(1 << 1);
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
                    REG_SOUNDCNT_X &= ~(1 << 2);
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
                        REG_SOUNDCNT_X &= ~(1 << 3);
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

        if (Sound.nextfreq_clocks >= 8)
        {
            Sound.nextfreq_clocks = 0;

            // Channel 1

            Sound.Chn1.frequency_steps++;
            if (Sound.Chn1.frequency_steps >= 2048)
            {
                Sound.Chn1.frequency_steps = Sound.Chn1.frequency;

                Sound.Chn1.out_sample =
                    GBA_SquareWave[Sound.Chn1.duty][Sound.Chn1.samplecount];

                Sound.Chn1.samplecount++;
                Sound.Chn1.samplecount %= 32;
            }

            // Channel 2

            Sound.Chn2.frequency_steps++;
            if (Sound.Chn2.frequency_steps >= 2048)
            {
                Sound.Chn2.frequency_steps = Sound.Chn2.frequency;

                Sound.Chn2.out_sample =
                    GBA_SquareWave[Sound.Chn2.duty][Sound.Chn2.samplecount];

                Sound.Chn2.samplecount++;
                Sound.Chn2.samplecount %= 32;
            }

            // Channel 3

            Sound.Chn3.frequency_steps++;
            if (Sound.Chn3.frequency_steps >= 2048)
            {
                Sound.Chn3.frequency_steps = Sound.Chn3.frequency;

                Sound.Chn3.out_sample = GBA_WavePattern[Sound.Chn3.samplecount];

                Sound.Chn3.samplecount++;
                Sound.Chn3.samplecount %= 64;
            }
        }

        // Frequency change of channel 4

        if (Sound.nextfreq_ch4_clocks >= 16)
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
            if (Sound.nextsample_clocks >= 512)
            {
                Sound.nextsample_clocks = 0;
                GBA_SoundMix();
            }
        }
    }
}

void GBA_SoundRegWrite16(u32 address, u16 value)
{
    GBA_ExecutionBreak();

    switch (address)
    {
        case WAVE_RAM + 0:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            Sound.Chn3.wave_ram_buffer[index][0] = value & 0xFF;
            Sound.Chn3.wave_ram_buffer[index][1] = (value >> 8) & 0xFF;
            return;
        }
        case WAVE_RAM + 2:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            Sound.Chn3.wave_ram_buffer[index][2] = value & 0xFF;
            Sound.Chn3.wave_ram_buffer[index][3] = (value >> 8) & 0xFF;
            return;
        }
        case WAVE_RAM + 4:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            Sound.Chn3.wave_ram_buffer[index][4] = value & 0xFF;
            Sound.Chn3.wave_ram_buffer[index][5] = (value >> 8) & 0xFF;
            return;
        }
        case WAVE_RAM + 6:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            Sound.Chn3.wave_ram_buffer[index][6] = value & 0xFF;
            Sound.Chn3.wave_ram_buffer[index][7] = (value >> 8) & 0xFF;
            return;
        }
        case WAVE_RAM + 8:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            Sound.Chn3.wave_ram_buffer[index][8] = value & 0xFF;
            Sound.Chn3.wave_ram_buffer[index][9] = (value >> 8) & 0xFF;
            return;
        }
        case WAVE_RAM + 10:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            Sound.Chn3.wave_ram_buffer[index][10] = value & 0xFF;
            Sound.Chn3.wave_ram_buffer[index][11] = (value >> 8) & 0xFF;
            return;
        }
        case WAVE_RAM + 12:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            Sound.Chn3.wave_ram_buffer[index][12] = value & 0xFF;
            Sound.Chn3.wave_ram_buffer[index][13] = (value >> 8) & 0xFF;
            return;
        }
        case WAVE_RAM + 14:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            Sound.Chn3.wave_ram_buffer[index][14] = value & 0xFF;
            Sound.Chn3.wave_ram_buffer[index][15] = (value >> 8) & 0xFF;
            return;
        }
        default:
            break;
    }

    if (Sound.master_enable == 0)
    {
        if ((address == SOUNDCNT_X) && (value & (1 << 7)))
        {
            GBA_SoundPowerOn();
            Sound.master_enable = 1;
        }

        // While Bit 7 is cleared, both PSG and FIFO sounds are disabled, and
        // all PSG registers (4000060h..4000081h) are reset to zero (and must be
        // re-initialized after re-enabling sound). However, registers 4000082h
        // and 4000088h are kept read/write-able (of which, 4000082h has no
        // function when sound is off, whilst 4000088h does work even when sound
        // is off).

        if ((address != SOUNDBIAS) && (address != SOUNDCNT_H))
            return;
    }

    switch (address)
    {
        // Voice 1
        case SOUND1CNT_L:
            Sound.Chn1.reg[0] = value;
            // Don't update sweep yet...
            return;

        case SOUND1CNT_H:
            Sound.Chn1.reg[1] = value;

            Sound.Chn1.stepsleft = 64 - (value & 0x3F);
            Sound.Chn1.duty = (value >> 6) & 3;

            // Don't update envelope yet...
            if (Sound.Chn1.running) // "Zombie mode"
            {
                if (Sound.Chn1.envactive && (Sound.Chn1.envelope == 0))
                    Sound.Chn1.vol = (Sound.Chn1.vol + 1) & 0xF;
                else if ((value & (1 << 11)) == 0)
                    Sound.Chn1.vol = (Sound.Chn1.vol + 2) & 0xF;

                Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                                        (Sound.Chn1.vol * Sound.leftvol) : 0;
                Sound.rightvol_1 = Sound.Chn1.speakerright ?
                                        (Sound.Chn1.vol * Sound.rightvol) : 0;
            }
            return;
        case SOUND1CNT_X:
            Sound.Chn1.frequency = value & 0x07FF;
            Sound.Chn1.frequency_steps = Sound.Chn1.frequency;

            Sound.Chn1.limittime = (value & (1 << 14));
            if (value & (1 << 15))
            {
                Sound.Chn1.samplecount = 0;

                Sound.Chn1.running = 1;

                // Update sweep
                Sound.Chn1.sweeptime = (Sound.Chn1.reg[0] >> 4) & 0x07;
                Sound.Chn1.sweepstepsleft = Sound.Chn1.sweeptime << 1;
                Sound.Chn1.sweepinc = ((Sound.Chn1.reg[0] & (1 << 3)) == 0);
                Sound.Chn1.sweepshift = Sound.Chn1.reg[0] & 0x07;
                Sound.Chn1.sweepfreq = Sound.Chn1.frequency;

                // Update envelope
                Sound.Chn1.vol = (Sound.Chn1.reg[1] >> 12);
                Sound.Chn1.envelope = (Sound.Chn1.reg[1] >> 8) & 0x07;
                Sound.Chn1.envactive = 1; //Sound.Chn1.envelope != 0;
                Sound.Chn1.envincrease = Sound.Chn1.reg[1] & (1 << 11);
                Sound.Chn1.envstepstochange = Sound.Chn1.envelope << 2;
                Sound.leftvol_1 = Sound.Chn1.speakerleft ?
                                        (Sound.Chn1.vol * Sound.leftvol) : 0;
                Sound.rightvol_1 = Sound.Chn1.speakerright ?
                                        (Sound.Chn1.vol * Sound.rightvol) : 0;

                REG_SOUNDCNT_X |= (1 << 0);
            }
            else
            {
                //REG_SOUNDCNT_X &= ~(1 << 0);
            }
            return;

        // Voice 2
        case SOUND2CNT_L:
            Sound.Chn2.reg[1] = value;

            Sound.Chn2.stepsleft = 64 - (value & 0x3F);
            Sound.Chn2.duty = (value >> 6) & 3;

            // Don't update envelope yet...
            if (Sound.Chn2.running) // "Zombie mode"
            {
                if (Sound.Chn2.envactive && (Sound.Chn2.envelope == 0))
                    Sound.Chn2.vol = (Sound.Chn2.vol + 1) & 0xF;
                else if ((value & (1 << 11)) == 0)
                    Sound.Chn2.vol = (Sound.Chn2.vol + 2) & 0xF;

                Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                                        (Sound.Chn2.vol * Sound.leftvol) : 0;
                Sound.rightvol_2 = Sound.Chn2.speakerright ?
                                        (Sound.Chn2.vol * Sound.rightvol) : 0;
            }
            return;

        case SOUND2CNT_H:
            Sound.Chn2.frequency = value & 0x07FF;
            Sound.Chn2.frequency_steps = Sound.Chn2.frequency;

            Sound.Chn2.limittime = (value & (1 << 14));
            if (value & (1 << 15))
            {
                Sound.Chn2.samplecount = 0;

                Sound.Chn2.running = 1;

                // Update envelope
                Sound.Chn2.vol = (Sound.Chn2.reg[1] >> 12);
                Sound.Chn2.envelope = (Sound.Chn2.reg[1] >> 8) & 0x07;
                Sound.Chn2.envactive = 1; //Sound.Chn2.envelope != 0;
                Sound.Chn2.envincrease = Sound.Chn2.reg[1] & (1 << 11);
                Sound.Chn2.envstepstochange = Sound.Chn2.envelope << 2;
                Sound.leftvol_2 = Sound.Chn2.speakerleft ?
                                        (Sound.Chn2.vol * Sound.leftvol) : 0;
                Sound.rightvol_2 = Sound.Chn2.speakerright ?
                                        (Sound.Chn2.vol * Sound.rightvol) : 0;

                REG_SOUNDCNT_X |= (1 << 1);
            }
            else
            {
                //REG_SOUNDCNT_X &= ~(1 << 1);
            }
            return;

        // Voice 3
        case SOUND3CNT_L:
            Sound.Chn3.reg[0] = value;

            Sound.Chn3.doublesize = (value & BIT(5)) ? 1 : 0;
            Sound.Chn3.buffer_playing = (value & BIT(6)) ? 1 : 0;

            if (value & (1 << 7))
            {
                REG_SOUNDCNT_X |= (1 << 2);
                Sound.Chn3.playing = 1;
            }
            else
            {
                Sound.Chn3.playing = 0;
                REG_SOUNDCNT_X &= ~(1 << 2);
                Sound.Chn3.running = 0;
            }
            return;

        case SOUND3CNT_H:
            Sound.Chn3.reg[1] = value;

            Sound.Chn3.stepsleft = 256 - (value & 0xFF);

            if (value & BIT(15))
            {
                Sound.Chn3.vol = 0xC;
            }
            else
            {
                switch ((value >> 13) & 3)
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
            }

            Sound.leftvol_3 = Sound.Chn3.speakerleft ?
                                        (Sound.Chn3.vol * Sound.leftvol) : 0;
            Sound.rightvol_3 = Sound.Chn3.speakerright ?
                                        (Sound.Chn3.vol * Sound.rightvol) : 0;
            return;

        case SOUND3CNT_X:
            Sound.Chn3.reg[2] = value;

            if (Sound.Chn3.playing)
            {
                Sound.Chn3.frequency = value & 0x07FF;
                Sound.Chn3.frequency_steps = Sound.Chn3.frequency;
            }

            Sound.Chn3.limittime = (value & (1 << 6));

            if (Sound.Chn3.playing) // ?
            {
                Sound.Chn3.samplecount = 0;

                if (value & (1 << 15))
                {
                    //Sound.Chn3.playing = 1; // ?
                    Sound.Chn3.samplecount = 0;
                    REG_SOUNDCNT_X |= (1 << 2);
                    GBA_SoundLoadWave();
                    Sound.Chn3.running = 1;
                }
            }
            return;

        // Voice 4
        case SOUND4CNT_L:
            Sound.Chn4.reg[1] = value;

            Sound.Chn4.stepsleft = 64 - (value & 0x3F);

            // Don't update envelope yet...

            // TODO: No zombie mode in channel 4?

            return;

        case SOUND4CNT_H:
        {
            Sound.Chn4.reg[2] = value;

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

            Sound.Chn4.limittime = (value & (1 << 14));

            if (value & (1 << 15))
            {
                Sound.Chn4.running = 1;

                // Update envelope
                Sound.Chn4.vol = (Sound.Chn4.reg[1] >> 13);
                Sound.Chn4.envelope = (Sound.Chn4.reg[1] >> 8) & 0x07;
                Sound.Chn4.envactive = 1; //Sound.Chn4.envelope != 0;
                Sound.Chn4.envincrease = Sound.Chn4.reg[1] & (1 << 11);
                Sound.Chn4.envstepstochange = Sound.Chn4.envelope << 2;
                Sound.leftvol_4 = Sound.Chn4.speakerleft ?
                                        (Sound.Chn4.vol * Sound.leftvol) : 0;
                Sound.rightvol_4 = Sound.Chn4.speakerright ?
                                        (Sound.Chn4.vol * Sound.rightvol) : 0;

                REG_SOUNDCNT_X |= (1 << 3);
            }
            else
            {
                //REG_SOUNDCNT_X &= ~(1 << 3);
            }
            return;
        }

        // Control
        case SOUNDCNT_L:
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
            Sound.Chn1.speakerright = (value & (1 << 8));
            Sound.Chn1.speakerleft = (value & (1 << 12));
            Sound.Chn2.speakerright = (value & (1 << 9));
            Sound.Chn2.speakerleft = (value & (1 << 13));
            Sound.Chn3.speakerright = (value & (1 << 10));
            Sound.Chn3.speakerleft = (value & (1 << 14));
            Sound.Chn4.speakerright = (value & (1 << 11));
            Sound.Chn4.speakerleft = (value & (1 << 15));
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

        case SOUNDCNT_H:
        {
            static const int vol[4] = { 0, 1, 2, 0 };
            Sound.PSG_master_volume = vol[value & 3];

            Sound.FifoA.speakerright = (value & BIT(8));
            Sound.FifoA.speakerleft = (value & BIT(9));
            Sound.FifoB.speakerright = (value & BIT(12));
            Sound.FifoB.speakerleft = (value & BIT(13));

            Sound.FifoA.vol = (value & BIT(2)) ? 1 : 2;
            Sound.leftvol_A = Sound.FifoA.speakerleft ? Sound.FifoA.vol : 0;
            Sound.rightvol_A = Sound.FifoA.speakerright ? Sound.FifoA.vol : 0;

            Sound.FifoB.vol = (value & BIT(3)) ? 1 : 2;
            Sound.leftvol_B = Sound.FifoB.speakerleft ? Sound.FifoB.vol : 0;
            Sound.rightvol_B = Sound.FifoB.speakerright ? Sound.FifoB.vol : 0;

            Sound.FifoA.timer = (value & BIT(10)) ? 1 : 0;
            Sound.FifoB.timer = (value & BIT(14)) ? 1 : 0;

            if (value & BIT(11))
            {
                Sound.FifoA.cursample = 0;
                Sound.FifoA.cursamplewrite = 0;
                Sound.FifoA.datalen = 0;
                memset(Sound.FifoA.buffer, 0, sizeof(Sound.FifoA.buffer));
            }

            if (value & BIT(15))
            {
                Sound.FifoB.cursample = 0;
                Sound.FifoB.cursamplewrite = 0;
                Sound.FifoB.datalen = 0;
                memset(Sound.FifoB.buffer, 0, sizeof(Sound.FifoB.buffer));
            }
            return;
        }

        case SOUNDCNT_X:
            if ((value & (1 << 7)) == 0)
            {
                GBA_SoundPowerOff();
                Sound.master_enable = 0;
            }
            return;

        case FIFO_A + 0:
        {
            int index = Sound.FifoA.cursamplewrite;
            Sound.FifoA.buffer[index + 0] = value & 0xFF;
            Sound.FifoA.buffer[index + 1] = (value >> 8) & 0xFF;
            Sound.FifoA.datalen += 2;
            return;
        }
        case FIFO_A + 2:
        {
            int index = Sound.FifoA.cursamplewrite;
            Sound.FifoA.buffer[index + 2] = value & 0xFF;
            Sound.FifoA.buffer[index + 3] = (value >> 8) & 0xFF;
            Sound.FifoA.datalen += 2;
            Sound.FifoA.cursamplewrite += 4;
            Sound.FifoA.cursamplewrite &= FIFO_BUFFER_SIZE - 1;
            return;
        }
        case FIFO_B + 0:
        {
            int index = Sound.FifoB.cursamplewrite;
            Sound.FifoB.buffer[index + 0] = value & 0xFF;
            Sound.FifoB.buffer[index + 1] = (value >> 8) & 0xFF;
            Sound.FifoB.datalen += 2;
            return;
        }
        case FIFO_B + 2:
        {
            int index = Sound.FifoB.cursamplewrite;
            Sound.FifoB.buffer[index + 2] = value & 0xFF;
            Sound.FifoB.buffer[index + 3] = (value >> 8) & 0xFF;
            Sound.FifoB.datalen += 2;
            Sound.FifoB.cursamplewrite += 4;
            Sound.FifoB.cursamplewrite &= FIFO_BUFFER_SIZE - 1;
            return;
        }

        case SOUNDBIAS:
            return;

        default:
            Debug_DebugMsgArg("GBA Sound: [%08x]=%04x (?)\n", address, value);
            GBA_ExecutionBreak();
            return;
    }
}

u16 GBA_SoundRegRead16(u32 address)
{
    switch (address)
    {
        case WAVE_RAM + 0:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            u16 low = Sound.Chn3.wave_ram_buffer[index][0];
            u16 high = Sound.Chn3.wave_ram_buffer[index][1];
            return (high << 8) | low;
        }
        case WAVE_RAM + 2:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            u16 low = Sound.Chn3.wave_ram_buffer[index][2];
            u16 high = Sound.Chn3.wave_ram_buffer[index][3];
            return (high << 8) | low;
        }
        case WAVE_RAM + 4:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            u16 low = Sound.Chn3.wave_ram_buffer[index][4];
            u16 high = Sound.Chn3.wave_ram_buffer[index][5];
            return (high << 8) | low;
        }
        case WAVE_RAM + 6:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            u16 low = Sound.Chn3.wave_ram_buffer[index][6];
            u16 high = Sound.Chn3.wave_ram_buffer[index][7];
            return (high << 8) | low;
        }
        case WAVE_RAM + 8:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            u16 low = Sound.Chn3.wave_ram_buffer[index][8];
            u16 high = Sound.Chn3.wave_ram_buffer[index][9];
            return (high << 8) | low;
        }
        case WAVE_RAM + 10:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            u16 low = Sound.Chn3.wave_ram_buffer[index][10];
            u16 high = Sound.Chn3.wave_ram_buffer[index][11];
            return (high << 8) | low;
        }
        case WAVE_RAM + 12:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            u16 low = Sound.Chn3.wave_ram_buffer[index][12];
            u16 high = Sound.Chn3.wave_ram_buffer[index][13];
            return (high << 8) | low;
        }
        case WAVE_RAM + 14:
        {
            int index = Sound.Chn3.buffer_playing ^ 1;
            if (Sound.master_enable == 0)
                index = 1;
            u16 low = Sound.Chn3.wave_ram_buffer[index][14];
            u16 high = Sound.Chn3.wave_ram_buffer[index][15];
            return (high << 8) | low;
        }
        default:
            // This shouldn't happen
            Debug_DebugMsgArg("GBA Sound: Read [%08x] (?)\n", address);
            GBA_ExecutionBreak();
            return 0;
    }

    return 0;
}

void GBA_SoundTimerCheck(u32 number)
{
    if (Sound.FifoA.timer == number)
    {
        Sound.FifoA.running = 0;
        if (Sound.FifoA.datalen > 0)
        {
            Sound.FifoA.datalen--;
            Sound.FifoA.out_sample = Sound.FifoA.buffer[Sound.FifoA.cursample++];
            Sound.FifoA.cursample &= FIFO_BUFFER_SIZE - 1;
            Sound.FifoA.running = 1;
        }
        if (Sound.FifoA.datalen <= 16) // Request data!!!
            GBA_DMASoundRequestData(1, 0);
    }

    if (Sound.FifoB.timer == number)
    {
        Sound.FifoB.running = 0;
        if (Sound.FifoB.datalen > 0)
        {
            Sound.FifoB.datalen--;
            Sound.FifoB.out_sample = Sound.FifoB.buffer[Sound.FifoB.cursample++];
            Sound.FifoB.cursample &= FIFO_BUFFER_SIZE - 1;
            Sound.FifoB.running = 1;
        }
        if (Sound.FifoB.datalen <= 16) // Request data!!!
            GBA_DMASoundRequestData(0, 1);
    }
}

void GBA_SoundEnd(void)
{

}

//-----------------------------------------------

void GBA_SoundGetConfig(int *vol, int *chn_flags)
{
    *vol = EmulatorConfig.volume;
    *chn_flags = EmulatorConfig.chn_flags;
}

void GBA_SoundSetConfig(int vol, int chn_flags)
{
    EmulatorConfig.volume = vol;
    EmulatorConfig.chn_flags = chn_flags;
}

//-------------------------------------------------

int gba_debug_get_psg_vol(int chan)
{
    switch (chan)
    {
        case 1:
            return Sound.Chn1.vol;
        case 2:
            return Sound.Chn2.vol;
        case 3:
            return Sound.Chn3.vol;
        case 4:
            return Sound.Chn4.vol;
        default:
            return 0;
    }
}
