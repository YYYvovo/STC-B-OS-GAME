#ifndef _RS485_H_
#define _RS485_H_

#include "global.h"

#define RS485_STATE P37
#define RS485_R    0
#define RS485_D    1

extern XDATA u8 rs485_buf[128];
extern XDATA u8 rs485_rxcnt;
extern XDATA u32 rs485_timeout;
extern XDATA u8 rs485_evtstate;

void rs485_init(u32 baudrate);

#define rs485_write(buf,len) NOINT_ATOMIC(__rs485_write(buf,len);)
void __rs485_write(u8* buf, u8 len);

#endif //_RS485_H_