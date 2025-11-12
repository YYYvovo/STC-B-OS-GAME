#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "global.h"
#include "adsensor.h"

//Button is connected to Weak Pullup pin. When pushed, the pin is pulled down.
//True if button is pushed down.
#define BUTTON1 (~P32)  
#define BUTTON2 (~P33)

//Navigation Button and Button3 are connected to analog input.

//Get raw analog data.
#define nav_btn3_adc() adc_read(ADC_NAV)


#define BTNSTATE_B1     0x1
#define BTNSTATE_B2     0x2
#define BTNSTATE_B3     0x4
#define BTNSTATE_UP     0x8
#define BTNSTATE_DOWN   0x10
#define BTNSTATE_LEFT   0x20
#define BTNSTATE_RIGHT  0x40
#define BTNSTATE_PUSH   0x80

#define BTN_DENOISE_LEVEL 20

extern XDATA u8 btnstate_posedge;
extern XDATA u8 btnstate_negedge;
extern XDATA u8 last_btn_states[BTN_DENOISE_LEVEL];

/*
    Usage:

    update_nav_btn3_state();
    if(IS_NAV_LEFT)
        ......
    
*/
extern XDATA u8 __nav_btn3_state;
#define update_nav_btn3_state() {__nav_btn3_state = adc_read(ADC_NAV);}

#define IS_NAV_BTN3_IDLE    ((__nav_btn3_state) >> 5 >= 6)     // 110 or 111
#define IS_NAV_UP           ((__nav_btn3_state) >> 5 == 5)            // 101
#define IS_NAV_LEFT         ((__nav_btn3_state) >> 5 == 4)          // 100
#define IS_NAV_PUSH         ((__nav_btn3_state) >> 5 == 3)          // 011
#define IS_NAV_DOWN         ((__nav_btn3_state) >> 5 == 2)          // 010
#define IS_NAV_RIGHT        ((__nav_btn3_state) >> 5 == 1)         // 001
#define IS_BTN3_PUSH        ((__nav_btn3_state) >> 5 == 0)         // 000

void buttons_init();
void update_button_state();

#endif //_BUTTON_H_