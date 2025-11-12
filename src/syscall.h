#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "xstack.h"
#include "timer0_isr.h"
#include "scheduler.h"
#include "bit_ops.h"
#include "semaphore.h"
#include "events.h"
//#include "seg_led.h"


void syscall(u8 syscallid, u16 payload1, u16 payload2) small;

void syscall_c();

void pca_isr_c();


#define proc_exit() 					{syscall(0,0,0);}
#define proc_yield() 					{syscall(1,0,0);}
#define proc_sleep(t) 				{syscall(2,t,0);}
#define sem_init(sem_id,val) 	{syscall(4,sem_id,val);}
#define sem_post(sem_id) 			{syscall(5,sem_id,0);}
#define sem_wait(sem_id) 			{syscall(6,sem_id,0);}
#define proc_wait_evts(e) 		{syscall(3,e,0);}
#define proc_wait_evts_timeout(e, t)  {syscall(3,(e)|EVT_TIMER,t);}
#define deliver_event(pid, e) {syscall(7,pid,e);}

#endif