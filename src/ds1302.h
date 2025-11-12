#ifndef _DS1302_H_
#define _DS1302_H_

#include "global.h"
#include "bit_ops.h"
#include "scheduler.h"

#define DS1302_RST  P16
#define DS1302_SCLK P15
#define DS1302_IO   P54

#define DS1302_SEC 0
#define DS1302_MIN 1
#define DS1302_HR  2

#define DS1302_DATE  3
#define DS1302_MON  4
#define DS1302_DAY   5
#define DS1302_YEAR 6     

#define DS1302_WP   7    

void ds1302_writebyte(u8 addr, u8 write);

u8 ds1302_readbyte(u8 addr);

// P54->HiZ
#define ds1302_din() {SETBIT(P5M1, 4); CLEARBIT(P5M0, 4);}

// P54->Push-Pull
#define ds1302_dout() {CLEARBIT(P5M1, 4); CLEARBIT(P5M0, 4);}

// P15,P16 -> Push-Pull, P54 -> Push-Pull
#define ds1302_init()\
{\
    ds1302_dout();\
    CLEARBIT(P1M1,5); SETBIT(P1M0,5);\
    CLEARBIT(P1M1,6); SETBIT(P1M0,6);\
    DS1302_RST = 0;\
    DS1302_SCLK = 0;\
    DS1302_IO = 0;\
}

#define ds1302_clockpulse()\
{\
    DS1302_SCLK = 1;\
    DS1302_SCLK = 0;\
}

#define DS1302_CONVERTSEC(x) (((x)&0x0f) + ((x&0x70)>>4)*10)
#define DS1302_CONVERTMIN(x) DS1302_CONVERTSEC(x)
#define DS1302_CONVERTHOUR(x) ((x&0x80) ? \
															(((x&0x0f)+((x&0x10)>>4)*10)+((x&0x20)?12:0)) : \
															((x&0x30>>4)*10+(x&0x0f))) 
#define DS1302_CONVERTDATE(x)  DS1302_CONVERTSEC(x)
#define DS1302_CONVERTMONTH(x) DS1302_CONVERTSEC(x)
#define DS1302_CONVERTDAY(x) (x)
#define DS1302_CONVERTYEAR(x) (((x)&0x0f)+((((x)&0xf0)>>4) *10))

#endif