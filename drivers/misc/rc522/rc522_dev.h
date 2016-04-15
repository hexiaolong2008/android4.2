#ifndef __RC522_DEV_H__
#define __RC522_DEV_H__

typedef unsigned char uchar;
//typedef unsigned int  uint;

//sbit  beep=P3^5;

//#define nop() _nop_()

//#define  CLR_BEEP  beep=0
//#define  SET_BEEP  beep=1

#define  N_1  1
#define  N_2  2
#define  N_3  3
#define  N_4  4
#define  N_NOP  255

#define  READCARD   0xA1
#define  WRITECARD  0xA2
#define  KEYCARD    0xA3
#define  SETTIME    0xA4
#define  SENDID     0xA5

#define  RELOAD_COUNT  0xfb    //18.432M  9600  0xfb
                               //         4800  0xf6 

#endif
