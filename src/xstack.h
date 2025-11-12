#include "global.h"

#define KERNEL_XSTACKSIZE 32
#define PROCESS_XSTACKSIZE 64

extern XDATA u8 kernel_xstack[KERNEL_XSTACKSIZE];
extern XDATA u8 process_xstack[8][PROCESS_XSTACKSIZE];

u16 getxbp() small;
void setxbp(u16 xbp) small;