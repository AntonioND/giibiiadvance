// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdlib.h>

#include <png.h>

#include "debug_utils.h"
#include "general_utils.h"

static void png_warn_fn_(unused__ png_structp sp, png_const_charp cp)
{
    Debug_LogMsgArg("libpng warning: %s", cp);
}

static void png_err_fn_(unused__ png_structp sp, png_const_charp cp)
{
    Debug_LogMsgArg("libpng error: %s", cp);
}

// Save a RGBA buffer into a PNG file
int Save_PNG(const char *file_name, int width, int height, void *buffer,
             int save_alpha)
{
    int ret = -1;

    FILE *fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep *row_pointers = NULL;

    fp = fopen(file_name, "wb");
    if (fp == NULL)
    {
        Debug_LogMsgArg("%s(): Can't open file for writing: %s",
                        __func__, file_name);
        goto cleanup;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                      png_err_fn_, png_warn_fn_);
    if (png_ptr == NULL)
    {
        Debug_LogMsgArg("%s(): png_ptr = NULL", __func__);
        goto cleanup;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        Debug_LogMsgArg("%s(): info_ptr = NULL", __func__);
        goto cleanup;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        Debug_LogMsgArg("%s(): setjmp() error", __func__);
        goto cleanup;
    }

    png_init_io(png_ptr, fp);

    row_pointers = calloc(height, sizeof(png_bytep));
    if (row_pointers == NULL)
    {
        Debug_LogMsgArg("%s(): row_pointers = NULL", __func__);
        goto cleanup;
    }

    if (save_alpha)
    {
        png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                     PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);

        for (int k = 0; k < height; k++)
        {
            row_pointers[k] = malloc(width * 4);
            if (row_pointers[k] == NULL)
            {
                Debug_LogMsgArg("%s(): row_pointers[%d] = NULL", __func__, k);
                goto cleanup;
            }
        }

        unsigned char *buf = (unsigned char *)buffer;
        for (int k = 0; k < height; k++)
        {
            for (int i = 0; i < width; i++)
            {
                row_pointers[k][(i * 4) + 0] = *buf++;
                row_pointers[k][(i * 4) + 1] = *buf++;
                row_pointers[k][(i * 4) + 2] = *buf++;
                row_pointers[k][(i * 4) + 3] = *buf++;
            }
        }
    }
    else
    {
        png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                     PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);

        for (int k = 0; k < height; k++)
        {
            row_pointers[k] = malloc(width * 3);
            if (row_pointers[k] == NULL)
            {
                Debug_LogMsgArg("%s(): row_pointers[%d] = NULL", __func__, k);
                goto cleanup;
            }
        }

        unsigned char *buf = (unsigned char *)buffer;
        for (int k = 0; k < height; k++)
        {
            for (int i = 0; i < width; i++)
            {
                row_pointers[k][(i * 3) + 0] = *buf++;
                row_pointers[k][(i * 3) + 1] = *buf++;
                row_pointers[k][(i * 3) + 2] = *buf++;
                buf++; // Ignore alpha
            }
        }
    }

    png_write_image(png_ptr, row_pointers);

    png_write_end(png_ptr, info_ptr);

    ret = 0;

cleanup:

    if (row_pointers)
    {
        for (int i = 0; i < height; i++)
            free(row_pointers[i]);
        free(row_pointers);
    }

    if (info_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);
    else if (png_ptr)
        png_destroy_write_struct(&png_ptr, NULL);

    if (fp)
        fclose(fp);

    return ret;
}

// Load a PNG file into a RGBA buffer
int Read_PNG(const char *file_name, char **_buffer, int *_width, int *_height)
{
    int width, height;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;

    unsigned char header[8]; // 8 is the maximum size that can be checked

    // Open file and test for it being a png
    FILE *fp = fopen(file_name, "rb");
    if (!fp)
    {
        Debug_LogMsgArg("Read_PNG(): File %s could not be opened for reading",
                        file_name);
        return 1;
    }

    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8))
    {
        fclose(fp);
        Debug_LogMsgArg("Read_PNG(): File %s is not recognized as a PNG file",
                        file_name);
        return 1;
    }

    // Initialize structs
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_err_fn_,
                                     png_warn_fn_);
    if (!png_ptr)
    {
        fclose(fp);
        Debug_LogMsgArg("Read_PNG(): png_create_read_struct failed");
        return 1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        Debug_LogMsgArg("Read_PNG(): png_create_info_struct failed");
        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        Debug_LogMsgArg("Read_PNG(): Error during init_io");
        return 1;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    if ((color_type == PNG_COLOR_TYPE_GRAY)
        || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
        png_set_gray_to_rgb(png_ptr);

    if (!(color_type & PNG_COLOR_MASK_ALPHA))
        png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

    png_read_update_info(png_ptr, info_ptr);

    // Read file
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        Debug_LogMsgArg("Read_PNG(): Error during read_image");
        return 1;
    }

    row_pointers = malloc(sizeof(png_bytep) * height);
    if (row_pointers == NULL)
    {
        Debug_LogMsgArg("Read_PNG(): Couldn't allocate row_pointers");
        png_read_end(png_ptr, NULL);
        fclose(fp);
        return 1;
    }

    for (int y = 0; y < height; y++)
    {
        row_pointers[y] = malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (row_pointers[y] == NULL)
        {
            Debug_LogMsgArg("Read_PNG(): Couldn't allocate row_pointers[%d]",
                            y);
            png_read_end(png_ptr, NULL);
            fclose(fp);
            for (; y >= 0; y--)
                free(row_pointers[y]);
            free(row_pointers);
            return 1;
        }
    }

    png_read_image(png_ptr, row_pointers);

    png_read_end(png_ptr, NULL);

    fclose(fp);

    char *buffer = malloc(width * height * 4);
    if (buffer == NULL)
    {
        for (int y = 0; y < height; y++)
            free(row_pointers[y]);
        free(row_pointers);
        Debug_LogMsgArg("Read_PNG(): Couldn't allocate buffer for image");
        return 1;
    }

    *_buffer = buffer;
    *_width = width;
    *_height = height;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            *buffer++ = row_pointers[y][x * 4 + 0];
            *buffer++ = row_pointers[y][x * 4 + 1];
            *buffer++ = row_pointers[y][x * 4 + 2];
            *buffer++ = row_pointers[y][x * 4 + 3];
        }
    }

    for (int y = 0; y < height; y++)
        free(row_pointers[y]);
    free(row_pointers);

    return 0;
}
