#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "global.h"
#include "bit_ops.h"
#include "scheduler.h"

#define MAX_SEMAPHORES 8

extern XDATA char semaphores[MAX_SEMAPHORES];
extern XDATA u8 proc_sem_wait_flag[MAX_SEMAPHORES];
extern XDATA u8 sem_wake_round[MAX_SEMAPHORES];
extern XDATA u8 proc_waiting_sem;
/*
    If process P is waiting for semaphore SEM,
    then bit P of proc_sem_wait_flag[SEM] is set,
    and 

    If process P is the last process that has 
    been waked by semaphore SEM, then bit P of
    sem_wake_round[SEM] is set.

    When a semaphore is waking up a process, it
    will find the first process waiting for it,
    starting from the [pid last waked]+1.

    For example, when SEM1 waked up PROC3 last time,
    it will find a process to wake up from PROC4,5,6,...
    This behaviour can prevent starving some process.
*/

void __sem_init(u8 sem_id, char val);
void __sem_post(u8 sem_id);
u8 __sem_wait(u8 sem_id);

#endif