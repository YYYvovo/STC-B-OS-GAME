#ifndef _SEG_LED_H_
#define _SEG_LED_H_

#include "global.h"

//LED Pins:
#define LED0 P00
#define LED1 P01
#define LED2 P02
#define LED3 P03
#define LED4 P04
#define LED5 P05
#define LED6 P06
#define LED7 P07
#define LEDs P0

//Segment Selection Pins:
#define SEGA P00
#define SEGB P01
#define SEGC P02
#define SEGD P03
#define SEGE P04
#define SEGF P05
#define SEGG P06
#define SEGH P07
#define SEGs P0

//Digit Selection Pins:
#define SEG_DIGIT_SEL0 P20
#define SEG_DIGIT_SEL1 P21
#define SEG_DIGIT_SEL2 P22

/*
Board Tracing:
P23 -> NOT -> EN_138_Decoder -> Segments
         `--> VDD_LEDs
*/

#define LED_SEG_SWITCH P23  // USE_LED or USE_SEG
#define USE_LED 1
#define USE_SEG 0

//set all related pins to Push-Pull mode (P0, P20-P23)
#define seg_led_init()\ 
{\
    P0M1 &= 0x00;\
    P0M0 |= 0xff;\
    P2M1 &= 0xf0;\
    P2M0 |= 0x0f;\
}

#define seg_digit_sel(x)\
{\
    SEG_DIGIT_SEL0 = x&1;\
    SEG_DIGIT_SEL1 = (x&2) > 0;\
    SEG_DIGIT_SEL2 = (x&4) > 0;\
}

#define DISP_SEG(x)\
{\
    SEGs = seg_display_content[x];\
    seg_digit_sel(x);\
    LED_SEG_SWITCH = USE_SEG;\
}

#define DISP_LED()\
{\
    LEDs = led_display_content;\
    LED_SEG_SWITCH = USE_LED;\
}

extern XDATA u8 seg_display_content[8];
extern XDATA u8 led_display_content;
extern CODE u8 seg_decoder[128];
extern XDATA u8 seg_led_current;

//VPUSH void* str
void seg_set_str(char* str) large reentrant;
void seg_set_number(u32 n) large reentrant;
void seg_led_scan_next();

#endif