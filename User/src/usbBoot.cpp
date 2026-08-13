#include "usbBoot.h"
#include "serial.h"
#include "flash_if.h"

typedef  void (*pFunction)(void);
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t appSize;
uint8_t appBuffer[512];

FIL myFile;
FRESULT res;
UINT byteswritten, bytesread;

unsigned int i;
unsigned int  readBytes;

void jumpcode()
{// jump to the application

	if (((*(__IO uint32_t*)FLASH_USER_START_ADDR) & 0x2FFD0000 ) == 0x20000000)
		  {
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
}

void CopyAppToUserMemory(void)
{
	uint32_t appTailSize,appBodySize,appAddrPointer,j;
	appTailSize = 1024;

	HAL_FLASH_Unlock();

	serial.println("Copy application code...");

	f_lseek(&myFile, 0); //Go to the fist position of file
	appTailSize = appSize % APP_BLOCK_TRANSFER_SIZE;
	appBodySize = appSize - appTailSize;
	appAddrPointer = 0;

	serial.print("AppBodySize :"); serial.println((int)appBodySize);
	serial.print("AppTailSize :"); serial.println((int)appTailSize);

	uint32_t endAddr = FLASH_USER_START_ADDR + appSize;

	if(FLASH_If_EraseSectors(endAddr) != 0x00)
	{
		while(1) {serial.println("Erase Sector Error!!");}
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
		f_read(&myFile, appBuffer, APP_BLOCK_TRANSFER_SIZE, &readBytes); //Read 512 byte from file
		for(j = 0; j < APP_BLOCK_TRANSFER_SIZE; j += SIZE_OF_U32) //write 512 byte to FLASH
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FLASH_USER_START_ADDR + i + j, *((volatile uint32_t*)(appBuffer + j))) !=0)
			{
				while(1) {serial.println("Flash body program Error!!");}
			}
		}
		appAddrPointer += APP_BLOCK_TRANSFER_SIZE; //pointer to current position in FLASH for write

		PrintProgrammingProgress(i + APP_BLOCK_TRANSFER_SIZE, appSize);
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
			while(1) {serial.println("Flash tail program Error!!");}
		}
	}
	PrintProgrammingProgress(appSize, appSize);
	serial.println("");
	//FLASH_WaitForLastOperation(100);
	HAL_FLASH_Lock();
}

bool IsApplicationValid(void)
{
    uint32_t appStackAddress;
    uint32_t appResetHandler;

    appStackAddress = *(__IO uint32_t*)FLASH_USER_START_ADDR;
    appResetHandler = *(__IO uint32_t*)(FLASH_USER_START_ADDR + 4);

    //Check application's initial stack pointer
    if ((appStackAddress & 0x2FFD0000U) != 0x20000000U)  return false;
    //Check application's reset handler address
    if ((appResetHandler < FLASH_USER_START_ADDR) || (appResetHandler >= FLASH_LAST_ADDR))  return false;

    return true;
}

void PrintProgrammingProgress(uint32_t currentBytes, uint32_t totalBytes)
{
    uint32_t percentage;

    if (totalBytes == 0) return;
    percentage = (currentBytes * 100U) / totalBytes;
    serial.print((int)percentage);serial.print("% .. ");
}
