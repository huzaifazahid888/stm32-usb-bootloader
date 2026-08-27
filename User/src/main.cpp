#include "main.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "gpio.h"
#include "clock.h"
#include "uart.h"
#include "serial.h"
#include "bootConfig.h"
#include "usb_host.h"
#include "usbBoot.h"
#include "flash_if.h"
#include "crc32.h"

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

	bool mountCheck=false;;
	uint32_t offset = 0;

#ifdef	USE_APPLICATION_HEADER  //define in bootconfig.
	uint32_t magicNumber = 0;
	uint32_t version = 0;
	uint32_t imageSize = 0;
	uint32_t app_crc = 0;
	uint32_t cal_crc = 0;
#endif

	serial.println("#....STM32f4 USB BootLoader....#");
	usbInsrtWaitTime = HAL_GetTick();

	serial.println("...Programming Mode...");

	while ((HAL_GetTick() - usbInsrtWaitTime) < 3000) // wait 3s for usb pendrive
	{
		MX_USB_HOST_Process();

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
		    usbDetected = true;
		    /*
		     * File name on the usb must be same
		     * file name must not exceed 8 characters
		     */
		    fRes = f_open(&myFile, "crc.bin", FA_READ);

		    if (fRes == FR_OK)
		    {
		        serial.println("File opened...");

		        appSize = f_size(&myFile) - HEADER_OFFSET;

		        serial.print("File size : ");
		        serial.println((int)appSize);

#ifdef USE_APPLICATION_HEADER

		        /*
		         * Read the 16-byte application header first, if application header is uncommented in config. file
		         */
		        if (f_read(&myFile, appBuffer, 16, &readBytes) != FR_OK || readBytes != 16)
		        {
		            serial.println("Application header read error!");
		            f_close(&myFile);
		            break;
		        }

		        magicNumber = ((uint32_t)appBuffer[0]) | ((uint32_t)appBuffer[1] << 8) | ((uint32_t)appBuffer[2] << 16) | ((uint32_t)appBuffer[3] << 24);
		        version     = ((uint32_t)appBuffer[4]) | ((uint32_t)appBuffer[5] << 8) | ((uint32_t)appBuffer[6] << 16) | ((uint32_t)appBuffer[7] << 24);
		        imageSize   = ((uint32_t)appBuffer[8]) | ((uint32_t)appBuffer[9] << 8) | ((uint32_t)appBuffer[10] << 16)| ((uint32_t)appBuffer[11] << 24);
		        app_crc     = ((uint32_t)appBuffer[12])| ((uint32_t)appBuffer[13] << 8)| ((uint32_t)appBuffer[14] << 16)| ((uint32_t)appBuffer[15] << 24);

		        serial.print("Magic No: 0x");   serial.printlnHex(magicNumber);
		        serial.print("Version: ");    serial.println((int)version);
		        serial.print("Image Size: "); serial.println((int)imageSize);
		        serial.print("APP_CRC: 0x");  serial.printlnHex(app_crc);


		        if ((magicNumber != MAGIC_NUMBER) ||(version != APP_VERSION) || (imageSize != (appSize )))  //Validate header.
		        {
		            serial.println("Application Header is not correct!");
		            f_close(&myFile); break;
				}
		        while(uartTrans(&huart3) !=1 );
#endif

		        offset = 0;
		        while (1)
		        {
		            if (f_read(&myFile, appBuffer, 512, &readBytes) == FR_OK)
		            {
		                if (readBytes == 0)
		                {
		                    firmwareIdentical = true;    break;
		                }

		                if (memcmp(appBuffer, (void *)(FLASH_USER_START_ADDR + offset), readBytes) != 0)
		                {
		                    serial.println("Copying bin file into memory....");
		                    CopyAppToUserMemory();
		                    serial.println("Firmware Upgraded Successfully!!!!");
		                    updatePerformed = true;
		                    break;
		                }

		                offset += readBytes;
		            }
		            else
		            {
		                serial.println("File read error!!"); break;
		            }
		        }

#ifdef USE_APPLICATION_HEADER
		        /*
		         * crc validation
		         */
		        cal_crc = crc32((const uint8_t *)FLASH_USER_START_ADDR, appSize);
		    	serial.print("CAL_CRC: 0x");   serial.printlnHex(cal_crc);

		    	if (cal_crc != app_crc)
		    	{
		    		flashEraseApplication();  break;
		    	}
#endif

		        f_close(&myFile);
		        serial.println("Closing file...");
		    }
		    else
		    {

		        serial.print("f_open failed. Error code = ");
		        serial.println((int)fRes);//file error
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

void validateCrc()
{

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
