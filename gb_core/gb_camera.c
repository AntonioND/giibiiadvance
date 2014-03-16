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

#include "../build_options.h"
#include "../general_utils.h"
#include "../debug_utils.h"

#include "gameboy.h"

//------------------------------------------------------------------------------
//
//          WARNING: THIS FILE IS ALMOST EVERYTHING GUESSWORK!
//
//           Some information taken from M64282FP datasheet.
//
//------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>

#ifndef NO_CAMERA_EMULATION

#include <opencv/cv.h>
#include <opencv/highgui.h>

static CvCapture * capture;
//static int gbcamwindow = 0;
static int gbcamenabled = 0;
static int gbcamerafactor = 1;
#endif

//----------------------------------------------------------------------------

static int gb_camera_retina_output[16*8][14*8];
static int gb_cam_shoot_buf[16*8][14*8];

void GB_CameraEnd(void)
{
#ifndef NO_CAMERA_EMULATION
    if(gbcamenabled == 0) return;

    //if(gbcamwindow) cvDestroyWindow("GiiBii - Webcam Output");

    cvReleaseCapture(&capture);

    //gbcamwindow = 0;
    gbcamenabled = 0;
#endif
}

int GB_CameraInit(int createwindow)
{
#ifndef NO_CAMERA_EMULATION
    if(gbcamenabled) return 1;

    capture = cvCaptureFromCAM(CV_CAP_ANY); // TODO : Select camera from configuration file?
    if(!capture)
    {
        Debug_ErrorMsgArg("OpenCV ERROR: capture is NULL\nNo camera detected?");
        return 0;
    }

    gbcamenabled = 1;

    //gbcamwindow = createwindow;

    //if(gbcamwindow) cvNamedWindow("GiiBii - Webcam Output",CV_WINDOW_AUTOSIZE);

    // TODO : Select resolution from configuration file?
    int w = 160; //This is the minimum standard resolution that works with GB Camera (128x112)
    int h = 120;
    while(1) // Get the smaller valid resolution
    {
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, w);
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, h);

        w = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
        h = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);

        if( (w >= 128) && (h >= 112) )
        {
            break;
        }
        else
        {
            w *= 2;
            h *= 2;
        }

        if(w >= 1024) // too big, stop now.
            break;
    }

    IplImage * frame = cvQueryFrame(capture);
    if(!frame)
    {
        Debug_ErrorMsgArg("OpenCV ERROR: frame is NULL");
        GB_CameraEnd();
        return 0;
    }
    else
    {
        Debug_LogMsgArg("Camera resolution is: %dx%d",frame->width,frame->height);
        if( (frame->width < 16*8) || (frame->height < 14*8) )
        {
            Debug_ErrorMsgArg("Camera resolution is too small..");
            GB_CameraEnd();
            return 0;
        }

        int xfactor = frame->width / (16*8);
        int yfactor = frame->height / (14*8);

        gbcamerafactor = (xfactor > yfactor) ? yfactor : xfactor; //min
    }

    return 1;
#else
    Debug_ErrorMsgArg("GiiBiiAdvance was compiled without camera features.");
    return 0;
#endif
}

void GB_CameraShoot(void)
{
    int i, j;

#ifndef NO_CAMERA_EMULATION
    //Get image
    if(gbcamenabled == 0)
    {
#endif
        //just some random output...
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++) gb_camera_retina_output[i][j] = rand();
#ifndef NO_CAMERA_EMULATION
    }
    else
    {
        //get image from webcam
        IplImage * frame = cvQueryFrame(capture);
        if(!frame)
        {
            GB_CameraEnd();
            Debug_ErrorMsgArg("OpenCV ERROR: frame is null...\n");
        }
        else
        {
            if(frame->depth == 8 && frame->nChannels == 3)
            {

                for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
                {
                    u8 * data = &( ((u8*)frame->imageData)
                            [((j*gbcamerafactor)*frame->widthStep)+((i*gbcamerafactor)*3)] );
                    s16 value = ((u32)data[0]+(u32)data[1]+(u32)data[2])/3;
                    gb_camera_retina_output[i][j] = value;
                    //if(gbcamwindow) { data[0] = ~data[0]; data[1] = ~data[1]; data[2] = ~data[2]; }
                }
            }
            else
            {
                Debug_ErrorMsgArg("Invalid camera output. Depth = %d bits | Channels = %d",
                                  frame->depth,frame->nChannels);
            }
            //if(gbcamwindow) cvShowImage("GiiBii - Webcam Output", frame );
        }
    }
