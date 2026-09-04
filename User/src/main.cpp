#include "main.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "gpio.h"
#include "clock.h"
#include "uart.h"
#include "serial.h"
#include "usbBoot.h"
#include "usb_host.h"
#include "flash_if.h"

FATFS myUsbFatFS;
extern char USBHPath[4];   /* USBH logical drive path */
extern ApplicationTypeDef Appli_state;

bool usbDetected = false;
bool updatePerformed = false;
bool firmwareIdentical = false;

Serial serial;
AppHeader_t appHeader;

uint32_t usbInsrtWaitTime;

void sysInit();

int main(void)
{
	sysInit();

	bool mountCheck=false;;
	uint32_t offset = 0;

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
		    fRes = f_open(&myFile, "app_crc.bin", FA_READ);

		    if (fRes == FR_OK)
		    {
		        serial.println("File opened...");

		        appSize = f_size(&myFile) - HEADER_LENGTH;

		        serial.print("File size : ");
		        serial.println((int)appSize);

#ifdef USE_APPLICATION_HEADER //define in bootconfig. file

		        /*
		         * Read the 16-byte application header first, if use application header is uncommented in config. file
		         */
		        parseHeader(&appHeader);

		        if ((appHeader.magicNumber != MAGIC_NUMBER) ||(appHeader.version != APP_VERSION) || (appHeader.imageSize != (appSize )))  //Validate header.
		   		{
					serial.println("Application Header incorrect!");
					f_close(&myFile); break;
		   		}

		    	copyHeaderToMemory();
		    	uartTransCmplt();
#endif
		        f_lseek(&myFile, HEADER_LENGTH);

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
		                    updatePerformed = true;
		                    serial.println("Firmware Upgraded Successfully!!!!");
		                    break;
		                }

		                offset += readBytes;
		            }
		            else
		            {
		                serial.println("File read error!!"); break;
		            }
		        }

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

#ifdef USE_APPLICATION_HEADER
			if (calCrc() != appHeader.app_crc) //crc validation
			{
			    serial.println("CRC failed!");
			    serial.println("Erasing invalid application...");

				flashEraseApplication();
				updatePerformed = false; break;
			}
#endif

	        f_close(&myFile);
	        serial.println("Closing file...");

			f_mount(NULL, "", 1);
			serial.println("System Reseting.........");
			uartTransCmplt();
			HAL_Delay(300);
			jumpcode();
		}
		if (firmwareIdentical)
		{
			serial.println("Firmware identical Jumping to existing application");
			f_mount(NULL, "", 1);
			serial.println("System Reseting.........");
			HAL_Delay(300);
			uartTransCmplt();

			jumpcode();
		}

	}

	if (!usbDetected)
	{
		serial.println("No USB detected");
		if (validStoredApp()==0)
		{
			serial.println("Jumping to store application!");
			uartTransCmplt();

			jumpcode();
		}
	}

	serial.println(".....running wait loop.....");

	while (1)
	{
		uartTrans(&huart3);
		MX_USB_HOST_Process();


#ifdef SOFT_RESET_ON

		if (Appli_state == APPLICATION_START)
		{
			if (f_mount(&myUsbFatFS, (TCHAR const*) USBHPath, 0) == FR_OK)
			{
				serial.println("soft reset");
				uartTransCmplt();
				NVIC_SystemReset();
			}
		}
#endif

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

void uartTransCmplt()
{
	while (uartTrans(&huart3) != 1);
}

void Error_Handler(void)
{
	__disable_irq();
	uartTransCmplt();
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

