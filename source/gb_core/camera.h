// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef __GB_CAMERA__
#define __GB_CAMERA__

//----------------------------------------------------------------

int GB_CameraInit(void);
void GB_CameraEnd(void);

int GB_CameraReadRegister(int address);
void GB_CameraWriteRegister(int address, int value);

int GB_MapperIsGBCamera(void); // returns 1 if mapper is the GB Camera

//----------------------------------------------------------------

void GB_CameraClockCounterReset(void);
void GB_CameraUpdateClocksCounterReference(int reference_clocks);
int GB_CameraGetClocksToNextEvent(void);

//----------------------------------------------------------------

// For debug window
int GB_CameraWebcamImageGetPixel(int x, int y); // webcam image
int GB_CameraRetinaProcessedImageGetPixel(int x, int y); // image processed by retina chip

//----------------------------------------------------------------

#endif // __GB_CAMERA__

