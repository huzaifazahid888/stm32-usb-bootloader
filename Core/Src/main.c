
#include "main.h"
#include "fatfs.h"
#include "usb_host.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "flash_if.h"
typedef  void (*pFunction)(void);
pFunction JumpToApplication;
uint32_t JumpAddress;

extern char USBHPath[4];   /* USBH logical drive path */
FATFS myUsbFatFS;

FIL myFile;
FRESULT res;
UINT byteswritten, bytesread;
char rwtext[100];
char msg[100];
unsigned int x;
unsigned char pointer,hexBuffer[0xFF];
bool startPacket,newPacket;
void CopyAppToUserMemory(void);
void debug(char *text);
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
extern ApplicationTypeDef Appli_state;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
void MX_USB_HOST_Process(void);

#define APP_BLOCK_TRANSFER_SIZE 512
#define SIZE_OF_U32 4
char msg[100];

uint32_t appSize;
unsigned char appBuffer[512];
unsigned int readBytes;
unsigned int i;
unsigned char programmed = 0;
uint32_t usbInsertTim = 0;

void jumpcode()
{
	 debug("Jumping to the application address:");

		  // jump to the application
		  if (((*(__IO uint32_t*)FLASH_USER_START_ADDR) & 0x2FFD0000 ) == 0x20000000)
		  {
			  sprintf(msg,"%d",FLASH_USER_START_ADDR);
			  debug(msg);
				// Jump to user application
				JumpAddress = *(__IO uint32_t*) (FLASH_USER_START_ADDR + 4);

				JumpToApplication = (pFunction) JumpAddress;
				// Initialize user application's Stack Pointer
				__set_MSP(*(__IO uint32_t*) FLASH_USER_START_ADDR);
				SCB->VTOR = FLASH_USER_START_ADDR;

				HAL_RCC_DeInit();
				HAL_DeInit();

				//__disable_irq();

				JumpToApplication();
			}
		  debug("\r\n");
}
int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_FATFS_Init();
	MX_USB_HOST_Init();
	MX_USART3_UART_Init();

	programmed = 0;
	debug("\r\n\r\n");
	debug("#             STM32f4 USB BootLoader         #\r\n");
	debug("\r\n");
	usbInsertTim = HAL_GetTick();

	debug("Programming Mode ... \r\n");
	bool mountCheck;

	mountCheck = false;

	while ((HAL_GetTick() - usbInsertTim) < 3000)
	{
		MX_USB_HOST_Process();

		switch (Appli_state)
		{
		case APPLICATION_IDLE:    break;

		case APPLICATION_START:
			if (f_mount(&myUsbFatFS, (TCHAR const*) USBHPath, 0) != FR_OK)
			{
				/* FatFs Initialization Error */
				//Error_Handler();
				if (mountCheck == false)
				{
					debug("failed to mount ..\r\n");
					mountCheck = true;
				}
			}
			else
			{
				LED(GREEN_LED_GPIO_Port, GREEN_LED_Pin, LED_SET);
				if (mountCheck == false)
				{
					debug("drive mounted ..\r\n");
					mountCheck = true;
				}
			}
			break;

		case APPLICATION_READY:
			//use here
			uint32_t offset = 0;

			if (f_open(&myFile, "BLINK.BIN", FA_READ) == FR_OK)
			{
				debug("file opened ..\r\n");
				appSize = f_size(&myFile);

				debug("file size : ");
				sprintf(msg, "%d", (int) appSize);
				debug(msg);
				debug("\r\n");

				while (1)
				{
					if (f_read(&myFile, appBuffer, 512, &readBytes) != FR_OK)
					{
						debug("file read error!! \r\n");
						break;
					}

					if (readBytes == 0)
					{
						// end of file no mismatch found
						debug("firmware identical\r\n");    break;
					}
					if(memcmp(appBuffer,(void*)(FLASH_USER_START_ADDR + offset), readBytes) != 0)
					{
						debug("copying bin file in to memory ..\r\n");
						CopyAppToUserMemory();
						debug("App installed\r\n");
						break;
					}

					offset += readBytes;
				}
				f_close(&myFile);
				debug("closing file\r\n");
				programmed = 1;

			}
			else
			{
				debug("error opening file ..\r\n");

			}

		case APPLICATION_DISCONNECT:
			// if the pendrive is in disconnected state
			debug("disconnecting drive ..\r\n");
			LED(GREEN_LED_GPIO_Port, GREEN_LED_Pin, LED_RESET);
			break;
		}

		if (programmed != 0)
		{
			debug("exiting programming mode ..\r\n");
			f_mount(NULL, "", 1);
			debug("System Reseting.........\r\n");
			HAL_Delay(1500);
			jumpcode();
			//NVIC_SystemReset();
			break;
		}

	}

	if (Appli_state != APPLICATION_READY)
	{
		debug("No USB detected, jumping to application\r\n");
		jumpcode();
	}

  debug("running wait loop ..\r\n");
	while (1)
	{
		if (programmed == 0)
		{
			LED(BLUE_LED_GPIO_Port, BLUE_LED_Pin, LED_TOGGLE);
		}
		else
		{
			//NVIC_SystemReset();
			LED(GREEN_LED_GPIO_Port, GREEN_LED_Pin, LED_TOGGLE);
		}

	HAL_Delay(500);
    MX_USB_HOST_Process();
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}


