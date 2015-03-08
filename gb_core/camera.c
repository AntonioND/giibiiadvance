/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2015 Antonio Niño Díaz (AntonioND)

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
#include "cpu.h"

//------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>

#ifndef NO_CAMERA_EMULATION

#include <opencv/cv.h>
#include <opencv/highgui.h>

static CvCapture * capture;
static int gbcamenabled = 0;
static int gbcamerafactor = 1;
#endif

//----------------------------------------------------------------------------

static int gb_camera_webcam_output[16*8][14*8];  // webcam image
static int gb_cam_retina_output_buf[16*8][14*8]; // image processed by retina chip

void GB_CameraEnd(void)
{
#ifndef NO_CAMERA_EMULATION
    if(gbcamenabled == 0) return;

    cvReleaseCapture(&capture);

    gbcamenabled = 0;
#endif
}

int GB_CameraInit(void)
{
#ifndef NO_CAMERA_EMULATION
    if(gbcamenabled) return 1;

    capture = cvCaptureFromCAM(CV_CAP_ANY); // TODO : Select camera from configuration file?
    if(!capture)
    {
        Debug_DebugMsgArg("OpenCV ERROR:\ncvCaptureFromCAM(CV_CAP_ANY) is NULL.\nNo camera detected?");
        return 0;
    }

    gbcamenabled = 1;

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
        Debug_DebugMsgArg("OpenCV ERROR: frame is NULL");
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
    Debug_ErrorMsgArg("This version of GiiBiiAdvance was compiled without webcam support.");
    return 0;
#endif
}

void GB_CameraWebcamCapture(void)
{
    int i, j;

#ifdef NO_CAMERA_EMULATION
    //Random output...
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++) gb_camera_webcam_output[i][j] = rand();
#else
    //Get image
    if(gbcamenabled == 0)
    {
        //Random output...
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++) gb_camera_webcam_output[i][j] = rand();
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

                    u32 r = data[0];
                    u32 g = data[1];
                    u32 b = data[2];

                    gb_camera_webcam_output[i][j] = ( 2*r + 5*g + 1*b) >> 3;
                }
            }
            else
            {
                Debug_ErrorMsgArg("Invalid camera output. Depth = %d bits | Channels = %d",
                                  frame->depth,frame->nChannels);
            }
        }
    }
#endif // NO_CAMERA_EMULATION
}

//----------------------------------------------------------------

static int gb_camera_clock_counter = 0;

inline void GB_CameraClockCounterReset(void)
{
    gb_camera_clock_counter = 0;
}

static inline int GB_CameraClockCounterGet(void)
{
    return gb_camera_clock_counter;
}

static inline void GB_CameraClockCounterSet(int new_reference_clocks)
{
    gb_camera_clock_counter = new_reference_clocks;
}

void GB_CameraUpdateClocksCounterReference(int reference_clocks)
{
    if(GameBoy.Emulator.MemoryController != MEM_CAMERA) return;

    int increment_clocks = reference_clocks - GB_CameraClockCounterGet();

    {
        _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

        if(cam->clocks_left > 0)
        {
            cam->clocks_left -= increment_clocks;

            if(cam->clocks_left <= 0)
            {
                cam->reg[0] &= ~BIT(0); // ready
                cam->clocks_left = 0;
            }
        }
    }

    GB_CameraClockCounterSet(reference_clocks);
}

