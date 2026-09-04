#include "usbBoot.h"
#include "serial.h"
#include "flash_if.h"
#include "usb_host.h"
#include "crc32.h"

typedef  void (*pFunction)(void);
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t appSize;
uint8_t appBuffer[512];

FIL myFile;
FRESULT fRes;
UINT byteswritten, bytesread;

unsigned int i;
unsigned int  readBytes;

// jump to the application
void jumpcode()
{
	uint32_t appStackAddress;
	uint32_t appResetHandler;

	appStackAddress = *(volatile uint32_t*) FLASH_USER_START_ADDR;
	appResetHandler = *(volatile uint32_t*) (FLASH_USER_START_ADDR + 4);

	// Check if there is valid code in flash (Stack pointer pointing to SRAM, 128K of SRAM)
	//if ((appStackAddress & 0x2FFD0000) == 0x20000000)
	if ((appStackAddress >= 0x20000000UL) && (appStackAddress <=  0x20020000UL))
	{
		// Jump to user application
		JumpToApplication = (pFunction) appResetHandler;

		// Reset peripherals
		DeInit_Usb();
		HAL_UART_DeInit(&huart3);
		HAL_DeInit();
		HAL_RCC_DeInit();

		/*
		 * Stop Systick and NVIC interruptsi
		 * it is recommended to disable glabal irq, then in application code must enable it,
		 * otherwise interrupt base operation will not work
		 */
		__disable_irq();
		SysTick->CTRL = 0;  // Disable SysTick
		SysTick->VAL = 0;   // Reset current value
		SysTick->LOAD = 0;  // Reset reload value

		for (uint8_t i=0; i<8; i++)
		{
			NVIC->ICER[i]= 0xffffffffU;//Disable all interrupts
			NVIC->ICPR[i]= 0xffffffffU;//Clear all pending interrupts
		}

		SCB->VTOR = FLASH_USER_START_ADDR;
		/*
		 *BOOT start from the default but for application provide the offset
		 *SCB -> System control block register(ARM cortex reg.)
		 *VTOR-> vector table offset register
		 */
		__set_MSP(*(volatile uint32_t*) FLASH_USER_START_ADDR);// Initialize user application's Stack Pointer

		JumpToApplication();
	}
}

