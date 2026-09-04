#ifndef INC_BOOTCONFIG_H_
#define INC_BOOTCONFIG_H_

#define USE_APPLICATION_HEADER

#ifdef USE_APPLICATION_HEADER
#define HEADER_LENGTH 16
#else
#define HEADER_LENGTH 0
#endif

#define MAGIC_NUMBER       0x46574D47UL
#define APP_VERSION        1

typedef struct __attribute__((packed))
{
    uint32_t magicNumber;
    uint32_t version;
    uint32_t imageSize;
    uint32_t app_crc;
} AppHeader_t;


/*******************************************************************/
//For STM32F407
#define FLASH_START_ADDR	0x08000000UL//Origin
#define FLASH_MAX_SIZE      0x00100000UL// 1 MB Flash

#define FLASH_END_ADDR		 (FLASH_START_ADDR + FLASH_MAX_SIZE) //FLASH end address
#define FLASH_LAST_ADDR      (FLASH_END_ADDR - 1)

/* Bootloader: Sectors 0-3 "16x4 => 64K" */
#define FLASH_BOOT_START_ADDR    (FLASH_START_ADDR)//Bootloader start address
#define FLASH_BOOT_SIZE		      0x00010000 //64 Kbyte for bootloader

/*
 * Reserved: Sector 4
 * currently using to store application header
 */
#define FLASH_RESERVED_START_ADDR  0x08010000UL
#define FLASH_RESERVED_SIZE        0x00010000UL // 64 KB

/*Application: From Sector 5*/
#define FLASH_USER_START_ADDR	  0x08020000UL	//User application start address
#define FLASH_USER_SIZE		      (FLASH_END_ADDR - FLASH_USER_START_ADDR)

/*
 * ============================================================================
 * STM32F407 Flash Memory Map
 * ============================================================================
 *
 * 0x08000000  +---------------------------+
 *             |                           |
 *             | Bootloader                |
 *             | 64 KB                     |
 *             | Sectors 0 - 3             |
 * 0x08010000  +---------------------------+
 *             |                           |
 *             | Reserved / App Header     |
 *             | 64 KB                     |
 *             | Sector 4                  |
 * 0x08020000  +---------------------------+
 *             |                           |
 *             | Application               |
 *             | 896 KB                    |
 *             | Sectors 5 - 11            |
 *             |                           |
 * 0x08100000  +---------------------------+
 *
 * Application Flash Start = 0x08020000
 * Application Flash End   = 0x080FFFFF
 * Application Size        = 896 KB
 *
 * ============================================================================
 */

#endif /* INC_BOOTCONFIG_H_ */