inline int GB_CameraGetClocksToNextEvent(void)
{
    if(GameBoy.Emulator.MemoryController != MEM_CAMERA) return 0x7FFFFFFF;

    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    if(cam->clocks_left == 0) return 0x7FFFFFFF;
    return cam->clocks_left;
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

static inline u32 gb_cam_matrix_process(u32 value, int x, int y)
{
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    x = x & 3;
    y = y & 3;

    int base = 6 + (y*4 + x) * 3;

    u32 r0 = cam->reg[base+0];
    u32 r1 = cam->reg[base+1];
    u32 r2 = cam->reg[base+2];

    if(value < r0) return 0x00;
    else if(value < r1) return 0x40;
    else if(value < r2) return 0x80;
    return 0xC0;
}

static void GB_CameraTakePicture(void)
{
    int i, j;
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    //------------------------------------------------

    // Get webcam image
    GB_CameraWebcamCapture();

    //------------------------------------------------

    // Register 0
    u32 P_bits = 0;
    u32 M_bits = 0;
    //u32 X_bits = 0x01; //Whatever...

    switch( (cam->reg[0]>>1)&3 )
    {
        case 0: P_bits = 0x00; M_bits = 0x01; break;
        case 1: P_bits = 0x01; M_bits = 0x00; break;
        case 2: case 3: P_bits = 0x01; M_bits = 0x02; break; // They are the same
        default: break;
    }

    // Register 1
    u32 N_bit = (cam->reg[1] & BIT(7)) >> 7;
    u32 VH_bits = (cam->reg[1] & (BIT(6)|BIT(5))) >> 5;

    const float gain_lut[32] = { // Absolute, not dB
       5.01,  5.96,  7.08,   8.41,  10.00,  11.89,  14.13,  16.79,
      19.95, 28.18, 39.81,  56.23,  79.43, 112.20, 188.36, 375.84,
      10.00, 11.89, 14.13,  16.79,  19.95,  23.71,  28.18,  33.50,
      39.81, 56.23, 79.43, 112.20, 158.49, 223.87, 375.84, 749.89,
    };

    float GAIN = gain_lut[cam->reg[1] & 0x1F];

    // Registers 2 and 3
    u32 EXPOSURE_bits = cam->reg[3] | (cam->reg[2]<<8);

    // Register 4
    const int edge_ratio_lut[8] = { 50, 75, 100, 125, 200, 300, 400, 500 }; // Percent

    int EDGE_RATIO = edge_ratio_lut[(cam->reg[4] & 0x70)>>4];

    u32 E3_bit = (cam->reg[4] & BIT(7)) >> 7;
    u32 I_bit = (cam->reg[4] & BIT(3)) >> 3;

    float VREF = 0.5f * (float)(cam->reg[4]&7);

    // Register 5
    u32 Z_bits = (cam->reg[5] & (BIT(7)|BIT(6))) >> 6;

    float offset_step = ( (cam->reg[5] & 0x20) ? 0.032 : -0.032 );
    float OFFSET = offset_step * (cam->reg[5] & 0x1F);

    // Check what kind of image processing has to be done

    u32 _3x3_matrix_enabled = 0;
    u32 _1D_filtering_enabled = 0;

    u32 switch_value = (N_bit<<3) | (VH_bits<<1) | E3_bit;
    switch(switch_value)
    {
        case 0x0: _1D_filtering_enabled = 1; break;
        case 0xE: _3x3_matrix_enabled = 1; break;
        default:
            Debug_LogMsgArg("Unsupported GB Cam mode: 0x%X",switch_value);
            break;
    }

    //------------------------------------------------

    // Calculate timings
    cam->clocks_left = 4 * ( 32446 + ( N_bit ? 0 : 512 ) + 16 * EXPOSURE_bits );

    //------------------------------------------------

    // Apply exposure time
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        int result = gb_camera_webcam_output[i][j];
        result = ( (result * EXPOSURE_bits ) / 0x0800 );
        gb_cam_retina_output_buf[i][j] = gb_clamp_int(0,result,255);
    }

/*
    // Apply 3x3 programmable 1-D filtering
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {

        // 2D edge
        int ms = gb_camera_webcam_output[i][gb_min_int(j+1,14*8-1)];
        int mn = gb_camera_webcam_output[i][gb_max_int(0,j-1)];
        int mw = gb_camera_webcam_output[gb_max_int(0,i-1)][j];
        int me = gb_camera_webcam_output[gb_min_int(i+1,16*8-1)][j];
        gb_cam_retina_output_buf[i][j] =
            gb_clamp_int( 0, (3*gb_camera_webcam_output[i][j]) - (ms+mn+mw+me)/2 , 255);

        gb_cam_retina_output_buf[i][j] = gb_camera_webcam_output[i][j];
    }
*/
    //------------------------------------------------

    int fourcolorsbuffer[16*8][14*8]; // buffer after controller matrix

    // Convert to Game Boy colors using the controller matrix
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
        fourcolorsbuffer[i][j] = gb_cam_matrix_process(gb_cam_retina_output_buf[i][j],i,j);

    // Convert to tiles
    u8 finalbuffer[14][16][16]; // final buffer
    memset(finalbuffer,0,sizeof(finalbuffer));
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        u8 outcolor = 3 - (fourcolorsbuffer[i][j] >> 6);

        u8 * tile_base = finalbuffer[j>>3][i>>3];
        tile_base = &tile_base[(j&7)*2];

        if(outcolor & 1) tile_base[0] |= 1<<(7-(7&i));
        if(outcolor & 2) tile_base[1] |= 1<<(7-(7&i));
    }

    // Copy to cart ram...
    memcpy(&(GameBoy.Memory.ExternRAM[0][0xA100-0xA000]),finalbuffer,sizeof(finalbuffer));
}

int GB_CameraReadRegister(int address)
{
    GB_CameraUpdateClocksCounterReference(GB_CPUClockCounterGet());

    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    int reg = (address&0x7F); // Mirror
    if(reg == 0) return cam->reg[0]; //'ready' register
    return 0x00; //others are write-only and they return 0.
}

void GB_CameraWriteRegister(int address, int value)
{
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    int reg = (address&0x7F); // Mirror

    if(reg < 0x36)
    {
        //Debug_LogMsgArg("Cam [0x%02X]=0x%02X",(u32)(u16)(address&0xFF),(u32)(u8)value);

        if(reg == 0) // Take picture...
        {
            cam->reg[reg] = value & 7;

            if(value & BIT(0)) GB_CameraTakePicture();
        }
        else
        {
            cam->reg[reg] = value;
        }
    }
    else
    {
        Debug_DebugMsgArg("Camera wrote to invalid register: [%04x] = %02X",address,value);
    }
}

//----------------------------------------------------------------

int GB_MapperIsGBCamera(void)
{
    return (GameBoy.Emulator.MemoryController == MEM_CAMERA);
}

inline int GB_CameraWebcamImageGetPixel(int x, int y)
{
    return gb_camera_webcam_output[x][y];
}

inline int GB_CameraRetinaProcessedImageGetPixel(int x, int y)
{
    return gb_cam_retina_output_buf[x][y];
}

//----------------------------------------------------------------------------
