// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "build_options.h"
#include "debug_utils.h"
#include "file_utils.h"
#include "font_utils.h"
#include "general_utils.h"
#include "png_utils.h"

//------------------------------------------------------------------------------

#define FONT_CHARS_IN_ROW    (32)
#define FONT_CHARS_IN_COLUMN (8)

extern const uint8_t fnt_data[]; // in font_data.c

//------------------------------------------------------------------------------

int FU_Print(char *buffer, int bufw, int bufh, int tx, int ty,
             const char *txt, ...)
{
    char txtbuffer[300];

    va_list args;
    va_start(args, txt);
    int ret = vsnprintf(txtbuffer, sizeof(txtbuffer), txt, args);
    va_end(args);

    txtbuffer[sizeof(txtbuffer) - 1] = '\0';

    if ((tx < 0) || (ty < 0))
        return 0;
    if ((ty + FONT_HEIGHT) > bufh)
        return 0; // Multiline is not supported

    int i = 0;
    while (1)
    {
        unsigned char c = txtbuffer[i++];

        if (c == '\0')
            break;

        if ((tx + FONT_WIDTH) > bufw)
            break;

        int texx = (c % FONT_CHARS_IN_ROW) * FONT_WIDTH;
        int texy = (c / FONT_CHARS_IN_ROW) * FONT_HEIGHT;

        int tex_offset = (texy * (FONT_CHARS_IN_ROW * FONT_WIDTH * 4))
                         + texx * 4;             // Font is 4 bytes per pixel
        int buf_offset = ((ty * bufw) + tx) * 3; // Buffer is 3 bytes per pixel

        for (int y = 0; y < FONT_HEIGHT; y++)
        {
            char *bufcopy = &(buffer[buf_offset]);
            const u8 *texcopy = &(fnt_data[tex_offset]);

            for (int x = 0; x < FONT_WIDTH; x++)
            {
                *bufcopy++ = *texcopy++;
                *bufcopy++ = *texcopy++;
                *bufcopy++ = *texcopy++;
                texcopy++;
            }
            tex_offset += (FONT_CHARS_IN_ROW * FONT_WIDTH * 4);
            buf_offset += bufw * 3;
        }
        tx += FONT_WIDTH;
    }

#if 0
    // Print font
    for (int y = 0; y < 12 * 8; y++)
    {
        for (int x = 0; x < 32 * 7; x++)
        {
            buffer[((y * bufw + x) * 3) + 0] =
                    fnt12px[(y * fnt12->pitch) + (x * 4) + 0];
            buffer[((y * bufw + x) * 3) + 1] =
                    fnt12px[(y * fnt12->pitch) + (x * 4) + 1];
            buffer[((y * bufw + x) * 3) + 2] =
                    fnt12px[(y * fnt12->pitch) + (x * 4) + 2];
        }
    }
#endif

    return ret;
}

int FU_PrintColor(char *buffer, int bufw, int bufh, int tx, int ty, int color,
                  const char *txt, ...)
{
    char txtbuffer[300];

    va_list args;
    va_start(args, txt);
    int ret = vsnprintf(txtbuffer, sizeof(txtbuffer), txt, args);
    va_end(args);

    txtbuffer[sizeof(txtbuffer) - 1] = '\0';

    if ((tx < 0) || (ty < 0))
        return 0;
    if ((ty + FONT_HEIGHT) > bufh)
        return 0; // Multiline is not supported

    int i = 0;
    while (1)
    {
        unsigned char c = txtbuffer[i++];

        if (c == '\0')
            break;

        if ((tx + FONT_WIDTH) > bufw)
            break;

        int texx = (c % FONT_CHARS_IN_ROW) * FONT_WIDTH;
        int texy = (c / FONT_CHARS_IN_ROW) * FONT_HEIGHT;

        int tex_offset = (texy * (FONT_CHARS_IN_ROW * FONT_WIDTH * 4))
                         + texx * 4;             // Font is 4 bytes per pixel
        int buf_offset = ((ty * bufw) + tx) * 3; // Buffer is 3 bytes per pixel

        for (int y = 0; y < FONT_HEIGHT; y++)
        {
            char *bufcopy = &(buffer[buf_offset]);
            const u8 *texcopy = &(fnt_data[tex_offset]);

            for (int x = 0; x < FONT_WIDTH; x++)
            {
                int r = (uint8_t)(*texcopy++);
                int g = (uint8_t)(*texcopy++);
                int b = (uint8_t)(*texcopy++);

                *bufcopy++ = (r * (color & 0xFF)) >> 8;
                *bufcopy++ = (g * ((color >> 8) & 0xFF)) >> 8;
                *bufcopy++ = (b * ((color >> 16) & 0xFF)) >> 8;

                texcopy++; // Skip alpha
            }
            tex_offset += FONT_CHARS_IN_ROW * FONT_WIDTH * 4;
            buf_offset += bufw * 3;
        }
        tx += FONT_WIDTH;
    }

    return ret;
}

int FU_PrintChar(char *buffer, int bufw, int bufh, int tx, int ty,
                 unsigned char c, int color)
{
    if ((tx < 0) || (ty < 0))
        return 0;
    if ((ty + FONT_HEIGHT) > bufh)
        return 0;

    if (c == '\0')
        return 0;

    if ((tx + FONT_WIDTH) > bufw)
        return 0;

    int texx = (c % FONT_CHARS_IN_ROW) * FONT_WIDTH;
    int texy = (c / FONT_CHARS_IN_ROW) * FONT_HEIGHT;

    // Font is 4 bytes per pixel
    int tex_offset = (texy * (FONT_CHARS_IN_ROW * FONT_WIDTH * 4)) + texx * 4;
    int buf_offset = ((ty * bufw) + tx) * 3; // Buffer is 3 bytes per pixel

    for (int y = 0; y < FONT_HEIGHT; y++)
    {
        char *bufcopy = &(buffer[buf_offset]);
        const u8 *texcopy = &(fnt_data[tex_offset]);

        for (int x = 0; x < FONT_WIDTH; x++)
        {
            int r = (uint8_t)(*texcopy++);
            int g = (uint8_t)(*texcopy++);
            int b = (uint8_t)(*texcopy++);

            *bufcopy++ = (r * (color & 0xFF)) >> 8;
            *bufcopy++ = (g * ((color >> 8) & 0xFF)) >> 8;
            *bufcopy++ = (b * ((color >> 16) & 0xFF)) >> 8;

            texcopy++; // Skip alpha
        }

        tex_offset += (FONT_CHARS_IN_ROW * FONT_WIDTH * 4);
        buf_offset += bufw * 3;
    }

    return 1;
}
