
#ifndef __WIN_MAIN__
#define __WIN_MAIN__

int Win_MainCreate(char * rom_path); // returns 1 if error
void Win_MainRender(void);
int Win_MainRunningGBA(void);
int Win_MainRunningGB(void);
void Win_MainLoopHandle(void);

#endif // __WIN_MAIN__
