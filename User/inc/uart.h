#ifndef INC_UART_H_
#define INC_UART_H_

#include"main.h"
#include "cfifo.h"


extern UART_HandleTypeDef huart3;

#ifdef __cplusplus
extern "C" {
#endif

void initDebugUart(void);


int uartWrite(UART_HandleTypeDef *huart, char ch);
int uartReadByte(UART_HandleTypeDef *huart, char * rxByte);
void uartPrintStr(UART_HandleTypeDef *huart, char * str);
int uartTrans(UART_HandleTypeDef *huart);
void transFIFOs();
int uartDataAvailable(UART_HandleTypeDef *huart);

void uart1Handler();

#ifdef __cplusplus
}
#endif


#endif
