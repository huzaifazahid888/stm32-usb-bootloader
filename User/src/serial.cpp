#include "serial.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Serial::Serial()
{

}

void Serial::init(UART_HandleTypeDef * _huart)
{
	huart = _huart;
	linePtr = 0;
	lineBuf[0] = 0;
}


int Serial::readLine(char *line)
{
char rxByte;

    for(uint8_t i=0; i<20; i++)
    {
        if(uartReadByte(huart, &rxByte) == 0)	//byte available
        {
            if(rxByte == '\n')
            {
                lineBuf[linePtr] = 0;
                strcpy(line, lineBuf);
                linePtr = 0;
                return 0;
            }

            if(linePtr < (LINE_BUF_SIZE-1)) { lineBuf[linePtr++] = rxByte; } // else continue till '\n'
            //return 1;
            //continue;
        }
        return 2;
    }
    return 1;
}


void Serial::print(char txByte)
{
	uartWrite(huart, txByte);
}

void Serial::println(char txByte)
{
	uartWrite(huart, txByte);
	uartWrite(huart, '\n');
}

void Serial::print(const char *str)
{
int i;

	for(i=0; i<1000; i++)
	{
		if(*(str+i)==0) break;
		print((char)*(str+i));
	}
}

void Serial::println(const char *str)
{
	print(str);
	uartWrite(huart, '\n');
}

void Serial::printNumber(unsigned long n, unsigned char numBase)
{
char ch[20];
int i=20;
char c;

 	do
	{
		unsigned long m = n;
		n /= numBase;
		c = m - numBase * n;
		ch[--i] = c < 10 ? c + '0' : c + 'A' - 10;

  	}
 	while(n);
	while(i<20) print(ch[i++]);
}


void Serial::print(long n)
{
	if (n < 0)
	{
		print('-');
		n = -n;
		printNumber(n, 10);
		return;
	}
	printNumber(n, 10);
}

void Serial::println(long n)
{
	print(n);
	uartWrite(huart, '\n');
}

void Serial::print(int n)
{
	print((long)n);
}

void Serial::println(int n)
{
	print((long)n);
	uartWrite(huart, '\n');
}


void Serial::print(float number, unsigned char digits)
{
float rounding;
unsigned char i;
unsigned long int_part;
float remainder;
int toPrint;

 	if (number < 0.0)
  	{
 		print('-');
 		number = -number;
  	}

  	// Round correctly so that print(1.999, 2) prints as "2.00"
  	rounding = 0.5;
  	for (i=0; i<digits; ++i) rounding /= 10.0;

	number += rounding;

  	// Extract the integer part of the number and print it
  	int_part = (unsigned long)number;
  	remainder = number - (float)int_part;

	printNumber(int_part, 10);

  	// Print the decimal point, but only if there are digits beyond
  	if (digits > 0) print('.');

  	// Extract digits from the remainder one at a time
  	while (digits-- > 0)
  	{
    		remainder *= 10.0;
    		toPrint = (int)remainder;
    		printNumber(toPrint, 10);
    		remainder -= toPrint;
  	}

}

void Serial::println(float number, unsigned char digits)
{
	print(number, digits);
	uartWrite(huart, '\n');
}

void Serial::printHex(uint32_t n)
{
    printNumber((unsigned long)n, 16);
}

void Serial::printlnHex(uint32_t n)
{
    printHex(n);
    uartWrite(huart, '\n');
}

