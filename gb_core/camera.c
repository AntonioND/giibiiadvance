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

                    s16 value = ( 2*r + 5*g + 1*b) >> 3;
                    gb_camera_webcam_output[i][j] = value;
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
                cam->reg[0] = 0; // ready
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
/*
static int gb_cam_matrix_process(int value, int x, int y)
{
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    x = x % 4;
    y = y % 4;

    int base = 6 + y*3 + x;

    int r0 = cam->reg[base+0];
    int r1 = cam->reg[base+1];
    int r2 = cam->reg[base+2];

    value = (255 - value);

    if(value < r0) return 0x00;
    if(value < r1) return 0x40;
    if(value < r2) return 0x40;
    return 0xC0;
}
*/
static void GB_CameraTakePicture(u32 exposure_time, int offset, int dithering_enabled, int contrast) // contrast: 0..100
{
    GB_CameraWebcamCapture();

    int inbuffer[16*8][14*8]; // buffer after applying effects
    u8 readybuffer[14][16][16];
    int i, j;

    //Apply 3x3 programmable 1-D filtering
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        // 2D edge
        int ms = gb_camera_webcam_output[i][gb_min_int(j+1,14*8-1)];
        int mn = gb_camera_webcam_output[i][gb_max_int(0,j-1)];
        int mw = gb_camera_webcam_output[gb_max_int(0,i-1)][j];
        int me = gb_camera_webcam_output[gb_min_int(i+1,16*8-1)][j];
        gb_cam_retina_output_buf[i][j] =
            gb_clamp_int( 0, (3*gb_camera_webcam_output[i][j]) - (ms+mn+mw+me)/2 , 255);
    }

    //Apply exposure time
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        int result = gb_cam_retina_output_buf[i][j];
        result = ( ( (result + (exposure_time>>8)) * exposure_time ) / 0x800 );
        gb_cam_retina_output_buf[i][j] = gb_clamp_int(0,result,255);
    }

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
        gb_cam_retina_output_buf[i][j] = contrast_lookup[gb_cam_retina_output_buf[i][j]];

    if(dithering_enabled)
    {
        // Standard ordered dithering. Bayer threshold matrix - http://bisqwit.iki.fi/story/howto/dither/jy/
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
            int oldpixel = gb_cam_retina_output_buf[i][j];
            //int matrix_value = matrix[(i & 7) + ((j & 7) << 3)];
            int matrix_value = matrix[(i & 3) + ((j & 3) << 2)];
            int newpixel = gb_clamp_int(0, oldpixel + ( (matrix_value * treshold) / matrix_div), 255 ) & 0xC0;
            inbuffer[i][j] = (u8)newpixel;
        }
    }
    else
    {
        // No dithering
        for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
        {
            inbuffer[i][j] = (u8)(gb_cam_retina_output_buf[i][j] & 0xC0);
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
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    GB_CameraUpdateClocksCounterReference(GB_CPUClockCounterGet());

    if(address == 0xA000) return cam->reg[0]; //'ready' register
    return 0x00; //others are write-only and they return 0.
}

void GB_CameraWriteRegister(int address, int value)
{
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    int reg = (address&0x7F);

    //printf("[%04X]=%02X\n",(u32)(u16)address,(u32)(u8)value);
    if(reg < 0x36)
    {
        //Debug_LogMsgArg("Cam [0x%02X]=0x%02X",(u32)(u16)(address&0xFF),(u32)(u8)value);

        cam->reg[reg] = value & 7;

        if(reg == 0) // Take picture...
        {
            if(value == 0x3) //execute command? take picture?
            {
                u32 exposure_steps = cam->reg[3] | (cam->reg[2]<<8);

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

                GB_CameraTakePicture(exposure_steps,
                                     (cam->reg[1]&0x20) ? -(cam->reg[1]&0x1F) : (cam->reg[1]&0x1F),
                                     dithering,
                                     contrast);

                int N_bit = (cam->reg[1] & BIT(7)) ? 0 : 512;
                cam->clocks_left = 4 * ( 32446 + N_bit + 16 * exposure_steps );
            }
            //else Debug_DebugMsgArg("CAMERA WROTE - %02x to %04x",value,address);
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

int GB_CameraWebcamImageGetPixel(int x, int y)
{
    return gb_camera_webcam_output[x][y];
}

int GB_CameraRetinaProcessedImageGetPixel(int x, int y)
{
    return gb_cam_retina_output_buf[x][y];
}

//----------------------------------------------------------------------------
