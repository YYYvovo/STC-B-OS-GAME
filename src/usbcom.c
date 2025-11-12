#include "usbcom.h"


void usbcom_init(u32 baudrate) 
{
		XDATA u16 reload;
		
    //Set TXD(P31) to Push-Pull RXD(P30) to HiZ
    {
		CLEARBIT(P3M1, 1);
		SETBIT(P3M0, 1);
		
		SETBIT(P3M1, 0);
		CLEARBIT(P3M0, 0);
	}

    //Configure baudrate timer
    {
        reload = 65536 - (MAIN_Fosc / 4) / baudrate;
        
        AUXR |= 0x01;		//S1 BRT Use Timer2;
        AUXR &= ~(1<<4);	//Timer stop
        AUXR &= ~(1<<3);	//Timer2 set As Timer
        AUXR |=  (1<<2);	//Timer2 set as 1T mode
        IE2  &= ~(1<<2);	//Disable interrup

        TH2 = (u8)(reload>>8);
        TL2 = (u8)reload;
        AUXR |=  (1<<4);	//Timer run enable
    }

    //Initialize UART1 on P30 and P31
    {
        PS = 1;	//High priority interrupt
        SCON = (SCON & 0x3f) | (1<<6); //8bit mode
        REN = 1; //Enable Rx
        P_SW1 = (P_SW1 & 0x3f) | (0 & 0xc0);	//Select IO port P30/P31
        ES = 1;	//Enable interrupt (wait for incoming data)
    }
}


//this function blocks current process until all data is sent
//not a VFUNC because there's no reentrancy support
void __usbcom_write(u8* buf, u8 len) small
{
    u8 i;
	
    //Disable interrupt (Tx uses polling mode)
    ES = 0;

		//length is set
		if(len)
		{
			for(i=0;i<len;i++)
			{
					//clear TX end flag
					TI = 0;
					//feed byte into UART1 buffer
					SBUF = buf[i];
					//spin until TX end flag is set (by hardware)
					while(TI == 0);
			}
		}
		else
		{
			//length is 0, auto detect '\0'
			for(i=0;;i++)
			{
					//clear TX end flag
					TI = 0;
					//feed byte into UART1 buffer
					if(buf[i] == '\0') break;		
					SBUF = buf[i];
					//spin until TX end flag is set (by hardware)
					while(TI == 0);
			}
		}
    //Enable UART1 interrupt
    //(Rx uses interrupt to trigger, but still runs in polling mode)
		RI = 0;//Reset all flags
    TI = 0;
    ES = 1;
}

XDATA u8 usbcom_buf[128];
XDATA u8 usbcom_rxcnt;
XDATA u32 usbcom_timeout;
XDATA u8 usbcom_evtstate = 0;

void usbcom_interrupt (void) interrupt UART1_VECTOR
{
	//Tx uses polling mode, INT source can only be RX

    //Disable UART1 interrupt
    //(Rx uses interrupt to trigger, but still runs in polling mode)
    ES = 0;

    usbcom_rxcnt = 0;
    while(1)
    {
        RI = 0; //Remove RX end flag.
        usbcom_buf[usbcom_rxcnt++] = SBUF; //Retrieve byte.

        //Spin until RX end flag is set (by hardware)
        usbcom_timeout = 500;//You have to try out a proper value for specific baud and clock frequency settings
        while(RI==0) 
        {
            if(--usbcom_timeout == 0)
                goto RX_END;
        }
    }

    RX_END:;
    
    TI = 0;  //Reset all flags
    RI = 0;
    ES = 1; //Enable UART1 interrupt

    usbcom_evtstate = 1;
}