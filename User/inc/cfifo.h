#ifndef _CFIFO_H
#define	_CFIFO_H

#define FIFOSIZE 512

typedef struct
{
    unsigned char buf[FIFOSIZE];
    unsigned int head;
    unsigned int tail;
  
}FifoBuf;


#if defined(__cplusplus)
extern "C" {
#endif

void fifoInit(FifoBuf * fifo);
int fifoAvailableData(FifoBuf *fifo);
int fifoIsFull(FifoBuf *fifo);
int fifoPush(FifoBuf *fifo, unsigned char dat);
unsigned char fifoPop(FifoBuf *fifo);

#if defined(__cplusplus)
}
#endif

#endif

