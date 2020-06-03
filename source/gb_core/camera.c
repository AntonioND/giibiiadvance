// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdlib.h>
#include <string.h>

#include "../build_options.h"
#include "../config.h"
#include "../debug_utils.h"
#include "../general_utils.h"
#include "../webcam_utils.h"

#include "cpu.h"
#include "gameboy.h"

//------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------

// Webcam image (exposed in gc_core/camera.h, values in the range 0-255)
int gb_camera_webcam_output[GBCAM_SENSOR_W][GBCAM_SENSOR_H];
// Image processed by the retina chip
static int gb_cam_retina_output_buf[GBCAM_SENSOR_W][GBCAM_SENSOR_H];

void GB_CameraEnd(void)
{
    Webcam_End();
}

int GB_CameraInit(void)
{
    return Webcam_Init();
}

void GB_CameraWebcamCapture(void)
{
    Webcam_GetFrame();
}

//----------------------------------------------------------------

static int gb_camera_clock_counter = 0;

void GB_CameraClockCounterReset(void)
{
    gb_camera_clock_counter = 0;
}

static int GB_CameraClockCounterGet(void)
{
    return gb_camera_clock_counter;
}

static void GB_CameraClockCounterSet(int new_reference_clocks)
{
    gb_camera_clock_counter = new_reference_clocks;
}

void GB_CameraUpdateClocksCounterReference(int reference_clocks)
{
    if (GameBoy.Emulator.MemoryController != MEM_CAMERA)
        return;

    int increment_clocks = reference_clocks - GB_CameraClockCounterGet();

    _GB_CAMERA_CART_ *cam = &GameBoy.Emulator.CAM;

    if (cam->clocks_left > 0)
    {
        cam->clocks_left -= increment_clocks;

        if (cam->clocks_left <= 0)
        {
            cam->reg[0] &= ~BIT(0); // Ready
            cam->clocks_left = 0;
        }
    }

    GB_CameraClockCounterSet(reference_clocks);
}

int GB_CameraGetClocksToNextEvent(void)
{
    if (GameBoy.Emulator.MemoryController != MEM_CAMERA)
        return 0x7FFFFFFF;

    _GB_CAMERA_CART_ *cam = &GameBoy.Emulator.CAM;

    if (cam->clocks_left == 0)
        return 0x7FFFFFFF;

    return cam->clocks_left;
}

//----------------------------------------------------------------------------

static int gb_clamp_int(int min, int value, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;

    return value;
}

static int gb_min_int(int a, int b)
{
    return (a < b) ? a : b;
}

static int gb_max_int(int a, int b)
{
    return (a > b) ? a : b;
}

static u32 gb_cam_matrix_process(u32 value, int x, int y)
{
    _GB_CAMERA_CART_ *cam = &GameBoy.Emulator.CAM;

    x = x & 3;
    y = y & 3;

    int base = 6 + (y * 4 + x) * 3;

    u32 r0 = cam->reg[base + 0];
    u32 r1 = cam->reg[base + 1];
    u32 r2 = cam->reg[base + 2];

    if (value < r0)
        return 0x00;
    if (value < r1)
        return 0x40;
    if (value < r2)
        return 0x80;

    return 0xC0;
}

