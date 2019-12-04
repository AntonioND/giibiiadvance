// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef WEBCAM_UTILS__
#define WEBCAM_UTILS__

#include "gb_core/camera.h"

int Webcam_Init(void);
int Webcam_GetFrame(void);
void Webcam_End(void);

#endif // WEBCAM_UTILS__
