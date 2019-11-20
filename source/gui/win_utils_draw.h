// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef WIN_UTILS_DRAW__
#define WIN_UTILS_DRAW__

#include "win_utils.h"

#define GUI_BACKGROUND_GREY      (216)
#define GUI_BACKGROUND_GREY_RGBA ((0xFF << 24) | (GUI_BACKGROUND_GREY << 16) | \
                                  (GUI_BACKGROUND_GREY << 8) | \
                                  (GUI_BACKGROUND_GREY))

#define GUI_WINDOWBAR_GREY      (232)
#define GUI_WINDOWBAR_GREY_RGBA ((0xFF << 24) | (GUI_WINDOWBAR_GREY << 16) | \
                                 (GUI_WINDOWBAR_GREY << 8) | \
                                 (GUI_WINDOWBAR_GREY))

void GUI_Draw_SetDrawingColor(int r, int g, int b);
void GUI_Draw_HorizontalLine(char *buffer, int w, int h, int x1, int x2, int y);
void GUI_Draw_VerticalLine(char *buffer, int w, int h, int x, int y1, int y2);
void GUI_Draw_Rect(char *buffer, int w, int h, int x1, int x2, int y1, int y2);
void GUI_Draw_FillRect(char *buffer, int w, int h, int x1, int x2, int y1,
                       int y2);

void GUI_Draw(_gui *gui, char *buffer, int w, int h, int clean);

#endif // WIN_UTILS_DRAW__
