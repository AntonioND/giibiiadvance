// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef DEBUG_UTILS__
#define DEBUG_UTILS__

void Debug_Init(void);
void Debug_End(void);
void Debug_LogMsgArg(const char *msg, ...);
void Debug_DebugMsgArg(const char *msg, ...);
void Debug_ErrorMsgArg(const char *msg, ...);

void Debug_DebugMsg(const char *msg);
void Debug_ErrorMsg(const char *msg);

void ConsoleReset(void);
void ConsolePrint(const char *msg, ...);
void ConsoleShow(void);

void SysInfoShow(void);

#endif // DEBUG_UTILS__
