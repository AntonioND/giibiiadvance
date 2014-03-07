/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2014 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __FONT_UTILS__
#define __FONT_UTILS__

//---------------------------------------------------------------------------------

int FU_Init(void);
void FU_End(void);

//---------------------------------------------------------------------------------

#define CHR_MUSIC           14
#define STR_MUSIC           "\x0E"

#define CHR_ARROW_LEFT      16
#define STR_ARROW_LEFT      "\x10"
#define CHR_ARROW_RIGHT     17
#define STR_ARROW_RIGHT     "\x11"

#define CHR_SLIM_ARROW_UP   24
#define STR_SLIM_ARROW_UP   "\x18"
#define CHR_SLIM_ARROW_DOWN 25
#define STR_SLIM_ARROW_DOWN "\x19"

#define CHR_ARROW_UP        30
#define STR_ARROW_UP        "\x1E"
#define CHR_ARROW_DOWN      31
#define STR_ARROW_DOWN      "\x1F"

#define CHR_AACUTE_MINUS    160
#define STR_AACUTE_MINUS    "\xA0"
#define CHR_IACUTE_MINUS    161
#define STR_IACUTE_MINUS    "\xA1"
#define CHR_OACUTE_MINUS    162
#define STR_OACUTE_MINUS    "\xA2"
#define CHR_UACUTE_MINUS    163
#define STR_UACUTE_MINUS    "\xA3"
#define CHR_NTILDE_MINUS    164
#define STR_NTILDE_MINUS    "\xA4"
#define CHR_NTILDE_MAYUS    165
#define STR_NTILDE_MAYUS    "\xA5"

#define CHR_DBLBARS_UDL     185
#define STR_DBLBARS_UDL     "\xB9"
#define CHR_DBLBARS_UD      186
#define STR_DBLBARS_UD      "\xBA"
#define CHR_DBLBARS_DL      187
#define STR_DBLBARS_DL      "\xBB"
#define CHR_DBLBARS_UL      188
#define STR_DBLBARS_UL      "\xBC"

#define CHR_BARS_DL         191
#define STR_BARS_DL         "\xBF"

#define CHR_BARS_UR         192
#define STR_BARS_UR         "\xC0"
#define CHR_BARS_URL        193
#define STR_BARS_URL        "\xC1"
#define CHR_BARS_RDL        194
#define STR_BARS_RDL        "\xC2"
#define CHR_BARS_URD        195
#define STR_BARS_URD        "\xC3"
#define CHR_BARS_RL         196
#define STR_BARS_RL         "\xC4"
#define CHR_BARS_URDL       197
#define STR_BARS_URDL       "\xC5"

#define CHR_DBLBARS_UR      200
#define STR_DBLBARS_UR      "\xC8"
#define CHR_DBLBARS_RD      201
#define STR_DBLBARS_RD      "\xC9"
#define CHR_DBLBARS_URL     202
#define STR_DBLBARS_URL     "\xCA"
#define CHR_DBLBARS_RDL     203
#define STR_DBLBARS_RDL     "\xCB"
#define CHR_DBLBARS_URD     204
#define STR_DBLBARS_URD     "\xCC"
#define CHR_DBLBARS_RL      205
#define STR_DBLBARS_RL      "\xCD"
#define CHR_DBLBARS_URDL    206
#define STR_DBLBARS_URDL    "\xCE"

#define CHR_BARS_LU         217
#define STR_BARS_LU         "\xD9"
#define CHR_BARS_RD         218
#define STR_BARS_RD         "\xDA"


//---------------------------------------------------------------------------------

#define FONT_12_WIDTH (7)
#define FONT_12_HEIGHT (12)

int FU_Print12(char * buffer, int bufw, int bufh, int tx, int ty, const char * txt, ...);
int FU_PrintChar12(char * buffer, int bufw, int bufh, int tx, int ty, unsigned char c, int color);
int FU_Print12Color(char * buffer, int bufw, int bufh, int tx, int ty, int color, const char * txt, ...);

//---------------------------------------------------------------------------------

#define FONT_14_WIDTH (8)
#define FONT_14_HEIGHT (14)

int FU_Print14(char * buffer, int bufw, int bufh, int tx, int ty, const char * txt, ...);

//---------------------------------------------------------------------------------

#endif // __FONT_UTILS__

