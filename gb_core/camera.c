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

#define GBCAM_SENSOR_EXTRA_LINES (8)
#define GBCAM_SENSOR_W (128)
#define GBCAM_SENSOR_H (112+GBCAM_SENSOR_EXTRA_LINES) // The actual sensor is 128x126 or so

#define GBCAM_W (128)
#define GBCAM_H (112)

//------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>

#ifndef NO_CAMERA_EMULATION

#include <opencv/cv.h>
#include <opencv/highgui.h>

static CvCapture * capture;
static int gbcamera_enabled = 0;
static int gbcamera_zoomfactor = 1;

#endif

//----------------------------------------------------------------------------

static int gb_camera_webcam_output[GBCAM_SENSOR_W][GBCAM_SENSOR_H];  // webcam image
static int gb_cam_retina_output_buf[GBCAM_SENSOR_W][GBCAM_SENSOR_H]; // image processed by retina chip

void GB_CameraEnd(void)
{
#ifndef NO_CAMERA_EMULATION
    if(gbcamera_enabled == 0) return;

    cvReleaseCapture(&capture);

    gbcamera_enabled = 0;
#endif
}

int GB_CameraInit(void)
{
#ifndef NO_CAMERA_EMULATION
    if(gbcamera_enabled) return 1;

    capture = cvCaptureFromCAM(CV_CAP_ANY); // TODO : Select camera from configuration file?
    if(!capture)
    {
        Debug_DebugMsgArg("OpenCV ERROR:\ncvCaptureFromCAM(CV_CAP_ANY) is NULL.\nNo camera detected?");
        return 0;
    }

    gbcamera_enabled = 1;

    // TODO : Select resolution from configuration file?
    int w = 160; //This is the minimum standard resolution that works with GB Camera (128 x 112+4)
    int h = 120;
    while(1) // Get the smallest valid resolution
    {
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, w);
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, h);

        w = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
        h = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);

        if( (w >= GBCAM_SENSOR_W) && (h >= GBCAM_SENSOR_H) )
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
        if( (frame->width < GBCAM_SENSOR_W) || (frame->height < GBCAM_SENSOR_H) )
        {
            Debug_ErrorMsgArg("Camera resolution is too small..");
            GB_CameraEnd();
            return 0;
        }

        int xfactor = frame->width / GBCAM_SENSOR_W;
        int yfactor = frame->height / GBCAM_SENSOR_H;

        gbcamera_zoomfactor = (xfactor > yfactor) ? yfactor : xfactor; //min
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
    for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++) gb_camera_webcam_output[i][j] = rand();
