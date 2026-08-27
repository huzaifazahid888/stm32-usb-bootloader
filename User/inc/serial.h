#ifndef SERIAL_H_
#define SERIAL_H_
#include "main.h"
#include "uart.h"
#include <stdio.h>

#define LINE_BUF_SIZE 256

class Serial
{
public:

	UART_HandleTypeDef *huart;

	char lineBuf[LINE_BUF_SIZE];
	int linePtr;

	Serial();
	void init(UART_HandleTypeDef * _huart);
	int readLine(char *line);
	void print(char txByte);
	void print(const char *str);
	void print(long n);
	void print(int n);
	void print(float number, unsigned char digits=3);
	void printNumber(unsigned long n, unsigned char numBase);
	
	void println(char txByte);
	void println(const char *str);
	void println(long n);
	void println(int n);
	void println(float number, unsigned char digits=2);


    void printHex(uint32_t n);
    void printlnHex(uint32_t n);

};

extern Serial serial;

#endif