#endif
}

//----------------------------------------------------------------------------

static inline int gb_clamp_int(int min, int value, int max)
{
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

static inline int gb_min_int(int a, int b)
{
    return (a < b) ? a : b;
}

static inline int gb_max_int(int a, int b)
{
    return (a > b) ? a : b;
}


static void GB_CameraTakePicture(u32 exposure_time, int offset, int dithering_enabled, int contrast) // contrast: 0..100
{
    GB_CameraShoot();

    int inbuffer[16*8][14*8]; // buffer after aplying effects
    u8 readybuffer[14][16][16];
    int i, j;

    //Apply 3x3 programmable 1-D filtering
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
#if 0
        // Horizontal edge
        if( (i > 0) && (i < 16*8-1))
            gb_cam_shoot_buf[i][j] =
                gb_clamp_int( 0, (2 * gb_camera_retina_output[i][j]) -
                ( (gb_camera_retina_output[i-1][j] + gb_camera_retina_output[i+1][j]) / 2 ),
                             255);
        else
            gb_cam_shoot_buf[i][j] = gb_camera_retina_output[i][j];
#else
        // 2D edge
        int ms = gb_camera_retina_output[i][gb_min_int(j+1,14*8-1)];
        int mn = gb_camera_retina_output[i][gb_max_int(0,j-1)];
        int mw = gb_camera_retina_output[gb_max_int(0,i-1)][j];
        int me = gb_camera_retina_output[gb_min_int(i+1,16*8-1)][j];
        gb_cam_shoot_buf[i][j] =
            gb_clamp_int( 0, (3*gb_camera_retina_output[i][j]) - (ms+mn+mw+me)/2 , 255);
#endif
    }

    //Apply exposure time
#if 0
    //exposure_time = (exposure_time * 16) / 10;
    if(exposure_time >= 0x8000)
    {
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
        {
            int result = 255 - gb_cam_shoot_buf[i][j];
            result = 255 - ( ( result * (0xFFFF-exposure_time) ) / 0x8000 );
            gb_cam_shoot_buf[i][j] = gb_clamp_int(0,result,255);
        }
    }
    else
    {
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
        {
            int result = gb_cam_shoot_buf[i][j];
            result = ( ( result * exposure_time ) / 0x8000 );
            gb_cam_shoot_buf[i][j] = gb_clamp_int(0,result,255);
        }
    }
#else
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        int result = gb_cam_shoot_buf[i][j];
        result = ( ( result * exposure_time ) / 0x6000 );
        gb_cam_shoot_buf[i][j] = gb_clamp_int(0,result,255);
    }
#endif

#if 0
    //Apply offset
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        gb_cam_shoot_buf[i][j] = gb_clamp_int(0,
                            gb_cam_shoot_buf[i][j] + offset,
                            255);
    }
