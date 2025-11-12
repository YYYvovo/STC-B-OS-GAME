#ifndef _ADSENSOR_H_
#define _ADSENSOR_H_

#include "global.h"
	
#define	ADC_P12		0x04    //HALL
#define	ADC_P13		0x08	//Temperature
#define	ADC_P14		0x10	//Light
#define	ADC_P17		0x80	//Button3 / Nav
#define ADC_90T		(3<<5)

#define ADC_HALL    2
#define ADC_TEMP    3
#define ADC_LIGHT   4
#define ADC_NAV     7

#define ADC_FLAG	(1<<4)  //ADC OK (flag reset by software)
#define ADC_START	(1<<3)  //ADC Start (flag reset by hardware)

extern XDATA u8 adc_initialized;
void adc_init();
u8 adc_read(u8 channel);

#endif