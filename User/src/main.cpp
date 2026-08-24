#include "main.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "gpio.h"
#include "clock.h"
#include "uart.h"
#include "serial.h"
#include "usb_host.h"
#include "usbBoot.h"
#include "flash_if.h"

FATFS myUsbFatFS;
extern char USBHPath[4];   /* USBH logical drive path */
extern ApplicationTypeDef Appli_state;

bool usbDetected = false;
bool updatePerformed = false;
bool firmwareIdentical = false;

Serial serial;
uint32_t usbInsrtWaitTime;

void sysInit();

int main(void)
{
	sysInit();

	serial.println("#....STM32f4 USB BootLoader....#");
	usbInsrtWaitTime = HAL_GetTick();

	serial.println("...Programming Mode...");
	bool mountCheck;

	mountCheck = false;

	while ((HAL_GetTick() - usbInsrtWaitTime) < 3000) // wait 3s for usb pendrive
	{
		MX_USB_HOST_Process();
		uint32_t offset = 0;
		switch (Appli_state)
		{
		case APPLICATION_IDLE:  break;

		case APPLICATION_START:
			if (f_mount(&myUsbFatFS, (TCHAR const*) USBHPath, 0) != FR_OK)
			{
				if (mountCheck == false)
				{
					serial.println("Failed to mount!!");
					mountCheck = true;
				}
			}
			else
			{
				if (mountCheck == false)
				{
					serial.println("Usb mounted...");
					mountCheck = true;
				}
			}
			break;

		case APPLICATION_READY:
			usbDetected=true;

			if (f_open(&myFile, "BLINK.BIN", FA_READ) == FR_OK)
			{
				serial.println("File opened...");
				appSize = f_size(&myFile);

				serial.print("File size : "); serial.println((int)appSize);

				while (1)
				{
					if (f_read(&myFile, appBuffer, 512, &readBytes) != FR_OK)
					{
						serial.println("File read error!!");    break;
					}

					if (readBytes == 0)// end of file no mismatch found
					{
						firmwareIdentical = true; break;
					}
					if(memcmp(appBuffer,(void*)(FLASH_USER_START_ADDR + offset), readBytes) != 0)
					{
						serial.println("Copying bin file in to memory....");
						CopyAppToUserMemory();
						serial.println("Firmware Upgraded Successfully!!!! ");
						updatePerformed = true;   break;
					}
					offset += readBytes;
				}
				f_close(&myFile);
				serial.println("Closing file...");
			}
			else
			{
				serial.println("Error opening file!!");
			}
			break;

		case APPLICATION_DISCONNECT: // if the pendrive is in disconnected state
			serial.println("Disconnecting drive...");     break;
		}

		if (updatePerformed)
		{
			serial.println("Exiting programming mode...");
			f_mount(NULL, "", 1);
			serial.println("System Reseting.........");
			HAL_Delay(400);
			while(uartTrans(&huart3) !=1 );

			jumpcode();  break;
		}
		if (firmwareIdentical)
		{
			serial.println("Firmware identical Jumping to existing application");
			f_mount(NULL, "", 1);
			serial.println("System Reseting.........");
			HAL_Delay(400);
			while(uartTrans(&huart3) !=1 );

			jumpcode();  break;

		}

	}

	if (!usbDetected)
	{
		if (IsApplicationValid())
		{
		serial.println("No USB detected, jumping to application!");
		while(uartTrans(&huart3) !=1 );

		jumpcode();
		}
	    else
	    {
	        serial.println("No valid application found!");
	    }
	}

	serial.println(".....running wait loop.....");
	while(uartTrans(&huart3) !=1 );

	while (1)
	{
		HAL_Delay(500);
		MX_USB_HOST_Process();
	}
}

void sysInit()
{
	HAL_Init();
	SystemClock_Config();
	GPIO_Init();
	MX_FATFS_Init();
	MX_USB_HOST_Init();
	initDebugUart();
	serial.init(&huart3);
	FLASH_If_Init();

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
