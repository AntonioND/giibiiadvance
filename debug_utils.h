
#ifndef __DEBUG_UTILS__
#define __DEBUG_UTILS__

void Debug_Init(void);
void Debug_End(void);
void Debug_LogMsgArg(const char * msg, ...);
void Debug_DebugMsgArg(const char * msg, ...);
void Debug_ErrorMsgArg(const char * msg, ...);

void Debug_DebugMsg(const char * msg);
void Debug_ErrorMsg(const char * msg);

void ConsoleReset(void);
void ConsolePrint(const char * msg, ...);
void ConsoleShow(void);

#endif // __DEBUG_UTILS__