static void GB_CameraTakePicture(void)
{
    _GB_CAMERA_CART_ *cam = &GameBoy.Emulator.CAM;

    //------------------------------------------------

    // Get webcam image
    // ----------------

    GB_CameraWebcamCapture();

    //------------------------------------------------

    // Get configuration
    // -----------------

    // Register 0
    u32 P_bits = 0;
    u32 M_bits = 0;

    switch ((cam->reg[0] >> 1) & 3)
    {
        case 0:
            P_bits = 0x00;
            M_bits = 0x01;
            break;
        case 1:
            P_bits = 0x01;
            M_bits = 0x00;
            break;
        case 2:
        case 3:
            P_bits = 0x01;
            M_bits = 0x02;
            break;
        default:
            break;
    }

    // Register 1
    u32 N_bit = (cam->reg[1] & BIT(7)) >> 7;
    u32 VH_bits = (cam->reg[1] & (BIT(6) | BIT(5))) >> 5;

    // Registers 2 and 3
    u32 EXPOSURE_bits = cam->reg[3] | (cam->reg[2] << 8);

    // Register 4
    const float edge_ratio_lut[8] = {
        0.50, 0.75, 1.00, 1.25, 2.00, 3.00, 4.00, 5.00
    };

    float EDGE_alpha = edge_ratio_lut[(cam->reg[4] & 0x70) >> 4];

    u32 E3_bit = (cam->reg[4] & BIT(7)) >> 7;
    u32 I_bit = (cam->reg[4] & BIT(3)) >> 3;

    //------------------------------------------------

    // Calculate timings
    // -----------------

    cam->clocks_left = 4 * (32446 + (N_bit ? 0 : 512) + 16 * EXPOSURE_bits);

    //------------------------------------------------

    // Sensor handling
    // ---------------

    // Copy webcam buffer to sensor buffer applying color correction and
    // exposure time
    for (int i = 0; i < GBCAM_SENSOR_W; i++)
    {
        for (int j = 0; j < GBCAM_SENSOR_H; j++)
        {
            int value = gb_camera_webcam_output[i][j];
            value = (value * EXPOSURE_bits)
                    / EmulatorConfig.gbcam_exposure_reference;

            value = 128 + (((value - 128) * 1) / 8); // "adapt" to "3.1"/5.0 V
            gb_cam_retina_output_buf[i][j] = gb_clamp_int(0, value, 255);
        }
    }

    if (I_bit) // Invert image
    {
        for (int i = 0; i < GBCAM_SENSOR_W; i++)
        {
            for (int j = 0; j < GBCAM_SENSOR_H; j++)
            {
                gb_cam_retina_output_buf[i][j] =
                        255 - gb_cam_retina_output_buf[i][j];
            }
        }
    }

    // Make signed
    for (int i = 0; i < GBCAM_SENSOR_W; i++)
    {
        for (int j = 0; j < GBCAM_SENSOR_H; j++)
        {
            gb_cam_retina_output_buf[i][j] =
                    gb_cam_retina_output_buf[i][j] - 128;
        }
    }

    int temp_buf[GBCAM_SENSOR_W][GBCAM_SENSOR_H];

    u32 filtering_mode = (N_bit << 3) | (VH_bits << 1) | E3_bit;
    switch (filtering_mode)
    {
        // 1-D filtering
        case 0x0:
        {
            for (int j = 0; j < GBCAM_SENSOR_H; j++)
            {
                for (int i = 0; i < GBCAM_SENSOR_W; i++)
                {
                    temp_buf[i][j] = gb_cam_retina_output_buf[i][j];
                }
            }

            for (int j = 0; j < GBCAM_SENSOR_H; j++)
            {
                for (int i = 0; i < GBCAM_SENSOR_W; i++)
                {
                    int ms = temp_buf[i][gb_min_int(j + 1, GBCAM_SENSOR_H - 1)];
                    int px = temp_buf[i][j];

                    int value = 0;
                    if (P_bits & BIT(0))
                        value += px;
                    if (P_bits & BIT(1))
                        value += ms;
                    if (M_bits & BIT(0))
                        value -= px;
                    if (M_bits & BIT(1))
                        value -= ms;
                    gb_cam_retina_output_buf[i][j] =
                            gb_clamp_int(-128, value, 127);
                }
            }
            break;
        }

        // 1-D filtering + Horiz. enhancement : P + {2P - (MW + ME)} * alpha
        case 0x2:
        {
            for (int j = 0; j < GBCAM_SENSOR_H; j++)
            {
                for (int i = 0; i < GBCAM_SENSOR_W; i++)
                {
                    int mw = gb_cam_retina_output_buf[gb_max_int(0, i - 1)][j];
                    int me = gb_cam_retina_output_buf
                            [gb_min_int(i + 1, GBCAM_SENSOR_W - 1)][j];
                    int px = gb_cam_retina_output_buf[i][j];

                    temp_buf[i][j] = gb_clamp_int(0,
                            px + ((2 * px - mw - me) * EDGE_alpha),
                            255);
                }
            }
            for (int j = 0; j < GBCAM_SENSOR_H; j++)
            {
                for (int i = 0; i < GBCAM_SENSOR_W; i++)
                {
                    int ms = temp_buf[i][gb_min_int(j + 1, GBCAM_SENSOR_H - 1)];
                    int px = temp_buf[i][j];

                    int value = 0;
                    if (P_bits & BIT(0))
                        value += px;
                    if (P_bits & BIT(1))
                        value += ms;
                    if (M_bits & BIT(0))
                        value -= px;
                    if (M_bits & BIT(1))
                        value -= ms;
                    gb_cam_retina_output_buf[i][j] =
                            gb_clamp_int(-128, value, 127);
                }
            }
            break;
        }

        // 2D enhancement : P + {4P - (MN + MS + ME + MW)} * alpha
        case 0xE:
        {
            for (int j = 0; j < GBCAM_SENSOR_H; j++)
            {
                for (int i = 0; i < GBCAM_SENSOR_W; i++)
                {
                    int ms = gb_cam_retina_output_buf[i]
                            [gb_min_int(j + 1, GBCAM_SENSOR_H - 1)];
                    int mn = gb_cam_retina_output_buf[i][gb_max_int(0, j - 1)];
                    int mw = gb_cam_retina_output_buf[gb_max_int(0, i - 1)][j];
                    int me = gb_cam_retina_output_buf
                            [gb_min_int(i + 1, GBCAM_SENSOR_W - 1)][j];
                    int px = gb_cam_retina_output_buf[i][j];

                    temp_buf[i][j] = gb_clamp_int(-128,
                            px + ((4 * px - mw - me - mn - ms) * EDGE_alpha),
                            127);
                }
            }
            for (int j = 0; j < GBCAM_SENSOR_H; j++)
            {
                for (int i = 0; i < GBCAM_SENSOR_W; i++)
                {
                    gb_cam_retina_output_buf[i][j] = temp_buf[i][j];
                }
            }
            break;
        }

        // In my GB Camera cartridge this is always the same color. The
        // datasheet of the sensor doesn't have this configuration documented.
        // Maybe this is a bug?
        case 0x1:
        {
            for (int j = 0; j < GBCAM_SENSOR_H; j++)
            {
                for (int i = 0; i < GBCAM_SENSOR_W; i++)
                {
                    gb_cam_retina_output_buf[i][j] = 0;
                }
            }
            break;
        }

        // Ignore filtering
        default:
        {
            Debug_DebugMsgArg("Unsupported GB Cam mode: 0x%X\n%02X %02X %02X "
                              "%02X %02X %02X",
                              filtering_mode, cam->reg[0], cam->reg[1],
                              cam->reg[2], cam->reg[3], cam->reg[4],
                              cam->reg[5]);
            break;
        }
    }

    // Make unsigned
    for (int j = 0; j < GBCAM_SENSOR_H; j++)
    {
        for (int i = 0; i < GBCAM_SENSOR_W; i++)
        {
            gb_cam_retina_output_buf[i][j] =
                    gb_cam_retina_output_buf[i][j] + 128;
        }
    }

    //------------------------------------------------

    // Controller handling
    // -------------------

    int fourcolorsbuffer[GBCAM_W][GBCAM_H]; // Buffer after controller matrix

    // Convert to Game Boy colors using the controller matrix
    for (int j = 0; j < GBCAM_H; j++)
    {
        for (int i = 0; i < GBCAM_W; i++)
        {
            int v;
            v = gb_cam_retina_output_buf[i][j + (GBCAM_SENSOR_EXTRA_LINES / 2)];
            fourcolorsbuffer[i][j] = gb_cam_matrix_process(v, i, j);
        }
    }

    // Convert to tiles
    u8 finalbuffer[14][16][16]; // Final buffer
    memset(finalbuffer, 0, sizeof(finalbuffer));
    for (int j = 0; j < GBCAM_H; j++)
    {
        for (int i = 0; i < GBCAM_W; i++)
        {
            u8 outcolor = 3 - (fourcolorsbuffer[i][j] >> 6);

            u8 *tile_base = finalbuffer[j >> 3][i >> 3];
            tile_base = &tile_base[(j & 7) * 2];

            if (outcolor & 1)
                tile_base[0] |= 1 << (7 - (7 & i));
            if (outcolor & 2)
                tile_base[1] |= 1 << (7 - (7 & i));
        }
    }

    // Copy to cart ram...
    memcpy(&(GameBoy.Memory.ExternRAM[0][0xA100 - 0xA000]), finalbuffer,
           sizeof(finalbuffer));
}

