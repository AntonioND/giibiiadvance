// SPDX-License-Identifier: MIT
//
// Copyright (c) 2014, 2019, Antonio Niño Díaz

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../../source/png_utils.h"

void Debug_LogMsgArg(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
    printf("\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;

    char *fin = argv[1];

    char *buf;
    int w, h;

    if (Read_PNG(fin, &buf, &w, &h))
        return 1;

    FILE *fout = fopen("out.c", "w");
    if (fout) {
        int n = 0;

        fprintf(fout, "    ");

        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                unsigned char r = buf[(j * w + i) * 4 + 0];
                unsigned char g = buf[(j * w + i) * 4 + 1];
                unsigned char b = buf[(j * w + i) * 4 + 2];
                unsigned char a = buf[(j * w + i) * 4 + 3];

                fprintf(fout, "0x%02X, 0x%02X, 0x%02X, 0x%02X,",
                        r, g, b, a);

                n++;
                if (n == 3)
                {
                    n = 0;
                    fprintf(fout, "\n    ");
                }
                else
                {
                    fprintf(fout, " ");
                }
            }
        }

        fclose(fout);
    }

    free(buf);

    return 0;
}
