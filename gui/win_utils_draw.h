
#ifndef __WIN_UTILS_DRAW__
#define __WIN_UTILS_DRAW__

#include "win_utils.h"

void GUI_Draw_SetDrawingColor(int r, int g, int b);
void GUI_Draw_HorizontalLine(char * buffer, int w, int h, int x1, int x2, int y);
void GUI_Draw_VerticalLine(char * buffer, int w, int h, int x, int y1, int y2);
void GUI_Draw_Rect(char * buffer, int w, int h, int x1, int x2, int y1, int y2);
void GUI_Draw_FillRect(char * buffer, int w, int h, int x1, int x2, int y1, int y2);

void GUI_Draw(_gui * gui, char * buffer, int w, int h, int clean);

#endif // __WIN_UTILS_DRAW__