int GB_CameraReadRegister(int address)
{
    GB_CameraUpdateClocksCounterReference(GB_CPUClockCounterGet());

    _GB_CAMERA_CART_ *cam = &GameBoy.Emulator.CAM;

    int reg = (address & 0x7F); // All registers are mirrored

    if (reg == 0)
        return cam->reg[0]; // 'Ready' register

    // Others are write-only and they return 0.
    return 0x00;
}

void GB_CameraWriteRegister(int address, int value)
{
    _GB_CAMERA_CART_ *cam = &GameBoy.Emulator.CAM;

    int reg = (address & 0x7F); // Mirror

    if (reg < 0x36)
    {
        //Debug_LogMsgArg("Cam [0x%02X]=0x%02X", (u32)(u16)(address & 0xFF),
        //                (u32)(u8)value);

        cam->reg[reg] = value;

        if (reg == 0) // Take picture...
        {
            cam->reg[0] &= 7;
            if (value & BIT(0))
                GB_CameraTakePicture();
        }
    }
    else
    {
        Debug_DebugMsgArg("Camera wrote to invalid register: [%04x] = %02X",
                          address, value);
    }
}

//----------------------------------------------------------------

int GB_MapperIsGBCamera(void)
{
    return (GameBoy.Emulator.MemoryController == MEM_CAMERA);
}

int GB_CameraWebcamImageGetPixel(int x, int y)
{
    return gb_camera_webcam_output[x][y + (GBCAM_SENSOR_EXTRA_LINES / 2)];
}

int GB_CameraRetinaProcessedImageGetPixel(int x, int y)
{
    // 4 extra rows, 2 on each border
    return gb_cam_retina_output_buf[x][y + (GBCAM_SENSOR_EXTRA_LINES / 2)];
}
