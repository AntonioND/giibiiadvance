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

#ifndef __DEBUG_VIDEO__
#define __DEBUG_VIDEO__

//---------------------------------------------------------------------------------

//buffer is 24 bit
void GB_Debug_GetSpriteInfo(int sprnum, u8 * x, u8 * y, u8 * tile, u8 * info);
void GB_Debug_PrintSprites(char * buf);
void GB_Debug_PrintZoomedSprite(char * buf, int sprite);

//buffer is 32 bit
void GB_Debug_PrintSpritesAlpha(char * buf);
void GB_Debug_PrintSpriteAlpha(char * buf, int sprite);

//---------------------------------------------------------------------------------

void GB_Debug_GetPalette(int is_sprite, int num, int color, u32 * red, u32 * green, u32 * blue);

//---------------------------------------------------------------------------------

#endif //__DEBUG_VIDEO__

