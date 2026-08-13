#include "main.h"
#include "uart.h"
#include "io_pins.h"
#include "string.h"

UART_HandleTypeDef huart3;
static FifoBuf txFifo1, rxFifo1;

void initDebugUart(void)
{
	//FIFO Init
	fifoInit(&txFifo1);
	fifoInit(&rxFifo1);

	huart3.Instance = USART3;
	huart3.Init.BaudRate = 115200;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart3) != HAL_OK) {
		Error_Handler();
	}
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (huart->Instance == USART3)
	{
		__HAL_RCC_USART3_CLK_ENABLE();

		__HAL_RCC_GPIOC_CLK_ENABLE();

		GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	}

}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART3)
	{
		__HAL_RCC_USART3_CLK_DISABLE();

		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);
	}

}
int uartWrite(UART_HandleTypeDef *huart, char ch)
{
	if (huart->Instance == USART3)
	{

		if(fifoIsFull(&txFifo1)) return 1;
		fifoPush(&txFifo1, ch);
		return 0;
	}

	return 2;
}


void uartPrintStr(UART_HandleTypeDef *huart, char *str)
{
	int i;

	for (i = 0; i < 1000; i++)
	{
		if(*(str+i)==0) break;
		uartWrite(huart, *(str + i));
	}
}

int uartTrans(UART_HandleTypeDef *huart)
{
	uint8_t byte = 0;

	if (huart->Instance == USART3)
	{
		if (fifoAvailableData(&txFifo1) <= 0) return 1;   // fifo empty
        if (huart3.gState != HAL_UART_STATE_READY) return 2;
		byte = fifoPop(&txFifo1);
		HAL_UART_Transmit(&huart3, &byte, 1, 5);
		return 0;
	}

	return 2;
}

void transFIFOs(int mask)
{
	if ((mask & 0x01) != 0)
		uartTrans(&huart3);

}

int uartDataAvailable(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART3)
	{
		return (fifoAvailableData(&rxFifo1));
	}

	return 999;
}

int uartReadByte(UART_HandleTypeDef *huart, char *byte)
{

	if (huart->Instance == USART3)
	{
		if (fifoAvailableData(&rxFifo1) <= 0) return 1;   // Rx fifo empty
		*byte = fifoPop(&rxFifo1);
		return 0;
	}

	return 2;
}
//============================ INTERRUPT CALLBACK ===========================

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	/*
	 if(huart->Instance == USART1)
	 {
	 fifoPush(&rxFifo1, rxByte1);
	 HAL_UART_Receive_IT(&huart1, &rxByte1, 1);	//re-enable Rx Interrupt for more data
	 }
	 */
}

void uart1Handler()
{
	char ch = huart3.Instance->DR;

	fifoPush(&rxFifo1, ch);
}







