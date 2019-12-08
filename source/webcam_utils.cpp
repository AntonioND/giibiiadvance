// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

extern "C"
{
#include <stdlib.h>

#include "config.h"
#include "debug_utils.h"
#include "webcam_utils.h"

#include "gb_core/camera.h"
}

#if NO_CAMERA_EMULATION

extern "C" int Webcam_Init(void)
{
    Debug_ErrorMsgArg("This version of GiiBiiAdvance was compiled without "
                      "webcam support.");
    return 0;
}

extern "C" int Webcam_GetFrame(void)
{
    // Random output

    for (int j = 0; j < GBCAM_SENSOR_H; j++)
    {
        for (int i = 0; i < GBCAM_SENSOR_W; i++)
            gb_camera_webcam_output[i][j] = rand() & 0xFF;
    }

    return 0;
}

extern "C" void Webcam_End(void)
{
    // Do nothing
}

#else

#include "opencv2/opencv.hpp"

static cv::VideoCapture cap;

static int camera_enabled = 0;
static int camera_zoomfactor = 1;

// Returns 1 on success
extern "C" int Webcam_Init(void)
{
    if (camera_enabled)
        return 1;

    // Try to open the default camera
    if (!cap.open(EmulatorConfig.webcam_select))
    {
        Debug_DebugMsgArg("OpenCV error:\n"
                          "Couldn't open camera %d",
                          EmulatorConfig.webcam_select);
        return 0;
    }

    camera_enabled = 1;

    // TODO : Select resolution from configuration file?

    // This is the minimum standard resolution that works with GB Camera
    // (128 x (112 + 4))
    int w = 160;
    int h = 120;

    while (1) // Get the smallest valid resolution
    {
        cap.set(cv::CAP_PROP_FRAME_WIDTH, w);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, h);

        w = cap.get(cv::CAP_PROP_FRAME_WIDTH);
        h = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

        if ((w >= GBCAM_SENSOR_W) && (h >= GBCAM_SENSOR_H))
        {
            break;
        }
        else
        {
            w *= 2;
            h *= 2;
        }

        if (w >= 1024) // Too big, stop now.
            break;
    }

    cv::Mat frame;
    cap >> frame;
    if (frame.empty())
    {
        Webcam_End();
        Debug_ErrorMsgArg("OpenCV error: Couldn't get frame");
        return -1;
    }

    cv::Size size = frame.size();
    w = size.width;
    h = size.height;

    Debug_LogMsgArg("Camera resolution is: %dx%d", w, h);
    if ((w < GBCAM_SENSOR_W) || (h < GBCAM_SENSOR_H))
    {
        Debug_ErrorMsgArg("Camera resolution is too small..");
        GB_CameraEnd();
        return 0;
    }

    int xfactor = w / GBCAM_SENSOR_W;
    int yfactor = h / GBCAM_SENSOR_H;

    camera_zoomfactor = (xfactor > yfactor) ? yfactor : xfactor; // Min

    return 1;
}

extern "C" int Webcam_GetFrame(void)
{
    if (camera_enabled == 0)
    {
        // Random output
        for (int j = 0; j < GBCAM_SENSOR_H; j++)
        {
            for (int i = 0; i < GBCAM_SENSOR_W; i++)
                gb_camera_webcam_output[i][j] = rand() & 0xFF;
        }
    }
    else
    {
        // Get image
        cv::Mat frame, convertedframe;
        cap >> frame;
        if (frame.empty())
        {
            Webcam_End();
            Debug_ErrorMsgArg("OpenCV error: Couldn't get frame");
            return -1;
        }

        frame.convertTo(convertedframe, CV_8U);
        unsigned char *p = convertedframe.data;

        cv::Size size = convertedframe.size();
        int w = size.width;
        //int h = size.height;

        int channels = convertedframe.channels();
        if (channels != 3)
        {
            Debug_ErrorMsgArg("Invalid camera output.\n"
                              "Channels = %d",
                              channels);
            return -1;
        }

        // How much to jump from one element of a row to the next one
        int step = convertedframe.elemSize();

        for (int j = 0; j < GBCAM_SENSOR_H; j++)
        {
            for (int i = 0; i < GBCAM_SENSOR_W; i++)
            {
                int index = ((j * camera_zoomfactor) * w * step)
                            + ((i * camera_zoomfactor) * 3);

                int r = p[index + 0];
                int g = p[index + 1];
                int b = p[index + 2];

                gb_camera_webcam_output[i][j] = (2 * r + 5 * g + 1 * b) >> 3;
            }
        }
    }

    return 0;
}

extern "C" void Webcam_End(void)
{
    if (camera_enabled == 0)
        return;

    cap.release();

    camera_enabled = 0;
}

#endif // ENABLE_OPENCV
