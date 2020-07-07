// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdlib.h>
#include <string.h>

#include <png.h>

#include "debug_utils.h"

#if !defined(PNG_SIMPLIFIED_READ_SUPPORTED) || \
    !defined(PNG_SIMPLIFIED_WRITE_SUPPORTED)
# error "This code needs libpng 1.6"
#endif

// Save a RGBA buffer into a PNG file
int Save_PNG(const char *filename, unsigned char *buffer,
             int width, int height, int is_rgba)
{
    png_image image;
    memset(&image, 0, sizeof(image));

    image.version = PNG_IMAGE_VERSION;
    image.width = width;
    image.height = height;
    image.flags = 0;
    image.colormap_entries = 0;

    if (is_rgba)
        image.format = PNG_FORMAT_RGBA;
    else
        image.format = PNG_FORMAT_RGB;

    int row_stride = is_rgba ? (width * 4) : (width * 3);

    if (!png_image_write_to_file(&image, filename, 0, buffer, row_stride, NULL))
    {
        Debug_LogMsgArg("%s(): png_image_write_to_file(): %s",
                        __func__, image.message);
        return 1;
    }

    return 0;
}

// Load a PNG file into a RGBA buffer
int Read_PNG(const char *filename, unsigned char **_buffer,
             int *_width, int *_height)
{
    png_image image;

    // Only the image structure version number needs to be set
    memset(&image, 0, sizeof image);
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_file(&image, filename))
    {
        Debug_LogMsgArg("%s(): png_image_begin_read_from_file(): %s",
                        __func__, image.message);
        return 1;
    }

    image.format = PNG_FORMAT_RGBA;

    png_bytep buffer;
    buffer = malloc(PNG_IMAGE_SIZE(image));

    if (buffer == NULL)
    {
        png_image_free(&image);
        Debug_LogMsgArg("%s(): Not enough memory", __func__);
        return 1;
    }

    if (!png_image_finish_read(&image, NULL, buffer, 0, NULL))
    {
        Debug_LogMsgArg("%s(): png_image_finish_read(): %s",
                        __func__, image.message);
        free(buffer);
        return 1;
    }

    *_buffer = buffer;
    *_width = image.width;
    *_height = image.height;

    return 0;
}
