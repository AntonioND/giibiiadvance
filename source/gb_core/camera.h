// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef GB_CAMERA__
#define GB_CAMERA__

//----------------------------------------------------------------

// The actual sensor is 128x126 or so
#define GBCAM_SENSOR_EXTRA_LINES    (8)
#define GBCAM_SENSOR_W              (128)
#define GBCAM_SENSOR_H              (112 + GBCAM_SENSOR_EXTRA_LINES)

#define GBCAM_W                     (128)
#define GBCAM_H                     (112)

//----------------------------------------------------------------

// Values in range 0-255
extern int gb_camera_webcam_output[GBCAM_SENSOR_W][GBCAM_SENSOR_H];

//----------------------------------------------------------------

int GB_CameraInit(void);
void GB_CameraEnd(void);

int GB_CameraReadRegister(int address);
void GB_CameraWriteRegister(int address, int value);

// Returns 1 if mapper is the GB Camera
int GB_MapperIsGBCamera(void);

//----------------------------------------------------------------

void GB_CameraClockCounterReset(void);
void GB_CameraUpdateClocksCounterReference(int reference_clocks);
int GB_CameraGetClocksToNextEvent(void);

//----------------------------------------------------------------

// Debugger window helpers

// Webcam image
int GB_CameraWebcamImageGetPixel(int x, int y);
// Image processed by retina chip
int GB_CameraRetinaProcessedImageGetPixel(int x, int y);

#endif // GB_CAMERA__