static void MX_USART3_UART_Init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }


}


static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GREEN_LED_Pin|ORANGE_LED_Pin|RED_LED_Pin|BLUE_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : GREEN_LED_Pin ORANGE_LED_Pin RED_LED_Pin BLUE_LED_Pin */
  GPIO_InitStruct.Pin = GREEN_LED_Pin|ORANGE_LED_Pin|RED_LED_Pin|BLUE_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}



void CopyAppToUserMemory(void)
{
	uint32_t appTailSize,appBodySize,appAddrPointer,j;
	appTailSize = 1024;

	HAL_FLASH_Unlock();

	debug("function: copy use code ....\r\n app size:");
	sprintf(msg,"%d",(int)appSize);
	debug(msg);
	debug("\r\n");

	f_lseek(&myFile, 0); //Go to the fist position of file
	appTailSize = appSize % APP_BLOCK_TRANSFER_SIZE;
	appBodySize = appSize - appTailSize;
	appAddrPointer = 0;


	debug("appTailSize :");
	sprintf(msg,"%d",(int)appTailSize);
	debug(msg);
	debug("\r\n");

	debug("appBodySize :");
	sprintf(msg,"%d",(int)appBodySize);
	debug(msg);
	debug("\r\n");

	uint32_t endAddr = FLASH_USER_START_ADDR + appSize;

	if(FLASH_If_EraseSectors(endAddr) != 0x00)
	{
		while(1) {debug("Erase Sector Error: ");}
	}
	else
	{
		debug("erasing flash ..");
	}

	for(i = 0; i < appBodySize; i += APP_BLOCK_TRANSFER_SIZE)
	{
		/*
		 * For example, size of File1 = 1030 bytes
		 * File1 = 2 * 512 bytes + 6 bytes
		 * "body" = 2 * 512, "tail" = 6
		 * Let's write "body" and "tail" to MCU FLASH byte after byte with 512-byte blocks
		 */
		f_read(&myFile, appBuffer, APP_BLOCK_TRANSFER_SIZE, &readBytes); //Read 512 byte from file
		for(j = 0; j < APP_BLOCK_TRANSFER_SIZE; j += SIZE_OF_U32) //write 512 byte to FLASH
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FLASH_USER_START_ADDR + i + j, *((volatile uint32_t*)(appBuffer + j))) !=0)
			{
				while(1) {debug("Flash body program Error ");}
			}
		}
		appAddrPointer += APP_BLOCK_TRANSFER_SIZE; //pointer to current position in FLASH for write
	}

	f_read(&myFile, appBuffer, appTailSize, &readBytes); //Read "tail" that < 512 bytes from file
	while((appTailSize % SIZE_OF_U32) != 0)		//if appTailSize MOD 4 != 0 (seems not possible, but still...)
	{
		appTailSize++;				//increase the tail to a multiple of 4
		appBuffer[appTailSize - 1] = 0xFF;	//and put 0xFF in this tail place
	}

	for(i = 0; i < appTailSize; i += SIZE_OF_U32) //write "tail" to FLASH
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FLASH_USER_START_ADDR + appAddrPointer + i, *((volatile uint32_t*)(appBuffer + i))) !=0)
		{
			while(1) {debug("Flash tail program Error ");}
		}
	}
	//FLASH_WaitForLastOperation(100);
	HAL_FLASH_Lock();
}

void debug(char *text)
{
	HAL_UART_Transmit(&huart3,(uint8_t *)text,strlen(text),100);
}
void LED(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin,uint8_t PinState)
{
	  switch (PinState)
	  {
	  case LED_SET:    HAL_GPIO_WritePin(GPIOx,GPIO_Pin,GPIO_PIN_SET); break;
	  case LED_RESET:  HAL_GPIO_WritePin(GPIOx,GPIO_Pin,GPIO_PIN_RESET); break;
	  case LED_TOGGLE: HAL_GPIO_TogglePin(GPIOx,GPIO_Pin); break;
	  }
}

void Error_Handler(void)
{
	__disable_irq();
	while (1)
	{
	}
}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{

}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
