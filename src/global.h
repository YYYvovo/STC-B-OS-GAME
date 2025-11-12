#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "config.h"

#define STC

#ifndef STC
    #define XDATA
    #define IDATA
    #define DATA
    #define CODE
    #define INTERRUPT
	#define VFUNC
#else
    #define XDATA       xdata
    #define IDATA       idata
    #define DATA        data
    #define CODE        code
    #define INTERRUPT   interrupt
	#define VFUNC		small
#endif


#endif //_GLOBAL_H_