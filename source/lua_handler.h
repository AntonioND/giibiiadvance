// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef LUA_HANDLER__
#define LUA_HANDLER__

int Script_IsRunning(void);

// Run the script in the file pointed by path
int Script_RunLua(const char *path);

// Wait until the script is finished
void Script_WaitEnd(void);

#endif // LUA_HANDLER__
