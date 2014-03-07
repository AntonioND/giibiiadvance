
#ifndef __SOUND_UTILS__
#define __SOUND_UTILS__

typedef void (Sound_CallbackPointer)(void*,long);

void Sound_Init(void);

void Sound_SetCallback(Sound_CallbackPointer * fn);

void Sound_Enable(void);
void Sound_Disable(void);

void Sound_SetVolume(int vol);

#endif // __SOUND_UTILS__