#else
    //Get image
    if(gbcamera_enabled == 0)
    {
        //Random output...
        for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++) gb_camera_webcam_output[i][j] = rand();
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
                for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
                {
                    u8 * data = &( ((u8*)frame->imageData)
                            [((j*gbcamera_zoomfactor)*frame->widthStep)+((i*gbcamera_zoomfactor)*3)] );

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
    // ----------------

    GB_CameraWebcamCapture();

    //------------------------------------------------

    // Get configuration
    // -----------------

    // Register 0
    u32 P_bits = 0;
    u32 M_bits = 0;

    switch( (cam->reg[0]>>1)&3 )
    {
        case 0: P_bits = 0x00; M_bits = 0x01; break;
        case 1: P_bits = 0x01; M_bits = 0x00; break;
        case 2: case 3: P_bits = 0x01; M_bits = 0x02; break;
        default: break;
    }

    // Register 1
    u32 N_bit = (cam->reg[1] & BIT(7)) >> 7;
    u32 VH_bits = (cam->reg[1] & (BIT(6)|BIT(5))) >> 5;

    // Registers 2 and 3
    u32 EXPOSURE_bits = cam->reg[3] | (cam->reg[2]<<8);

    // Register 4
    const float edge_ratio_lut[8] = { 0.50, 0.75, 1.00, 1.25, 2.00, 3.00, 4.00, 5.00 };

    float EDGE_alpha = edge_ratio_lut[(cam->reg[4] & 0x70)>>4];

    u32 E3_bit = (cam->reg[4] & BIT(7)) >> 7;
    u32 I_bit = (cam->reg[4] & BIT(3)) >> 3;

    //------------------------------------------------

    // Calculate timings
    // -----------------

    cam->clocks_left = 4 * ( 32446 + ( N_bit ? 0 : 512 ) + 16 * EXPOSURE_bits );

    //------------------------------------------------

    // Sensor handling
    // ---------------

    //Copy webcam buffer to sensor buffer applying color correction
    for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
    {
        int value = gb_camera_webcam_output[i][j];
        value = 128 + (((value-128) * 5)/8); // "adapt" to 3.1/5.0 V
        gb_cam_retina_output_buf[i][j] = gb_clamp_int(0,value,255);
    }

    // Apply exposure time
    for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
    {
        int result = gb_cam_retina_output_buf[i][j];
        result = ( (result * EXPOSURE_bits ) / 0x0100 );
        gb_cam_retina_output_buf[i][j] = gb_clamp_int(0,result,255);
    }

    if(I_bit) // Invert image
    {
        for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
        {
            gb_cam_retina_output_buf[i][j] = 255-gb_cam_retina_output_buf[i][j];
        }
    }

    // Make signed
    for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
    {
        gb_cam_retina_output_buf[i][j] = gb_cam_retina_output_buf[i][j]-128;
    }

    int temp_buf[GBCAM_SENSOR_W][GBCAM_SENSOR_H];

    u32 filtering_mode = (N_bit<<3) | (VH_bits<<1) | E3_bit;
    switch(filtering_mode)
    {
        case 0x0: // 1-D filtering
        {
            for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
            {
                temp_buf[i][j] = gb_cam_retina_output_buf[i][j];
            }
            for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
            {
                int ms = temp_buf[i][gb_min_int(j+1,GBCAM_SENSOR_H-1)];
                int px = temp_buf[i][j];

                int value = 0;
                if(P_bits&BIT(0)) value += px;
                if(P_bits&BIT(1)) value += ms;
                if(M_bits&BIT(0)) value -= px;
                if(M_bits&BIT(1)) value -= ms;
                gb_cam_retina_output_buf[i][j] = gb_clamp_int(-128,value,127);
            }
            break;
        }
        case 0x2: //1-D filtering + Horiz. enhancement : P + {2P-(MW+ME)} * alpha
        {
            for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
            {
                int mw = gb_cam_retina_output_buf[gb_max_int(0,i-1)][j];
                int me = gb_cam_retina_output_buf[gb_min_int(i+1,GBCAM_SENSOR_W-1)][j];
                int px = gb_cam_retina_output_buf[i][j];

                temp_buf[i][j] = gb_clamp_int(0,px+((2*px-mw-me)*EDGE_alpha),255);
            }
            for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
            {
                int ms = temp_buf[i][gb_min_int(j+1,GBCAM_SENSOR_H-1)];
                int px = temp_buf[i][j];

                int value = 0;
                if(P_bits&BIT(0)) value += px;
                if(P_bits&BIT(1)) value += ms;
                if(M_bits&BIT(0)) value -= px;
                if(M_bits&BIT(1)) value -= ms;
                gb_cam_retina_output_buf[i][j] = gb_clamp_int(-128,value,127);
            }
            break;
        }
        case 0xE: //2D enhancement : P + {4P-(MN+MS+ME+MW)} * alpha
        {
            for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
            {
                int ms = gb_cam_retina_output_buf[i][gb_min_int(j+1,GBCAM_SENSOR_H-1)];
                int mn = gb_cam_retina_output_buf[i][gb_max_int(0,j-1)];
                int mw = gb_cam_retina_output_buf[gb_max_int(0,i-1)][j];
                int me = gb_cam_retina_output_buf[gb_min_int(i+1,GBCAM_SENSOR_W-1)][j];
                int px  = gb_cam_retina_output_buf[i][j];

                temp_buf[i][j] = gb_clamp_int(-128,px+((4*px-mw-me-mn-ms)*EDGE_alpha),127);
            }
            for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
            {
                gb_cam_retina_output_buf[i][j] = temp_buf[i][j];
            }
            break;
        }
        case 0x1:
        {
            // In my GB Camera cartridge this is always the same color. The datasheet of the
            // sensor doesn't have this configuration documented. Maybe this is a bug?
            for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
            {
                gb_cam_retina_output_buf[i][j] = 0;
            }
            break;
        }
        default:
        {
            // Ignore filtering
            Debug_DebugMsgArg("Unsupported GB Cam mode: 0x%X\n%02X %02X %02X %02X %02X %02X",filtering_mode,
                              cam->reg[0],cam->reg[1],cam->reg[2],cam->reg[3],cam->reg[4],cam->reg[5]);
            break;
        }
    }

    // Make unsigned
    for(i = 0; i < GBCAM_SENSOR_W; i++) for(j = 0; j < GBCAM_SENSOR_H; j++)
    {
        gb_cam_retina_output_buf[i][j] = gb_cam_retina_output_buf[i][j]+128;
    }

    //------------------------------------------------

    // Controller handling
    // -------------------

    int fourcolorsbuffer[GBCAM_W][GBCAM_H]; // buffer after controller matrix

    // Convert to Game Boy colors using the controller matrix
    for(i = 0; i < GBCAM_W; i++) for(j = 0; j < GBCAM_H; j++)
        fourcolorsbuffer[i][j] =
            gb_cam_matrix_process(gb_cam_retina_output_buf[i][j+(GBCAM_SENSOR_EXTRA_LINES/2)],i,j);

    // Convert to tiles
    u8 finalbuffer[14][16][16]; // final buffer
    memset(finalbuffer,0,sizeof(finalbuffer));
    for(i = 0; i < GBCAM_W; i++) for(j = 0; j < GBCAM_H; j++)
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

        cam->reg[reg] = value;

        if(reg == 0) // Take picture...
        {
            cam->reg[0] &= 7;
            if(value & BIT(0)) GB_CameraTakePicture();
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
    return gb_camera_webcam_output[x][y+(GBCAM_SENSOR_EXTRA_LINES/2)];
}

inline int GB_CameraRetinaProcessedImageGetPixel(int x, int y)
{
    return gb_cam_retina_output_buf[x][y+(GBCAM_SENSOR_EXTRA_LINES/2)]; // 4 extra rows, 2 on each border
}

//----------------------------------------------------------------------------
