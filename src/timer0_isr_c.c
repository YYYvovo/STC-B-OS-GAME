#include "timer0_isr.h"

DATA u32 timer0_cnt = 0;
DATA u8 interrupt_counter = 0;
DATA u8 interrupt_context[18];


void save_current_context()
{
	XDATA u8 i;
	for(i=0;i<18;i++)
		process_context[current_process][i] = interrupt_context[i];
}

void load_current_context()
{
	XDATA u8 i;
	for(i=0;i<18;i++)
		interrupt_context[i] = process_context[current_process][i];
}

void timer0_isr_c()
{	
	//increment system time, and update all counters
	timer0_cnt += interrupt_counter;
	COUNTDOWN(remaining_timeslices, interrupt_counter);	
	decrement_sleep_counters();
	interrupt_counter = 0;
	
	//Update display
	seg_led_scan_next();
	
	//Process events
	process_events();

	/*
	Timer0 and Syscall interrupt at the same CPU cycle.
	If a task switch is done, current_process is modified,
	syscall won't be able to determine the correct process 
	calling it.
	*/
	if (CCON&4)
		goto SKIPSCHED;
	if(remaining_timeslices > 0)
		goto SKIPSCHED;
		
	reschedule();

	SKIPSCHED: 
		return;
}
