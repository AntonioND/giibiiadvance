// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef WIN_UTILS_EVENTS__
#define WIN_UTILS_EVENTS__

#include "win_utils.h"

int GUI_SendEvent(_gui *gui, SDL_Event *e); // Returns 1 if needed to redraw

#endif // WIN_UTILS_EVENTS__