#endif

    //Apply contrast
    double c;
    if(contrast <= 50)
    {
        c = (double)(50-contrast);
        c /= 60.0;
        c *= c;
        c = (double)1.0 - c;
    }
    else
    {
        c = (double)(contrast-50);
        c /= 60.0;
        c *= c;
        c *= c;
        c *= 100;
        c = (double)1.0 + c;
    }

    //_GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;
    //extern int WinIDMain;
    //char caption[60];
    //s_snprintf(caption,sizeof(caption),"%d %f |  %04X", contrast,c,cam->reg[3] | (cam->reg[2]<<8));
    //WH_SetCaption(WinIDMain,caption);

    //double c = (100.0 + (double)contrast__) / 100.0;
    //c *= c;
    int contrast_lookup[256];
    double newValue = 0;
    for(i = 0; i < 256; i++)
    {
        newValue = (double)i;
        newValue /= 255.0;
        newValue -= 0.5;
        newValue *= c;
        newValue += 0.5;
        newValue *= 255;

        newValue = gb_clamp_int(0,newValue,255);
        contrast_lookup[i] = (int)newValue;
    }

    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
        gb_cam_shoot_buf[i][j] = contrast_lookup[gb_cam_shoot_buf[i][j]];


    if(dithering_enabled)
    {
#if 0
        // Floyd–Steinberg dithering - Wikipedia
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
        {
            int oldpixel = gb_cam_shoot_buf[i][j]; // oldpixel  := pixel[x][y]
            int newpixel = oldpixel & 0xC0;   // newpixel  := find_closest_palette_color(oldpixel)
            inbuffer[i][j] = (u8)newpixel;    // pixel[x][y]  := newpixel
            int error = oldpixel - newpixel;  // quant_error  := oldpixel - newpixel
            //pixel[x+1][y  ] := pixel[x+1][y  ] + 7/16 * quant_error
            //pixel[x-1][y+1] := pixel[x-1][y+1] + 3/16 * quant_error
            //pixel[x  ][y+1] := pixel[x  ][y+1] + 5/16 * quant_error
            //pixel[x+1][y+1] := pixel[x+1][y+1] + 1/16 * quant_error
            if(i < (16*8-1))
            {
                gb_cam_shoot_buf[i+1][j] = gb_clamp_int(0,gb_cam_shoot_buf[i+1][j] + (7*error)/16,255);
                if(j < (14*8-1))
                    gb_cam_shoot_buf[i+1][j+1] = gb_clamp_int(0,gb_cam_shoot_buf[i+1][j+1] + (1*error)/16,255);
            }
            if(j < (14*8-1))
            {
                gb_cam_shoot_buf[i][j+1] = gb_clamp_int(0,gb_cam_shoot_buf[i][j+1] + (5*error)/16,255);
                if(i > 0)
                    gb_cam_shoot_buf[i-1][j+1] = gb_clamp_int(0,gb_cam_shoot_buf[i-1][j+1] + (3*error)/16,255);
            }
        }
#else
        // Standard ordered dithering. Bayer threshold matrix - http://bisqwit.iki.fi/story/howto/dither/jy/

        //const int matrix[64] =  { // divided by 64
        //    0, 48, 12, 60,  3, 51, 15, 63,
        //    32, 16, 44, 28, 35, 19, 47, 31,
        //     8, 56,  4, 52, 11, 59,  7, 55,
        //    40, 24, 36, 20, 43, 27, 39, 23,
        //     2, 50, 14, 62,  1, 49, 13, 61,
        //    34, 18, 46, 30, 33, 17, 45, 29,
        //    10, 58,  6, 54,  9, 57,  5, 53,
        //    42, 26, 38, 22, 41, 25, 37, 21,
        //};
        //const int matrix_div = 64;

        const int matrix[16] =  { // divided by 256
             15, 135,  45, 165,
            195,  75, 225, 105,
             60, 180,  30, 150,
            240, 120, 210,  90
        };
        const int matrix_div = 256;

        int treshold = 256/4;
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
        {
            int oldpixel = gb_cam_shoot_buf[i][j];
            //int matrix_value = matrix[(i & 7) + ((j & 7) << 3)];
            int matrix_value = matrix[(i & 3) + ((j & 3) << 2)];
            int newpixel = gb_clamp_int(0, oldpixel + ( (matrix_value * treshold) / matrix_div), 255 ) & 0xC0;
            inbuffer[i][j] = (u8)newpixel;
        }
#endif
    }
    else
    {
        // No dithering
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
        {
            inbuffer[i][j] = (u8)(gb_cam_shoot_buf[i][j] & 0xC0);
        }
    }

    //Convert to tiles
    memset(readybuffer,0,sizeof(readybuffer));
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        u8 outcolor = 3 - (inbuffer[i][j] >> 6);

        u8 * tile_base = readybuffer[j>>3][i>>3];

        tile_base = &tile_base[(j&7)*2];

        if(outcolor & 1) tile_base[0] |= 1<<(7-(7&i));
        if(outcolor & 2) tile_base[1] |= 1<<(7-(7&i));
    }

    //Copy to cart ram...
    memcpy(&(GameBoy.Memory.ExternRAM[0][0xA100-0xA000]),readybuffer,sizeof(readybuffer));
}