void CopyAppToUserMemory(void)
{
	uint32_t appTailSize,appBodySize,appAddrPointer,j;
	appTailSize = 1024;

	HAL_FLASH_Unlock();

	f_lseek(&myFile, HEADER_LENGTH); //pointer to start of application
	if (f_lseek(&myFile, HEADER_LENGTH) != FR_OK)
	{
	    serial.println("Application Seek Error!"); return;
	}
	appTailSize = appSize % APP_BLOCK_TRANSFER_SIZE;
	appBodySize = appSize - appTailSize;
	appAddrPointer = 0;

	serial.print("AppBodySize :"); serial.println((int)appBodySize);
	serial.print("AppTailSize :"); serial.println((int)appTailSize);

	uint32_t startAddress = FLASH_USER_START_ADDR;
	uint32_t endAddress = FLASH_USER_START_ADDR + appSize;

	if(FLASH_If_EraseSectors(startAddress,endAddress) != 0x00)
	{
		serial.println("Erase Sector Error!!");
		HAL_FLASH_Lock();
	    Error_Handler();
	}
	else
	{
		serial.println("Erasing flash...");
	}
	serial.print("Programming: ");

	for (i = 0; i < appBodySize; i += APP_BLOCK_TRANSFER_SIZE)
	{
		/*
		 * For example, size of File1 = 1030 bytes
		 * File1 = 2 * 512 bytes + 6 bytes
		 * "body" = 2 * 512, "tail" = 6
		 * Let's write "body" and "tail" to MCU FLASH byte after byte with 512-byte blocks
		 */
		if(f_read(&myFile, appBuffer, APP_BLOCK_TRANSFER_SIZE, &readBytes) != FR_OK)//Read 512 byte from file
		{
			serial.print("AppBody Read Error! ");
			f_close(&myFile);
			HAL_FLASH_Lock();
			Error_Handler();
		}
		for(j = 0; j < APP_BLOCK_TRANSFER_SIZE; j += SIZE_OF_U32) //write 512 byte to FLASH
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FLASH_USER_START_ADDR + i + j, *((volatile uint32_t*)(appBuffer + j))) !=HAL_OK)
			{
				serial.println("Flash body program Error!!");
				HAL_FLASH_Lock();
				Error_Handler();
			}
		}
		appAddrPointer += APP_BLOCK_TRANSFER_SIZE; //pointer to current position in FLASH for write
	}

	if(f_read(&myFile, appBuffer, appTailSize, &readBytes) != FR_OK) //Read "tail" that < 512 bytes from file
	{
		serial.print("AppTail Read Error! ");
		f_close(&myFile);
		HAL_FLASH_Lock();
		Error_Handler();

	}
	while((appTailSize % SIZE_OF_U32) != 0)		//if appTailSize MOD 4 != 0 (seems not possible, but still...)
	{
		appTailSize++;				//increase the tail to a multiple of 4
		appBuffer[appTailSize - 1] = 0xFF;	//and put 0xFF in this tail place
	}

	for(i = 0; i < appTailSize; i += SIZE_OF_U32) //write "tail" to FLASH
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FLASH_USER_START_ADDR + appAddrPointer + i, *((volatile uint32_t*)(appBuffer + i))) !=HAL_OK)
		{
			serial.println("Flash tail program Error!!");
			HAL_FLASH_Lock();
	        Error_Handler();
		}
	}

	serial.println("");
	//FLASH_WaitForLastOperation(100);
	HAL_FLASH_Lock();
}

void copyHeaderToMemory()
{
	if (f_lseek(&myFile, 0) != FR_OK)
	{
	    serial.println("Header Seek Error!"); return;
	}

	if(f_read(&myFile, appBuffer, HEADER_LENGTH, &readBytes) != FR_OK)
	{
		serial.print("Header Read Error! ");
		f_close(&myFile);
		Error_Handler();
	}

    if (memcmp(appBuffer, (void *)FLASH_RESERVED_START_ADDR,HEADER_LENGTH) != 0)
    {
    	HAL_FLASH_Unlock();

    	uint32_t startAddress = FLASH_RESERVED_START_ADDR;
		uint32_t endAddress = FLASH_RESERVED_START_ADDR + HEADER_LENGTH;

		if (FLASH_If_EraseSectors(startAddress,endAddress) != 0x00)
		{
			serial.println("Erase Sector Error!!");
			HAL_FLASH_Lock();
			Error_Handler();

		}

		for (i = 0; i < HEADER_LENGTH; i += SIZE_OF_U32) //write "header" to FLASH
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FLASH_RESERVED_START_ADDR + i,*((volatile uint32_t*) (appBuffer + i))) != HAL_OK)
			{
				serial.println("Header programming error!");
				HAL_FLASH_Lock();
	            Error_Handler();
			}
		}
		HAL_FLASH_Lock();
		serial.println("Header programmed successfully.");
	}
}

