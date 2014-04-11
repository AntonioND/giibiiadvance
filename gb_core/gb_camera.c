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
//           GB Camera documentation in ../docs/
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

int GB_CameraInit(void) // (int createwindow)
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
            // it can't be detected if the webcam is disconnected...
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
/*
static int gb_cam_matrix_process(int value, int x, int y)
{
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    x = x % 4;
    y = y % 4;

    int r0 = cam->reg[y*3+x+0];
    int r1 = cam->reg[y*3+x+1];
    int r2 = cam->reg[y*3+x+2];

    value = (255 - value);

    if(value < r0) return 0xC0;
    if(value < r1) return 0x80;
    if(value < r2) return 0x40;
    return 0x00;
}
*/
static void GB_CameraTakePicture(u32 exposure_time, int offset, int dithering_enabled, int contrast) // contrast: 0..100
{
    GB_CameraShoot();

    int inbuffer[16*8][14*8]; // buffer after aplying effects
    u8 readybuffer[14][16][16];
    int i, j;

    //Apply 3x3 programmable 1-D filtering
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
/*
        // Horizontal edge
        if( (i > 0) && (i < 16*8-1))
            gb_cam_shoot_buf[i][j] =
                gb_clamp_int( 0, (2 * gb_camera_retina_output[i][j]) -
                ( (gb_camera_retina_output[i-1][j] + gb_camera_retina_output[i+1][j]) / 2 ),
                             255);
        else
            gb_cam_shoot_buf[i][j] = gb_camera_retina_output[i][j];
*/
        // 2D edge
        int ms = gb_camera_retina_output[i][gb_min_int(j+1,14*8-1)];
        int mn = gb_camera_retina_output[i][gb_max_int(0,j-1)];
        int mw = gb_camera_retina_output[gb_max_int(0,i-1)][j];
        int me = gb_camera_retina_output[gb_min_int(i+1,16*8-1)][j];
        gb_cam_shoot_buf[i][j] =
            gb_clamp_int( 0, (3*gb_camera_retina_output[i][j]) - (ms+mn+mw+me)/2 , 255);
    }

    //Apply exposure time
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        int result = gb_cam_shoot_buf[i][j];
        result = ( ( (result + (exposure_time>>8)) * exposure_time ) / 0x800 );
        gb_cam_shoot_buf[i][j] = gb_clamp_int(0,result,255);
    }

/*
    //Apply offset
    for(i = 0; i < 16*8; i++) for(j = 0; j < 14*8; j++)
    {
        gb_cam_shoot_buf[i][j] = gb_clamp_int(0,
                            gb_cam_shoot_buf[i][j] + offset,
                            255);
    }
*/

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
        gb_cam_shoot_buf[i][j] = contrast_lookup[gb_cam_shoot_buf[i][j]];

    if(dithering_enabled)
    {
/*
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
*/
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
            int oldpixel = gb_cam_shoot_buf[i][j];
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
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    if(address == 0xA000) return cam->reg[0]; //'ready' register
    return 0xFF; //others are write-only? they are never read so the value doesn't matter...
}

void GB_CameraWriteRegister(int address, int value)
{
    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    int reg = address-0xA000;

    //printf("[%04X]=%02X\n",(u32)(u16)address,(u32)(u8)value);
    if(reg < 0x36)
    {
        //Debug_LogMsgArg("Cam [0x%02X]=0x%02X",(u32)(u16)(address&0xFF),(u32)(u8)value);

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

                // 4194304 Hz -> 0.2384185791015625 (us/clock)
                // each exposure time step is 16 (us)
                // 16 (us) / 0.2384185791015625 (us/clock) = 67.108864 clocks
                cam->clocks_left = 67 * exposure_time;
                //exposure_time = 0xFFFF -> 1,048576 second delay

                // Needed clocks for transfer (chip transfers 128*128 pixels, the last rows are garbage)
                // Since the output value is analog, only one clock per pixel.
                cam->clocks_left += 128*128;
                // I assume the value is stored in cartridge RAM while it is being transfered.
            }
            //else Debug_DebugMsgArg("CAMERA WROTE - %02x to %04x",value,address);
        }
    }
    else
    {
        Debug_DebugMsgArg("Camera wrote to invalid register: [%04x] = %02X",address,value);
    }
}

int GB_CameraClock(int clocks)
{
    if(GameBoy.Emulator.MemoryController != MEM_CAMERA)
        return 0x7FFFFFFF;

    _GB_CAMERA_CART_ * cam = &GameBoy.Emulator.CAM;

    if(cam->clocks_left == 0)
        return 0x7FFFFFFF;

    cam->clocks_left -= clocks;

    if(cam->clocks_left <= 0)
    {
        cam->reg[0] = 0; // ready
        cam->clocks_left = 0;
    }

    return cam->clocks_left;
}

//----------------------------------------------------------------------------

int GB_MapperIsGBCamera(void)
{
    return (GameBoy.Emulator.MemoryController == MEM_CAMERA);
}

//----------------------------------------------------------------------------
