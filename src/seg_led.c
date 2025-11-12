#include "seg_led.h"

XDATA u8 seg_display_content[8] = {0,0,0,0,0,0,0,0};
XDATA u8 led_display_content = 0;
XDATA u8 seg_led_current = 0;

CODE u8 seg_decoder[128] ={
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77,0x7c,0x39,0x5e,0x79,0x71,      //hex 0~F
    0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,

    0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, //space and others
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f,                                     //numbers
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,                                                       //others
    0x77,0x7c,0x39,0x5e,0x79,0x71,0x3d,0x76,0x0f,0x0e,0x75,0x38,0x37,0x54,0x5c,0x73,0x67,0x31,0x6d/*s:0x49, 5:0x6d*/,0x78,0x3e,0x1c,0x7e,0x64,0x6e,0x59, //A-Z
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80,                                                             //others
    0x77,0x7c,0x39,0x5e,0x79,0x71,0x3d,0x76,0x0f,0x0e,0x75,0x38,0x37,0x54,0x5c,0x73,0x67,0x31,0x6d/*s:0x49, 5:0x6d*/,0x78,0x3e,0x1c,0x7e,0x64,0x6e,0x59, //a-z
    0x80, 0x80, 0x80, 0x80, 0x80                                                                    //others
};

void seg_set_str(char* str) large reentrant
{
	seg_display_content[0] = seg_decoder[str[0]];
	seg_display_content[1] = seg_decoder[str[1]];
	seg_display_content[2] = seg_decoder[str[2]];
	seg_display_content[3] = seg_decoder[str[3]];
	seg_display_content[4] = seg_decoder[str[4]];
	seg_display_content[5] = seg_decoder[str[5]];
	seg_display_content[6] = seg_decoder[str[6]];
	seg_display_content[7] = seg_decoder[str[7]];
}

void seg_set_number(u32 n) large reentrant
{
    seg_display_content[7] = seg_decoder[n%10]; n/=10;
    seg_display_content[6] = seg_decoder[n%10]; n/=10;
    seg_display_content[5] = seg_decoder[n%10]; n/=10;
    seg_display_content[4] = seg_decoder[n%10]; n/=10;
    seg_display_content[3] = seg_decoder[n%10]; n/=10;
    seg_display_content[2] = seg_decoder[n%10]; n/=10;
    seg_display_content[1] = seg_decoder[n%10]; n/=10;
    seg_display_content[0] = seg_decoder[n%10]; n/=10;
}

//this function should be called in system timer ISR
//no reentrancy is supported
void seg_led_scan_next()
{
    switch(seg_led_current)
    {
        case 0: DISP_SEG(0)     seg_led_current++; break;
        case 1: DISP_SEG(1)     seg_led_current++; break;
        case 2: DISP_SEG(2)     seg_led_current++; break;
        case 3: DISP_SEG(3)     seg_led_current++; break;
        case 4: DISP_SEG(4)     seg_led_current++; break;
        case 5: DISP_SEG(5)     seg_led_current++; break;
        case 6: DISP_SEG(6)     seg_led_current++; break;
        case 7: DISP_SEG(7)     seg_led_current++; break;
        case 8: DISP_LED()      seg_led_current=0; break;
		default: seg_led_current = 0;
    }
}