#include "global.h"
#include "random.h"
#include "bit_ops.h"

//physical stack = stack + 1byte indicating which process is using the stack.
#define KERNEL_STACKSIZE 14+1	
#define PROCESS_STACKSIZE 20+1
//the size settings must match stack.inc


extern IDATA u8 kernel_stack[KERNEL_STACKSIZE];
extern IDATA u8 process_stack[5][PROCESS_STACKSIZE];
extern XDATA u8 process_stack_swap[3][PROCESS_STACKSIZE];
char get_stack_index(u8 pid);
char get_stackswap_index(u8 pid);
void stackswap(u8 swap_index);