void parseHeader(AppHeader_t *h)
{
	if (f_lseek(&myFile, 0) != FR_OK)
	{
	    serial.println("Header Seek Error!"); return;
	}

    if (f_read(&myFile, appBuffer, HEADER_LENGTH, &readBytes) != FR_OK || readBytes != HEADER_LENGTH)
    {
        serial.println("Application header read error!");
        f_close(&myFile);
        Error_Handler();
    }

    h->magicNumber = ((uint32_t)appBuffer[0]) | ((uint32_t)appBuffer[1] << 8) | ((uint32_t)appBuffer[2] << 16) | ((uint32_t)appBuffer[3] << 24);
    h->version     = ((uint32_t)appBuffer[4]) | ((uint32_t)appBuffer[5] << 8) | ((uint32_t)appBuffer[6] << 16) | ((uint32_t)appBuffer[7] << 24);
    h->imageSize   = ((uint32_t)appBuffer[8]) | ((uint32_t)appBuffer[9] << 8) | ((uint32_t)appBuffer[10] << 16)| ((uint32_t)appBuffer[11] << 24);
    h->app_crc     = ((uint32_t)appBuffer[12])| ((uint32_t)appBuffer[13] << 8)| ((uint32_t)appBuffer[14] << 16)| ((uint32_t)appBuffer[15] << 24);

    serial.print("Magic No: 0x"); serial.printlnHex(h->magicNumber);
    serial.print("Version: ");    serial.println((int)h->version);
    serial.print("Image Size: "); serial.println((int)h->imageSize);
    serial.print("APP_CRC: 0x");  serial.printlnHex(h->app_crc);

}

uint32_t calCrc()
{
	uint32_t cal_crc = 0;

	cal_crc = crc32((const uint8_t*) FLASH_USER_START_ADDR, appSize);
	serial.print("CAL_CRC: 0x");
	serial.printlnHex(cal_crc);

	return cal_crc;
}

void flashEraseApplication(void)
{
    HAL_FLASH_Unlock();

	uint32_t startAddress = FLASH_USER_START_ADDR;
	uint32_t endAddress = FLASH_USER_START_ADDR + appSize;

	if (FLASH_If_EraseSectors(startAddress,endAddress) != 0x00)
	{
		serial.println("Erase Sector Error!!");
		HAL_FLASH_Lock();
		Error_Handler();
	}

    HAL_FLASH_Lock();
}

uint8_t validStoredApp(void)
{
#ifdef USE_APPLICATION_HEADER

    AppHeader_t storedHeader;
    uint32_t cal_crc;
    uint32_t appStackAddress;
    appStackAddress = *(volatile uint32_t*) FLASH_USER_START_ADDR;

	if ((appStackAddress & 0x2FFD0000) != 0x20000000)
	{
		serial.println("No valid application found"); return 1;
	}
    //Read header from reserved Flash
    storedHeader.magicNumber = *(volatile uint32_t *)(FLASH_RESERVED_START_ADDR + 0);
    storedHeader.version     = *(volatile uint32_t *)(FLASH_RESERVED_START_ADDR + 4);
    storedHeader.imageSize   = *(volatile uint32_t *)(FLASH_RESERVED_START_ADDR + 8);
    storedHeader.app_crc     = *(volatile uint32_t *)(FLASH_RESERVED_START_ADDR + 12);

    /* Header validation */
    if (storedHeader.magicNumber != MAGIC_NUMBER)
    {
        serial.println("Invalid application magic!"); return 2;
    }

    if (storedHeader.imageSize == 0 || storedHeader.imageSize > FLASH_USER_SIZE)
    {
        serial.println("Invalid application size!"); return 3;
    }

    //Calculate CRC from actual application in Flash
    cal_crc = crc32((const uint8_t *)FLASH_USER_START_ADDR,storedHeader.imageSize);
    if (cal_crc != storedHeader.app_crc)
    {
        serial.println("Stored application CRC mismatch!"); return 4;
    }

    serial.println("Stored application is valid.");
    return 0;
#else
    uint32_t appStackAddress;
    appStackAddress = *(volatile uint32_t*) FLASH_USER_START_ADDR;

	if ((appStackAddress & 0x2FFD0000) != 0x20000000)
	{
		serial.println("No valid application found"); return 1;
	}
    return 0;
#endif
}


void PrintProgrammingProgress(uint32_t currentBytes, uint32_t totalBytes)
{
    uint32_t percentage;

    if (totalBytes == 0) return;
    percentage = (currentBytes * 100U) / totalBytes;
    serial.print((int)percentage);serial.print("% .. ");
}
