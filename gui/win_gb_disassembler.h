
#ifndef __WIN_GB_DISASSEMBLER__
#define __WIN_GB_DISASSEMBLER__

void Win_GBDisassemblerStartAddressSetDefault(void);
void Win_GBDisassemblerUpdate(void);
int Win_GBDisassemblerCreate(void); // returns 1 if error
void Win_GBDisassemblerSetFocus(void);
void Win_GBDisassemblerClose(void);

#endif // __WIN_GB_DISASSEMBLER__

