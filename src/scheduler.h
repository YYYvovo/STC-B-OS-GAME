#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "global.h"
#include "timer0_isr.h"
#include "xstack.h"
#include "stack.h"
#include "bit_ops.h"
#include "spin.h"
#include "semaphore.h"
#include "events.h"
/*

#include "mem.h"
#include "vstack.h"

#include "semaphore.h"
#include "spin.h"
*/

#define DEFAULT_TIMESLICES 3
#define NEXT(x) (((x)+1) & 0x7)
#define PROC_EXISTS(x) ((process_slot & BIT(x))>0)


extern XDATA u8 process_context[8][18];

extern XDATA u8 current_process;
extern DATA u32 timer0_cnt;
extern XDATA u8 process_slot;

extern XDATA u8 remaining_timeslices;
extern XDATA u8 proc_time_share[8];

#define COUNTDOWN(from, cnt) {if((from)>=(cnt)) (from)-=(cnt); else (from)=0;}
extern XDATA u16 proc_sleep_countdown[8];

/*
  PUSH32 	arg32
*/
void start_process(u16 entry, u16 pid, u32 param);
u8 select_process();
u8 process_ready(u8 pid);
void decrement_sleep_counters();
void reschedule();


/*
This variable MUST be in DATA memory, because write or read to this
variable must be done atomically (single instruction), otherwise
rescheduel may happen during data access.

When set to 1, no rescheduling is performed in ISR, but system time
counter is incremented.
*/
extern DATA u8 flag_nosched;
#define ATOMIC_START() {flag_nosched = 1;}
#define ATOMIC_END() {flag_nosched = 0;}
#define ATOMIC(atomic_code)\
{\
    flag_nosched = 1;\
    {atomic_code;}\
    flag_nosched = 0;\
}

#define NOINT_ATOMIC_START(){TR0 = 0;}
#define NOINT_ATOMIC_END(){TR0 = 1;}
#define NOINT_ATOMIC(atomic_code)\
{\
    TR0 = 0;\
    {atomic_code;}\
    TR0 = 1;\
}


#endif