int GB_CameraReadRegister(int address)
{
    if(address == 0xA000) return 0; //'ready' register -- always ready in the emulator
    return 0xFF; //others are write-only? they are never read so the value doesn't matter...

    // TODO : Registers C0 and C1 of M64282FP - Exposure time (each step is 16 µs) -- emulate delay
}

void GB_CameraWriteRegister(int address, int value)
{
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    int reg = address-0xA000;

    //printf("[%04X]=%02X\n",(u32)(u16)address,(u32)(u8)value);
    if(reg < 0x36)
    {
        //Debug_LogMsgArg("Cam [0x%02X]=0x%02X",(u32)(u16)(address&0xFF),(u32)(u8)value);

/***************************************************************************************************

                            GB CAMERA REGISTERS
                            -------------------

Register 0
----------

    Execute command?
    Take picture when writing 03h

Register 1
----------

    Changes all the time when camera is working.
    Values seen: 00, 0A, 20, 24, 28, E4, E8.

    This corresponds to this register of M64282FP:

          No. Address   7   6   5   4   3   2   1   0
          0   000      Z1  Z0  O5  O4  O3  O2  O1  O0

          Symbol   Bit Assignment  Operation
          Z        2 bits          Zero point calibration ( Set the dark level output signal to Vref )
          O        6 bits          Output reference voltage ( In both plus and minus direction )

Registers 2 and 3
-----------------

    Exposure time (2 MSB, 3 LSB)
    They correspond to registers C0 and C1 of M64282FP - Exposure time (each step is 16 us)

Register 4
----------

    At boot is written (in hex):

          05
          01 02 03 04 05 06 07
          03 04 05 06 07
          23 24 25 26 27
          23 24 25 26 27
          23 24 25 26 27
          03 04 05 06 07
          05
          08 09 0A 0B 0C 0D 0E 0F
          07 - and stays like that forever (sometimes writes 07 again?)

    This corresponds to this register of M64282FP:

         No. Address   7   6   5   4   3   2   1   0
         7   111      E3  E2  E1  E0   I  V2  V1  V0

         Symbol   Bit Assignment  Operation
         E        4 bits          Edge enhancement ratio
         I        1 bit           Select inverted/non-inverted output
         V        3 bits          Output node bias voltage (Vref)

    At the end:

        E = 0000 - Edge Enhancement Ratio (alpha) = 50% in edge enhancement mode (normal image sensing operation).
                   100 % means the same level as the P signal, which is the signal of the central pixel in the
                   3x3 processing kernel. Changed during boot for calibrating?
        I = 0    - Normal (non-inverted) output. Changed during boot for calibrating?
        V = 111  - Output node voltage Vref = 3.5 V. Changed during boot for calibrating?

Register 5
----------

    At boot is written (in hex):

          80
          BF A0 B0 B8 BC BE
          BF BF
          3F
          BF A0 B0 B8 BC BE
          BF BF
          BF A0 B0 B8 BC BE
          BF BF
          BF A0 B0 B8 BC BE
          BF BF
          BF A0 B0 B8 BC BE
          BF BF
          BF A0 B0 B8 BC BE
          BF BF
          80
          BF A0 B0 B8 BC BE
          BF BF
          3F
          BF -- and stays like that forever (sometimes writes BF again?)

    This corresponds to this register of M64282FP:

         No. Address   7   6   5   4   3   2   1   0
         1   001       N  VH1 VH0 G4  G3  G2  G1  G0

         Symbol   Bit Assignment  Operation
         N        1 bit           Exclusively set edge enhancement mode
         VH       2 bits          Select vertical - horizontal edge operation mode
         G        5 bits          Analog output gain

    I don't know what it does at boot... Calibrating or something? :P

    Upper nibble:

        3-0011
        8-1000
        A-1010
        B-1011

    It enables and disables "N" register.
    VH1 is never set.
    G register changes between values 0, 10, 18, 1C, 1E, 1F.

    At the end:

        N  = 1  - Edge enhancement mode. Access to P and M registers is always ignored!!!!
        VH = 01 - Horizontal edge mode.
        G  = 1F - Total Gain (dB) = 57.5 dB

Registers 6..35h
----------------

    Unknown
    Individual tile filter? 48 = 8*6?
    Contrast seems to change all 48 registers...

    It seems that they are used by the GB cartridge itself, not by the M64282FP.

    Values change when selecting dithering/normal mode, or when changing contrast.
    In normal mode values seems to mirror.

    Normal mode
    ------

    First 3 values of dithering mode are repeated 48/3 = 16 times

    Dithering
    ---------

    High light

Reg  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35
    Min contrast
    80 8F D0 8B BF E0 82 9B D4 8E CB E4 87 AF DB 83 9F D5 8A BB DF 86 AB D9 81 97 D2 8D C7 E3 80 93 D1 8C C3 E1 89 B7 DD 85 A7 D8 88 B3 DC 84 A3 D6
    82 90 C8 8C BA DC 84 9A CD 8F C4 E1 89 AC D5 85 9E CE 8B B6 DA 88 A8 D3 83 97 CB 8E C1 DF 82 93 C9 8D BD DD 8A B3 D8 87 A5 D2 89 AF D7 86 A1 D0
    84 90 C0 8D B4 D8 86 99 C6 8F BD DE 8A A8 D0 87 9C C8 8C B1 D6 89 A5 CE 85 96 C4 8E BA DC 84 93 C2 8D B7 DA 8B AE D4 88 A2 CC 8A AB D2 87 9F CA
    85 91 B8 8E AE D3 87 98 BE 90 B5 DA 8B A4 CA 88 9A C1 8D AB D1 8A A2 C8 86 95 BC 8F B3 D8 85 93 BA 8E B0 D6 8C A9 CF 89 9F C5 8B A6 CC 88 9D C3
    86 91 B1 8E A9 D0 88 97 B8 90 AF D8 8B A1 C6 88 99 BB 8D A7 CD 8A 9F C3 87 95 B6 8F AD D5 86 93 B3 8E AB D3 8C A5 CB 8A 9D C0 8C A3 C8 89 9B BE
    87 92 AA 8F A4 CC 89 96 B2 91 A8 D5 8C 9E C1 89 98 B5 8E A2 C9 8B 9C BE 88 95 AF 90 A7 D2 87 93 AC 8F A5 CF 8D A1 C6 8B 9B BB 8D 9F C3 8A 99 B8
    88 92 A5 8F A0 C9 89 95 AE 91 A3 D2 8D 9B BD 8A 96 B1 8E 9F C6 8C 9A BA 89 94 AB 90 A2 CF 88 93 A8 90 A1 CC 8E 9D C3 8B 99 B7 8D 9C C0 8B 97 B4
    89 92 A2 8F 9E C6 8A 95 AB 91 A1 CF 8D 9A BA 8B 96 AE 8F 9D C3 8C 99 B7 8A 94 A8 90 A0 CC 89 93 A5 90 9F C9 8E 9C C0 8C 98 B4 8E 9B BD 8B 97 B1
    8A 92 A1 90 9D BE 8B 94 A8 91 A0 C5 8E 99 B4 8C 95 AA 8F 9C BB 8D 98 B2 8B 93 A5 91 9F C3 8A 92 A3 90 9E C0 8F 9B B9 8D 97 AF 8E 9A B6 8C 96 AD
    8B 92 A0 90 9C B6 8C 94 A5 91 9F BC 8E 99 AF 8C 95 A7 8F 9B B4 8E 98 AD 8B 93 A3 91 9E BA 8B 92 A1 90 9D B8 8F 9A B2 8D 97 AB 8E 99 B0 8D 96 A9
    8C 92 9E 90 9B AE 8D 94 A2 91 9D B2 8F 98 A9 8D 95 A3 90 9A AD 8E 97 A7 8C 93 A0 91 9C B1 8C 92 9F 90 9B AF 8F 99 AB 8E 96 A6 8F 98 AA 8D 95 A4
    8D 92 9C 90 99 A8 8D 93 9F 91 9B AB 8F 97 A4 8E 94 A0 90 98 A7 8F 96 A3 8D 93 9E 91 9A AA 8D 92 9D 91 9A A9 90 98 A6 8E 95 A2 8F 97 A5 8E 95 A1
    8E 92 9B 91 98 A2 8E 93 9C 91 9A A4 90 96 A0 8F 94 9D 90 98 A1 8F 95 9F 8E 93 9C 91 99 A3 8E 92 9B 91 99 A3 90 97 A1 8F 95 9E 90 97 A0 8F 94 9E
    8F 92 99 91 97 9E 8F 93 9A 91 98 9F 90 95 9C 8F 93 9A 91 96 9D 90 95 9C 8F 92 99 91 98 9F 8F 92 99 91 97 9E 90 96 9D 90 94 9B 90 95 9C 8F 94 9B
    90 92 97 91 95 99 90 92 97 91 96 99 91 94 98 90 93 97 91 95 99 90 94 98 90 92 97 91 96 99 90 92 97 91 96 99 91 95 98 90 93 98 91 94 98 90 93 97
    92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92 92
    Max contrast

    Low light

Reg  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35
    Min contrast
    80 94 DC 8F CA F6 83 A1 E2 92 D7 FC 8A B8 ED 85 A6 E4 8D C5 F4 88 B3 EB 82 9D E0 91 D3 FA 81 98 DE 90 CE F8 8C C1 F1 87 AF E9 8B BC EF 86 AA E6
    82 95 D2 90 C2 F3 85 A0 DA 93 CE FC 8B B3 E8 86 A4 DD 8F BE F0 8A AF E5 84 9C D7 92 CA F9 83 98 D4 91 C6 F6 8D BB EE 89 AB E2 8C B7 EB 87 A8 E0
    84 96 CA 91 BD F1 87 9F D3 94 C6 FB 8D B0 E4 88 A3 D7 90 B9 EE 8B AC E1 86 9C D0 93 C3 F8 85 99 CD 92 C0 F5 8F B6 EB 8A A9 DD 8E B3 E7 89 A6 DA
    86 96 C4 92 B8 F0 89 9E CF 95 C1 FB 8E AD E1 8A A1 D2 91 B5 EC 8D AA DD 88 9B CB 94 BE F7 87 98 C7 93 BB F3 90 B2 E8 8C A7 DA 8F AF E5 8B A4 D6
    88 97 BE 93 B4 EE 8A 9E CA 96 BB FA 8F AA DE 8B A0 CE 92 B1 EA 8E A8 DA 89 9B C6 95 B9 F6 88 99 C2 94 B6 F2 91 AF E6 8D A5 D6 90 AC E2 8C A3 D2
    8A 97 B8 93 AF ED 8C 9D C5 96 B5 FA 90 A7 DB 8D 9F C9 92 AD E8 8F A5 D7 8B 9B C0 95 B3 F6 8A 99 BC 94 B1 F1 92 AB E4 8E A3 D2 91 A9 DF 8E A1 CE
    8B 98 B2 94 AB E4 8D 9C BE 97 B0 F0 91 A5 D3 8E 9E C2 93 A9 E0 90 A3 CF 8C 9B BA 96 AE EC 8B 99 B6 95 AD E8 93 A8 DB 8F A1 CB 92 A6 D7 8F A0 C6
    8C 98 AC 95 A7 DB 8E 9B B7 97 AA E7 92 A2 CB 8F 9D BB 94 A5 D7 91 A0 C7 8D 9A B3 96 A9 E3 8C 99 AF 95 A8 DF 93 A4 D3 90 9F C3 92 A3 CF 8F 9E BF
    8D 98 AA 95 A5 D0 8F 9B B3 97 A8 D9 92 A1 C3 8F 9C B6 94 A4 CD 91 9F C0 8E 9A B0 96 A7 D6 8D 99 AD 95 A6 D3 93 A3 C9 91 9E BD 93 A2 C6 90 9D B9
    8E 98 A8 95 A4 C6 8F 9B AF 97 A7 CD 93 A0 BC 90 9C B2 94 A3 C3 92 9F B9 8F 9A AD 96 A6 CB 8E 99 AA 96 A5 C8 94 A2 C1 91 9E B7 93 A1 BE 91 9D B4
    8F 98 A6 95 A2 BC 90 9A AB 97 A5 C2 93 9F B5 91 9B AD 95 A1 BA 92 9E B3 90 99 A9 96 A4 C0 8F 98 A7 96 A3 BE 94 A0 B8 92 9D B1 94 9F B6 91 9C AF
    90 98 A4 96 A1 B4 91 9A A8 97 A3 B8 94 9E AF 92 9B A9 95 A0 B3 93 9D AD 91 99 A6 97 A2 B7 90 98 A5 96 A1 B5 95 9F B1 93 9C AC 94 9E B0 92 9B AA
    92 98 A1 96 9E AD 93 99 A4 97 A0 B0 95 9C A9 93 9A A5 96 9E AC 94 9B A8 92 99 A3 97 9F AF 92 98 A2 96 9F AE 95 9D AB 94 9B A7 95 9D AA 93 9A A6
    94 98 9D 97 9B A5 94 98 9F 97 9C A7 96 9A A2 95 99 9F 96 9B A4 95 9A A1 94 98 9E 97 9C A6 94 98 9D 97 9C A5 96 9B A3 95 99 A1 96 9A A3 95 99 A0
    96 98 99 97 98 9E 96 98 9A 97 98 9F 97 98 9C 96 98 9A 97 98 9D 96 98 9C 96 98 99 97 98 9F 96 98 99 97 98 9E 97 98 9D 96 98 9B 97 98 9C 96 98 9B
    98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98 98
    Max contrast


3x3 programmable 1-D filtering
------------------------------
       +--+
       |MN|
    +--+--+--+
    |MW| P|ME|  Out = P + [2P - (MW+ME)] * alpha
    +--+--+--+  Out = P + [4P - (MN+MS+ ME+ MW)] * alpha
       |MS|
       +--+

    With this configuration: Out = 2P - [ (MW+ME) / 2 ]
                             Out = 3P - [ (MN+MS+ME+MW) / 2 ]

******************************************************************************************/

        cam->reg[reg] = value;

        if(reg == 0) // Take picture...
        {
            if(value == 0x3) //execute command? take picture?
            {
                u32 exposure_time = cam->reg[3] | (cam->reg[2]<<8);

                int dithering = 0;

                int i;
                for(i = 6; i < 0x36; i += 3) // if first 3 registers mirror, not dithering
                {
                    if(cam->reg[6+(i%3)] != cam->reg[i])
                    {
                        dithering = 1;
                        break;
                    }
                }

                int contrast = gb_clamp_int(0, ((cam->reg[6] - 0x80) * 100) / 0x12, 100);

                GB_CameraTakePicture(exposure_time,
                                     (cam->reg[1]&0x20) ? -(cam->reg[1]&0x1F) : (cam->reg[1]&0x1F),
                                     dithering,
                                     contrast);
            }
            //else Debug_DebugMsgArg("CAMERA WROTE - %02x to %04x",value,address);
        }
    }
    else
    {
        Debug_DebugMsgArg("CAMERA WROTE [%04x] = %02X",address,value);
    }
}

//----------------------------------------------------------------------------
