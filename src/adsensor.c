#include "adsensor.h"

XDATA u8 adc_initialized = 0;
void adc_init()
{
    if(adc_initialized)
        return;

    /*
    Initalize ADC:
        P1.2 for HALL sensor
        P1.3 for Temperature sensor
        P1.4 for Light sensor
        P1.7 for Button3 and Navigation Button
    */
    P1ASF = ADC_P12 | ADC_P13 | ADC_P14 | ADC_P17;

    ADC_CONTR |= 0x80; //Power On
    ADC_CONTR = (ADC_CONTR & ~ADC_90T) | ADC_90T; //Speed select
    PCON2 &= ~(1<<5); //result byte order: high->ADC_RES
    EADC = 0; //use polling, no interrupt

    adc_initialized = 1;
}

u8 adc_read(u8 channel) //u8
{
    XDATA u8 timeout;
	ADC_RES = 0;

	ADC_CONTR = (ADC_CONTR & 0xe0) | ADC_START | channel; //select channel
	NOP(4);

    //poll
	for(timeout=0; timeout<250; timeout++)	
	{
		if(ADC_CONTR & ADC_FLAG)
		{
			ADC_CONTR &= ~ADC_FLAG;
			return ADC_RES;
		}
	}

   	return 0;
}