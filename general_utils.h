
#ifndef __TYPE_UTILS__
#define __TYPE_UTILS__

//----------------------------------------------------------------------------------

typedef unsigned long long int u64;
typedef signed long long int s64;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned short int u16;
typedef signed short int s16;
typedef unsigned char u8;
typedef signed char s8;

#define BIT(n) (1<<(n))

//----------------------------------------------------------------------------------

int s_snprintf(char * dest, int _size, const char * msg, ...);
void s_strncpy(char * dest, const char * src, int _size);
void s_strncat(char * dest, const char * src, int _size);

void memset_rand(u8 * start, u32 _size);

u64 asciihex_to_int(const char * text);
u64 asciidec_to_int(const char * text);

void ScaleImage24RGB(int zoom, char * srcbuf, int srcw, int srch, char * dstbuf, int dstw, int dsth);

//----------------------------------------------------------------------------------

#endif // __TYPE_UTILS__

