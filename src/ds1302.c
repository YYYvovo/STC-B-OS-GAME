#include "ds1302.h"
#include "seg_led.h"

void ds1302_writebyte(u8 addr, u8 write)
{
    ds1302_dout();

    DS1302_RST = 1; //Initiate communication

    DS1302_IO = 0;  //WRITE
    ds1302_clockpulse();

    //address
    {
        DS1302_IO = GETBIT(addr,0);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,1);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,2);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,3);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,4);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,5); // 0-> Time, 1-> RAM
        ds1302_clockpulse();
    }

    DS1302_IO = 1;
    ds1302_clockpulse();

    
    //data
    {
        DS1302_IO = GETBIT(write,0);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(write,1);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(write,2);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(write,3);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(write,4);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(write,5);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(write,6);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(write,7);
        ds1302_clockpulse();
    }


    //reset pins
    DS1302_SCLK = 0;
    DS1302_IO = 0;
    DS1302_RST = 0;
}


u8 ds1302_readbyte(u8 addr) 
{
    XDATA u8 read = 0;
		
    ds1302_dout();

    DS1302_RST = 1; //Initiate communication

    DS1302_IO = 1;  //READ
    ds1302_clockpulse();

    //address
    {
        DS1302_IO = GETBIT(addr,0);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,1);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,2);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,3);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,4);
        ds1302_clockpulse();

        DS1302_IO = GETBIT(addr,5); // 0-> Time, 1-> RAM
        ds1302_clockpulse();
    }

    DS1302_IO = 1;
    ds1302_clockpulse();

    ds1302_din();
    //data
    {
        WRITEBIT(read, 0, DS1302_IO);
        ds1302_clockpulse();

        WRITEBIT(read, 1, DS1302_IO);
        ds1302_clockpulse();

        WRITEBIT(read, 2, DS1302_IO);
        ds1302_clockpulse();

        WRITEBIT(read, 3, DS1302_IO);
        ds1302_clockpulse();

        WRITEBIT(read, 4, DS1302_IO);
        ds1302_clockpulse();

        WRITEBIT(read, 5, DS1302_IO);
        ds1302_clockpulse();

        WRITEBIT(read, 6, DS1302_IO);
        ds1302_clockpulse();

        WRITEBIT(read, 7, DS1302_IO);
        ds1302_clockpulse();
    }

    //reset pins
    DS1302_RST = 0;
    DS1302_SCLK = 0;
    DS1302_IO = 0;
		
		return read;
}
