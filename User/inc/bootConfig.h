#ifndef INC_BOOTCONFIG_H_
#define INC_BOOTCONFIG_H_

#define USE_APPLICATION_HEADER

#ifdef USE_APPLICATION_HEADER
#define HEADER_OFFSET 16
#else
#define HEADER_OFFSET 0
#endif

#define MAGIC_NUMBER       0x46574D47UL
#define APP_VERSION        1


/*******************************************************************/
#define FLASH_PAGE_SIZE		2048 //2 Kbyte per page
#define FLASH_START_ADDR	0x08000000//Origin
#define FLASH_MAX_SIZE      0x00100000// 1 MB Flash

#define FLASH_END_ADDR		   (FLASH_START_ADDR + FLASH_MAX_SIZE)	//FLASH end address
#define FLASH_BOOT_START_ADDR  (FLASH_START_ADDR)//Bootloader start address
#define FLASH_BOOT_SIZE		    0x00020000//64 Kbyte for bootloader

#define FLASH_USER_START_ADDR	(FLASH_BOOT_START_ADDR + FLASH_BOOT_SIZE)	//User application start address

#define FLASH_LAST_ADDR      (FLASH_END_ADDR - 1)
#define FLASH_USER_SIZE		 (FLASH_LAST_ADDR - FLASH_USER_START_ADDR + 1)



#endif /* INC_BOOTCONFIG_H_ */
