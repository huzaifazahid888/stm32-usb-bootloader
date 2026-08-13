#include "cfifo.h"

void fifoInit(FifoBuf *fifo) {
	fifo->head = 0;
	fifo->tail = 0;
}

int fifoAvailableData(FifoBuf *fifo) {
    if(fifo->head >= fifo->tail) return(fifo->head - fifo->tail);
    return((FIFOSIZE - fifo->tail) + fifo->head );
}

int fifoIsFull(FifoBuf *fifo) {
	unsigned int hd = fifo->head;

	hd++; if(hd >= FIFOSIZE) hd = 0;
    if(hd == fifo->tail) return(1);
	return (0);
}

int fifoPush(FifoBuf *fifo, unsigned char dat) {
	unsigned int hd = fifo->head;

	hd++;
	hd = hd % FIFOSIZE;
    if(hd == fifo->tail) return(1);
    fifo->buf[fifo->head] = dat;
	fifo->head = hd;
	return (0);
}

unsigned char fifoPop(FifoBuf *fifo) {
	unsigned char dat;

    if(fifo->head == fifo->tail) return(0);
    dat = fifo->buf[fifo->tail];
    fifo->tail++;
    fifo->tail = fifo->tail % FIFOSIZE;
    return(dat);
}
