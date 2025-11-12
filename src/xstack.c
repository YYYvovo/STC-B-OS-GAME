#include "xstack.h"

XDATA u8 kernel_xstack[KERNEL_XSTACKSIZE];
XDATA u8 process_xstack[8][PROCESS_XSTACKSIZE];