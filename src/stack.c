#include "stack.h"
//补充调用
#include "scheduler.h"

IDATA u8 kernel_stack[KERNEL_STACKSIZE];
IDATA u8 process_stack[5][PROCESS_STACKSIZE];
XDATA u8 process_stack_swap[3][PROCESS_STACKSIZE];

/*
		OS allows 8 processes maximum, but there are only
		5 process stacks allowed in memory due to size 
		limitations.
		
		the process stacks can be swapped out to xdata 
		swap area if there are more than 5 processes at
		the same time.
*/

char get_stack_index(u8 pid)
{
	XDATA u8 i;
	for(i=0;i<5;i++)
		if(process_stack[i][PROCESS_STACKSIZE-1] == pid)
			return i;
	
	return -1;
}

char get_stackswap_index(u8 pid)
{
	XDATA u8 i;
	for(i=0;i<3;i++)
		if(process_stack_swap[i][PROCESS_STACKSIZE-1] == pid)
			return i;
	
	return -1;
}


extern XDATA u8 process_context[8][18];
extern XDATA u8 process_slot;

extern XDATA u8 debug_pid1[50];
extern XDATA volatile u8 debug_pid1_count;
extern XDATA u8 debug_pid2[50];
extern XDATA volatile u8 debug_pid2_count;

//指针的定义 以及使用位的声明
IDATA u8 clock_victim_ptr = 0;
extern DATA u8 clock_used_bits;

void stackswap(u8 swap_index)
{
	XDATA u8 i, temp, physical_index;
	XDATA u8 pid_at_ptr;//指针指向的PID
	
	//first try to find a stack slot that's not being used
	//寻找一个当前不需要被使用的进程栈进行替换
	physical_index = 0xff;
	/*
	for(i=0;i<5;i++)
	{	//获取进程的掩码 并检查是否还存在slot位图中（即进程有没有退出）
		if((process_slot & BIT(process_stack[i][PROCESS_STACKSIZE-1])) == 0)
		{
			physical_index = i;
			break;
		}
	}
	*/
	
	
	/*
	
	//if there's no free stack slot, select random victim
	//如果没找到 则随机选一个进程进行替换
	if(physical_index == 0xff){
		physical_index = rand32()%5;
		//当需要随机替换时，我们记录PID
		if(debug_pid1_count <= 20){
			debug_pid1[debug_pid1_count] = process_stack_swap[swap_index][PROCESS_STACKSIZE-1];
			debug_pid2[debug_pid2_count] = process_stack[physical_index][PROCESS_STACKSIZE-1];
			debug_pid1_count++;
			debug_pid2_count++;
		}
	}
	
	*/
	
	if(physical_index == 0xff){
		while(1){
			pid_at_ptr = process_stack[clock_victim_ptr][PROCESS_STACKSIZE-1];
			if(GETBIT(clock_used_bits, pid_at_ptr)){
				//bit = 1 给予二次机会
				CLEARBIT(clock_used_bits, pid_at_ptr);
			}
			else{
				//bit = 0 选中
				physical_index = clock_victim_ptr;
				if(debug_pid1_count < 40){
					debug_pid1[debug_pid1_count] = process_stack_swap[swap_index][PROCESS_STACKSIZE-1];
					debug_pid2[debug_pid2_count] = process_stack[physical_index][PROCESS_STACKSIZE-1];
					debug_pid1_count++;
					debug_pid2_count++;
				}
				//下一个进程
				clock_victim_ptr++;
				if (clock_victim_ptr == 5) 
					clock_victim_ptr = 0;
				break;
			}
			//下一个进程
			clock_victim_ptr++;
			if (clock_victim_ptr == 5)
				clock_victim_ptr = 0;
		}
	}
	
	for(i=0; i<PROCESS_STACKSIZE; i++)
	{
		temp = process_stack[physical_index][i];
		process_stack[physical_index][i] = process_stack_swap[swap_index][i];
		process_stack_swap[swap_index][i] = temp;
	}
	
	//convert context SP of process whose stack is being swapped out to a relative address
	process_context[process_stack_swap[swap_index][PROCESS_STACKSIZE-1]][17] -= (u8)process_stack[physical_index];
	
	//convert context SP of process whose stack is being swapped in to an absolute address
  process_context[process_stack[physical_index][PROCESS_STACKSIZE-1]][17] += (u8)process_stack[physical_index];
}