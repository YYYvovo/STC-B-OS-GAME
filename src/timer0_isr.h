#ifndef _TIMER0_ISR_H_
#define _TIMER0_ISR_H_

#include "global.h"
#include "scheduler.h"
#include "spin.h"
#include "seg_led.h"
/*
#include "vstack.h"
#include "bit_ops.h"
#include "events.h"
*/

extern DATA u32 timer0_cnt;
extern DATA u8 interrupt_counter;
extern DATA u8 interrupt_context[18];

void timer0_isr_c();
void save_current_context();
void load_current_context();

#endif