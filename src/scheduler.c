#include "scheduler.h"

XDATA u8 process_context[8][18];
XDATA u8 current_process = 8;
DATA u8 flag_nosched = 0;
XDATA u8 process_slot = 0;
XDATA u8 remaining_timeslices = DEFAULT_TIMESLICES;
XDATA u8 proc_time_share[8];
//This determines the number of timeslices assigned when each process is scheduled
//proc_time_share[0] in filled upon initialization. other elements will be filled when
//corresponding process gets started

//Only processes with 0 sleep countdown can be scheduled
XDATA u16 proc_sleep_countdown[8] ={0, 0, 0, 0, 0, 0, 0, 0};

//CLOCK定义
DATA u8 clock_used_bits = 0;

void start_process(u16 entry, u16 pid, u32 param)
{
	//XBPH,L
	process_context[pid][13] = (((u16)process_xstack[pid] + PROCESS_XSTACKSIZE)&0xff00)>>8;
	process_context[pid][14] = ((u16)process_xstack[pid] + PROCESS_XSTACKSIZE)&0x00ff;

	//PCH, PCL
	process_context[pid][15] = (entry&0xff00) >> 8; 
	process_context[pid][16] = (entry&0x00ff);

	
	//SP
	if(get_stack_index(pid)!=-1)
		process_context[pid][17] = process_stack[get_stack_index(pid)]; //stack present in memory, use absolute address
	else
		process_context[pid][17] = 0; //stack in stackswap space, use address relative to stack start
	
	//R4-R7 (param)
	process_context[pid][9] = (param & 0xff000000) >> 24;
	process_context[pid][10] = (param & 0x00ff0000) >> 16;
	process_context[pid][11] = (param & 0x0000ff00) >> 8;
	process_context[pid][12] = (param & 0x000000ff);

	proc_time_share[pid] = DEFAULT_TIMESLICES;
	process_slot |= BIT(pid);
} 


u8 select_process()
{
		XDATA u8 tmp_process;
		
		//current_process can be 8 (kernel startup) or 9 (idle spin)
		//so we have to set current_process to 0 in that situation.
		if(current_process >= 8) current_process = 0;
		tmp_process = current_process;
    

    //sequentially check other processes
    while((tmp_process = NEXT(tmp_process)) != current_process)
        if(process_ready(tmp_process))
        	goto SCHEDULER_END;

    //if no other process can run, check if current process can run again
    if (process_ready(current_process))
    {
		tmp_process = current_process;
		goto SCHEDULER_END;
	}
    
  //Can't find a process to run, reuturn 9(invalid)
	//ISR should recognize this and put system to spin until next interrupt
	tmp_process = 9;
	
	SCHEDULER_END:;
	return tmp_process;
}

u8 process_ready(u8 pid)
{
    //Check process exists
    if(!PROC_EXISTS(pid))
        return 0;

    //Check process is not waiting for semaphore
    if(proc_waiting_sem & BIT(pid))
        return 0;

    //Check process is not sleeping or waitiong for events
    if(proc_waiting_evt & BIT(pid))
        return 0;
 
		return 1;
}

//ONLY CALL THIS FUNCTION ATOMICALLY IN ISR!!!
void reschedule()
{
	//kernel or spin context doesn't need to be saved
	if(current_process < 8) 
		save_current_context();
	
	//select a process to run
	current_process = select_process();
	if(current_process == 9) goto IDLE_SPIN;
	
	//allocate timeslices and load context
	remaining_timeslices = proc_time_share[current_process];
	if(get_stack_index(current_process) == -1)
		stackswap(get_stackswap_index(current_process));
		//do stack swap.
	
	/*
	if(current_process < 8)
		//调度器决定调度后 在加载上下文前更新时间戳
		proc_last_used_time[current_process] = timer0_cnt;
	*/
	
	if(current_process < 8)
		//即将运行进程使用位置1
		SETBIT(clock_used_bits, current_process);
	
	load_current_context();
	return;
	
	IDLE_SPIN:;
	//load address of spin() into interrupt context.
	//allocate 1 timeslice to it.
	interrupt_context[15] = HIGH16((u16)spin);
	interrupt_context[16] = LOW16((u16)spin);
	remaining_timeslices = 1;
	return;
}

void decrement_sleep_counters()
{
    COUNTDOWN(proc_sleep_countdown[0], interrupt_counter);
    COUNTDOWN(proc_sleep_countdown[1], interrupt_counter);
    COUNTDOWN(proc_sleep_countdown[2], interrupt_counter);
    COUNTDOWN(proc_sleep_countdown[3], interrupt_counter);
    COUNTDOWN(proc_sleep_countdown[4], interrupt_counter);
    COUNTDOWN(proc_sleep_countdown[5], interrupt_counter);
    COUNTDOWN(proc_sleep_countdown[6], interrupt_counter);
    COUNTDOWN(proc_sleep_countdown[7], interrupt_counter);
}



