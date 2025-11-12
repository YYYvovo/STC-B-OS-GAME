#include "rs485.h"

void rs485_init(u32 baudrate)
{
    //Set TX/DR pins (P37, P47) to Push-Pull, RX pin (P46) to input.
    {
        P4M1 &= 0x3F;	// 0111 1111
        P4M0 |= 0xC0;   // 1000 0000

        P3M1 &= 0x7F;   // 0111 1111
        P3M0 |= 0x80;   // 1000 0000

        P4M1 |= 0x40;   // 0100 0000
        P4M0 &= 0xBF;   // 1011 1111
    }

    //Configure baudrate timer
    {
        XDATA u16 reload = 65536 - ((MAIN_Fosc / 4) / baudrate);	//1T
        AUXR &= ~(1<<4);	//Timer stop
        AUXR &= ~(1<<3);	//Timer2 set As Timer
        AUXR |=  (1<<2);	//Timer2 set as 1T mode
        IE2  &= ~(1<<2);	//Disable interrupt

        TH2 = (u8)(reload>>8);
        TL2 = (u8)reload;
        AUXR |=  (1<<4);	//Timer run enable
    }

    //Initialize UART2 on P46 and P47
    {
        IP2 |=  1;  //High priority interrupt
        S2CON &= ~(1<<7); //8bit mode
        S2CON |=  (1<<4); //Enable Rx
        P_SW2 = (P_SW2 & ~1) | (1 & 0x01); //Select IO port P46/P47
        RS485_STATE = RS485_R; //Half duplex mode set to Rx
        IE2 |= 1; //Enable interrupt (wait for incoming data)
    }
}

//this function blocks current process until all data is sent
void __rs485_write(u8* buf, u8 len)
{
    u8 i;
    //Tx enable.
    RS485_STATE = RS485_D;
    //Disable UART2 interrupt (Tx uses polling mode)
    IE2 &= ~1;

		if(len)
		{
			for(i=0;i<len;i++)
			{
					//clear TX end flag
					S2CON &= ~2;
					//feed byte into UART2 buffer
					S2BUF = buf[i];
					//spin until TX end flag is set (by hardware)
					while((S2CON & 2) == 0);
			}
		}
		else
		{
			for(i=0;;i++)
			{
					//clear TX end flag
					S2CON &= ~2;
					//feed byte into UART2 buffer
					if(buf[i] == '\0') break;		
					S2BUF = buf[i];
					//spin until TX end flag is set (by hardware)
					while((S2CON & 2) == 0);
			}
		}
    //Tx disable.
    RS485_STATE = RS485_R;

    //Enable UART2 interrupt
    //(Rx uses interrupt to trigger, but still runs in polling mode)
    S2CON &= ~1;   //Reset all flags
    S2CON &= ~2;
    IE2 |= 1;
}


XDATA u8 rs485_buf[128];
XDATA u8 rs485_rxcnt;
XDATA u32 rs485_timeout;
XDATA u8 rs485_evtstate = 0;

void rs485_interrupt (void) interrupt UART2_VECTOR
{
	//Tx uses polling mode, INT source can only be RX

    //Disable UART2 interrupt
    //(Rx uses interrupt to trigger, but still runs in polling mode)
    IE2 &= ~1;

    rs485_rxcnt = 0;
    while(1)
    {
        S2CON &= ~1; //Remove RX end flag.
        rs485_buf[rs485_rxcnt++] = S2BUF; //Retrieve byte.

        //Spin until RX end flag is set (by hardware)
        rs485_timeout = 500;//You have to try out a proper value for specific baud and clock frequency settings
        while((S2CON & 1)==0) 
        {
            if(--rs485_timeout == 0)
                goto RX_END;
        }
    }

    RX_END:;
    
    S2CON &= ~1;  //Reset all flags
    S2CON &= ~2;
    IE2 |= 1; //Enable UART2 interrupt

    rs485_evtstate = 1;
}