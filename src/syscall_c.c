#include "syscall.h"

void syscall_c()
{
	XDATA u8 syscall_id;
	XDATA u16 syscall_data, syscall_data2;
	
	syscall_id = interrupt_context[12]; //R7 of the process invoking syscall
	//payload1: MSB in R4, LSB in R5
	syscall_data = interrupt_context[10] + (interrupt_context[9] << 8);
	//payload2: MSB in R2, LSB in R3
	syscall_data2 = interrupt_context[8] + (interrupt_context[7] << 8);
	
	//syscall 1: process yield it's remaining timeslices
	if (syscall_id == 1)
	{
		SYSCALL_YIELD:;
		reschedule();
		return;
	}
	
	//syscall 2: process sleep, payload1 = time
	else if (syscall_id == 2)
	{
		SETBIT(proc_waiting_evt, current_process);
		proc_listening_list[current_process] = EVT_TIMER;
		proc_sleep_countdown[current_process] = syscall_data;
		goto SYSCALL_YIELD;
	}	

	//syscall 3: process wait on event, payload1 = events
	else if (syscall_id == 3)
	{
		SETBIT(proc_waiting_evt, current_process);
		proc_listening_list[current_process] = syscall_data;
		proc_sleep_countdown[current_process] = syscall_data2;
		goto SYSCALL_YIELD;
	}

	//syscall 0: process exit
	else if(syscall_id == 0)
	{
		CLEARBIT(process_slot, current_process);
		return;
	}

	//syscall 4: semaphore init
	else if (syscall_id == 4)
	{
		__sem_init(syscall_data, syscall_data2);
		return;
	}
	
	//syscall 5: semaphore post
	else if (syscall_id == 5)
	{
		__sem_post(syscall_data);
		return;
	}
	
	//syscall 6: semaphore wait
	else if (syscall_id == 6)
	{
		if(__sem_wait(syscall_data))
			return;
		else
			goto SYSCALL_YIELD;
	}
	
	//syscall 7: deliver event
	else if (syscall_id == 7)
	{
		if((proc_waiting_evt & BIT(syscall_data)) && 
				proc_listening_list[syscall_data] & syscall_data2)
		{
			proc_listening_list[syscall_data] = curr_events; 
			CLEARBIT(proc_waiting_evt, syscall_data);
		}
	}
}

void pca_isr_c()
{
	//CCF2 -> syscall
	if(CCON & 4)
	{
		CCON &= ~4;
		syscall_c();
	}
	else
	{
	}
}
