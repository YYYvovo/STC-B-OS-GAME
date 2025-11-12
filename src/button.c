#include "button.h"

XDATA u8 __nav_btn3_state = 0xff;
XDATA u8 btnstate_posedge   =0;
XDATA u8 btnstate_negedge   =0;
XDATA u8 last_btn_states[BTN_DENOISE_LEVEL];

void buttons_init()
{
    //Initialize ADC for Nav and Button3
    adc_init();

    //Set Button1 and Button2 pin to Weak Pullup (M1 = 0, M0 = 0)
    P3M1 &= 0xF3;   // 1111 0011
    P3M0 &= 0xF3;   // 1111 0011

    //Set P17 to Weak Pullup (M1 = 0, M0 = 0)
    //P17 Pullup -> Key3 -> Nav Button(different resistance) -> GND
    P1M1 &= 0x7F; // 0111 1111
    P1M0 &= 0x7F; // 0111 1111
}

void update_button_state()
{
		XDATA u8 i; //Only use this function in timer0 isr. this variable prevents reentrancy.
		
		btnstate_posedge = btnstate_negedge = 0;
		
		//rotate previous button states. older one replaced by newer one.
		for(i = BTN_DENOISE_LEVEL-1; i>=1; i--)
			last_btn_states[i] = last_btn_states[i-1];	
		
		//update current button state, and cauculate bit flags
    update_nav_btn3_state();
		last_btn_states[0]  = 0;
    last_btn_states[0] |= BUTTON1       ?BTNSTATE_B1    :0;
    last_btn_states[0] |= BUTTON2       ?BTNSTATE_B2    :0;
    last_btn_states[0] |= IS_BTN3_PUSH  ?BTNSTATE_B3    :0;
    last_btn_states[0] |= IS_NAV_LEFT   ?BTNSTATE_LEFT  :0;
    last_btn_states[0] |= IS_NAV_RIGHT  ?BTNSTATE_RIGHT :0;
    last_btn_states[0] |= IS_NAV_UP     ?BTNSTATE_UP    :0;
    last_btn_states[0] |= IS_NAV_DOWN   ?BTNSTATE_DOWN  :0;
    last_btn_states[0] |= IS_NAV_PUSH  ?BTNSTATE_PUSH  :0;

		//check new state hold
		for(i=0;i<BTN_DENOISE_LEVEL-1;i++)
		{
				if(last_btn_states[i] != last_btn_states[0])
					return;
		}
		
		//different from old states, and currently set -> posedge
		btnstate_posedge = (last_btn_states[0] ^ last_btn_states[BTN_DENOISE_LEVEL-1]) & (last_btn_states[0]);   

		//different from old states, and previously set -> negedge
		btnstate_negedge = (last_btn_states[0] ^ last_btn_states[BTN_DENOISE_LEVEL-1]) & (last_btn_states[BTN_DENOISE_LEVEL-1]);
}